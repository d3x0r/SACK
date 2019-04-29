#include "board.hpp"

#define OnPeiceCreate(name)  \
	DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY,"automaton",OnPieceCreate,"board/" name "/OnCreate","f","no-desc",uintptr_t,(uintptr_t,PLAYER_DATA), __LINE__)

#define OnPeiceDestroy(name)  \
	DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY,"automaton",OnPeiceDestroy,"board" name "/OnDestroy","f","no-desc",void,(uintptr_t), __LINE__)

#define OnPeiceTap(name)  \
	DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY,"automaton",OnPeiceTap,"board" name "/OnMouseTap","f","no-desc",int,(uintptr_t,int32_t,int32_t), __LINE__)

#define OnPeiceClick(name)  \
	DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY,"automaton",OnPeiceClick,"board" name "/OnMouseDown","f","no-desc",int,(uintptr_t,int32_t,int32_t), __LINE__)

#define OnPeiceBeginDrag(name)  \
	DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY,"automaton",OnPeiceBeginDrag,"board" name "/OnMouseBeginDrag","f","no-desc",int,(uintptr_t,int32_t,int32_t), __LINE__)

#define OnPeiceProperty( name ) \
	DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY,"automaton",OnPeiceProperty,"board" name "/Properties","f","no-desc",void,(uintptr_t,PSI_CONTROL), __LINE__)

#define OnPeiceBeginConnect( name ) \
	DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY,"automaton",OnPeiceBeginConnect,"board" name "/OnBeginConnect","f","no-desc",int,(uintptr_t,int32_t,int32_t,PIPEICE,uintptr_t), __LINE__)

#define OnPeiceEndConnect( name ) \
	DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY,"automaton",OnPeiceEndConnect,"board" name "/OnEndConnect","f","no-desc",int,(uintptr_t,int32_t,int32_t,PIPEICE,uintptr_t), __LINE__)

#define OnPeiceExtraDraw( name ) \
   DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY,"automaton",OnPeiceExtraDraw,"board" name "/OnEndConnect","f","no-desc",void,( uintptr_t psv, Image surface, int32_t x, int32_t y, _32 w, _32 h ), __LINE__)

#define OnPeiceDraw( name ) \
	DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY,"automaton",OnPeiceDraw,"board" name "/OnDraw","f","no-desc",void,( uintptr_t psv, Image surface, Image peice, int32_t x, int32_t y ), __LINE__)

//   ( uintptr_t psv, Image surface, int32_t x, int32_t y, _32 w, _32 h )
#define BeginConnectFrom( name ) gasga
#define BeginConnectTo(name) hgha
#define EndConnectFrom(name) asdfd
#define EndConnectTo(name) hhha
