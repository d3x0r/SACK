// SQLParse
// This functionality parses a sql create statement and
// creates the appropriate MySQL DataTables
// Contributors:  Jim Buckeyne, Christopher Green.
#include <stdhdrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sack_types.h>
#include <sharemem.h>
#include <logging.h>
#include <pssql.h>
#include <filedotnet.h>

#define DEFAULT_LEVEL LOG_NOISE

SQL_NAMESPACE

	typedef struct my_field_tag{

		TEXTSTR name;
		TEXTSTR type;
		TEXTSTR extra;

	} MY_FIELD, *PMY_FIELD;
typedef struct my_generic_def_tag{

	_32 count;
	_32 fldCnt;
	TEXTSTR cName;
	int cType;
	PLIST listFields;

	PMY_FIELD pFld;

} MY_GENERIC_DEF, *PMY_GENERIC_DEF;
enum{ ENU_BEGIN = 0
	 , ENU_FIRST_WORD
	 , ENU_SECOND_WORD
	 , ENU_THIRD_WORD
	 , ENU_NEXT_WORD
	 , ENU_CREATE_TABLE
	 , ENU_COLUMNS
	 , ENU_REGULAR_COLUMN
	 , ENU_TABLE_OPTIONS
	 , ENU_TABLE_OPTION_TYPE
	 , ENU_TABLE_OPTION_COMMENT
	 , ENU_UNIQUE_KEY
	 , ENU_PRIMARY_KEY
	 , ENU_INDEX_KEY
	 , ENU_KEYNAME // substates for key index parsing
	 , ENU_FINDKEYCOLUMN // a state between NAME and (columns...) which may contain attributes
	 , ENU_KEYCOLUMN // substates for key index parsing
	 , ENU_KEYEXTRA //  substates for key index parsing
	 , ENU_END} ;


void DestroyMyField( PMY_GENERIC_DEF pstColumn )
{
	INDEX idx;
	PMY_FIELD field;
	//	lprintf("pstColumn->fldCnt is %lu", pstColumn->fldCnt );
	//DebugBreak();
	if( pstColumn->fldCnt   && pstColumn->listFields )
	{
		LIST_FORALL( pstColumn->listFields, idx, PMY_FIELD, field )
		{
			if( field )
			{
				xlprintf(LOG_NOISE)("DestroyMyField for %s %s %s"
										 , field->name
										 , field->type
										 , field->extra
										 );
				if( strlen( field->name) )
					Release( field->name );
				if( strlen(field->type ) )
					Release( field->type );
				if( strlen(field->extra) )
					Release( field->extra );
				if( pstColumn->pFld == field )
					pstColumn->pFld = NULL;
				Release( field );
			}
		}
		DeleteList( &pstColumn->listFields );
		//Release( pstColumn->cType );
		Release( pstColumn->cName );
		Release((POINTER)pstColumn );
		pstColumn = NULL;

	}
}

///*//***********************************************
//       According to http://dev.mysql.com/doc/refman/4.1/en/identifiers.html

// Database, table, index, column, and alias names are identifiers.
// This section describes the allowable syntax for identifiers in MySQL.
// The following table describes the maximum length for each type of
// identifier.
// Identifier 	Maximum Length (bytes)
// Database 	64
// Table 	   64
// Column 	   64
// Index 	   64
// Alias 	  255

// There are some restrictions on the characters that may appear in
// identifiers:
//     * No identifier can contain ASCII 0 (0x00) or a byte with a
//       value of 255.
//     * Before MySQL 4.1, identifier quote characters should not
//       be used in identifiers. As of 4.1, the use of identifier
//       quote characters in identifiers is permitted, although it
//       is best to avoid doing so if possible.
//     * Database, table, and column names should not end with
//       space characters.
//     * Database names cannot contain '/', '\', '.', or
//       characters that are not allowed in a directory name.
//     * Table names cannot contain '/', '\', '.', or
//       characters that are not allowed in a filename.
//     * The length of the identifier is in bytes, not characters.
//       If you use multi-byte characters in your identifier names,
//       then the maximum length will depend on the byte count of
//       all the characters used.
// Beginning with MySQL 4.1, identifiers are stored using Unicode (UTF-8)
//***********************************************   */
//#define STRING_UNIQUE WIDE("unique")
//#define STRING_PRIMARY WIDE("primary")
//#define STRING_INDEX_KEY WIDE("key")
//#define STRING_COLUMN WIDE("column")
//#define STRING_TYPE WIDE("type")
//#define STRING_COMMENT WIDE("comment")

char *types[] = { WIDE("nothing")
					 ,WIDE("unique")
					 ,WIDE("primary")
					 ,WIDE("key")
					 ,WIDE("column")
					 ,WIDE("type")
					 ,WIDE("comment")
};
enum {
	STRING_NOTHING
	  , STRING_UNIQUE
	  , STRING_PRIMARY
	  , STRING_INDEX_KEY
	  , STRING_COLUMN
	  , STRING_TYPE
	  , STRING_COMMENT
};

PTABLE GetFieldsInSQLEx( CTEXTSTR cmd, int writestate DBG_PASS )
//PTABLE GetFieldsInSQL( CTEXTSTR cmd, int writestate)
{
	PTABLE pTable = New ( TABLE );
	PTEXT pNew, pOld, busy;
	TEXTCHAR *c = NULL;
	TEXTCHAR buf[128];
	_32 n = 0;
	_32 total = 0;
	PTEXT word1, word2, word3; /* Jim sez: duh these should have been text segments */
	PTEXT phrase1, phrase2 = NULL;
	_32 bFlagA = 0, bFlagB = 0, uiSubstateA = ENU_BEGIN;
	LOGICAL gottatick = FALSE, bInNeedOfColumnAllocation = FALSE, gottaslash=FALSE , bEnum = FALSE;
	INDEX state = ENU_CREATE_TABLE;
	DB_KEY_DEF *pKeyDef;//to be alloc'ed after parsing of key.

	PMY_GENERIC_DEF pstColumn = NULL;// to be alloc'ed during parsing of key.
	PLIST listIndexColumn = NULL;
	PLIST listRegularColumn = NULL;
	FIELD *pFields;

	PVARTEXT pvt = VarTextCreate();
	FILE * file = NULL;
	//actually these next declarations were originally within a case, but since they have to be used  in a mutually-exclusive manner in two different places, might as well declare them to be function-global scope.
	PTEXT tmp;
	TEXTSTR name;
	TEXTSTR szNormalizedName = NULL;//unfortunately, this has to be exposed to the whole function because it is used during parsing and printing, compare with pTable->name.
	_8 countParen = 0;


	if( 0 )
	{
		_32 a,b,c,d;
		GetMemStats( &a, &b, &c, &d );
		lprintf( WIDE("Mem stats: %d %d %d %d"), a, b, c, d );
		DebugDumpMem();
	}
	MemSet( pTable, 0, sizeof( TABLE ));
	pTable->flags.bDynamic = TRUE; // set this as a dynamic table so DestroySQLTable can destroy it.
	//xlprintf(DEFAULT_LEVEL)(WIDE("about to SegCreateFromText and burst for %s") ,cmd);

	pOld = SegCreateFromText( cmd);
	pNew = burst( pOld );
	LineRelease( pOld );

	word1 = word2 = word3 =  NULL;
	buf[0] = 0;

	for( busy = pNew; busy ;busy = NEXTLINE(busy) )
	{
		if( !GetTextSize( busy ) )
			continue; // no size, skip to next iteration.

		c = GetText(busy);
		//xlprintf(DEFAULT_LEVEL)(WIDE(" c is %s"), c );
		if( bInNeedOfColumnAllocation )
		{
			pstColumn = New( MY_GENERIC_DEF ); // to put fields in, this is the column definition.
			pstColumn->listFields = NULL;
			pstColumn->cName = NULL;
			pstColumn->cType = STRING_NOTHING;
			pstColumn->count = 0; //count of fields.
			pstColumn->fldCnt = 0;
			pstColumn->pFld = New( MY_FIELD );
			MemSet( pstColumn->pFld, 0, sizeof( MY_FIELD ) );
			bInNeedOfColumnAllocation = FALSE;
		}

		switch( state )
		{
		case ENU_CREATE_TABLE:
			{
				switch( uiSubstateA )
				{
				case ENU_BEGIN:
					{
						if( !( strnicmp( c, WIDE("create"), 6 ) ) )
						{
							uiSubstateA = ENU_FIRST_WORD;
						}
						else
						{
							// One day, for the purposes of modularization, implement  PTABLE Goodbye(BOOL bNice, ... )
							// But for now, just get out and don't even try to fix it.  If the parameter passed in
							// is malformed, Jim sez "You should just return NULL and exit.  People pass in malformed stuff all the time".
							// So...just return NULL then and exit.
							xlprintf(LOG_ALWAYS)("The passed in text is supposed to be CREATE but is %s, which is unhandled.  Goodbye for now.  Sorry it didn't work out.", c );
							Release( pTable );
							return NULL;
						}
						break;
					}
				case ENU_FIRST_WORD:
					{
						if( !( strnicmp( c, WIDE("table"), 5 ) ) )
						{
							//xlprintf(DEFAULT_LEVEL)(WIDE("Hey, HeY, HEY! Got Create Table!"));
							uiSubstateA = ENU_SECOND_WORD;
							bFlagA=bFlagB=FALSE;
						}
						else
						{
							// One day, for the purposes of modularization, implement  PTABLE Goodbye(BOOL bNice, ... )
							// But for now, just get out and don't even try to fix it. If the parameter passed in
							// is malformed, Jim sez "You should just return NULL and exit.  People pass in malformed stuff all the time".
							// So...just return NULL then and exit.
							xlprintf(LOG_ALWAYS)("The passed in text is supposed to be TABLE but is %s, which is unhandled.   Goodbye for now.  Sorry it didn't work out.", c );
							Release( pTable );
							return NULL;
						}
						break;
					}
				case ENU_SECOND_WORD:
					{
						{
							switch ( *c )
							{
							case ' ':
								lprintf("Gotta space? Exactly how does burst pass a space?");
								break;
								// case '\\':
							case 0x5c:
								{
									if( !gottaslash )
									{
										gottaslash = TRUE;
										lprintf("gotta slash?  Escape character?");
									}
									else
									{
										lprintf("gotta another slash.");
										word1 = SegAppend( word1, SegDuplicate( busy ) );
										gottaslash = FALSE;
									}
								}
								break;
							case '`':
								if( !gottaslash )
								{
									if( !gottatick )
									{
										gottatick = TRUE;
									}
									else
									{
										gottatick = FALSE;
									}
								}
								else
								{
									word1 = SegAppend( word1, SegDuplicate( busy ) );
									gottaslash = FALSE;
								}
								break;
							case ')':
								{
									if( gottatick )
									{
										lprintf("Hey...what is this, a joke?  A Close Parenthesis inside a tick as part of the table name?");
										word1 = SegAppend( word1, SegDuplicate( busy ) );
									}
									else
									{
										if( countParen )
											countParen--;
										else
											lprintf("Wait, no open parenthesis, why a close parenthesis?");

										lprintf("Gotta Close Parenthesis. %lu still open.", countParen );
									}
								}
								break;
							case '(':
								//
								//  In order to create a table, at least one column must be specified.
								//  The column specification begins with an open parenthesis.
								//  Without the column specification, there is no point in going any further.
								//
								{
									// GuFiNeS 4 database.table
									TEXTSTR dotplus;
									if( gottatick )
									{
										lprintf("Hey...what is this, a joke?  An Open Parenthesis inside a tick as part of the table name?");
										word1 = SegAppend( word1, SegDuplicate( busy ) );
									}
									else
									{
										if( gottaslash )
										{
											lprintf("Error check. gottaslash true but no defined handling.  Setting gottaslash to flase and continuing, but this condition needs to be examined" );
											gottaslash=FALSE;
										}

										bFlagA = bFlagB = FALSE;

										// database.table is parsed here.

										// Begin Modularization Candidate for TableNameBuilder
										word1->format.position.spaces = 0;
										name = StrDup( GetText( tmp = BuildLine( word1 ) ) );
										LineRelease( word1 );
										word1 = NULL;
										LineRelease( tmp );
										if( dotplus = strchr( name, '.' ) )
										{
											/* extract database...*/
											TEXTSTR d = NULL;
											d = strtok( name , ".");
											if( d )
											{
												pTable->database = StrDup( d );
												pTable->name = StrDup( ( dotplus + 1 )  );
											}
											else
											{
												xlprintf(LOG_ALWAYS)("ERROR: there is no dot in %s??? Now what? I guess make that into pTable->name", name );
												pTable->name = StrDup( name );
											}
										}
										else
										{
											if( pTable->name  )
											{
												lprintf( "pTable->name was %s, oh well.  It's NULL now.", pTable->name );
												// Jim sez: although it's a CTEXTSTR for static init, this ->name was allocated by me
												Release( (POINTER)pTable->name );
												pTable->name = NULL;
											}
											if( pTable->database  )
											{
												lprintf("pTable->database was %s, oh well.  it's NULL now.", pTable->database );
												// Jim sez: although it's a CTEXTSTR for static init, this ->name was allocated by me
												Release( (POINTER)pTable->database );
												pTable->database = NULL;
											}
											pTable->name = StrDup( name );
										}

										Release( name );// and this variable, name, will be used later, so leave it alone for now.
										//  table name normalization asdf pTable->name
										if( strchr( pTable->name, ' ' ) )
										{
											_32 ln = ( ( strlen( pTable->name) ) + 1 );
											TEXTSTR t;
											TEXTSTR n = StrDup( pTable->name );
											t = strtok( (char *) n, " ");// needs a char *, but is effectively a char const *, so typecast.
											while( ( t != NULL ) && ( ln < 65 ) )
											{
												if( !szNormalizedName )
												{
													szNormalizedName = NewArray( TEXTCHAR, 65 );// Table names can only be 64 bytes large...plus a gratuitous terminating null.
													strcpy( szNormalizedName, t );
												}
												else
												{
													strncat(szNormalizedName, "SPACE" , 5 );
													strcat (szNormalizedName, t );
													ln = strlen( szNormalizedName);
												}
												t = strtok( NULL, " ");
											}
										}
										else
										{
											//lprintf("table name normalization noit needed for %s", pTable->name );// Line 420 is not legal in the United States. ;)

										}
										//xlprintf(LOG_ADVISORY)("Eventually, got the normalized table name of \"%s%s%s\" from \"%s\"", ( pTable->database ? pTable->database : "" ), ( pTable->database ? "." : "" ), ( szNormalizedName ? szNormalizedName : pTable->name ), pTable->name );

										// END Modularization Candidate for TableNameBuilder
									}

									if(writestate)
									{
//                              DebugBreak();
										//pvt = VarTextCreate();
										Fopen( file, WIDE("sparse.txt"), WIDE("ab") );
										if( file )
											fseek( file, 0, SEEK_SET );
										else
										{
											xlprintf(DEFAULT_LEVEL)(WIDE("Unable to open the file sparse.txt.  Now what?  Is this the end?"));
										}
									}

									// this should look similar to the comma case in the next state.
									bInNeedOfColumnAllocation  = TRUE;
									n = 0;
									uiSubstateA = ENU_BEGIN;
									// this should look similar to the comma case in the next state.

									word2 = word1 = NULL;
									state = ENU_COLUMNS;
								}
								break;
							default:
								{
									if(   !bFlagA
										&& !bFlagB
										&& ( !strnicmp( c, "IF", 2 ) )
									  )
									{
										bFlagA = TRUE;
										//word1 = SegAppend( word1, SegDuplicate( busy ) );
									}
									else if(    bFlagA
											  && !bFlagB
											  && ( !strnicmp( c, "NOT", 3 ) )
											 )
									{
										bFlagB = TRUE;
										//word1 = SegAppend( word1, SegDuplicate( busy ) );
									}
									else if(   bFlagA
											  && bFlagB
											  && (! strnicmp( c, "EXISTS", 6 ) )
											 )
									{
										//word1 = SegAppend( word1, SegDuplicate( busy ) );
										lprintf("IF NOT EXISTS is considered superfluous and ignored");
										bFlagA = bFlagB = 0;

									}
									else
									{
										//  								lprintf("adding %s to word1", c);
										word1 = SegAppend( word1, SegDuplicate( busy ) );
									}

								}
								break;
							}//end of switch( *c )
						}//end of else
						break;
					}//end of case 2 of uiSubstateA:
				}//end of switch( uiSubstateA )
				break;
			}//end of case 0 of state:
		case ENU_COLUMNS://   switch(state)
			{
				if( c[0] == ',' )
				{
					xlprintf(DEFAULT_LEVEL)(WIDE("Hm. got %s. This isn't handled. Sorry"), c );
				}
				else if( c[0] == ')' )
				{
					state = ENU_TABLE_OPTIONS;
				}
				else//ok, this is some data.
				{
					//WIDE("  `ID` int(11) NOT NULL default '0',")
					if( c[0] == '`' )
					{
						//xlprintf(DEFAULT_LEVEL)(WIDE("Gotta tick"));
						bFlagB = 0;
						gottatick = TRUE;
						phrase2 = NULL; //cpg02jan2007 phrase2 = SegDuplicate( busy ); // Start the phrase.
						word2 = SegAppend(word2,SegDuplicate(busy));//( c );//fixup for lagging iteration.
						state = ENU_REGULAR_COLUMN;
						uiSubstateA = ENU_BEGIN;

					}
					else
					{
						//WIDE("  ID int(11) NOT NULL default '0',")
						//it's not a tick we got, this is the name of the column. moving right along, then....
						pstColumn->cName =  StrDup( c );
						pstColumn->cType = ( STRING_COLUMN );
						//xlprintf(DEFAULT_LEVEL)(WIDE("%s is a column of type %s without ticks"), pstColumn->cName, types[pstColumn->cType] );
						state = ENU_REGULAR_COLUMN;
						uiSubstateA = ENU_FIRST_WORD;//B-52's: You're living in your own Private Idaho. Get out of that state, get out of the state yer in!
						//word1 = word2 = word3 = NULL;// cheap trigger trick
					}
				}
				break;
			}//end of case ENU_COLUMNS

		case ENU_REGULAR_COLUMN:
			{
				if( ( uiSubstateA != (  ENU_BEGIN     ) ) &&
					( uiSubstateA != (  ENU_NEXT_WORD ) ) &&
					( uiSubstateA != (  ENU_FIRST_WORD) )
				  )
				{
					xlprintf(DEFAULT_LEVEL)(WIDE("Hold it.  What happened?  Why is uiSubstateA != ENU_BEGIN or ENU_NEXT_WORD or ENU_FIRST_WORD?  It's %d.  This is an error condition. "), uiSubstateA );
				}

				if( uiSubstateA == ENU_BEGIN )
				{
					//Lo, and behold.  Verily, this must be the column name.
					switch( *c )
					{
					case ',':
						xlprintf(DEFAULT_LEVEL)(WIDE("Hm. got %s. This isn't handled."), c );
						break;
					case '`':
						{
							if( !gottatick )
							{
								gottatick = TRUE;
							}
							else
							{
								//BEGIN ------------------------------The Column Definition Module, might make this a function---------------------------------------------
								//PTEXT tmp;
								pstColumn->cName = StrDup( GetText( tmp = BuildLine( word1 ) ) );//cpg 2jan2007
								pstColumn->cType = ( STRING_COLUMN );
								LineRelease( tmp );
								tmp = NULL;
								//xlprintf(DEFAULT_LEVEL)(WIDE("%s is a column of type %s"), pstColumn->cName, types[pstColumn->cType] );
								uiSubstateA = ENU_FIRST_WORD;
								gottatick = FALSE;

								LineRelease( word1 );
								word1 = NULL;
								//END  ------------------------------The Column Definition Module, might make this a function---------------------------------------------
							}
						}
						break;// case '`'
					default:
						//hate to be repetitive, but this should be straight-forward enough.
						word1 = SegAppend( word1, SegDuplicate( busy ) );
						if( !gottatick )
						{
							//BEGIN ------------------------------The Column Definition Module, might make this a function---------------------------------------------
							//PTEXT tmp;
							pstColumn->cName =  StrDup( GetText( tmp = BuildLine( word1 ) ) );
							pstColumn->cType = ( STRING_COLUMN );
							xlprintf(DEFAULT_LEVEL)(WIDE("%s is a column of type %s without ticks"), pstColumn->cName, types[pstColumn->cType] );
							uiSubstateA = ENU_FIRST_WORD;//B-52's: You're living in your own Private Idaho. Get out of that state, get out of the state yer in!
							LineRelease( word1 );
							LineRelease( tmp );
							tmp = NULL;
							word1 = word2 = word3 = NULL;// cheap trigger trick
							//END   -----------------------------The Column Definition Module, might make this a function---------------------------------------------

						}

						break;//default
					}
				}
				else if( uiSubstateA == ENU_FIRST_WORD )
				{
					//And as the sun doth shine, that makes this the data type.  This is gonna be tricky.
					if( !( strnicmp( c, WIDE("auto_increment"), 14 ) ) )
					{

						if( bFlagB )
							phrase1 = SegAppend( phrase1, SegDuplicate( busy ));
						else
						{
							phrase1 = SegDuplicate( busy );
							bFlagB = 1;
						}
						uiSubstateA = ENU_NEXT_WORD;

					}
					else if( !( strnicmp( c, WIDE("DEFAULT"), 7 ) ) )
					{
						//xlprintf(DEFAULT_LEVEL)(WIDE("got default, moving to ENU_NEXT_WORD, c is %s"), c);
						if( bFlagB )
							phrase1 = SegAppend( phrase1, SegDuplicate( busy ));
						else
						{
							phrase1 = SegDuplicate( busy );
							bFlagB = 1;
						}
						uiSubstateA = ENU_NEXT_WORD;

					}
					else if( !( strnicmp( c, WIDE("ENUM"), 4 ) ) )
					{
						bEnum = TRUE;
						//xlprintf(DEFAULT_LEVEL)(WIDE("got ENUM"));
						if( bFlagB )
							phrase1 = SegAppend( phrase1, SegDuplicate( busy ));
						else
						{
							phrase1 = SegDuplicate( busy );
							bFlagB = 1;
						}
						uiSubstateA = ENU_NEXT_WORD;
					}
					else if(
							  ( !(strnicmp( c, WIDE("NOT") , 3 ) ) ) ||
							  ( !(strnicmp( c, WIDE("NULL"), 4 ) ) )
							 )
					{
						if( !bFlagB )
						{
							pstColumn->pFld->name = StrDup( pstColumn->cName );
							phrase1 = SegDuplicate( busy );
							xlprintf(DEFAULT_LEVEL)(WIDE("Ok, c is %s and that means pstColumn->pFld->name  is %s"), c, pstColumn->pFld->name );
							bFlagB = 1;
						}
						else
						{
							xlprintf(DEFAULT_LEVEL)(WIDE("I guess we're appending %s") , c );
							phrase1 = SegAppend( phrase1, SegDuplicate( busy ) );
							word2 = NULL;  //cheap trigger trick.
						}

					}
					else if( c[0] == ',' )
					{
						{
							//xlprintf(DEFAULT_LEVEL)(WIDE("Got a comma"));
							//in a regular column definition, the comma is the only thing
							//that tells you the definition is finished, unlike
							//keys (unique key, primary key, key) which has a close-parenthesis
							//telling you the definition is finished.
							if( phrase2 )
							{
								PTEXT out;
								phrase2->format.position.spaces = 0;
								out = BuildLine( phrase2 );
								pstColumn->cType = ( STRING_COLUMN );
								pstColumn->pFld->type = StrDup( GetText(out) );
								//xlprintf(DEFAULT_LEVEL)(WIDE("pstColumn->cType is now %s, as is pstColumn->pFld->type  %s "), types[pstColumn->cType], pstColumn->pFld->type  );
								LineRelease( out );
								LineRelease( phrase2 );
								phrase2=NULL;
								out = NULL;
							}
							else
							{
								pstColumn->pFld->type = NULL;
								pstColumn->cType = NULL;
							}
							if( phrase1 && bFlagB )//bFlagB is used right here in this context to indicate whether phrase1 has been SegDuplicate'd (initialized) or not
							{
								PTEXT out ;
								phrase1->format.position.spaces = 0;
								out = BuildLine( phrase1 );
								pstColumn->pFld->extra = StrDup( GetText( out ) );
								//xlprintf(DEFAULT_LEVEL)(WIDE("pstColumn->pFld->extra is now %s"), pstColumn->pFld->extra );
								LineRelease( out );
								LineRelease( phrase1 ); //release?
								phrase1=NULL;
								out = NULL;

							}
							else
								pstColumn->pFld->extra = NULL;
							AddLink( &listRegularColumn, pstColumn );
							//xlprintf(DEFAULT_LEVEL)(WIDE("%s of type %s with extra %s was added to listRegularColumn"), pstColumn->cName, types[pstColumn->cType], pstColumn->pFld->extra?pstColumn->pFld->extra:WIDE("{nullthing}") );
							//the end. Get ready for the next column definition.....
							n = 0;
							word1 = NULL;
							bFlagB=0;
							uiSubstateA = ENU_BEGIN;
							state = ENU_COLUMNS;
							bInNeedOfColumnAllocation = TRUE;
							//xlprintf(DEFAULT_LEVEL)(WIDE("ok, that should do it for the next column definition, or whatever is on the next line."));
							break;
						}

					}
					else
					{//			"  `unit_type_id` int(11) NOT NULL auto_increment,"
						if( !word2 )// cheap trigger trick
						{
							if( c[0] != '`' ) // as long as it isn't a tick....store it.
							{
								bFlagB = 0;
								phrase2 = SegDuplicate( busy );
								word2 = SegAppend( word2, SegDuplicate( busy ) ); //( c );// cheap trigger trick
							}
						}
						else// cheap trigger trick
						{
							phrase2 = SegAppend( phrase2, SegDuplicate( busy ) );
						}
					}
				}
				else if( uiSubstateA == ENU_NEXT_WORD )
				{
					switch(*c)
					{
					case ' ':
						lprintf("Gotta space in ENU_NEXT_WORK");
						break;
					case ')':
						//xlprintf(DEFAULT_LEVEL)(WIDE("Got a close parenthesis"));

						if( !gottatick )
						{
							if( countParen )
							{
								countParen--;
							}
							//else
							//lprintf("Wait, no open parenthesis, why a close parenthesis?");
						}
						if( bEnum )
						{
							phrase1 = SegAppend( phrase1, SegDuplicate( busy ) );
							bEnum = FALSE;
							break;//aw, heck.  go ahead and keep adding to phrase1.
						}
						//no break here, fall through to the next case.
					case ','://comma
						{
							//xlprintf(DEFAULT_LEVEL)(WIDE("The comma case.  Could be a comma, could be a close parenthesis.  There are %lu open parenthesis."), countParen );
							//in a regular column definition, the comma is the only thing
							//that tells you the definition is finished, unlike
							//keys (unique key, primary key, key) which has a close-parenthesis
							//telling you the definition is finished.
							//This is true except when there are no keys, and then parenthesis closes.
							if( bEnum || countParen ) //aw, heck.  go ahead and keep adding to phrase1.
							{
								phrase1 = SegAppend( phrase1, SegDuplicate( busy ) );
							}
							else
							{
								if( phrase2 )
								{
									PTEXT out;
									phrase2->format.position.spaces = 0;
									out = BuildLine( phrase2 );
									pstColumn->cType = ( STRING_COLUMN );
									pstColumn->pFld->type = StrDup( GetText(out) );
									//pstColumn->pFldtype = StrDup( GetText(out) );
									//xlprintf(DEFAULT_LEVEL)(WIDE("pstColumn->cType is now %s, as is pstColumn->pFld->type  %s "), types[pstColumn->cType], pstColumn->pFld->type );
									LineRelease( out );
									LineRelease( phrase2 );
									phrase2=NULL;
									out = NULL;
								}
								else
									pstColumn->pFld->type = NULL; /* probably invalid!? */

								if( phrase1 )
								{
									PTEXT out ;
									phrase1->format.position.spaces = 0;
									out = BuildLine( phrase1 );
									pstColumn->pFld->extra = StrDup( GetText( out ) );
									//pstColumn->pFld->extra = StrDup( GetText( out ) );
									//xlprintf(DEFAULT_LEVEL)(WIDE("pstColumn->pFld->extra is now %s"), pstColumn->pFld->extra );
									LineRelease( out );
									LineRelease( phrase1 ); //release?
									phrase1=NULL;
									out = NULL;
								}
								else
									pstColumn->pFld->extra = NULL;
								AddLink( &listRegularColumn, pstColumn );
								//xlprintf(DEFAULT_LEVEL)(WIDE("%s of type %s with extra %s was added to both lists"), pstColumn->cName, types[pstColumn->cType], pstColumn->pFld->extra?pstColumn->pFld->extra:WIDE("NULL") );
								//the end. Get ready for the next column definition.....
								n = 0;
								word1 = NULL;
								bFlagB=0;
								uiSubstateA = ENU_BEGIN;
								state = ENU_COLUMNS;
								bInNeedOfColumnAllocation = TRUE;
								//xlprintf(DEFAULT_LEVEL)(WIDE("ok, that should do it for the next column definition, or whatever is on the next line."));
							}
							break;
						}
					default:
						{
							phrase1 = SegAppend( phrase1, SegDuplicate( busy ) );
							break;
						}
					}
				}
			}
			break;
		case ENU_INDEX_KEY:
		case ENU_PRIMARY_KEY:
		case ENU_UNIQUE_KEY:
			{
				switch ( *c )
				{
				case '(':
					//cpg02jan2007
					phrase1 =  NULL;//start the phrase1.
					//					phrase1 = SegDuplicate( busy );//start the phrase1.	//cpg02jan2007
					uiSubstateA = ENU_KEYCOLUMN;//if in case it wasn't clear because there was no optional name here, goto next word.
					xlprintf(DEFAULT_LEVEL)(WIDE("Got a ( moving to ENU_NEXT_WORD"));
					break;
				case ',':
					if( uiSubstateA != ENU_BEGIN )
					{
						//phrase1 = SegAppend( phrase1, SegDuplicate( busy ));
						xlprintf(DEFAULT_LEVEL)(WIDE("gotta comma with %s, c is %s"),GetText(word1), c );
					}
					break;
				case ' ':
					lprintf("Gotta space in ENU_xxx_KEY case");
					break;
				case '`':
					switch( uiSubstateA )
					{
					case ENU_KEYNAME:
						{
							if( !gottatick )
							{
								// enables collect in word1
								gottatick = TRUE;
							}
							else
							{
								//PTEXT tmp;
								gottatick = FALSE;
							SetKeyName:
								xlprintf(DEFAULT_LEVEL)(WIDE("looks like this key is called %s"), GetText(word1));
								pstColumn->cName =  StrDup( GetText( tmp = BuildLine( word1 ) ) );
								LineRelease( tmp );
								LineRelease( word1 );
								tmp = word1 = NULL;
								if( state == ENU_UNIQUE_KEY )
								{
									pstColumn->cType = (  STRING_UNIQUE  );
								}
								else if( state == ENU_PRIMARY_KEY )
								{
									pstColumn->cType = (  STRING_PRIMARY );
								}
								else if( state == ENU_INDEX_KEY )
								{
									pstColumn->cType = ( STRING_INDEX_KEY );
								}
								else
								{
									xlprintf(DEFAULT_LEVEL)(WIDE("What is %lu ??"), state );
									xlprintf(DEFAULT_LEVEL)(WIDE("This is totally bad!"));
								}
								xlprintf(DEFAULT_LEVEL)(WIDE("%s is of type %s  "), pstColumn->cName, types[pstColumn->cType] );
								uiSubstateA = ENU_FINDKEYCOLUMN; // should only be a paren from here?
							}
						}
						break;
					case ENU_FINDKEYCOLUMN:
					case ENU_KEYCOLUMN:
						{
							if( !gottatick )
							{
								gottatick = TRUE;
							}
							else
							{
								PMY_FIELD s;
								gottatick = FALSE;
								s = New(MY_FIELD );
								MemSet( s, 0 , sizeof( MY_FIELD ) );
								{
									//PTEXT tmp;
									s->name = StrDup( GetText( tmp = BuildLine( word1 ) ) );
									LineRelease( tmp );
									LineRelease( word1 );
									tmp = word1 = NULL;
								}
								xlprintf(DEFAULT_LEVEL)(WIDE("Adding link %s"), s->name );
								AddLink(&pstColumn->listFields , s );
								pstColumn->fldCnt++; // implied from listfields forall though...
								//xlprintf(DEFAULT_LEVEL)(WIDE("%s is of type %s  "), pstColumn->cName, types[pstColumn->cType] );
								uiSubstateA = ENU_KEYCOLUMN;
							}
						}
						break;
					default:
						{
							xlprintf(DEFAULT_LEVEL)(WIDE("This is totally bad!"));
						}
					}
					break;
				case ')':
					//in a regular column definition, the comma is the only thing
					//that tells you the definition is finished, unlike
					//keys (unique key, primary key, key) which has a close-parenthesis
					//teling you the definition is finished.
					//cpg02jan2007phrase1 = SegAppend( phrase1, SegDuplicate( busy ));//tack on that close-parenthesis
					/* at this point we should be at SubState = ENU_KEYCOLUMN */
					if( phrase1 )
					{
						PTEXT out;
						phrase1->format.position.spaces = 0;
						out = BuildLine( phrase1 );
						pstColumn->pFld->extra = StrDup( GetText( out ) );
						pstColumn->count++;
						xlprintf(DEFAULT_LEVEL)(WIDE("pstColumn->pFld->extra is now %s"), pstColumn->pFld->extra );
						LineRelease( out );
						LineRelease( phrase1 ); //yeah, get rid of this now.
						phrase1=NULL;
						out = NULL;
					}
					else
						pstColumn->pFld->extra = NULL;
					//  ENU_KEYEXTRA
					AddLink( &listIndexColumn, pstColumn );
					xlprintf(DEFAULT_LEVEL)(WIDE("%s of type %s with extra %s was added to both lists"), pstColumn->cName, types[pstColumn->cType], pstColumn->pFld->extra?pstColumn->pFld->extra:WIDE("NULL") );
					//the end. Get ready for the next column definition.....
					n = 0;
					bFlagA = 0;
					state = ENU_COLUMNS;
					uiSubstateA = ENU_BEGIN;
					gottatick = FALSE;
					bInNeedOfColumnAllocation = TRUE;
					//the end.
					xlprintf(DEFAULT_LEVEL)(WIDE("ok, that should do it for the next column definition, or whatever is on the next line."));
					break;
				default:
					if( uiSubstateA == ENU_KEYNAME )
					{
						word1 = SegAppend( word1, SegDuplicate( busy ) );
						if( !gottatick )
						{
							word1->format.position.spaces = 0;
							goto SetKeyName;
						}
					}

					if( uiSubstateA == ENU_BEGIN )
					{
						if( ( strcmp( c, WIDE("TYPE")) ) && ( strcmp( c, WIDE("BTREE") ) ) )
						{
							phrase1 = SegDuplicate( busy );//start the phrase1.
							word1 = SegAppend( word1, SegDuplicate( busy ) );//StrDup( c );
							xlprintf(DEFAULT_LEVEL)(WIDE("ENU_BEGIN got %s"), GetText( word1 ) );
						}

					}
					else if( uiSubstateA == ENU_FINDKEYCOLUMN )
					{
						if( ( strcmp( c, WIDE("USING") ) ) && ( strcmp( c, WIDE("TYPE") ) ) && ( strcmp( c, WIDE("BTREE") ) ) )
						{
						}
					}
					else if( uiSubstateA == ENU_KEYCOLUMN )
					{
						if( n < MAX_KEY_COLUMNS )
						{
							if( !gottatick )
							{
								PMY_FIELD s;
								s = New(MY_FIELD );
								MemSet( s, 0 , sizeof( MY_FIELD ) );
								{
									//PTEXT tmp;
									s->name = StrDup( GetText( tmp = BuildLine( word1 ) ) );
									LineRelease( tmp );
									LineRelease( word1 );
									tmp = word1 = NULL;
								}
								xlprintf(DEFAULT_LEVEL)(WIDE("Adding link %s"), s->name );
								AddLink(&pstColumn->listFields , s );
								pstColumn->fldCnt++; // implied from listfields forall though...
							}
							else
							{
								word1 = SegAppend( word1, SegDuplicate( busy ) );
							}
						}
					}
					else
					{
						if( gottatick )
							word1 = SegAppend( word1, SegDuplicate( busy ) );
						else
						{
							lprintf( "Ignoring word %s?!", c );
						}
					}
					break;
				}//end of switch( *c )
				break;
			}//end of case ENU_UNIQUE_KEY of state
		case ENU_TABLE_OPTIONS:
			{
				if((  !( strnicmp( c, WIDE("TYPE") , 4 ) ) ) || (!(strnicmp( c, WIDE("ENGINE"), 6 ) ) ) )
				{
					state = ENU_TABLE_OPTION_TYPE;
					uiSubstateA = ENU_BEGIN;
					pstColumn->cName = StrDup( WIDE("TYPE"));

				}
				else if( !( strnicmp( c, WIDE("COMMENT"), 7 ) ) )
				{
					xlprintf(DEFAULT_LEVEL)(WIDE("Gotta comment"));
					state = ENU_TABLE_OPTION_COMMENT;
					uiSubstateA = ENU_BEGIN;
					pstColumn->cName = StrDup( WIDE("COMMENT"));
				}
				break;
			}//end of case ENU_TABLE_OPTIONS of state
		case ENU_TABLE_OPTION_COMMENT:
			{
				xlprintf(DEFAULT_LEVEL)(WIDE("Comment. c is %s") , c );
				if ( uiSubstateA == ENU_BEGIN )
				{
					xlprintf(DEFAULT_LEVEL)(WIDE("Beginning comment. c is %s"), c);
					if( !( StrCmpEx( c, WIDE("="), 1 ) ) )
					{
						uiSubstateA = ENU_NEXT_WORD;
					}
				}
				else if( uiSubstateA == ENU_NEXT_WORD )
				{
					switch( *c )
					{
					case '`'://keep honest people honest.
						break;
					case '\'':
						if( !gottatick)
						{
							//							phrase1 = SegDuplicate( busy );//start the phrase1.
							phrase1 = NULL;//start the phrase1.
							gottatick = TRUE;
						}
						else
						{
							PTEXT out;
							//phrase1 = SegAppend( phrase1, SegDuplicate( busy ));//pick up the trailing apostrophe.
							phrase1->format.position.spaces = 0;
							out = BuildLine( phrase1 );
							pTable->comment = StrDup( GetText(out) );
							xlprintf(DEFAULT_LEVEL)(WIDE("Comment is now %s "), pTable->comment );
							LineRelease( out );
							LineRelease( phrase1 );
							phrase1 = NULL;
							out = NULL;
							gottatick = FALSE;
							state = ENU_TABLE_OPTIONS;//gotta stay here.
							xlprintf(DEFAULT_LEVEL)(WIDE("%s of type %s with extra %s was added to both lists"), pstColumn->cName, types[pstColumn->cType], pstColumn->pFld->extra?pstColumn->pFld->extra:WIDE("NULL") );
							//the end. Get ready for the next column definition.....
							uiSubstateA = ENU_BEGIN;
						}
						break;
					default:
						if( gottatick )
							phrase1 = SegAppend( phrase1, SegDuplicate( busy ));
						else
						{
							//do nothing until we get a tick.
						}

						break;
					}
				}
				break;
			}
		case ENU_TABLE_OPTION_TYPE:
			{
				switch( *c )
				{
				case '\''://keep honest people honest.
				case '`'://keep honest people honest.
				case '=':
					break;
				default:
					pTable->type = StrDup( c );
					//the end. Get ready for the next column definition.....
					uiSubstateA = ENU_BEGIN;
					state = ENU_TABLE_OPTIONS;//gotta stay here.
					break;
				}
				break;
			}
		}//end of switch(state)
		//		busy = NEXTLINE(busy);
	}//end of while(busy)

	// lprintf("Ok. That's it.  Done parsing.  Let's move on...");
	//got some memory that was allocated waiting to be dealt with.
	if( !bInNeedOfColumnAllocation && pstColumn )
	{
		DestroyMyField( pstColumn );
	}

	{
		INDEX ColumnIndex;
		_32 c = 0;
		PMY_GENERIC_DEF pColumn = NULL;// to be alloc'ed during parsing of key.
		PMY_FIELD pFld;//to be alloc'ed during parsing of the field.

		total = 0; n = 0;
		//ok, quickly count the counts. please pardon our dust, we're improving your shopping experience.
		LIST_FORALL( listIndexColumn, ColumnIndex, PMY_GENERIC_DEF, pColumn )
		{

			if( ( (( pColumn->cType == STRING_UNIQUE ) ) ) ||
				(  (( pColumn->cType== STRING_PRIMARY ) ) ) ||
				(  (( pColumn->cType== STRING_INDEX_KEY ) ) )
			  )
			{
				n++;
			}
			else
			{
				xlprintf(DEFAULT_LEVEL)(WIDE("This is totally bad!  pColumn->cType is %s, has to be unique, primary, or index_key"), types[pColumn->cType]);
			}
		}

		LIST_FORALL( listRegularColumn, ColumnIndex, PMY_GENERIC_DEF, pColumn )
		{
			if( pColumn->cType
				&&( ( pColumn->cType== STRING_COLUMN ) )
			  )
			{
				c++;
			}
		}

		// start the output of the save file here.

		if( file )
		{
			vtprintf( pvt, WIDE("\r\n//--------------------------------------------------------------------------"));
			vtprintf( pvt, WIDE("\r\n// %s \r\n// Total number of fields = %d\r\n// Total number of keys = %d"), pTable->name  , c, n);
			vtprintf( pvt, WIDE("\r\n//--------------------------------------------------------------------------"));
			vtprintf( pvt, WIDE("\r\n\r\nFIELD %s_fields[] = { "), ( szNormalizedName ? szNormalizedName : pTable->name ) );
		}
		total = (_32)( sizeof( DB_KEY_DEF ) ) * ( (_32)( n  ) );
		pKeyDef = NewArray( DB_KEY_DEF, n  );
		//xlprintf(DEFAULT_LEVEL)(WIDE("For %lu indexes ( times %lu bytes), so allocated %lu bytes"), n , (sizeof(DB_KEY_DEF)), total );
		MemSet( pKeyDef, 0, total );

		total = (_32)( sizeof( FIELD ) ) * ( (_32)( c ) );
		pFields = NewArray( FIELD , c );
		//		xlprintf(DEFAULT_LEVEL)(WIDE("For %lu regulars ( times %lu bytes), allocated %lu bytes"), c , (sizeof( FIELD) ), total );
		MemSet( pFields, 0, total );

		total = c; c = 0;//we're reusing these for now.

		LIST_FORALL( listRegularColumn, ColumnIndex, PMY_GENERIC_DEF, pColumn )
		{
			if( pColumn->cType
				&& ( (( pColumn->cType == STRING_COLUMN ) ) )
			  )
			{
				if( pColumn->cName )
					pFields[c].name  = StrDup( pColumn->cName );

				pFields[c].type  = StrDup( pColumn->pFld->type );
				pFields[c].extra = StrDup( pColumn->pFld->extra );
				pFields[c].previous_names[0] = 0;
				if( file )
				{
					vtprintf( pvt, WIDE("{\"%s\" , \"%s\" , \"%s\" }"),pColumn->cName, pColumn->pFld->type, pColumn->pFld->extra);
				}

				if( c != ( total-1 ) )
				{
					if( file )
					{
						vtprintf( pvt, WIDE("\r\n\t, "));
					}
				}
				c++;
			}


			DestroyMyField( pColumn );

		}
		if( file )
		{
			vtprintf( pvt, WIDE("\r\n};\r\n\r\n"));
			vtprintf( pvt, WIDE("DB_KEY_DEF %s_keys[] = { "),  ( szNormalizedName ? szNormalizedName : pTable->name ) );
		}

		//		xlprintf(DEFAULT_LEVEL)(WIDE("c is %lu, n is %lu"), c, n);
		total = n; n = 0; //recycle.  be kind, rewind. hug a tree today.

		LIST_FORALL( listIndexColumn, ColumnIndex, PMY_GENERIC_DEF, pColumn )
		{
			if( (( pColumn->cType == STRING_UNIQUE ) ) )
			{
				pKeyDef[n].flags.bUnique = 1; pKeyDef[n].flags.bPrimary = 0;
			}
			else if(  (( pColumn->cType== STRING_PRIMARY ) ) )
			{
				pKeyDef[n].flags.bUnique = 0; pKeyDef[n].flags.bPrimary = 1;
			}
			else if(  (( pColumn->cType== STRING_INDEX_KEY ) ) )
			{
				pKeyDef[n].flags.bUnique = 0; pKeyDef[n].flags.bPrimary = 0;
			}
			else
			{
				//				xlprintf(DEFAULT_LEVEL)(WIDE("This is totally bad!"));
			}

			if( file )
			{
				vtprintf( pvt, WIDE("/*%u*/ %s{ { %lu , %lu } ,")
						  , n
						  , (ColumnIndex)?( WIDE("\r\n\t, ") ):( WIDE("") )
						  , pKeyDef[n].flags.bPrimary , pKeyDef[n].flags.bUnique
						  );
			}

			pKeyDef[n].null = NULL;

			if( pColumn->cName )
			{
				pKeyDef[n].name = StrDup( pColumn->cName );
				if( file )
				{
					vtprintf( pvt, WIDE(" \"%s\" , { "),pColumn->cName);
				}
			}
			else
			{
				//xlprintf(DEFAULT_LEVEL)(WIDE("Wait a minute.  No pColumn->cName? Sometimes a primary key will have no name."));
				pKeyDef[n].name = StrDup(WIDE("") ); // maybe pKeyDef[n].name = NULL;
				if( file ) vtprintf( pvt, WIDE(" NULL , { ") );

				//				xlprintf(DEFAULT_LEVEL)(WIDE("This is totally bad!"));
			}


			if( ( pColumn->fldCnt ) )
			{
				PMY_FIELD s;
				INDEX idx5;

				lprintf( WIDE("found %d fields within a key !!"), pColumn->fldCnt);
				LIST_FORALL( pColumn->listFields, idx5, PMY_FIELD, s )
				{
					if( file ) vtprintf( pvt, WIDE("%s\"%s\""),( (idx5)?( ","):("")) , s->name );
					pKeyDef[n].colnames[idx5] = StrDup( s->name );

				}
				pColumn->fldCnt = 0;
			}
			else
			{
				pFld = pColumn->pFld;
				if( file ) vtprintf( pvt, WIDE("\"%s\""),pFld->extra);
				pKeyDef[n].colnames[0] = StrDup( pFld->extra );
			}

			if( file )
			{
				vtprintf( pvt, WIDE(" } }") );
			}
			n++;
			//xlprintf(DEFAULT_LEVEL)(WIDE("n is now %lu"), n );
			DestroyMyField( pColumn );
		}

		//xlprintf(DEFAULT_LEVEL)(WIDE("Finally, stored %lu keys in pKeyDef, and %lu regular fields"), n , c );
		pTable->fields.count = c;
		pTable->keys.count = (  n );

		if( file )
		{
			vtprintf( pvt, WIDE("\r\n};\r\n\r\n"));
			vtprintf( pvt, WIDE("\r\nTABLE %s = { \"%s\" \r\n"),  ( szNormalizedName ? szNormalizedName : pTable->name )   ,   pTable->name   );
			vtprintf( pvt, WIDE("\t , FIELDS( %s_fields )\r\n"),  ( szNormalizedName ? szNormalizedName : pTable->name ) );
			vtprintf( pvt, WIDE("\t , TABLE_KEYS( %s_keys )\r\n"), ( szNormalizedName ? szNormalizedName : pTable->name )  );
			//			if( ( pTable->type ) || ( pTable->comment ) )
			{
				vtprintf( pvt, WIDE("\t, { 0 }\r\n"));//the table flags.
				if( pTable->database )
				{
					xlprintf(DEFAULT_LEVEL)(WIDE("Yeah, gotta database %s"), pTable->database );
					vtprintf( pvt, WIDE("\t,\"%s\"\r\n"), pTable->database );
				}
				else
				{
					xlprintf(DEFAULT_LEVEL)(WIDE("Nope, using null for database"));
					vtprintf( pvt, WIDE("\t, NULL\r\n") );
				}
				if( pTable->type )
				{
					xlprintf(DEFAULT_LEVEL)(WIDE("Yeah, gotta type %s"), pTable->type );
					vtprintf( pvt, WIDE("\t,\"%s\"\r\n"), pTable->type );
				}
				else
				{
					xlprintf(DEFAULT_LEVEL)(WIDE("Nope, using null for type"));
					vtprintf( pvt, WIDE("\t, NULL\r\n") );
				}
				if( pTable->comment )
				{
					xlprintf(DEFAULT_LEVEL)(WIDE("Yeah, gotta comment. %s"), pTable->comment );
					vtprintf( pvt, WIDE("\t,\"%s\""), pTable->comment );
				}
				else
				{
					xlprintf(DEFAULT_LEVEL)(WIDE("Nope, no comment"));
					vtprintf( pvt, WIDE("\t,NULL")  ); //GetTime
				}
			}
			vtprintf( pvt, WIDE("\r\n};\r\n "));

			fwrite( WIDE(" "),1 ,1, file);
			{
				PTEXT result = VarTextGet( pvt );
				fwrite( GetText( result ), 1 , GetTextSize(result) , file );
				LineRelease( result );
				result=NULL;
			}
			VarTextDestroy( &pvt );
			fclose( file );
		}

		pTable->keys.key =  pKeyDef;
		pTable->fields.field = pFields;
	}
	LineRelease( pNew ); // release the burst buffer
	if( 0 )
	{
		_32 a,b,c,d;
		GetMemStats( &a, &b, &c, &d );
		lprintf( WIDE("Mem stats: %d %d %d %d"), a, b, c, d );
		DebugDumpMem();
	}
	return pTable;
}


SQL_NAMESPACE_END
