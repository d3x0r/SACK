#define DO_LOGGING
#include <stdhdrs.h>
#include <idle.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#define ZLIB_DLL
// zlib has a CRC...
#include <zlib/zlib.h>
#ifdef _WIN32
#include <direct.h> // directory junk
#include <io.h>
#else
#include <unistd.h>
#endif
#ifdef __LINUX__
#include <ctype.h>
//#include <linux/fcntl.h>
#else
#include <fcntl.h>
#endif

#include <string.h>

//#define DO_LOGGING

#include <configscript.h>
#include <sharemem.h>
#include <logging.h>
#include <network.h>
#include <timers.h>
#include <filesys.h>
//#include <futcrc.h>
#include "relay.h"
#include "accstruc.h"
#include "account.h"

#include "global.h"

static FILE *logfile;
//SOCKADDR *server;
//char defaultlogin[128];
extern int maxconnections;
extern int bForceLowerCase;
extern int bDone;
#define FILEPERMS

static void ProcessLocalVerifyCommands( PACCOUNT account );
static void ProcessLocalUpdateFailedCommands( PACCOUNT account );

//-------------------------------------------------------------------------

int SendFileChange( PACCOUNT account, PCLIENT pc, uint32_t PathID, uint32_t ID, char *file, uint32_t start, uint32_t length )
{
	// char *data;
	int thisread;
	INDEX hFile = -1;
	PNETWORK_STATE pns = (PNETWORK_STATE)GetNetworkLong( pc, 0 );
	PCLIENT_CONNECTION pcc = pns->client_connection;
	CTEXTSTR attempted_name = NULL;
	if( length )
	{
		if( file )
			hFile = open( attempted_name=file, O_RDONLY|O_BINARY );              // open for reading
		else
		{
			PDIRECTORY pDir = (PDIRECTORY)GetLink( &account->Directories, PathID );
			PFILE_INFO pFileInfo = pDir?(PFILE_INFO)GetLink( &pDir->files, ID ):NULL;
			if( pFileInfo )
				hFile = open( attempted_name =pFileInfo->full_name, O_RDONLY|O_BINARY );
		}

		//xlprintf(2100)( "open of %s = %p", file, hFile );
		if( hFile != -1 )
		{
			uint32_t extra = 0;
			uint32_t *msg;
			msg = (uint32_t*)Allocate( length + sizeof( uint32_t[5]) );
			if( pns->longbuffer )
			{
				// a new buffer shouldn't be requested untli this buffer has been received; so
				// this is safe to release here instead of having a writecomplete callback.
				Release( pns->longbuffer );
				pns->longbuffer = NULL;
			}
			pns->longbuffer = msg;
			msg[0] = *(uint32_t*)"FDAT";
			msg[1] = start;
			msg[2] = length;
			lseek( hFile, start, SEEK_SET );
			if( pcc->version >= VER_CODE( 3, 2 ) )
			{
				extra = sizeof( uint32_t[5] );
				msg[3] = PathID;
				msg[4] = ID;
				thisread = read( hFile, msg+5, length );
			}
			else
			{
				extra = sizeof( uint32_t[3] );
				thisread = read( hFile, msg+3, length );
			}
			msg[2] = thisread;
			xlprintf( 2200 )( "read of %d = %d", length, thisread );
			close( hFile );
			pcc->flags.bUpdated = 1;
			//xlprintf(2100)( "Send TCP Long %d", thisread + extra );
			SendTCPLong( pc, msg, thisread + extra );
			//xlprintf(2100)( "Done TCP Long" );
		}
		else
		{
			xlprintf(2100)( "Failed to open file %s", attempted_name?attempted_name:"<NoName>" );
			return FALSE; // resulting false will terminate the connection.
		}
	}
	else
	{
		uint32_t msg[5];
		msg[0] = *(uint32_t*)"FDAT";
		msg[1] = start;
		msg[2] = 0;
		if( pcc->version >= VER_CODE( 3, 2 ) )
		{
			msg[3] = PathID;
			msg[4] = ID;
			SendTCP( pc, msg, 20 );
		}
		else
		{
			SendTCP( pc, msg, 12 );
		}
	}
	return TRUE;
}

//-------------------------------------------------------------------------

int CheckDirectoryOnAccount( PACCOUNT account
                             , PDIRECTORY pDir
                             , char *filename )
{
	char filepath[280];
#ifdef __GNUC__
	struct _stat statbuf;
#else
	struct stat statbuf;
#endif
	if( !pDir->path[0] )
		return 0;
	sprintf( filepath, "%s/%s"
			 , pDir->path
			 , filename );
	if( stat( filepath, &statbuf ) < 0 )
	{
#ifdef _WIN32
		if( mkdir( filepath ) < 0 ) // all permissions?
#else
  		if( mkdir( filepath, -1 ) < 0 ) // all permissions?
#endif
  			xlprintf(LOG_ALWAYS)( "Failed to create directory: %s", filepath );
	}
	else
	{
#ifdef __GNUC__
		if( statbuf.st_mode & _S_IFDIR )
#else
  		if( statbuf.st_mode & S_IFDIR )
#endif
  		{
  			//xlprintf(LOG_ALWAYS)( "Directory Existed" );
  		}
  		else
  		{
  			xlprintf(LOG_ALWAYS)( "File existed in its place - THIS IS BAD!!!!!" );
      }
	}
	//SendTCP( account->pc, &account->NextResponce, 4 );
	return 0;
}

//-------------------------------------------------------------------------

int ReadValidateCRCs( PCLIENT_CONNECTION pcc, uint32_t *crc, size_t crclen
						  , char *name, size_t finalsize
						  , PFILE_INFO pFileInfo
						  )
{
	int matches = 1;
	size_t blocklen = 4096;
	uintptr_t size = 0;
	uint8_t* mem;
	PFILECHANGE pfc = NULL;
   PFILECHANGE pfc_last = NULL;

	if( finalsize == 0 )
	{
		FILE *file = fopen( name, "wb" );
		xlprintf(2100)( "Target file is 0 bytes, let's just create a 0 byte file and set it's time stamps here. (okay it matches.)" );
		if( file )
		{
			fclose( file );
			SetFileTimes( pFileInfo->full_name
							, ConvertFileTimeToInt( &pFileInfo->lastcreate )
							, ConvertFileTimeToInt( &pFileInfo->lastmodified )
							, ConvertFileTimeToInt( &pFileInfo->lastaccess ) );
		}
		return 1;
	}

	mem = (uint8_t*)OpenSpace( NULL, name, &size );
	//lprintf( "Validating CRCs %p %s %ld(%ld) %d", mem, name, size, finalsize, crclen );
	if( mem )
	{
		uint32_t n;
		uint32_t n_start = 0;
		for( n = 0; n < crclen; n++ )
		{
			uint32_t crc_result;
			size_t check_length = blocklen;
			//lprintf( "Checking block: %ld of %ld", n, crclen );
			if( ( finalsize - ( n * blocklen ) ) < blocklen )
				check_length = finalsize - ( n * blocklen );

			if( ( n * blocklen ) > finalsize )
			{
				xlprintf(LOG_ALWAYS)( "fatality - more done than was accounted for in CRCLEN!" );
				Release( mem );
				return 2;
			}
			if( (( n*blocklen) + check_length ) > size )
			{
				// more in real file than our file, so
				// please request all left, and get out.
				if( !pfc )
				{
					pfc = New( FILECHANGE );
               pfc_last = pfc;
					pfc->pFileInfo = pFileInfo;
					pfc->start = n*blocklen;
					xlprintf(2100)( "pfc start: %ld", pfc->start );
				}
				else
					xlprintf(LOG_ALWAYS)( "Invalid CRC continued (length short)" );
				pfc->size = finalsize - pfc->start;
				xlprintf(2100)( "Enque: %ld %ld (%ld)", pfc->start, pfc->size, finalsize );
				if( pcc )
				{
					EnqueLink( &pcc->segments, pfc );
					pcc->segment_total_size += pfc->size;
					pcc->segment_total++;
				}
				pfc = NULL;
				break;
			}

			crc_result = crc32( crc32(0L, NULL, 0), mem + n * blocklen, (uInt)check_length );
      	// need to put a block limit on this also....
			if( crc[n] != crc_result )
			{
				xlprintf(2100)( "block %d manifest_crc:%08x file_crc:%08x", n, crc[n], crc_result );
				matches = 0;
				if( !pfc )
				{
					pfc = New( FILECHANGE );
               pfc_last = pfc;
					pfc->last_block = FALSE;
					pfc->pFileInfo = pFileInfo;
					pfc->start = n * blocklen; // start of this block is bad.
					n_start = n;
					//lprintf( "pfc start: %ld", pfc->start );
				}
				else //- we already marked the start of invalid data...
				{
					if( ( n - n_start ) > 19 )
					{
						pfc->size = ( n * blocklen) - pfc->start;
						xlprintf(2100)( "Enque: %ld %ld (%ld)", pfc->start, pfc->size, finalsize );
						if( pcc )
						{
							EnqueLink( &pcc->segments, pfc );
							pcc->segment_total_size += pfc->size;
							pcc->segment_total++;
						}
						pfc = New( FILECHANGE );
						pfc_last = pfc;
						pfc->last_block = FALSE;
						pfc->pFileInfo = pFileInfo;
						pfc->start = n * blocklen; // start of this block is bad.
						n_start = n;
					}
				}
			}
			else
			{
				if( pfc )
				{
					// up to the beginning of this block...
					pfc->size = ( n * blocklen) - pfc->start;
					xlprintf(2100)( "Enque: %ld %ld (%ld)", pfc->start, pfc->size, finalsize );
					if( pcc )
					{
						EnqueLink( &pcc->segments, pfc );
						pcc->segment_total_size += pfc->size;
						pcc->segment_total++;
					}
					pfc = NULL;
				}
				else
				{
               // no change block , and no change required....
				}
			}
		}
		if( pfc )
		{
			// if we started a change, close it.
			pfc->size = finalsize - pfc->start;
			xlprintf(2100)( "Enque: %ld %ld (%ld)", pfc->start, pfc->size, finalsize );
			if( pcc )
			{
				EnqueLink( &pcc->segments, pfc );
				pcc->segment_total_size += pfc->size;
				pcc->segment_total++;
			}
			pfc = NULL;
		}

		if( pfc_last )
		{
         xlprintf(2100)( "block %p is last.", pfc_last );
			pfc_last->last_block = TRUE;
		}
		else
         xlprintf(2100)( "no last block" );

		CloseSpace( mem );
		
		if( size > finalsize )
		{
			SetFileLength( name, finalsize );
			//SetFileTimes( name, *(uint64_t*)&pFileInfo->lastcreate, *(uint64_t*)&pFileInfo->lastmodified, *(uint64_t*)&pFileInfo->lastmodified );
		}
		if( matches )
			return 1;
		return 3;
	}
	else
	{
		// build list of requests which are nicely sized...
		PFILECHANGE pfc;
		uint32_t n;
		xlprintf(2100)( "Requesting whole file[%s] (in parts) since we know nothing... ", pFileInfo->name );
		for( n = 0; n < (finalsize + ((10*blocklen) - 1)) / (10*blocklen); n++ )
		{
			pfc = New( FILECHANGE );
			pfc_last = pfc;
			pfc->last_block = FALSE;
			pfc->pFileInfo = pFileInfo;
			pfc->start = n * blocklen*10; // start of this block is bad.
			pfc->size = blocklen * 10;
			if( pfc->start + pfc->size > finalsize )
				pfc->size = finalsize - pfc->start;
			//lprintf( "Finalsize: %ld start: %ld size: %ld", finalsize, pfc->start, pfc->size );
			if( pcc )
			{
				xlprintf(2100)( "Enque Link at %d for %d %s", pfc->start, pfc->size, pfc->pFileInfo->full_name );
				EnqueLink( &pcc->segments, pfc );
				pcc->segment_total_size += pfc->size;
				pcc->segment_total++;
			}
		}
		if( pfc_last )
		{
         xlprintf(2100)( "block %p is last.", pfc_last );
			pfc_last->last_block = TRUE;
		}
		else
         xlprintf(2100)( "no last block" );
		return 0;
	}
}

//-------------------------------------------------------------------------

int OpenFileOnAccount( PNETWORK_STATE pns //PACCOUNT account
                     , uint32_t PathID
                     , char *filename
                     , uint32_t size
                     , FILETIME time_create
                     , FILETIME time_modify
                     , FILETIME time_access
                     , uint32_t *crc
                     , uint32_t crclen )
{
	if( pns->account )
	{
		PCLIENT_CONNECTION pcc = pns->client_connection;
		PACCOUNT account = pns->account;
		// struct stat statbuf;
		PDIRECTORY pDir = (PDIRECTORY)GetLink( &account->Directories, PathID );
		if( !pDir )
		{
			xlprintf(LOG_ALWAYS)( "Could not find the directory referenced...%d", PathID );
			SendTCP( pcc->pc, &account->WhatResponce, 4 );
			return 0;
		}
		//lprintf( "Checking file %s in %s", filename, pDir->path );
		EnterCriticalSec( &account->cs );
		if( !account->flags.bLoggedIn )
		{
			xlprintf(LOG_ALWAYS)( "login incomplete." );
			return 0;
		}

		if( bForceLowerCase )
		{
			char *fname = filename;
			for( fname = filename; fname[0]; fname++ )
				fname[0] = tolower( fname[0] );
			xlprintf(2100)( "Lowered case..: %s", filename );
		}
		if( size == 0xFFFFFFFF )
		{
			int result = CheckDirectoryOnAccount( account, pDir, filename );
			//lprintf( "Checked directory %s( size -1 )", filename );
			LeaveCriticalSec( &account->cs );
			return result;
		}
      //lprintf( "file %d", pcc->file );
		if( pcc->file != INVALID_INDEX )
		{
			//lprintf( "Closing existing file before opening a new one... ");
			lseek( pcc->file, size, SEEK_SET );
			set_eof( pcc->file );
			close( pcc->file );
			SetFileTimes( pcc->LastFile, ConvertFileTimeToInt( &time_create ), ConvertFileTimeToInt( &time_modify ), ConvertFileTimeToInt( &time_access ) );
			pcc->file = INVALID_INDEX;
		}

		snprintf( pcc->LastFile, sizeof( pcc->LastFile ) * sizeof( TEXTCHAR ), "%s/%s", pDir->path, filename );
		xlprintf(2100)( WIDE("File: %s size: %d time: %") _64fs
			 , pcc->LastFile, size, time );

		if( pcc->version < VER_CODE( 3, 2 ) )
		{
			if( pcc->buffer )
			{
				Release( pcc->buffer );
				pcc->buffer = NULL;
			}
		}
		// hmm - with this check I don't care about time/date
		// I don't care about existance....

		if( !ReadValidateCRCs( pcc, crc, crclen
									, pcc->LastFile, size, NULL ) )
		{
#ifdef _WIN32
			pcc->file = open( pcc->LastFile, O_RDWR|O_CREAT|O_BINARY );
			xlprintf(2100)( "open file [%s]=%d", pcc->LastFile, pcc->file );
			SetFileAttributes( pcc->LastFile, FILE_ATTRIBUTE_NORMAL   );
#else
			pcc->file = open( pcc->LastFile, O_RDWR|O_CREAT, 0666 );
#endif
			if( pcc->file < 0 )
			{
				xlprintf(2100)( "Failed to open... %s", pcc->LastFile );
				SendTCP( pcc->pc, &account->WhatResponce, 4 );
				LeaveCriticalSec( &account->cs );
				return 0;
			}
			// need to open the file....
		}
		else
		{
#ifdef _WIN32
			//xlprintf(2100)( "open file..." );
			pcc->file = open( pcc->LastFile, O_RDWR|O_BINARY );
			SetFileAttributes( pcc->LastFile, FILE_ATTRIBUTE_NORMAL   );
#else
			pcc->file = open( pcc->LastFile, O_RDWR, 0666 );
#endif
			if( pcc->file < 0 )
			{
				xlprintf(2100)( "Failed to open... %s", pcc->LastFile );
				SendTCP( pcc->pc, &account->WhatResponce, 4 );
				LeaveCriticalSec( &account->cs );
				return 0;
			}
			// ndfseed to open the file....
		}
		//xlprintf(2100)( "And now the file is open: %d", pcc->file );
		{
			PFILECHANGE pfc = (PFILECHANGE)DequeLink( &pcc->segments );
			if( pfc )
			{
				uint32_t msg[5];

				// buffer allocation happens when FDAT comes back.

				//xlprintf(2100)( "Allocate buffer %d", pfc->size );
				//account->buffer = Allocate( pfc->size );
				if( pDir->flags.bVerify )
				{
					do
					{
						Release( pfc );
					}
					while( pfc = (PFILECHANGE)DequeLink( &pcc->segments ) );
					msg[0] = *(uint32_t*)"FAIL";
					SendTCP( pcc->pc, msg, 4 );
				}
				else
				{
					msg[0] = account->SendResponce;
					msg[1] = (uint32_t)pfc->start;  // file position
					msg[2] = (uint32_t)pfc->size;   // length...
               xlprintf(2100)( "set lastblock %d", pfc->last_block );
					pns->flags.last_block = pfc->last_block;
					if( pns->version >= VER_CODE( 3, 2 ) )
					{
						msg[3] = (uint32_t)pfc->pFileInfo->PathID;
						msg[4] = (uint32_t)pfc->pFileInfo->ID;
						SendTCP( pcc->pc, msg, 20 );
					}
					else
					{
						//xlprintf(2100)( "%s Sent message: (first) SEND fpi:%d len:%d"
						//		 , account->unique_name, pfc->start, pfc->size );
						SendTCP( pcc->pc, msg, 12 );
					}
					pns->account->finished_files.size += pfc->size;
					Release( pfc );
				}
			}
			else
			{
				// no changes... get next file.
				SendTCP( pcc->pc, &account->NextResponce, 4 );
			}
		}
		LeaveCriticalSec( &account->cs );
	}
	else
		xlprintf(LOG_ALWAYS)( "No Account!" );
	return 0;
}

//-------------------------------------------------------------------------

void CloseCurrentFile( PACCOUNT account, PNETWORK_STATE pns )
{
	PCLIENT_CONNECTION pcc = pns->client_connection;
	if( pcc->buffer )
	{
		Release( pcc->buffer );
		pcc->buffer = NULL;
	}
	if( pcc->file != INVALID_INDEX )
	{
		xlprintf(LOG_ALWAYS)( "Done with file: %d", pcc->file );
		set_eof( pcc->file );
		close( pcc->file );
		pcc->file = INVALID_INDEX;
		xlprintf(LOG_ALWAYS)( "Set file %s time to %lld", pcc->LastFile, ConvertFileTimeToInt( &pns->filetime_create ) );
		SetFileTimes( pcc->LastFile, ConvertFileTimeToInt( &pns->filetime_create ), ConvertFileTimeToInt( &pns->filetime_modify ), ConvertFileTimeToInt( &pns->filetime_access ) );
	}
}

//-------------------------------------------------------------------------

static LOGICAL BackupAndCopy( CTEXTSTR basename )
{
	size_t len;
	TEXTSTR tmp_name = NewArray( TEXTCHAR, (len = StrLen( basename ) + 6 ) );
	snprintf( tmp_name, len, "%s.bak", basename );
	sack_unlink( 0, tmp_name );
#ifdef WIN32
	if( !MoveFile( basename, tmp_name ) )
	{
      lprintf( "move File Failed. %s to %s", basename, tmp_name );
		return FALSE;
	}
	if( !CopyFile( tmp_name, basename, FALSE ) )
	{
      lprintf( "copy File Failed. %s to %s", tmp_name, basename );
      return FALSE;
	}
#else
#error need a move and copy solution for non-windows
#endif
   return TRUE;
}

//-------------------------------------------------------------------------

void UpdateAccountFile( PACCOUNT account, int start, int size, PNETWORK_STATE pns )
{
	PCLIENT_CONNECTION pcc = pns->client_connection;

	if( pns->version >= VER_CODE( 3, 2 ) )
	{
		PDIRECTORY pDir = (PDIRECTORY)GetLink( &account->Directories, pns->path_id );
		PFILE_INFO pFileInfo = (PFILE_INFO)GetLink( &pDir->files, pns->file_id );
		if( !pFileInfo || ( pFileInfo->Source_ID != pns->file_id ) )
		{
			// scan for right file.
			INDEX idx;
			LIST_FORALL( pDir->files, idx, PFILE_INFO, pFileInfo )
				if( pns->file_id == pFileInfo->Source_ID )
					break;
		}
		if( pFileInfo )
		{
			INDEX file = open( pFileInfo->full_name, O_RDWR|O_BINARY );
			if( g.flags.log_file_ops )
				xlprintf(2100)( "Storing Data : %s %d %d %d", pFileInfo->name, file, start, size );
			if( file == INVALID_INDEX )
			{
				if( GetLastError() == ERROR_FILE_NOT_FOUND )
				{
					file = open( pFileInfo->full_name, O_RDWR|O_BINARY|O_CREAT );
					SetFileAttributes( pFileInfo->full_name, FILE_ATTRIBUTE_NORMAL );
				}
				else
				{
					xlprintf(2100)( "file open failed for (try rename and open?) read/write %s", pFileInfo->full_name );
					if( BackupAndCopy( pFileInfo->full_name ) )
					{
						file = open( pFileInfo->full_name, O_RDWR|O_BINARY );
					}
				}
			}
			if( file == INVALID_INDEX )
			{
				xlprintf(2100)( "Failed to open:%s", pFileInfo->full_name );
				ProcessLocalUpdateFailedCommands( account );
				return;
			}

			if( lseek( file, start, SEEK_SET ) < 0 )
				xlprintf(2100)( "Error in seek: %d", errno );
			if( write( file, pcc->buffer, size ) < 0 )
				xlprintf(2100)( "Error in write: %d", errno );

			lseek( file, pFileInfo->dwSize, SEEK_SET );

			// only set times and truncate on receive of last block in file.
			if( pns->flags.last_block )
			{
				xlprintf(2100)( "Set file length to %d", pFileInfo->dwSize );
				set_eof( file );
				close( file );
				SetFileTimes( pFileInfo->full_name
								, ConvertFileTimeToInt( &pFileInfo->lastcreate )
								, ConvertFileTimeToInt( &pFileInfo->lastmodified )
								, ConvertFileTimeToInt( &pFileInfo->lastaccess )
								);
				// gets  set as each block is sent
				pns->flags.last_block = 0;
			}
			else
			{
				xlprintf(2100)( "do not set file length to %d (not last block)", pFileInfo->dwSize );
				close( file );
			}
		}
		else
		{
			lprintf( "Get file info failed by ID: %d in %d(%s)", pns->file_id, pns->path_id, pDir->path );
		}

	}
	else
	{
		if( g.flags.log_file_ops )
			xlprintf(2100)( "Storing Data : %d %d %d", pcc->file, start, size );
		if( size )
		{
			if( lseek( pcc->file, start, SEEK_SET ) < 0 )
				lprintf( "Error in seek: %d", errno );
			if( write( pcc->file, pcc->buffer, size ) < 0 )
				lprintf( "Error in write: %d", errno );
		}
		{
			// dequeue the segment block, if any left, ask for that data...
			// if non left, close the file, request next.
			PFILECHANGE pfc = (PFILECHANGE)DequeLink( &pcc->segments );
			if( pfc )
			{
				uint32_t msg[4];
				// going to need a secondary buffer here...
				//Release( account->buffer );
				xlprintf(2100)( "Allocate buffer %d", pfc->size );
				//account->buffer = Allocate( pfc->size );
				msg[0] = account->SendResponce;
				msg[1] = (uint32_t)pfc->start;
				msg[2] = (uint32_t)pfc->size;
				xlprintf(2100)( "set lastblock %d", pfc->last_block );
				pns->flags.last_block = pfc->last_block;

				xlprintf(2100)( "asking for more data... %d %d"
						 , pfc->start, pfc->size );
				SendTCP( pcc->pc, msg, 12 );
 			}
			else
			{
				xlprintf(2100)( "Closing file: %d", pcc->file );
				CloseCurrentFile( account, pns );
				//lprintf( "Asking for next file..." );
				SendTCP( pcc->pc, &account->NextResponce, 4 );
			}
		}
	}
}

//-------------------------------------------------------------------------

char *GetAccountBuffer( PCLIENT_CONNECTION account, int length )
{
	if( length )
	{
		if( account->buffer )
			Release( account->buffer );
		account->buffer = NewArray( char, length );
	}
	return account->buffer;
}

//-------------------------------------------------------------------------

char *GetAccountDirectory( PACCOUNT account, uint32_t PathID )
{
	if( account )
		return ((PDIRECTORY)GetLink( &account->Directories, PathID ))->path;
	return NULL;
}

//-------------------------------------------------------------------------

void SendCRCs( PCLIENT pc, CTEXTSTR name, size_t insize )
{
	uint32_t *crc = NULL;
	size_t crclen;
	uintptr_t size = 0;
	uint8_t* mem;
	if( insize )
	{
		if( g.flags.log_file_ops )
			lprintf( "open %s", name );
		mem = (uint8_t*)OpenSpace( NULL, name, &size );
		if( g.flags.log_file_ops )
			lprintf( "Result of open %p %d", mem, size );
		crclen = ( size + 4095 ) / 4096;
		if( mem )
		{
			uint32_t n;
			if( size != insize )
				lprintf( "Mismatched sizes in send crcs - will cause problems... %d,%d", (uint32_t)size, (uint32_t)insize );
			crclen = ( size + 4095 ) / 4096;
			crc = (uint32_t*)Allocate( sizeof( uint32_t ) * crclen );
			for( n = 0; n < crclen; n++ )
			{
				size_t blocklen = 4096;
				if( ( size - ( n * 4096 ) ) < blocklen )
					blocklen = size - ( n * 4096 );
				crc[n] = crc32( crc32(0L, NULL, 0), mem + n * 4096, (uInt)blocklen );
			}
		}
		else if( size )
			crclen = 0;
		CloseSpace( mem );
		//lprintf( "crclen %d (%d)", crclen, crclen*4 );
		if( crclen )
			SendTCP( pc, crc, crclen * 4 );
		if( crc )
			Release( crc );
	}
}

//-------------------------------------------------------------------------

// returns TRUE if CRCs were changed.
// returns FALSE if CRCs were the same.
// (this allows caller to set file times on mis-matched files)
LOGICAL LoadCRCs( PFILE_INFO pFileInfo )
{
   LOGICAL result = TRUE; // assume file changed
	static int bInitial = 1;
	static int first_crc;
	uint32_t *crc = NULL;
	size_t crclen;
	uintptr_t size = 0;
	uint8_t* mem;

	if( bInitial )
	{
		first_crc = crc32(0L, NULL, 0);
		bInitial = 0;
	}
	if( pFileInfo->dwSize )
	{
		if( g.flags.log_file_ops )
			xlprintf(LOG_ALWAYS)( "open %s", pFileInfo->full_name );
		mem = (uint8_t*)OpenSpace( NULL, pFileInfo->full_name, &size );
		if( g.flags.log_file_ops )
			xlprintf(LOG_ALWAYS)( "Result of open %p %d", mem, size );
		crclen = ( size + 4095 ) / 4096;
		if( mem )
		{
			uint32_t n;
			if( size != pFileInfo->dwSize )
				lprintf( "Mismatched sizes in send crcs - will cause problems... %d,%d", (uint32_t)size, (uint32_t)pFileInfo->dwSize );
			crclen = ( size + 4095 ) / 4096;
			crc = (uint32_t*)Allocate( sizeof( uint32_t ) * crclen );
			for( n = 0; n < crclen; n++ )
			{
				crc[n] = mem[n * 4096];
			}
			for( n = 0; n < crclen; n++ )
			{
				size_t blocklen = 4096;
				if( ( size - ( n * 4096 ) ) < blocklen )
					blocklen = size - ( n * 4096 );
				crc[n] = crc32( first_crc, mem + n * 4096, (uInt)blocklen );
			}
		}
		else if( size )
			crclen = 0;
		CloseSpace( mem );

		//lprintf( "crclen %d (%d)", crclen, crclen*4 );
		if( pFileInfo->crc )
		{
			if( pFileInfo->crclen == crclen )
			{
				// if there were CRCs and the same length, if they match
            // result NO CHANGE
				if( MemCmp( pFileInfo->crc, crc, crclen * sizeof( uint32_t ) ) == 0 )
					result = FALSE;
			}
			Release( pFileInfo->crc );
		}
		pFileInfo->crc = crc;
		pFileInfo->crclen = crclen;
	}
	else
	{
		if( pFileInfo->crc )
			Release( pFileInfo->crc );
		else
         result = FALSE;
		pFileInfo->crc = NULL;
		pFileInfo->crclen = 0;
	}
   return result;
}

//-------------------------------------------------------------------------


//-------------------------------------------------------------------------

int NextFileChange( uintptr_t psv
                        , CTEXTSTR filepath
                        , uint64_t size
                        , uint64_t timestamp_create
                        , uint64_t timestamp_modify
                        , uint64_t timestamp_access
                        , LOGICAL bCreated
                        , LOGICAL bDirectory
                        , LOGICAL bDeleted 
						, uint32_t ID
						)
{
	PMONDIR pDir = (PMONDIR)psv;
   //lprintf( "Next file change on %p", pDir );
	if( filepath )
	{
		if( !bDeleted )
		{
			int len ;
			uint32_t msg[10+64]; // 4 for 4 words of message header
			// +64 for 256 bytes of name
			CTEXTSTR name;
			char *charmsg;
			if( pDir->flags.bIncoming )
				msg[0] = *(uint32_t*)"STAT";
			else
			{
				msg[0] = *(uint32_t*)"FILE";
				snprintf( pDir->pcc->LastFile, sizeof( pDir->pcc->LastFile ), "%s/%s", pDir->pDirectory->path, filepath );
				//strcpy( pDir->pcc->LastFile, filepath );
			}
			msg[1] = (uint32_t)size;
			//lprintf( "file time is %lld %lld %lld", *(uint64_t*)&timestamp_create, *(uint64_t*)&timestamp_modify, *(uint64_t*)&timestamp_access );
			msg[2] = (uint32_t)timestamp_create;
			msg[3] = (uint32_t)(timestamp_create>>32);
			msg[4] = (uint32_t)timestamp_modify;
			msg[5] = (uint32_t)(timestamp_modify>>32);
			msg[6] = (uint32_t)timestamp_access;
			msg[7] = (uint32_t)(timestamp_access>>32);
			msg[8] = (uint32_t)pDir->PathID;
			if( pDir->pcc->version >= VER_CODE( 3, 2 ) )
			{
				msg[9] = (uint32_t)ID;
				len = 40;
			}
			else
			{
				len = 36;
			}
			//lprintf( "original path %s", filepath );
			//name = pathrchr( filepath );
			//if( !name )
			name = filepath;
			//else
			//	name++;
			charmsg = (char*)msg;
			len += sprintf( charmsg+len, "%c%s"
							  , (char)strlen( name )
							  , name );
			if( g.flags.log_file_ops )
				lprintf( "Announced %4.4s %s (%d,%lld,%lld,%lld) %d", msg
						 , filepath, msg[1], (*(uint64_t*)(msg+2)), (*(uint64_t*)(msg+4)), (*(uint64_t*)(msg+6)), len );

			while( !NetworkLock( pDir->pcc->pc ) );
			//LogBinary( msg, len );
			SendTCP( pDir->pcc->pc, msg, len );
			if( !pDir->flags.bIncoming && !bDirectory )
			{
				TEXTCHAR tmpname[512];
				snprintf( tmpname, sizeof( tmpname ), "%s/%s", pDir->pDirectory->path, filepath );

				SendCRCs( pDir->pcc->pc, tmpname, (size_t)size );
			}

			NetworkUnlock( pDir->pcc->pc );
			return 0; // stop looping, we need to wait for other side to ack this.
		}
		else
		{
			CTEXTSTR name;
			int words = 9;
			uint8_t msg[256];
			*(uint32_t*)msg = *(uint32_t*)"KILL";
			*(uint32_t*)(msg+4) = (uint32_t)pDir->PathID;
			name = pathrchr( filepath );
			if( !name )
				name = filepath;
			else
				name++;
			if( !pDir->flags.bIncoming )
				pDir->pcc->LastFile[0] = 0;
			if( pDir->pcc->version >= VER_CODE( 3, 2 ) )
			{
				words = 10;
			}
			msg[8] = sprintf( (char*)(msg + 9), "%s", name );
			SendTCP( pDir->pcc->pc, msg, 9 + msg[8] );
			lprintf( "Announced %4.4s %s", msg
				 , filepath );
			return 0; // stop looping, we need to wait for other side to ack this.
		}
	}
	else
	{
		pDir->pcc->LastFile[0] = 0;
		if( pDir->flags.bIncoming )
		{
			lprintf( "All files on incoming directories have reported for stat." );
			EndMonitor( pDir->monitor ); // no longer need THIS...
			DeleteLink( &pDir->pcc->Monitors, pDir );
			Release( pDir );
		}
		else
		{
			lprintf( "Reported NULL filepath on outgoing..." );
		}
		return 1;
	}
}

void BuildManifest( PACCOUNT account, LOGICAL keep_manifest )
{
	size_t len = 0;
	size_t available_msg_len = 0;
	uint32_t *msg = NULL;
	uint32_t *_msg;
	INDEX idx;
	PDIRECTORY pDir;
   // release old manifest
	if( account->manifest )
      Release( account->manifest );
	lprintf( "Begin building manifest packet for %s", account->unique_name );
	LIST_FORALL( account->Directories, idx, PDIRECTORY, pDir )
	{
		lprintf( "source dir %s", pDir->path );
		if( !pDir->flags.bStaticData )
		{
		}
		else
		{
			int result = 0;
			INDEX idx2;
			PFILE_INFO pFileInfo;
			LIST_FORALL( pDir->files, idx2, PFILE_INFO, pFileInfo )
			{
				size_t oldlen = len;
				int newlen;
				// make sure we stay dword aligned....
				int filename_len = ( ( strlen( pFileInfo->name ) + 1 ) + 3 ) & 0x7fffffc;
				if( pFileInfo->flags.bDeleted )
				{
					lprintf( "file was deleted...(%s)", pFileInfo->name );
					continue;
				}
			
				if( !len )
				{
					len = 8;
					oldlen = 8;
					msg = (uint32_t*)Allocate( len );
					msg[0] = *(uint32_t*)"MANI"; // MANIfest
				}

				if( pFileInfo->flags.bDirectory )
				{
					len += ( newlen = ( 3 * sizeof( uint32_t ) ) + filename_len );
					if( len > available_msg_len )
					{
						available_msg_len += ((len-available_msg_len)>16384)?((len-available_msg_len) + 16384):16384;
						_msg = (uint32_t*)Allocate( available_msg_len );
						MemCpy( _msg, msg, oldlen );
						Release( msg );
						msg = _msg;
					}

					_msg = (uint32_t*)(((uintptr_t)msg) + len - newlen);

					_msg[0] = 1;
					_msg[1] = filename_len;
					_msg[2] = (uint32_t)idx;
					snprintf( (char*)(_msg + 3), filename_len, "%s", pFileInfo->name );
				}
				else
				{
					len += ( newlen = (uint32_t)( ( 12 * sizeof( uint32_t ) ) + filename_len + ( pFileInfo->crclen * sizeof( uint32_t ) ) ) );
					if( len > available_msg_len )
					{
						available_msg_len += ((len-available_msg_len)>16384)?((len-available_msg_len) + 16384):16384;
						_msg = (uint32_t*)Allocate( available_msg_len );
						MemCpy( _msg, msg, oldlen );
						Release( msg );
						msg = _msg;
					}
					_msg = (uint32_t*)(((uintptr_t)msg) + len - newlen);

					_msg[0] = 0;
					_msg[1] = (uint32_t)pFileInfo->dwSize;
					//lprintf( "file time is %lld %lld %lld", *(uint64_t*)&timestamp_create, *(uint64_t*)&timestamp_modify, *(uint64_t*)&timestamp_access );
					_msg[2] = (uint32_t)pFileInfo->lastcreate.dwLowDateTime;
					_msg[3] = (uint32_t)(pFileInfo->lastcreate.dwHighDateTime);
					_msg[4] = (uint32_t)pFileInfo->lastmodified.dwLowDateTime;
					_msg[5] = (uint32_t)(pFileInfo->lastmodified.dwHighDateTime);
					_msg[6] = (uint32_t)pFileInfo->lastaccess.dwLowDateTime;
					_msg[7] = (uint32_t)pFileInfo->lastaccess.dwHighDateTime;
					_msg[8] = (uint32_t)idx;
					//lprintf( "file id %d and %d", pFileInfo->ID, idx2 );
					_msg[9] = (uint32_t)pFileInfo->ID;
					_msg[10] = filename_len;
					_msg[11] = (uint32_t)pFileInfo->crclen;
					snprintf( (char*)(_msg + 12), filename_len, "%s", pFileInfo->name );
					if( pFileInfo->crc && pFileInfo->crclen )
					{
						MemCpy( _msg + 12 + (filename_len/4), pFileInfo->crc, pFileInfo->crclen * sizeof( uint32_t ) );
					}
					else
					{
						if( !pFileInfo->crc && pFileInfo->crclen )
							lprintf( "Failed to get CRC for file %s (%d)", pFileInfo->full_name, pFileInfo->crclen );
					}
				}
			}
		}
	}
   if( keep_manifest )
	{
		TEXTCHAR tmpname[256];
		FILE *manifest_save;
		snprintf( tmpname, sizeof( tmpname ), "%s.%s.manifest", g.configname, account->unique_name );
		lprintf( "Here we save the manifest... %s", tmpname );
		if( msg )
		{
			manifest_save = fopen( tmpname, "wb" );
			msg[1] = (uint32_t)(len - 8);
			if( manifest_save )
			{
				fwrite( msg, 1, len, manifest_save );
				fclose( manifest_save );
			}
			else
				lprintf( "Failed to save manifest %s", tmpname );
		}
		else
		{
			sack_unlink( 0, tmpname );
		}
	}
	else
      lprintf( "Did not save the manifest" );
	account->manifest_len = len;
	account->manifest = msg;
}

void AllFiles( PACCOUNT account, PCLIENT_CONNECTION pcc )
{
	size_t len;
	uint32_t *msg;

	if( account->manifest )
	{
		PDIRECTORY NewPath;
		INDEX idx;
		INDEX idx_path;
		PFILE_INFO file;
		LIST_FORALL( account->Directories, idx_path, PDIRECTORY, NewPath )
		{
			LIST_FORALL( NewPath->files, idx, PFILE_INFO, file )
			{
				if( file->flags.bDeleted || file->flags.bResequenced )
				{
					xlprintf(2100)( "Old manifest had files that have been deleted, rebuild manifest" );
					Deallocate( POINTER, account->manifest );
					account->manifest = NULL;
					account->manifest_len = 0;
					break;
				}
			}
			if( file )
				break;
		}
	}

	if( account->manifest )
	{
		xlprintf(2100)( "already have a manfest... use it.(%d)", account->manifest_len );
		len = account->manifest_len;
		msg = (uint32_t*)account->manifest;
	}
	else
	{
		BuildManifest( account, TRUE );
		len = account->manifest_len;
		msg = (uint32_t*)account->manifest;
	}
	if( pcc )
	{
		if( len )
		{
			msg[1] = (uint32_t)(len - 8);
			xlprintf(2100)( "Manifest is %d", len );
			SendTCP( pcc->pc, msg, len );
		}
		else
		{
			xlprintf(2100)( "Nothing to manifest... on %p", pcc->pc );
			SendTCP( pcc->pc, "DONE", 4 );
		}
	}
}


int CPROC FileMonNextFileChange( uintptr_t psv
                        , CTEXTSTR filepath
                        , uint64_t size
                        , uint64_t timestamp_modify
                        , LOGICAL bCreated
                        , LOGICAL bDirectory
								, LOGICAL bDeleted )
{
   return NextFileChange(psv,filepath,size,timestamp_modify,timestamp_modify,timestamp_modify,bCreated
	   , bDirectory
	   , bDeleted
	   , 0 );
}

void CPROC ProcessScannedFile( uintptr_t psv, CTEXTSTR name, int flags )
{
	PMONDIR pDir = (PMONDIR)psv;
	FILETIME lastmodified;
	FILETIME lastcreate;
	FILETIME lastaccess;
	TEXTCHAR filepath[512];
	DWORD dwSize;
	snprintf( filepath, sizeof( filepath ), "%s/%s", pDir->pDirectory->path, name );
	dwSize = GetFileTimeAndSize( filepath, &lastcreate, &lastaccess, &lastmodified, NULL );
	//lprintf( "File : [%s][%s] %d %lld %lld %lld", name, filepath, dwSize, *(uint64_t*)&lastmodified,*(uint64_t*)&lastaccess,*(uint64_t*)&lastcreate  );
	NextFileChange( psv
					  , name
					  , (uint64_t)dwSize
					  , ConvertFileTimeToInt( &lastcreate )
					  , ConvertFileTimeToInt( &lastmodified )
					  , ConvertFileTimeToInt( &lastaccess )
					  , TRUE      // eh - might as well claim created? unused at implementation
					  , (flags&SFF_DIRECTORY)  // is a directory
					  , FALSE 
					  , 0
					  ); // never deleted

}

//-------------------------------------------------------------------------

int NextChange( PCLIENT_CONNECTION pcc )
{
	INDEX idx;
	PMONDIR pDir;
	LIST_FORALL( pcc->Monitors, idx, PMONDIR, pDir )
	{
		if( !pDir->flags.bStaticData )
		{
			if( DispatchChanges( pDir->monitor ) )
				return 1;
		}
		else
		{
			int result = 0;
			PFILE_INFO pFileInfo;
			do
			{
				pFileInfo = (PFILE_INFO)GetLink( &pDir->pDirectory->files, pDir->current_file );
				if( pFileInfo )
				{
					pDir->current_file++;
					NextFileChange( (uintptr_t)pDir
									  , pFileInfo->name
									  , (uint64_t)pFileInfo->dwSize
									  , ConvertFileTimeToInt( &pFileInfo->lastcreate )
									  , ConvertFileTimeToInt( &pFileInfo->lastmodified )
									  , ConvertFileTimeToInt( &pFileInfo->lastaccess )
									  , TRUE      // eh - might as well claim created? unused at implementation
									  , pFileInfo->flags.bDirectory  // is a directory
									  , FALSE 
									  , (uint32_t)pFileInfo->ID
									  ); // never deleted
					result = 1;
				}
			} while( pFileInfo && pFileInfo->flags.bDirectory );
			return result;
		}
	}
	return 0;
}

//-------------------------------------------------------------------------

void LoadAccountManifest( PACCOUNT account )
{
	{
		TEXTCHAR tmpname[256];
		FILE *manifest_load;
		snprintf( tmpname, sizeof( tmpname ), "%s.%s.manifest", g.configname, account->unique_name );
		manifest_load = sack_fopen( 0, tmpname, "rb" );
		if( manifest_load )
		{
			lprintf( "Found manifest for account %s", account->unique_name );
			fseek( manifest_load, 0, SEEK_END );
			account->manifest_len = ftell( manifest_load );
			lprintf( "and it's length is %d", account->manifest_len );
			fseek( manifest_load, 0, SEEK_SET );
			account->manifest = Allocate( account->manifest_len );
			fread( account->manifest, 1, account->manifest_len, manifest_load );
			fclose( manifest_load );
		}
		ExpandManifest( account, TRUE );
	}
}

//-------------------------------------------------------------------------

#if 0
void CPROC ScanForChanges( uintptr_t psv )
{
	PACCOUNT current = (PACCOUNT)psv;
	if( current->DoScanTime && (current->DoScanTime < GetTickCount()) )
	{
		NextChange(current);
		current->DoScanTime = 0;
	}
}
#endif

//-------------------------------------------------------------------------

struct monitor_data {
	PDIRECTORY pDirectory;
	PLIST *monitors;
	INDEX PathID;
	PACCOUNT account;
	PCLIENT_CONNECTION pcc;
};

void MonitorSubdirectories( PNETWORK_STATE pns, PDIRECTORY pDirectory, PLIST *monitors, CTEXTSTR path );

void CPROC MonitorCheck( uintptr_t psv, CTEXTSTR name, int flags )
{
	if( flags & SFF_DIRECTORY )
	{
		TEXTCHAR tmpname[256];
		struct monitor_data *data = (struct monitor_data *)psv;
		PMONDIR pDir = New( MONDIR );
		pDir->pcc = data->pcc;
		pDir->PathID = data->PathID++;
		pDir->account = data->account;
		pDir->flags.bIncoming = data->pDirectory->flags.bIncoming;
		pDir->pDirectory = data->pDirectory;
		snprintf( tmpname, sizeof( tmpname ), "%s/%s", data->pDirectory->path, name );
		pDir->monitor = MonitorFiles( tmpname, 0 );
		pDir->pHandler = AddExtendedFileChangeCallback( pDir->monitor
																	 , data->pDirectory->mask
																	 , FileMonNextFileChange
																	 , (uintptr_t)pDir );
		lprintf( "%s %s monitor pathname is : %s"
			 , "..user.."
			 , pDir->flags.bIncoming?"incoming":"outgoing"
			 , tmpname );
		AddLink( data->monitors, pDir );
	}
}

void MonitorSubdirectories( PNETWORK_STATE pns, PDIRECTORY pDirectory, PLIST *monitors, CTEXTSTR path )
{
	PACCOUNT account = pns->account;
	PMONDIR pDir = New( MONDIR );
	INDEX PathID = 0;
	pDir->pcc = pns->client_connection;
	pDir->PathID = PathID++;
	pDir->account = account;
	pDir->flags.bIncoming = pDirectory->flags.bIncoming;
	pDir->flags.bStaticData = pDirectory->flags.bStaticData;
	pDir->pDirectory = pDirectory;
	pDir->monitor = MonitorFiles( path, 0 );
	pDir->pHandler = AddExtendedFileChangeCallback( pDir->monitor
																 , pDirectory->mask
																 , FileMonNextFileChange
																 , (uintptr_t)pDir );
	lprintf( "%s %s monitor pathname is : %s"
		 , "..user.."
		 , pDir->flags.bIncoming?"incoming":"outgoing"
		 , pDirectory->path );
	AddLink( monitors, pDir );
	{
		POINTER info = NULL;
		struct monitor_data data;
		data.pDirectory = pDirectory;
		data.monitors = monitors;
		data.PathID = PathID;
		data.account = account;
		data.pcc = pns->client_connection;
		while( ScanFiles( path
							 , "*"
							 , &info
							 , MonitorCheck, SFF_NAMEONLY|SFF_SUBCURSE|SFF_DIRECTORIES, (uintptr_t)&data ) );
	}
}

//-------------------------------------------------------------------------

PACCOUNT LoginEx( PCLIENT pc, char *user, uint32_t dwIP, uint32_t version DBG_PASS )
{
	PACCOUNT account;
	PNETWORK_STATE pns = (PNETWORK_STATE)GetNetworkLong( pc, 0 );
	if( !dwIP ) // localhost login
	{

	}
	for( account = g.AccountList; account; account = account->next )
	{
		if( CompareMask( account->unique_name, user, FALSE ) )
		{
			uint32_t start = GetTickCount() + 2000;
			PCLIENT_CONNECTION pcc = New( CLIENT_CONNECTION );
			memset( pcc, 0, sizeof( CLIENT_CONNECTION ) );
			pcc->version = version;
			pcc->file = INVALID_INDEX;
			account->logincount++;
			pcc->pc = pc;
			AddLink( &account->connections, pcc );
			pns->account = account;
			pns->client_connection = pcc;

			SendTCP( pc, "OKAY", 4 );

			// if other flags for options are used; should test the OR of all options
			if( g.flags.bServeClean || account->flags.bClean )
			{
				char msg[256];
				size_t len;
				lprintf( "Sending options" );
				// format of options ...
				// 4 characters (a colon followed by 3 is a good format)
				// the last option must be ':end'.
				// SO - tell the other side we're a windows system and are going to
				// give badly cased files - set forcelower, and this side will do only
				// case-insensitive comparisons on directories and names given.
				len = snprintf( msg, sizeof( msg ), "OPTS:win:end"
								  //, (g.flags.bServeClean || account->flags.bClean)?":cln":""
								  );
				lprintf( "Sending message: %s(%d)", msg, len );
				SendTCP( pc, msg, len );
			}

			{
				uint32_t msg[3];
				msg[0] = *(uint32_t*)"OVRL";
				msg[1] = account->files.count;
				msg[2] = (uint32_t)account->files.size;
				SendTCP( pc, msg, 12 );
			}
			SendTimeEx( pc, FALSE );

#ifdef _DEBUG
			_lprintf(DBG_RELAY)( "Login Success:(%d) %s at %s"
									 , account->logincount
									 , user
									 , (char*)inet_ntoa( *(struct in_addr*)&dwIP ) );
#endif
			if( logfile )
			{
				fprintf( logfile, "Login Success:(%d) %s at %s\n"
						 , account->logincount
						 , user
						 , (char*)inet_ntoa( *(struct in_addr*)&dwIP ) );
				fflush( logfile );
			}
			else
				lprintf( "Would have crashed login.log" );

			{
				INDEX PathID = 0;
				INDEX idx;
				PDIRECTORY pDirectory;
				PMONDIR pDir;
				LIST_FORALL( pcc->Monitors, idx, PMONDIR, pDir )
				{
					lprintf( "Closing account account monitors... " );
					// otherwise we probably just opened these!!!!
					if( !pDir->flags.bIncoming )
					{
						EndMonitor( pDir->monitor );
						Release( pDir );
						SetLink( &pcc->Monitors, idx, NULL );
					}
				}
				LIST_FORALL( account->Directories, idx, PDIRECTORY, pDirectory )
				{
					if( pDirectory->flags.bStaticData )
					{
#if 0
						PMONDIR pDir = New( MONDIR );
						MemSet( pDir, 0, sizeof( MONDIR ) );
						pDir->pcc = pcc;
						pDir->PathID = idx;
						pDir->account = account;
						pDir->flags.bIncoming = pDirectory->flags.bIncoming;
						pDir->flags.bStaticData = pDirectory->flags.bStaticData;
						pDir->pDirectory = pDirectory;
						AddLink( &pcc->Monitors, pDir );
						//lprintf( "Begin Scanning directory: %p %s", pDir, pDirectory->path );
						pDir->current_file = 0;
#endif
						if( version >= VER_CODE( 3, 2 ) )
						{
						}
						else
							NextChange( pcc );
						//ScanFiles( pDirectory->path, "*", &pDir->data, ProcessScannedFile, SFF_NAMEONLY|SFF_SUBCURSE|SFF_DIRECTORIES, (uintptr_t)pDir );
						break;
					}
					else
					{
						lprintf( "NOTICE: File Monitor functionality is broken...." );
						MonitorSubdirectories( pns, pDirectory, &pcc->Monitors, pDirectory->path );
					}
				}
				if( version >= VER_CODE( 3, 2 ) )
				{
					// send manifest (after OKAY and OVRL)
					AllFiles( account, pcc );
				}
			}
			return account;
		}
	}
	if( logfile && user )
	{
		fprintf( logfile, "Login Error  : %s at %s\n",user, inet_ntoa( *(struct in_addr*)&dwIP ) );
		fflush( logfile );
	}

	SendTCP( pc, "RJCT", 4 );
	return NULL;
}

//-------------------------------------------------------------------------

void Logout( PACCOUNT current_account, PCLIENT_CONNECTION pcc )
{
	INDEX idx;
	PMONDIR pDir;
	PNETWORK_STATE pns;
	if( !current_account )
		return;
	lprintf( "Logout - %s", current_account->unique_name );
	EnterCriticalSec( &current_account->cs );
	pns = (PNETWORK_STATE)GetNetworkLong( pcc->pc, 0 );
	SetNetworkLong( pcc->pc, 0, 0 );
	pns->account = NULL;
	if( pns->longbuffer )
		Release( pns->longbuffer );

	// release all directory and file info; and redo scans as appropriate
	// server should keep his same info's, they are reused
   // and not subject to change
   if( current_account->flags.client )
	{
		INDEX idx2;
		PDIRECTORY pDir;
		LIST_FORALL( current_account->Directories, idx2, PDIRECTORY, pDir )
		{
			INDEX idx3;
			PFILE_INFO pFileInfo;
			LIST_FORALL( pDir->files, idx3, PFILE_INFO, pFileInfo )
			{
				Release( pFileInfo );
				SetLink( &pDir->files, idx3, NULL );
			}
		}
	}
   // close all monitors (live data)
	LIST_FORALL( pcc->Monitors, idx, PMONDIR, pDir )
	{
		//lprintf( "Logout..." );
		EndMonitor( pDir->monitor );
		Release( pDir );
		SetLink( &pcc->Monitors, idx, NULL );
	}
	{
		PFILECHANGE pfc;
		while ( pfc = (PFILECHANGE)DequeLink( &pcc->segments ) )
			Release( pfc );
	}

	DeleteLink( &current_account->connections, pcc );
	DeleteLinkQueue( &pcc->segments );
	DeleteList( &pcc->Monitors );
	RemoveTimer( pcc->timer );
	current_account->logincount--;
	pcc->pc = NULL;
	Release( pcc );

	if( logfile )
	{
		fprintf( logfile, "Logout Success:(%d) %s\n"
				 , current_account->logincount
				 , current_account->unique_name );
		fflush( logfile );
	}
	current_account->flags.bLoggedIn = 0;
	current_account->flags.sent_login = 0;
	current_account->finished_files.count = 0;
	current_account->finished_files.size = 0;


   // on the client side, reload our current state; would be set from initial load.
	if( current_account->flags.client  && !bDone )
	{
		if( current_account->manifest )
			Release( current_account->manifest );
		current_account->manifest = NULL;
		current_account->manifest_len = 0;
		// reload the manifest (if there is one)
      lprintf( "reload manifest for account %s", current_account->unique_name );
		LoadAccountManifest( current_account );
	}

	LeaveCriticalSec( &current_account->cs );
	//lprintf( "And done closing..." );
}

//-------------------------------------------------------------------------

void CloseAllAccounts( void )
{
	PACCOUNT account;
	while( account = g.AccountList )
	{
		PDIRECTORY pDirectory;
		INDEX idx;
		PCLIENT_CONNECTION pcc;
		LIST_FORALL( account->connections, idx, PCLIENT_CONNECTION, pcc )
		{
			if( pcc->pc )
			{
				lprintf( "logout %p %p", pcc->pc, account );
				Logout( account, pcc );
			}
		}
		lprintf( "Release directories..." );
		LIST_FORALL( account->Directories, idx, PDIRECTORY, pDirectory )
		{
			Release( pDirectory );
		}
		DeleteList( &account->Directories );
		UnlinkThing( account );
		lprintf( "Client socket? ");
		if( account->flags.client )
			SetNetworkLong( account->client.TCPClient, 0, 0 );
		lprintf( "Account %p is gone.", account );
		Release( account );
	}
	{
		PNETBUFFER pNetBuf;
		while( pNetBuf = g.NetworkBuffers )
		{
			UnlinkThing( pNetBuf );
			ReleaseAddress( pNetBuf->sa );
			Release( pNetBuf->buffer );
			Release( pNetBuf );
		}
	}
}

//-------------------------------------------------------------------------


PNETBUFFER FindNetBuffer( CTEXTSTR address )
{
	PNETBUFFER pNetBuffer;
	SOCKADDR *sa = CreateSockAddress( address, 3000 );
	pNetBuffer = g.NetworkBuffers;
	while( pNetBuffer )
	{
		if( *(uint64_t*)pNetBuffer->sa == *(uint64_t*)sa )
		{
			ReleaseAddress( sa );
			return pNetBuffer;
		}
		pNetBuffer = pNetBuffer->next;
	}
	pNetBuffer = New( NETBUFFER );
	pNetBuffer->sa = sa;
	pNetBuffer->buffer = (char*)Allocate( 1024 );
	pNetBuffer->size = 0;
	pNetBuffer->valid = 0;
	LinkThing( g.NetworkBuffers, pNetBuffer );
	return pNetBuffer;
}

////-------------------------------------------------------------------------

static int PathCmpEx( CTEXTSTR s1, CTEXTSTR s2, int maxlen )
{
	if( !s1 )
		if( s2 )
			return -1;
		else
			return 0;
	else
		if( !s2 )
			return 1;
	if( s1 == s2 )
		return 0; // ==0 is success.
	for( ;maxlen && s1[0] && s2[0]
		 && ( ( ( s1[0] == '/' || s1[1] == '\\' )
				 && ( s2[0] == '/' || s2[0] == '\\' ) )
			  || (((s1[0] >='a' && s1[0] <='z' )?s1[0]-('a'-'A'):s1[0])
					== ((s2[0] >='a' && s2[0] <='z' )?s2[0]-('a'-'A'):s2[0]) ) );
		  s1++, s2++, maxlen-- )
	{
		//lprintf( "Continue... compared %c(%d) vs %c(%d)", s1[0]<32?'?':s1[0], s1[0], s2[0]<32?'?':s2[0], s2[0] );
	}
	if( !maxlen )
      return 0;
	return tolower(s1[0]) - tolower(s2[0]);
}

//-------------------------------------------------------------------------

uintptr_t CPROC SetCommon( uintptr_t psv, arg_list args )
{
	//PARAM( args, char*, name );
	PACCOUNT account;
	if( account = (PACCOUNT)psv )
	{
		//  strcpy( account->CommonDirectory, name );
	}
	return psv;
}

//-------------------------------------------------------------------------

static PDIRECTORY GetDirectory( PACCOUNT account, INDEX expected_index, CTEXTSTR path )
{
	PDIRECTORY NewPath = ( expected_index != INVALID_INDEX )
		?(PDIRECTORY)GetLink( &account->Directories, expected_index )
		:NULL;
	//lprintf( "Expected %d got %p", expected_index, NewPath );
	if( NewPath )
	{
		//lprintf( "Found it %s == '%s'?" , path?path:"NoPath", NewPath->path );
		if( path )
			if( NewPath->path[0] == 0 ||  PathCmpEx( path, NewPath->path, 1000 ) )
			{
				PDIRECTORY RemakePath;
				RemakePath = (PDIRECTORY)Allocate( strlen( path ) + 1 + sizeof( DIRECTORY ) );
				MemCpy( RemakePath, NewPath, sizeof( DIRECTORY ) );
				strcpy( RemakePath->path, path );
				RemakePath->mask = pathrchr( RemakePath->path );
				SetLink( &account->Directories, expected_index, RemakePath );
				Release( NewPath );
				NewPath = RemakePath;
			}
		// else this should be the same name and good index...
	}
	if( !NewPath )
	{
		NewPath = (PDIRECTORY)Allocate( (path?strlen( path ):0) + 1 + sizeof( DIRECTORY ) );
		MemSet( NewPath, 0, sizeof( DIRECTORY ) );
		//NewPath->files = NULL;
		NewPath->account = account;
		//NewPath->ID = 0;
		//NewPath->flags.bIncoming = 0;
		//NewPath->name = 0;
		if( path )
			strcpy( NewPath->path, path );
		else
			NewPath->path[0] = 0;
		NewPath->mask = pathrchr( NewPath->path );

		SetLink( &account->Directories, expected_index, NewPath );
		NewPath->ID = (uint32_t)FindLink( &account->Directories, NewPath );
	}
	return NewPath;
}

//-------------------------------------------------------------------------

static PFILE_INFO GetFileInfo( PDIRECTORY directory, CTEXTSTR name )
{
	PFILE_INFO pFileInfo;
	INDEX idx;
	size_t len;
	size_t dirlen;
	dirlen = StrLen( directory->path );
	LIST_FORALL( directory->files, idx, PFILE_INFO, pFileInfo )
	{
		if( PathCmpEx( pFileInfo->name, name, MAXPATH ) == 0 )
		{
			if( PathCmpEx( pFileInfo->full_name, directory->path, (uint32_t)dirlen ) != 0 )
			{
				len = StrLen( name ) + ( dirlen ) + 2;
				Release( pFileInfo->full_name );
				pFileInfo->full_name = NewArray( TEXTCHAR, len );
				snprintf( pFileInfo->full_name, sizeof( TEXTCHAR ) * len, "%s/%s", directory->path, name );
				{
					int tmp;
					for( tmp = 0; pFileInfo->full_name[tmp]; tmp++ )
						if( pFileInfo->full_name[tmp]=='\\' )
							pFileInfo->full_name[tmp] = '/';
				}
				pFileInfo->name = pFileInfo->full_name + ( dirlen + 1 );
			}
			pFileInfo->PathID = directory->ID;
			//lprintf( "Set ID on file %s to %d", pFileInfo->full_name, pFileInfo->PathID );
			return pFileInfo;
		}
	}
	if( !pFileInfo )
	{
		pFileInfo = New( FILE_INFO );
		MemSet( pFileInfo, 0, sizeof( FILE_INFO ) );
		
		len = StrLen( name ) + ( dirlen ) + 2;
		pFileInfo->full_name = NewArray( TEXTCHAR, len );
		snprintf( pFileInfo->full_name, sizeof( TEXTCHAR ) * len, "%s/%s", directory->path, name );
		{
			int tmp;
			for( tmp = 0; pFileInfo->full_name[tmp]; tmp++ )
				if( pFileInfo->full_name[tmp]=='\\' )
					pFileInfo->full_name[tmp] = '/';
		}
		pFileInfo->name = pFileInfo->full_name + ( dirlen + 1 );
		pFileInfo->PathID = directory->ID;
		AddLink( &directory->files, pFileInfo );
		pFileInfo->ID = FindLink( &directory->files, pFileInfo );
		//lprintf( "Creating new file info at %d:%d (%s):", pFileInfo->ID, pFileInfo->PathID, pFileInfo->name );
	}
	return pFileInfo;
}

//-------------------------------------------------------------------------

void ExpandManifest( PACCOUNT account, LOGICAL mark_deleted )
{
	uint32_t local_check = FALSE;
	LOGICAL status = TRUE;
	uint32_t *msg;
	size_t start = 8;  // skip 'mani' and packet length
	size_t next_offset = 0;
	int segment_count = 0;
	uintptr_t oldsize = 0;
	LOGICAL same = FALSE;
	lprintf( "Expand is default deleted? %d", mark_deleted );
	//lprintf( "Doing expand... %d %d", start, account->manifest_len );
	while( start < account->manifest_len )
	{
		PFILE_INFO pFileInfo;
		PDIRECTORY pDir;
		uint32_t PathID;
		uint32_t size;
		uint32_t crclen;
		char *filename;
		//size_t len;
		account->manifest_files.count++;
		//lprintf( "... %p %d", account->manifest, start );
		msg = (uint32_t*)(((uintptr_t)account->manifest) + start );
		//LogBinary( msg, 64 );

		//lprintf( "msg %p msg_orig %p", msg, msg_original );
		//lprintf( "start %ld start_orig %ld", start, start_original );
		if( msg[0] )
		{
			xlprintf(2100)( "Is a directory... %d %d %d %s", msg[0], msg[1], msg[2], msg + 3  );
			next_offset = ( 3 * sizeof( uint32_t ) + msg[1] );

			//lprintf( "next %d" , next_offset );
			if( msg[2] > 50000 )
			{
				xlprintf( LOG_ALWAYS )( "only expect 50000 directories to be mirrored.  This is a protection against badly formatted manifests.  Aborting manifest expansion" );
            return;
			}
			pDir = GetDirectory( account, msg[2], NULL );
			filename = (char*)(msg + 3);
			pFileInfo = GetFileInfo( pDir, filename );
			
			pFileInfo->crc = NULL;
			pFileInfo->crclen = 0;
			pFileInfo->name = StrDup(filename);
			pFileInfo->PathID = msg[2];
			pFileInfo->flags.bDirectory = 1;
			pFileInfo->ID = INVALID_INDEX;
			pFileInfo->Source_ID = INVALID_INDEX;
			pFileInfo->dwSize = 0xFFFFFFFFU;
			pFileInfo->flags.bDeleted = mark_deleted;
			PathID = msg[2];
			filename = (char*)(msg + 3);
			size = 0xFFFFFFFF;
			start += next_offset;
			crclen = 0;
			//lprintf( "Check directory %s", filename );
			CheckDirectoryOnAccount( account, pDir, filename );
		}
		else
		{
			LOGICAL bStoreCRC = FALSE;
			xlprintf(2100)( "Is a file... %d %d %s",msg[10],msg[11], msg + 12 );

			next_offset = ( ( 12 + msg[11] ) * sizeof( uint32_t ) + msg[10] );
			//lprintf( "next %d next_orig %d", next_offset, next_offset_original );

			if( msg[8] > 50000 )
			{
				xlprintf( LOG_ALWAYS )( "only expect 50000 directories to be mirrored.  This is a protection against badly formatted manifests.  Aborting manifest expansion" );
            return;
			}
			pDir = GetDirectory( account, msg[8], NULL );
			filename = (char*)(msg + 12);
			pFileInfo = GetFileInfo( pDir, filename );

			//lprintf( "Scanning and dir char 0 is %d", pDir->path[0] );
			// if fully initialized... (should always be, just being safe)
			if( pDir->path[0] )
			{
				FILE *test;
				test = fopen( pFileInfo->full_name, "rb" );
				if( test )
				{
					pFileInfo->flags.bDeleted = mark_deleted;
					bStoreCRC = TRUE;
					fclose( test );
				}
				else
				{
					lprintf( "During expand : Failed to open %s", pFileInfo->full_name );
					pFileInfo->flags.bDeleted = 1;
				}
			}
			else
			{
				// the client side should be expanding after login?
				// make sure the server saves the file during account load. (before directory names exist)
				//if( !account->flags.client )
				bStoreCRC = TRUE;
				//lprintf( "Set on file %s %s", pFileInfo->full_name, mark_deleted?"deleted":"exists" );
				pFileInfo->flags.bDeleted = mark_deleted;
			}

			pFileInfo->crc = NULL;
			pFileInfo->crclen = 0;
			pFileInfo->name = StrDup(filename);
			pFileInfo->Source_ID = INVALID_INDEX;  // this isn't a valid Source_ID
			pFileInfo->dwSize = msg[1];
			pFileInfo->flags.bDirectory = 0;
			pFileInfo->lastcreate.dwLowDateTime = msg[2];
			pFileInfo->lastcreate.dwHighDateTime = msg[3];
			pFileInfo->lastmodified.dwLowDateTime = msg[4];
			pFileInfo->lastmodified.dwHighDateTime = msg[5];
			pFileInfo->lastaccess.dwLowDateTime = msg[6];
			pFileInfo->lastaccess.dwHighDateTime = msg[7];
			{
				ULARGE_INTEGER _modified2;
				ULARGE_INTEGER _create2;
				ULARGE_INTEGER _access2;
				_modified2.u.LowPart = pFileInfo->lastmodified.dwLowDateTime;
				_modified2.u.HighPart = pFileInfo->lastmodified.dwHighDateTime;
				_create2.u.LowPart = pFileInfo->lastcreate.dwLowDateTime;
				_create2.u.HighPart = pFileInfo->lastcreate.dwHighDateTime;
				_access2.u.LowPart = pFileInfo->lastaccess.dwLowDateTime;
				_access2.u.HighPart = pFileInfo->lastaccess.dwHighDateTime;
				//lprintf( "%p %s %d %lld %lld %lld", pFileInfo, pFileInfo->full_name, pFileInfo->dwSize, _modified2.QuadPart, _create2.QuadPart, _access2.QuadPart );
			}
			if( pFileInfo->ID != msg[9] )
			{
				xlprintf(2100)( "File sequence changed... %d, %d", msg[9], pFileInfo->ID );
				pFileInfo->flags.bResequenced = 1;
			}
			// if the file is deleted (no longer exists), then we leave the CRC blank
			// otherwise, fill the existing crc blocks into the pFileInfo to be checked
			// then if the manifest and fileinfo differ, then check the file for the real blocks
			if( bStoreCRC )
			{
				// crc length is in blocks.
				pFileInfo->crc = NewArray( uint32_t, msg[11] );
				MemCpy( pFileInfo->crc, msg + 12 + (msg[10]/4), msg[11] * sizeof( uint32_t ) );
				pFileInfo->crclen = msg[11];
			}

			start += next_offset;
			//lprintf( "Added file %s[%s]", pFileInfo->name, pFileInfo->full_name );
		}
		// add files that aren't directories to the list of files on this directory.
		if( pFileInfo->dwSize != 0xFFFFFFFF )
		{
			account->manifest_files.size += pFileInfo->dwSize;
			//lprintf( "Re-set link at %d", pFileInfo->ID );
			//SetLink( &pDir->files, pFileInfo->ID, pFileInfo );
		}
	}
}

//-------------------------------------------------------------------------

PACCOUNT CreateAccount( CTEXTSTR name, LOGICAL client )
{
	PACCOUNT account;
	lprintf( "Create user account: %s", name );
	account = New( ACCOUNT );
	MemSet( account, 0, sizeof( ACCOUNT ) );
	InitializeCriticalSec( &account->cs );
	strcpy( account->unique_name, name );
	account->flags.client = client;
#ifndef VERIFY_MANIFEST
	//if( !client )
#endif
   LoadAccountManifest( account );
	account->SendResponce = *(uint32_t*)"SEND";
	account->NextResponce = *(uint32_t*)"NEXT";
	account->WhatResponce = *(uint32_t*)"WHAT";
	LinkThing( g.AccountList, account );
	return account;
}

//-------------------------------------------------------------------------

static CTEXTSTR SubstituteNameVars( CTEXTSTR name )
{
	PVARTEXT pvt = VarTextCreate();
	const TEXTCHAR *start = name;
	const TEXTCHAR *this_var = name;
	const TEXTCHAR *end;

	while( this_var = StrChr( start, '%' ) )
	{
		if( start < this_var )
			vtprintf( pvt, "%*.*s", (int)(this_var-start), (int)(this_var-start), start );
		end = StrChr( this_var + 1, '%' );
		if( end )
		{
			TEXTCHAR *tmpvar = NewArray( TEXTCHAR, end - this_var );
			CTEXTSTR envvar;
			snprintf( tmpvar, end-this_var, "%*.*s", (int)(end-this_var-1), (int)(end-this_var-1), this_var + 1 );
			envvar = OSALOT_GetEnvironmentVariable( tmpvar );
			if( envvar )
				vtprintf( pvt, "%s", OSALOT_GetEnvironmentVariable( tmpvar ) );
			else
				lprintf( "failed to find environment variable '%s'", tmpvar );
			Release( tmpvar );
			start = end + 1;
		}
		else
			lprintf( "Bad framing on environment variable %%var%% syntax got [%s]", start );
	}
	if( start[0] )
		vtprintf( pvt, "%s", start );
	{
		TEXTSTR result = StrDup( GetText( VarTextPeek( pvt ) ) );
		VarTextDestroy( &pvt );
		return result;
	}
}
//-------------------------------------------------------------------------

static uintptr_t CPROC CreateUser( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );
	return (uintptr_t)CreateAccount( name, FALSE );
}

//-------------------------------------------------------------------------

static void TrimSlashes( char *path )
{
	while( ( path[strlen(path)-1] == '/' )
			|| ( path[strlen(path)-1] == '\\' )
			|| ( path[strlen(path)-1] == ' ' )
			|| ( path[strlen(path)-1] == '\t' )
		  )
		path[strlen(path)-1] = 0;
}

//-------------------------------------------------------------------------

static uintptr_t CPROC AddIncomingPathEx( LOGICAL live, LOGICAL verify, uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, path );
	PACCOUNT account;
	if( account = (PACCOUNT)psv )
	{
		TrimSlashes( (TEXTSTR)path );
		{
			
			PDIRECTORY NewPath = GetDirectory( account, account->defined_directories++, path );
			NewPath->flags.bIncoming = 1;
			NewPath->flags.bStaticData = !live;
			NewPath->flags.bVerify = verify;

			if( !IsPath( NewPath->path ) )
				MakePath( NewPath->path );
		}
		lprintf( "add incoming %s to %s", path, account->unique_name );
	}
	else
	{
		lprintf( "Incoming specified without a user name." );
	}
	return psv;
}

static uintptr_t CPROC AddIncomingPath( uintptr_t psv, arg_list args )
{
	return AddIncomingPathEx( TRUE, FALSE, psv, args );
}
static uintptr_t CPROC AddIncomingDataPath( uintptr_t psv, arg_list args )
{
	return AddIncomingPathEx( FALSE, FALSE, psv, args );
}
static uintptr_t CPROC AddVerifyDataPath( uintptr_t psv, arg_list args )
{
	return AddIncomingPathEx( FALSE, TRUE, psv, args );
}
////-------------------------------------------------------------------------

int CPROC PrescanFileUpdate( uintptr_t psv
                        , CTEXTSTR _filepath
                        , uint64_t size
                        , uint64_t timestamp_modify
                        , LOGICAL bCreated
                        , LOGICAL bDirectory
								, LOGICAL bDeleted )
{
	if( !bDirectory && _filepath )
	{
		TEXTSTR filepath = (TEXTSTR)_filepath;
		PDIRECTORY pDir = (PDIRECTORY)psv;
		PFILE_INFO pFileInfo;
		INDEX idx;
		LIST_FORALL( pDir->files, idx, PFILE_INFO, pFileInfo )
		{
			//lprintf( "is %s==%s?", pFileInfo->full_name, filepath );
			if( PathCmpEx( pFileInfo->full_name, filepath, 1000 ) == 0 )
			{
				if( pFileInfo->flags.bScanned )
				{
					if( bDeleted )
					{
						pFileInfo->flags.bDeleted = 1;
					}
					else
					{
						lprintf( "Got File update : %s", _filepath );
						//lprintf( "Found file, updating file information..." );
						pFileInfo->flags.bDeleted = 0;
						pFileInfo->dwSize = GetFileTimeAndSize( pFileInfo->full_name
																		  , &pFileInfo->lastcreate
																		  , &pFileInfo->lastaccess
																		  , &pFileInfo->lastmodified
																		  , NULL );
						LoadCRCs( pFileInfo );
					}
				}
				else
					pFileInfo->flags.bScanned = 1;
				break;
			}
		}
		if( !pFileInfo )
		{
			PACCOUNT account = pDir->account;
			PFILE_INFO pFileInfo = New( FILE_INFO );
			xlprintf(2100)( "New file! %s", filepath );
			MemSet( pFileInfo, 0, sizeof( FILE_INFO ) );
			pFileInfo->full_name = StrDup( filepath );
			pFileInfo->name = pFileInfo->full_name + StrLen( pDir->path ) + 1;

			pFileInfo->flags.bDirectory = 0;
			pFileInfo->dwSize = GetFileTimeAndSize( pFileInfo->full_name
																, &pFileInfo->lastcreate
																, &pFileInfo->lastaccess
																, &pFileInfo->lastmodified
																, NULL );
			LoadCRCs( pFileInfo );
			if( account )
			{
				account->files.count++;
				account->files.size += pFileInfo->dwSize;
			}
			else
				lprintf( "No Account?" );

			AddLink( &pDir->files, pFileInfo );
			pFileInfo->ID = FindLink( &pDir->files, pFileInfo );
			lprintf( "new file?" );
			// new file added to structure?
		}
	}
   return 1;
}


static void CPROC PrescanFile( uintptr_t psv, CTEXTSTR name, int flags )
{
	PDIRECTORY directory = (PDIRECTORY)psv;
	PACCOUNT account = directory->account;
	PFILE_INFO pFileInfo = GetFileInfo( directory, name );
	xlprintf(2100)( "Scan file %s %s %d %p", name, pFileInfo->full_name, pFileInfo->dwSize, pFileInfo->crc );
	pFileInfo->flags.bDeleted = 0;
	if( !(flags&SFF_DIRECTORY) )
	{
		DWORD dwSize;
		FILETIME lastmodified;
		FILETIME lastcreate;
		FILETIME lastaccess;

      // it's not deleted if we find it. (could be initilaized from a expanded manifest)

		// IF IT'S A FILE...
		pFileInfo->flags.bDirectory = 0;
		dwSize = GetFileTimeAndSize( pFileInfo->full_name
											, &lastcreate
											, &lastaccess
											, &lastmodified
											, NULL );
		if( pFileInfo->dwSize != dwSize ||
			lastcreate.dwLowDateTime != pFileInfo->lastcreate.dwLowDateTime ||
			lastcreate.dwHighDateTime != pFileInfo->lastcreate.dwHighDateTime ||
			//lastaccess.dwLowDateTime != pFileInfo->lastaccess.dwLowDateTime ||
			//lastaccess.dwHighDateTime != pFileInfo->lastaccess.dwHighDateTime ||
			lastmodified.dwLowDateTime != pFileInfo->lastmodified.dwLowDateTime ||
			lastmodified.dwHighDateTime != pFileInfo->lastmodified.dwHighDateTime 
			)
		{
			if( pFileInfo->dwSize )
			{
				xlprintf(2100)( "File is different? %d %d %lld %lld %lld %lld "
					, dwSize
					, pFileInfo->dwSize
					, ConvertFileTimeToInt( &lastcreate )
					, ConvertFileTimeToInt( &pFileInfo->lastcreate )
					, ConvertFileTimeToInt( &lastmodified )
					, ConvertFileTimeToInt( &pFileInfo->lastmodified )
					  );
			}
			pFileInfo->dwSize = dwSize;
			pFileInfo->lastaccess = lastaccess;
			pFileInfo->lastcreate = lastcreate;
			pFileInfo->lastmodified = lastmodified;
			xlprintf(2100)( "updated file info to current file on disk... load CRCs..." );
			if( LoadCRCs( pFileInfo ) )
			{
            // since CRCs changed between file and manifest, use file times for check later instead of manifest times.
				pFileInfo->lastaccess = lastaccess;
				pFileInfo->lastcreate = lastcreate;
				pFileInfo->lastmodified = lastmodified;
			}
			else
			{
				// CRCs were the same as before (no change), so set file times to match manifest (where pFileInfo came from)
            // this shouldn't happen; but will for version fixes from missed last_block flag checks.
				SetFileTimes( pFileInfo->full_name
								, ConvertFileTimeToInt( &pFileInfo->lastcreate )
								, ConvertFileTimeToInt( &pFileInfo->lastmodified )
								, ConvertFileTimeToInt( &pFileInfo->lastaccess ) );
			}

			if( account->manifest )
			{
				lprintf( "Old manifest is no longer valid for sure." );
				Release( account->manifest );
				account->manifest = NULL;
				account->manifest_cursor = 0;
				account->manifest_len = 0;
			}
		}
		account->files.size += pFileInfo->dwSize;
		account->files.count++;
	}
	else
	{
		// IF IT'S A DIRECTORY...
		pFileInfo->flags.bDirectory = 1;
		pFileInfo->lastmodified.dwLowDateTime = 0;
		pFileInfo->lastmodified.dwHighDateTime = 0;
		pFileInfo->lastcreate =
			pFileInfo->lastaccess =
			pFileInfo->lastmodified;
		pFileInfo->ID = 
		pFileInfo->dwSize = 0xFFFFFFFF;
	}
}

//-------------------------------------------------------------------------

struct DeleteScanInfo {
	PACCOUNT account;
	PDIRECTORY pDir;
	PLINKSTACK delete_dirlist;
};

static void CPROC DeleteScanFile( uintptr_t psv, CTEXTSTR name, int flags )
{
	struct DeleteScanInfo *info = (struct DeleteScanInfo*)psv;
	PFILE_INFO pFileInfo;
	INDEX idx;
	//lprintf( "Find %s in directory %s", name, info->pDir->path );
	LIST_FORALL( info->pDir->files, idx, PFILE_INFO, pFileInfo )
	{
		if( StrCaseCmp( pFileInfo->name, name ) == 0 )
			break;
	}
	// if it wasn't found, then it's a file we need to delete
	if( !pFileInfo )
	{
		TEXTCHAR tmpname[280];
		snprintf( tmpname, sizeof( tmpname ), "%s/%s", info->pDir->path, name );
		if( flags & SFF_DIRECTORY )
		{
			lprintf( "Delete directory %s", tmpname );
			PushLink( &info->delete_dirlist, StrDup( tmpname ) );
		}
		else
		{
			INDEX idx2;
			CTEXTSTR keep_name;
			if( info->pDir->keep_files )
				lprintf( "Have a list of files to keep..." );
			LIST_FORALL( info->pDir->keep_files, idx2, CTEXTSTR, keep_name )
			{
				lprintf( "is [%s] == [%s]? (if so, end loop, and don't delete)", keep_name, name );
				if( CompareMask( keep_name, name, FALSE ) )
					break;
			}
			if( !keep_name )
			{
				lprintf( "Delete file %s", tmpname );
				sack_unlink( 0, tmpname );
			}
		}
	}
}

void ScanForDeletes( PACCOUNT account )
{
	INDEX idx;
	PDIRECTORY pDir;
	struct DeleteScanInfo info;
	info.account = account;
	info.delete_dirlist = CreateLinkStack();
	LIST_FORALL( account->Directories, idx, PDIRECTORY, pDir )
	{
		POINTER data = NULL;
		info.pDir = pDir;
		lprintf( "Scan for deletes in %s", pDir->path );
		while( ScanFiles( pDir->path, "*", &data
							 , DeleteScanFile, SFF_NAMEONLY|SFF_SUBCURSE|SFF_DIRECTORIES, (uintptr_t)&info ) );
	}
	{
		TEXTSTR tmpname;
		lprintf( "Need to remove some directories..." );
		while( tmpname = (TEXTSTR)PopLink( &info.delete_dirlist ) )
		{
			lprintf( "Remove: %s", tmpname );
			sack_rmdir( 0, tmpname );
			Release( tmpname );
		}
	}
	DeleteLinkStack( &info.delete_dirlist );
}

//-------------------------------------------------------------------------

static uintptr_t CPROC AddOutgoingPathEx( LOGICAL live, uintptr_t psv, arg_list args )
{
	PARAM( args, TEXTSTR, path );
	PACCOUNT account;
	if( account = (PACCOUNT)psv )
	{
		TrimSlashes( path );
		{
			PDIRECTORY NewPath = GetDirectory( account, account->defined_directories++, path );
#ifndef VERIFY_MANIFEST
			//lprintf( "Path on dir is (%d) %s (%s)", account->defined_directories-1,NewPath->path, path );
			NewPath->flags.bStaticData = !live;

			if( NewPath->mask && ( strchr( NewPath->mask, '*' ) ||
										 strchr( NewPath->mask, '?' ) ) )
			{
				((TEXTSTR)NewPath->mask)[0] = 0;
			}
			else
				NewPath->mask = NULL;


			{
				POINTER data = NULL;
#if 0
            // this is code to hook up live change monitoring... needed work.
				NewPath->pDirMon = MonitorFiles( NewPath->path, 500 );
				NewPath->pHandler = AddExtendedFileChangeCallback( NewPath->pDirMon
																, "*"
																, PrescanFileUpdate
																				 , (uintptr_t)NewPath );
#endif
				lprintf( "Begin scanning %s for files...", NewPath->path );
				while( ScanFiles( NewPath->path, "*", &data
									 , PrescanFile, SFF_NAMEONLY|SFF_SUBCURSE|SFF_DIRECTORIES, (uintptr_t)NewPath ) );
				lprintf( "File Totals now: %d(%d)", account->files.count, account->files.size );
			}
#endif
		}
		lprintf( "add outgoing %s to %s", path, account->unique_name );
	}
	else
	{
		lprintf( "Outgoing specified without a user name." );
	}
	return psv;
}

static uintptr_t CPROC AddOutgoingPath( uintptr_t psv, arg_list args )
{
	return AddOutgoingPathEx( TRUE, psv, args );
}
static uintptr_t CPROC AddOutgoingDataPath( uintptr_t psv, arg_list args )
{
	return AddOutgoingPathEx( FALSE, psv, args );
}

//-------------------------------------------------------------------------

static uintptr_t CPROC SetAccountAddress( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, address );
	PACCOUNT account;
	if( account = (PACCOUNT)psv )
	{
		account->netbuffer = FindNetBuffer( address );
		lprintf( "Set account %s listento at: %s (3000?)", account->unique_name, address );
	}
	else
	{
		lprintf( "Listen specified without a user name." );
	}
	return psv;

}

//-------------------------------------------------------------------------

static uintptr_t CPROC SetServerAddress( uintptr_t psv, arg_list args )
{
	PACCOUNT account = (PACCOUNT)psv;
	PARAM( args, CTEXTSTR, address );
	if( account->client.server )
	{
		lprintf( "Multiple servers specified, replacing first..." );
		ReleaseAddress( account->client.server );
	}
	lprintf( "connect to: %s (3000?)", address );
	account->client.server = CreateSockAddress( address, 3000 );
	return psv;
}

//-------------------------------------------------------------------------

static uintptr_t CPROC SetAccountClean( uintptr_t psv, arg_list args )
{
	PACCOUNT account = (PACCOUNT)psv;
	if( account )
	{
		account->flags.bClean = 1;
	}
	else
		g.flags.bServeClean = 1;
	return psv;
}

//-------------------------------------------------------------------------

static uintptr_t CPROC SetLoginName( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );
   uintptr_t psv_result;
	name = SubstituteNameVars( name );
	psv_result = (uintptr_t)CreateAccount( name, TRUE );
	return psv_result;
}

//-------------------------------------------------------------------------

static uintptr_t CPROC AddKeepFile( uintptr_t psv, arg_list args )
{
	PACCOUNT account = (PACCOUNT)psv;
	PARAM( args, CTEXTSTR, name );
	PDIRECTORY Path = GetDirectory( account, account->defined_directories - 1, NULL );
	AddLink( &Path->keep_files, StrDup( name ) );
	return psv;
}

//-------------------------------------------------------------------------

static uintptr_t CPROC SetForceCase( uintptr_t psv, arg_list args )
{
	PARAM( args, char *, opt );
	if( opt[0] == 'l' || opt[0] == 'L' )
		bForceLowerCase = 1;
	return psv;
}

//-------------------------------------------------------------------------

static uintptr_t CPROC FinishReading( uintptr_t psv )
{
	PACCOUNT account;
	if( account = (PACCOUNT)psv )
	{
		//LinkThing( g.AccountList, account );
	}
	return 0;
}

//---------------------------------------------------------------------------

static uintptr_t CPROC DoProcessLocalVerifyCommands( PTHREAD thread )
{
   PACCOUNT account = (PACCOUNT)GetThreadParam( thread );
	INDEX idx;
	CTEXTSTR update_cmd;
	g.threads++;
	LIST_FORALL( account->verify_commands, idx, CTEXTSTR, update_cmd )
	{
		System( update_cmd, LogOutput, 0 );
	}
	g.threads--;
	return 0;
}

static void ProcessLocalVerifyCommands( PACCOUNT account )
{
   ThreadTo( DoProcessLocalVerifyCommands, (uintptr_t)account );
}

//---------------------------------------------------------------------------

static uintptr_t CPROC DoProcessLocalUpdateCommands( PTHREAD thread )
{
   PACCOUNT account = (PACCOUNT)GetThreadParam( thread );
	INDEX idx;
	CTEXTSTR update_cmd;
	g.threads++;
	LIST_FORALL( account->update_commands, idx, CTEXTSTR, update_cmd )
	{
		System( update_cmd, LogOutput, 0 );
	}
	g.threads--;
	return 0;
}

static void ProcessLocalUpdateCommands( PACCOUNT account )
{
   ThreadTo( DoProcessLocalUpdateCommands, (uintptr_t)account );
}

//---------------------------------------------------------------------------

static uintptr_t CPROC DoProcessLocalUpdateFailedCommands( PTHREAD thread )
{
   PACCOUNT account = (PACCOUNT)GetThreadParam( thread );
	INDEX idx;
	CTEXTSTR update_cmd;
	g.threads++;
	LIST_FORALL( account->update_failure_commands, idx, CTEXTSTR, update_cmd )
	{
		System( update_cmd, LogOutput, 0 );
	}
	g.threads--;
	return 0;
}

static void ProcessLocalUpdateFailedCommands( PACCOUNT account )
{
   ThreadTo( DoProcessLocalUpdateFailedCommands, (uintptr_t)account );
}

//-------------------------------------------------------------------------

ATEXIT( WaitForTasks )
{
	while( g.threads )
	{
		//lprintf( WIDE( "Waiting for %d task threads to finish" ), g.threads );
		IdleFor( 100 );
	}
}

//-------------------------------------------------------------------------

static uintptr_t CPROC AddAccountUpdateCommand( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, command );
	PACCOUNT account;
	if( account = (PACCOUNT)psv )
	{
		AddLink( &account->update_commands, StrDup( command ) );
	}
	return psv;
}

//-------------------------------------------------------------------------

static uintptr_t CPROC AddAccountUpdateFailCommand( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, command );
	PACCOUNT account;
	if( account = (PACCOUNT)psv )
	{
		AddLink( &account->update_failure_commands, StrDup( command ) );
	}
	return psv;
}

//-------------------------------------------------------------------------

static uintptr_t CPROC AddAccountVerifyCommand( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, command );
	PACCOUNT account;
	if( account = (PACCOUNT)psv )
	{
		AddLink( &account->verify_commands, StrDup( command ) );
	}
	return psv;
}

//-------------------------------------------------------------------------

static uintptr_t CPROC SetMaxConnections( uintptr_t psv, arg_list args )
{
	PARAM( args, int64_t, value );
   maxconnections = (int)value;
   return psv;
}

//-------------------------------------------------------------------------

void ReadAccounts( const char *configname )
{
	PCONFIG_HANDLER pch;
	logfile = fopen( "login.log", "at+" );
	if( !logfile )
		logfile = fopen( "login.log", "wt" );
	pch = CreateConfigurationEvaluator();
	// AddConfiguration( pch, "common=%m", SetCommon );
	AddConfiguration( pch, "user=%m", CreateUser );
	//AddConfiguration( pch, "path %i incoming=%m", AddIncomingPathID );
	//AddConfiguration( pch, "path %i outgoing=%m", AddOutgoingPathID );
	//AddConfiguration( pch, "path named %m incoming=%m", AddIncomingPathName );
	//AddConfiguration( pch, "path named %m outgoing=%m", AddOutgoingPathName );
	AddConfiguration( pch, "incoming=%m", AddIncomingDataPath );
	AddConfiguration( pch, "verify=%m", AddVerifyDataPath );
	AddConfiguration( pch, "incoming live=%m", AddIncomingPath );
	AddConfiguration( pch, "outgoing=%m", AddOutgoingDataPath );
	AddConfiguration( pch, "outgoing live=%m", AddOutgoingPath );
	AddConfiguration( pch, "listen=%w", SetAccountAddress );
	AddConfiguration( pch, "On Update Command=%m", AddAccountUpdateCommand );
	AddConfiguration( pch, "On Update Fail Command=%m", AddAccountUpdateFailCommand );
	AddConfiguration( pch, "On verify Command=%m", AddAccountVerifyCommand );
	AddConfiguration( pch, "max connections=%i", SetMaxConnections );
	AddConfiguration( pch, "keep file=%m", AddKeepFile );
	//AddConfiguration( pch, "allow=%w", AddAllowedAddress );
	AddConfiguration( pch, "server=%w", SetServerAddress );
	AddConfiguration( pch, "login=%m", SetLoginName );
   AddConfiguration( pch, "clean", SetAccountClean );
	//AddConfiguration( pch, "forcecase=%b", SetForceCase );
	SetConfigurationEndProc( pch, FinishReading );
	ProcessConfigurationFile( pch, configname, 0 );
	DestroyConfigurationEvaluator( pch );
#ifndef VERIFY_MANIFEST
	{
		PACCOUNT account;
		lprintf( "------ RE-BUILD MANIFESTS ------------" );
		for( account = g.AccountList; account; account = account->next )
		{
			if( !account->flags.client )
				AllFiles( account, NULL );
		}
	}
#endif
}

static LOGICAL FileMatches( PFILE_INFO pFileInfo )
{
	// test if the file still matches the fileinfo
	DWORD dwSize;
	FILETIME lastmodified = { 0,0 };
	FILETIME lastcreate = { 0,0 };
	FILETIME lastaccess = { 0,0 };
	ULARGE_INTEGER _modified;
	ULARGE_INTEGER _create;
	ULARGE_INTEGER _access;
	ULARGE_INTEGER _modified2;
	ULARGE_INTEGER _create2;
	ULARGE_INTEGER _access2;
	// IF IT'S A FILE...
	xlprintf(2100)( "See if fileinfo matches filesystem %s", pFileInfo->full_name );
	pFileInfo->flags.bDirectory = 0;
	dwSize = GetFileTimeAndSize( pFileInfo->full_name
										, &lastcreate
										, &lastaccess
										, &lastmodified
										, NULL );
	_modified.u.LowPart = lastmodified.dwLowDateTime;
	_modified.u.HighPart = lastmodified.dwHighDateTime;
	_create.u.LowPart = lastcreate.dwLowDateTime;
	_create.u.HighPart = lastcreate.dwHighDateTime;
	_access.u.LowPart = lastaccess.dwLowDateTime;
	_access.u.HighPart = lastaccess.dwHighDateTime;
	if( pFileInfo->dwSize != dwSize ||
		lastcreate.dwLowDateTime != pFileInfo->lastcreate.dwLowDateTime ||
		lastcreate.dwHighDateTime != pFileInfo->lastcreate.dwHighDateTime ||
		//lastaccess.dwLowDateTime != pFileInfo->lastaccess.dwLowDateTime ||
		//lastaccess.dwHighDateTime != pFileInfo->lastaccess.dwHighDateTime ||
		lastmodified.dwLowDateTime != pFileInfo->lastmodified.dwLowDateTime ||
		lastmodified.dwHighDateTime != pFileInfo->lastmodified.dwHighDateTime 
		)
	{
		// so the file system and local manifest do not match, re-read the file?

		_modified2.u.LowPart = pFileInfo->lastmodified.dwLowDateTime;
		_modified2.u.HighPart = pFileInfo->lastmodified.dwHighDateTime;
		_create2.u.LowPart = pFileInfo->lastcreate.dwLowDateTime;
		_create2.u.HighPart = pFileInfo->lastcreate.dwHighDateTime;
		_access2.u.LowPart = pFileInfo->lastaccess.dwLowDateTime;
		_access2.u.HighPart = pFileInfo->lastaccess.dwHighDateTime;

		if( pFileInfo->dwSize || _create2.QuadPart )
		{
#if 0
			SYSTEMTIME st;
			FileTimeToSystemTime( &pFileInfo->lastcreate, &st );
			lprintf( "fileinfo create %02d/%02d/%04d %02d:%02d:%02d.%03d", st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds );
			FileTimeToSystemTime( &pFileInfo->lastaccess, &st );
			lprintf( "fileinfo access %02d/%02d/%04d %02d:%02d:%02d.%03d", st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds );
			FileTimeToSystemTime( &pFileInfo->lastmodified, &st );
			lprintf( "fileinfo modifi %02d/%02d/%04d %02d:%02d:%02d.%03d", st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds );

			FileTimeToSystemTime( &lastcreate, &st );
			lprintf( "file     create %02d/%02d/%04d %02d:%02d:%02d.%03d", st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds );
			FileTimeToSystemTime( &lastaccess, &st );
			lprintf( "file     access %02d/%02d/%04d %02d:%02d:%02d.%03d", st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds );
			FileTimeToSystemTime( &lastmodified, &st );
			lprintf( "file     modifi %02d/%02d/%04d %02d:%02d:%02d.%03d", st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds );
#endif
			xlprintf(2100)( "File is different. %d %d %lld %lld %lld %lld  %s"
							  , dwSize
							  , pFileInfo->dwSize
							  , _create.QuadPart
							  , _create2.QuadPart
							  , _modified.QuadPart
							  , _modified2.QuadPart
							  , pFileInfo->full_name
							  );

         // file doesn't exist, but it's only a 0 byte file.
			if( dwSize == (DWORD)-1 &&
				pFileInfo->dwSize == 0 )
			{
				fclose( sack_fopen( 0, pFileInfo->full_name, "wb" ) );
				SetFileTimes( pFileInfo->full_name
								, *(uint64_t*)&pFileInfo->lastcreate
								, *(uint64_t*)&pFileInfo->lastmodified
								, *(uint64_t*)&pFileInfo->lastaccess );
            return TRUE;
			}
		}
		else
		{
			xlprintf(2100)( "file is new? (not in an old manifest) %p %d %lld", pFileInfo,  pFileInfo->dwSize, _create2.QuadPart );
		}
		pFileInfo->dwSize = dwSize;
		if( LoadCRCs( pFileInfo ) )
		{
			// since CRCs changed between file and manifest, use file times for check later instead of manifest times.
			pFileInfo->lastaccess = lastaccess;
			pFileInfo->lastcreate = lastcreate;
			pFileInfo->lastmodified = lastmodified;
		}
		else
		{
			// CRCs were the same as before (no change), so set file times to match manifest (where pFileInfo came from)
			// this shouldn't happen; but will for version fixes from missed last_block flag checks.
         xlprintf(2100)( "Fixing file times to old manifest." );
			SetFileTimes( pFileInfo->full_name
							, ConvertFileTimeToInt( &pFileInfo->lastcreate )
							, ConvertFileTimeToInt( &pFileInfo->lastmodified )
							, ConvertFileTimeToInt( &pFileInfo->lastaccess ) );
         return TRUE; // CRCs were the same... only a time difference would have caused this... and the time is really manifest time.
		}
		return FALSE;
	}
	else
	{

	}
	return TRUE;
}

LOGICAL ProcessManifest( PNETWORK_STATE pns, PACCOUNT account, uint32_t *buffer, size_t length )
{
	uint32_t local_check = FALSE;
	LOGICAL status = TRUE;
	uint32_t *msg;
	size_t start = 0;
	size_t next_offset = 0;
	int segment_count = 0;
	uintptr_t oldsize = 0;
   LOGICAL send_fail = 0;
	POINTER mem;
	LOGICAL same = FALSE;
	xlprintf(2100)( "So when we process the client manifest we have %p ", account->Directories );
	mem = account->manifest;
   oldsize = account->manifest_len;

	xlprintf(2100)( "and we start at %d,%d", start, length );
	while( !pns->flags.bWantClose && ( start < length ) )
	{
		PFILE_INFO pFileInfo;
		PDIRECTORY pDir;
		char *filename;
		account->manifest_files.count++;

		msg = (uint32_t*)(((uintptr_t)buffer) + start );

		if( msg[0] )
		{
			next_offset = ( 3 * sizeof( uint32_t ) + msg[1] );

			pDir = GetDirectory( account, msg[2], NULL );

			filename = (char*)(msg + 3);
			pFileInfo = GetFileInfo( pDir, filename );
			pFileInfo->flags.bDeleted = 0;

			pFileInfo->flags.bDirectory = 1;
			pFileInfo->ID = INVALID_INDEX;
			pFileInfo->Source_ID = INVALID_INDEX;
			pFileInfo->dwSize = 0xFFFFFFFFU;

			start += next_offset;

			CheckDirectoryOnAccount( account, pDir, filename );
		}
		else
		{
         // check the physical file also... (updates fileinfo to physical, instead of manifest)
			LOGICAL file_matches;
			next_offset = ( ( 12 + msg[11] ) * sizeof( uint32_t ) + msg[10] );

			//lprintf( "next %d ", next_offset );

			pDir = GetDirectory( account, msg[8], NULL );

			filename = (char*)(msg + 12);
			pFileInfo = GetFileInfo( pDir, filename );
			pFileInfo->flags.bDeleted = 0;
			pFileInfo->Source_ID = msg[9];  // this is now correctly the server's source_ID


			// in update mode, read physical file
			if( !pDir->flags.bVerify )
				file_matches = FileMatches( pFileInfo );


			//lprintf( "Set file %d as %d %s", pFileInfo->ID, pFileInfo->Source_ID, pFileInfo->name );
			xlprintf(2100)( "file sizes %5d %5d  %16lld %16lld %16lld %16lld (%s)"
					 , msg[1]
					 , pFileInfo->dwSize
					 , ((uint64_t*)(msg+2))[0]
					 , ConvertFileTimeToInt( &pFileInfo->lastcreate )
					 , ((uint64_t*)(msg+4))[0]
					 , ConvertFileTimeToInt( &pFileInfo->lastmodified )
							  , filename );

         // this will be (if they are different from manifest to disk, instead of manifest to manfiest except in verify)
			if( !pFileInfo->crc 
				|| ( pFileInfo->dwSize != msg[1] )
				|| ( pFileInfo->lastcreate.dwLowDateTime != msg[2] )
				|| ( pFileInfo->lastcreate.dwHighDateTime != msg[3] )
				|| ( pFileInfo->lastmodified.dwLowDateTime != msg[4] )
				|| ( pFileInfo->lastmodified.dwHighDateTime != msg[5] )
				//|| ( pFileInfo->lastaccess.dwLowDateTime != msg[6] )
				//|| ( pFileInfo->lastaccess.dwHighDateTime != msg[7] )
				)
			{
				LOGICAL checked_crcs = FALSE;
            LOGICAL needed_changes = FALSE;

				// file didn't exist, file is simple 0 byte file....
            // just create an empty file....
				if( pFileInfo->dwSize == (DWORD)-1 &&
					msg[1] == 0 )
				{
					FILE *file = sack_fopen( 0, pFileInfo->full_name, "wb" );
					if( !file )
					{
						xlprintf(LOG_ALWAYS)( "Failed to create file [%s]", pFileInfo->full_name );
					}
					else
					{
						fclose( file );
						SetFileTimes( pFileInfo->full_name, ((uint64_t*)(msg + 2))[0], ((uint64_t*)(msg + 4))[0], ((uint64_t*)(msg + 6))[0] );
					}
					// redundant work to what's below....
					// this has to be set before FileMatches to get a TRUE result.
					// pFileInfo DOES match the physical file at this time.
					pFileInfo->dwSize = msg[1];
					pFileInfo->flags.bDirectory = 0;
					pFileInfo->lastcreate.dwLowDateTime = msg[2];
					pFileInfo->lastcreate.dwHighDateTime = msg[3];
					pFileInfo->lastmodified.dwLowDateTime = msg[4];
					pFileInfo->lastmodified.dwHighDateTime = msg[5];
					pFileInfo->lastaccess.dwLowDateTime = msg[6];
					pFileInfo->lastaccess.dwHighDateTime = msg[7];
				}

				// if in update mode, pfile info will already have been updated, and difference selected...
            // so only in verify mode read it here to see if maybe the file is already up to date
				if( pDir->flags.bVerify )
					file_matches = FileMatches( pFileInfo );

				xlprintf(2100)( "manifest file and message manifest are %s %p"
								  , file_matches?"same":"different"
								  , pFileInfo->crc
								  );


				pFileInfo->dwSize = msg[1];
				pFileInfo->flags.bDirectory = 0;
				pFileInfo->lastcreate.dwLowDateTime = msg[2];
				pFileInfo->lastcreate.dwHighDateTime = msg[3];
				pFileInfo->lastmodified.dwLowDateTime = msg[4];
				pFileInfo->lastmodified.dwHighDateTime = msg[5];
				pFileInfo->lastaccess.dwLowDateTime = msg[6];
				pFileInfo->lastaccess.dwHighDateTime = msg[7];

				// if there were existing CRCs (from old manifest) then we can just check here and not
				// have to scan the file.
			check_crcs:
				{
					uint32_t* newcrc = msg + 12 + (msg[10]/4);
					uint32_t newcrclen = msg[11];

					{
						// if the file matches on disk what we think it should be, then
						//just quick check the CRC in memory.
						PFILECHANGE pfc = NULL;
                  PFILECHANGE pfc_last = NULL;
						uint32_t n = 0;
						int b = 0;
						xlprintf(2100)( "check manifest or file CRCs (file if file was not the same as manifest)" );
						while( ( pFileInfo->crclen && ( n < pFileInfo->crclen ) ) && n < newcrclen )
						{
							if( !pFileInfo->crc || ( newcrc[n] != pFileInfo->crc[n] ) )
							{
								if( pfc )
								{
									pfc->size += 4096;
									b++;
									if( b > 20 )
									{
										xlprintf(2100)( "Enque Link at %d for %d %s", pfc->start, pfc->size, pfc->pFileInfo->full_name );
										EnqueLink( &pns->client_connection->segments, pfc );
										needed_changes = TRUE;
										pns->client_connection->segment_total_size += pfc->size;
										pns->client_connection->segment_total++;
										pfc = NULL;
									}
								}
								else
								{
									pfc = New( FILECHANGE );
									pfc_last = pfc;
									pfc->last_block = FALSE;
									pfc->pFileInfo = pFileInfo;
									pfc->size = 4096;
									pfc->start = n * 4096;
									b = 0;
								}
							}
							else
							{
								if( pfc )
								{
									xlprintf(2100)( "Enque Link at %d for %d %s", pfc->start, pfc->size, pfc->pFileInfo->full_name );
									EnqueLink( &pns->client_connection->segments, pfc );
									needed_changes = TRUE;
									pns->client_connection->segment_total_size += pfc->size;
									pns->client_connection->segment_total++;
									pfc = NULL;
								}
							}
							n++;
						}
						while( n < newcrclen )	
						{
							if( pfc )
							{
								pfc->size += 4096;
								b++;
								if( b > 20 )
								{
									xlprintf(2100)( "Enque Link at %d for %d %s", pfc->start, pfc->size, pfc->pFileInfo->full_name );
									EnqueLink( &pns->client_connection->segments, pfc );
									needed_changes = TRUE;
									pns->client_connection->segment_total_size += pfc->size;
									pns->client_connection->segment_total++;
									pfc = NULL;
								}
							}
							else
							{
								pfc = New( FILECHANGE );
								pfc_last = pfc;
								pfc->last_block = FALSE;
								pfc->pFileInfo = pFileInfo;
								pfc->size = 4096;
								pfc->start = n * 4096;
								b = 0;
							}
							n++;
						}
						if( pfc )
						{
							xlprintf(2100)( "Enque Link at %d for %d %s", pfc->start, pfc->size, pfc->pFileInfo->full_name );
							EnqueLink( &pns->client_connection->segments, pfc );
                     needed_changes = TRUE;
							pns->client_connection->segment_total_size += pfc->size;
							pns->client_connection->segment_total++;
							pfc = NULL;
						}
						if( pDir->flags.bVerify && needed_changes )
						{
							FILETIME ft;
							SYSTEMTIME st;
							static ULARGE_INTEGER ul;
							if( ul.QuadPart == 0 )
							{
								GetSystemTime(&st);              // Gets the current system time
								SystemTimeToFileTime(&st, &ft);
								ul.LowPart = ft.dwLowDateTime;
								ul.HighPart = ft.dwHighDateTime;
							}
							// in verify mode, just report that we're failed, and be done.
							lprintf( "Verify failed. (on %s)", pFileInfo->name );
							if( !SetFileTimes( pFileInfo->full_name, ul.QuadPart, ul.QuadPart, ul.QuadPart ) )
							{
                        BackupAndCopy( pFileInfo->full_name );
								if( !SetFileTimes( pFileInfo->full_name, ul.QuadPart, ul.QuadPart, ul.QuadPart ) )
								{
                           lprintf( "Failure." );
								}
							}
                     send_fail = TRUE;
							//SendTCP( pns->client_connection->pc, "FAIL", 4 );
							//return FALSE;
						}
						if( pFileInfo->crc )
						{
							Release( pFileInfo->crc );
							pFileInfo->crc = NULL;
                     pFileInfo->crclen = 0;
						}
						checked_crcs = TRUE;

						if( pfc_last )
						{
							xlprintf(2100)( "block %p is last.", pfc_last );
							pfc_last->last_block = TRUE;
						}
						//else
						//	xlprintf(2100)( "no last block" );
					}
				}

				if( !send_fail )
				{
					// save the new crc's (the files will match this)
					pFileInfo->crc = NewArray( uint32_t, msg[11] );
					MemCpy( pFileInfo->crc, msg + 12 + (msg[10]/4), msg[11] * sizeof( uint32_t ) );

					pFileInfo->crclen = msg[11];
				}
			}
			else
			{
				if( 0 )
				{
					LogBinary( pFileInfo->crc, pFileInfo->crclen * sizeof( uint32_t ) );
					LogBinary( msg + 12 + (msg[10]/4), msg[11] * sizeof( uint32_t ) );
				}
            // reload CRCs...
				if( pDir->flags.bVerify )
				{
					file_matches = FileMatches( pFileInfo );
					if( 0 )
						LogBinary( pFileInfo->crc, pFileInfo->crclen * sizeof( uint32_t ) );
				}

				xlprintf(2100)( "(local)manifest file and (server)manifest message are %s time/size... check CRC? %p %p (%d)"
								  , file_matches?"Same":"Different", pFileInfo->crc, msg, msg[11] * sizeof( uint32_t ) );
				{
					int first = 1;
					do
					{
						// file times matched, size matched... shouldn't even really check the crc content...
						if( MemCmp( pFileInfo->crc, msg + 12 + (msg[10]/4), msg[11] * sizeof( uint32_t ) ) == 0 )
						{
							if( !first )
								xlprintf(2100)( "and the CRC reload worked." );
							break;
						}
						else
						{
							if( first )
							{
								xlprintf(2100)( "Everything else was the same, load the CRCs again and make sure... manifests differed" );
								LoadCRCs( pFileInfo );
								first = 0;
								continue;
							}
							xlprintf(2100)( "Strange - same size, and times, but, different CRC buffers?!" );
							goto check_crcs;
						}
					}
					while( 1 );
				}
			}

			account->manifest_files.size += pFileInfo->dwSize;

			start += next_offset;
			//xlprintf(2100)( "Added file %s[%s]", pFileInfo->name, pFileInfo->full_name );
		}
	}

	if( send_fail )
	{
		{
			PFILECHANGE pfc;
			while ( pfc = (PFILECHANGE)DequeLink( &pns->client_connection->segments ) )
				Release( pfc );
		}
		lprintf( "Send FAIL." );
		if( account->verify_commands )
			ProcessLocalVerifyCommands( account );

      pns->client_connection->flags.failed = 1;
		SendTCP( pns->client_connection->pc, "FAIL", 4 );
      // give network a tick.
      Relinquish();
	}

	// clear the manifest, because files may have changed, and we'll have to rebuild off the current info
	if( account->manifest )
		Release( account->manifest );
	account->manifest = NULL;
	account->manifest_len = 0;

	if( pns && pns->client_connection->segments )
	{
		account->files.count = pns->client_connection->segment_total;
		account->files.size = pns->client_connection->segment_total_size;
		xlprintf(2100)( "Need segments...." );
		// don't change the manifest file, until when scanning the nwe one it matches the files.
	}
	else
	{
		// okay manifest compared OK, and we found no files needing change, output the manifest.
		// opens file in truncated mode, so the size will be correct.
		//xlprintf(2100)( "Did not need segments; save manifest: %s(%p)", expanded_filename, file );
	}
	account->flags.manifest_process = 0;

	return status;
}

void ProcessFileChanges( PACCOUNT account, PCLIENT_CONNECTION pcc )
{
	PFILECHANGE pfc = (PFILECHANGE)DequeLink( &pcc->segments );
	if( pfc )
	{
		uint32_t msg[5];
		msg[0] = *(uint32_t*)"SEND";
		msg[1] = (uint32_t)pfc->start;
		msg[2] = (uint32_t)pfc->size;
		msg[3] = (uint32_t)pfc->pFileInfo->PathID;
		msg[4] = (uint32_t)pfc->pFileInfo->Source_ID;
      account->flags.bRequestedUpdates = 1;
		xlprintf(2100)( "asking for more data... %d %d in (%d/%d : %d) %s", pfc->start, pfc->size, pfc->pFileInfo->ID, pfc->pFileInfo->Source_ID, pfc->pFileInfo->PathID, pfc->pFileInfo->full_name );
		{
			PNETWORK_STATE pns = (PNETWORK_STATE)GetNetworkLong( pcc->pc, 0 );
			//xlprintf(2100)( "set lastblock %d", pfc->last_block );
			pns->flags.last_block = pfc->last_block;
		}
		SendTCP( pcc->pc, msg, 20 );

		Release( pfc );
	}
	else
	{
		uint32_t msg[1];
		if( pcc->version >= VER_CODE( 3, 2 ) )
		{
			// send manifest (after OKAY and OVRL)
			xlprintf(2100)( "RE-BUILD manifest after receiving all our data (also saves it)" );
			BuildManifest( account, !pcc->flags.failed );
		}
		if( account->flags.bRequestedUpdates && account->update_commands )
		{
			ProcessLocalUpdateCommands( account );
		}
		lprintf( "Send NEXT" );
		msg[0] = *(uint32_t*)"NEXT";
		SendTCP( pcc->pc, msg, 4 );
	}
}
