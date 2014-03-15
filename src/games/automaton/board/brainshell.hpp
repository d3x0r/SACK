
#include "../brain/brain.hpp"
//#include "BRAINSHELL.hpp"
#include "board.hpp"

#ifdef BRAINSHELL_SOURCE 
//#define BRAINSHELL_PROC(type,name) __declspec(dllexport) type CPROC name
#define BRAINSHELL_PROC(type,name) EXPORT_METHOD type name
#define BRAINSHELL_EXTERN(type,name) EXPORT_METHOD type name
#else
//#define BRAINSHELL_PROC(type,name) __declspec(dllimport) type CPROC name
#define BRAINSHELL_PROC(type,name) IMPORT_METHOD type name
#define BRAINSHELL_EXTERN(type,name) IMPORT_METHOD type name
#endif
typedef class BRAINBOARD *PBRAINBOARD;
BRAINSHELL_PROC( class BRAINBOARD *, CreateBrainBoard )( PBRAIN brain );
BRAINSHELL_PROC( class IBOARD *, GetBoard )( PBRAINBOARD brain_board );