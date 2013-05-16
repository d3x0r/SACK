
// 4
#ifndef WORLD_SERVICE
WORLD_PROC( INDEX, OpenWorld )( CTEXTSTR world_name );
#else
WORLD_PROC( INDEX, OpenWorld )( _32 iClient, CTEXTSTR name );
#endif
WORLD_PROC( void, DestroyWorld )( INDEX world );
WORLD_PROC( void, ResetWorld )( INDEX pWorld ); // clears the world of all parts.

WORLD_PROC( _32, GetSectorCount )( INDEX iWorld );
WORLD_PROC( _32, GetWallCount )( INDEX iWorld );
WORLD_PROC( _32, GetLineCount )( INDEX iWorld );
// 10
WORLD_PROC( int, SaveWorldToFile )( INDEX iWorld );
WORLD_PROC( int, LoadWorldFromFile )( INDEX iWorld );

WORLD_PROC( int, MergeSelectedWalls )( INDEX iWorld, INDEX iDefiniteWall, PORTHOAREA rect );
WORLD_PROC( int, MarkSelectedSectors )( INDEX iWorld, PORTHOAREA rect, INDEX **sectorarray, int *sectorcount );
WORLD_PROC( int, MarkSelectedWalls )( INDEX iWorld, PORTHOAREA rect, INDEX **wallarray, int *wallcount );
WORLD_PROC( void, MergeOverlappingWalls )( INDEX iWorld, PORTHOAREA rect );

WORLD_PROC( int, ValidateWorldLinks )( INDEX iWorld );

//------ Sectors ----------
WORLD_PROC( INDEX, FindSectorAroundPoint )( INDEX iWorld, P_POINT p );
WORLD_PROC( INDEX, CreateSquareSector )( INDEX iWorld, PC_POINT pOrigin, RCOORD size );
WORLD_PROC( INDEX, AddConnectedSector )( INDEX iWorld, INDEX iWall, RCOORD offset );
//20
WORLD_PROC( int, MoveWalls )( INDEX iWorld, int nWalls, INDEX *WallList, P_POINT del, int bLockSlope );
// send current line, get all updates from world-scape server
WORLD_PROC( int, UpdateMatingLines )( INDEX iWorld, INDEX iWall, int bLockSlopes, int bErrorOK );
WORLD_PROC( int, WallInSector )( INDEX iWorld, INDEX iSector, INDEX iWall );

WORLD_PROC( void, GetSectorOrigin )( INDEX iWorld, INDEX sector, P_POINT o );
WORLD_PROC( int, LineInCur )( INDEX iWorld
									 , INDEX *SectorList, int nSectors
									 , INDEX *WallList, int nWalls
									 , INDEX pLine );
WORLD_PROC( INDEX, FindIntersectingWall )( INDEX iWorld, INDEX iSector, P_POINT n, P_POINT o );

WORLD_PROC( int, DestroySector )( INDEX iWorld, INDEX iSector );

WORLD_PROC( INDEX, FlatlandPointWithin )( INDEX iWorld, int nSectors, INDEX *piSectors, P_POINT p );
WORLD_PROC( INDEX, FlatlandPointWithinSingle )( INDEX world, INDEX iSector, P_POINT p );
WORLD_PROC( INDEX, FlatlandPointWithinLoopSingle )( INDEX iSector, PTRSZVAL psv );
//30
WORLD_PROC( void, ComputeSectorOrigin )( INDEX iWorld, INDEX iSector );
WORLD_PROC( void, ComputeSectorSetOrigin )( INDEX iWorld, int nSectors, INDEX *sectors, P_POINT origin );
WORLD_PROC( void, ComputeWallSetOrigin )( INDEX iWorld, int nWalls, INDEX *walls, P_POINT origin );

WORLD_PROC( int, MoveSectors )( INDEX iWorld
										, int nSectors
										, INDEX *pSectors, P_POINT del );


WORLD_PROC( INDEX, CreateWall )( INDEX world, INDEX iSector
										 , INDEX pStart, int bFromStartEnd
										 , INDEX pEnd,   int bFromEndEnd
										 , _POINT o, _POINT n );

WORLD_PROC( int, DestroyWall )( INDEX iWorld, INDEX iWall );
WORLD_PROC( int, MergeWalls )( INDEX iWorld, INDEX iCurWall, INDEX iMarkedWall );
WORLD_PROC( void, SplitWall )( INDEX iWorld, INDEX iWall );
WORLD_PROC( int, RemoveWall )( INDEX iWorld, INDEX iWall ); // remove wall, and relink mating...


WORLD_PROC( _POINT*, CheckPointOrder )( PC_POINT normal, _POINT *plist, int npoints );
// 40
WORLD_PROC( _POINT*, ComputeSectorPointList )( INDEX iWorld, INDEX sector, int *pnpoints );
WORLD_PROC( void, BreakWall )( INDEX iWorld, INDEX wall );

WORLD_PROC( void, GetNameText )( INDEX iWorld, INDEX name, TEXTCHAR *text, int maxlen );
WORLD_PROC( INDEX, GetWallLine )( INDEX iWorld, INDEX wall );
WORLD_PROC( INDEX, GetWallSector )( INDEX iWorld, INDEX iSector );
WORLD_PROC( INDEX, GetMatedWall )( INDEX iWorld, INDEX wall );
WORLD_PROC( INDEX, GetFirstWall )( INDEX iWorld, INDEX iSector, int *priorend );
WORLD_PROC( INDEX, GetNextWall )( INDEX iWorld, INDEX wall, int *priorend );
WORLD_PROC( INDEX, GetSectorName )( INDEX iWorld, INDEX iSector );

WORLD_PROC( void, GetLineData )( INDEX iWorld, INDEX iLine, PFLATLAND_MYLINESEG *line );
// 50
WORLD_PROC( void, BalanceALine )( INDEX iWorld, INDEX iWall, INDEX iLine, PLINESEG newseg, LOGICAL bLockSlope );
WORLD_PROC( void, GetNameData )( INDEX iWorld, INDEX iName, PNAME *name );
WORLD_PROC( INDEX, GetSectorTexture )( INDEX iWorld, INDEX iSector );
WORLD_PROC( void, GetTextureData )( INDEX iWorld, INDEX iTexture, PFLATLAND_TEXTURE *texture );
WORLD_PROC( void, GetSectorPoints )( INDEX iWorld, INDEX iSector, _POINT **list, int *npoints );

WORLD_PROC( void, ForAllSectors )( INDEX iWorld, FESMCallback,PTRSZVAL);

// internal only function use
// BalanceALine and pass world and line index
//WORLD_PROC( void, BalanceLine )( PFLATLAND_MYLINESEG pls );


WORLD_PROC( INDEX, MakeTexture )( INDEX iWorld, INDEX iName );
WORLD_PROC( void, SetSolidColor )( INDEX iWorld, INDEX iTexture, CDATA color );
WORLD_PROC( void, DeleteTexture )( INDEX iWorld, INDEX iTexture );
WORLD_PROC( void, DeleteTextures )( INDEX iWorld );
// 60
WORLD_PROC( INDEX, MakeName )( INDEX iWorld, CTEXTSTR text );
WORLD_PROC( void, SetName )( INDEX iWorld, INDEX iName, CTEXTSTR text );
WORLD_PROC( void, DeleteNames )( INDEX iWorld );
WORLD_PROC( void, DeleteName )( INDEX iWorld, INDEX iName );

WORLD_PROC( void, ClearUndo )( INDEX iWorld ); // 0x3f ?
WORLD_PROC( void, DoUndo )( INDEX iWorld );

WORLD_PROC( void, AddUndo )( INDEX iWorld, int type, ... );
// 67
WORLD_PROC( void, EndUndo )( INDEX iWorld, int type, ... );

WORLD_PROC( INDEX, ForAllTextures )( INDEX iWorld, INDEX (CPROC*)(INDEX,PTRSZVAL), PTRSZVAL );
WORLD_PROC( void, GetTextureNameText )( INDEX iWorld, INDEX iTexture, TEXTCHAR *buf, int sizeof_buf );
// 70
WORLD_PROC( INDEX, SetTexture )( INDEX iWorld, INDEX iSector, INDEX iTexture );

//WORLD_PROC( void, MarkTextureUpdated )( INDEX iWorld, INDEX iTexture );
//WORLD_PROC( void, MarkSectorUpdated )( INDEX iWorld, INDEX iSector );
//WORLD_PROC( void, MarkWorldUpdated )( INDEX iWorld );

WORLD_PROC( void, SetSectorName )( INDEX iWorld, INDEX iSector, INDEX iName );

//-------------------------------------------------------
/* intended for client application registration for update notifications */

#ifndef WORLD_SCAPE_INTERFACE_NAME
typedef void (CPROC *WorldScapeUdpateProc)( PTRSZVAL );
#endif
WORLD_PROC( void, AddUpdateCallback )( WorldScapeUdpateProc, PTRSZVAL psv );

//-------------------------------------------------------
// meant for client->server communication
WORLD_PROC( void, UpdateLine )( void ); 
WORLD_PROC( void, UpdateSector )( void ); 
WORLD_PROC( void, UpdateWall )( void ); 
WORLD_PROC( void, UpdateName )( void ); 
WORLD_PROC( void, UpdateTexture )( void ); 
WORLD_PROC( void, FlushUpdates )( void ); // meant for client->server communication


/* client application things... for sending request for change to server... */
WORLD_PROC( void, MarkLineChanged )( INDEX iWorld, INDEX iLine );
//80
WORLD_PROC( void, SendLinesChanged )( INDEX iWorld );
WORLD_PROC( void, SendLineNormalsChanged )( INDEX iWorld, INDEX iThing, MYLINESEG *lineseg );
WORLD_PROC( void, SendLineChanged )( INDEX iWorld, INDEX iWall, INDEX iThing, LINESEG *lineseg, LOGICAL no_mate_update, LOGICAL lock_mating_slopes );

// client only things...
WORLD_PROC( LOGICAL, IsSectorClockwise )( INDEX iWorld, INDEX iSector );

WORLD_PROC( void, AcceptChanges )( void );

/* must be last! */
WORLD_PROC( void, __LastServiceFunction )( void );
