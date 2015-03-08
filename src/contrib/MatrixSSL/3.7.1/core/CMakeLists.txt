

ADD_LIBRARY( mssl_core corelib.c ${OSDEP}/osdep.c )
SET_TARGET_PROPERTIES( mssl_core PROPERTIES
                  COMPILE_FLAGS  "${SYS_C_FLAGS}"
                  COMPILE_DEFINITIONS "${LIBFLAGS}"
                  FOLDER "Matrix SSL/Core" 
                  )

