message( "Included" )

set( ANDROID_SDK_VERSION_NUMBER 8 )
set( ANDROID_NDK_TARGET_PLATFORM 14 )
set( ANDROID_NDK_VERSION android-ndk-r8b )
set( ANDROID_SDK_ROOT $ENV{ANDROID_SDK_ROOT} )

#set( ANDROID_NDK_PREBUILT arm-linux-androideabi-4.4.3 )
set( ANDROID_NDK_PREBUILT_VERSION 4.6 )
set( ANDROID_NDK_PREBUILT_VERSION_LONG 4.6.x )
set( ANDROID_NDK_PREBUILT_ARCH_LONG arm-linux-androideabi )
set( ANDROID_NDK_PREBUILT_ARCH thumb )
#set( ANDROID_NDK_PREBUILT_ARCH armv7-a )
#set( ANDROID_NDK_PREBUILT_ARCH armv7-a/thumb )
#set( ANDROID_NDK_PREBUILT_ARCH ${ANDROID_NDK_PREBUILT_VERSION_LONG}-google )

set( ANDROID_NDK_PREBUILT ${ANDROID_NDK_PREBUILT_ARCH_LONG}-${ANDROID_NDK_PREBUILT_VERSION} )


# if the version is less than X, then build is before prebuilt and platforms
#set( ANDROID_NDK_BUILD build )


SET(ANDROID true)
SET(ANDROID_OPTION_LIST
    -I${ANDROID_SDK_ROOT}/${ANDROID_NDK_VERSION}/build/platforms/android-8/arch-arm/usr/include
    -fpic
    -mthumb-interwork
    -ffunction-sections
    -funwind-tables
    -fstack-protector
    -fno-short-enums
    -D__ARM_ARCH_5__
    -D__ARM_ARCH_5T__
    -D__ARM_ARCH_5E__
    -D__ARM_ARCH_5TE__
    -Wno-psabi
    -march=armv5te
    -mtune=xscale
    -msoft-float
    -mthumb
    -Os
    -fomit-frame-pointer
    -fno-strict-aliasing
    -finline-limit=64
    -DANDROID
    -Wa,--noexecstack
#more options
    -fvisibility=hidden
    -fvisibility-inlines-hidden
    -fdata-sections
    -DBOOST_THREAD_LINUX
    -DBOOST_HAS_PTHREADS
    -D_REENTRANT
    -D_GLIBCXX__PTHREADS
    -DANDROID
    -D__ANDROID__
    -DBOOST_HAS_GETTIMEOFDAY
    -DSQLITE_OMIT_LOAD_EXTENSION
)

foreach(arg ${ANDROID_OPTION_LIST})
  set(ANDROID_COMPILE_FLAGS "${ANDROID_COMPILE_FLAGS} ${arg}")
endforeach(arg ${ANDROID_OPTION_LIST})

SET(CMAKE_CXX_COMPILE_OBJECT "<CMAKE_CXX_COMPILER>  <DEFINES> <FLAGS> ${ANDROID_COMPILE_FLAGS} -o <OBJECT> -c <SOURCE>")
SET(CMAKE_C_COMPILE_OBJECT "<CMAKE_C_COMPILER>  <DEFINES> <FLAGS> ${ANDROID_COMPILE_FLAGS} -o <OBJECT> -c <SOURCE>")

SET(ANDROID_NDK_PREFIX "${ANDROID_SDK_ROOT}/${ANDROID_NDK_VERSION}/")
SET(ANDROID_LINK_EXE_FLAGS
    "-nostdlib -Bdynamic -Wl,-dynamic-linker,/system/bin/linker -Wl,--gc-sections -Wl,-z,nocopyreloc" 
    )
SET(ANDROID_CRT_PRE
    "${ANDROID_NDK_PREFIX}/build/platforms/android-${ANDROID_NDK_TARGET_PLATFORM}/arch-arm/usr/lib/crtbegin_dynamic.o" 
   )
SET(ANDROID_CRT_POST_LIST
    "${ANDROID_NDK_PREFIX}/${ANDROID_NDK_BUILD}/platforms/android-${ANDROID_NDK_TARGET_PLATFORM}/arch-arm/usr/lib/libstdc++.a" 
    "${ANDROID_NDK_PREFIX}/${ANDROID_NDK_BUILD}/toolchains/${ANDROID_NDK_PREBUILT}/prebuild/windows/lib/gcc/arm-linux-androideabi/${ANDROID_NDK_PREBUILT_VERSION}/${ANDROID_NDK_PREBUILT_VERSION}/thumb/libgcc.a" 
    "${ANDROID_NDK_PREFIX}/${ANDROID_NDK_BUILD}/platforms/android-${ANDROID_NDK_TARGET_PLATFORM}/arch-arm/usr/lib/libc.so" 
    "${ANDROID_NDK_PREFIX}/${ANDROID_NDK_BUILD}/platforms/android-${ANDROID_NDK_TARGET_PLATFORM}/arch-arm/usr/lib/libm.so" 
    "-Wl,--no-undefined" 
    "-Wl,-z,noexecstack" 
    "-L${ANDROID_NDK_PREFIX}/${ANDROID_NDK_BUILD}/platforms/android-${ANDROID_NDK_TARGET_PLATFORM}/arch-arm/usr/lib" 
    "-llog" 
    "-Wl,-rpath-link=${ANDROID_NDK_PREFIX}/${ANDROID_NDK_BUILD}/platforms/android-${ANDROID_NDK_TARGET_PLATFORM}/arch-arm/usr/lib" 
    "${ANDROID_NDK_PREFIX}/${ANDROID_NDK_BUILD}/platforms/android-${ANDROID_NDK_TARGET_PLATFORM}/arch-arm/usr/lib/libstdc++.a" 
    "${ANDROID_NDK_PREFIX}/${ANDROID_NDK_BUILD}/toolchains/${ANDROID_NDK_PREBUILT}/prebuild/windows/lib/gcc/arm-linux-androideabi/${ANDROID_NDK_PREBUILT_VERSION}/${ANDROID_NDK_PREBUILT_VERSION}/thumb/libgcc.a" 
    "${ANDROID_NDK_PREFIX}/${ANDROID_NDK_BUILD}/platforms/android-${ANDROID_NDK_TARGET_PLATFORM}/arch-arm/usr/lib/crtend_android.o" 
   )
foreach(arg ${ANDROID_CRT_POST_LIST})
  set(ANDROID_CRT_POST "${ANDROID_CRT_POST} ${arg}")
endforeach(arg ${ANDROID_CRT_POST_LIST})
SET(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_C_COMPILER>  ${ANDROID_LINK_EXE_FLAGS} <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> ${ANDROID_CRT_PRE} <OBJECTS>  -o <TARGET> <LINK_LIBRARIES> ${ANDROID_CRT_POST}")
SET(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER>  ${ANDROID_LINK_EXE_FLAGS} <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> ${ANDROID_CRT_PRE} <OBJECTS>  -o <TARGET> <LINK_LIBRARIES> ${ANDROID_CRT_POST}")

SET(ANDROID_LINK_SHARED_FLAGS
    "-nostdlib -Wl,-soname,<TARGET> -Wl,-shared,-Bsymbolic" 
    )
SET(ANDROID_LINK_SHARED_LIBS_LIST
    "-Wl,--whole-archive" 
    "-Wl,--no-whole-archive" 
    "${ANDROID_NDK_PREFIX}/${ANDROID_NDK_BUILD}/platforms/android-${ANDROID_NDK_TARGET_PLATFORM}/arch-arm/usr/lib/libmissing.a" 
    "${ANDROID_NDK_PREFIX}/${ANDROID_NDK_BUILD}/platforms/android-${ANDROID_NDK_TARGET_PLATFORM}/arch-arm/usr/lib/libstdc++.a" 
    "${ANDROID_NDK_PREFIX}/${ANDROID_NDK_BUILD}/toolchains/${ANDROID_NDK_PREBUILT}/prebuild/windows/lib/gcc/arm-linux-androideabi/${ANDROID_NDK_PREBUILT_VERSION}/${ANDROID_NDK_PREBUILT_VERSION}/thumb/libgcc.a" 
    "${ANDROID_NDK_PREFIX}/${ANDROID_NDK_BUILD}/platforms/android-${ANDROID_NDK_TARGET_PLATFORM}/arch-arm/usr/lib/libc.so" 
    "${ANDROID_NDK_PREFIX}/${ANDROID_NDK_BUILD}/platforms/android-${ANDROID_NDK_TARGET_PLATFORM}/arch-arm/usr/lib/libm.so" 
    "-Wl,--no-undefined" 
    "-Wl,-z,noexecstack" 
    "-Wl,-rpath-link=${ANDROID_NDK_PREFIX}/${ANDROID_NDK_BUILD}/platforms/android-${ANDROID_NDK_TARGET_PLATFORM}/arch-arm/usr/lib" 
    "${ANDROID_NDK_PREFIX}/${ANDROID_NDK_BUILD}/platforms/android-${ANDROID_NDK_TARGET_PLATFORM}/arch-arm/usr/lib/libstdc++.a" 
    "${ANDROID_NDK_PREFIX}/${ANDROID_NDK_BUILD}/toolchains/${ANDROID_NDK_PREBUILT}/prebuild/windows/lib/gcc/arm-linux-androideabi/${ANDROID_NDK_PREBUILT_VERSION}/${ANDROID_NDK_PREBUILT_VERSION}/thumb/libgcc.a" 
   )
foreach(arg ${ANDROID_LINK_SHARED_LIBS_LIST})
  set(ANDROID_LINK_SHARED_LIBS "${ANDROID_LINK_SHARED_LIBS} ${arg}")
endforeach(arg ${ANDROID_LINK_SHARED_LIBS_LIST})
SET(CMAKE_CXX_CREATE_SHARED_LIBRARY
    "<CMAKE_C_COMPILER> ${ANDROID_LINK_SHARED_FLAGS} <CMAKE_SHARED_LIBRARY_CXX_FLAGS><LANGUAGE_COMPILE_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS> <CMAKE_SHARED_LIBRARY_SONAME_CXX_FLAG><TARGET_SONAME> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> ${ANDROID_LINK_SHARED_LIBS}" 
    )
