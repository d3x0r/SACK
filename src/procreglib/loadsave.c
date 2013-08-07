#include <sack_types.h>
#include <sharemem.h>
#include <stdhdrs.h>
#include "registry.h"
#include <procreg.h>
#include "global.h"
//---------------------------------------------------------------------------


PROCREG_PROC( int, SaveTreeEx )( int bDoOpen );

void FindNameSpace( CTEXTSTR name, PNAMESPACE *pSpace, int *pID )
{
	PNAMESPACE NameSpace;
   int ID = 0;
	for( NameSpace = g.NameSpace;
		  NameSpace;
		  NameSpace = NextLink( NameSpace ), ID++ )
	{
		if( name > NameSpace->buffer &&
			(name - NameSpace->buffer) < NameSpace->nextname )
			break;
	}
	if( NameSpace )
	{
      if( pSpace )
			*pSpace = NameSpace;
		if( pID )
			*pID = ID;
	}
}

//---------------------------------------------------------------------------

PNAMESPACE GetNameSpace( PNAMESPACE NameSpace, int ID )
{
	while( NameSpace && ID )
	{
		NameSpace = NextLink( NameSpace );
		ID--;
	}
   return NameSpace;
}

//---------------------------------------------------------------------------

static int WriteName( POINTER user, PTRSZVAL key )
{
	struct {
		_32 bTree : 1;
		_32 bValue : 1;
		_32 bIntVal : 1;
		_32 bProc : 1;
		_32 nNameSpace : 10;   // 0-4096
		_32 nNameOfs : 10; // 0-4096
		// wow still ahve 9 bits left...
	} minname;
	PNAME name;
	name = (PNAME)user;
	// adjust name offsets pointers... maybe write data
	// after this and adjust name to lengths

	{
		int ID;
		PNAMESPACE NameSpace;
		FindNameSpace( name->name, &NameSpace, &ID );
		minname.nNameSpace = ID;
      minname.nNameOfs = name->name - NameSpace->buffer;
		//name.name -= (unsigned int)NameSpace->buffer;
		//name.name += ID << 16;
	}
	if( name->flags.bValue )
	{
	}
#if 0
	else if( name->flags.bProc )
	{
		int ID;
      PNAMESPACE NameSpace;
		//FindNameSpace( name.data.proc.ret
		//				 , &NameSpace, &ID );
		//name.data.proc.ret -= (unsigned int)NameSpace->buffer;
		//name.data.proc.ret += ID << 16;
		FindNameSpace( name->data.proc.name
						 , &NameSpace, &ID );
		name->data.proc.name -= (unsigned int)NameSpace->buffer;
		name->data.proc.name += ID << 16;
		//FindNameSpace( name.data.proc.args
		//				 , &NameSpace, &ID );
		//name.data.proc.args -= (unsigned int)NameSpace->buffer;
		//name.data.proc.args += ID << 16;
	}
#endif
	minname.bTree = ((name->tree.Tree) != NULL);
	minname.bValue = name->flags.bValue;
   minname.bIntVal = name->flags.bIntVal;
   minname.bProc = name->flags.bProc;
   fwrite( &minname, 1, sizeof( minname ), g.file );
	if( name->tree.Tree )
	{
      DumpTree( &name->tree, WriteName );
	}
	else if( name->flags.bValue )
	{
		int len;
      // 2 bytes of length should be plenty ...
		if( name->flags.bIntVal )
		{
			fwrite( &name->data.name.iValue, 1, sizeof( name->data.name.iValue ), g.file );
		}
		else
		{
			if( name->data.name.sValue )
				len = strlen( name->data.name.sValue );
			else
				len = 0;
			fwrite( &len, 1, 2, g.file );
			fwrite( name->data.name.sValue, 1, len, g.file );
		}
	}

   //if( name.flags.bTree )
	//	fwrite( &key, 1, sizeof( key ), g.file );
   return 0;
}

//---------------------------------------------------------------------------


PROCREG_PROC( int, SaveTreeEx )( int bDoOpen )
{
	if( bDoOpen )
		g.file = fopen( WIDE("Registered.Tree"), WIDE("wb") );
	if( g.file )
	{
		DumpTree( g.Names, WriteName );
		if( bDoOpen )
			fclose( g.file );
		g.file = NULL;
	}
   return 0;
}

//---------------------------------------------------------------------------

PROCREG_PROC( int, SaveTree )( void )
{
	while( g.file )
		Relinquish();
	g.file = fopen( WIDE("Names.Data"), WIDE("wb") );
   if( g.file )
	{
		PNAMESPACE NameSpace;
		NameSpace = g.NameSpace;
		for( NameSpace = g.NameSpace; NameSpace; NameSpace = NextLink( NameSpace ) )
		{
         fwrite( &NameSpace->nextname, 1, sizeof( NameSpace->nextname ), g.file );
			fwrite( NameSpace->buffer
					, 1
					, NameSpace->nextname
               , g.file );
		}
		fclose( g.file );
	}
   return SaveTreeEx( TRUE );
}

//---------------------------------------------------------------------------

PROCREG_PROC( PTREEDEF, LoadTreeEx )( PTREEDEF root
												 , PNAMESPACE NameSpace
												 , int bDoOpen )
{
	if( !root )
	{
		lprintf( WIDE("Everything is bad - fell off the tree while adding values") );
      return NULL;
	}
   if( bDoOpen )
		g.file = fopen( WIDE("Registered.Tree"), WIDE("rb") );

	if( g.file )
	{
		struct {
			_32 bTree : 1;
			_32 bValue : 1;
			_32 bProc : 1;
			_32 nNameSpace : 10;   // 0-4096
			_32 nNameOfs : 10; // 0-4096
			// wow still ahve 9 bits left...
		} minname;
		while( fread( &minname, 1, sizeof( minname ), g.file ) )
		{
         char *pName = GetNameSpace(NameSpace,minname.nNameSpace)->buffer
				+ minname.nNameOfs;
         if( minname.bTree )
			{
            LoadTreeEx( GetClassTreeEx( root, pName, NULL), NameSpace, FALSE );
			}
			else
			{
				PNAME name = Allocate( sizeof( NAME ) );
				name->flags.bValue = minname.bValue;
				name->flags.bProc = minname.bProc;
				name->name = SaveName( pName );
				if( name->flags.bValue )
				{
					if( name->flags.bIntVal )
					{
						fread( &name->data.name.iValue, 1, sizeof( name->data.name.iValue ), g.file );
					}
					else
					{
						unsigned short len;
						fread( &len, 1, 2, g.file );
						name->data.name.sValue = Allocate( len + 1 );
						fread( (char*)name->data.name.sValue, 1, len, g.file );
						// now add name...
						if( !AddBinaryNode( root->Tree, name, (PTRSZVAL)name->name ) )
						{
							Log( WIDE("Failed to add name to tree...") );
							Release( name );
						}
					}
				}
				else if( name->flags.bProc )
				{
               // not sure what to do here cause I haven't saved one.
				}
			}
		}
		//DumpTree( g.Names, ReadName );
		if( bDoOpen )
		{
			fclose( g.file );
			g.file = NULL;
		}
	}
   return 0;
}

//---------------------------------------------------------------------------

PROCREG_PROC( int, LoadTree )( void )
{
	PNAMESPACE pMyNameSpace = NULL;
	PNAMESPACE NameSpace;
	while( g.file )
		Relinquish();
	g.file = fopen( WIDE("Names.Data"), WIDE("rb") );
	{
      NameSpace = Allocate( sizeof( NAMESPACE ) );
		while( fread( &NameSpace->nextname, 1, sizeof( NameSpace->nextname ), g.file ) )
		{
			fread( NameSpace->buffer, 1, NameSpace->nextname, g.file );
			NameSpace->next = NULL;
         NameSpace->me = NULL;
			LinkLast( pMyNameSpace, PNAMESPACE, NameSpace );
         NameSpace = Allocate( sizeof( NAMESPACE ) );
		}
      Release( NameSpace );
		fclose( g.file );
	}
	// pass my namespace so that duplicates are used..
	// now what to do about names both in this space
	// and the existing space? I guess just add them in...
	// therefore this space is merely used as a reference to
   // know what names to use when building values...
	LoadTreeEx( g.Names, pMyNameSpace, TRUE );
	// delete my name space which has been rolled into existing name space.
	{
		PNAMESPACE next, cur;
		next = pMyNameSpace;
		while( ( cur = next ) )
		{
         next = NextLink( cur );
         Release( cur );
		}
	}
   return 1;
}

