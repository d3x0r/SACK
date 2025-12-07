#ifndef FILEIO_DEFINED
#define FILEIO_DEFINED

#include <stdio.h>
#include "sack_ucb_filelib.h"

#define __MAX_PATH__ 256


typedef struct file_tracking_tag
{
	int nLine;
	char name[__MAX_PATH__]; // if I use malloc this dynamic member failed ...
	char longname[__MAX_PATH__]; // if I use malloc this dynamic member failed ...
	FILE *file;
	PTEXT line; // last line read...
	PTEXT pParsed; // last line parsed
	PTEXT pNextWord; // next token to be used...
	struct file_tracking_tag *prior; // stack...
	struct file_dependancy_tag *pFileDep;
	int  bBlockComment; // state remains multi-lines
	int nIfLevel; // level of ifs started when this file is opened.
				  // -- state tracking per file --
				  /*
				  int nState;

				  int nIfLevels;  // count up and down always...
				  int nIfLevelElse; // what level to find the else on...
				  */
} FILETRACK, *PFILETRACK;


typedef struct file_dependancy_tag
{
	char full_name[__MAX_PATH__];
	char base_name[__MAX_PATH__];
	int bAllowMultipleInclude;
	struct file_dependancy_tag *pDependedBy  // what file included this
		, *pDependsOn // first file this one depends on
		, *pAlso;  // next file which pDepended by depends on
} FILEDEP, *PFILEDEP;


void PPC_SetCurrentPath( char *path );
uintptr_t OpenInputFile( char *basename, char *file );
uintptr_t OpenOutputFile( char *newfile );
uintptr_t OpenStdOutputFile( void );
uintptr_t OpenNewInputFile( char *basename, char *name, char *pFile, int nLine, int bDepend, int bNext );
void CloseInputFileEx( DBG_VOIDPASS );
#define CloseInputFile() CloseInputFileEx( DBG_VOIDSRC )

PFILEDEP AddFileDepend( PFILETRACK pft, char *basename, char *filename );

LOGICAL AlreadyLoaded( char *filename );

void SetIfBegin( void );
void ClearIfBegin( void );
void GetIfBegin( char **file, int *line );

FILE *GetCurrentOutput(void);

int GetCurrentLine( void );
char *GetCurrentFileName( void );
char *GetCurrentShortFileName( void );
void GetCurrentFileLine( char *name, int *line );
int CurrentFileDepth( void ); // how many files deep we're processing

PTEXT GetCurrentWord( void );
PTEXT *GetCurrentTextLine( void );
PTEXT StepCurrentWord( void );
PTEXT GetNextWord( void );
void  SetCurrentWord( PTEXT word );
//char *pathrchr( char *path );

PTEXT ReadLineEx( int Append DBG_PASS );
#define ReadLine(a) ReadLineEx(a DBG_SRC)

void WriteLineInfo( char *file, int line );
void WriteCurrentLineInfo( void );

void WriteLine( size_t len, char *line );

void DumpDepends( void );
void DestoyDepends( void );

//char *pathrchr( char *path );
//char *pathchr( char *path );

//---------- actually is args.c; but it's only the one function......
//void ParseIntoArgs( char *lpCmdLine, int *pArgc, char ***pArgv );

#endif
