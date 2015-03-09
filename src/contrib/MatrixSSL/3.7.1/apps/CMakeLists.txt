
###### Server #####
add_executable( MatrixSSL_server server.c http.c XGetopt.c )

SET_TARGET_PROPERTIES( MatrixSSL_server PROPERTIES
                  # DEFINE_SYMBOL _USRDLL
                  COMPILE_FLAGS  "${SYS_C_FLAGS}"
                  COMPILE_DEFINITIONS "${LIBFLAGS}"
                  FOLDER "Matrix SSL" 
                  )

target_link_libraries( MatrixSSL_server MatrixSSL )
if( WIN32 )
    target_link_libraries( MatrixSSL_server wsock32 )
endif( WIN32 )

install( TARGETS MatrixSSL_server DESTINATION bin )


###### Client #####
add_executable( MatrixSSL_client client.c http.c XGetopt.c )

SET_TARGET_PROPERTIES( MatrixSSL_client PROPERTIES
                  # DEFINE_SYMBOL _USRDLL
                  COMPILE_FLAGS  "${SYS_C_FLAGS}"
                  COMPILE_DEFINITIONS "${LIBFLAGS}"
                  FOLDER "Matrix SSL" 
                  )

target_link_libraries( MatrixSSL_client MatrixSSL )
if( WIN32 )
    target_link_libraries( MatrixSSL_client wsock32 )
endif( WIN32 )

install( TARGETS MatrixSSL_client DESTINATION bin )
