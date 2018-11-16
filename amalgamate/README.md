
# Amalgamation project

This is an attempt to carve out 'small' peices of the overall library into single source file
amalgamations.

This is used in internal work projects for Karaway, and for sack.vfs Node.JS plugin.  Which builds
a single sack.cc and then the extra wrap code in small modules for Node.

Amalgamation Product repositories and individual documentations

 - [Minimal; just types](https://github.com/d3x0r/micro-C-Boost-Types)
 - [System(Tasks) and File System](https://github.com/d3x0r/micro-C-Boost-FileSystem)
 - [Netowrking](https://github.com/d3x0r/micro-C-Boost-Network)
 - [Full Core](https://github.com/d3x0r/micro-C-Boost-Core)


[JSOX](jsox) Amalgametion is an attempt to get jsut the C JSOX parser to compile to web assembly.
I have another repository that has the basic work of this;  It needs work interfaceing on the 
revivial of JS Objects through wasm.

