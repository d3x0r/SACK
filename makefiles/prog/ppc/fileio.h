#ifndef FILEIO_DEFINED
#define FILEIO_DEFINED

#include <stdio.h>
#include "text.h"

#define __MAX_PATH__ 256



int OpenInputFile( char *file );
int OpenOutputFile( char *newfile );
int OpenStdOutputFile( void );
int OpenNewInputFile( char *name, char *pFile, int nLine, int bDepend, int bNext );
void CloseInputFileEx( DBG_VOIDPASS );
#define CloseInputFile() CloseInputFileEx( DBG_VOIDSRC )


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

PTEXT ReadLineEx( int Append DBG_PASS );
#define ReadLine(a) ReadLineEx(a DBG_SRC)

void WriteLineInfo( char *file, int line );
void WriteCurrentLineInfo( void );

void WriteLine( int len, char *line );

void DumpDepends( void );
void DestoyDepends( void );

char *pathrchr( char *path );
char *pathchr( char *path );

#endif
