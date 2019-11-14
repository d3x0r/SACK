gcc -shared -DSQLITE_ENABLE_COLUMN_METADATA=1 -std=c++17 dekware_trigger.cc  ..\..\fullcore\sack_ucb_full.cc -I M:/sack/src/contrib/sqlite/3.27.1-TableAlias -l odbc32 M:/sack/src/contrib/sqlite/3.27.1-TableAlias/sqlite3.c -lws2_32 -lwinmm  -liphlpapi -lole32 -lrpcrt4  -lsupc++

: -lrpcrt4   - uuid
: -liphlpapi - SendARP
: -lole32    - CoInitialize