:goto skip1

mkdir build
cd build
echo !!! need to do a configuration here...
call ..\..\android.bat
make install

pause
cd /D %~dp0

:call %ANDROID_HOME%\tools\android.bat list target

: name is <name>.apk else 

:call %ANDROID_HOME%\tools\android.bat create project --target "android-14" --name EditOptions  --path ./sack --activity NativeActivity --package org.d3x0r.sack.EditOptions

:skip1

cd .\sack
mkdir libs
mkdir libs\armeabi
mkdir assets
mkdir res
mkdir res\drawable
copy ..\..\test1\debug_out\core\bin\images res\drawable
copy ..\..\test1\debug_out\core\bin\interface.conf assets
copy ..\..\test1\debug_out\core\lib libs\armeabi

c:\tools\apache-ant-1.8.3\bin\ant debug

cd bin

adb install EditOptions-debug.apk

