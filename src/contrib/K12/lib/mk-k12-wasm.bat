

call emcc -O3 --memory-init-file 0 -o ./k12-1.js -s EXPORTED_FUNCTIONS="['_KangarooTwelve_Initialize','_KangarooTwelve_Update','_KangarooTwelve_Final','_KangarooTwelve_Squeeze','_NewKangarooTwelve','_malloc','_free','_KangarooTwelve_IsAbsorbing', '_KangarooTwelve_IsSqueezing','_KangarooTwelve_phase']" -IInplace32BI  KangarooTwelve.c

call emcc -O3 --memory-init-file 0 -o ./k12-w.js -s WASM=1  -s EXPORTED_FUNCTIONS="['_KangarooTwelve_Initialize','_KangarooTwelve_Update','_KangarooTwelve_Final','_KangarooTwelve_Squeeze','_NewKangarooTwelve','_malloc','_free','_KangarooTwelve_IsAbsorbing', '_KangarooTwelve_IsSqueezing','_KangarooTwelve_phase']" -IInplace32BI  KangarooTwelve.c

:call google-closure-compiler.cmd --language_out NO_TRANSPILE --formatting=pretty_print  --js=K12.js    --js_output_file=K12.db.js

M:\javascript\wasm\tools\wasm2wat k12.wasm >k12.wat
M:\javascript\wasm\tools\wat2wasm k12.wat -o k12b.wasm

:call emcc -O3  -o ./k122.js -s EXTRA_EXPORTED_RUNTIME_METHODS=["stringToUTF8"] -s EXPORTED_FUNCTIONS="['_KangarooTwelve_Initialize','_KangarooTwelve_Update','_KangarooTwelve_Final','_KangarooTwelve_Squeeze','_NewKangarooTwelve','_malloc','_free','stringToUTF8']" -IInplace32BI  KangarooTwelve.c emscripten_bindings.c

