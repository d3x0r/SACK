
set SOURCE_ROOT=%~dp0..\debug_solution\core
set BUILD_ROOT=%~dp0..\debug_out\core
:goto skip1

cd %SOURCE_ROOT%
make install

pause
cd /D %~dp0

:call %ANDROID_HOME%\tools\android.bat list target

: name is <name>.apk else 

:call %ANDROID_HOME%\tools\android.bat create project --target "android-14" --name EditOptions  --path ./sack --activity NativeActivity --package org.d3x0r.sack.EditOptions
call %ANDROID_HOME%\tools\android.bat update project --target "android-14" --path ./sack 
:skip1

cd .\sack
mkdir libs
mkdir libs\armeabi
mkdir assets
mkdir res
mkdir res\drawable
copy %BUILD_ROOT%\bin\images res\drawable
copy %BUILD_ROOT%\bin\interface.conf assets
copy %BUILD_ROOT%\lib libs\armeabi

call ant debug

cd bin

adb install EditOptions-debug.apk

