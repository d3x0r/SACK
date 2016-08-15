#define WORLD_SOURCE
#define USE_WORLDSCAPE_INTERFACE
#define WORLDSCAPE_INTERFACE_USED
#define DEFINE_DEFAULT_IMAGE_INTERFACE


#include <stdhdrs.h> // debugbreak
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sharemem.h>
#include <logging.h>

#include "world.h"
#include "lines.h"
#include "sector.h"
#include "names.h"
#include "texture.h"
#include "walls.h"

#include "service.h"

#include "global.h"
//#define LOG_SAVETIMING

extern GLOBAL g;
//----------------------------------------------------------------------------

INDEX SrvrCreateSquareSector( uint32_t client_id, INDEX iWorld, PC_POINT pOrigin, RCOORD size )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PSECTOR pSector;
	INDEX iSector;
	_POINT o, n;
	pSector = GetFromSet( SECTOR, &world->sectors );
#ifdef OUTPUT_TO_VIRTUALITY
	pSector->facet = AddNormalPlane( world->object, VectorConst_0, VectorConst_Z, 0 );
#endif
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
	pSector->iWall = SrvrCreateWall( client_id, iWorld, iSector, INVALID_INDEX, FALSE, INVALID_INDEX, FALSE, o, n );
	/* south */
	addscaled( o, pOrigin, VectorConst_Y, size/2 );
	scale( n, VectorConst_X, -20 );
	// creates a wall whos start is at the start of the wall...
	SrvrCreateWall( client_id, iWorld, iSector, pSector->iWall, TRUE, INVALID_INDEX, FALSE, o, n );
	/* west */
	addscaled( o, pOrigin, VectorConst_Y, -size/2 );
	scale( n, VectorConst_X, -20 );
	SrvrCreateWall( client_id, iWorld, iSector, pSector->iWall, FALSE, INVALID_INDEX, FALSE, o, n );
	/* north */
	addscaled( o, pOrigin, VectorConst_X, -size/2 );
	scale( n, VectorConst_Y, 20 );
	{
		PWALL pWall = GetSetMember( WALL, &world->walls, pSector->iWall );
		SrvrCreateWall( client_id, iWorld, iSector
					 , pWall->iWallStart, TRUE
					 , pWall->iWallEnd, TRUE, o, n );
	}
	//ComputeSectorPointList( iWorld, iSector, NULL );
	//ComputeSectorOrigin( iWorld, iSector );
#ifdef OUTPUT_TO_VIRTUALITY
	OrderObjectLines( world->object );
#endif
	SetPoint( pSector->r.n, VectorConst_Y );
	//DumpWall( pSector->wall );
	//DumpWall( pSector->wall->wall_at_start );
	//DumpWall( pSector->wall->wall_at_end );
	//DumpWall( pSector->wall->wall_at_start->wall_at_end );
	{
		INDEX texture = SrvrMakeTexture( client_id, iWorld, SrvrMakeName( client_id, iWorld, WIDE( "Default" ) ) );
		PFLATLAND_TEXTURE pTexture = GetSetMember( FLATLAND_TEXTURE, &world->textures, texture );
		if( !pTexture->flags.bColor )
			SrvrSetSolidColor( client_id, iWorld, texture, AColor( 170, 170, 170, 0x80 ) );
		SrvrSetTexture( client_id, iWorld, iSector, texture );
	}
#ifdef WORLDSCAPE_SERVER
	/* this doesn't exist in direct library */
	MarkSectorUpdated( client_id, iWorld, iSector );
#endif
	return iSector;
}

//----------------------------------------------------------------------------

uintptr_t CPROC CompareWorldName( POINTER p, uintptr_t psv )
{
	CTEXTSTR name = (CTEXTSTR)psv;
	PWORLD world = (PWORLD)p;
	TEXTCHAR buffer[256];
	GetNameText( GetMemberIndex( WORLD, &g.worlds, world ), world->name, buffer, sizeof( buffer ) );
	if( StrCmp( buffer, name ) == 0 )
		return (uintptr_t)world;
	return 0;
}

//----------------------------------------------------------------------------

INDEX SrvrOpenWorld( uint32_t client_id, CTEXTSTR name )
{
	PWORLD world;
	INDEX iWorld;
	uintptr_t psvResult;
	psvResult = ForAllInSet( WORLD, g.worlds, CompareWorldName, (uintptr_t)name );
	if( psvResult )
	{
		world = (PWORLD)psvResult;
		iWorld = GetMemberIndex( WORLD, &g.worlds, world );
#if defined( WORLD_SCAPE_SERVER_EXPORTS ) || defined( WORLDSCAPE_SERVICE )
		SrvrMarkWorldUpdated( client_id, iWorld );
#endif
	}
	else
	{
		world = GetFromSet( WORLD, &g.worlds );
		iWorld = GetMemberIndex( WORLD, &g.worlds, world );
#if defined( WORLD_SCAPE_SERVER_EXPORTS ) || defined( WORLDSCAPE_SERVICE )
		SrvrMarkWorldUpdated( client_id, iWorld );
#endif
		world->name = SrvrMakeName( client_id, iWorld, name );
		// get from set is zero initialized.
		//world->lines = NULL;
		//world->walls = NULL;
		//world->sectors = NULL;
		//world->textures = NULL;
		//world->names = NULL; // derr just made a name for this world itself... then do what?!
		//world->spacetree = NULL;
		SrvrCreateSquareSector( client_id, iWorld, VectorConst_0, 50 );
	}
	return iWorld;
}


//----------------------------------------------------------------------------

INDEX CreateBasicWorld( uint32_t client_id )
{
	INDEX world = SrvrOpenWorld( client_id, "Default world" );
	SrvrCreateSquareSector( client_id, world, VectorConst_0, 50 );
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
	DeleteSectors( iWorld );// deletes walls also...

	// should probably check that all walls are now now treferences...
	DeleteSet( (PGENERICSET*)&world->walls );
	// should probably check that all lines are now now treferences...
	DeleteSet( (PGENERICSET*)&world->lines );

	DeleteTextures( iWorld );

	DeleteNames( iWorld );

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

			#define SAVE_PORTED
#ifdef SAVE_PORTED
int SrvrSaveWorldToFile( FILE *pFile, INDEX iWorld )
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
	Log1( "Built arrays: %d", GetTickCount() - start );
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

	Log1( "Saving version %d", tmp );
	sz += fwrite( &tmp, 1, sizeof( tmp ), pFile );

	//----- write lines -------
	sz += fwrite( "LINE", 1, 4, pFile );
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
	Log1( "Wrote Lines: %d", GetTickCount() - start );
	start = GetTickCount();
#endif
	sz += fwrite( "WALL", 1, 4, pFile );
	WriteSize( wall );
	wallsize += fwrite( &nwalls, 1, sizeof(nwalls), pFile );
	for( cnt = 0; cnt < nwalls; cnt++ )
	{
		FILEWALLV2 WriteWall;
		PWALL pwall = wallarray[cnt];
		WriteWall.flags = pwall->flags;

		tmp = WriteWall.nSector = FindInArray( (POINTER*)sectorarray, nsectors, GetUsedSetMember( SECTOR, &pWorld->sectors, pwall->iSector ) );
		if( tmp < 0 )
			Log1( "Failed to find referenced sector... save will fail %d", cnt );

		tmp = WriteWall.nLine = FindInArray( (POINTER*)linearray, nlines, GetUsedSetMember( FLATLAND_MYLINESEG, &pWorld->lines, pwall->iLine ) );
		if( tmp < 0 )
			Log1( "Failed to find referenced line... save will fail %d", cnt );

		if( pwall->iWallInto != INVALID_INDEX )
		{
			tmp = WriteWall.nWallInto = FindInArray( (POINTER*)wallarray, nwalls, GetUsedSetMember( WALL, &pWorld->walls, pwall->iWallInto ) );
			if( tmp < 0 )
				Log1( "Failed to find referenced wall into... save will fail %d", cnt );
		}
		else
			WriteWall.nWallInto = -1;

		tmp = WriteWall.nWallStart = FindInArray( (POINTER*)wallarray, nwalls, GetUsedSetMember( WALL, &pWorld->walls, pwall->iWallStart ) );
		if( tmp < 0 )
			Log1( "Failed to find referenced starting wall... save will fail %d", cnt );

		tmp = WriteWall.nWallEnd = FindInArray( (POINTER*)wallarray, nwalls, GetUsedSetMember( WALL, &pWorld->walls, pwall->iWallEnd ) );
		if( tmp < 0 )
			Log1( "Failed to find referenced ending wall... save will fail %d", cnt );

		wallsize += fwrite( &WriteWall, 1, sizeof( WriteWall ), pFile );	
	}
	UpdateSize( wall );
#ifdef LOG_SAVETIMING
	Log1( "Wrote Walls: %d", GetTickCount() - start );
	start = GetTickCount();
#endif
	sz += fwrite( "SECT", 1, 4, pFile );
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
			Log1( "Failed to find wall referenced by sector.. save failing %d", cnt );
		WriteSector.nTexture = FindInArray( (POINTER*)texturearray, ntextures, GetUsedSetMember( FLATLAND_TEXTURE, &pWorld->textures, sectorarray[cnt]->iTexture ) );
		if( tmp < 0 )
			Log1( "Failed to find texture referenced by sector.. save failing %d", cnt );
		sectorsize += fwrite( &WriteSector, 1, sizeof( WriteSector ), pFile );
	}
	UpdateSize( sector );
#ifdef LOG_SAVETIMING
	Log1( "Wrote Sectors: %d", GetTickCount() - start );
	start = GetTickCount();
#endif
	sz += fwrite( "NAME", 1, 4, pFile );
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
	Log1( "Wrote Names: %d", GetTickCount() - start );
	start = GetTickCount();
#endif
	sz += fwrite( "TEXT", 1, 4, pFile );

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
	Log1( "Wrote Textures: %d", GetTickCount() - start );
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
	Log1( "Wrote File: %d", GetTickCount() - begin );
#endif

	Release( texturearray );
	Release( namearray );
	Release( sectorarray );
	Release( wallarray );
	Release( linearray );
#ifdef LOG_SAVETIMING
	Log1( "Released arrays: %d", GetTickCount() - start );
#endif

	return sz;
}
#endif
//----------------------------------------------------------------------------

#define LOAD_PORTED
#ifdef LOAD_PORTED
int SrvrLoadWorldFromFile( uint32_t client_id, FILE *file, INDEX iWorld )
{
	FILE *pFile;
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	int sz = 0, cnt, version, size;
	char tag[4]; // read in make sure we're still aligned in the file...
	int linesize;
	int nlines;
	PFLATLAND_MYLINESEG *linearray;
	int wallsize;
	int nwalls;
	PWALL *wallarray;
	int sectorsize;
	int nsectors;
	PSECTOR *sectorarray;
	int namesize;
	int nnames;
	PNAME *namearray;
	int texturesize;
	int ntextures;
	PFLATLAND_TEXTURE *texturearray;

	INDEX texture;

	if( !world )
		return 0;
	{
		char buffer[256];
      GetNameText( iWorld, world->name, buffer, sizeof( buffer ) );
		Fopen( pFile, buffer, "rb" );
	}
	if( !pFile )
      return 0;
	ResetWorld( iWorld ); // make sure this thing is empty );

	sz += fread( tag, 1, 4, pFile );
	if( strncmp( tag, "FLAT", 4 ) ) 
	{
		Log( "Alignment error in file load... totally invalid file." );
		return 0;
	}
	sz += fread( &size, 1, sizeof( size ), pFile );
	if( size < 10 ) // can assume that it was a version ID
		version = size;
	else
		sz = fread( &version, 1, sizeof( version ), pFile );

	if( version < 8 )
	{
		texture = SrvrMakeTexture( client_id, iWorld, SrvrMakeName( client_id, iWorld, WIDE( "Default" ) ) );
		SrvrSetSolidColor( client_id, iWorld, texture, AColor( 43, 76, 180, 0x80 ) );
	}
	Log1( "Loading version: %d", version );

	sz += fread( tag, 1, 4, pFile );
	if( strncmp( tag, "LINE", 4 ) )
	{
		// was not LINE or FLAT tag...
		Log( "Alignment error in file line load." );
		return 0;
	}

	if( version > 9 )
		sz += fread( &linesize, 1, sizeof( linesize ), pFile );

	sz += fread( &nlines, 1, sizeof(nlines), pFile );
	linearray = (PFLATLAND_MYLINESEG*)Allocate( sizeof( PFLATLAND_MYLINESEG ) * nlines );
	for( cnt = 0; cnt < nlines; cnt++ )
	{
		PFLATLAND_MYLINESEG pls;
		LINESEGFILE lsf;
		sz += fread( &lsf, 1, sizeof(LINESEGFILE), pFile );
		pls = GetFromSet( FLATLAND_MYLINESEG, &world->lines );
#ifdef OUTPUT_TO_VIRTUALITY
		pls->pfl = CreateFacetLine( world->object, lsf.r.o, lsf.r.n, lsf.start, lsf.end );
#endif
		linearray[cnt] = pls;
		/* some sort of update line... */
		SetRay( &pls->r, &lsf.r );
		if( version < 9 )
		{
			pls->r.o[1] = -pls->r.o[1];
			pls->r.n[1] = -pls->r.n[1];
		}
		pls->dFrom = lsf.start;
		pls->dTo = lsf.end;

	}

	sz += fread( tag, 1, 4, pFile );
	if( strncmp( tag, "WALL", 4 ) )
	{
		Log( "Alignment error in file wall load." );
		return 0;
	}
	if( version > 9 )
		sz += fread( &wallsize, 1, sizeof( wallsize ), pFile );

	sz += fread( &nwalls, 1, sizeof(nwalls), pFile );
	wallarray = (PWALL*)Allocate( sizeof( PWALL ) * nwalls );
	for( cnt = 0; cnt < nwalls; cnt++ )
	{
		wallarray[cnt] = GetFromSet( WALL, &world->walls );
	}
	for( cnt = 0; cnt < nwalls; cnt++ )
	{
		FILEWALLV1 ReadWall1;
		FILEWALLV2 ReadWall2;
		PWALL pwall;
		pwall = wallarray[cnt];
		if( version < 2 )
		{
			sz += fread( &ReadWall1, 1, sizeof( FILEWALLV1 ), pFile );
			pwall->flags = ReadWall1.flags;
			pwall->iSector = ReadWall1.nSector;

			SetLineWorld( world, pwall->iLine, linearray[ReadWall1.nLine] );

			if( ReadWall1.nWallInto == -1 )
				pwall->iWallInto = INVALID_INDEX;
			else
				pwall->iWallInto = GetMemberIndex( WALL, &world->walls, wallarray[ReadWall1.nWallInto] );
			pwall->iWallStart = GetMemberIndex( WALL, &world->walls, wallarray[ReadWall1.nWallStart] );
			pwall->iWallEnd = GetMemberIndex( WALL, &world->walls, wallarray[ReadWall1.nWallEnd] );
		}
		else 
		{
			sz += fread( &ReadWall2, 1, sizeof( FILEWALLV2 ), pFile );
			pwall->flags = ReadWall2.flags;
			pwall->iSector = ReadWall2.nSector;
			if( ReadWall2.nLine != -1 )
			{
				pwall->iLine = GetMemberIndex( FLATLAND_MYLINESEG, &world->lines, linearray[ReadWall2.nLine] );
				//SetLine( pwall->line, linearray[ReadWall2.nLine] );
			}
			else
			{
				Log( "Wall without line? cant be!" );
				DebugBreak();
				pwall->iLine = INVALID_INDEX;
			}
			if( ReadWall2.nWallInto == -1 )
				pwall->iWallInto = INVALID_INDEX;
			else
				pwall->iWallInto = GetMemberIndex( WALL, &world->walls, wallarray[ReadWall2.nWallInto] );
			pwall->iWallStart = GetMemberIndex( WALL, &world->walls, wallarray[ReadWall2.nWallStart] );
			pwall->iWallEnd = GetMemberIndex( WALL, &world->walls, wallarray[ReadWall2.nWallEnd] );
		}
	}	

	sz += fread( tag, 1, 4, pFile );
	if( strncmp( tag, "SECT", 4 ) )
	{
		Log2( "Alignment error in file sector load. %s(%d)", __FILE__, __LINE__ );
		return 0;
	}
	if( version > 9 )
		sz += fread( &sectorsize, 1, sizeof( sectorsize ), pFile );
	sz += fread( &nsectors, 1, sizeof(nsectors), pFile );
	sectorarray = (PSECTOR*)Allocate( sizeof( PSECTOR ) * nsectors );
	for( cnt = 0; cnt < nsectors; cnt++ )
	{
		FILESECTORV3 ReadSector3;
		FILESECTORV4 ReadSector4;
		FILESECTORV5 ReadSector5;
		FILESECTORV8 ReadSector8;
		INDEX sector;
		sectorarray[cnt] = GetFromSet( SECTOR, &world->sectors );
#ifdef OUTPUT_TO_VIRTUALITY
		sectorarray[cnt]->facet = AddNormalPlane( world->object, VectorConst_0, VectorConst_Z, 0 );
#endif
		sector =  GetMemberIndex( SECTOR, &world->sectors, sectorarray[cnt] );
		if( version < 8 )
		{
			// one texture is sufficient...
			// in fact textures should be highly sharable.
			sectorarray[cnt]->iTexture = texture;
		}
		if( version < 4 )
		{	
			/*
			{
			char name[20];
			sprintf( name, "%d", SectorIDs++ );
			sectorarray[cnt]->name = GetName( &world->names, name );
			}
			*/
			sectorarray[cnt]->iName = INVALID_INDEX;
			sz += fread( &ReadSector3, 1, sizeof( FILESECTORV3 ), pFile );
			sectorarray[cnt]->flags = ReadSector3.flags;
			sectorarray[cnt]->iWorld = iWorld;
			SetRay( &sectorarray[cnt]->r, &ReadSector3.r );
			sectorarray[cnt]->iWall = GetMemberIndex( WALL, &world->walls, wallarray[ReadSector3.nwall] );

			if( version < 3 )
			{
				ComputeSectorOrigin( iWorld, sector);
			}
		}
		else if( version < 5 )
		{
			sz += fread( &ReadSector4, 1, sizeof( FILESECTORV4 ), pFile );
			/*
			{
			char name[20];
			sprintf( name, "%d", ReadSector4.nID );
			sectorarray[cnt]->name = GetName( &world->names, name );
			if( atoi( name ) > SectorIDs )
			SectorIDs = atoi( name );
			}
			*/
			sectorarray[cnt]->iName = INVALID_INDEX;
			sectorarray[cnt]->flags = ReadSector4.flags;
			sectorarray[cnt]->iWorld = iWorld;
			SetRay( &sectorarray[cnt]->r, &ReadSector4.r );
			sectorarray[cnt]->iWall = GetMemberIndex( WALL, &world->walls, wallarray[ReadSector4.nwall] );
		}
		else if( version < 8 )
		{
			sz += fread( &ReadSector5, 1, sizeof( FILESECTORV5 ), pFile );
			sectorarray[cnt]->iName = ReadSector5.nName;
			sectorarray[cnt]->flags = ReadSector5.flags;
			sectorarray[cnt]->iWorld = iWorld;
			SetRay( &sectorarray[cnt]->r, &ReadSector5.r );
			sectorarray[cnt]->iWall = GetMemberIndex( WALL, &world->walls, wallarray[ReadSector5.nwall] );
		}
		else
		{
			sz += fread( &ReadSector8, 1, sizeof( FILESECTORV8 ), pFile );
			sectorarray[cnt]->iName = ReadSector8.nName;
			sectorarray[cnt]->flags = ReadSector8.flags;
			sectorarray[cnt]->iWorld = iWorld;
			SetRay( &sectorarray[cnt]->r, &ReadSector8.r );
			sectorarray[cnt]->iWall = ReadSector8.nwall;
			sectorarray[cnt]->iTexture = ReadSector8.nTexture;
		}
		// walls should all be valid at this point...
		// have valid lines, and valid linkings...
		ComputeSectorPointList( iWorld, sector, NULL );
		ComputeSectorOrigin( iWorld, sector );
#ifdef OUTPUT_TO_VIRTUALITY
		OrderObjectLines( world->object );
#endif
	}

	// fix sector references in walls.
	//for( cnt = 0; cnt < nwalls; cnt++ )
	//{
	//	wallarray[cnt]->iSector = wallarray[cnt]->iSector;
	//}

	for( cnt = 0; cnt < nwalls; cnt++ )
	{
		if( wallarray[cnt]->iLine == INVALID_INDEX )
		{
			SrvrDestroySector( client_id, iWorld, wallarray[cnt]->iSector );
			Log( "attempting to fix broken walls" );
			break;
		}
	}

	if( version >= 5 )
	{
		sz += fread( tag, 1, 4, pFile );
		if( strncmp( tag, "NAME", 4 ) )
		{
			Log2( "Alignment error in name section load. %s(%d)", __FILE__, __LINE__ );
			return 0;
		}
		if( version > 9 )
			sz += fread( &namesize, 1, sizeof( namesize ), pFile );
		sz += fread( &nnames, 1, sizeof(nnames), pFile );
		namearray = (PNAME*)Allocate( sizeof( PNAME ) * nnames );
		for( cnt = 0; cnt < nnames; cnt++ )
		{  	
			namearray[cnt] = GetFromSet( NAME, &world->names );
			if( version < 6 )
			{
				uint32_t length;
 				namearray[cnt]->name = (struct name_data*)Allocate( sizeof( *namearray[cnt]->name ) );
				sz += fread( &length, 1, sizeof( length ), pFile );
            namearray[cnt]->name[0].length = length;
				namearray[cnt]->name[0].name = (TEXTSTR)Allocate( namearray[cnt]->name[0].length + 1 );
 				sz += fread( namearray[cnt]->name[0].name, 1, namearray[cnt]->name[0].length, pFile );
		      namearray[cnt]->name[0].name[namearray[cnt]->name[0].length] = 0;
		      namearray[cnt]->lines = 1;
			}
			else if( version < 7 )
			{
				int l;
				sz += fread( &namearray[cnt]->lines, 1, sizeof( namearray[cnt]->lines ), pFile );
				if( namearray[cnt]->lines )
				{
					namearray[cnt]->name = (struct name_data*)Allocate( sizeof( *namearray[cnt]->name ) * namearray[cnt]->lines );
					for( l = 0; l < namearray[cnt]->lines; l++ )
					{
						sz += fread( &namearray[cnt]->name[l].length, 1, sizeof( namearray[cnt]->name[l].length ), pFile );
						namearray[cnt]->name[l].name = (TEXTSTR)Allocate( namearray[cnt]->name[l].length + 1 );
						sz += fread( namearray[cnt]->name[l].name, 1, namearray[cnt]->name[l].length, pFile );
						namearray[cnt]->name[l].name[namearray[cnt]->name[l].length] = 0;
					}	
				}
				else
					namearray[cnt]->name = NULL;
			}
			else
			{
				int l;
				sz += fread( &namearray[cnt]->flags, 1, sizeof( namearray[cnt]->flags ), pFile );
				sz += fread( &namearray[cnt]->lines, 1, sizeof( namearray[cnt]->lines ), pFile );
				if( namearray[cnt]->lines )
				{
					namearray[cnt]->name = (struct name_data*)Allocate( sizeof( *namearray[cnt]->name ) * namearray[cnt]->lines );
					for( l = 0; l < namearray[cnt]->lines; l++ )
					{
						sz += fread( &namearray[cnt]->name[l].length, 1, sizeof( namearray[cnt]->name[l].length ), pFile );
						namearray[cnt]->name[l].name = (TEXTSTR)Allocate( namearray[cnt]->name[l].length + 1 );
						sz += fread( namearray[cnt]->name[l].name, 1, namearray[cnt]->name[l].length, pFile );
						namearray[cnt]->name[l].name[namearray[cnt]->name[l].length] = 0;
					}	
				}
				else
					namearray[cnt]->name = NULL;
			}
		}
       /*
		for( cnt = 0; cnt < nsectors; cnt++ )
		{
			if( (int)sectorarray[cnt]->iName == -1 )
				sectorarray[cnt]->iName = INVALID_INDEX;
			else
			{
				int n;
				PNAME pName = namearray[(int)sectorarray[cnt]->name];
				if( pName->name && pName->name[0].name )
					if( ( n = atoi( pName->name[0].name ) ) > SectorIDs )
						SectorIDs = n;
				sectorarray[cnt]->name = pName;
			}
	   }
      */
		if( version >= 8 )
		{
			sz += fread( tag, 1, 4, pFile );
			if( strncmp( tag, "TEXT", 4 ) )
			{
				Log2( "Alignment error in texture section load. %s(%d)", __FILE__, __LINE__ );
				return 0;
			}
			if( version > 9 )
				sz += fread( &texturesize, 1, sizeof( texturesize ), pFile );
			sz += fread( &ntextures, 1, sizeof(ntextures), pFile );
			texturearray = (PFLATLAND_TEXTURE*)Allocate( sizeof( PFLATLAND_TEXTURE ) * ntextures );
			for( cnt = 0; cnt < ntextures; cnt++ )
			{
				char flag;
				int nName;
				sz += fread( &nName, 1, sizeof( nName ), pFile );
				texturearray[cnt] = GetFromSet( FLATLAND_TEXTURE, &world->textures );
				texturearray[cnt]->iName = GetMemberIndex( NAME, &world->names, namearray[nName] );
				sz += fread( &flag, 1, sizeof( flag ), pFile );
				texturearray[cnt]->flags.bColor = flag;
				if( flag )
				{
					sz += fread( &texturearray[cnt]->data.color, 1, sizeof(CDATA), pFile );
				}
			}
		   
			//for( cnt = 0; cnt < nsectors; cnt++ )
			//{
			//	int ntexture = (int)sectorarray[cnt]->iTexture;
         //   sectorarray[cnt]->texture = NULL;
			//	SetTexture( sectorarray[cnt]->texture, texturearray[ntexture] );
			//}
			Release( texturearray );
		}

		Release( namearray );
	}

	if( version > 9 )
	{
		if( sz != size )
		{
			Log2( "Total load size %d is not %d", sz, size );
		}
	}

	//for( cnt = 0; cnt < nsectors; cnt++ )
	{
		//PSECTOR pSector = sectorarray[cnt];
		// again with the min/max fetch with
		// spacetree add
		//pSector->spacenode = AddSpaceNode( &world->spacetree, pSector, min, max );
	}

	Release( linearray );
	Release( wallarray );
	Release( sectorarray );
	ValidateWorldLinks(iWorld);
	return version;
}
#endif
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
		//Log4( "Results: %g %g (%g %g)", t1, t2, line->start, line->dTo );
	}
	return 0;
}

//----------------------------------------------------------------------------

int SrvrMergeSelectedWalls( uint32_t client_id, INDEX iWorld, INDEX iDefinite, PORTHOAREA rect )
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
		Log1( "Found %d walls to merge: %d", si.nwalls );
		if( si.nwalls == 2 && 
			( GetWall( si.walls[0] )->iSector != GetWall( si.walls[1] )->iSector ) )
		{
			SrvrMergeWalls( client_id, iWorld, si.walls[0], si.walls[1] );
		}
		if( si.nwalls == 1 &&
			 iDefinite != INVALID_INDEX &&
		    si.walls[0] != iDefinite )
		{
			SrvrMergeWalls( client_id, iWorld, iDefinite, si.walls[0] );
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
		//Log7( "Checking (%g,%g) vs (%g,%g)-(%g,%g)", 
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
	Log( "Marking Sectors" );
	DoForAllSectors( world->sectors, CheckSectorInRect, (uintptr_t)&si );
	if( si.nsectors )
	{
		Log1( "Found %d sectors in range", si.nsectors );
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

void DumpBinary( unsigned char *pc, int sz )
{
	char msg[256];
	int n = 0;
	while( sz-- )
	{
		n += sprintf( msg + n, "%02x", *pc++ );
		if( ( sz & 3 ) == 0 )
			n += sprintf( msg + n, " " );
	}
	Log( msg );
}

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

int CPROC CheckWallInRect( uint32_t client_id, PWALL wall, PGROUPWALLSELECTINFO psi )
{
	GETWORLD(psi->iWorld);
	_POINT p1, p2;
	PORTHOAREA rect = psi->rect; // shorter pointer
	if( wall->iLine == INVALID_INDEX )
	{
		PSECTOR sector = GetSector( wall->iSector );
   		Log( "Line didn't exist..." );
   		if( sector )
		{
			PNAME name = GetName( sector->iName );
   			Log( "Sector exists..." );
   			if( name &&
				name[0].name )
		   		Log1( "Wall in Sector %s does not have a line", name[0].name );
			else
				Log( "Sector referenced does not have a name" );
		}
		else
			Log( "Wall should not be active... WHY is it?!" );
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
				SrvrBalanceALine( client_id, psi->iWorld, wall->iLine );
			}
			else
				psi->nwalls++;
		}
	}
	return 0; // abort immediate...
}

//----------------------------------------------------------------------------

void SrvrMergeOverlappingWalls( uint32_t client_id, INDEX iWorld, PORTHOAREA rect )
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
							Log4( "starts: (%12.12g,%12.12g) vs (%12.12g,%12.12g)" 
										,start[0], start[1]
										,start2[0], start2[1] );
							Log4( "ends  : (%12.12g,%12.12g) vs (%12.12g,%12.12g)" 
										,end[0], end[1]
										,end2[0], end2[1] );
						}
						*/
						if( ( Near( start2, start ) &&
						      Near( end2, end ) ) 
						  ||( Near( start2, end ) &&
						      Near( end2, start ) ) )
						{
							SrvrMergeWalls( client_id, iWorld, GetWallIndex( wall ), GetWallIndex( wall2 ) );
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
				Log1( "Line %08x is unreferenced... deleting now.", pLines[n] );
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
				Log3( "Wall %08x in Sector %d referenced line %08x that does not exist", 
							pWalls[n], GetSector( pWalls[n]->iSector )->iName, pWalls[n]->iLine );
			}
		}
		for( n = 0; n < nLines; n++ )
		{
			int count = 0;
			if( !pLines[n]->refcount )
				Log( "Line exists with no reference count" );
			for( m = 0; m < nWalls; m++ )
			{
				if( GetLine( pWalls[m]->iLine ) == pLines[n] )
				{
					count++;
				}
			}
			if( count != pLines[n]->refcount )
			{
				Log2( "Line reference count of %d does not match actual %d"
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
					Log2( "Name %08x referenced by Sector %d does not exist", pSectors[n]->iName, n );
				}
			}

			pCur = pStart = GetWall( pSectors[n]->iWall );
			do
			{
				if( pCur->iLine == INVALID_INDEX )
				{
					Log1( "Wall in sector %d has an invalid line def", pSectors[n]->iName );
				}
				
				for( m = 0; m < nWalls; m++ )
				{
					if( pWalls[m] == pCur )
						break;
				}
				if( m == nWalls )
				{
					status = FALSE;
					Log4( "Sector %*.*s referenced wall %08x that does not exist",
								GetName( pSectors[n]->iName )->name[0].length,
								GetName( pSectors[n]->iName )->name[0].length,
								GetName( pSectors[n]->iName )->name[0].name, pCur );
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
	return DoForAllSectors( world->sectors, FlatlandPointWithinLoopSingle, &data ) - 1;
}

//----------------------------------------------------------------------------
