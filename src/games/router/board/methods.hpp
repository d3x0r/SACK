#include "board.hpp"

#define OnPeiceCreate(name)  \
	__DefineRegistryMethodP(DEFAULT_PRELOAD_PRIORITY,WIDE("automaton"),OnPieceCreate,WIDE("board"),name WIDE("/OnCreate"),WIDE("f"),PTRSZVAL,(PTRSZVAL,PLAYER_DATA), __LINE__)

#define OnPeiceDestroy(name)  \
	__DefineRegistryMethodP(DEFAULT_PRELOAD_PRIORITY,WIDE("automaton"),OnPeiceDestroy,WIDE("board"),name WIDE("/OnDestroy"),WIDE("f"),void,(PTRSZVAL), __LINE__)

#define OnPeiceTap(name)  \
	__DefineRegistryMethodP(DEFAULT_PRELOAD_PRIORITY,WIDE("automaton"),OnPeiceTap,WIDE("board"),name WIDE("/OnMouseTap"),WIDE("f"),int,(PTRSZVAL,S_32,S_32), __LINE__)

#define OnPeiceClick(name)  \
	__DefineRegistryMethodP(DEFAULT_PRELOAD_PRIORITY,WIDE("automaton"),OnPeiceClick,WIDE("board"),name WIDE("/OnMouseDown"),WIDE("f"),int,(PTRSZVAL,S_32,S_32), __LINE__)

#define OnPeiceBeginDrag(name)  \
	__DefineRegistryMethodP(DEFAULT_PRELOAD_PRIORITY,WIDE("automaton"),OnPeiceBeginDrag,WIDE("board"),name WIDE("/OnMouseBeginDrag"),WIDE("f"),int,(PTRSZVAL,S_32,S_32), __LINE__)

#define OnPeiceProperty( name ) \
	__DefineRegistryMethodP(DEFAULT_PRELOAD_PRIORITY,WIDE("automaton"),OnPeiceProperty,WIDE("board"),name WIDE("/Properties"),WIDE("f"),void,(PTRSZVAL,PSI_CONTROL), __LINE__)

#define OnPeiceBeginConnect( name ) \
	__DefineRegistryMethodP(DEFAULT_PRELOAD_PRIORITY,WIDE("automaton"),OnPeiceBeginConnect,WIDE("board"),name WIDE("/OnBeginConnect"),WIDE("f"),int,(PTRSZVAL,S_32,S_32,PIPEICE,PTRSZVAL), __LINE__)

#define OnPeiceEndConnect( name ) \
	__DefineRegistryMethodP(DEFAULT_PRELOAD_PRIORITY,WIDE("automaton"),OnPeiceEndConnect,WIDE("board"),name WIDE("/OnEndConnect"),WIDE("f"),int,(PTRSZVAL,S_32,S_32,PIPEICE,PTRSZVAL), __LINE__)

#define OnPeiceExtraDraw( name ) \
   __DefineRegistryMethodP(DEFAULT_PRELOAD_PRIORITY,WIDE("automaton"),OnPeiceExtraDraw,WIDE("board"),name WIDE("/OnEndConnect"),WIDE("f"),void,( PTRSZVAL psv, Image surface, S_32 x, S_32 y, _32 w, _32 h ), __LINE__)

#define OnPeiceDraw( name ) \
	__DefineRegistryMethodP(DEFAULT_PRELOAD_PRIORITY,WIDE("automaton"),OnPeiceDraw,WIDE("board"),name WIDE("/OnDraw"),WIDE("f"),void,( PTRSZVAL psv, Image surface, Image peice, S_32 x, S_32 y ), __LINE__)

//   ( PTRSZVAL psv, Image surface, S_32 x, S_32 y, _32 w, _32 h )
#define BeginConnectFrom( name ) gasga
#define BeginConnectTo(name) hgha
#define EndConnectFrom(name) asdfd
#define EndConnectTo(name) hhha
