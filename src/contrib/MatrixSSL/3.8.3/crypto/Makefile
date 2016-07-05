#
#   Makefile for MatrixSSL crypto static library
#   'make' builds debug (Default).
#   'make gold' builds optimized.
#
#   Copyright (c) 2013-2016 INSIDE Secure Corporation. All Rights Reserved.
#

MATRIXSSL_ROOT:=..
include $(MATRIXSSL_ROOT)/common.mk

SRC:=\
	symmetric/aes.c \
	symmetric/aesCBC.c \
	symmetric/aesGCM.c \
	symmetric/aes_aesni.c \
	symmetric/arc4.c \
	symmetric/des3.c \
	symmetric/idea.c \
	symmetric/rc2.c \
	symmetric/seed.c \
	digest/sha1.c \
	digest/sha256.c \
	digest/sha512.c \
	digest/md5sha1.c \
	digest/md5.c \
	digest/hmac.c \
	digest/md4.c \
	digest/md2.c \
	keyformat/asn1.c \
	keyformat/base64.c \
	keyformat/pkcs.c \
	keyformat/x509.c \
	layer/matrix.c \
	math/pstm.c \
	math/pstm_montgomery_reduce.c \
	math/pstm_mul_comba.c \
	math/pstm_sqr_comba.c \
	prng/prng.c \
	prng/yarrow.c \
	pubkey/dh.c \
	pubkey/ecc.c \
	pubkey/pubkey.c \
	pubkey/rsa.c
#OPENSSL
ifdef PS_OPENSSL
SRC+=\
	digest/digest_openssl.c \
	symmetric/symmetric_openssl.c \
	pubkey/rsa_openssl.c
endif
#OPENSSL
#LIBSODIUM
ifdef PS_LIBSODIUM
SRC+=\
	symmetric/symmetric_libsodium.c \
	digest/digest_libsodium.c
endif
#LIBSODIUM

# Generated files
STATIC:=libcrypt_s.a

all: compile

compile: $(OBJS) $(STATIC)
	@echo $(STROPTS)

# Additional Dependencies
$(OBJS): $(MATRIXSSL_ROOT)/common.mk Makefile *.h */*.h

# Build the static library
# Direct stderr to null so we don't see the 'empty file' warnings
$(STATIC): $(OBJS)
	$(AR) -rcuv $@ $^ 2>/dev/null

clean:
	rm -f $(STATIC) $(OBJS)

# Allows to check configuration options.
parse-config:
	echo '#include "cryptoConfig.h"' | $(CC) $(CFLAGS) -dM -E -x c -
