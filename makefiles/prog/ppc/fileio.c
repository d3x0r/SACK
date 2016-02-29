#include <stdio.h>
#include <errno.h>

#include "mem.h"

#include "input.h"
#include "text.h"
#include "fileio.h"

#include "global.h"




//----------------------------------------------------------------------

// all files in Root->pAlso are top level dependancies.
static PFILEDEP FileDependancyRoot;


void FixSlashes( char *path )
{
	if( g.flags.bForceForeslash )
	{
		while( path[0] )
		{
			if( path[0] == '\\' )
				path[0] = '/';
			path++;
		}
	}
	else if( g.flags.bForceBackslash )
	{
		while( path[0] )
		{
			if( path[0] == '/' )
				path[0] = '\\';
			path++;
		}
	}
}

//----------------------------------------------------------------------

// unused thought the CPP main might use this ...
int CurrentFileDepend( void )
{
	if( g.pFileStack )
		return (int)g.pFileStack->pFileDep;
	return 0;
}

//----------------------------------------------------------------------

// unused thought the CPP main might use this ...
int CurrentFileDepth( void )
{
	int n = 0;
	PFILETRACK pft;
	for( pft = g.pFileStack; pft; n++, pft = pft->prior );
	return n;
}


//----------------------------------------------------------------------

void SetCurrentPath( char *path )
{
	strcpy( g.pCurrentPath.data.data, path );
	g.pCurrentPath.data.size = strlen( path );
}

//----------------------------------------------------------------------

PFILEDEP FindDependFile( PFILEDEP root, char *filename )
{
	PFILEDEP pDep = root, next;
	while( pDep )
	{
		next = pDep->pAlso;
		if( (!strcmp( pDep->base_name, filename )) ||
			 ( pDep = FindDependFile( pDep->pDependsOn, filename ) ) )
		{
			return pDep;
		}
		pDep = next;
	}
	return NULL;
}
//----------------------------------------------------------------------

LOGICAL AlreadyLoaded( char *filename )
{
	if( FindDependFile( FileDependancyRoot, filename ) )
		return TRUE;
	return FALSE;
}

//----------------------------------------------------------------------

PFILEDEP AddDepend( PFILEDEP root, char *basename, char *filename )
{
	PFILEDEP pfd = root;
	if( !root )
		root = FileDependancyRoot;
	fprintf( stderr, WIDE("Adding dependancy for: %s %s\n"), root?root->full_name:"Base File", filename );
	while( pfd && pfd->pDependedBy )
		pfd = pfd->pDependedBy;
	if( !( pfd = FindDependFile( pfd, basename ) ) )
	{
		pfd = Allocate( sizeof( FILEDEP ) );
		strcpy( pfd->base_name, basename );
		strcpy( pfd->full_name, filename );
		pfd->pAlso = NULL;
		pfd->pDependsOn = NULL;
		pfd->pDependedBy = NULL;
		if( root )
		{
			// this file is included by what?
			pfd->pDependedBy = root;
			// link this as another file depended file of root...
			pfd->pAlso = root->pDependsOn;
			root->pDependsOn = pfd;
		}
		else
		{
			pfd->pAlso = FileDependancyRoot;
			FileDependancyRoot = pfd;
		}
		{
			PFILEDEP pCheck = pfd->pDependedBy;
			int count = 1;
			while( pCheck )
			{
				if( strcmp( pCheck->full_name, pfd->full_name ) == 0 )
				{
					fprintf( stderr, WIDE("Check name matched...\n") );
					count++;
					if( count > 3 )
					{
						PFILEDEP pDump = pfd->pDependedBy;
						fprintf( stderr, WIDE("Possible header recursion: \'%s\'"), pfd->full_name );
						while( pDump && pDump != pCheck )
						{
							fprintf( stderr, WIDE(" included by \'%s\'"), pDump->full_name );
							pDump = pDump->pDependedBy;
						}
						fprintf( stderr, WIDE("\n") );
						fprintf( stderr, WIDE("Aborting processing.\n") );
						exit(0);
					}
				}
				pCheck = pCheck->pDependedBy;
			}
		}
		return pfd;
	}
	return pfd;
}

//----------------------------------------------------------------------

PFILEDEP AddFileDepend( PFILETRACK pft, char *basename, char *filename ) {
	AddDepend( pft->pFileDep, basename, filename );
}

//----------------------------------------------------------------------

void DumpDependLevel( PFILEDEP pfd, int level )
{
	PFILEDEP pDep = pfd->pDependsOn;
	if( pDep )
	{
		if( !level && g.AutoTargetName[0] && !pfd->pDependedBy )
		{
			fprintf( g.AutoDependFile, WIDE("%s:%s "), g.AutoTargetName, pfd->full_name );
		}
		//else if( level )
		//	fprintf( g.AutoDependFile, WIDE("%s "), pfd->name );

		//fprintf( g.AutoDependFile, WIDE("%s:"), pfd->name );
		while( pDep )
		{
			fprintf( g.AutoDependFile, WIDE("%s "), pDep->full_name );
			pDep = pDep->pAlso;
		}
		// for all files which this one depended on, 
		// dump the files those depend on...
		pDep = pfd->pDependsOn;
		while( pDep )
		{
			if( pDep->pDependsOn )
				DumpDependLevel( pDep, ++level );
			pDep = pDep->pAlso;
		}
		//fprintf( g.AutoDependFile, WIDE("\n") );
	}
}

//----------------------------------------------------------------------

void DumpDepends( void )
{
	int level = 0;
	// trace through root files....
	PFILEDEP pfd = FileDependancyRoot;
	if( !g.AutoDependFile )
		g.AutoDependFile = stdout;
	while( pfd )
	{
		// post increment to make sure we start at 0...
		DumpDependLevel( pfd, level++ );
		pfd = pfd->pAlso;
	}
  	fprintf( g.AutoDependFile, WIDE("\n") );
}

//----------------------------------------------------------------------

void DestroyDependLevel( PFILEDEP pfd )
{
	PFILEDEP pDep = pfd->pDependsOn, next;
	//if( pfd )
	//	fprintf( stderr, WIDE("destory level...%s \n"), pfd->name );
	if( pDep )
	{
		//pDep = pfd->pDependsOn;
		while( pDep )
		{
			next = pDep->pAlso;
			DestroyDependLevel( pDep );
			pDep = next;
		}
	}
	if( pfd )
		Release( pfd );
}

//----------------------------------------------------------------------

void DestoyDepends( void )
{
	PFILEDEP pfd = FileDependancyRoot, next;
	while( pfd )
	{
		next = pfd->pAlso;
		DestroyDependLevel( pfd );
		pfd = next;
	}
	FileDependancyRoot = NULL;
}

//----------------------------------------------------------------------

int GetCurrentLine( void )
{
	if( g.pFileStack )
		return g.pFileStack->nLine;
	return 0;
}

//----------------------------------------------------------------------

char *GetCurrentFileName( void )
{
	if( g.pFileStack )
		return g.pFileStack->longname;
	return "<Command Line>";
	//return NULL;
}

//----------------------------------------------------------------------

char *FixName( char *file )
{
	static char realname[__MAX_PATH__];
	if( file[0] != '/' && file[1] != ':' )
		sprintf( realname, WIDE("%s/%s"), g.pWorkPath, file );
	else
		strcpy( realname, file );

	return realname;
}

//----------------------------------------------------------------------

char *GetCurrentShortFileName( void )
{
	if( g.pFileStack )
		return g.pFileStack->name;
	return NULL;
}

//----------------------------------------------------------------------

void GetCurrentFileLine( char *name, int *line )
{
	if( name )
		strcpy( name, GetCurrentFileName() );
	if( line )
		*line = GetCurrentLine();
}

//----------------------------------------------------------------------

void WriteLineInfo( char *name, int line )
{
	FILE *out = GetCurrentOutput();
	static char  LastFileWritten[__MAX_PATH__];
	static int	LastLineWritten;
	if( out )
	{
		LastLineWritten++; 
		if( strcmp( name, LastFileWritten ) || // not match
			 line != LastLineWritten )
		{
			strcpy( LastFileWritten, GetCurrentFileName() );
			LastLineWritten = line;
			if( g.flags.bWriteLineInfo )
			{
			if( g.flags.bLineUsesLineKeyword )
			{
				fprintf( out, WIDE("#line %d \"%s\"\n")
						  //"//line %s(%d)\n"
						 , LastLineWritten
						 , LastFileWritten
						 );
			}
			else
			{
				// gcc is wonderful, eh?
				fprintf( out, WIDE("# %d \"%s\"\n")
						  //"//line %s(%d)\n"
						 , LastLineWritten
						 , LastFileWritten
						 );
			}
			}
		}
	}
}

//----------------------------------------------------------------------

void WriteCurrentLineInfo( void )
{
	WriteLineInfo( GetCurrentFileName(), GetCurrentLine() );
}

//----------------------------------------------------------------------

void WriteLine( int len, char *line )
{
	FILE *out = GetCurrentOutput();
	if( out )
	{
		fwrite( line, 1, len, out );
		fputc( '\n', out );
		//fflush( out );
	}
	//fprintf( out, WIDE("%s\n"), line );
}

//----------------------------------------------------------------------

PTEXT GetCurrentWord( void )
{
	if( g.pFileStack )
		return g.pFileStack->pNextWord;
	return NULL;
}

//----------------------------------------------------------------------

PTEXT *GetCurrentTextLine( void )
{
	if( g.pFileStack )
		return &g.pFileStack->pParsed;
	return NULL;
}

//----------------------------------------------------------------------

PTEXT GetNextWord( void )
{
	if( g.pFileStack )
		return g.pFileStack->pNextWord = NEXTLINE( g.pFileStack->pNextWord );
	return NULL;
}

//----------------------------------------------------------------------

void SetCurrentWord( PTEXT word )
{
	if( g.pFileStack->pParsed )
	if( g.pFileStack->pNextWord == g.pFileStack->pParsed )
		g.pFileStack->pParsed = word;
	g.pFileStack->pNextWord = word;
}

//----------------------------------------------------------------------

FILE *GetCurrentOutput(void)
{
	if( !g.flags.bNoOutput )
	{
		return g.output;
	}
	return NULL;
}
//----------------------------------------------------------------------

PTEXT StepCurrentWord( void )
{
	if( g.pFileStack )
		return g.pFileStack->pNextWord = NEXTLINE( g.pFileStack->pNextWord );
	return NULL;
}


//----------------------------------------------------------------------
// root level open.
int OpenInputFile( char *basename, char *file )
{
	PFILETRACK pft;
	FILE *fp;
	char *tmp;
	if( g.pFileStack )
	{
			fprintf( stderr, WIDE("warning: Already have a root level file open.") );
	}
	if( AlreadyLoaded( basename ) )
	{
		return 0;
	}
	fp = fopen( tmp = FixName(file), WIDE("rt") );
	if( fp )
	{
		pft = (PFILETRACK)Allocate( sizeof( FILETRACK ) );
		pft->bBlockComment = 0;
		pft->nLine = 0;
		pft->file = fp;
		pft->nIfLevel = g.nIfLevels;
		strcpy( pft->name, file );
		strcpy( pft->longname, tmp );
		FixSlashes( pft->longname );
		pft->line = NULL;
		//pft->output = NULL;
		pft->pParsed = NULL;
		pft->pNextWord = NULL;
		pft->prior = g.pFileStack;
		//fprintf( stderr, WIDE("Add in OpenInputFile\n") );
		pft->pFileDep = AddDepend( NULL, basename, file );
		g.pFileStack = pft;
	}
	else
		pft = NULL;
	return (int)pft;
}

//----------------------------------------------------------------------

int OpenNewInputFile( char *basename, char *name, char *pFile, int nLine, int bDepend, int bNext )
{
	PFILETRACK pft = g.pFileStack;
	PFILETRACK pftNew = NULL;
	FILE *fp;
	char *tmp;
	if( bNext )
	{
		PFILETRACK pftTest = g.pFileStack;
		while( pftTest )
		{
			if( strcmp( name, pftTest->name ) == 0 )
				return FALSE;
			pftTest = pftTest->prior;
		}
	}

	fp = fopen( tmp = FixName(name), WIDE("rt") );
	if( fp )
	{
		pftNew = (PFILETRACK)Allocate( sizeof( FILETRACK ) );
		pftNew->bBlockComment = 0;
		pftNew->nLine = 0;
		pftNew->file = fp;
		pftNew->nIfLevel = g.nIfLevels;
		strcpy( pftNew->name, name );
		strcpy( pftNew->longname, tmp );
		FixSlashes( pftNew->longname );

		// move the current line to the current file... (so we can log #include?)
		pftNew->line = NULL;

		pftNew->pNextWord = pft->pNextWord;
		pft->pNextWord = NULL;

		//pftNew->output = pft->output;
		pftNew->pParsed = NULL;
		pftNew->pNextWord = NULL;
		pftNew->prior = g.pFileStack;
		if( bDepend && ( pft->pFileDep ) )
		{
			//fprintf( stderr, WIDE("Add in OpenNewInputFile\n") );
			pftNew->pFileDep = AddDepend( pft->pFileDep, basename, name );
		}
		else
			pftNew->pFileDep = NULL;
		g.pFileStack = pftNew;
	}
	return (int)pftNew;
}

//----------------------------------------------------------------------

int OpenOutputFile( char *newfile )
{
	//PFILETRACK pft = g.pFileStack;
	//if( pft )
	{
		g.output = fopen( FixName( newfile ), WIDE("wt") );
		if( g.output )
		{
			return 1;
		}
	}
	return 0;
}

//----------------------------------------------------------------------

int OpenStdOutputFile( void )
{
	PFILETRACK pft = g.pFileStack;
	if( pft )
	{
		g.output = stdout;
		//if( pft->output )
		{
			return 1;
		}
	}
	return 0;
}

//----------------------------------------------------------------------

void CloseInputFileEx( DBG_VOIDPASS )
{
	PFILETRACK pft = g.pFileStack;
	if( pft->nIfLevel != g.nIfLevels )
	{
		fprintf( stderr, WIDE("Warning: Unmatched #if/#endif in %s (%d extra #if)\n")
				, pft->longname
				 , g.nIfLevels - pft->nIfLevel );
	}
	if( pft->file )
	{
		fclose( pft->file );
		pft->file = NULL;
	}
	if( !pft->prior )
	{
		//if( pft->output )
		{
			//fclose( pft->output );
			//pft->output = NULL;
		}
	}
	if( pft->line )
		LineRelease( pft->line );
	if( pft->pParsed )
		LineRelease( pft->pParsed );
	pft->line = NULL;
	pft->pParsed = NULL;
	g.pFileStack = pft->prior;
	ReleaseExx( (void**)&pft DBG_RELAY );
}

//----------------------------------------------------------------------


void CloseAllFiles( void )
{
	while( g.pFileStack )
		CloseInputFile();
}

//----------------------------------------------------------------------

PTEXT ReadLineEx( int Append DBG_PASS )
{
	PFILETRACK pft;
	PTEXT pNew;
Restart:
	pft = g.pFileStack;
	if( !pft  )
		return NULL;
	do
	{
		int bContinue;
		PTEXT current;
	GetNewLine: // loop this far when comments consume entire line...
	if( (pft->pParsed == pft->line) )
		DebugBreak();
		if( pft->line )
			LineReleaseEx( &pft->line DBG_RELAY );
		pft->line = NULL;
		if( !Append && ( pft->pParsed != pft->line ) )
		{
		  current = pft->pParsed;
			do {
				if( ( ( (PTRSZVAL)current)&0xFFFF0000) == (PTRSZVAL)0xDDDD0000U )
				{
					DebugBreak();
				}
				current = NEXTLINE( current );
			} while( current );
			if( pft->pParsed )
			{
				current = pft->pParsed;
				LineReleaseEx( &pft->pParsed DBG_RELAY );
			}
			pft->pNextWord = NULL;
		}
		bContinue = 0;
		do
		{
			pft->line = SegAppend( pft->line, current = get_line( pft->file, &pft->nLine ) );
			if( !current )
			{
				if( bContinue )
				{
					fprintf( stderr, WIDE("%s(%d) Warning: Continuation(\\) at end of file will continue to next file...\n"),
									GetCurrentFileName(), GetCurrentLine() );
				}
				CloseInputFile();
				goto Restart;
			}
			else
			{
				if( !current->format.spaces )
					current->format.spaces = 1;
			}
			if( current->data.data[current->data.size-1] == '\\' )
			{
				current->data.data[current->data.size = (current->data.size-1)] = 0;
				bContinue = 1;
			}
			else
				bContinue = 0;
		}while( pft && !pft->line && !bContinue );

		if( !pft || !pft->line )
		{
			if( g.bDebugLog & DEBUG_READING )
			{
				fprintf( stddbg, WIDE("Returning NULL line...\n") );
			}
			return NULL;
		}
		pNew = burst( pft->line );

		// strip blank lines...
		if( pNew && !GetTextSize( pNew ) )
		{
			if( g.bDebugLog & DEBUG_READING )
			{
				printf( WIDE("No content...") );
			}
			LineRelease( pNew );
			goto GetNewLine;
		}

		{
			PTEXT p, pStart = NULL;
			int quote  = 0; // set to current quote to match ... " or '
			int escape = 0; // if(quote) and prior == '\' skip " or ' chars
			int nSlash = 0;
			int nStar  = 0;
			int nLessthan = 0;
			int nGreaterthan = 0;
			int nPercent = 0;

			for( p = pNew; p; p = NEXTLINE( p ) )
			{
				char *pText;
			ContinueNoIncrement:
				pText = GetText( p );
				if( !pft->bBlockComment )
				{
					if( !quote )
					{
						if( pText[0] == '\'' )
						{
							quote = '\'';
							continue;
						}
						else if( pText[0] == '\"' )
						{
							quote = '\"';
							continue;
						}
					}
				}
				if( quote )
				{
					if( !escape )
					{
						if( pText[0] == '\\' )
							escape = 1;
						else if( pText[0] == quote )
							quote = 0;
					}
					else
						escape = 0;
				}
				else if( pText[0] == '/'  )
				{
					if( g.bDebugLog & DEBUG_READING )
					{
						fprintf( stddbg, WIDE("Have a slash...\n") );
					}
					if( nStar ) // leading stars up to close...
					{
						if( g.bDebugLog & DEBUG_READING )
						{
							fprintf( stddbg, WIDE("ending comment...\n") );
						}
						if( p->format.spaces )
						{
							nStar = 0;
							continue;
						}
						nStar = 0;
						if( !pft->bBlockComment )
						{
							// this may be the case - may also be a case of invalid paramters....
							// */ is an illegal operator combination anyhow....
							fprintf( stderr, WIDE("%s(%d) Warning: close block comment which was not started.\n")
											, pft->name, pft->nLine );
						}
						pft->bBlockComment = 0;
						if( pStart ) // began on this line.... ending here also
						{
							if( g.bDebugLog & DEBUG_READING )
							{
								fprintf( stddbg, WIDE("had a start of comment...\n") );
							}
							if( NEXTLINE( p ) )
							{
								p = NEXTLINE( p );
								SegBreak( p );
								if( g.flags.keep_comments )
								{
									PTEXT pOut;
									pOut = BuildLineEx( pStart, FALSE DBG_SRC );
									if( pOut )
									{
										if( g.flags.bWriteLine )
										{
											WriteCurrentLineInfo();
										}
										WriteLine( GetTextSize( pOut ), GetText( pOut ) );
										LineRelease( pOut );
									}
								}
								if( pStart != pNew )
									SegAppend( SegBreak( pStart ), p );
								else
								{
									pNew = p;
								}
								LineRelease( pStart );
								pStart = NULL;
								goto ContinueNoIncrement;
							}
							else
							{
								if( g.bDebugLog & DEBUG_READING )
								{
									fprintf( stddbg, WIDE("Trailing part of line was block comment...\n") );
								}
								// whole line is a block comment...
								if( pStart == pNew )
								{
									if( g.bDebugLog & DEBUG_READING )
									{
										fprintf( stddbg, WIDE("while line In block comment...") );
									}
									if( g.flags.keep_comments /*&&
										( !g.flags.bSkipSystemIncludeOut && !g.flags.doing_system_file )*/
									   )
									{
										PTEXT pOut;
										pOut = BuildLineEx( pNew, FALSE DBG_SRC );
										if( pOut )
										{
											if( g.flags.bWriteLine )
											{
												WriteCurrentLineInfo();
											}
											WriteLine( GetTextSize( pOut ), GetText( pOut ) );
											LineRelease( pOut );
										}
									}
									LineRelease( pNew );
									goto GetNewLine;
								}
								// else there is something before left...
								SegBreak( pStart );
								LineRelease( pStart );
								break; // loop ends anyway...
							}
						}
						else
						{
							// up to this point we have been in a block comment
							if( NEXTLINE( p ) )
							{
								p = NEXTLINE( p );
								SegBreak( p );
								if( g.flags.keep_comments )
								{
									PTEXT pOut;
									pOut = BuildLineEx( pNew, FALSE DBG_SRC );
									if( pOut )
									{
										if( g.flags.bWriteLine )
										{
											WriteCurrentLineInfo();
										}
										WriteLine( GetTextSize( pOut ), GetText( pOut ) );
										LineRelease( pOut );
									}
								}
								LineRelease( pNew );
								pNew = p;
								goto ContinueNoIncrement;
							}
							else
							{
								// entire line within block comment...
								//printf( WIDE("in block comment... ") );
								if( g.flags.keep_comments )
								{
									PTEXT pOut;
									pOut = BuildLineEx( pNew, FALSE DBG_SRC );
									if( pOut )
									{
										if( g.flags.bWriteLine )
										{
											WriteCurrentLineInfo();
										}
										WriteLine( GetTextSize( pOut ), GetText( pOut ) );
										LineRelease( pOut );
									}
								}
								LineRelease( pNew );
								goto GetNewLine;
							}
						}
					}

					if( !nSlash && !nStar &&
						( !pft->bBlockComment ) )
					{
						if( g.bDebugLog & DEBUG_READING )
						{
							fprintf( stddbg, WIDE("Marking begin...\n") );
						}
						pStart = p;
					}
					else
					{
						if( g.bDebugLog & DEBUG_READING )
						{
							fprintf( stddbg, WIDE("Pending states: %s%s%s\n")
									, nSlash?"nSlash ":""
									, nStar?"nStar ":""
									, pft->bBlockComment?"In block": "" );
						}
					}
					if( !(pft->bBlockComment ) )
					{
						if( nSlash )
						{
						  	if( p->format.spaces )
							{
								nSlash = 1; // reset/set count...
								continue;
							}
						}
						nSlash++;

						if( nSlash >= 2 )
						{
							PTEXT pout;

							if( pStart == pNew )
							{
								// releasing the whole line...
								//printf( WIDE("Whole line commented...\n") );
								if( g.flags.keep_comments )
								{
									PTEXT pOut;
									// throw in a newline, comments end up above the actual line
									// should force a #line indicator also?
									//SegAppend( pNew, SegCreate(0));
									pOut = BuildLineEx( pNew, FALSE DBG_SRC );
									if( pOut )
									{
										if( g.flags.bWriteLine )
										{
											WriteCurrentLineInfo();
										}
										WriteLine( GetTextSize( pOut ), GetText( pOut ) );
										LineRelease( pOut );
									}
								}
								// internally we keep pParsed which is what pNew is also...
								// pParsed will delete pNew
								//LineRelease( pNew );
								goto GetNewLine;
							}
							SegBreak( pStart );
							if( g.flags.keep_comments )
							{
								PTEXT pOut;
								// throw in a newline, comments end up above the actual line
								// should force a #line indicator also?
								pOut = BuildLineEx( pStart, FALSE DBG_SRC );
								if( pOut )
								{
									if( g.flags.bWriteLine )
									{
											WriteCurrentLineInfo();
									}
									WriteLine( GetTextSize( pOut ), GetText( pOut ) );
									LineRelease( pOut );
								}
							}
							LineRelease( pStart );
							break;
						}
					}
				}
				else if( pText[0] == '*' )
				{
					if( g.bDebugLog & DEBUG_READING )
					{
						fprintf( stddbg, WIDE("found a star... was there a slash?\n") );
					}
					if( nSlash == 1 ) // begin block comment
					{
						if( p->format.spaces )
						{
							nSlash = 0;
							nStar = 1; // set/reset count.
							continue;
						}
						if( g.bDebugLog & DEBUG_READING )
						{
							fprintf( stddbg, WIDE("okay defineatly block comment...\n") );
						}
						if( pft->bBlockComment )
							nStar++;
						else
							pft->bBlockComment = TRUE;
					// pStart should point to the beginning '/' already
					// /*/ is not a valid comment but /**/ is.
					}
					else
					{
						if( p->format.spaces )
						{
						 	nStar = 1; // set/reset count.
						 	continue;
						}
						if( g.bDebugLog & DEBUG_READING )
						{
							fprintf( stddbg, WIDE("Adding another star...\n") );
						}
						nStar++; // this is beginning of end block comment maybe
					}
					// begin block comment....
				}
				else // character is neither a '/' or a '*'
				{
					nSlash = 0;
					nStar = 0;
				}
			}

			if( pft->bBlockComment ) // fell off end of line without a close on this.
			{
				if( g.bDebugLog & DEBUG_READING )
				{
					fprintf( stddbg, WIDE("Daning block comment - continue reading...\n") );
				}
				if( pStart )
				{
					// began a block comment, but it continues....
					if( pStart == pNew )
					{
						//printf( WIDE("In Block comment(3)...\n") );
						if( g.flags.keep_comments )
						{
							PTEXT pOut;
							pOut = BuildLineEx( pNew, FALSE DBG_SRC );
							if( pOut )
							{
								if( g.flags.bWriteLine )
								{
									WriteCurrentLineInfo();
								}
								WriteLine( GetTextSize( pOut ), GetText( pOut ) );
								LineRelease( pOut );
							}
						}
						LineRelease( pNew );
						goto GetNewLine; // this block started here and was whole line
					}
					else
					{
						PTEXT pOut;
						SegBreak( pStart );

						pOut = BuildLineEx( pNew, FALSE DBG_SRC );
						if( pOut )
						{
							if( g.flags.bWriteLine )
							{
								WriteCurrentLineInfo();
							}
							WriteLine( GetTextSize( pOut ), GetText( pOut ) );
							LineRelease( pOut );
						}
						LineRelease( pNew );
						pNew = pStart;
					}
				if( !g.flags.keep_comments ) {
					SegBreak( pStart );
					LineRelease( pStart );
				}
				/*
					if( g.flags.keep_comments )
					{
						PTEXT pOut;
						pOut = BuildLineEx( pStart, FALSE DBG_SRC );
						if( pOut )
						{
							if( g.flags.bWriteLine )
							{
								WriteCurrentLineInfo();
							}
							WriteLine( GetTextSize( pOut ), GetText( pOut ) );
							LineRelease( pOut );
						}
					}
					*/
				}
				else
				{
				  	//printf( WIDE("In Block comment(3)...\n") );
					// ignore this line completely!
					if( g.flags.keep_comments )
					{
						PTEXT pOut;
						pOut = BuildLineEx( pNew, FALSE DBG_SRC );
						if( pOut )
						{
							if( g.flags.bWriteLine )
							{
								WriteCurrentLineInfo();
							}
							WriteLine( GetTextSize( pOut ), GetText( pOut ) );
							LineRelease( pOut );
						}
					}
					LineRelease( pNew );
					goto GetNewLine;
				}
			}
		}
	}while( !pNew );
	//printf( WIDE("Adding %lp to %lp\n"), pNew, pft->pParsed );
	pft->pParsed = SegAppend( pft->pParsed, pNew );
	if( !Append )
		pft->pNextWord = pNew;
	if( g.bDebugLog & DEBUG_READING )
	{
		fprintf(stddbg, WIDE("Readline result: ") );
		DumpSegs( pNew );
		fprintf( stddbg, WIDE("\n") );
	}
	return pNew;
}


char *pathrchr( char *path )
{
	char *end1, *end2;
	end1 = strrchr( path, '\\' );
	end2 = strrchr( path, '/' );
	if( end1 > end2 )
		return end1;
	return end2;
}

//-----------------------------------------------------------------------

char *pathchr( char *path )
{
	char *end1, *end2;
	end1 = strchr( path, '\\' );
	end2 = strchr( path, '/' );
	if( end1 && end2 )
	{
		if( end1 < end2 )
			return end1;
		return end2;
	}
	else if( end1 )
		return end1;
	else if( end2 )
		return end2;
	return NULL;
}



