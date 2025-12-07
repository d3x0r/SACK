
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

// this can be used to set some internal options that I'm too lazy
// to expose a proper API to set.
INTERFACE_METHOD int processArguments( int argc, char **argv );

// Process a file, appending the output to pvtOut.
// The state after processing includes all files that were included,
// symbols that were defined, and counters, so another file can
// be processed in sequence with the original.
INTERFACE_METHOD void ProcessFile( char *file, PVARTEXT pvtOutput );

// Resets counters, and defined symbols to be current information and 
// start to process a new series of files.
INTERFACE_METHOD void ResetProcessor( void );

// This cleans up everything that it is able to clean.
// the preprocessor should not be used after calling this.
INTERFACE_METHOD void CloseProcessor( void );

#ifdef __cplusplus
}
#endif

#undef INTERFACE_METHOD
