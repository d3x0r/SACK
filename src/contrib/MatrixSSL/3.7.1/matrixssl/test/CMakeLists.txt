
if( LINK_TOKEN_BUILDER )
   set( EXTRA_SOURCES tstack.c )
endif( LINK_TOKEN_BUILDER )
   

INCLUDE_DIRECTORIES( .. ../.. ../../crypto )

add_executable( matrixssl_test_certValidate certValidate.c )
target_link_libraries( matrixssl_test_certValidate MatrixSSL )
install( TARGETS matrixssl_test_certValidate DESTINATION bin )

add_executable( matrixssl_test_sslTest sslTest.c )
target_link_libraries( matrixssl_test_sslTest MatrixSSL )
install( TARGETS matrixssl_test_sslTest DESTINATION bin )

