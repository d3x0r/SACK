@echo This script builds all source libraries for amalgamate/wasmgui

cd src\contrib\genx
call emccmk.bat
copy *.lo ..\..\..\amalgamate\wasmgui\libs

cd ..\sexpat
call emccmk.bat
copy *.lo ..\..\..\amalgamate\wasmgui\libs

cd ..\freetype-2.8
call emccmk.bat
copy *.lo ..\..\..\amalgamate\wasmgui\libs

cd ..\zlib-1.2.11
call emccmk.bat
copy *.lo ..\..\..\amalgamate\wasmgui\libs

cd ..\jpeg-9
call emccmk.bat
copy *.lo ..\..\..\amalgamate\wasmgui\libs

cd ..\libpng-1.6.34
call emccmk.bat
copy *.lo ..\..\..\amalgamate\wasmgui\libs

cd ..\..\imglib
call emccmk.bat
copy *.lo ..\..\amalgamate\wasmgui\libs

cd puregl2
call emccmk.bat
copy *.lo ..\..\..\amalgamate\wasmgui\libs

cd ..\..\vidlib\puregl2
call emccmk.bat
copy *.lo ..\..\..\amalgamate\wasmgui\libs

cd ..\..\psilib
call emccmk.bat
copy *.lo ..\..\..\amalgamate\wasmgui\libs
@echo on
: reset to home
cd %~dp0

cd amalgamate\fullcore

call mk.bat
copy sack_ucb* ..\wasmgui\libs

