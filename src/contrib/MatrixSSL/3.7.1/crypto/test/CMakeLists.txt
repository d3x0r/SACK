
add_executable( matrixssl_test_vector algorithmTest.c )
target_link_libraries( matrixssl_test_vector mssl_crypto mssl_core )
install( TARGETS matrixssl_test_vector DESTINATION bin )

add_executable( matrixssl_test_throughput throughputTest.c )
target_link_libraries( matrixssl_test_throughput mssl_crypto mssl_core )
install( TARGETS matrixssl_test_throughput DESTINATION bin )

add_executable( matrixssl_test_rsaperf rsaperf/rsaperf.c )
target_link_libraries( matrixssl_test_rsaperf mssl_crypto mssl_core )
install( TARGETS matrixssl_test_rsaperf DESTINATION bin )

add_executable( matrixssl_test_dhperf dhperf/dhperf.c )
target_link_libraries( matrixssl_test_dhperf mssl_crypto mssl_core )
install( TARGETS matrixssl_test_dhperf DESTINATION bin )

add_executable( matrixssl_test_eccperf eccperf/eccperf.c )
target_link_libraries( matrixssl_test_eccperf mssl_crypto mssl_core )
install( TARGETS matrixssl_test_eccperf DESTINATION bin )
