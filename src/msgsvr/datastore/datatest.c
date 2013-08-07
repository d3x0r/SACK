
#include <vectlib.h>

#include <../../games/flatland/include/worldstructs.h>

tyepdef struct index_tag
{
	INDEX Vector;
   INDEX Name;
   INDEX Line;
	INDEX Sector;
	INDEX SectorTexture;
	INDEX SectorWorld;
   INDEX Wall;
	INDEX WallWorld;
	INDEX WallSector;
	INDEX WallLine   ;
   INDEX WallWallInto;
   INDEX WallWallStart ;
   INDEX WallWallEnd ;
   INDEX SectorWall ;
} INDEXLIST;

static INDEXLIST i;

// let's just work on exporting things in this header...

#define RegisterDataType(n) RegisterNamedDataType( #n, sizeof( n ) )

PRELOAD( RegisterVectorLibrary )
{
	// okay these members are kinda silly to register
   // so let's just do it for feasibility testing....
	i.Vector = RegisterDataType( VECTOR );
	RegisterDataType( RAY );
   RegisterDataType( RCOORD );
	RegisterDataType( TRANSFORM );

	i.World = RegisterDataType( WORLD );

   i.Line = RegsiterDataType( FLATLAND_MYLINESEG );

	i.Name = RegisterDataType( NAME );

	i.Texture = RegisterDataType( FLATLAND_TEXTURE );
   i.TextureName = CreateLink( i.Texture, offsetof( FLATLAND_TEXTURE, iName ), i.Name );


	i.Sector = RegisterDataType( SECTOR );
	i.SectorTexture = CreateLink( i.Sector, offsetof( SECTOR, iTexture ), i.Texture );
	i.SectorWorld = CreateLink( i.Sector, offsetof( SECTOR, iWorld ), i.World );

	i.Wall = RegisterDataType( WALL );
	i.WallWorld = CreateLink( i.Wall, offset( WALL, iWorld ), i.World );
	i.WallSector = CreateLink( i.Wall, offset( WALL, iSector ), i.Sector );
	i.WallLine = CreateLink( i.Wall, offset( WALL, iLine ), i.Line );
   i.WallWallInto = CreateLink( i.Wall, offsetof( WALL, iWallInto ), i.Wall );
   i.WallWallStart = CreateLink( i.Wall, offsetof( WALL, iWallStart ), i.Wall );
	i.WallWallEnd = CreateLink( i.Wall, offsetof( WALL, iWallEnd ), i.Wall );
	i.WallName = CreateLink( i.Wall, offsetof( WALL, iName ) );

  	i.SectorWall = CreateLink( i.Sector, offsetof( SECTOR, iWall ), i.Wall );



	// hhmmm
   //
}

PWORLD CreateWorld( void )
{
   return CreateDataType( i.World );
}




int main( void )
{
	// begin test program here.
	return 0;
}

