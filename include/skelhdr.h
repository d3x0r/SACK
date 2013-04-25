//----------------------------------------------------------
// Please replace SKELETON with the symbol you wish to use
// for your library.



#if !defined(__STATIC__) && !defined(__UNIX__)
#ifdef SKELETON_LIBRARY_SOURCE
#define SKELETON_PROC(type,name) __declspec(dllexport) type name
#else
#define SKELETON_PROC(type,name) __declspec(dllimport) type name
#endif
#else
#ifdef SKELETON_LIBRARY_SOURCE
#define SKELETON_PROC(type,name) type name
#else
#define SKELETON_PROC(type,name) extern type name
#endif



//----------------------------------------------------------
// $Log: $
//
