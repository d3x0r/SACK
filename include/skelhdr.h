//----------------------------------------------------------
// Please replace SKELETON with the symbol you wish to use
// for your library.



#ifdef SKELETON_LIBRARY_SOURCE
#define SKELETON_PROC(type,name) EXPORT_METHOD type name
#else
#define SKELETON_PROC(type,name) IMPORT_METHOD type name
#endif
#else



//----------------------------------------------------------
// $Log: $
//
