	
set( MatrixSSL_Sources
		cipherSuite.c
                extDecode.c
                hsDecode.c
		hsHash.c
		matrixssl.c
		matrixsslApi.c
		prf.c
		psk.c
		sslDecode.c
		sslEncode.c 
		sslv3.c
		tls.c
		dtls.c
)

ADD_DEFINITIONS( -DMATRIX_SSL_SOURCE )

add_library( MatrixSSL ${MatrixSSL_Sources} )

target_link_libraries( MatrixSSL mssl_crypto mssl_core )

SET_TARGET_PROPERTIES( MatrixSSL PROPERTIES
                  COMPILE_FLAGS  "${SYS_C_FLAGS}"
                  COMPILE_DEFINITIONS "${LIBFLAGS}"
                  FOLDER "Matrix SSL" 
                  )

include( GNUInstallDirs )
if( WIN32 )
	install( TARGETS MatrixSSL RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} )
else()
	install( TARGETS MatrixSSL LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} )
endif()
