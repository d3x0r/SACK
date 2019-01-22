

call emcc -O3 --memory-init-file 0 -o ./k12.js -s EXTRA_EXPORTED_RUNTIME_METHODS=["stringToUTF8"] -s EXPORTED_FUNCTIONS="['_KangarooTwelve_Initialize','_KangarooTwelve_Update','_KangarooTwelve_Final','_KangarooTwelve_Squeeze','_NewKangarooTwelve','_malloc','_free','stringToUTF8','_KangarooTwelve_IsAbsorbing', '_KangarooTwelve_IsSqueezing','_KangarooTwelve_phase']" -IInplace32BI  KangarooTwelve.c emscripten_bindings.c
:call emcc -O3  -o ./k122.js -s EXTRA_EXPORTED_RUNTIME_METHODS=["stringToUTF8"] -s EXPORTED_FUNCTIONS="['_KangarooTwelve_Initialize','_KangarooTwelve_Update','_KangarooTwelve_Final','_KangarooTwelve_Squeeze','_NewKangarooTwelve','_malloc','_free','stringToUTF8']" -IInplace32BI  KangarooTwelve.c emscripten_bindings.c

