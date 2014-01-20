
BUILD_TYPE=debug

BUILD=$(pwd)
HERE=$(dirname $(pwd)/$0)

MONOLITHIC=0
__LINUX__=1
__NO_GUI__=1
__LINUX64__=1
__LINUX__=1
NEED_EXPAT=1
USE_OPTIONS=0
USE_ODBC=0
USE_SQLITE_EXTERNAL=1

mkdir -p $BUILD/sack/${BUILD_TYPE}_solution/sack
cd $BUILD/sack/${BUILD_TYPE}_solution/sack
SACK_SDK_ROOT_PATH=${BUILD}/sack/${BUILD_TYPE}_out/sack
echo cmake $HERE -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DNEED_EXPAT=${NEED_EXPAT} -D__NO_GUI__=${__NO_GUI__} -D__LINUX__=${__LINUX__} -D__LINUX64__=${__LINUX64__} -DCMAKE_INSTALL_PREFIX=$SACK_SDK_ROOT_PATH >mk
echo 'make install' >>mk
chmod 755 mk
./mk
${BUILD}/sack/${BUILD_TYPE}_out/sack/bin/${BUILD_TYPE}/sack_deploy

mkdir -p $BUILD/sack/${BUILD_TYPE}_solution/binary
cd $BUILD/sack/${BUILD_TYPE}_solution/binary
SACK_BINARY_SDK_ROOT_PATH=${BUILD}/sack/${BUILD_TYPE}_out/binary
cmake $HERE/binary -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DSACK_SDK_ROOT_PATH=$SACK_SDK_ROOT_PATH -DCMAKE_INSTALL_PREFIX=$SACK_BINARY_SDK_ROOT_PATH
make install
${SACK_BINARY_SDK_ROOT_PATH}/Sack.Binary.Deploy


cd $BUILD
