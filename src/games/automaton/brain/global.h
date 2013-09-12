#ifndef GLOBAL_DEFINED
#define GLOBAL_DEFINED

#ifdef __ANDROID__
#define IMPORT
#else
#ifdef BRAIN_SOURCE
#define IMPORT EXPORT_METHOD
#else
#define IMPORT IMPORT_METHOD
#endif
#endif

//typedef class BRAIN_STEM *PBRAIN_STEM;
//typedef class BRAIN *PBRAIN;


#endif
