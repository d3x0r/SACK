#
#	Common makefile definitions
#	Copyright (c) INSIDE Secure, 2002-2014. All Rights Reserved.
#
###########################
#
#	Things you can set
#
# 	1. Whether debug build or not
BUILD = release 
#BUILD = debug 

# 	2. Platform (./core/$OSDEP/osdep.c)
OSDEP	= POSIX

# 	3. Optional path to cryptoki.h if using PKCS11 
#PKCS11_INCL = -I./

# 	4. Optional path and names of PKCS#11 libraries
#PKCS11_LIB =


###################################
#
#	Shouldn't have to touch below
#

DFLAGS  =$(WARNINGS) 

MATRIXSSL_ROOT:=$(dir $(lastword $(MAKEFILE_LIST)))
CCARCH:=$(shell $(CC) -v 2>&1 | sed -n '/Target: / s/// p')

ifdef MATRIX_DEBUG
DFLAGS  += -g -DDEBUG 
STRIP   = test
endif

ifdef PS_OPENSSL
ifndef CROSS
$(error Please define OPENSSL_ROOT)
	OPENSSL_ROOT = 
	DFLAGS += -DUSE_OPENSSL -I$(OPENSSL_ROOT)/include
	STATICS += $(OPENSSL_ROOT)/libcrypto.a
else
	DFLAGS += -DUSE_OPENSSL
	STATICS += -lcrypto
endif
endif

default: $(BUILD) 

debug:
	@$(MAKE) compile "MATRIX_DEBUG=1"

release:
	@$(MAKE) compile

#
# For cross platform, call using something like this:
#   make TILERA=y
#   make WRT54G=y
#   make WRT54G=1
#   make WRT54G=anything_but_blank
#   make LINUXSTAMP=1 
#   make MARVELL=1 
#
ifdef TILERA
  ifndef TILERA_ROOT
  	$(error Please define TILERA_ROOT)
  endif
  TSTACK_DIR = $(MATRIXSSL_ROOT)../iTCPstack/
  CROSS:=$(TILERA_ROOT)/bin/tile-
  LINK_TOKEN_BUILDER:=1
  CFLAGS += -DUSE_HARDWARE_CRYPTO_PKA -DUSE_HARDWARE_CRYPTO_RECORD \
		-DUSE_TILERA_MICA -DUSE_TILERA_TSTACK
  LDFLAGS += -lgxio -lgxcr -ltmc -lrt -lpthread
ifndef MATRIX_DEBUG
  DFLAGS += -O2
endif
endif
ifdef WRT54G
  CROSS   = mipsel-openwrt-linux-uclibc-
endif
ifdef LINUXSTAMP
  CROSS   = arm-linux-uclibc-
endif
ifdef MARVELL
  CROSS   = arm-none-eabi-
endif
ifdef ANDROID
  CROSS   = arm-linux-androideabi-
endif
ifdef ANDROIDX86
  CROSS   = i686-android-linux-
endif

SHARED = -shared
STRIP   = strip
LDFLAGS += -lc
CFLAGS += $(DFLAGS) $(FILE_SYS) $(PKCS11_INCL)

ifdef CROSS
  CC:=$(CROSS)gcc
  STRIP:=$(CROSS)strip
  AR:=$(CROSS)ar
  LDFLAGS += -fno-builtin -static
else
  CFLAGS += -I/usr/include
endif

O       = .o
SO      = .so
A       = .a
E       =

ifndef CROSS

ifeq ($(shell uname),Darwin)
  STRIP = test
  SO = .dylib
  DFLAGS += -mdynamic-no-pic
  CFLAGS += -isystem -I/usr/include
  ifndef MATRIX_DEBUG
    DFLAGS += -O3
  endif
  ifneq (,$(findstring x86_64,$(CCARCH)))
    CFLAGS += -m64
    ifeq ($(shell sysctl -n hw.optional.aes),1)
      CFLAGS += -maes -mpclmul -msse4.1
    endif
  else
    LDFLAGS += -read_only_relocs suppress
  endif
else
  #
  # Linux shows the architecture in uname
  #
  UNAMEM:=$(shell uname -m)
  CFLAGS += -fomit-frame-pointer
  ifeq ($(UNAMEM),x86_64)
    CFLAGS  += -m64 -fPIC 
	ifndef MATRIX_DEBUG
		DFLAGS += -O3
	endif
  else ifeq ($(UNAMEM),i686)
	ifndef MATRIX_DEBUG
		DFLAGS += -O2
	endif
  else ifeq ($(UNAMEM),tilegx)
	TILERA:=1
    LINK_TOKEN_BUILDER:=1
    TSTACK_DIR = $(MATRIXSSL_ROOT)../iTCPstack/
    CFLAGS += -DUSE_HARDWARE_CRYPTO_PKA -DUSE_HARDWARE_CRYPTO_RECORD \
		-DUSE_TILERA_MICA -DUSE_TILERA_TSTACK
    LDFLAGS += -lgxio -lgxcr -ltmc -lrt -lpthread
	ifndef MATRIX_DEBUG
		DFLAGS += -O2
	endif
  endif
  HARDWARE:=$(shell cat /proc/cpuinfo | sed -n '/Hardware[ \t]*: / s/// p')
  ifneq (,$(findstring BCM2708,$(HARDWARE)))
    CFLAGS += -mfpu=vfp -mfloat-abi=hard -ffast-math \
              -march=armv6zk -mtune=arm1176jzf-s
    ifndef MATRIX_DEBUG
      DFLAGS += -Ofast
    endif
  endif
  ifneq (,$(findstring x86_64-,$(CCARCH)))
    CFLAGS+=-DTARGET_X86_64
  endif
  ifneq (,$(findstring arm-,$(CCARCH)))
    CFLAGS+=-DTARGET_ARM
  endif
  ifneq (,$(findstring -linux,$(CCARCH)))
    CFLAGS+=-DTARGET_LINUX
  endif
  ifneq (,$(findstring -apple,$(CCARCH)))
    CFLAGS+=-DTARGET_OSX
  endif
  ifneq (,$(findstring -none,$(CCARCH)))
    CFLAGS+=-DTARGET_NONE
  endif
  # Check for aes-ni support, will define __AES__ in preprocessor
  ifeq ($(shell cat /proc/cpuinfo | grep -o -m1 aes),aes)
    CFLAGS += -maes -mpclmul -msse4.1
  endif
endif
#Cross compile
else
  ifneq (,$(findstring mips,$(CROSS)))
  endif
  ifneq (,$(findstring arm,$(CROSS)))
#    CFLAGS += -mthumb -mcpu=arm9tdmi -mthumb-interwork
    CFLAGS += -mcpu=arm9tdmi -mthumb-interwork
  endif
  ifneq (,$(findstring i686,$(CROSS)))
#	TODO - for android x86 try to optimize
  endif
endif

ifeq ($(shell cc --version | grep -o clang),clang)
WARNINGS = -Wall -Wno-error=unused-variable -Wno-error=\#warnings
else ifeq ($(shell cc --version | grep -o Tilera),Tilera)
WARNINGS = -Wall
else ifdef TILERA
WARNINGS = -Wall -Werror -Wno-error=cpp -Wno-error=unused-variable -Wno-error=unused-but-set-variable
#else
WARNINGS = -Wall
endif

# STROPTS holds a string with current settings for informational display
STROPTS = "Optimizations: $(CC) "
STROPTS +=$(findstring -O0,$(CFLAGS))
STROPTS +=$(findstring -O1,$(CFLAGS))
STROPTS +=$(findstring -O2,$(CFLAGS))
STROPTS +=$(findstring -O3,$(CFLAGS))
STROPTS +=$(findstring -Os,$(CFLAGS))
STROPTS +=$(findstring -g,$(CFLAGS))

ifneq (,$(findstring -m64,$(CFLAGS)))
  STROPTS +=", 64-bit Intel RSA/ECC ASM"
else ifneq (,$(findstring PSTM_X86,$(CFLAGS)))
  STROPTS +=", 32-bit Intel RSA/ECC ASM"
else ifneq (,$(findstring PSTM_ARM,$(CFLAGS)))
  STROPTS +=", 32-bit ARM RSA/ECC ASM"
else ifneq (,$(findstring PSTM_MIPS,$(CFLAGS)))
  STROPTS +=", 32-bit MIPS RSA/ECC ASM"
else ifneq (,$(findstring PSTM_PPC,$(CFLAGS)))
  STROPTS +=", 32-bit PPC RSA/ECC ASM"
endif

ifneq (,$(findstring TILERA,$(CFLAGS)))
  STROPTS +=", Tilera SHA-2 ASM"
endif

ifneq (,$(findstring -maes,$(CFLAGS)))
  STROPTS +=", AES-NI ASM"
endif

