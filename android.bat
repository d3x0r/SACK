

set ANDROID_SDK_ROOT=c:/general/storage/android
set ANDROID_NDK_VERSION=android-ndk-r8e
set ANDROID_DIR=c:/general/storage/%ANDROID_NDK_VERSION%
set PWD=%~dp0
set PWD=%PWD:\=/%

cmake -G "MinGW Makefiles" %PWD%cmake_all -DBUILD_MONOLITHIC=0 -D__LINUX__=1 -D__ARM__=1 -D__NO_ODBC__=1 -D__ANDROID__=1 -DCMAKE_TOOLCHAIN_FILE=%PWD%a2_toolchain.txt -DANDROID_DIR=%ANDROID_DIR% -DANDROID_NDK_VERSION=%ANDROID_NDK_VERSION%


