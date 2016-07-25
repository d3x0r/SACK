

if( UNIX )
	set( MORE_SOURCES memset_s.c )
else()
	#make sure to clear it; it's a common symbol name.
	set( MORE_SOURCES )
endif( UNIX )

ADD_LIBRARY( mssl_core corelib.c ${OSDEP}/osdep.c ${MORE_SOURCES} )

SET_TARGET_PROPERTIES( mssl_core PROPERTIES
                  COMPILE_FLAGS  "${SYS_C_FLAGS}"
                  COMPILE_DEFINITIONS "${LIBFLAGS}"
                  FOLDER "Matrix SSL/Core" 
                  )

