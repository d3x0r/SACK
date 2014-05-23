#ifndef WORLD_DEFINED
#define WORLD_DEFINED

#include <sack_types.h>
#include <stdio.h>
#define NEED_VECTLIB_COMPARE

#include <vectlib.h>
#include <colordef.h>

//#if defined( WORLDSCAPE_INTERFACE_USED )
//# define WORLD_PROC INTERFACE_METHOD
//#else
# if !defined(__STATIC__) && !defined(__UNIX__)
#  ifdef WORLD_SOURCE
#   define WORLD_PROC(type,name) EXPORT_METHOD type CPROC name
#  else
#   define WORLD_PROC(type,name) IMPORT_METHOD type CPROC name
#  endif
# else
#  ifdef WORLD_SOURCE
#   define WORLD_PROC(type,name) type CPROC name
#  else
#   define WORLD_PROC(type,name) extern type CPROC name
#  endif
# endif
//#endif

#define CURRENTSAVEVERSION 10

// clients need this directly.
// there may be aliases within....
#include <worldstrucs.h>

//------------ UNDO/ROLLBACK -------------------------

enum {
	UNDO_SECTORMOVE
	,UNDO_WALLMOVE
	,UNDO_SLOPEMOVE
	,UNDO_STARTMOVE
	,UNDO_ENDMOVE
	,UNDO_SPLIT
	,UNDO_MERGE
	,UNDO_DELETESECTOR
	,UNDO_DELETEWALL
};
// UNDO_ENDMOVE - PWALL wall (save line on wall.)
// UNDO_STARTMOVE - PWALL wall (save line on wall)
// UNDO_SLIPEMOVE - PWALL wall (save line on wall)
// UNDO_WALLMOVE - int walls, PWALL *wall (save lines on walls)
// UNDO_SECTORMOVE(add) - int sectors, PSECTOR *sectors (save origins), P_POINT start
// UNDO_SECTORMOVE(end) - P_POINT end ...


#include <msgprotocol.h>

enum FLATLAND_SERVICE_EVENTS{
	// all others connected get this event message
   // the creator will get his own responce back.
	MSG_EVENT_CREATEWALL = MSG_EventUser
	  , MSG_EVENT_CREATESECTOR
	  , MSG_EVENT_CREATELINE
	  , MSG_EVENT_CREATENAME
	  , MSG_EVENT_CREATEWORLD
	  , MSG_EVENT_CREATETEXTURE

	  , MSG_EVENT_UPDATEWALL
	  , MSG_EVENT_UPDATESECTOR
	  , MSG_EVENT_UPDATELINE
	  , MSG_EVENT_UPDATENAME
	  , MSG_EVENT_UPDATETEXTURE
	  , MSG_WORLD_CREATE_UNLOCK
	  , MSG_WORLD_CREATE_LOCK

	  , MSG_EVENT_DELETEWALL
	  , MSG_EVENT_DELETESECTOR
	  , MSG_EVENT_DELETELINE
	  , MSG_EVENT_DELETENAME
	  , MSG_EVENT_DELETETEXTURE

	  , MSG_EVENT_CHANGES_DONE // sent when all changes in a world are done being posted to a client.
};
#ifndef WORLD_SERVICE
#include <world_proto.h>
#endif

#define WORLD_SCAPE_INTERFACE_NAME WIDE("World Scape")
#ifdef WORLDSCAPE_INTERFACE_USED
#undef WORLD_PROC
# define WORLD_PROC INTERFACE_METHOD

typedef struct world_scape_interface WORLD_SCAPE_INTERFACE, *PWORLD_SCAPE_INTERFACE;
struct world_scape_interface {
#include <world_proto.h>
};
#undef WORLD_PROC
#define WORLD_PROC(type,name) type name
#include <msgprotocol.h>

/*********************************************************/
/* if there is an interface variable defined to use..... */
#ifdef USE_WORLD_SCAPE_INTERFACE
#define  OpenWorld  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, OpenWorld  )
#define  DestroyWorld  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, DestroyWorld  )
#define  ResetWorld  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, ResetWorld  )

#define  GetSectors  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, GetSectors  )
#define  GetWalls  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, GetWalls  )
#define  GetLines  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, GetLines  )
#define  GetTextures  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, GetTextures  )
#define  GetSectorCount  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, GetSectorCount  )
#define  GetWallCount  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, GetWallCount  )
#define  GetLineCount  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, GetLineCount  )

#define  SaveWorldToFile  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, SaveWorldToFile  )
#define  LoadWorldFromFile  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, LoadWorldFromFile  )

#define  MergeSelectedWalls  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, MergeSelectedWalls  )
#define  MarkSelectedSectors  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, MarkSelectedSectors  )
#define  MarkSelectedWalls  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, MarkSelectedWalls  )
#define  MergeOverlappingWalls  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, MergeOverlappingWalls  )

#define  ValidateWorldLinks  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, ValidateWorldLinks  )

//------ Sectors ----------
#define  FindSectorAroundPoint  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, FindSectorAroundPoint  )

//#define  DeleteSectors  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, DeleteSectors  )

#define  CreateSquareSector  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, CreateSquareSector  )
#define  AddConnectedSector  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, AddConnectedSector  )
#define  MoveWalls  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, MoveWalls  )
// send current line, get all updates from world-scape server
#define  UpdateMatingLines  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, UpdateMatingLines  )
#define  WallInSector  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, WallInSector  )

#define  GetSectorOrigin  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, GetSectorOrigin  )
#define  LineInCur  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, LineInCur  )
#define  FindIntersectingWall  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, FindIntersectingWall  )

#define  DestroySector  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, DestroySector  )

#define  FlatlandPointWithin  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, FlatlandPointWithin  )
#define  FlatlandPointWithinSingle  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, FlatlandPointWithinSingle  )
#define  FlatlandPointWithinLoopSingle  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, FlatlandPointWithinLoopSingle  )

#define  ComputeSectorOrigin  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, ComputeSectorOrigin  )
#define  ComputeSectorSetOrigin  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, ComputeSectorSetOrigin  )
#define  ComputeWallSetOrigin  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, ComputeWallSetOrigin  )

#define  MoveSectors  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, MoveSectors  )


#define  CreateWall  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, CreateWall  )

#define  MergeWalls  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, MergeWalls  )
#define  SplitWall  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, SplitWall  )
#define  RemoveWall  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, RemoveWall  )


#define  CheckPointOrder  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, CheckPointOrder  )
#define  ComputeSectorPointList  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, ComputeSectorPointList  )
#define  BreakWall  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, BreakWall  )

#define  GetNameText  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, GetNameText  )
#define  GetWallLine  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, GetWallLine  )
#define  GetWallSector  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, GetWallSector  )
#define  GetMatedWall  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, GetMatedWall  )
#define  GetFirstWall  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, GetFirstWall  )
#define  GetNextWall  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, GetNextWall  )
#define  GetSectorName  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, GetSectorName  )
#define  SetSectorName  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, SetSectorName  )

#define  GetLineData  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, GetLineData  )
#define  BalanceALine  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, BalanceALine  )
#define  GetNameData  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, GetNameData  )
#define  GetSectorTexture  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, GetSectorTexture  )
#define  GetTextureData  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, GetTextureData  )
#define  GetSectorPoints  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, GetSectorPoints  )
//#define  GetSectorData  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, GetSectorData  )

#define  ForAllSectors  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, ForAllSectors  )

// internal only function use
// BalanceALine and pass world and line index
#define  BalanceLine  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, BalanceLine  )


#define  MakeTexture  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, MakeTexture  )
#define  SetSolidColor  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, SetSolidColor  )
#define  DeleteTexture  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, DeleteTexture  )
#define  DeleteTextures  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, DeleteTextures  )

#define  MakeName  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, MakeName  )
#define  SetName  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, SetName  )
#define  DeleteNames  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, DeleteNames  )
#define  DeleteName  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, DeleteName  )

#define  ClearUndo  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, ClearUndo  )
#define  DoUndo  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, DoUndo  )

#define  AddUndo  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, AddUndo  )
#define  EndUndo  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, EndUndo  )
#define  SendLineChanged  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, SendLineChanged  )
#define  SendLinesChanged  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, SendLinesChanged  )
#define  MarkLineChanged  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, MarkLineChanged  )


#define  ForAllTextures        METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, ForAllTextures  )
#define  GetTextureNameText    METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, GetTextureNameText  )
#define  SetTexture	           METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, SetTexture  )
#define  MarkTextureUpdated    METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, MarkTextureUpdated )
#define  MarkSectorUpdated     METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, MarkSectorUpdated )
#define  MarkWorldUpdated     METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, MarkWorldUpdated )

#define  AcceptChanges     METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, AcceptChanges )


#define AddUpdateCallback  METHOD_ALIAS( USE_WORLD_SCAPE_INTERFACE, AddUpdateCallback)

#endif // USE_WORLD_SCAPE_INTERFACE

#endif // WORLDSCAPE_INTERFACE_USED

#endif
