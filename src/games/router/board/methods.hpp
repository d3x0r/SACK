#include "board.hpp"

#define OnPeiceCreate(name)  \
	__DefineRegistryMethodP(DEFAULT_PRELOAD_PRIORITY,WIDE("automaton"),OnPieceCreate,WIDE("board"),name WIDE("/OnCreate"),WIDE("f"),uintptr_t,(uintptr_t,PLAYER_DATA), __LINE__)

#define OnPeiceDestroy(name)  \
	__DefineRegistryMethodP(DEFAULT_PRELOAD_PRIORITY,WIDE("automaton"),OnPeiceDestroy,WIDE("board"),name WIDE("/OnDestroy"),WIDE("f"),void,(uintptr_t), __LINE__)

#define OnPeiceTap(name)  \
	__DefineRegistryMethodP(DEFAULT_PRELOAD_PRIORITY,WIDE("automaton"),OnPeiceTap,WIDE("board"),name WIDE("/OnMouseTap"),WIDE("f"),int,(uintptr_t,int32_t,int32_t), __LINE__)

#define OnPeiceClick(name)  \
	__DefineRegistryMethodP(DEFAULT_PRELOAD_PRIORITY,WIDE("automaton"),OnPeiceClick,WIDE("board"),name WIDE("/OnMouseDown"),WIDE("f"),int,(uintptr_t,int32_t,int32_t), __LINE__)

#define OnPeiceBeginDrag(name)  \
	__DefineRegistryMethodP(DEFAULT_PRELOAD_PRIORITY,WIDE("automaton"),OnPeiceBeginDrag,WIDE("board"),name WIDE("/OnMouseBeginDrag"),WIDE("f"),int,(uintptr_t,int32_t,int32_t), __LINE__)

#define OnPeiceProperty( name ) \
	__DefineRegistryMethodP(DEFAULT_PRELOAD_PRIORITY,WIDE("automaton"),OnPeiceProperty,WIDE("board"),name WIDE("/Properties"),WIDE("f"),void,(uintptr_t,PSI_CONTROL), __LINE__)

#define OnPeiceBeginConnect( name ) \
	__DefineRegistryMethodP(DEFAULT_PRELOAD_PRIORITY,WIDE("automaton"),OnPeiceBeginConnect,WIDE("board"),name WIDE("/OnBeginConnect"),WIDE("f"),int,(uintptr_t,int32_t,int32_t,PIPEICE,uintptr_t), __LINE__)

#define OnPeiceEndConnect( name ) \
	__DefineRegistryMethodP(DEFAULT_PRELOAD_PRIORITY,WIDE("automaton"),OnPeiceEndConnect,WIDE("board"),name WIDE("/OnEndConnect"),WIDE("f"),int,(uintptr_t,int32_t,int32_t,PIPEICE,uintptr_t), __LINE__)

#define OnPeiceExtraDraw( name ) \
   __DefineRegistryMethodP(DEFAULT_PRELOAD_PRIORITY,WIDE("automaton"),OnPeiceExtraDraw,WIDE("board"),name WIDE("/OnEndConnect"),WIDE("f"),void,( uintptr_t psv, Image surface, int32_t x, int32_t y, _32 w, _32 h ), __LINE__)

#define OnPeiceDraw( name ) \
	__DefineRegistryMethodP(DEFAULT_PRELOAD_PRIORITY,WIDE("automaton"),OnPeiceDraw,WIDE("board"),name WIDE("/OnDraw"),WIDE("f"),void,( uintptr_t psv, Image surface, Image peice, int32_t x, int32_t y ), __LINE__)

//   ( uintptr_t psv, Image surface, int32_t x, int32_t y, _32 w, _32 h )
#define BeginConnectFrom( name ) gasga
#define BeginConnectTo(name) hgha
#define EndConnectFrom(name) asdfd
#define EndConnectTo(name) hhha
