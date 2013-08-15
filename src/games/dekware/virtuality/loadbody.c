#include <stdio.h>
#include "bone.h"


typedef struct file_input 
{
	FILE *file;
	PTEXT pInput; // last input line...
	PTEXT pPartial;
	PTEXT pParsed;
	PTEXT pToken;
} FILEIN, *PFILEIN;

PFILEIN OpenFileInput( char *pName )
{
	PFILEIN pFile;
	pFile = New( FILEIN );
	MemSet( pFile, 0, sizeof( FILEIN ) );
	pFile->file = fopen( pName, "rb" );
	if( !pFile->file )
	{
		Release( pFile );
		return NULL;
	}
	return pFile;
}

void CloseFileInput( PFILEIN pFile )
{
	fclose( pFile->file );
	LineRelease( pFile->pInput );
	LineRelease( pFile->pParsed );
	Release( pFile );
}

PTEXT get_token( PFILEIN source, FILE *out )
{
   #define WORKSPACE 128  // character for workspace
   if( !source )
      return NULL;
restart:
	if( !(source->pToken) || !(NEXTLINE(source->pToken) ) ) 
	{
		if( source->pParsed )
		{
			LineRelease( source->pParsed );
			source->pParsed = NULL;
		}
		do
		{
			PTEXT pIn;
			pIn = SegCreate( WORKSPACE );
			if( !fgets( GetText( pIn ), WORKSPACE, source->file ) )
			{
				// hmm end of file....
				LineRelease( pIn );
				break;
			}
			else // got some data from the file...
			{
				char *p;
				p = strchr( GetText( pIn ), '#' );
				if( p )
				{
					*p = 0;	
					SetTextSize( pIn, p - GetText( pIn ) );
				}
				else
               SetTextSize( pIn, strlen( GetText( pIn ) ) );
				source->pParsed = burst( pIn );
			}
		}
		while( !source->pParsed );
		if( source->pParsed )
		{
			source->pToken = source->pParsed;
			while( source->pToken &&
			        !(source->pToken->flags & IS_DATA_FLAGS) && 
					  !GetTextSize( source->pToken ) )
			{
				source->pToken = NEXTLINE( source->pToken );
			}
  		if( !source->pToken )
  		{
  			goto restart;
  		}
		}
		else
		{
			source->pToken = source->pInput;
			source->pInput = NULL;
		}
	}
	else
	{
		source->pToken = NEXTLINE( source->pToken );
		// skip end of lines generated...
		while( source->pToken &&
		        !(source->pToken->flags & IS_DATA_FLAGS) && 
				  !GetTextSize( source->pToken ) )
		{
			source->pToken = NEXTLINE( source->pToken );
		}
		// ended the line?
		if( !source->pToken )
		{
			goto restart;
		}
	}	
	fprintf( out, "Returning: %s\n", GetText( source->pToken ) );
	return source->pToken;
}

char *Types[] = { "form", "bone", "body" };



int ProcessType( PFILEIN in, PTEXT pType, FILE* out )
{
	
	return 1;
}


int LoadBody( char *pName )
{
	PFILEIN in;
	FILE *out;
	in = OpenFileInput( pName );
	if( in )
	{
		PTEXT pTok;
		PTEXT pName, pType, pParams;
		out = fopen( "blah.output", "wt" );

		pName = get_token( in, out );
		if( !pName )
			return 0;
		pType = get_token( in, out );
		if( !pType )
			return 0;
/*
		if( !IsValidType( pType ) )
		{
			fprintf( out, "Invalid type specification in line...\n" );
			return 0;
		}	
*/
		pParams = get_token( in, out );

		{

			fprintf( out, "Name: %s  Type: %s ...\nParams: "
								, GetText( pName )
								, GetText( pType ) );
			{
				PTEXT pList, pExtra;
				pExtra = NULL;
				pList = burst( pParams );
				while( pList )
				{
					fprintf( out, "%s ", GetText( pList ) );
					pList = NEXTLINE( pList );
				}
				fprintf( out, "\n" );
				LineRelease( pList );
				LineRelease( pExtra );
			}
		} 	
		fclose( out );
		CloseFileInput( in );
	}
	return FALSE;
}

// $Log: loadbody.c,v $
// Revision 1.4  2003/03/25 09:41:17  panther
// Fix what CVS logging broke...
//
// Revision 1.3  2003/03/25 08:59:03  panther
// Added CVS logging
//
