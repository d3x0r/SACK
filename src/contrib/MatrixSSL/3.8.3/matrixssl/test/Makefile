#
#	Makefile for MatrixSSL testing
#
#	Copyright (c) 2013-2016 INSIDE Secure Corporation. All Rights Reserved.
#

MATRIXSSL_ROOT:=../..
TEST_SRC:=sslTest.c
CERT_SRC:=certValidate.c
SRC:=$(TEST_SRC) $(CERT_SRC)

include $(MATRIXSSL_ROOT)/common.mk

# Generated files
CERT_EXE:=certValidate$(E)
TEST_EXE:=sslTest$(E)

# Linked files
STATIC:=../libssl_s.a $(MATRIXSSL_ROOT)/crypto/libcrypt_s.a $(MATRIXSSL_ROOT)/core/libcore_s.a


# Generate map file with sections and C source reference for sslTest
ifndef MATRIX_DEBUG
ifeq (,$(findstring -apple,$(CCARCH)))
LDFLAGS += \
 -Wl,--cref \
 -Wl,-s \
 -Wl,-Map="$@.map"
endif
endif


all: compile

compile: $(OBJS) $(CERT_EXE) $(TEST_EXE)

# Additional Dependencies
$(OBJS): $(MATRIXSSL_ROOT)/common.mk Makefile $(wildcard *.h)

$(TEST_EXE): $(TEST_SRC:.c=.o) $(STATIC)
	$(CC) -o $@ $^ $(LDFLAGS)

$(CERT_EXE):$(CERT_SRC:.c=.o) $(STATIC)
	$(CC) -o $@ $^ $(LDFLAGS)

ifneq (,$(findstring -octeon,$(CCARCH)))
 SIM:=oct-sim
 SIMFLAGS:=-quiet -numcores=1 -noperf
endif

test:
	$(SIM) ./$(TEST_EXE) $(SIMFLAGS)

clean:
	rm -f $(TEST_EXE) $(CERT_EXE) $(OBJS)

