set(
	TLS_SRC
	tls.c
	tls_bio_cb.c
	tls_client.c
	tls_config.c
	tls_conninfo.c
	tls_keypair.c
	tls_server.c
	tls_ocsp.c
	tls_peer.c
	tls_util.c
	tls_verify.c
)

if(WIN32)
	set(
		TLS_SRC
		${TLS_SRC}
		compat/ftruncate.c
		compat/pread.c
		compat/pwrite.c
	)
endif()

if(NOT "${OPENSSLDIR}" STREQUAL "")
	add_definitions(-D_PATH_SSL_CA_FILE=\"${OPENSSLDIR}/cert.pem\")
else()
	add_definitions(-D_PATH_SSL_CA_FILE=\"${CMAKE_INSTALL_PREFIX}/etc/ssl/cert.pem\")
endif()


if( EXISTS ${PPCTOOL} )
  set( INCLUDES -I../ssl/. -I ../include/compat -I ../include )
  set( TLS_SRCS )
  foreach( SRC ${TLS_SRC} )
     set( TLS_SRCS ${TLS_SRCS} ../tls/${SRC})
  endforeach()
  set( ALL_SOURCES ${ALL_SOURCES} ${TLS_SRCS} PARENT_SCOPE)
  set( ALL_INCLUDES ${ALL_INCLUDES} ${INCLUDES} PARENT_SCOPE)

  set( AMALG_COMMAND ${PPCTOOL} -c -K -once -ssio -sd ${INCLUDES} -p -o${CMAKE_CURRENT_SOURCE_DIR}/tls_amalg.c ${TLS_SRC} )

  add_custom_command( OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/tls_amalg.c
   			COMMAND ${AMALG_COMMAND}
                   DEPENDS ${CRYPTO_SRC} always_rebuild
				   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
  add_custom_target( doittls DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tls_amalg.c )

endif( EXISTS ${PPCTOOL} )


add_library(tls ${TLS_SRC})
target_include_directories(tls
	PRIVATE
		.
		../include/compat
	PUBLIC
		../include)

if (BUILD_SHARED_LIBS)
	export_symbol(tls ${CMAKE_CURRENT_SOURCE_DIR}/tls.sym)
	target_link_libraries(tls ssl crypto ${PLATFORM_LIBS})
	if (WIN32)
		set(TLS_POSTFIX -${TLS_MAJOR_VERSION})
	endif()
	set_target_properties(tls PROPERTIES
		OUTPUT_NAME tls${TLS_POSTFIX}
		ARCHIVE_OUTPUT_NAME tls${TLS_POSTFIX})
	set_target_properties(tls PROPERTIES VERSION ${TLS_VERSION}
		SOVERSION ${TLS_MAJOR_VERSION})
endif()

if(ENABLE_LIBRESSL_INSTALL)
	install(
		TARGETS tls
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	)
endif(ENABLE_LIBRESSL_INSTALL)
