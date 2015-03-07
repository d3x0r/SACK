#
#	Makefile for crypto dynamic and static library
#
#	Copyright (c) INSIDE Secure, 2013. All Rights Reserved.
#

include ../common.mk

LIBRARY	= libpscrypto$(SO)
STATIC	= libpscryptostatic$(A)

CORE_DYN = ../core/libcore$(SO)

OBJECTS	= \
		prng/prng$(O) \
		prng/yarrow$(O) \
		symmetric/aes$(O) \
		symmetric/aesGCM$(O) \
		symmetric/des3$(O) \
		symmetric/seed$(O) \
		symmetric/arc4$(O) \
		symmetric/rc2$(O) \
		symmetric/idea$(O) \
		digest/sha1$(O) \
		digest/sha256$(O) \
		digest/sha224$(O) \
		digest/sha384$(O) \
		digest/sha512$(O) \
		digest/md5$(O) \
		digest/md4$(O) \
		digest/md2$(O) \
		digest/hmac$(O) \
		hardware/aesni$(O) \
		math/pstm$(O) \
		math/pstm_montgomery_reduce$(O) \
		math/pstm_sqr_comba$(O) \
		math/pstm_mul_comba$(O) \
		pubkey/pubkey$(O) \
		pubkey/rsa$(O) \
		pubkey/ecc$(O) \
		pubkey/dh$(O) \
		pubkey/pkcs$(O) \
		keyformat/asn1$(O) \
		keyformat/base64$(O) \
		keyformat/x509$(O)

#
#	Compile options
#
CFLAGS += -I./ -I../

all: compile

compile: $(OBJECTS) $(STATIC) $(LIBRARY)
	@echo $(STROPTS)

*.o: *.h */*.h *.c */*.c

#
# Build the library
#
$(LIBRARY): $(OBJECTS)
	$(CC) $(SHARED) -o $@ $^ $(CORE_DYN) $(LDFLAGS)
	$(STRIP) $(LIBRARY)

#
# Build the static library
# Direct stderr to null so we don't see the 'empty file' warnings
#
$(STATIC): $(OBJECTS)
	$(AR) -rcuv $@ $^ 2>/dev/null

#
#	Clean up all generated files
#
clean:
	rm -f $(LIBRARY) $(STATIC) $(OBJECTS)

