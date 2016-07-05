##
#	Common Makefile definitions.
#	Copyright (c) 2013-2016 INSIDE Secure Corporation. All Rights Reserved.
#
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
## Makefile variables that must be defined in this file
# @param[out] $(BUILD) Set here for release or debug
BUILD:=release  ##< Release build strips binary and optimizes
#BUILD:=debug 	##< Debug build keeps debug symbols and disables compiler optimizations. Assembly language optimizations remain enabled

#-------------------------------------------------------------------------------
## Makefile variables that are read by this file.
# @param[in] $(MATRIXSSL_ROOT) Must be set to root MatrixSSL directory
# @param[in] $(CC) Used to determine the target platform, which will differ
# from host if cross compiling.
# @param[in] $(CPU) If set, should be the target cpu for the compiler,
# suitable for the '-mcpu=' flag. See 'gcc --help=target' for valid values.
# @param[in] $(SRC) List of source files to be compiled. Used to make $(OBJS),
# the list of object files to build.

#-------------------------------------------------------------------------------
## Makefile variables that are modified by this file
# @param[in,out] $(CFLAGS) Appended with many options as determined by this file, to be passed to compiler
# @param[in,out] $(LDFLAGS) Appended with many options as determined by this file, to be passed to linker

#-------------------------------------------------------------------------------
## Makefile variables that are created by this file
# @param[out] $(OSDEP) Set to platform code directory (./core/$OSDEP/osdep.c), based on $(CC)
# @param[out] $(CCARCH) Set to compiler's target architecture, based on $(CC)
# @param[out] $(STRIP) Set to the executable to use to strip debug symbols from executables
# @param[out] $(STROPS) Human readable description of relevant MatrixSSL compile options.
# @param[out] $(O) Set to the target platform specific object file extension
# @param[out] $(A) Set to the target phatform specific static library (archive) file extension
# @param[out] $(E) Set to the target platform specific executable file extension
# @param[out] $(OBJS) Set to the list of objects that is to be built

#-------------------------------------------------------------------------------

## Auto-detect cross compiler for some platforms based on environment variables

## Based on the value of CC, determine the target, eg.
#  x86_64-redhat-linux
#  i686-linux-gnu
#  x86_64-apple-darwin14.0.0
#  arm-linux-gnueabi
#  arm-linux-gnueabihf
#  arm-none-eabi
#  mips-linux-gnu
#  mipsisa64-octeon-elf-gcc
#  powerpc-linux-gnu
CCARCH:=$(shell $(CC) -v 2>&1 | sed -n '/Target: / s/// p')
CCVER:=$(shell $(CC) --version 2>&1)
STROPTS:="Built for $(CCARCH)"

## uname of the Host environment, eg.
#  Linux
#  Darwin
# @note Unused
#UNAME:=$(shell uname)

## Standard file extensions for Linux/OS X.
O:=.o
A:=.a
E=

# Check if this version of make supports undefine
ifneq (,$(findstring undefine,$(.FEATURES)))
 HAVE_UNDEFINE:=1
endif

#On OS X, Xcode sets CURRENT_VARIANT to normal, debug or profile
ifneq (,$(findstring -apple,$(CCARCH)))
 ifneq (,$(findstring ebug,$(CONFIGURATION)))
  MATRIX_DEBUG:=1
 endif
endif

#Manually enable debug here
#MATRIX_DEBUG:=1

ifdef MATRIX_DEBUG
 OPT:=-O0 -g -DDEBUG -Wall
 #OPT+=-Wconversion
 STRIP:=test # no-op
else
 ifneq (,$(findstring -none,$(CCARCH)))
  OPT:=-Os -Wall	# Compile bare-metal for size
 else
  OPT:=-O3 -Wall	# Compile all others for speed
 endif
 STRIP:=strip
endif
CFLAGS+=$(OPT)

# Detect multicore and do parallel build. Uncomment if desired
#ifneq (,$(findstring -linux,$(CCARCH)))
# JOBS:=-j$(shell grep -ic processor /proc/cpuinfo)
#else ifneq (,$(findstring apple,$(CCARCH)))
# JOBS:=-j$(shell sysctl -n machdep.cpu.thread_count)
#endif

default: $(BUILD)

debug:
	@$(MAKE) compile "MATRIX_DEBUG=1"

release:
	@$(MAKE) $(JOBS) compile

# 64 Bit Intel Target
ifneq (,$(findstring x86_64-,$(CCARCH)))
 CFLAGS+=-m64
 STROPTS+=", 64-bit Intel RSA/ECC ASM"
 # Enable AES-NI if the host supports it (assumes Host is Target)
 ifneq (,$(findstring -linux,$(CCARCH)))
  ifeq ($(shell grep -o -m1 aes /proc/cpuinfo),aes)
   CFLAGS+=-maes -mpclmul -msse4.1
   STROPTS+=", AES-NI ASM"
  endif
 else ifneq (,$(findstring apple,$(CCARCH)))
  ifeq ($(shell sysctl -n hw.optional.aes),1)
   CFLAGS+=-maes -mpclmul -msse4.1
   STROPTS+=", AES-NI ASM"
  endif
 endif

# 32 Bit Intel Edison Target
else ifneq (,$(findstring i586-,$(CCARCH)))
 CFLAGS+=-m32
 ifneq (,$(findstring edison,$(shell uname -n)))
  ifneq (,$(findstring -O3,$(OPT)))
   #Edison does not like -O3
   OPT:=-O2
  endif
  CFLAGS+=-DEDISON -maes -mpclmul -msse4.1
  STROPTS+=", 32-bit Intel RSA/ECC ASM, AES-NI ASM, Intel Edison"
 else
  STROPTS+=", 32-bit Intel RSA/ECC ASM"
 endif

# 32 Bit Intel Target
else ifneq (,$(findstring i686-,$(CCARCH)))
 CFLAGS+=-m32
 STROPTS+=", 32-bit Intel RSA/ECC ASM"

# MIPS Target
else ifneq (,$(findstring mips-,$(CCARCH)))
 STROPTS+=", 32-bit MIPS RSA/ECC ASM"

# MIPS64 Target 
else ifneq (,$(filter mips%64-,$(CCARCH)))
# STROPTS+=", 64-bit MIPS64 RSA/ECC ASM"

# ARM Target
else ifneq (,$(findstring arm,$(CCARCH)))
 STROPTS+=", 32-bit ARM RSA/ECC ASM"
 ifneq (,$(findstring linux-,$(CCARCH)))
  HARDWARE:=$(shell sed -n '/Hardware[ \t]*: / s/// p' /proc/cpuinfo)
  # Raspberry Pi Host and Target
  ifneq (,$(findstring BCM2708,$(HARDWARE)))
   CFLAGS+=-DRASPBERRYPI -mfpu=vfp -mfloat-abi=hard -ffast-math -march=armv6zk -mtune=arm1176jzf-s
   STROPTS+=", Raspberry Pi"
  endif
  # Raspberry Pi 2 Host and Target
  ifneq (,$(findstring BCM2709,$(HARDWARE)))
   ifneq (,$(findstring 4.6,$(CCVER)))
    CFLAGS+=-march=armv7-a
   else
    # Newer gcc (4.8+ supports this cpu type)
    CFLAGS+=-mcpu=cortex-a7
   endif
   CFLAGS+=-DRASPBERRYPI2 -mfpu=neon-vfpv4 -mfloat-abi=hard
   STROPTS+=", Raspberry Pi2"
  endif
  # Beagleboard/Beaglebone Host and Target
  ifneq (,$(findstring AM33XX,$(HARDWARE)))
   CFLAGS+=-BEAGLEBOARD -mfpu=neon -mfloat-abi=hard -ffast-math -march=armv7-a -mtune=cortex-a8
   STROPTS+=", Beagleboard"
  endif
  # Samsung Exynos 5 (Can also -mtune=cortex-a15 or a8)
  ifneq (,$(findstring EXYNOS5,$(HARDWARE)))
   CFLAGS+=-DEXYNOS5 -mfpu=neon -mfloat-abi=hard -ffast-math -march=armv7-a
   STROPTS+=", Exynos 5"
  endif
  ifdef HAVE_UNDEFINE
   undefine HARDWARE
  endif
 endif

endif

ifdef MATRIX_DEBUG
CFLAGS+=-ffunction-sections -fdata-sections
else
CFLAGS+=-ffunction-sections -fdata-sections -fomit-frame-pointer
endif

# If we're using clang (it may be invoked via 'cc' or 'gcc'),
#  handle minor differences in compiler behavior vs. gcc
ifneq (,$(findstring clang,$(CCVER)))
 CFLAGS+=-Wno-error=unused-variable -Wno-error=\#warnings -Wno-error=\#pragma-messages
endif

# Handle differences between the OS X ld and GNU ld
ifneq (,$(findstring -apple,$(CCARCH)))
 LDFLAGS+=-Wl,-dead_strip
else
 LDFLAGS+=-Wl,--gc-sections
endif

CFLAGS+=-I$(MATRIXSSL_ROOT)

#OPENSSL
#PS_OPENSSL:=1
ifdef PS_OPENSSL
 OPENSSL_ROOT:=/opt/openssl-1.0.2d
 ifdef OPENSSL_ROOT
  # Statically link against a given openssl tree
  CFLAGS+=-I$(OPENSSL_ROOT)/include
  LDFLAGS+=$(OPENSSL_ROOT)/libcrypto.a -ldl
 else ifneq (,$(findstring -apple,$(CCARCH)))
  # Dynamically link against the sytem default openssl tree
  # Apple has deprecated the built in openssl, so supress warnings here
  CFLAGS+=-Wno-error=deprecated-declarations -Wno-deprecated-declarations
  LDFLAGS+=-lcrypto
 else ifneq (,$(findstring -linux,$(CCARCH)))
  # Dynamically link against the sytem default openssl tree
  LDFLAGS+=-lcrypto
 else
  $(error Please define OPENSSL_ROOT)
 endif
 CFLAGS+=-DUSE_OPENSSL_CRYPTO
 STROPTS+=", USE_OPENSSL_CRYPTO"
endif
#OPENSSL

#LIBSODIUM
#PS_LIBSODIUM:=1
ifdef PS_LIBSODIUM
 LIBSODIUM_ROOT:=/opt/libsodium-1.0.10/src/libsodium
 ifdef LIBSODIUM_ROOT
  # Statically link against a given libsodium
  CFLAGS+=-I$(LIBSODIUM_ROOT)/include
  LDFLAGS+=$(LIBSODIUM_ROOT)/.libs/libsodium.a
 else
  $(error Please define LIBSODIUM_ROOT)
 endif
 CFLAGS+=-DUSE_LIBSODIUM_CRYPTO
 STROPTS+=", USE_LIBSODIUM_CRYPTO"
endif
#LIBSODIUM

# Linux Target
ifneq (,$(findstring -linux,$(CCARCH)))
 OSDEP:=POSIX
 #For USE_HIGHRES_TIME
 LDFLAGS+=-lrt

# OS X Target
else ifneq (,$(findstring -apple,$(CCARCH)))
 OSDEP:=POSIX
 CFLAGS+=-isystem -I/usr/include

# Bare Metal / RTOS Target
else ifneq (,$(findstring -none,$(CCARCH)))
 OSDEP:=METAL
 CFLAGS+=-fno-exceptions -fno-unwind-tables -fno-non-call-exceptions -fno-asynchronous-unwind-tables -ffreestanding -fno-builtin -nostartfiles
 ifneq (,$(findstring cortex-,$(CPU)))
  CFLAGS+=-mthumb -mcpu=$(CPU) -mslow-flash-data
  ifeq (cortex-m4,$(CPU))
   CFLAGS+=-mcpu=cortex-m4 -mtune=cortex-m4
  else ifeq (cortex-m3,$(CPU))
   CFLAGS+=-mcpu=cortex-m3 -mtune=cortex-m3 -mfpu=vpf
  else ifeq (cortex-m0,$(CPU))
   CFLAGS+=-mcpu=cortex-m0 -mtune=cortex-m0 -mfpu=vpf
  endif
 endif
endif

# This must be defined after OSDEP, because core/Makefile uses OSDEP in SRC
OBJS=$(SRC:.c=.o) $(SRC:.S:*.o)

# Remove extra spaces in CFLAGS
#CFLAGS=$(strip $(CFLAGS))

# Display the precompiler defines for the current build settings
defines:
	:| $(CC) $(CFLAGS) -dM -E -x c -

