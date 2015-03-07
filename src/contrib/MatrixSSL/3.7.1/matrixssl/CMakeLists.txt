
set( MatrixSSL_Sources
		prf.c
		sslv3.c
		tls.c
		psk.c
		cipherSuite.c
		hsHash.c
		matrixssl.c
		matrixsslApi.c
		sslDecode.c
sslEncode.c )

add_library( MatrixSSL SHARED ${MatrixSSL_Sources} pscrypto core )

