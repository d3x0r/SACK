

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


void ClearUndo( void );
void DoUndo( void );

void AddUndo( int type, ... );
void EndUndo( int type, ... );

// UNDO_ENDMOVE - PWALL wall (save line on wall.)
// UNDO_STARTMOVE - PWALL wall (save line on wall)
// UNDO_SLIPEMOVE - PWALL wall (save line on wall)
// UNDO_WALLMOVE - int walls, PWALL *wall (save lines on walls)
// UNDO_SECTORMOVE(add) - int sectors, PSECTOR *sectors (save origins), P_POINT start
// UNDO_SECTORMOVE(end) - P_POINT end ...
