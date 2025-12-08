#ifndef PANHTHERS_C_PREPROCESSOR_INTERFACE_DEFINED
#	define PANHTHERS_C_PREPROCESSOR_INTERFACE_DEFINED

#ifndef PUBLIC 
#  error Have to include SACK runtime headers first...
#endif

#ifdef CPP_MAIN_SOURCE
#  define INTERFACE_METHOD EXPORT_METHOD
#else
#  define INTERFACE_METHOD IMPORT_METHOD
#endif


#ifdef __cplusplus
namespace d3x0r {
namespace ppc {
#endif

enum LocationType {
	LOCATION_TYPE_UNIFORM = 0,
	LOCATION_TYPE_IN,
	LOCATION_TYPE_OUT
};

typedef struct location_binding_tag {
	// type of this location
	enum LocationType type;
	// the binding/location number assigned
	int number;
	// type of this variable
	char *varType;
	// name of this variable
	char name[ 64 ];
	// the file this binding/ location was defined in
	char const *file;
	// a unique identifier passed with the file name to process
	uintptr_t shaderInfo;
} *PLOCATION_BINDING;

// this can be used to set some internal options that I'm too lazy
// to expose a proper API to set.
INTERFACE_METHOD int processArguments( int argc, char **argv );

// Process a file, appending the output to pvtOut.
// The state after processing includes all files that were included,
// symbols that were defined, and counters, so another file can
// be processed in sequence with the original.
INTERFACE_METHOD void
ProcessFile( char *file, PVARTEXT pvtOutput, uintptr_t processInfo );

// Resets counters, and defined symbols to be current information and
// start to process a new series of files.
INTERFACE_METHOD void ResetProcessor( void );

// This cleans up everything that it is able to clean.
// the preprocessor should not be used after calling this.
INTERFACE_METHOD void CloseProcessor( void );

//--------------- custom interface -----------------

INTERFACE_METHOD PLIST GetBindings( void );
INTERFACE_METHOD PLIST GetLocations( void );

INTERFACE_METHOD int GetFirstBinding( void );
INTERFACE_METHOD int GetFirstLocation( void );

INTERFACE_METHOD void
DefineExternalDefine( char *name,
                      char *args,
                      void ( *cb )( uintptr_t psvShaderInfo, PLIST pArgs, PTEXT *pOutput ) );

#ifdef __cplusplus
	}
}
#endif

#undef INTERFACE_METHOD

#endif