#include <stdio.h>

#include "mem.h"
#include "fileio.h"
#include "./types.h"

typedef struct enum_entry_tag
{
   PTEXT name;
	LONGEST_INT value;
   struct enum_entry_tag *next;
} ENUM_ENTRY, *PENUM_ENTRY;

typedef struct enum_table_tag
{
	PTEXT name;
	PENUM_ENTRY entries;
   PENUM_ENTRY last_entry;
} ENUM_TABLE, *PENUM_TABLE;

typedef struct enum_global_tag
{
	struct {
		uint32_t get_identifier: 1;
		uint32_t current_value_set : 1;
	} flags;
   PENUM_TABLE enumerations;
	PENUM_TABLE current_enum;
   PENUM_ENTRY current_entry;
} ENUM_GLOBAL;

static ENUM_GLOBAL g;

int IsEnumeratedValue( PTEXT word, LONGEST_INT *value )
{
   return FALSE;
}

void ProcessEnum( void )
{
	PTEXT pLine = GetCurrentWord();
   char *text;
	// look for enum keyword....
	while( pLine )
	{
      text = GetText( pLine );
		if( TextIs( pLine, WIDE("enum") ) )
		{
			if( g.current_enum )
			{
				fprintf( stderr, WIDE("%s(%d): Syntax error enumeration within enum?\n")
                    , GetCurrentFileName(), GetCurrentLine() );
			}
			g.current_enum = Allocate( sizeof( ENUM_TABLE ) );
			g.current_enum->entries = NULL;
			g.current_enum->name = NULL;
			if( g.current_entry )
			{
				fprintf( stderr, WIDE("Coding error - uncommited enumeration value. Lost memory.\n") );
            g.current_entry = NULL;
			}
         g.flags.get_identifier = 1;
		}
		else
		{
			if( g.flags.get_identifier )
			{
				if( text[0] != '{' )
				{
					if( g.current_enum->name )
					{
						fprintf( stderr, WIDE("%s(%d): Error multiple identifers for enum\n")
								 , GetCurrentFileName(), GetCurrentLine() );
					}
					else
                  g.current_enum->name = SegDuplicate( pLine );
				}
				else
				{
					// well whether or not there was an identifier (name)
               // we're done getting it....
					g.flags.get_identifier = 0;
				}
			}
			else // collecting enumeration values...
			{
				if( text[0] == '}' )
				{
				}
				else if( text[0] == ',' )
				{
				}
				else if( text[0] == '=' )
				{
				}
            else if( g.current_entry )
				{
					if( text[0] == ',' )
					{
						if( !g.flags.current_value_set )
						{
                     if( g.current_enum->last_entry )
								g.current_entry->value = g.current_enum->last_entry->value + 1;
							else
								g.current_entry->value = 0;
						}
						if( g.current_enum->last_entry )
							g.current_enum->last_entry->next = g.current_entry;
						else
                     g.current_enum->entries = g.current_entry;
						g.current_enum->last_entry = g.current_entry;
                  g.current_entry = NULL;
					}
					else
					{
					}
				}
				else
				{
					if( text[0] == ',' )
					{
                  fprintf( stderr, WIDE("Error - unexpected comma sepeartor enumeration.\n") );
					}
					if( text[0] == '=' )
					{
                  fprintf( stderr, WIDE("Error - unexpected comma sepeartor enumeration.\n") );
					}
					g.current_entry = Allocate( sizeof( ENUM_ENTRY ) );
               g.flags.current_value_set = 0;
					g.current_entry->next = NULL;
					g.current_entry->name = SegDuplicate( pLine );
				}
			}
		}
      pLine = NEXTLINE( pLine );
	}
}

void DestroyEnumerations( void )
{

}

