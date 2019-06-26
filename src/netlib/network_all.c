///////////////////////////////////////////////////////////////////////////
//
// Filename    -  network_all.c
//
// Description -  Utility source to merge network providers for
//                amalgamated targets; provides #ifdef wrappers
//
// Author      -  James Buckeyne
//
// Create Date -  2019-06-26
//
///////////////////////////////////////////////////////////////////////////


#ifdef __MAC__
#include "network_mac.c"
#endif

#ifdef _WIN32
#include "network_win32.c"
#endif

#ifdef __LINUX__
#include "network_linux.c"
#endif
