
set VAR=%~dp0
set VAR2=%VAR:\=/%
cmake -G "Unix Makefiles" -DINTERSHELL_SDK_ROOT_PATH=%VAR2%../../../test1/debug_out/intershell M:/sack/android_build/intershell
