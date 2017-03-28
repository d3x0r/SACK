#if defined( __WINTIME__ )
#include <windows.h>
#endif
#include <stdlib.h>
#include "mem.h"
#include "define.h"
#include "global.h"

#include <time.h>

//#define DEFINE_STDC_VERSION


#define DEBUG_SUBST 0x02
#define DEBUG_DEFINES 0x04



static PDEF pDefineRoot, pCurrentDefine;
static DEF DefineLine  // static symbol....
, DefineFile // static symbol....
, DefineStdCVersion // 199901L
, DefineTime        // "00:00:00.000"
, DefineDate;       // "mmm dd, yyyy"
static int nDefines;


//----------------------------------------------------------------------
#ifdef _DEBUG
void ValidateTree( PDEF *root )
{
	if( root && *root )
	{
		if( (*root)->me != root )
		{
			fprintf( stderr, WIDE("Invalid linking this->me is not itself!\n") );
			DebugBreak();
		}
		if( (*root)->pGreater )
		{
			if( (*root)->pGreater->me != &(*root)->pGreater )
			{
				fprintf( stderr, WIDE("Invalid linking this->greater does not reference me\n") );
				DebugBreak();
			}
			ValidateTree( &(*root)->pGreater );
		}
		if( (*root)->pLesser )
		{
			if( (*root)->pLesser->me != &(*root)->pLesser )
			{
				fprintf( stderr, WIDE("Invalid linking this->lesser does not reference me\n") );
				DebugBreak();
			}
			ValidateTree( &(*root)->pLesser );
		}
		if( (*root)->pSame )
		{
			if( (*root)->pSame->me != &(*root)->pSame )
			{
				fprintf( stderr, WIDE("Invalid linking this->same does not reference me\n") );
				DebugBreak();
			}
			ValidateTree( &(*root)->pSame );
		}
	}
}
#else
#define ValidateTree(r)
#endif
#define ValidateTree(r)

//----------------------------------------------------------------------

void FixQuoting( PTEXT test )
{
	while( test )
	{
		if( GetText( test )[0] == '\"' )
		{
			PTEXT insert = SegCreateFromText( WIDE("\\") );
			insert->format.spaces = test->format.spaces;
			test->format.spaces = 0;
			SegInsert( insert, test );
		}
		else if( GetText( test )[0] == '\'' )
		{
			PTEXT insert = SegCreateFromText( WIDE("\\") );
			insert->format.spaces = test->format.spaces;
			test->format.spaces = 0;
			SegInsert( insert, test );
		}
		else if( GetText( test )[0] == '\\' )
		{
			PTEXT insert = SegCreateFromText( WIDE("\\") );
			insert->format.spaces = test->format.spaces;
			test->format.spaces = 0;
			SegInsert( insert, test );
		}
		test = NEXTLINE( test );
	}
}

//----------------------------------------------------------------------

void DeleteDefineContent( PDEF p )
{
	if( p->pName )
		LineRelease( p->pName );
	if( p->pData )
		LineRelease( p->pData );
	{
		INDEX idx;
		PTEXT param;
		FORALL( p->pParams, idx, PTEXT, param )
		{
			LineRelease( param );
		}
		DeleteList( p->pParams );
	}
}

//----------------------------------------------------------------------

void DeleteStaticDefines( void )
{
	DeleteDefineContent( &DefineLine );
	DeleteDefineContent( &DefineFile );
#ifdef DEFINE_STDC_VERSION
	DeleteDefineContent( &DefineStdCVersion );
#endif
	DeleteDefineContent( &DefineDate );
	DeleteDefineContent( &DefineTime );
}

//----------------------------------------------------------------------

void InitDefines( void )
{
#if defined( __UNIXTIME__ )
	struct tm tm;
	time_t now;
	now = time( NULL );
	localtime_r( &now, &tm );
#elif defined( __WINTIME__ )
	SYSTEMTIME st;
	GetLocalTime( &st );
#endif

	{
		DECLTEXT( constname, WIDE("__LINE__") );
		DefineLine.pName = (PTEXT)&constname;
		DefineLine.pParams = NULL;
		DefineLine.bUsed = FALSE;
		DefineLine.pLesser = NULL;
		DefineLine.pGreater = NULL;
	}
	{
		DECLTEXT( constname, WIDE("__FILE__") );
		DefineFile.pName = (PTEXT)&constname;
		DefineFile.pParams = NULL;
		DefineFile.bUsed = FALSE;
		DefineFile.pLesser = NULL;
		DefineFile.pGreater = NULL;
	}
	//if( TextIs( pName, WIDE("__TIME__ ") ) ) // hh:mm:ss
	{
		DECLTEXT( constname, WIDE("__TIME__") );
		char time[15];
#if defined( __UNIXTIME__ )
		strftime( time, 15, WIDE("\"%H:%M:%S\""), &tm );
#elif defined( __WINTIME__ )
		snprintf( time, 15, WIDE("\"%2d:%02d:%02d.%03d\"")
				  , st.wHour, st.wMinute
				  , st.wSecond, st.wMilliseconds );
#endif
		DefineTime.pName = (PTEXT)&constname;
		DefineTime.pData = SegCreateFromText( time );
		DefineTime.pData->format.spaces = 1;
		DefineTime.pParams = NULL;
		DefineTime.bUsed = FALSE;
		DefineTime.pLesser = NULL;
		DefineTime.pGreater = NULL;
	}
	//if( TextIs( pName, WIDE("__DATE__ ") ) ) // Mmm dd yyyy dd = ' x' if <10
	{
		DECLTEXT( constname, WIDE("__DATE__") );
		char date[20];
#if defined( __UNIXTIME__ )
		strftime( date, 20, WIDE("\"%b %e %Y\""), &tm );
#elif defined( __WINTIME__ )
		snprintf( date, 20, WIDE("\"%02d/%02d/%04d\"")
				  , st.wMonth, st.wDay, st.wYear );
#endif
		DefineDate.pName = (PTEXT)&constname;
		DefineDate.pData = SegCreateFromText( date );
		DefineDate.pData->format.spaces = 1;
		DefineDate.pParams = NULL;
		DefineDate.bUsed = FALSE;
		DefineDate.pLesser = NULL;
		DefineDate.pGreater = NULL;
	}
	// on GCC - these end up being defined....
	//if( TextIs( pName, WIDE("__STDC__ ") ) ) // 1
	//if( TextIs( pName, WIDE("__STDC_HOSTED__ ") ) ) // ?? what's a hosted?
	// and this - well the compiler itself needs to support this - so
	// this should come in from the
#ifdef DEFINE_STDC_VERSION
	{
		DECLTEXT( constname, WIDE("__STDC_VERSION__") );
		DefineStdCVersion.pParams = NULL;
		DefineStdCVersion.pName = (PTEXT)&constname;
		DefineStdCVersion.pData = SegCreateFromInt( 199901L );
		DefineStdCVersion.pData->format.spaces = 1;
		DefineStdCVersion.bUsed = FALSE;
		DefineStdCVersion.pLesser = NULL;
		DefineStdCVersion.pGreater = NULL;
	}
#endif
}

//----------------------------------------------------------------------

void DeinitDefines( void )
{
	DeleteStaticDefines();
}

//----------------------------------------------------------------------

// if params are negative - the count is -(absolute count)
//  
PDEF FindDefineName( PTEXT pName, int params )
{
	PDEF p;
	int levels = 1;
	if( TextIs( pName, WIDE("__LINE__") ) )
	{
		if( DefineLine.pData )
			LineRelease( DefineLine.pData );
		DefineLine.pData = SegCreateFromInt( GetCurrentLine() );
		DefineLine.bUsed = FALSE;
		return &DefineLine;
	}
	else if( TextIs( pName, WIDE("__FILE__") ) )
	{
		VARTEXT vt;
		VarTextInit( &vt );
		if( DefineFile.pData )
			LineRelease( DefineFile.pData );
		//printf( WIDE("Building result: %s\n"), GetCurrentFileName() );
		VarTextAddCharacter( &vt, '\"' );
		VarTextEnd( &vt );
		vtprintf( &vt, WIDE("%s"), GetCurrentShortFileName() );
		VarTextEnd( &vt );
		VarTextAddCharacter( &vt, '\"' );
		DefineFile.pData = VarTextGet( &vt );
		VarTextEmpty( &vt );
		DefineFile.bUsed = FALSE;
		return &DefineFile;
	}
	//if( TextIs( pName, WIDE("__TIME__ ") ) ) // hh:mm:ss
	else if( TextIs( pName, WIDE("__TIME__") ) )
	{
		DefineTime.bUsed = FALSE;
		return &DefineTime;
	}
	//if( TextIs( pName, WIDE("__DATE__ ") ) ) // Mmm dd yyyy dd = ' x' if <10
	else if( TextIs( pName, WIDE("__DATE__") ) )
	{
		DefineDate.bUsed = FALSE;
		return &DefineDate;
	}

	// This is probably compiler specific.... but let's see what
	// happens for now.
#ifdef DEFINE_STDC_VERSION
	else if( TextIs( pName, WIDE("__STDC_VERSION__") ) )  // 199901L
	{
		return &DefineStdCVersion;
	}
#endif
	// all of these are compiler specific....
	//if( TextIs( pName, WIDE("__STDC__ ") ) ) // 1
	//if( TextIs( pName, WIDE("__STDC_HOSTED__ ") ) ) // ?? what's a hosted?
	//if( TextIs( pName, WIDE("__STDC_IEC_559__ ") ) ) // 1 if conform annex f
	//if( TextIs( pName, WIDE("__STDC_IEC_559_COMPLEX__ ") ) ) // 1 if conform annex g
	//if( TextIs( pName, WIDE("__STDC_ISO_10646__ ") ) ) // 199712L yyyymmL wchar_t encodings
	// do not allow __cplusplus to redefine...

	p = pDefineRoot;
	while( p )
	{
		int d = SameText( pName, p->pName );
		if(  d == 0 )
		{
			// just checking to see if defined...
			if( params == IGNORE_PARAMS &&
				 !p->bUsed )
				return p;
			do
			{
				if( params < 0 )
				{
					// looking for macro with var args...
					if( p->bVarParams &&
						p->pParams->Cnt == -params )
					{
						return p; // same one...
					}
				}

				if( p->pParams )
				{	
					if( p->pParams->Cnt == params 
						|| ( p->bVarParams 
						 && ( (int)p->pParams->Cnt <= params ) )
					  )
					{
						if( !p->bUsed )
							return p;
						// otherwise - maybe we can use the next one?
					}
				}
				else if( !params )
				{
					if( !p->bUsed )
						return p;
				}

				p = p->pSame;
			}		 
			while( p );
			if( p )
				break;
		}
		else if( d > 0 )
			p = p->pGreater;
		else
			p = p->pLesser;
		levels++;
	}
	//if( g.bDebugLog )
	//   fprintf( stderr, WIDE("levels checked for name: %d/%d\n"), levels, nDefines );
	return NULL;
}

//----------------------------------------------------------------------

// inserts pDef where p is, pushing p to be under pDef as pSame
void InsertDefine( PDEF pDef, PDEF p )
{
	*p->me = pDef;
	pDef->me = p->me;
	if( pDef->pGreater = p->pGreater )
		pDef->pGreater->me = &pDef->pGreater;
	if( pDef->pLesser = p->pLesser )
		pDef->pLesser->me = &pDef->pLesser;
	pDef->pSame = p;
	p->me = &pDef->pSame;
	p->pGreater = NULL;
	p->pLesser = NULL;

}

//----------------------------------------------------------------------


void HangNode( PDEF *root, PDEF pDef );

void DeleteDefine( PDEF *ppDef )
{
	if( ppDef && *ppDef )
	{
		PDEF pDef = *ppDef;
		PDEF tmp;
		// this ends up clearing &pDef usually....
		tmp = pDef;
		// take me out of the tree...
		*pDef->me = NULL;

		if( pDef->pSame )
		{
			pDef->pSame->me = NULL;
			HangNode( &pDefineRoot, pDef->pSame );
			pDef->pSame = NULL;
		}
		if( pDef->pLesser )
		{
			pDef->pLesser->me = NULL;
			HangNode( &pDefineRoot, pDef->pLesser );
			pDef->pLesser = NULL;
		}
		if( pDef->pGreater )
		{
			pDef->pGreater->me = NULL;
			HangNode( &pDefineRoot, pDef->pGreater );
			pDef->pGreater = NULL;
		}
		DeleteDefineContent( pDef );
		Release( tmp );
		ValidateTree( &pDefineRoot );
		nDefines--;
	}
}

//----------------------------------------------------------------------

void HangNode( PDEF *root, PDEF pDef )
{
	PDEF p, prior = NULL;
	if( !pDef )	
		return;
	// allow definition of internal __DATE__ and __TIME__
	if( TextIs( pDef->pName, WIDE("__DATE__") ) )
	{
		if( pDef->nType == DEFINE_COMMANDLINE )
		{
			LineRelease( DefineDate.pData );
			DefineDate.pData = pDef->pData;
			pDef->pData = NULL;
			DeleteDefine( &pDef );
		}
		else
		{
			fprintf( stderr, WIDE("%s(%d): May only define __DATE__ from a command line parameter\n")
					 , GetCurrentFileName()
					 , GetCurrentLine()
					 );
			DeleteDefine( &pDef );
		}
		return;
	}
	else if( TextIs( pDef->pName, WIDE("__TIME__") ) )
	{
		if( pDef->nType == DEFINE_COMMANDLINE )
		{
			LineRelease( DefineTime.pData );
			DefineTime.pData = pDef->pData;
			pDef->pData = NULL;
			DeleteDefine( &pDef );
		}
		else
		{
			fprintf( stderr, WIDE("%s(%d): May only define __TIME__ from a command line parameter\n")
					 , GetCurrentFileName()
					 , GetCurrentLine()
					 );
			DeleteDefine( &pDef );
		}
		return;
	}

	if( !*root )
	{
		*root = pDef;
		pDef->me = root;
		ValidateTree( &pDefineRoot );
		return;
	}
	p = *root;
	while( p )
	{
		int d = SameText( pDef->pName, p->pName );;
		if( d > 0 )
		{
			if( !p->pGreater )
			{
				pDef->me = &p->pGreater;
				p->pGreater = pDef;
				ValidateTree( &pDefineRoot );
				return;
			}
			else
			{
				prior = p;
				p = p->pGreater;
			}
		}
		else if( d < 0 )
		{
			if( !p->pLesser )
			{
				pDef->me = &p->pLesser;
				p->pLesser = pDef;
				ValidateTree( &pDefineRoot );
				return;
			}
			else
			{
				prior = p;
				p = p->pLesser;
			}
		}
		else
		{
			if( pDef->pParams )
			{  // has parameters () - may be 0, but still...
				// nparams...
				do
				{
					if( p->pParams )
					{
						if( pDef->pParams->Cnt < p->pParams->Cnt )
						{
							InsertDefine( pDef, p );
							ValidateTree( &pDefineRoot );
							return;
						}
						else if( pDef->pParams->Cnt == p->pParams->Cnt )
						{
							if( p->bVarParams )
							{
								if( pDef->bVarParams )
								{
									fprintf( stderr, WIDE("%s(%d): Error attempt to redefine macro %s defined at %s(%d).\n")
											  , GetCurrentFileName()
											  , GetCurrentLine()
											  , GetText( pDef->pName )
											  , p->pFile, p->nLine );
									g.ErrorCount++;
									DeleteDefine( &pDef );
									ValidateTree( &pDefineRoot );
									return;
								}
								InsertDefine( pDef, p );
								ValidateTree( &pDefineRoot );
								return;
							}
							else
							{
								if( !pDef->bVarParams )
								{
									fprintf( stderr, WIDE("%s(%d): Error attempt to redefine macro %s defined at %s(%d).\n")
											  , GetCurrentFileName()
											  , GetCurrentLine()
											  , GetText( pDef->pName )
											  , p->pFile, p->nLine );
									g.ErrorCount++;
									DeleteDefine( &pDef );
									ValidateTree( &pDefineRoot );
									return;
								}
								// else step to next ( need to add after this)
							}				
						}
					}
					prior = p;
					p = p->pSame;
				} while( p );
				pDef->me = &prior->pSame;
				prior->pSame = pDef;
				ValidateTree( &pDefineRoot );
				return;
			}
			else
			{
				// no paramters - symbol alone...
				if( !p->pParams )
				{
					/*
					if( !pDef->pParams )
					{
						fprintf( stderr, WIDE("%s(%d): WARNING Attempt to Redefine macro with no params - failed.!\n")
								, GetCurrentFileName()
								, GetCurrentLine() );
						DeleteDefine( &pDef );
						ValidateTree( &pDefineRoot );
						return;
					}
					else
					*/
						fprintf( stderr, WIDE("%s(%d): WARNING Redefining macro \'%s\'(overloading name alone with params)!\n")
								, GetCurrentFileName()
								, GetCurrentLine() 
								, GetText( pDef->pName )
								);
				}
				InsertDefine( pDef, p );
				ValidateTree( &pDefineRoot );
				return;
			}
		}
	}
}

//----------------------------------------------------------------------

void DeleteDefineTree( PDEF *ppRoot, int type )
{
	if( ppRoot && *ppRoot )
	{
		PDEF root = *ppRoot;
		DeleteDefineTree( &(root->pLesser), type );
		DeleteDefineTree( &(root->pGreater), type );
		DeleteDefineTree( &(root->pSame), type );
		if( type == DEFINE_ALL ||
			 root->nType == type )
			DeleteDefine( ppRoot );
		ValidateTree( &pDefineRoot );
	}
}

//----------------------------------------------------------------------

void DeleteAllDefines( int type )
{
	// uhmm this works - but it's hideous...
	// should do something like delete bottom up
	DeleteDefineTree( &pDefineRoot, type );
}

//----------------------------------------------------------------------

void DefineDefine( char *name, char *value )
{
	PDEF pDefine = Allocate( sizeof( DEF ) );
	MemSet( pDefine, 0, sizeof( DEF ) );
	pDefine->nType = DEFINE_COMMANDLINE;
	pDefine->pName = SegCreateFromText( name );
	pDefine->pData = SegCreateFromText( value );
	if( pDefine->pData )
		pDefine->pData->format.spaces = 1;
	HangNode( &pDefineRoot, pDefine );

}

//----------------------------------------------------------------------

int ProcessDefine( int type )
{
	//PTEXT def;
	if( !pCurrentDefine )
	{
		PTEXT pWord = GetCurrentWord();
		if( TextIs( pWord, WIDE("__LINE__") )
			|| TextIs( pWord, WIDE("__FILE__") )
#ifdef DEFINE_STDC_VERSION
			|| TextIs( pWord, WIDE("__STDC_VERSION__") )
#endif
		  )
			 // none of the above constants mentioned may be redefined
			 // nor 'defined'
		{
			if( g.bDebugLog )
			{
				fprintf( stddbg, WIDE("%s(%d) Warning: Cannot define predefined symbols.")
									, GetCurrentFileName(), GetCurrentLine() );
			}
			fprintf( stderr, WIDE("%s(%d) Warning: Cannot define predefined symbols.")
								, GetCurrentFileName(), GetCurrentLine() );
			return TRUE; // just a warning
		}
		pCurrentDefine = Allocate( sizeof( DEF ) );
		MemSet(pCurrentDefine, 0, sizeof( DEF ) );
		strcpy( pCurrentDefine->pFile, GetCurrentFileName() );
		pCurrentDefine->nLine = GetCurrentLine();
		pCurrentDefine->nType = type;
		pWord = GetCurrentWord();
		if( g.bDebugLog & DEBUG_DEFINES )
		{
			fprintf( stddbg, WIDE("%s(%d) Defining macro: %s\n")
							, GetCurrentFileName(), GetCurrentLine()
							, GetText( pWord ) );
		}

		pCurrentDefine->pName = SegDuplicate( pWord );
		if( ( NEXTLINE( pWord ) &&
			 ( NEXTLINE( pWord )->format.spaces == 0 ) ) )
		{
			pWord = StepCurrentWord();
			if( GetText( pWord )[0] == '(' )
			{
				// these parameters are macros vars for define...
				// required count....
				pCurrentDefine->pParams = CreateList();
				StepCurrentWord();
				while( ( pWord = GetCurrentWord() ) &&
						 GetText( pWord )[0] != ')' )
				{
					if( GetText( pWord )[0] != ',' )
					{
						if( TextIs( pWord, WIDE("...") ) )
						{
							if( g.bDebugLog & DEBUG_DEFINES )
								fprintf( stderr, WIDE("Adding var args...\n") );
							if( pCurrentDefine->bVarParams )
							{
								fprintf( stderr, WIDE("%s(%d): Duplicate \'...\' used in define definition.\n")
											, GetCurrentFileName()
											, GetCurrentLine() );
							}
							else
								pCurrentDefine->bVarParams = TRUE;
						}
						else
						{
							PTEXT param = NULL;
							INDEX idx;
							if( !pCurrentDefine->bVarParams )
							{
								if( g.bDebugLog & DEBUG_DEFINES )
									fprintf( stddbg, WIDE("Adding argument: %s\n"), GetText( pWord ) );
								FORALL( pCurrentDefine->pParams, idx, PTEXT, param )
								{
									if( SameText( param, pWord ) == 0 )
									{
										fprintf( stderr, WIDE("%s(%d): Error same parameter name defined twice.\n")
												 , GetCurrentFileName()
												 , GetCurrentLine() );
										g.ErrorCount++;
									}
									param = NULL; // have to manual reset this...
								}
								if( !param )
								{
									AddLink( pCurrentDefine->pParams
											 , SegDuplicate( pWord ) );
								}
								else if( g.bDebugLog & DEBUG_DEFINES )
									fprintf( stddbg, WIDE("Parameter already existed???\n") );
							}
							else
							{
								fprintf( stderr, WIDE("%s(%d): regular parameters are not allowed to follow var args (...)\n")
										 , GetCurrentFileName()
										 , GetCurrentLine() );
							}
						}
					}
					StepCurrentWord();
				}
			}
		}
	  	StepCurrentWord();
	}
	//----------------
	// add data to the current macro here...
	//----------------
	if( GetCurrentWord() )
	{
		PTEXT p;
		p = GetCurrentWord();
		while( p )
		{
			PTEXT newseg = SegDuplicate( p );
			if( p == GetCurrentWord() )
				newseg->format.spaces = 0;
			pCurrentDefine->pData = SegAppend( pCurrentDefine->pData
													  , newseg );
			p = NEXTLINE(p);
		}
		pCurrentDefine->pData->format.spaces = 0;
	}
	if( g.bDebugLog & DEBUG_DEFINES )
	{
		PTEXT out = BuildLine( pCurrentDefine->pData );
		fprintf( stddbg, WIDE("Define %s == %s\n"), GetText( pCurrentDefine->pName ), GetText( out ) );
		LineRelease( out );
	}
	if( g.bDebugLog & DEBUG_SUBST )
	{
		fprintf( stderr, WIDE("searching for %s(%zd) ... \n")
				 ,GetText( pCurrentDefine->pName)
				 , pCurrentDefine->pParams
				  ? pCurrentDefine->bVarParams
				  ?-(int)pCurrentDefine->pParams->Cnt
				  :pCurrentDefine->pParams->Cnt
				  : 0 );
	}

	{
		PDEF pOld = FindDefineName( pCurrentDefine->pName
										  , pCurrentDefine->pParams
											? pCurrentDefine->bVarParams
											?-(int)pCurrentDefine->pParams->Cnt
											:(int)pCurrentDefine->pParams->Cnt
											: 0 );
		if( pOld )
		{
			if( g.flags.bAllWarnings )
				fprintf( stderr, WIDE("%s(%d) Warning: redefining symbol: %s Previously defined at %s(%d).\n")
						, GetCurrentFileName(), GetCurrentLine()
						, GetText( pCurrentDefine->pName )
						, pOld->pFile, pOld->nLine );
			DeleteDefine( &pOld );
		}
		//else if( g.bDebugLog )
		//{
		//	fprintf( stderr, WIDE("Symbol not found - continue normall.\n") );
		//}

	}
	// completed the macro definition here....
	nDefines++;
	HangNode( &pDefineRoot, pCurrentDefine );
	pCurrentDefine = NULL;
	if( g.bDebugLog & DEBUG_DEFINES )
		fprintf( stderr, WIDE("done with define...\n") );
	return TRUE;
}

//----------------------------------------------------------------------

PTEXT BuildSizeofArgs( PTEXT args )
{
	PTEXT arg = NULL
		 , trinary_then = NULL
		 , trinary_else = NULL;
	PTEXT result = NULL;
	char *text;
	int quote = 0
	  , paren = 0
	  , trinary_collect_then = 0
	  , trinary_collect_else = 0;
	for( ; args; args = NEXTLINE( args ) )
	{
		text = GetText( args );
		if( !quote )
		{
			if( text[0] == '\"' ||
				text[0] == '\'' )
			{
				quote = text[0];
				continue;
			}
			if( text[0] == '(' )
				paren++;
			else if( text[0] == ')' )
				paren--;
			if( paren )
			{
				if( trinary_collect_else )
					trinary_else = SegAppend( trinary_else, SegDuplicate( args ) );
				else if( trinary_collect_then )
					trinary_then = SegAppend( trinary_then, SegDuplicate( args ) );
				else
					arg = SegAppend( arg, SegDuplicate( args ) );
				continue;
			}
			if( text[0] == '?' )
			{
				trinary_collect_then = 1;
				continue;
			}
			if( trinary_collect_then && text[0] == ':' )
			{
				trinary_collect_else = 1;
				continue;
			}
		}
		else // in quote...
		{
			if( text[0] == quote )
			{
				if( trinary_collect_else )
					trinary_else = SegAppend( trinary_else, SegCreateFromText( WIDE("char*") ) );
				else if( trinary_collect_then )
					trinary_then = SegAppend( trinary_then, SegCreateFromText( WIDE("char*") ) );
				else
					arg = SegAppend( arg, SegCreateFromText( WIDE("char*") ) );
				quote = 0;
			}
			continue;
		}
		if( text[0] == ',' )
		{
			PTEXT out = arg;
			PTEXT tmp;
			trinary_collect_else = 0;
			trinary_collect_then = 0;
			vtprintf( &g.vt, WIDE("(") );
			VarTextEnd( &g.vt );
			vtprintf( &g.vt , WIDE(" ( (") );
			VarTextEnd( &g.vt );
			if( trinary_then && trinary_else )
			{
				vtprintf( &g.vt , WIDE(" ( sizeof( ") );
				VarTextEnd( &g.vt );
				out = trinary_then;
				while( out )
				{
					vtprintf( &g.vt, WIDE("%s"), GetText( out ) );
					tmp = VarTextEnd( &g.vt );
					tmp->flags |= out->flags & TF_NOEXPAND;
					tmp->format.spaces = out->format.spaces;
					out = NEXTLINE( out );
				}
				vtprintf( &g.vt, WIDE(" ) > ") );
				VarTextEnd( &g.vt );
				vtprintf( &g.vt , WIDE("sizeof( ") );
				VarTextEnd( &g.vt );
				out = trinary_else;
				while( out )
				{
					vtprintf( &g.vt, WIDE("%s"), GetText( out ) );
					tmp = VarTextEnd( &g.vt );
					tmp->flags |= out->flags & TF_NOEXPAND;
					tmp->format.spaces = out->format.spaces;
					out = NEXTLINE( out );
				}
				vtprintf( &g.vt, WIDE(" ) ? ") );
				VarTextEnd( &g.vt );
				vtprintf( &g.vt , WIDE("sizeof( ") );
				VarTextEnd( &g.vt );
				out = trinary_then;
				while( out )
				{
					vtprintf( &g.vt, WIDE("%s"), GetText( out ) );
					tmp = VarTextEnd( &g.vt );
					tmp->flags |= out->flags & TF_NOEXPAND;
					tmp->format.spaces = out->format.spaces;
					out = NEXTLINE( out );
				}
				vtprintf( &g.vt, WIDE(" ) : ") );
				VarTextEnd( &g.vt );
				vtprintf( &g.vt , WIDE("sizeof( ") );
				VarTextEnd( &g.vt );
				out = trinary_else;
				while( out )
				{
					vtprintf( &g.vt, WIDE("%s"), GetText( out ) );
					tmp = VarTextEnd( &g.vt );
					tmp->flags |= out->flags & TF_NOEXPAND;
					tmp->format.spaces = out->format.spaces;
					out = NEXTLINE( out );
				}
				vtprintf( &g.vt, WIDE(" ) ) ") );
				VarTextEnd( &g.vt );
				LineRelease( trinary_then );
				LineRelease( trinary_else );
				// would be the 'if' part of this expression - unused.
				LineRelease( arg );
			}
			else
			{
				if( ( trinary_then && !trinary_else )
					|| ( !trinary_then && trinary_else ) )
				{
					fprintf( stderr, WIDE("%s(%d): Badly formed trinary operator!\n")
							, GetCurrentFileName()
							, GetCurrentLine() );
					LineRelease( trinary_then );
					LineRelease( trinary_else );
				}
				vtprintf( &g.vt , WIDE(" sizeof( ") );
				VarTextEnd( &g.vt );
				out = arg;
				while( out )
				{
					vtprintf( &g.vt, WIDE("%s"), GetText( out ) );
					tmp = VarTextEnd( &g.vt );
					tmp->flags |= out->flags & TF_NOEXPAND;
					tmp->format.spaces = out->format.spaces;
					out = NEXTLINE( out );
				}
				vtprintf( &g.vt, WIDE(")") );
				VarTextEnd( &g.vt );
				LineRelease( arg );
			}
			vtprintf( &g.vt, WIDE(" + 3 ) / 4 ) * 4 ") );
			VarTextEnd( &g.vt );
			vtprintf( &g.vt, WIDE(")") );
			VarTextEnd( &g.vt );
			vtprintf( &g.vt, WIDE("+") );
			VarTextEnd( &g.vt );
			result = SegAppend( result, VarTextGet( &g.vt ) );
		}
		else
		{
			if( trinary_collect_else )
				trinary_else = SegAppend( trinary_else, SegDuplicate( args ) );
			else if( trinary_collect_then )
				trinary_then = SegAppend( trinary_then, SegDuplicate( args ) );
			else
				arg = SegAppend( arg, SegDuplicate( args ) );
		}
	}
	if( arg )
	{
		{
			PTEXT out = arg;
			PTEXT tmp;
			vtprintf( &g.vt, WIDE("(") );
			VarTextEnd( &g.vt );
			vtprintf( &g.vt , WIDE(" ( (") );
			VarTextEnd( &g.vt );

			if( trinary_then && trinary_else )
			{
				vtprintf( &g.vt , WIDE(" ( sizeof( ") );
				VarTextEnd( &g.vt );
				out = trinary_then;
				while( out )
				{
					vtprintf( &g.vt, WIDE("%s"), GetText( out ) );
					tmp = VarTextEnd( &g.vt );
					tmp->flags |= out->flags & TF_NOEXPAND;
					tmp->format.spaces = out->format.spaces;
					out = NEXTLINE( out );
				}
				vtprintf( &g.vt, WIDE(" ) > ") );
				VarTextEnd( &g.vt );
				vtprintf( &g.vt , WIDE("sizeof( ") );
				VarTextEnd( &g.vt );
				out = trinary_else;
				while( out )
				{
					vtprintf( &g.vt, WIDE("%s"), GetText( out ) );
					tmp = VarTextEnd( &g.vt );
					tmp->flags |= out->flags &TF_NOEXPAND;
					tmp->format.spaces = out->format.spaces;
					out = NEXTLINE( out );
				}
				vtprintf( &g.vt, WIDE(" ) ? ") );
				VarTextEnd( &g.vt );
				vtprintf( &g.vt , WIDE("sizeof( ") );
				VarTextEnd( &g.vt );
				out = trinary_then;
				while( out )
				{
					vtprintf( &g.vt, WIDE("%s"), GetText( out ) );
					tmp = VarTextEnd( &g.vt );
					tmp->flags |= out->flags & TF_NOEXPAND;
					tmp->format.spaces = out->format.spaces;
					out = NEXTLINE( out );
				}
				vtprintf( &g.vt, WIDE(" ) : ") );
				VarTextEnd( &g.vt );
				vtprintf( &g.vt , WIDE("sizeof( ") );
				VarTextEnd( &g.vt );
				out = trinary_else;
				while( out )
				{
					vtprintf( &g.vt, WIDE("%s"), GetText( out ) );
					tmp = VarTextEnd( &g.vt );
					tmp->flags |= out->flags & TF_NOEXPAND;
					tmp->format.spaces = out->format.spaces;
					out = NEXTLINE( out );
				}
				vtprintf( &g.vt, WIDE(" ) ) ") );
				VarTextEnd( &g.vt );
				LineRelease( trinary_then );
				LineRelease( trinary_else );
				// would be the 'if' part of this expression - unused.
				LineRelease( arg );
			}
			else
			{
				if( ( trinary_then && !trinary_else )
					|| ( !trinary_then && trinary_else ) )
				{
					fprintf( stderr, WIDE("%s(%d): Badly formed trinary operator!\n")
							, GetCurrentFileName()
							, GetCurrentLine() );
					LineRelease( trinary_then );
					LineRelease( trinary_else );
				}
				vtprintf( &g.vt , WIDE(" sizeof( ") );
				VarTextEnd( &g.vt );
				out = arg;
				while( out )
				{
					vtprintf( &g.vt, WIDE("%s"), GetText( out ) );
					tmp = VarTextEnd( &g.vt );
					tmp->flags |= out->flags & TF_NOEXPAND;
					tmp->format.spaces = out->format.spaces;
					out = NEXTLINE( out );
				}
				vtprintf( &g.vt, WIDE(" )") );
				VarTextEnd( &g.vt );
			}
			vtprintf( &g.vt, WIDE(" + 3 ) / 4 ) * 4 ") );
			VarTextEnd( &g.vt );
			vtprintf( &g.vt, WIDE(")") );
			VarTextEnd( &g.vt );
			//vtprintf( &g.vt, WIDE("+") );
			//VarTextEnd( &g.vt );
			LineRelease( arg );
			result = SegAppend( result, VarTextGet( &g.vt ) );
			arg = NULL;
		}
	}
	else
		result = SegAppend( result, SegCreateFromInt( 0 ) );
	return result;
}

//----------------------------------------------------------------------

INDEX FindArg( PLIST pArgs, PTEXT pName )
{
	INDEX i;
	PTEXT pTest;
	if( TextIs( pName, WIDE("...") )
		|| TextIs( pName, WIDE("__VA_ARGS__") )
		|| TextIs( pName, WIDE("__SZ_ARGS__") ) )
	{
		return pArgs->Cnt;
	}
	FORALL( pArgs, i, PTEXT, pTest )
	{
		if( SameText( pName, pTest ) == 0 )
		{
			return i;
		}
	}
	return INVALID_INDEX;
}

void EmptyArgList( PLIST *pArgVals )
{
	INDEX idx;
	PTEXT pDelete;
	FORALL( *pArgVals, idx, PTEXT, pDelete )
	{
		LineRelease( pDelete );
	}
	DeleteListEx( pArgVals DBG_SRC );
}

//----------------------------------------------------------------------

PDEF AddArgumentEx( PLIST *pArgVals, PTEXT *pVal, INDEX *pi, PDEF pDefine, int *pbVarArg DBG_PASS )
#define AddArgument(pav,v,i,d,bva) AddArgumentEx(pav,v,i,d,bva DBG_SRC)
{
	if( !*pbVarArg )
	{
		while( *pi >= pDefine->pParams->Cnt &&
				pDefine->pSame )
		{
			if( g.bDebugLog & DEBUG_SUBST )
				fprintf( stddbg, WIDE("Attempting to find a define with more arguments...\n") );
			pDefine = pDefine->pSame;
		}
		if( *pi >= pDefine->pParams->Cnt )
		{
			if( g.bDebugLog & DEBUG_SUBST )
				fprintf( stddbg, WIDE("Too many paramters for %s (%zd of %zd) - try var arg\n")
						 , GetText( pDefine->pName )
						 , *pi, pDefine->pParams->Cnt );
			if( !pDefine->bVarParams )
			{
				fprintf( stderr, WIDE("%s(%d): Warning: excessive parameters to macro... resulting no replacement\n")
						 , GetCurrentFileName(), GetCurrentLine() );
				EmptyArgList( pArgVals );
				return NULL;
			}
			if( g.bDebugLog & DEBUG_SUBST )
				fprintf( stddbg, WIDE("okay and vararg it is!\n") );
			if( !(*pbVarArg) )
			{
				*pbVarArg = 1;
				*pi = pDefine->pParams->Cnt;
				return pDefine;
			}
		}
	}
	if( GetLinkEx( pArgVals, *pi ) )
		fprintf( stderr, WIDE("Overwriting exisitng parameter with new parameter?!") );
	SetLinkEx( pArgVals, *pi, *pVal DBG_RELAY );
	*pVal = NULL;
	if( !*pbVarArg )
		(*pi)++;
	return pDefine;
}
	//----------------------------------------------------------------------

void EvalSubstitutions( PTEXT *subst, int more )
{
	// pWord may be associated with parameters...
	// EvanSubst( &"min(a,b)" ) or basically...
	PTEXT pStart, pWord, pReset;
	PDEF pDefine;
	int Quote = 0, Escape = 0;
	static int nSubstLevel;

	// get word first....
	if( !subst || !*subst )
		return;

	nSubstLevel++;

	if( g.bDebugLog & DEBUG_SUBST )
	{
		fprintf( stderr, WIDE("Looking to substitute:") );
		DumpSegs( *subst );
		fprintf( stderr, WIDE("\n") );
	}

	// start at the beginning of all symbols...
	for( pWord = *subst;
		 //= GetCurrentWord();
		  pWord; 
		  pWord = pReset )
	{
		pReset = NEXTLINE( pWord );

		if( pWord->flags & (TF_INDIRECT|TF_NOEXPAND) )
			continue;

		if( Quote )
		{
			if( !Escape )
			{
				if( GetText( pWord )[0] == '\\' )
				{
					Escape = TRUE;
					continue;
				}
			}
			else
			{
				Escape = FALSE;
				continue;
			}
			if( GetText( pWord )[0] == Quote )
				Quote = 0;
			continue;
		}
		//fprintf( stderr, WIDE("Word is: %s\n"), GetText(pWord) );
	if( GetText( pWord )[0] == '\'' ||
		 GetText( pWord )[0] == '\"' )
	{
		Quote = GetText( pWord )[0];
		continue;
	}

		if( TextIs( pWord, WIDE("defined") ) )
		{
			PTEXT pNewWord = NULL, pEnd;
			pStart = pWord;
			pEnd = pWord = NEXTLINE( pWord );
			if( pWord && 
				 GetText( pWord )[0] == '(' )
			{
				pWord = NEXTLINE( pWord );
				if( pWord && 
					 GetText( pWord )[0] != ')' )
				{
					if( FindDefineName( pWord, IGNORE_PARAMS ) )
					{
						pNewWord = SegCreateFromText( WIDE("1") );
						pNewWord->format.spaces = pStart->format.spaces;
					}					else
					{
						pNewWord = SegCreateFromText( WIDE("0") );
						pNewWord->format.spaces = pStart->format.spaces;
					}
					pWord = NEXTLINE( pWord );
					if( GetText( pWord )[0] != ')' )
					{
						fprintf( stderr, WIDE("%s(%d): Error in parameter to 'defined' - more than one symbol?\n")
							 , GetCurrentFileName(), GetCurrentLine() );
						g.ErrorCount++;
					}
					pEnd = pWord;
				}
				else
				{
					fprintf( stderr, WIDE("%s(%d): Empty paranthesis to defined.\n")
							 , GetCurrentFileName(), GetCurrentLine() );
				}
			}
			else
			{
				if( pWord )
				{
					if( FindDefineName( pWord, IGNORE_PARAMS ) )
					{
						pNewWord = SegCreateFromText( WIDE("1") );
						pNewWord->format.spaces = pStart->format.spaces;
					}
					else
					{
						pNewWord = SegCreateFromText( WIDE("0") );
						pNewWord->format.spaces = pStart->format.spaces;
					}
				}
				else
					fprintf( stderr, WIDE("%s(%d): defined used with no parameter (last thing on line)\n")
							 , GetCurrentFileName(), GetCurrentLine() );
			}
			// okay having evaluated defined... do subst on that.
			if( pNewWord )
			{
				PTEXT pOrigin = pStart;
				pNewWord->flags |=TF_NOEXPAND;
				SegSubstRange( &pStart, pEnd, pNewWord );
				pReset = NEXTLINE( pNewWord ); // already have a good idea that 0/1 will not subst...
				if( pOrigin == *subst )
				{
					if( g.bDebugLog & DEBUG_SUBST )
					{
						fprintf( stddbg, WIDE("Resetitng line begin 1\n") );
					}
					*subst = pNewWord;
				}
			}
			else
			{
				PTEXT pOrigin = pStart;
				pReset = NEXTLINE( pEnd );
				SegSubstRange( &pStart, pEnd, NULL );
				if( pOrigin == *subst )
				{
					if( g.bDebugLog & DEBUG_SUBST )
					{
						fprintf( stddbg, WIDE("Resetitng line begin 2\n") );
					}
					*subst = pReset;
				}
			}
			continue;
		}
		if( g.bDebugLog & DEBUG_SUBST )
		{
			fprintf( stddbg, WIDE("%s(%d): Consider word: %s next: %s\n")
					, GetCurrentFileName(), GetCurrentLine()
					, GetText( pWord )
					, GetText( NEXTLINE( pWord ) ) );
		}

		pStart = pWord; // mark this is the first word we're grabbing

		// just check to see if this word is anything defined
		// then later get the actual define for pDefine...
		pDefine = FindDefineName( pWord, IGNORE_PARAMS ); // look up this symbol
		if( pDefine && !pDefine->bUsed ) // if we found the name as a symbol...
		{
			PTEXT pSubst = NULL, p;
			PLIST pArgVals = NULL;
			if( g.bDebugLog & DEBUG_SUBST )
			{
				fprintf( stddbg, WIDE("Found substitution for %s params:%p\n"), GetText( pWord ), pDefine->pParams );
			}
			// don't use it yet....
			//pDefine->bUsed = TRUE;

			// does the define expect parameters?
			if( pDefine->pParams )
			{
				INDEX i = 0;
				int bVarArg = 0;
				int bNoArgs = 1;
				PTEXT pVal = NULL;
				char *file_start;
				int line_start;
				file_start = GetCurrentFileName();
				line_start = GetCurrentLine();
				if( !NEXTLINE( pWord ) && more )
					pReset = pWord = ReadLine( TRUE );
				else
					pWord = NEXTLINE( pWord );
				if( !pWord ||
					 GetText( pWord )[0] != '(' )
				{
					if( g.flags.bAllWarnings )
						fprintf( stderr, WIDE("%s(%d) Warning: No parameters for macro defined at %s(%d)\n")
								, GetCurrentFileName(), GetCurrentLine()
								, pDefine->pFile, pDefine->nLine
								 );
					continue;
				}
				// step to the next word after the leading (
				pWord = NEXTLINE( pWord );
				// need to gather the parameters specified
				// then find the right define result for number of paramters...
				while(1)
				{
					int quote = 0;
					int parenlevels = 0;
					int escape = 0;
					char *text;

					while( pWord || ( more && ( pWord = ReadLine( TRUE ) ) ) )
					{
						// while I have a word, get the text for the word,
						// if quote levels or paren levels continue collection
						// or if it's not a ','
						//fprintf( stddbg, WIDE(" stuff... %08x %08x %s\n"), pWord, GetText( pWord ), GetText( pWord ) );
						text = GetText( pWord );
						//fprintf( stddbg, WIDE("Consider word: %s\n"), text );
						if( !quote )
						{
							if( text[0] == '\"' || text[0] == '\'' )
								quote = text[0];
							if( ( text[0] == '(' )
								&& ( GetTextSize( pWord ) == 1 ) ) // otherwise we can assume the paring paren is included...
							{
								parenlevels++;
							}
							else if( text[0] == ')' )
							{
								// last paren?
								if( !parenlevels )
									break;
								parenlevels--;
							}
							else if( !parenlevels && text[0] == ',' )
							{
								bNoArgs = FALSE;
								if( !bVarArg )
									break;
							}
						}
						else // is quote
						{
							if( !escape )
							{
								if( text[0] == '\\' )
									escape = TRUE;
								else if( text[0] == quote )
									quote = 0;
							}
							else // was escape
								escape = FALSE; // next character of course clears this
						}

						{
							PTEXT tmp = SegDuplicate(pWord);
							pVal = SegAppend( pVal, tmp );
							bNoArgs = FALSE;
						}

						pWord = NEXTLINE( pWord );
					}

					if( !pWord )
					{
						fprintf( stderr, WIDE("%s(%d): Error: Ran out of file data before end of macro params\n")
								 , GetCurrentFileName(), GetCurrentLine() );
						if( quote )
						{
							fprintf( stderr, WIDE("%s(%d) Error: Unterminated string - out of input.\n")
									 , GetCurrentFileName(), GetCurrentLine() );
							g.ErrorCount++;
							quote = 0;
						}
						if( parenlevels )
						{
							fprintf( stderr, WIDE("%s(%d) Error: Unterminated parenthized expression - out of input.\n")
									 , GetCurrentFileName(), GetCurrentLine() );
							g.ErrorCount++;
							parenlevels = 0;
						}
						EmptyArgList( &pArgVals );
						if( pVal )
							LineRelease( pVal );
						return;
					}
						//if( !bVarArg && text[0] == ',' )
						//{
						// 	pWord = NEXTLINE( pWord );
						//	if( !pWord )
						//      pWord = ReadLine( TRUE );
						//break; // end the read/gather loop also...
						//}
						//}
					if( GetText( pWord )[0] == ')' )
						break;
					if( !bVarArg )
					{
						if( pVal )
						{
							pVal->format.spaces = 0;
							if( g.bDebugLog & DEBUG_SUBST )
							{
								PTEXT tmp = BuildLine( pVal );
								fprintf( stddbg, WIDE("Adding parameter: %s\n"), GetText( tmp ) );
								LineRelease( tmp );
							}
						}
						else
						{
							if( g.bDebugLog & DEBUG_SUBST )
							{
								fprintf( stddbg, WIDE("Adding blank(empty,NIL) parameter\n") );
							}
						}
						//if( pVal )
						//{
							pDefine = AddArgument( &pArgVals, &pVal, &i, pDefine, &bVarArg );
							if( !pDefine )
							{
								if( pVal )
									LineRelease( pVal );
								return;
							}
							if( bVarArg && pVal )
								pVal = SegAppend( pVal, SegDuplicate( pWord ) );
						//}
					}
					else
					{
						break; // otherwise we'll have already collected the var arg in pVal...
					}
					pWord = NEXTLINE( pWord );
				}

				if( !bNoArgs )
				{
					// var args value is dangling when the above loop ends.
					//if( !pDefine->bVarParams )
					//{
					//   fprintf( stderr, WIDE("Adding var arg parameter to a macro without var args?\n") );
					//}
					if( pVal )
						pVal->format.spaces = 0;
					pDefine = AddArgument( &pArgVals, &pVal, &i, pDefine, &bVarArg );
					if( !pDefine )
					{
						if( pVal )
							LineRelease( pVal );
						return;
					}
					if( pVal )
						pDefine = AddArgument( &pArgVals, &pVal, &i, pDefine, &bVarArg );
					if( pVal )
						fprintf( stderr, WIDE("BLAH! Still have a parametner...\n") );
				}
				// hmm at this point the pDefine is auto updated as we find more arguments...
				// assuming that there have been a legal ordering of macros declared....
				//pDefine = FindDefineName( pDefine->pName
				//								, pArgVals?pArgVals->Cnt:0 );

				if( ( g.bDebugLog & DEBUG_SUBST )
					&& pDefine && pDefine->pParams ) // could be useful to dump arguments....
				{
					//PTEXT arg;
					INDEX idx;
					if( g.bDebugLog & DEBUG_SUBST )
						fprintf( stddbg, WIDE("Macro Arguments: \n") );
					if( pArgVals )
					{
						for( idx = 0; idx < pArgVals->Cnt; idx++ )
						//FORALL( pArgVals, idx, PTEXT, arg )
						{
							PTEXT name = GetLink( pDefine->pParams, idx );
							PTEXT full = BuildLine( (PTEXT)pArgVals->pNode[idx] );
								fprintf( stddbg, WIDE("%s = %s (%p)\n")
										, name?GetText( name ):"..."
										, GetText( full )
										, pArgVals->pNode[idx] );
								LineRelease( full );
						}
					}
				}

				if( pArgVals && pDefine->pParams &&
					( pArgVals->Cnt < pDefine->pParams->Cnt ) )
				{
					fprintf( stderr, WIDE("%s(%d): Warning: parameters to macro are short... assuming NIL parameters.\n")
						 , GetCurrentFileName(), GetCurrentLine() );
				}

			// ok now we have gathered macro parameters, and the macro
				if( pDefine )
				{
					int MakeString = 0;
					int quote = 0;
					int escape = 0;
					INDEX idx;

					if( pArgVals )
					{
						for( idx = 0; idx < pArgVals->Cnt; idx++ )
						{
							EvalSubstitutions( (PTEXT*)(pArgVals->pNode + idx), FALSE );
						}
					}
					pDefine->bUsed = TRUE; // is used. now...

					for( p = pDefine->pData; p; p = NEXTLINE( p ) )
					{
						if( !quote )
						{
							if( GetText( p )[0] == '\'' )
								quote = '\'';
							if( GetText( p )[0] == '\"' )
								quote = '\"';
						}
						else
						{
							if( !escape )
							{
								if( GetText( p )[0] == '\\' )
									escape = TRUE;
								else if( GetText( p )[0] == quote )
									quote = 0;
							}
							else
								escape = 0;
						}
						if( quote )
						{
							pSubst = SegAppend( pSubst, SegDuplicate( p ) );
							continue;
						}

						if( GetText( p )[0] == '#' )
						{
							MakeString++;
							continue;
						}

						idx = FindArg( pDefine->pParams, p );
						if( idx == INVALID_INDEX )
						{
							PTEXT seg;
							if( TextIs( p, WIDE("__VA_ARGS__") ) ||
								TextIs( p, WIDE("...") ) )
							{
								fprintf( stderr, WIDE("%s(%d): Should NEVER be here!\n"), __FILE__, __LINE__ );
								exit(-1);
							}
							seg = SegDuplicate( p );

							if( MakeString == 2 )
							{
								if( seg )
								{
									PTEXT valnow = pSubst;
									PTEXT newseg;
									SetEnd( valnow );
									// just a concatenation - make sure we put these right next to each other...
									vtprintf( &g.vt, WIDE("%s%s"), GetText( valnow ), GetText( seg ) );
									newseg = VarTextGet( &g.vt );
									newseg->format.spaces = valnow->format.spaces;
									SegGrab( valnow );
									if( valnow == pSubst )
										pSubst = NULL;
									LineRelease( valnow );
									SegSubst( seg, newseg );
									seg = newseg;
								}
								MakeString = 0;
							}
							else if( MakeString == 1 )
							{
								if( seg )
								{
									PTEXT tmp;
									VarTextAddCharacter( &g.vt, '\"' );
									tmp = VarTextEnd( &g.vt );
									if( !(tmp->format.spaces = p->format.spaces ) )
										tmp->format.spaces++;

									vtprintf( &g.vt, WIDE("%s"), GetText( seg ) );
									VarTextEnd( &g.vt );
									VarTextAddCharacter( &g.vt, '\"' );
									LineRelease( seg );
									seg = VarTextGet( &g.vt );
								}
								MakeString = 0;
							}
							else
							{
								if( p == pDefine->pData )
									seg->format.spaces++;
							}
							pSubst = SegAppend( pSubst, seg );
						}
						else
						{
							PTEXT seg;
							if( idx == pDefine->pParams->Cnt )
							{
								if( TextIs( p, WIDE("__SZ_ARGS__") ) )
									seg = BuildSizeofArgs( GetLink( pArgVals, idx ) );
								else
								{
									seg = TextDuplicate( GetLink( pArgVals, idx ) );
									if( !GetLink( pArgVals, idx ) )
									{
										PTEXT prior = pSubst;
										SetEnd( prior );
										if( prior && GetText( prior )[0] == ',' )
										{
											SegDelete( &prior );
										}
										continue;
									}
								}
							}
							else
							{
								seg = TextDuplicate( GetLink( pArgVals, idx ) );
							}

							if( MakeString == 2 )
							{
								if( seg )
								{
									PTEXT valnow = pSubst;
									PTEXT newseg;
									SetEnd( valnow );
									// just a concatenation - make sure we put these right next to each other...
									vtprintf( &g.vt, WIDE("%s%s"), GetText( valnow ), GetText( seg ) );
									newseg = VarTextGet( &g.vt );
									newseg->format.spaces = valnow->format.spaces;
									SegGrab( valnow );
									if( valnow == pSubst )
										pSubst = NULL;
									LineRelease( valnow );
									SegSubst( seg, newseg );
									seg = newseg;
								}
								/*
								// don't check this for substitution
								if( seg )
								seg->format.spaces = 0;
								*/
								MakeString = 0;
							}
							else if( MakeString == 1 )
							{
								PTEXT text;
								if( seg )
								{
									PTEXT tmp;
									FixQuoting( seg );
									text = BuildLine( seg );
									VarTextAddCharacter( &g.vt, '\"' );
									tmp = VarTextEnd( &g.vt );
									if( !(tmp->format.spaces = p->format.spaces ) )
										tmp->format.spaces++;
									vtprintf( &g.vt, WIDE("%s"), GetText( text ) );
									VarTextEnd( &g.vt );
									VarTextAddCharacter( &g.vt, '\"' );
									LineRelease( text );
									LineRelease( seg );
									seg = VarTextGet( &g.vt );
								}
									// don't check this for substitution
							MakeString = 0;
						}
						else
							{
								// seg will always be a unique thing unto itself here.
								/*
								if( g.bDebugLog & DEBUG_SUBST )
								{
									fprintf( stddbg, WIDE("%s(%d): Doing substitution on substituted parameter: ")
											, GetCurrentFileName()
											, GetCurrentLine()
											 );
									DumpSegs( seg );
									fprintf( stddbg, WIDE("\n") );
								}
								EvalSubstitutions( &seg, ppReset );
								if( g.bDebugLog & DEBUG_SUBST )
								{
									fprintf( stddbg, WIDE("%s(%d): Did substitution: ")
											, GetCurrentFileName()
											, GetCurrentLine()
											 );
									DumpSegs( seg );
									fprintf( stddbg, WIDE("\n") );
								}
								*/
								if( seg )
								{
									seg->format.spaces = p->format.spaces;
									//if( p == pDefine->pData )
									seg->format.spaces++;
								}
								// check seg itself as a symbol...
							}
							pSubst = SegAppend( pSubst, seg );
						}
					}
					EmptyArgList( &pArgVals );
				}
				else
				{
					fprintf( stderr, WIDE("%s(%d): Could not match macro(%s) with specified (%zd) parameters...\n")
							 , GetCurrentFileName(), GetCurrentLine()
							 , GetText( pStart )
							 , pArgVals?pArgVals->Cnt:0 );
					EmptyArgList( &pArgVals );
					continue;
				}
			}
			else // if !define->params...
			{
				pDefine->bUsed = TRUE;
				// simple case - no names, no nothing, just literatal substitue
				if( pSubst = TextDuplicate( pDefine->pData ) )
				{
					if( g.bDebugLog & DEBUG_SUBST )
						fprintf( stddbg, WIDE("First subst word: %s\n"), GetText( pSubst ) );
					pSubst->format.spaces = 1;
				}
				else
				{
					if( g.bDebugLog & DEBUG_SUBST )
						fprintf( stddbg, WIDE("Symbol has no subst data...\n") );
				}
			}

			if( pSubst )
			{
				PTEXT pOrigin = pStart;
				EvalSubstitutions( &pSubst, FALSE );
				pReset = NEXTLINE( pWord );
				if( pReset )
					pReset->format.spaces = 1;
				if( pSubst )
					pReset = pSubst;
				SegSubstRange( &pStart, pWord, pSubst );
				if( pOrigin == *subst )
				{
					if( g.bDebugLog & DEBUG_SUBST )
					{
						fprintf( stddbg, WIDE("Resetting line begin 3\n") );
					}
					*subst = pStart;
				}

				// while current is still marked as used, process from HERE
				// forward... this will allow recursive defines to work.
				/*
				if( pStart == *subst )
				{
					if( g.bDebugLog & DEBUG_SUBST )
						fprintf( stddbg, WIDE("Doing subst from start of line...\n") );
					EvalSubstitutions( subst, FALSE ); // need recursion to keep defined substs from redefining...
					pReset = *ppReset;
					fprintf( stddbg, WIDE("Returning from doing start of line subst...\n") );
				}
				else
				{
					if( g.bDebugLog & DEBUG_SUBST )
						fprintf( stddbg, WIDE("Doing subst from current start\n") );
					EvalSubstitutions( &pStart, FALSE ); // need recursion to keep defined substs from redefining...
					pReset = *ppReset;
					fprintf( stddbg, WIDE("Returning from current spot subst...\n") );
				}
				*/
			}
			else
			{
				PTEXT pOrigin = pStart;
				// step to the next word... substitution with NO data
				// just continue on with current line.
				// we will be breaking out of this loop, so this sets
				// the initial condition of the next recursion path.
				pReset = NEXTLINE( pWord );
				if( pReset )
					pReset->format.spaces = 1;
				//if( ppReset )
				//	*ppReset = pReset;
				SegSubstRange( &pStart, pWord, NULL );
				if( pOrigin == *subst )
				{
					if( g.bDebugLog & DEBUG_SUBST )
					{
						fprintf( stddbg, WIDE("Resetitng line begin 4\n") );
					}
					*subst = pReset;
				}
			}
			if( g.bDebugLog & DEBUG_SUBST )
			{
				fprintf( stddbg, WIDE("Line Now:") );
				DumpSegs( *subst );
				fprintf( stddbg, WIDE("\n") );
			}

			// release symbol just substituted.
			if( pDefine )
					pDefine->bUsed = FALSE;
		} // end if define && !used
		else
		{
			pWord->flags |= TF_NOEXPAND;
		}

		//else if( pDefine ) // if used - perhaps we should return now and see if parent substitutions can handle this...
		//{
		//	*ppReset = pWord;
		//   return;
		//}

	} // while there's a word...
	if( g.bDebugLog & DEBUG_SUBST )
	{
		fprintf( stddbg, WIDE("Done substituting....\n") );
		if( !*subst )
		{
			fprintf( stddbg, WIDE("Resulting No content.\n") );
		}
	}
	nSubstLevel--;
	return;
}

//----------------------------------------------------------------------

void CommitDefinesToCommandLineTree( PDEF root )
{
	if( root )
	{
		CommitDefinesToCommandLineTree( root->pLesser );
		CommitDefinesToCommandLineTree( root->pSame );
		CommitDefinesToCommandLineTree( root->pGreater );
		root->nType = DEFINE_COMMANDLINE;
	}
}

//----------------------------------------------------------------------

void CommitDefinesToCommandLine( void )
{
	CommitDefinesToCommandLineTree( pDefineRoot );
}

//----------------------------------------------------------------------
