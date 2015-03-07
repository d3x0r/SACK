#
#	makefile for MatrixSSL testing
#
#	Copyright (c) Inside Secure, 2014. All Rights Reserved
#

include ../../common.mk

STATICS		= ../libmatrixsslstatic$(A) ../../crypto/libpscryptostatic$(A) ../../core/libcorestatic$(A)

CERT_EXE 	= certValidate$(E) 
CERT_OBJS 	= certValidate$(O) 
TEST_EXE 	= sslTest$(E) 
TEST_OBJS 	= sslTest$(O) 

#
#	Compile options
#
CFLAGS  += -I.. -I../.. -I../../crypto
ifdef LINK_TOKEN_BUILDER
include $(MATRIXSSL_ROOT)matrixssl/hardware/tilera/include.mk
include $(MATRIXSSL_ROOT)matrixssl/hardware/insidesecure/include.mk
STATICS += $(TSTACK_DIR)build/libtstack.a
endif

all: compile

compile: $(CERT_EXE) $(TEST_EXE)

$(TEST_EXE):$(TEST_OBJS) $(STATICS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(CERT_EXE):$(CERT_OBJS) $(STATICS)
	$(CC) -o $@ $^ $(LDFLAGS)

#
#	Clean up all generated files
#
clean:
	rm -f $(TEST_EXE) $(TEST_OBJS) $(CERT_EXE) $(CERT_OBJS)
