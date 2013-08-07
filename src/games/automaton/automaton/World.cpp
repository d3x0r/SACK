#pragma warning ( disable : 4305 )
#include <time.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#include <memory.h>
#else
#include <conio.h>  // user input method
#include <mem.h>
#endif
#include <math.h>

static unsigned char byBuffer[256];
#undef OutputDebugString
#define OutputDebugString printf

#include "world.hpp"
#include "sphere.hpp"

unsigned char gbExitProgram;

unsigned long dwStart, dwCurrent, dwFrames;

#include "shapes.hpp"

POBJECT CreateCube( RCOORD fSize, PVECTOR pv )
{
   return CreateScaledInstance( CubeNormals, CUBE_SIDES, fSize, pv );
}

extern void ScanKeyboard( void );

