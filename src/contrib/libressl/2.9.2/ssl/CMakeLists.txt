set(
	SSL_SRC
	bio_ssl.c
	bs_ber.c
	bs_cbb.c
	bs_cbs.c
	d1_both.c
	d1_clnt.c
	d1_enc.c
	d1_lib.c
	d1_pkt.c
	d1_srtp.c
	d1_srvr.c
	pqueue.c
	s3_cbc.c
	s3_lib.c
	ssl_algs.c
	ssl_asn1.c
	ssl_both.c
	ssl_cert.c
	ssl_ciph.c
	ssl_ciphers.c
	ssl_clnt.c
	ssl_err.c
	ssl_init.c
	ssl_lib.c
	ssl_methods.c
	ssl_packet.c
	ssl_pkt.c
	ssl_rsa.c
	ssl_sess.c
	ssl_sigalgs.c
	ssl_srvr.c
	ssl_stat.c
	ssl_tlsext.c
	ssl_transcript.c
	ssl_txt.c
	ssl_versions.c
	t1_enc.c
	t1_lib.c
	tls13_buffer.c
	tls13_client.c
	tls13_handshake.c
	tls13_handshake_msg.c
	tls13_key_schedule.c
	tls13_lib.c
	tls13_record.c
	tls13_record_layer.c
)

if( EXISTS ${PPCTOOL} )
  set( INCLUDES -I../ssl/. -I../include/compat -I../include )
  SET( ALL_INCLUDES ${ALL_INCLUDES} ${INCLUDES} PARENT_SCOPE )
  set( SSL_SRCS )
  foreach( SRC ${SSL_SRC} )
     LIST( APPEND SSL_SRCS ../ssl/${SRC})
  endforeach()
  set( ALL_SOURCES ${ALL_SOURCES} ${SSL_SRCS} PARENT_SCOPE)

  set( AMALG_COMMAND ${PPCTOOL} -c -K -once -ssio -sd ${INCLUDES} -p -o${CMAKE_CURRENT_SOURCE_DIR}/ssl.c ${SSL_SRC} )

  add_custom_command( OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/ssl.c
   			COMMAND ${AMALG_COMMAND}
   			COMMAND echo ${AMALG_COMMAND}
                   DEPENDS ${CRYPTO_SRC} always_rebuild
				   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
  add_custom_target( doitssl DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ssl.c )

endif( EXISTS ${PPCTOOL} )

add_library(ssl ${SSL_SRC})
target_include_directories(ssl
	PRIVATE
		.
		../include/compat
	PUBLIC
		../include)

if (BUILD_SHARED_LIBS)
	export_symbol(ssl ${CMAKE_CURRENT_SOURCE_DIR}/ssl.sym)
	target_link_libraries(ssl crypto ${PLATFORM_LIBS})
	if (WIN32)
		set(SSL_POSTFIX -${SSL_MAJOR_VERSION})
	endif()
	set_target_properties(ssl PROPERTIES
		OUTPUT_NAME ssl${SSL_POSTFIX}
		ARCHIVE_OUTPUT_NAME ssl${SSL_POSTFIX})
	set_target_properties(ssl PROPERTIES VERSION ${SSL_VERSION}
		SOVERSION ${SSL_MAJOR_VERSION})
endif()

if(ENABLE_LIBRESSL_INSTALL)
	install(
		TARGETS ssl
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	)
endif(ENABLE_LIBRESSL_INSTALL)
