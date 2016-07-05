#
#	Makefile for crypto testing
#   'make' builds debug (Default).
#   'make gold' builds optimized.
#
#	Copyright (c) PeerSec Networks, 2002-2010. All Rights Reserved
#	Copyright (c) 2013-2016 INSIDE Secure Corporation. All Rights Reserved.
#

MATRIXSSL_ROOT:=../..
VECTOR_SRC:=algorithmTest.c
SPEED_SRC:=throughputTest.c
SRC:=$(VECTOR_SRC) $(SPEED_SRC) 

include $(MATRIXSSL_ROOT)/common.mk

# Generated files
VECTOR_EXE:=algorithmTest$(E)
SPEED_EXE:=throughputTest$(E)

# Linked files
STATICS:=../libcrypt_s.a $(MATRIXSSL_ROOT)/core/libcore_s.a
#STATICS+=$(MATRIXSSL_ROOT)/matrixssl/libssl_s.a

all: compile

compile: $(OBJS) $(SPEED_EXE) $(VECTOR_EXE)
	$(MAKE) --directory=rsaperf
	$(MAKE) --directory=eccperf
	$(MAKE) --directory=dhperf

# Additional Dependencies
$(OBJS): $(MATRIXSSL_ROOT)/common.mk Makefile $(wildcard *.h)

$(SPEED_EXE): $(SPEED_SRC:.c=.o) $(STATICS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(VECTOR_EXE): $(VECTOR_SRC:.c=.o) $(STATICS)
	$(CC) -o $@ $^ $(LDFLAGS)


ifneq (,$(findstring -octeon,$(CCARCH)))
 SIM:=oct-sim
 SIMFLAGS:=-quiet -numcores=1 -noperf
endif

test:
	$(SIM) ./$(VECTOR_EXE) $(SIMFLAGS)

clean:
	rm -f $(VECTOR_EXE) $(SPEED_EXE) $(OBJS)
	$(MAKE) clean --directory=rsaperf
	$(MAKE) clean --directory=eccperf
	$(MAKE) clean --directory=dhperf

