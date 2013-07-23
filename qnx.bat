

set PWD=%~dp0
set PWD=%PWD:\=/%

cmake -G "MinGW Makefiles" %PWD%cmake_all -DBUILD_MONOLITHIC=0 -D__LINUX__=1 -D__ARM__=0 -D__NO_ODBC__=1 -D__ANDROID__=0 -DNEED_UUID=1 -DCMAKE_TOOLCHAIN_FILE=%PWD%qnx_toolchain.txt 


