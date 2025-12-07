
#ifndef PUBLIC 
#  error Have to include SACK runtime headers first...
#endif

#ifdef CPP_MAIN_SOURCE
#  define INTERFACE_METHOD PUBLIC_METHOD
#else
#  define INTERFACE_METHOD REFERENCE_METHOD
#endif


#ifdef __cplusplus
extern "C" {
#endif


INTERFACE_METHOD void ProcessFile( char *file, PVARTEXT pvtOutput );

#ifdef __cplusplus
}
#endif

#undef INTERFACE_METHOD
