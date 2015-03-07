

set( crypto_sources 
		prng/prng.c
		prng/yarrow.c
		symmetric/aes.c
		symmetric/aesGCM.c
		symmetric/des3.c
		symmetric/seed.c
		symmetric/arc4.c
		symmetric/rc2.c
		symmetric/idea.c
		digest/sha1.c
		digest/sha256.c
		digest/sha224.c
		digest/sha384.c
		digest/sha512.c
		digest/md5.c
		digest/md4.c
		digest/md2.c
		digest/hmac.c
		hardware/aesni.c
		math/pstm.c
		math/pstm_montgomery_reduce.c
		math/pstm_sqr_comba.c
		math/pstm_mul_comba.c
		pubkey/pubkey.c
		pubkey/rsa.c
		pubkey/ecc.c
		pubkey/dh.c
		pubkey/pkcs.c
		keyformat/asn1.c
		keyformat/base64.c
		keyformat/x509.c 
		)

ADD_LIBRARY( mssl_crypto ${crypto_sources} )


source_group( keyformat FILES 
		keyformat/asn1.c
		keyformat/base64.c
		keyformat/x509.c 
                )


source_group( prng FILES 
		prng/prng.c
		prng/yarrow.c
                )
source_group( symmetric FILES 
		symmetric/aes.c
		symmetric/aesGCM.c
		symmetric/des3.c
		symmetric/seed.c
		symmetric/arc4.c
		symmetric/rc2.c
		symmetric/idea.c
                )
source_group( digest FILES
		digest/sha1.c
		digest/sha256.c
		digest/sha224.c
		digest/sha384.c
		digest/sha512.c
		digest/md5.c
		digest/md4.c
		digest/md2.c
		digest/hmac.c
                )
source_group( hardware FILES 
		hardware/aesni.c
                )
source_group( math FILES 
		math/pstm.c
		math/pstm_montgomery_reduce.c
		math/pstm_sqr_comba.c
		math/pstm_mul_comba.c
                )
source_group( pubkey FILES 
		pubkey/pubkey.c
		pubkey/rsa.c
		pubkey/ecc.c
		pubkey/dh.c
		pubkey/pkcs.c
                )




SET_TARGET_PROPERTIES( mssl_crypto PROPERTIES
#                  COMPILE_FLAGS  "-D"
                  COMPILE_DEFINITIONS "${LIBFLAGS}"
                  FOLDER "Matrix SSL/Crypto" 
                  )

