#include <stdhdrs.h>
#include <filesys.h>

TEXTCHAR *curdir=WIDE(".");

typedef struct path_mask_tag
{
	struct  {
		uint32_t ignore : 1;
	}flags;
	TEXTCHAR *mask;
} MASK, *PMASK;

PDATALIST masks;

TEXTCHAR *base;

FILE *in, *out;

enum {
	TYPE_UNKNOWN
	,TYPE_C
	,TYPE_H
	,TYPE_CPP
	,TYPE_HPP
	,TYPE_MAKEFILE
};

int tab	 // set to collapse spaces into N spaces per tab
  , detab // set to decompress tabs to N spaces per tab
  , carriage // set to write \r\n instead of just \n
  , fix_double // set to write \n instead of \r\r
  , linefeed // set to write \r\n isntead of just \r
  , type		 // set to know how to comment
  , bDoLog	 // do minor logging
  , subcurse // go into subdirectories...
  , strip_linefeed // remove \n and replace with ' '
;



TEXTCHAR linebuffer[1024 * 64];

#ifdef _WIN32
#define KEEP_CASE FALSE
#else
#define KEEP_CASE TRUE
#endif

void CPROC process( uintptr_t psv, CTEXTSTR file, int flags )
{
	static int count; // used for fix_double processing.
	TEXTCHAR outfile[256];
	int collect_idx, at_start = 1;
	int idx, nonewline = 0;
	{
		int matched = FALSE;
		INDEX idx;
		PMASK pMask;
		DATA_FORALL( masks, idx, PMASK, pMask )
		{
			CTEXTSTR name = pathrchr( file );
			if( !name )
				name = file;
			else
				name = name+1;
			if( !matched || pMask->flags.ignore )
			{
				if( CompareMask( pMask->mask, name, KEEP_CASE ) )
				{
					if( pMask->flags.ignore )
					{
						if( bDoLog )
							printf( WIDE("Ignoring file: %s\n"), file );
						return;
					}
					matched = TRUE;
				}
			}
		}
		if( !matched )
		{
			printf( WIDE("Skipping file: %s\n"), file );
			return;
		}
	}
	if( bDoLog )
		printf( WIDE("Processing file: %s\n"), file );
	in = sack_fopen( 0, file, WIDE("rb") );
	if( in )
	{
		int c;
		snprintf( outfile, sizeof( outfile ), WIDE("%s.new"), file );
		out = sack_fopen( 0, outfile, WIDE("wb") );
		if( !out )
		{
			fprintf( stderr, WIDE("Could not create %s\n"), outfile );
			return;
		}
		if( type )
		{
			// This code is UNUSED... was going to auto mark what I did
			// to a file, but have decided that might not be a good idea.
			// so since 'type' is never set, this is never done.

			switch( type )
			{
			case TYPE_C:
			case TYPE_H:
				fprintf( out, WIDE("/* Expanding tabs %d tabbing to %d %s '\r's */%s")
								, detab, tab
								, (carriage)?WIDE("with"):WIDE("without")
								, (carriage)?WIDE("\r\n"):WIDE("\n") );
				break;
			case TYPE_CPP:
			case TYPE_HPP:
				fprintf( out, WIDE("// Expanding tabs %d tabbing to %d %s '\r's%s")
								, detab, tab
								, (carriage)?WIDE("with"):WIDE("without")
								, (carriage)?WIDE("\r\n"):WIDE("\n") );
				break;
			case TYPE_MAKEFILE:
				fprintf( out, WIDE("# Expanding tabs %d tabbing to %d %s '\r's%s")
								, detab, tab
								, (carriage)?WIDE("with"):WIDE("without")
								, (carriage)?WIDE("\r\n"):WIDE("\n") );
				break;
			case TYPE_UNKNOWN:
			default:
				break;
			}
		}
		idx = 0;
		while( !nonewline &&
				 ( c = fgetc( in ) ) > 0 )
		{
			if( c == '\r' )
			{
				if( linefeed )
				{
					if( carriage )
						linebuffer[idx++] = '\r';
					linebuffer[idx++] = '\n';
				}
				if( fix_double )
				{
					count++;
					if( count == 2 )
					{
						count = 0;
						if( carriage )
							linebuffer[idx++] = '\r';
						linebuffer[idx++] = '\n';
					}
				}
				continue;
			}
			else { count = 0; if( detab && c == '\t' )
			{
				int end = ( ( idx + (detab) ) / detab ) * detab;
				do
				{
					linebuffer[idx++] = ' ';
				} while( idx < end );
				continue;
			}
			else if( c == '\n' )
			{
				if( strip_linefeed )
				{
					linebuffer[idx++] = ' ';
					continue;
				}
			storeline:
				collect_idx = 0; // flush current line.
				at_start = 1;
				if( tab )
				{
					int n, startblank, endblank, ofs;
					startblank = -1;
					ofs = 0;
					for( n = 0; n < idx; n++ )
					{
						if( linebuffer[n] == ' ' )
						{
							if( startblank < 0 )
							{
								startblank = n;
							}
						}
						else
						{
							int col;
							endblank = n;
							if( startblank >= 0
							  && ( endblank - startblank > 1 ) )
							{
								do
								{
									col = ( ( startblank + (tab) ) / tab ) * tab;
									if( col <= endblank )
									{
										linebuffer[ofs++] = '\t';
										startblank = col;
									}
								} while( col < endblank );
							}
							if( startblank >= 0 )
							{
								for( col = startblank; col < endblank; col++ )
									linebuffer[ofs++] = ' ';
								startblank = -1;
							}
							linebuffer[ofs++] = linebuffer[n];
						}
					}
					idx = ofs;
				}
				// well - since gcc often complains - no newline
				// we should add this to make it happy.
				//if( !nonewline )
				{
					TEXTCHAR *line_start = linebuffer;
					int len = idx;
					while( line_start[0] &&							 ( line_start[0] == '\t' || line_start[0] == ' ' ) )
					{
						line_start++;
						len--;
					}
				}

				{
					if( carriage )
						linebuffer[idx++] = '\r';
					linebuffer[idx++] = '\n';
				}
				fwrite( linebuffer, 1, idx, out );
				idx = 0;
				continue;
			}
			linebuffer[idx++] = c;
			}
		} // added brace after '\r' processing.
		if( idx )
		{
			nonewline = 1;
			goto storeline;
		}
		fclose( out );
		fclose( in );
		sack_unlink( 0, file );
		sack_rename( outfile, file );
	}
	else
		fprintf( stderr, WIDE("%s is not a file.\n"), file );
}


SaneWinMain( argc, argv )
{
	int argstart = 1;
	int all_ignored = 1;
	masks = CreateDataList( sizeof( MASK ) );
	if( argc == 1 )
	{
		fprintf( stderr, WIDE("usage: %s [-T#D#RSV] <path> <files...>\n"), argv[0] );
		fprintf( stderr, WIDE(" all parameter options are case-insensative\n") );
		fprintf( stderr, WIDE(" -[Tt]# - specifies the # of spaces to make a tab\n") );
		fprintf( stderr, WIDE(" -[Dd]# - specifies the # of spaces to make from a tab\n") );
		fprintf( stderr, WIDE(" -[Nn] - convert \\r to \\n(if -R also make \\r\\n\n") );
		fprintf( stderr, WIDE(" -[Aa] - Strip \\r and \\n into a single ' '\n") );
		fprintf( stderr, WIDE(" -[Ff] - Strip double \\r\\r into a single '\\n'\n") );
		fprintf( stderr, WIDE(" -[Ss] - subcurse (recurse) through all sub-directories\n") );
		fprintf( stderr, WIDE(" -[Rr] - add carriage returns (\\r) to output\n") );
		fprintf( stderr, WIDE(" -[Vv] - verbose operation - list files processed\n") );
		fprintf( stderr, WIDE(" -D, -T specified together the file is first de-tabbed and then tabbed\n") );
		fprintf( stderr, WIDE(" <path> is the starting path...\n") );
		fprintf( stderr, WIDE(" !<file> specifies NOT that mask...\n") );
		fprintf( stderr, WIDE(" no options - Remove \\r, check current directory only\n") );
		fprintf( stderr, WIDE("Minor effect - adds a line terminator to ALL lines\n") );
		fprintf( stderr, WIDE("GNUish compilers often complain 'no end of line at end of file'.\n") );
		return -1;
	}
	for( ;argv[argstart]; argstart++ )
	{
		int n;
		if( argv[argstart][0] == '-' )
		{
			for( n = 1; argv[argstart][n]; n++ )
			{
			switch( argv[argstart][n] )
			{
			case 'a':
			case 'A':
				strip_linefeed = 1;
				break;
			case 'T':
			case 't':
				if( argv[argstart][2] >= '0' &&
					 argv[argstart][2] <= '9' )
					tab = argv[argstart][2] - '0';
				else
					fprintf( stderr, WIDE("invalid length to Tab option\n") );
				break;
			case 'D':
			case 'd':
				if( argv[argstart][2] >= '0' &&
					 argv[argstart][2] <= '9' )
					detab = argv[argstart][2] - '0';
				else
					fprintf( stderr, WIDE("invalid length to Tab option\n") );
				break;
			case 'R':
			case 'r':
				carriage = 1;
				break;
			case 'F':
			case 'f':
				fix_double = 1;
				break;
			case 'N':
			case 'n':
				linefeed = 1;
				break;
			case 'V':
			case 'v':
				bDoLog = 1;
				break;
			case 's':
			case 'S':
				subcurse = 1;
			}
			}
		}
		else
		{
			if( !base )
			{
				if( strchr( argv[argstart], '*' )
				  ||strchr( argv[argstart], '?' ) )
				{
					base = WIDE(".");
				}
				else
				{
					base = argv[argstart];
					continue;
				}
			}
			{
				MASK mask;
				mask.mask = argv[argstart];
				if( mask.mask[0] == '!' )
				{
					mask.flags.ignore = 1;
					mask.mask++;
				}
				else
				{
					all_ignored = FALSE;
					mask.flags.ignore = 0;
				}
				AddDataItem( &masks, &mask );
			}
		}
	}
	if( !masks->Cnt || all_ignored )
	{
		MASK mask;
		mask.mask = WIDE("*");
		mask.flags.ignore = 0;
		AddDataItem( &masks, &mask );
	}
	if( base )
	{
		void *data = NULL;
		while( ScanFiles( base
							 , WIDE("*")
							 , &data
							 , process
							 , subcurse?SFF_SUBCURSE:0
							 , 0 ) );
	}
	return argstart;
}
EndSaneWinMain()
