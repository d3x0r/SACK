
#include "accstruc.h"

#define RELAY_VERSION "3.02" // 3 characters, string

// 3.02 - send a file manifest (send all files and wait for individual requests)
// 3.01 - send all 3 64 bit times; lots of mods...
// 2.11 - converted to using configscript lib, filemon lib.
// 2.10 - implemeted connection options
// 2.0 - implement multiple seperate outgoing directories
//     - disabled directory updates
//     - directory outgoing takes a file mask

// 1.8 - changed MESSAGE responce to controllers to include a length char
//     - Removed forced DoScan


