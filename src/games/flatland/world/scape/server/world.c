#define WORLD_SERVICE
#define WORLD_SOURCE

#include <stdhdrs.h> // debugbreak
#include <pssql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define DEFINE_DEFAULT_IMAGE_INTERFACE
#define NEED_VECTLIB_COMPARE

#include <sharemem.h>
#include <logging.h>

#include "world.h"
#include "lines.h"
#include "sector.h"
#include "names.h"
#include "walls.h"

#include "global.h"
#include "service.h"

//#define LOG_SAVETIMING

extern GLOBAL g;
//----------------------------------------------------------------------------

INDEX CreateSquareSector( INDEX iWorld, PC_POINT pOrigin, RCOORD size )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PSECTOR pSector;
	INDEX iSector;
	_POINT o, n;
	pSector = GetFromSet( SECTOR, &world->sectors );
	iSector = GetMemberIndex( SECTOR, &world->sectors, pSector );
	/*
	{
		char name[20];
		sprintf( name, "%d", SectorIDs++ );
		pSector->name = GetName( &world->names, name );
	}
	*/
	pSector->iName = INVALID_INDEX;
	pSector->iWorld = iWorld;
	/* east */
	addscaled( o, pOrigin, VectorConst_X, size/2 );
	scale( n, VectorConst_Y, 20 );
	// creates an open line....
	pSector->iWall = CreateWall( iWorld, iSector, INVALID_INDEX, FALSE, INVALID_INDEX, FALSE, o, n );
	/* south */
	addscaled( o, pOrigin, VectorConst_Y, size/2 );
	scale( n, VectorConst_X, -20 );
	// creates a wall whos start is at the start of the wall...
	CreateWall( iWorld, iSector, pSector->iWall, TRUE, INVALID_INDEX, FALSE, o, n );
	/* west */
	addscaled( o, pOrigin, VectorConst_Y, -size/2 );
	scale( n, VectorConst_X, -20 );
	CreateWall( iWorld, iSector, pSector->iWall, FALSE, INVALID_INDEX, FALSE, o, n );
	/* north */
	addscaled( o, pOrigin, VectorConst_X, -size/2 );
	scale( n, VectorConst_Y, 20 );
	{
		PWALL pWall = GetSetMember( WALL, &world->walls, pSector->iWall );
		CreateWall( iWorld, iSector
					 , pWall->iWallStart, TRUE
					 , pWall->iWallEnd, TRUE, o, n );
	}
	//ComputeSectorPointList( iWorld, iSector, NULL );
	ComputeSectorOrigin( iWorld, iSector );
	SetPoint( pSector->r.n, VectorConst_Y );
	//DumpWall( pSector->wall );
	//DumpWall( pSector->wall->wall_at_start );
	//DumpWall( pSector->wall->wall_at_end );
	//DumpWall( pSector->wall->wall_at_start->wall_at_end );
	{
		INDEX texture = MakeTexture( iWorld, MakeName( iWorld, WIDE("Default") ) );
		PFLATLAND_TEXTURE pTexture = GetSetMember( FLATLAND_TEXTURE, &world->textures, texture );
		if( !pTexture->flags.bColor )
			SetSolidColor( iWorld, texture, AColor( 170, 170, 170, 0x80 ) );
		SetTexture( iWorld, iSector, texture );
	}
	MarkSectorUpdated( iWorld, iSector );
	return iSector;
}

//----------------------------------------------------------------------------

uintptr_t CPROC CompareWorldName( INDEX iWorld, uintptr_t psv )
{
	CTEXTSTR name = (CTEXTSTR)psv;
	PWORLD world = GetUsedSetMember( WORLD, &g.worlds, iWorld );
	TEXTCHAR buffer[256];
	GetNameText( iWorld, world->name, buffer, sizeof( buffer ) );
	//   GetNameText( iWorld, world->name,
	if( StrCmp( buffer, name ) == 0 )
		return (uintptr_t)world;
	return 0;
}


INDEX ServerSideOpenWorld( PSERVICE_ROUTE iClient, CTEXTSTR name )
{
	PWORLD world;
	INDEX iWorld;
	uintptr_t psvResult;
	psvResult = ForEachSetMember( WORLD, g.worlds, CompareWorldName, (uintptr_t)name );
	if( psvResult )
	{
		world = (PWORLD)psvResult;
		iWorld = GetMemberIndex( WORLD, &g.worlds, world );
		MarkAllInWorldUpdated( iClient, iWorld );
	}
	else
	{
		world = GetFromSet( WORLD, &g.worlds );
		InitializeCriticalSec( &world->csDeletions );
		iWorld = GetMemberIndex( WORLD, &g.worlds, world );
		world->name = MakeName( iWorld, name );
		// getfromset initializes to zero now.
		//world->lines = NULL;
		//world->walls = NULL;
		//world->sectors = NULL;
		//world->names = NULL;
		//world->textures = NULL;
		//world->spacetree = NULL;
		ClientCreateWorld_init( iClient, iWorld );

		MarkSectorUpdated( iWorld, CreateSquareSector( iWorld, VectorConst_0, 50 ) );
	}
	return iWorld;
}

//----------------------------------------------------------------------------

INDEX CreateBasicWorld( INDEX iClient )
{
	INDEX world = ServerSideOpenWorld( iClient, WIDE("Default world") );
	CreateSquareSector( world, VectorConst_0, 50 );
	return world;
}

//----------------------------------------------------------------------------
WORLD_PROC( PSECTORSET, GetSectors )( INDEX iWorld )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	if( world )
		return world->sectors;
	return NULL;
}
//----------------------------------------------------------------------------
WORLD_PROC( PWALLSET, GetWalls )( INDEX iWorld )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	if( world )
		return world->walls;
	return NULL;
}
//----------------------------------------------------------------------------
WORLD_PROC( PFLATLAND_MYLINESEGSET, GetLines )( INDEX iWorld )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	if( world )
		return world->lines;
	return NULL;
}
//----------------------------------------------------------------------------
WORLD_PROC( uint32_t, GetSectorCount )( INDEX iWorld )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	if( world )
		return CountUsedInSet( SECTOR, world->sectors );
	return 0;
}
//----------------------------------------------------------------------------
WORLD_PROC( uint32_t, GetWallCount )( INDEX iWorld )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
   if( world )
		return CountUsedInSet( WALL, world->walls );
   return 0;
}
//----------------------------------------------------------------------------
WORLD_PROC( uint32_t, GetLineCount )( INDEX iWorld )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	if( world )
		return CountUsedInSet( FLATLAND_MYLINESEG, world->lines );
	return 0;
}
//----------------------------------------------------------------------------

WORLD_PROC( INDEX, GetWallSector )( INDEX iWorld, INDEX iWall )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PWALL wall = GetSetMember( WALL, &world->walls, iWall );
	if( wall )
		return wall->iSector;
	return INVALID_INDEX;
}

//----------------------------------------------------------------------------

void ResetWorld( INDEX iWorld )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	//SectorIDs = 0;
	EnterCriticalSec( &world->csDeletions );
	DeleteSectors( iWorld );// deletes walls also...

	// should probably check that all walls are now now treferences...
	DeleteWalls( iWorld );
	// should probably check that all lines are now now treferences...
	DeleteLines( iWorld );

	DeleteTextures( iWorld );

	DeleteNames( iWorld );

	// when using this internally... we have to update clients.
	UpdateClients();
	LeaveCriticalSec( &world->csDeletions );

	//if( world->spacetree )
	//{
	//	Log( "Spaces remained on the tree..." );
	//	DeleteSpaceTree( &world->spacetree );
	//}
}

//----------------------------------------------------------------------------

#define WriteSize( name ) {                                            \
			name##pos = ftell( pFile );                                   \
			name##size = 0;                                               \
			sz += fwrite( &name##size, 1, sizeof( name##size ), pFile );  \
			}

#define UpdateSize(name) { int endpos;                          \
			endpos = ftell( pFile );                               \
			fseek( pFile, name##pos, SEEK_SET );                   \
			fwrite( &name##size, 1, sizeof( name##size ), pFile ); \
			fseek( pFile, endpos, SEEK_SET );                      \
			sz += name##size; }                                    

CTEXTSTR table = WIDE("create table world_walls ( world_id int(11), world_wall_id int(11)")
		WIDE(", world_line_id int(11)")
		WIDE(", world_sector_id int(11)")
		WIDE(", flag_wall_end int(1), flag_wall_start int(1)")
		WIDE(", world_wall_id_into int(11), world_wall_id_start int(11), world_wall_id_end int(11)")
		WIDE(", PRIMARY KEY (world_id,world_wall_id )");
CTEXTSTR table2 = WIDE("create table world_lines ( world_id int(11), world_line_id int(11)")
		WIDE(", start float, end float")
		WIDE(", r_o_0 float, r_n_0 float")
		WIDE(", r_o_1 float, r_n_1 float")
		WIDE(", r_o_2 float, r_n_2 float")
		WIDE(", PRIMARY KEY (world_id,world_line_id )");
CTEXTSTR table3 = WIDE("create table world_sectors ( world_id int(11), world_sector_id int(11)")
		WIDE(", world_wall_id int(11)")
		WIDE(", world_texture_id int(11)")
		WIDE(", world_name_id int(11)")
		WIDE(", flags int(11)")
		WIDE(", PRIMARY KEY (world_id,world_sector_id )");

CTEXTSTR table4 = WIDE("create table world_textures ( world_id int(11), world_texture_id int(11)")
		WIDE(", world_name_id int(11)")
		WIDE(", is_color int")
		WIDE(", color int")
		WIDE(", PRIMARY KEY (world_id,world_texture_id )");

FIELD worlds_fields[] = {
	{ WIDE("world_id"), WIDE("int(11)"), WIDE("auto_increment") }
	, { WIDE("name"), WIDE("int(11)") }
	, { WIDE("version"), WIDE("int(11)") }
};

DB_KEY_DEF worlds_keys[] = {
#ifdef __cplusplus
	required_key_def( 1, 0, NULL, WIDE("world_id") )
	, required_key_def( 0, 1, WIDE("name"), WIDE("name") )
#else

	{ {1,0}, NULL, { WIDE("world_id") } }
	, { { 0, 1 }, WIDE("dictionary"), { WIDE("name") } }

#endif
};

TABLE worlds = { 
	 WIDE("worlds"), FIELDS( worlds_fields )
	 , TABLE_KEYS( worlds_keys )
};

CTEXTSTR table5 = WIDE("create table world_name_lines ( world_id int, world_name_id int")
		WIDE(", line_order int, text varchar(64)")
		WIDE(", UNIQUE KEY( world_id,world_name_id,line_order) )");

FIELD world_names_fields[] = {
	{ WIDE("world_name_id"), WIDE("int(11)") }
	, { WIDE("world_id"), WIDE("int(11)") }
	, { WIDE("flags"), WIDE("int(11)") }
	, { WIDE("text"), WIDE("varchar(128)"), NULL }
};


DB_KEY_DEF world_names_keys[] = { 

#ifdef __cplusplus

	required_key_def( 1, 0, NULL, WIDE("world_name_id") )
	, required_key_def( 0, 1, WIDE("dictionary"), WIDE("name") )
#else

	{ {1,0}, NULL, { WIDE("world_name_id") } }
	, { { 0, 1 }, WIDE("dictionary"), { WIDE("name") } }

#endif

};





TABLE world_names = { WIDE("world_names" )
	 , FIELDS( world_names_fields )
	 , TABLE_KEYS( world_names_keys )
};

void InitWorldsDb()
{
	static LOGICAL inited;
	if( !inited )
	{
		PODBC odbc = ConnectToDatabase( WIDE("worlds.db") );
		CheckODBCTable( odbc, &world_names, CTO_MERGE );
		CheckODBCTable( odbc, &worlds, CTO_MERGE );
		{
			PTABLE ptable;
			ptable = GetFieldsInSQL( table, TRUE );
			CheckODBCTable( odbc, ptable, CTO_MERGE );
			//DestroySQLTable( &ptable );
			ptable = GetFieldsInSQL( table2, TRUE );
			CheckODBCTable( odbc, ptable, CTO_MERGE );
			ptable = GetFieldsInSQL( table3, TRUE );
			CheckODBCTable( odbc, ptable, CTO_MERGE );
			ptable = GetFieldsInSQL( table4, TRUE );
			CheckODBCTable( odbc, ptable, CTO_MERGE );
		}
		inited=TRUE;
		CloseDatabase( odbc );
	}
}


int SaveData( PODBC odbc, INDEX world_id, INDEX iWorld )
{
	PWORLD pWorld = GetSetMember( WORLD, &g.worlds, iWorld );
	int sz = 0, tmp, cnt;
	int nlines;
	PFLATLAND_MYLINESEG *linearray;
	int nwalls;
	PWALL *wallarray;
	int sectorpos;
	int nsectors;
	PSECTOR *sectorarray;
	int nnames;
	PNAME *namearray;
	int texturesize, texturepos;
	int ntextures;
	PFLATLAND_TEXTURE *texturearray;

#ifdef LOG_SAVETIMING
	uint32_t begin, start = GetTickCount();
	begin = start;
#endif
	linearray   = GetLinearLineArray( pWorld->lines, &nlines );
	wallarray   = GetLinearWallArray( pWorld->walls, &nwalls );
	sectorarray = GetLinearSectorArray( pWorld->sectors, &nsectors );
	namearray   = GetLinearNameArray( pWorld->names, &nnames );
	texturearray = GetLinearTextureArray( pWorld->textures, &ntextures );

#ifdef LOG_SAVETIMING
	Log1( WIDE("Built arrays: %d"), GetTickCount() - start );
	start = GetTickCount();
#endif

	//-------------------------------------------------
	// Version 0: did not save "FLAT" tag - assumes version 1 info
	// Version 1: had extra nInto - sector reference for wall links...
	// Version 2: didn't compute sector origins, need to fix 
	// Version 3: lacked having sector ID saved
	// Version 4: lacked a text name per sector
	//   new section introduced - NAME ... referenced name sets
	//   for (walls?) and sectors.
	// version 5: lacked a count of lines in names - assume one line. 
	// Version 6: added flags to name type
	// version 7: (see version 8 notes for changes from 7)
	// version 8: Added Texture information (default texture for prior versions)
	//            internal added sector point sets (computed on load)
	//            internal added line reference count (computed on load)
	//            added current grid sizes/ grid color (saved in flatland)
	// version 9: flipped image upside down so +Y is 'top' and -Y is 'low'
	//            any version less than this must mult Y by -1 on load.
	// version 10: going to add sizes per section(added), and consider
	//            splitting off 'section' handlers keyed on 
	//            section names - will need coordination though :/ .. not added
	// (current - no changes to save or load)
	// version 11: 
	//-------------------------------------------------

	SQLCommand( odbc, WIDE("Begin transaction") );

	SQLCommandf( odbc, WIDE("update worlds set version=%d where world_id=%d"), CURRENTSAVEVERSION, world_id );

	
	//----- write lines -------
	SQLCommandf( odbc, WIDE("delete from world_lines where world_id=%d"), world_id );

	//sz += fwrite( "LINE", 1, 4, pFile );
	//WriteSize( line );
	//linesize += fwrite( &nlines, 1, sizeof(nlines), pFile );
	for( cnt = 0; cnt < nlines; cnt++ )
	{
		SQLCommandf( odbc, WIDE("insert into world_lines (world_id,world_line_id")
			WIDE(",r_o_0,r_o_1,r_o_2,r_n_0,r_n_1,r_n_2,start,end) ")
			WIDE("values (%d,%d,%lg,%lg,%g,%g,%g,%g,%g,%g)")
			, world_id
			, cnt + 1
			, linearray[cnt]->r.o[0]
			, linearray[cnt]->r.o[1]
			, linearray[cnt]->r.o[2]
			, linearray[cnt]->r.n[0]
			, linearray[cnt]->r.n[1]
			, linearray[cnt]->r.n[2]
			, linearray[cnt]->dFrom
			, linearray[cnt]->dTo
			);
	}
#ifdef LOG_SAVETIMING
	Log1( WIDE("Wrote Lines: %d"), GetTickCount() - start );
	start = GetTickCount();
#endif

	SQLCommandf( odbc, WIDE("delete from world_walls where world_id=%d"), world_id );
	for( cnt = 0; cnt < nwalls; cnt++ )
	{
		FILEWALLV2 WriteWall;
		PWALL pwall = wallarray[cnt];
		WriteWall.flags = pwall->flags;

		tmp = WriteWall.nSector = FindInArray( (POINTER*)sectorarray, nsectors, GetUsedSetMember( SECTOR, &pWorld->sectors, pwall->iSector ) );
		if( tmp < 0 )
			Log1( WIDE("Failed to find referenced sector... save will fail %d"), cnt );

		tmp = WriteWall.nLine = FindInArray( (POINTER*)linearray, nlines, GetUsedSetMember( FLATLAND_MYLINESEG, &pWorld->lines, pwall->iLine ) );
		if( tmp < 0 )
			Log1( WIDE("Failed to find referenced line... save will fail %d"), cnt );

		if( pwall->iWallInto != INVALID_INDEX )
		{
			tmp = WriteWall.nWallInto = FindInArray( (POINTER*)wallarray, nwalls, GetUsedSetMember( WALL, &pWorld->walls, pwall->iWallInto ) );
			if( tmp < 0 )
				Log1( WIDE("Failed to find referenced Intoing wall... save will fail %d"), cnt );
		}
		else
			WriteWall.nWallInto = INVALID_INDEX; // +1 will be 0.
		tmp = WriteWall.nWallStart = FindInArray( (POINTER*)wallarray, nwalls, GetUsedSetMember( WALL, &pWorld->walls, pwall->iWallStart ) );
		if( tmp < 0 )
			Log1( WIDE("Failed to find referenced starting wall... save will fail %d"), cnt );

		tmp = WriteWall.nWallEnd = FindInArray( (POINTER*)wallarray, nwalls, GetUsedSetMember( WALL, &pWorld->walls, pwall->iWallEnd ) );
		if( tmp < 0 )
			Log1( WIDE("Failed to find referenced ending wall... save will fail %d"), cnt );

		SQLCommandf( odbc, WIDE("insert into world_walls (world_id, world_wall_id")
			WIDE(", world_line_id")
			WIDE(", world_sector_id")
			WIDE(", world_wall_id_into, world_wall_id_start, world_wall_id_end, flag_wall_end, flag_wall_start )")
			WIDE("values (%d,%d,%d,%d,%d,%d,%d,%d,%d)") 
			, world_id
			, cnt + 1
			, WriteWall.nLine + 1
			, WriteWall.nSector + 1
			, WriteWall.nWallInto + 1
			, WriteWall.nWallStart + 1
			, WriteWall.nWallEnd + 1
			, pwall->flags.wall_end_end
			, pwall->flags.wall_start_end
			);
	}
#ifdef LOG_SAVETIMING
	Log1( WIDE("Wrote Walls: %d"), GetTickCount() - start );
	start = GetTickCount();
#endif
	SQLCommandf( odbc, WIDE("delete from world_sectors where world_id=%d"), world_id );
	for( cnt = 0; cnt < nsectors; cnt++ )
	{
		FILESECTORV8 WriteSector;
		WriteSector.nName = FindInArray( (POINTER*)namearray, nnames, GetUsedSetMember( NAME, &pWorld->names, sectorarray[cnt]->iName ) );
		//WriteSector.nID = sectorarray[cnt]->nID;
		WriteSector.flags = sectorarray[cnt]->flags;
		SetRay( &WriteSector.r, &sectorarray[cnt]->r );
		tmp = WriteSector.nwall = FindInArray( (POINTER*)wallarray, nwalls, GetUsedSetMember( WALL, &pWorld->walls, sectorarray[cnt]->iWall ) );
		if( tmp < 0 )
			Log1( WIDE("Failed to find wall referenced by sector.. save failing %d"), cnt );
		WriteSector.nTexture = FindInArray( (POINTER*)texturearray, ntextures, GetUsedSetMember( FLATLAND_TEXTURE, &pWorld->textures, sectorarray[cnt]->iTexture ) );
		if( tmp < 0 )
			Log1( WIDE("Failed to find texture referenced by sector.. save failing %d"), cnt );
		SQLCommandf( odbc, WIDE("insert into world_sectors (world_sector_id,world_id,world_texture_id,world_name_id,world_wall_id, flags)")
			WIDE(" values (%d,%d,%d,%d,%d,%d)")
			, cnt + 1
			, world_id
			, WriteSector.nTexture + 1
			, WriteSector.nName + 1
			, WriteSector.nwall + 1
			, WriteSector.flags
			);
	}
#ifdef LOG_SAVETIMING
	Log1( WIDE("Wrote Sectors: %d"), GetTickCount() - start );
	start = GetTickCount();
#endif
	SQLCommandf( odbc, WIDE("delete from world_names where world_id=%d"), world_id );
	for( cnt = 0; cnt < nnames; cnt++ )
	{
		int l;
		uint16_t lines = namearray[cnt]->lines;
		PVARTEXT pvt = VarTextCreate();
		for( l = 0; l < lines; l++ )
		{
			vtprintf( pvt, WIDE("%s%s")
				, namearray[cnt]->name[l].name
				, (l < (lines-1))?WIDE("\n"):WIDE("") );
		}
		SQLCommandf( odbc, WIDE("insert into world_names (world_id,world_name_id,text,flags) values (%d,%d,'%s',%d)")
			, world_id
			, cnt + 1
			, GetText( VarTextPeek( pvt ) )
			, namearray[cnt]->flags
			);
		VarTextDestroy( &pvt );
	}
#ifdef LOG_SAVETIMING
	Log1( WIDE("Wrote Names: %d"), GetTickCount() - start );
	start = GetTickCount();
#endif
	SQLCommandf( odbc, WIDE("delete from world_textures where world_id=%d"), world_id );
	for( cnt = 0; cnt < ntextures; cnt++ )
	{
		int nName = FindInArray( (POINTER*)namearray, nnames, GetUsedSetMember( NAME, &pWorld->names, texturearray[cnt]->iName ) );
		SQLCommandf( odbc, WIDE("insert into world_textures (world_id,world_texture_id,world_name_id,is_color,color) ")
			WIDE("values (%d,%d,%d,%d,%d)")
			, world_id, cnt + 1, nName + 1
			, texturearray[cnt]->flags.bColor 
			, texturearray[cnt]->data.color );
	}
	SQLCommand( odbc, WIDE("commit") );

#ifdef LOG_SAVETIMING
	Log1( WIDE("Wrote Textures: %d"), GetTickCount() - start );
	start = GetTickCount();
#endif

#ifdef LOG_SAVETIMING
	Log1( WIDE("Wrote File: %d"), GetTickCount() - begin );
#endif

	Release( texturearray );
	Release( namearray );
	Release( sectorarray );
	Release( wallarray );
	Release( linearray );
#ifdef LOG_SAVETIMING
	Log1( WIDE("Released arrays: %d"), GetTickCount() - start );
#endif

	return sz;
}

int SaveWorld( INDEX iWorld )
{
	PODBC odbc = ConnectToDatabase( WIDE("worlds.db") );

	if( odbc )
		
	{
		PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
		TEXTCHAR buffer[256];
		InitWorldsDb();
		GetNameText( iWorld, world->name, buffer, sizeof( buffer ) );
		{
 			INDEX world_id = SQLReadNameTable( odbc, buffer, WIDE("worlds"), WIDE("world_id") );
			
			SaveData( odbc, world_id, iWorld );

		}
	    CloseDatabase( odbc );
		return 1;
	}
	return 0;
}
#ifdef FIXED_FILE_SAVE_ERR_USE_DATABASE
int OldSaveWorldToFile
{
	PWORLD pWorld = GetSetMember( WORLD, &g.worlds, iWorld );
	int sz = 0, tmp, cnt;
	int worldpos;
	int linesize, linepos;
	int nlines;
	PFLATLAND_MYLINESEG *linearray;
	int wallsize, wallpos;
	int nwalls;
	PWALL *wallarray;
	int sectorsize, sectorpos;
	int nsectors;
	PSECTOR *sectorarray;
	int namesize, namepos;
	int nnames;
	PNAME *namearray;
	int texturesize, texturepos;
	int ntextures;
	PFLATLAND_TEXTURE *texturearray;

#ifdef LOG_SAVETIMING
	uint32_t begin, start = GetTickCount();
	begin = start;
#endif
	linearray   = GetLinearLineArray( pWorld->lines, &nlines );
	wallarray   = GetLinearWallArray( pWorld->walls, &nwalls );
	sectorarray = GetLinearSectorArray( pWorld->sectors, &nsectors );
	namearray   = GetLinearNameArray( pWorld->names, &nnames );
	texturearray = GetLinearTextureArray( pWorld->textures, &ntextures );

#ifdef LOG_SAVETIMING
	Log1( WIDE("Built arrays: %d"), GetTickCount() - start );
	start = GetTickCount();
#endif
	fwrite( "FLAT", 1, 4, pFile );
	//-------------------------------------------------
	// Version 0: did not save "FLAT" tag - assumes version 1 info
	// Version 1: had extra nInto - sector reference for wall links...
	// Version 2: didn't compute sector origins, need to fix 
	// Version 3: lacked having sector ID saved
	// Version 4: lacked a text name per sector
	//   new section introduced - NAME ... referenced name sets
	//   for (walls?) and sectors.
	// version 5: lacked a count of lines in names - assume one line. 
	// Version 6: added flags to name type
	// version 7: (see version 8 notes for changes from 7)
	// version 8: Added Texture information (default texture for prior versions)
	//            internal added sector point sets (computed on load)
	//            internal added line reference count (computed on load)
	//            added current grid sizes/ grid color (saved in flatland)
	// version 9: flipped image upside down so +Y is 'top' and -Y is 'low'
	//            any version less than this must mult Y by -1 on load.
	// version 10: going to add sizes per section(added), and consider
	//            splitting off 'section' handlers keyed on 
	//            section names - will need coordination though :/ .. not added
	// (current - no changes to save or load)
	// version 11: 
	//-------------------------------------------------
	tmp = CURRENTSAVEVERSION;  

	// writesize( pWorld )
	worldpos = ftell( pFile );
	fwrite( &sz, 1, sizeof( sz ), pFile ); 
	sz = 0;

	Log1( WIDE("Saving version %d"), tmp );
	sz += fwrite( &tmp, 1, sizeof( tmp ), pFile );

	//----- write lines -------
	sz += fwrite( WIDE("LINE"), 1, 4, pFile );
	WriteSize( line );
	linesize += fwrite( &nlines, 1, sizeof(nlines), pFile );
	for( cnt = 0; cnt < nlines; cnt++ )
	{
		LINESEGFILE lsf;
      SetRay( &lsf.r, &linearray[cnt]->r );
      lsf.start = linearray[cnt]->dFrom;
      lsf.end   = linearray[cnt]->dTo;
		linesize += fwrite( &lsf, 1, sizeof( LINESEGFILE ), pFile );
	}
	UpdateSize( line );
#ifdef LOG_SAVETIMING
	Log1( WIDE("Wrote Lines: %d"), GetTickCount() - start );
	start = GetTickCount();
#endif
	sz += fwrite( WIDE("WALL"), 1, 4, pFile );
	WriteSize( wall );
	wallsize += fwrite( &nwalls, 1, sizeof(nwalls), pFile );
	for( cnt = 0; cnt < nwalls; cnt++ )
	{
		FILEWALLV2 WriteWall;
		PWALL pwall = wallarray[cnt];
		WriteWall.flags = pwall->flags;

		tmp = WriteWall.nSector = FindInArray( (POINTER*)sectorarray, nsectors, GetUsedSetMember( SECTOR, &pWorld->sectors, pwall->iSector ) );
		if( tmp < 0 )
			Log1( WIDE("Failed to find referenced sector... save will fail %d"), cnt );

		tmp = WriteWall.nLine = FindInArray( (POINTER*)linearray, nlines, GetUsedSetMember( FLATLAND_MYLINESEG, &pWorld->lines, pwall->iLine ) );
		if( tmp < 0 )
			Log1( WIDE("Failed to find referenced line... save will fail %d"), cnt );

		if( pwall->iWallInto != INVALID_INDEX )
		{
			tmp = WriteWall.nWallInto = FindInArray( (POINTER*)wallarray, nwalls, GetUsedSetMember( WALL, &pWorld->walls, pwall->iWallInto ) );
			if( tmp < 0 )
				Log1( WIDE("Failed to find referenced wall into... save will fail %d"), cnt );
		}
		else
			WriteWall.nWallInto = -1;

		tmp = WriteWall.nWallStart = FindInArray( (POINTER*)wallarray, nwalls, GetUsedSetMember( WALL, &pWorld->walls, pwall->iWallStart ) );
		if( tmp < 0 )
			Log1( WIDE("Failed to find referenced starting wall... save will fail %d"), cnt );

		tmp = WriteWall.nWallEnd = FindInArray( (POINTER*)wallarray, nwalls, GetUsedSetMember( WALL, &pWorld->walls, pwall->iWallEnd ) );
		if( tmp < 0 )
			Log1( WIDE("Failed to find referenced ending wall... save will fail %d"), cnt );

		wallsize += fwrite( &WriteWall, 1, sizeof( WriteWall ), pFile );	
	}
	UpdateSize( wall );
#ifdef LOG_SAVETIMING
	Log1( WIDE("Wrote Walls: %d"), GetTickCount() - start );
	start = GetTickCount();
#endif
	sz += fwrite( WIDE("SECT"), 1, 4, pFile );
	WriteSize( sector );
	sectorsize += fwrite( &nsectors, 1, sizeof( nsectors ), pFile );
	for( cnt = 0; cnt < nsectors; cnt++ )
	{
		FILESECTORV8 WriteSector;
		WriteSector.nName = FindInArray( (POINTER*)namearray, nnames, GetUsedSetMember( NAME, &pWorld->names, sectorarray[cnt]->iName ) );
		//WriteSector.nID = sectorarray[cnt]->nID;
		WriteSector.flags = sectorarray[cnt]->flags;
		SetRay( &WriteSector.r, &sectorarray[cnt]->r );
		tmp = WriteSector.nwall = FindInArray( (POINTER*)wallarray, nwalls, GetUsedSetMember( WALL, &pWorld->walls, sectorarray[cnt]->iWall ) );
		if( tmp < 0 )
			Log1( WIDE("Failed to find wall referenced by sector.. save failing %d"), cnt );
		WriteSector.nTexture = FindInArray( (POINTER*)texturearray, ntextures, GetUsedSetMember( FLATLAND_TEXTURE, &pWorld->textures, sectorarray[cnt]->iTexture ) );
		if( tmp < 0 )
			Log1( WIDE("Failed to find texture referenced by sector.. save failing %d"), cnt );
		sectorsize += fwrite( &WriteSector, 1, sizeof( WriteSector ), pFile );
	}
	UpdateSize( sector );
#ifdef LOG_SAVETIMING
	Log1( WIDE("Wrote Sectors: %d"), GetTickCount() - start );
	start = GetTickCount();
#endif
	sz += fwrite( WIDE("NAME"), 1, 4, pFile );
	WriteSize( name );
	namesize += fwrite( &nnames, 1, sizeof( nnames ), pFile );
	for( cnt = 0; cnt < nnames; cnt++ )
	{
		int l;
		uint16_t lines = namearray[cnt]->lines;
		namesize += fwrite( &namearray[cnt]->flags, 1, sizeof( namearray[cnt]->flags ), pFile );
		namesize += fwrite( &lines, 1, sizeof( lines ), pFile );
		for( l = 0; l < lines; l++ )
		{
			namesize += fwrite( &namearray[cnt]->name[l].length, 1, sizeof( namearray[cnt]->name[l].length ), pFile );
			namesize += fwrite( namearray[cnt]->name[l].name, 1, namearray[cnt]->name[l].length, pFile );
		}
	}
	UpdateSize( name );
#ifdef LOG_SAVETIMING
	Log1( WIDE("Wrote Names: %d"), GetTickCount() - start );
	start = GetTickCount();
#endif
	sz += fwrite( WIDE("TEXT"), 1, 4, pFile );

	WriteSize( texture );

	texturesize += fwrite( &ntextures, 1, sizeof( ntextures ), pFile );
	for( cnt = 0; cnt < ntextures; cnt++ )
	{
		char flag;
		int nName = FindInArray( (POINTER*)namearray, nnames, GetUsedSetMember( NAME, &pWorld->names, texturearray[cnt]->iName ) );
		texturesize += fwrite( &nName, 1, sizeof( nName ), pFile );
		flag = texturearray[cnt]->flags.bColor;
		texturesize += fwrite( &flag, 1, sizeof( flag ), pFile );
		// if flag is set - write cdata 
		if( flag )
		{
			texturesize += fwrite( &texturearray[cnt]->data.color, 1, sizeof( CDATA ), pFile );
		}				
	}

	UpdateSize( texture );

#ifdef LOG_SAVETIMING
	Log1( WIDE("Wrote Textures: %d"), GetTickCount() - start );
	start = GetTickCount();
#endif
	{
		int endpos;
		endpos = ftell( pFile );
		fseek( pFile, worldpos, SEEK_SET );
		fwrite( &sz, 1, sizeof( sz ), pFile );
		fseek( pFile, endpos, SEEK_SET );
	}

#ifdef LOG_SAVETIMING
	Log1( WIDE("Wrote File: %d"), GetTickCount() - begin );
#endif

	Release( texturearray );
	Release( namearray );
	Release( sectorarray );
	Release( wallarray );
	Release( linearray );
#ifdef LOG_SAVETIMING
	Log1( WIDE("Released arrays: %d"), GetTickCount() - start );
#endif

	return sz;
}
#endif
//----------------------------------------------------------------------------

#define LOAD_PORTED
#ifdef LOAD_PORTED
int LoadData( PODBC odbc, INDEX world_id, INDEX iWorld )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	int sz = 0, cnt, version, size;
	char tag[4]; // read in make sure we're still aligned in the file...
	PFLATLAND_MYLINESEG *linearray;
	int nwalls;
	PWALL *wallarray;
	int nsectors;
	PSECTOR *sectorarray;
	int nnames;
	PNAME *namearray;
	int texturesize;
	int ntextures;
	PFLATLAND_TEXTURE *texturearray;
	CTEXTSTR *results;	
	CTEXTSTR result;

	INDEX texture;

	if( !world )
		return -1;
	ResetWorld( iWorld ); // make sure this thing is empty );

	// select the version in case there's more/better data..
	//Log1( WIDE("Loading version: %d"), version );
	// CTEXTSTR result;
	if( !SQLQueryf( odbc, &result, WIDE("select version from worlds where world_id=%d"), world_id ) )
		return -1;

	version = atoi( result );

	if( !SQLQueryf( odbc, &result, WIDE("select count(*) from world_lines where world_id=%d"), world_id ) || !result )
		return -1;

	linearray = NewArray( PFLATLAND_MYLINESEG, atoi( result ) );


	for( SQLRecordQueryf( odbc, NULL, &results, NULL, WIDE("select world_line_id,start,end,r_o_0,r_o_1,r_o_2,r_n_0,r_n_1,r_n_2 from world_lines where world_id=%d order by world_line_id"), world_id );
		results;
		FetchSQLRecord( odbc, &results ) )
	{
		int n;
		PFLATLAND_MYLINESEG pls;
		pls = GetFromSet( FLATLAND_MYLINESEG, &world->lines );
		linearray[cnt=atoi(results[0])-1] = pls;
		for( n = 0; n < 3; n++ )
			pls->r.o[n] = atof( results[3+n] );
		for( n = 0; n < 3; n++ )
			pls->r.n[n] = atof( results[6+n] );
		pls->dFrom = atof( results[1] );
		pls->dTo = atof( results[2] );
		MarkLineUpdated( iWorld, cnt );
	}

	if( !SQLQueryf( odbc, &result, WIDE("select count(*) from world_walls where world_id=%d"), world_id ) || !result )
		return -1;
	nwalls = atoi( result );
	wallarray = NewArray( PWALL, nwalls );

	for( cnt = 0; cnt < nwalls; cnt++ )
	{
		wallarray[cnt] = GetFromSet( WALL, &world->walls );
	}


	if( !SQLQueryf( odbc, &result, WIDE("select count(*) from world_sectors where world_id=%d"), world_id ) || !result )
		return -1;
	nsectors = atoi( result );
	sectorarray = NewArray( PSECTOR, nsectors );

	for( cnt = 0; cnt < nsectors; cnt++ )
	{
		sectorarray[cnt] = GetFromSet( SECTOR, &world->sectors );
	}

	if( !SQLQueryf( odbc, &result, WIDE("select count(*) from world_textures where world_id=%d"), world_id ) || !result )
		return -1;
	ntextures = atoi( result );
	texturearray = NewArray( PFLATLAND_TEXTURE, ntextures );

	for( cnt = 0; cnt < ntextures; cnt++ )
	{
		texturearray[cnt] = GetFromSet( FLATLAND_TEXTURE, &world->textures );
	}

	if( !SQLQueryf( odbc, &result, WIDE("select count(*) from world_names where world_id=%d"), world_id ) || !result )
		return -1;
	nnames = atoi( result );
	namearray = NewArray( PNAME, nnames );

	for( cnt = 0; cnt < ntextures; cnt++ )
	{
		texturearray[cnt] = GetFromSet( FLATLAND_TEXTURE, &world->textures );
	}

	for( SQLRecordQueryf( odbc, NULL, &results, NULL, WIDE("select world_wall_id,world_wall_id_start,world_wall_id_end,world_sector_id,world_line_id,flag_wall_end,flag_wall_start,world_wall_id_into from world_walls where world_id=%d order by world_wall_id"), world_id );
		results;
		FetchSQLRecord( odbc, &results ) )
	{
		PWALL pwall;
		cnt = atoi(results[0])-1;
		pwall = wallarray[cnt];

		pwall->iWallInto = atoi( results[7] ) - 1;
		pwall->iWallStart = atoi( results[1] ) - 1;
		pwall->iWallEnd = atoi( results[2] ) - 1;
		pwall->iSector = atoi( results[3] ) - 1;
		// set line increments reference counts on the line
#define SetLine( isetthis, pline ) { 				\
	if( isetthis != INVALID_INDEX )      			\
	{                              				\
      PFLATLAND_MYLINESEG psetthis = GetSetMember( FLATLAND_MYLINESEG, &world->lines, isetthis ); \
			if( !(--(psetthis)->refcount))   		\
				ServerDestroyLine( isetthis, iWorld ); \
		}                              				\
		if( pline != INVALID_INDEX )              \
		{                                         \
      PFLATLAND_MYLINESEG psetthis = GetSetMember( FLATLAND_MYLINESEG, &world->lines, pline ); \
			(isetthis) = (pline);                  \
			(psetthis)->refcount++;                \
		}                                         \
	}
      		SetLine( pwall->iLine, atoi( results[4] ) - 1 );
		pwall->iWorld = iWorld;
		if( results[5] )
			pwall->flags.wall_end_end = atoi( results[5] );
		if( results[6] )
			pwall->flags.wall_start_end = atoi( results[6] );
		MarkWallUpdated( iWorld, cnt );
	}	

	for( SQLRecordQueryf( odbc, NULL, &results, NULL, WIDE("select world_sector_id,world_texture_id,flags,world_name_id,world_wall_id from world_sectors where world_id=%d order by world_sector_id"), world_id );
		results;
		FetchSQLRecord( odbc, &results ) )
	{
		INDEX iSector;
		PSECTOR sector = sectorarray[iSector=(atoi( results[0] )-1)];
		sector->iTexture = atoi( results[1] )-1;
		//(*((int*)&sector->flags = atoi( results[2] );
		sector->iName = atoi( results[3] )-1;
		sector->iWall = atoi( results[4] )-1;
		sector->iWorld = iWorld;
		// walls should all be valid at this point...
		// have valid lines, and valid linkings...
		//ComputeSectorPointList( iWorld, iSector, NULL );
		ComputeSectorOrigin( iWorld, iSector );
		MarkSectorUpdated( iWorld, iSector );
	}

	
	for( SQLRecordQueryf( odbc, NULL, &results, NULL, WIDE("select world_name_id,text,flags from world_names where world_id=%d order by world_name_id"), world_id );
		results;
		FetchSQLRecord( odbc, &results ) )
	{
		INDEX iName = atoi( results[0] ) - 1;
		PNAME name = namearray[iName];
	}		

	for( SQLRecordQueryf( odbc, NULL, &results, NULL, WIDE("select world_texture_id,is_color,color,world_name_id from world_textures where world_id=%d order by world_texture_id"), world_id );
		results;
		FetchSQLRecord( odbc, &results ) )
	{
		INDEX iTexture = atoi( results[0] ) - 1;
		PFLATLAND_TEXTURE texture = texturearray[iTexture];
		texture->flags.bColor = atoi( results[1] );
		texture->data.color = atoi( results[2] );
		texture->iName = atoi( results[3] ) - 1;
	}		

	Release( texturearray );
	Release( namearray );
	Release( linearray );
	Release( wallarray );
	Release( sectorarray );
	ValidateWorldLinks(iWorld);
	return version;
}
#endif


int LoadWorld( INDEX iWorld )
{
	PODBC odbc = ConnectToDatabase( WIDE("worlds.db") );

	if( odbc )
		
	{
		PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
		TEXTCHAR buffer[256];
		InitWorldsDb();
		// should make sure reset tells client about deletions too.
		GetNameText( iWorld, world->name, buffer, sizeof( buffer ) );
		{
 			INDEX world_id = SQLReadNameTable( odbc, buffer, WIDE("worlds"), WIDE("world_id") );
			
			LoadData( odbc, world_id, iWorld );

		}
	    CloseDatabase( odbc );
		return 1;
	}
	return 0;
}

//----------------------------------------------------------------------------

typedef struct CollisionFind_tag {
	RAY r;
	INDEX walls[2];
	int nwalls;
	INDEX iWorld;
} WALLSELECTINFO, *PWALLSELECTINFO;

//----------------------------------------------------------------------------

INDEX CPROC CheckWallSelect( PWALL wall, PWALLSELECTINFO si )
{
	GETWORLD(si->iWorld);
	//PWALL wall = GetWall( iWall );
	PFLATLAND_MYLINESEG line = GetLine( wall->iLine );
	RCOORD t1, t2;
	if( FindIntersectionTime( &t1, si->r.n, si->r.o
						         , &t2, line->r.n, line->r.o ) )
	{
		if( t1 >= 0 && t1 <= 1 &&
		    t2 >= line->dFrom && t2 <= line->dTo )
		{
			if( si->nwalls < 2 )
			{
				si->walls[si->nwalls++] = GetMemberIndex( WALL, &world->walls, wall );
			}
			else
				return 1; // breaks for_all loop
		}
		//Log4( WIDE("Results: %g %g (%g %g)"), t1, t2, line->start, line->dTo );
	}
	return 0;
}

//----------------------------------------------------------------------------

int MergeSelectedWalls( INDEX iWorld, INDEX iDefinite, PORTHOAREA rect )
{
	GETWORLD(iWorld );
   //PWALL pDefinite = GetWall( iDefinite );
	WALLSELECTINFO si;
   // normal 1
	si.r.n[0] = rect->w;
	si.r.n[1] = rect->h;
	si.r.n[2] = 0;
	si.r.o[0] = rect->x;
	si.r.o[1] = rect->y;
	si.r.o[2] = 0;
	si.iWorld = iWorld;
	si.nwalls = 0;
	
	if( !ForAllWalls( world->walls, CheckWallSelect, &si ) )
	{
		Log1( WIDE("Found %d walls to merge: %d"), si.nwalls );
		if( si.nwalls == 2 && 
			( GetWall( si.walls[0] )->iSector != GetWall( si.walls[1] )->iSector ) )
		{
			MergeWalls( iWorld, si.walls[0], si.walls[1] );
		}
		if( si.nwalls == 1 &&
			 iDefinite != INVALID_INDEX &&
		    si.walls[0] != iDefinite )
		{
			MergeWalls( iWorld, iDefinite, si.walls[0] );
		}
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------

typedef struct sectorselectinfo_tag {
   INDEX iWorld;
	PORTHOAREA rect;
	int nsectors;
	INDEX *ppsectors;
} SECTORSELECTINFO, *PSECTORSELECTINFO;

//----------------------------------------------------------------------------

INDEX CPROC CheckSectorInRect( INDEX sector, PSECTORSELECTINFO psi )
{
	PORTHOAREA rect = psi->rect;
	INDEX pStart, pCur;
	int priorend = TRUE;
	_POINT p;
   //GETWORLD( psi->iWorld );
	pCur = pStart = GetFirstWall( psi->iWorld, sector, &priorend );
	do
	{
		PFLATLAND_MYLINESEG line;
		GetLineData( psi->iWorld, GetWallLine( psi->iWorld, pCur ), &line );
		if( priorend )
		{
			addscaled( p, line->r.o, line->r.n, line->dTo );
		}
		else
		{
			addscaled( p, line->r.o, line->r.n, line->dFrom );
		}
		//Log7( WIDE("Checking (%g,%g) vs (%g,%g)-(%g,%g)"), 
		if( p[0] < (rect->x) ||
		    p[0] > (rect->x + rect->w) ||
          p[1] < (rect->y) ||
		    p[1] > (rect->y + rect->h) )
		{
			pCur = INVALID_INDEX;
			break;
		}
		pCur = GetNextWall( psi->iWorld
								, pCur, &priorend );
	}while( pCur != pStart );
	if( pCur == pStart )
	{
		if( psi->ppsectors )
			psi->ppsectors[psi->nsectors++] = sector;
		else
			psi->nsectors++;
	}
	return 0;
}

//----------------------------------------------------------------------------

int MarkSelectedSectors( INDEX iWorld, PORTHOAREA rect, INDEX **sectorarray, int *sectorcount )
{
	GETWORLD( iWorld );
	SECTORSELECTINFO si;
	si.rect = rect;
	si.nsectors = 0;		
	si.ppsectors = NULL;
	si.iWorld = iWorld;
	if( rect->w < 0 )
	{
		rect->x += rect->w;
		rect->w = -rect->w;
	}
	if( rect->h < 0 )
	{
		rect->y += rect->h;
		rect->h = -rect->h;
	}
	Log( WIDE("Marking Sectors") );
	DoForAllSectors( world->sectors, CheckSectorInRect, (uintptr_t)&si );
	if( si.nsectors )
	{
		Log1( WIDE("Found %d sectors in range"), si.nsectors );
		if( sectorcount )
			*sectorcount = si.nsectors;
		if( sectorarray )
		{
			*sectorarray = (INDEX*)Allocate( sizeof( INDEX ) * si.nsectors );
			si.ppsectors = *sectorarray;
			si.nsectors = 0;
			DoForAllSectors( world->sectors, CheckSectorInRect, (uintptr_t)&si );
		}
		return TRUE;
	}
	else
	{
		if( sectorcount )
			*sectorcount = si.nsectors;
		if( sectorarray )
			*sectorarray = NULL;
	}
	return FALSE;
}

//----------------------------------------------------------------------------

typedef struct groupwallselectinfo_tag {
	INDEX iWorld;
	PORTHOAREA rect;
	int nwalls;
	INDEX *ppwalls;
} GROUPWALLSELECTINFO, *PGROUPWALLSELECTINFO;

//----------------------------------------------------------------------------

int PointInRect( P_POINT point, PORTHOAREA rect )
{
	if( point[0] < (rect->x) ||
		 point[0] > (rect->x + rect->w) ||
	    point[1] < (rect->y) ||
	    point[1] > (rect->y + rect->h) )
		return FALSE;
	return TRUE;
}

//----------------------------------------------------------------------------

int CPROC CheckWallInRect( PWALL wall, PGROUPWALLSELECTINFO psi )
{
	GETWORLD(psi->iWorld);
	_POINT p1, p2;
	PORTHOAREA rect = psi->rect; // shorter pointer
	if( wall->iLine == INVALID_INDEX )
	{
		PSECTOR sector = GetSector( wall->iSector );
   		Log( WIDE("Line didn't exist...") );
   		if( sector )
		{
			PNAME name = GetName( sector->iName );
   			Log( WIDE("Sector exists...") );
   			if( name &&
				name[0].name )
		   		Log1( WIDE("Wall in Sector %s does not have a line"), name[0].name );
			else
				Log( WIDE("Sector referenced does not have a name") );
		}
		else
			Log( WIDE("Wall should not be active... WHY is it?!") );
   }
   else
   {
		PFLATLAND_MYLINESEG line = GetLine( wall->iLine );
		addscaled( p1, line->r.o, line->r.n, line->dFrom );
		addscaled( p2, line->r.o, line->r.n, line->dTo );
		if( !PointInRect( p1, rect) ||
	   	 !PointInRect( p2, rect) )
		{
			return 0;
		}
   		else
		{
			if( psi->ppwalls )
			{
				psi->ppwalls[psi->nwalls++] = GetWallIndex( wall );
				lprintf( WIDE("Server side ") _WIDE(TARGETNAME) WIDE(" needs balance line.") );
				//BalanceALine( psi->iWorld, wall->iLine );
			}
			else
				psi->nwalls++;
		}
	}
	return 0; // abort immediate...
}

//----------------------------------------------------------------------------

void MergeOverlappingWalls( INDEX iWorld, PORTHOAREA rect )
{
	// for all walls - find a wall without a mate in the rect area...
	// then for all remaining walls - find another wall that is the 
	// same line as this one....
	GETWORLD(iWorld);

	int nwalls, n;
	PWALL *wallarray;
	wallarray = GetLinearWallArray( world->walls, &nwalls );
	for( n = 0; n < nwalls; n++ )
	{
		_POINT start, end;
		PWALL wall = wallarray[n];
		if( wall->iWallInto == INVALID_INDEX )
		{
			PFLATLAND_MYLINESEG line = GetLine( wall->iLine );
			addscaled( start, line->r.o, line->r.n, line->dFrom );
			addscaled( end, line->r.o, line->r.n, line->dTo );
			if( PointInRect( start, rect ) &&
			    PointInRect( end, rect ) )
			{
				int m;
				for( m = n+1; m < nwalls; m++ )
				{
					PWALL wall2 = wallarray[m];
					if( wall2->iWallInto == INVALID_INDEX )
					{
						_POINT start2, end2;
						PFLATLAND_MYLINESEG line = GetLine( wall2->iLine );
						addscaled( start2, line->r.o, line->r.n, line->dFrom );
						addscaled( end2, line->r.o, line->r.n, line->dTo );
						/*
						if( PointInRect( start2, rect ) && 
							 PointInRect( end2, rect ) )
						{
							Log4( WIDE("starts: (%12.12g,%12.12g) vs (%12.12g,%12.12g)") 
										,start[0], start[1]
										,start2[0], start2[1] );
							Log4( WIDE("ends  : (%12.12g,%12.12g) vs (%12.12g,%12.12g)") 
										,end[0], end[1]
										,end2[0], end2[1] );
						}
						*/
						if( ( Near( start2, start ) &&
						      Near( end2, end ) ) 
						  ||( Near( start2, end ) &&
						      Near( end2, start ) ) )
						{
							MergeWalls( iWorld, GetWallIndex( wall ), GetWallIndex( wall2 ) );
							break;
						}

					}
				}
			}
		}	
	}
	Release( wallarray );
}

//----------------------------------------------------------------------------

int MarkSelectedWalls( INDEX iWorld, PORTHOAREA rect, INDEX **wallarray, int *wallcount )
{
	GETWORLD( iWorld );
	GROUPWALLSELECTINFO si;
	si.iWorld = iWorld;
	si.rect = rect;
	si.nwalls = 0;
	si.ppwalls = NULL;

	ForAllWalls( world->walls, CheckWallInRect, &si );
	if( si.nwalls )
	{
		if( wallcount )
			*wallcount = si.nwalls;
		if( wallarray )
		{
			*wallarray = (INDEX*)Allocate( sizeof( INDEX ) * si.nwalls );
			si.ppwalls = *wallarray;
			si.nwalls = 0;
			ForAllWalls( world->walls, CheckWallInRect, &si );
		}
		return TRUE;
	}
	else
	{
		if( wallcount )
			*wallcount = si.nwalls;
		if( wallarray )
			*wallarray = NULL;
	}
	return FALSE;
}

//----------------------------------------------------------------------------

int ValidateWorldLinks( INDEX iWorld )
{
	GETWORLD(iWorld);
	int status = TRUE;
	int nLines;
	PFLATLAND_MYLINESEG *pLines;
	int nWalls;
	PWALL *pWalls;
	int nSectors;
	PSECTOR *pSectors;
	int nNames;
	PNAME *pNames;
	pSectors = GetLinearSectorArray( world->sectors, &nSectors );
	pWalls = GetLinearWallArray( world->walls, &nWalls );
	pLines = GetLinearLineArray( world->lines, &nLines );
	pNames = GetLinearNameArray( world->names, &nNames );
	{
   	int n, m, refcount;
		for( n = 0; n < nLines; n++ )
		{
			refcount = 0;
			for( m = 0; m < nWalls; m++ )
			{
				if( GetLine( pWalls[m]->iLine ) == pLines[n] )
				{
					if( pWalls[m]->iWallInto != INVALID_INDEX )
					{
						refcount++;
						if( refcount == 2 )
						{
							//pLines[n] = NULL;
							break;
						}
					}
				}
				else
				{
					//pLines[n] = NULL;
					break;
				}
			}
			if( m == nWalls )
			{
				status = FALSE;
				Log1( WIDE("Line %08x is unreferenced... deleting now."), pLines[n] );
				DeleteLine( world->lines, pLines[n] );
				pLines[n] = NULL;
			}
		}
		for( n = 0; n < nWalls; n++ )
		{
			if( !pWalls[n] )
				continue;
			for( m = 0; m < nLines; m++ )
			{
				if( GetLine( pWalls[n]->iLine ) == pLines[m] )
				{
					// if this line is shared - remove the other reference to it.
					/*
					if( pWalls[n]->wall_into )
					{
						int i;
						for( i = 0; i < nWalls; i++ )
						{
							if( pWalls[i] == pWalls[n]->wall_into )
								pWalls[i] = NULL;
						}
					}
					pLines[m] = NULL; // clear line reference...
					*/
					break;
				}
			}
			if( m == nLines )
			{
				status = FALSE;
				Log3( WIDE("Wall %08x in Sector %d referenced line %08x that does not exist"), 
							pWalls[n], GetSector( pWalls[n]->iSector )->iName, pWalls[n]->iLine );
			}
		}
		for( n = 0; n < nLines; n++ )
		{
			int count = 0;
			if( !pLines[n]->refcount )
				lprintf( WIDE("Line  %d exists with no reference count"), n );
			for( m = 0; m < nWalls; m++ )
			{
				if( GetLine( pWalls[m]->iLine ) == pLines[n] )
				{
					count++;
				}
			}
			if( count != pLines[n]->refcount )
			{
				Log2( WIDE("Line reference count of %d does not match actual %d")
							, pLines[n]->refcount
							, count );
			}
		}
		for( n = 0; n < nSectors; n++ )
		{
			PWALL pStart, pCur;
			int priorend = TRUE;
			if( pSectors[n]->iName != INVALID_INDEX )
			{
				int i;
				for( i = 0; i < nNames; i++ )
				{
					if( GetName( pSectors[n]->iName ) == pNames[i] )
						break;
				}
				if( i == nNames )
				{
					Log2( WIDE("Name %08x referenced by Sector %d does not exist"), pSectors[n]->iName, n );
				}
			}

			pCur = pStart = GetWall( pSectors[n]->iWall );
			do
			{
				if( pCur->iLine == INVALID_INDEX )
				{
					Log1( WIDE("Wall in sector %d has an invalid line def"), pSectors[n]->iName );
				}
				
				for( m = 0; m < nWalls; m++ )
				{
					if( pWalls[m] == pCur )
						break;
				}
				if( m == nWalls )
				{
					status = FALSE;
					Log4( WIDE("Sector %*.*s referenced wall %08x that does not exist"),
								GetName( pSectors[n]->iName )->name[0].length,
								GetName( pSectors[n]->iName )->name[0].length,
								GetName( pSectors[n]->iName )->name[0].name, pCur );
				}
				if( pCur->iWallStart == INVALID_INDEX )
				{
					lprintf( WIDE("wall has no start : %d"), GetMemberIndex( WALL, world->walls, pCur ) );
				}
				if( pCur->iWallEnd == INVALID_INDEX )
				{
					lprintf( WIDE("wall has no end : %d"), GetMemberIndex( WALL, world->walls, pCur ) );
				}
				// code goes here....
				if( priorend )
				{
					priorend = pCur->flags.wall_start_end;
					pCur = GetWall( pCur->iWallStart );
				}
				else
				{
					priorend = pCur->flags.wall_end_end;
					pCur = GetWall( pCur->iWallEnd );
				}
			}while( pCur != pStart );
		}
	}
	Release( pLines );
	Release( pWalls );
	Release( pSectors );
	Release( pNames );
	return status;
}

//----------------------------------------------------------------------------

// this really needs to use ForAllSectors ... 
// but then the callback needs to return a value to be able
// to stop that looping...

INDEX FindSectorAroundPoint( INDEX iWorld, P_POINT p )
{
	GETWORLD( iWorld );
	struct {
		INDEX iWorld;
		P_POINT p;
	} data;
	data.iWorld = iWorld;
	data.p = p;
	// hmm no spacetree now?
	//return FindPointInSpace( world->spacetree, p, PointWithinSingle );
	{
		INDEX FlatlandPointWithinLoopSingle( INDEX iSector, uintptr_t psv );

		return DoForAllSectors( world->sectors, FlatlandPointWithinLoopSingle, &data ) - 1;
	}
}

//----------------------------------------------------------------------------

