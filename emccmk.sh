echo This script builds all source libraries for amalgamate/wasmgui

COMMON_CFLAGS=-D__MANUAL_PRELOAD__

cd src/contrib/genx
./emccmk.sh

cd ../sexpat
./emccmk.sh

cd ../freetype-2.8
./emccmk.sh

cd ../zlib-1.2.11
./emccmk.sh

cd ../jpeg-9
./emccmk.sh

cd ../libpng-1.6.34
./emccmk.sh

cd ../../imglib/puregl2
./emccmk.sh

cd ../../vidlib/puregl2
./emccmk.sh

cd ../../psilib
./emccmk.sh

#: reset to home
#:cd %~dp0

cd ../../amalgamate/fullcore

./mk.sh
copy sack_ucb* ../wasmgui/libs

