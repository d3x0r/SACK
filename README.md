# SACK

[![Gitter](https://badges.gitter.im/FreedomCollective/Sack.svg)](https://gitter.im/FreedomCollective/Sack?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)


A document published from header document information - http://sack.sf.net  ( https://sourceforge.net/projects/sack/ )

Git is often more up to date.  Git is the primary development source control system.  Mercurial is no longer used. (Sourceforge obsolete)

Monotone would have been best; but; well... maybe they were too closed.

## Single Source+Header Amalgamations

C single source packages.  C/C++ compilable sources, and a single header.  Links to relavent docs in each project...

- [Maximal](https://github.com/d3x0r/micro-C-Boost-Core)
- [Minimal](https://github.com/d3x0r/micro-C-Boost-Types)
- [Minimal + File System Asctractions](https://github.com/d3x0r/micro-C-Boost-FileSystem)
- [Minimal + FS + Networking ](https://github.com/d3x0r/micro-C-Boost-Network)

Exposed through interop to V8 through Node.js...

- As a Node addon/V8 extension with [FS,Networking,Sqlite,ODBC,more](https://github.com/d3x0r/sack.vfs), or as a [NPM package](npmjs.org/package/sack.vfs).

---------------

# System Abstraction Component Kit

This is generally a large collection of tiny things.  It compiles for C/C++.  It provides abstractions between Windows and Posix and even various flavors of posix, such as Linux, Android, MAC, et al.
It's SDL (sort of), it's GTK (sorta), it's STL (stacks, lists, queue, constainers, generics, ...), it can rely on almost none of the C library even. 

### Where Did it Come from/Why Does it Exist?

In the beginning, there was a DOS serial terminal program.  It was written in C.  There were of course the basic container types missing, so basically every structure was some new implementation of something old; but it was concise and didn't have extra things it iddn't 'need' in the case of a generic class which may provide things that are unused except in specific cases. This actually had threads;  it had several instruction stacks at vairous states that it could relinquish time to.

It evolved into a proprietary OS called NIPC (new inter process communications), which not only had threads, but could load DOS .exe files as processes into the threads (not just .COM files).  This managed memory allocation, display access, disk acceses, and CPU time in a round-robin fashion.  In the beginning there was no sleep, but later threads could take themselves out of schedule to not even be woken.  On a 286-25 it got aound 1500 cycles a second.  But most of its real work was in interrupt handling, communicating with some other system.

Then there was windows 3.1... and NT 3.51 for a short time until NT4 came out.  So now all the custom thread control was fairly obsolete, but that allocator, that was pretty good.  And there was lots of development of shared memory pipes/queues/etc and Sockets.  There were sockets in NIPC too; written as a UDP/TCP/IP stack, to a network card that was directly written to; the API for that was of course event based, because I really didn't know any other sort of method to deal with network.  There was of course Berkley sockets, and they told me I should implement that API, and it was so very convoluted... I have to sit and wait for data?

Then there was the Internet, there was life as a hermit, pining for the one who wouldn't be caught, and aching from the one that changed the locks, and there was 'Vurt'.

There was a lot of tinkering on little things here and there amusing myself with Sound Blasters and 3DFX(Glide).  Some constructive geometry algorithms that work pretty well for convex solids...

So here the core really formed as a thing of its own.  It was the types (text, lists, stacks, queues, ... ), the memory allocator (which has a Hold() operation that allows that block of memory to be held after the owner frees it.) which is also wrapped into allocating in custom heaps of shared memory between processes; that and to use a file backed heap as a persistent memory that can be reloaded and the program resume in its existing memory state.
And fixed networking; turning what was polling into event based.  Networking has evolved a lot from the days in the beginning of using one WaitForMuitpleObjects(), after all, isn't 64 sockets enough for anyone? (No, 2000 clients connecting has issues with that with 80% failing altogether).
Around this time, the graphics interface layers were started.  Inspired by Allegro, and taking their ImageFile_tag {} structure initially, implemented dedicated 32 bit color path routines.  Allegro had support for all sort of pixel types, which, as fast as it was, was still slowed by calling through indirect function pointers.  I had for a time assembly versions of some of the block-copy and line routines both ASM and MMX flavors.  By the time SSE came out, compilers were doing a really good job of optimizing the C, and computers were just faster, that what was already really fast was just that much faster, that the maintainence of assmebly for various platforms/compilers obsoleted it.
Once you have images you can draw to, then it's just a matter of getting the system to show them, so a windows interface was done, with an intent on just linux framebuffer (which was never very fast). 

So, given this as a platform, Dekware as a application based on SACK took form and eventually manifested; serving as a test fixture for the library.  

SO there was a single Image library and Render library, so they were all sort of packaged into one large package.  (CMake still has BUILD_MONOLITHIC options, which is probably fairly broken now).  I was at the time playing a lot of MUD using Dekware (was even support Dekware for someone else who was running on FreeBSD, with non-gnu Make; what a pain that was... did I mention, back then I was using make, and had lots of various flavors of makefiles for lots of systems?)...  anyway, this is really the origin of SACK; before that it was just 'common' after the pattern I had learned in my previous job.
A sack is a large bag.  A bag can hold like 5 bags, but a sack can hold like 25 bags.  a BAG is a Basic Aggregate Group, or a sub-module... a peice that itself could be ommitted and not affect anything else; and SACK contains many BAGs.

So then there was a new job, building new software, and I was able to leverage SACK and build applications very quickly that would run on windows or linux with the same code and no #ifdef's.  I developed the msgsvr layer using SYSVIPC message queues, and did a common display driver for applications;  I've since lost that, I suppose I decided it was broken enough that it needed to be killed and redone.  At that time I had OpenGL, GL2, GLES, QNX, Websocket/HTML canvas display drivers already; and the message system looked good, but really was pretty bad.  (It's still there, and some things still use it; it works....)
Here, at this new job, I got to present my library to others; I didn't realize how 'sack' could mean something entirely other than I intended.  It became espcailly bad when I made this project (MILK - Modular Interface Layout Kit), which is based on my SACK; or comes from... 

With time, both for demonstration purposes, testing, and general utility, there become 'utils' folder under sack which is sort of really tiny applications based on SACK; and a 'games' folder with things that are closer to applications.  If for no other reason than reference code.

So there's a vector math library (remeber back for constructive geomtry?), a Fraction math library, which stores numbers a numbertor/denominator integers; this is used for scaling purposes in the GUI BAG.  There's a wrapper for Timers; like windows SetTimer there's AddTimer(); timers and threads (ThreadTo() instead of pthread_create/CreateThread/....).  

InterShell is a program meant for quick production of full screen dedicated function applications (kiosk interfaces, displays and signage).  It supports transparent windows, so you can layer static content over existing animated content perhaps played with something like VLC (although there is a tiny utility to play videos just using FFMPEG as a dropin control in intershell).



## What are the Peices?

Sources are generally separate, requiring fewest dependancies of others.
Someday this should be combed into actual dependancy tree that can be leveraged at a higher level.

1) deadstart support.  
  This is support for scheduling initializers performed on library load.   Provides a method to easily register a initializer that can create a thread that can run on windows (Impossible to do scheduled through DllMain).  
  Supports 0-255 levels of proirt to make sure subsystems requires are already initialized, instead of requiring the module to do 'if not initialized, initialize' at every exported function.
2) shared memory support.  Regions of memory are created specifying 'where' and 'what'.  Where specifies the physical file location that represents the memory.  The what is a name given that can be found, and the same shared memory can be known.   A custom allocator can allocate in shared memory heaps that are separated by processes, but that have been mapped together.  The internal allocator can be replaced with 'malloc/free' which are sometimes more optimal, but lack some tracking features. Memory library also supports a Hold operation, that increments an owner count.   This allows routines to 'hold' the memory and allow normal processes to deallocate the block.  Every additional 'hold' must be 'released'.   
3) Memory library provides lowest level interlocks for a system ( InterlockedExchanged ).  And TryEnterCriticalSection/LeaveCriticalSection.
4) Timer library provides an internally timer scheduler, and provides a common threading interface.  WakeableSleep is implemented here, (and WakeThread to wake a sleeping thread).  EnterCriticalSection, and LeaveCriticalSection are implemented here, because it does a thread-sleep and requires thread knowledge.  Timer schedule logic deducts the time that the timer proc ran, so the timer will be scheduled nearaest to starting on the multiple of the tick.  (Progressively losses milliseconds in reality due to lack of precision; mabye I should arbitrarily subtract one.)
5) Event based network interface.  Built on the timers and threads above, this provides a background thread that watches N sockets (limited by operating system handle limits), and provides event callbacks based on network events.   

--- 

6) type library (lists, stacks, queues, binarylists, family tree, text, text parsing, ... )
7) vector math library
8) faction library - keeps numbers in terms of numerator and denominator.  Can resolve multiple fractions mulitplied to lowest common denominator.
9) configuration script processing library.   Define format strings and a related callback called with variable arguments based on the format string, and the match to line of input processed.   Configuration library can scan for files in a variety of locations; or can be opened and fed arbitrary strings from some other external source... 
10) SQL abstraction library.  DoSQLCommand(), for( SQLRecordQueryf(); GetRecord(); ).  Supports ODBC (unixodbc on linux/android), and Sqlite native.   Option library based on sql abstraction, with a default of a sqlite database, for 0 configuration configuration options.  External utilities are given that can edit options after the face, or program startup configuration file can force options, or specify defaults for options, and read options, such that ```if( options/version == 1 ) option set options/version=2... option default otheroption/app/color=12344```

---
And somewhere ordered in the above...
11) Process registry, allows registering values, functions, and types for later consumption.  PSI control registry is based on this registry tree.  Names in the tree can be dumped for later browsing (debugging like, where did that end up?).  This module is also the library deadstart that reads interface.conf and handles loading additional modules, or configuring aliases to interfaces based on options specified.  (this deserves at least a whole wikipage)

12...n) Lots of little utilities written with this abstraction library that just generally work on all systems.

Some base functionality that is different  - create a process, handle sockets efficiently, file system abstractions (even an example virtual file system, that shows how you might implement your own file system interfaces), ping(raw sockets), arp, a whois query driver that's a little obsolete now, windows service hook to write your own services.  Even an example service that just runs an arbitrary executable.  Language translation system (nothing all that special or magical).

n+1) Intershell; THis is a generic application layout handler.  It handles high level plugins with generic controls that can be placed easily even after deployment.  Small plugins may be easily loaded through the interface providing easy event interfaces to user code.

n+10) Dekware - all of the above; it can load intershell, then extend buttons to provide scripting.  Terminal, general script processing utility... 


---

Image and Render libraries are connected via Interfaces.  An Interface is a struct of function pointers that is requested by name.  Interfaces may be aliased, so if video loads as 'video.opengl' an alias called 'video' can be created to select which of several are provided by default.

Images are 32 bit color; There are functions to provide platform ordered dwords to create colors from components or get components from colors.   Internally there are just a few tight loops optimized for 32 bit color transfers and operations.  Back in 2000 I searched for '64 bit color' which exists internally on video cards for higher precision (fewer lost decimals of precision)... but generally the user cannot differentiate every level of 256 colors so it's more than sufficient.

Linux support through navtive X now
Android can use GLES2 or Native Framebuffer (ANativeWindow?)
Windows can use Win32 Windows, OpenGL, d3d (roughly), in OpenGL, windows/controls created turn into surfaces in a 3d Space, which can be rendered in and around other 3d objects.   (This is why the image layer is a pluggable interface, because draw operations turn into opengl calls).
All can use the proxy which provides a network interafce that a browser can connect to, the draw commands are fowarded to the brwser to perform... 

Started a vulkan layer, which would simplify some things.  The image opengl interface for shaders could really use vulkan as a backend... but they're somewhat flexible now... so maybe implementing deeper layers of API In a Vulkan sort of way will be done.

## Panther's Slick Interface (psi) 

This is better implemented as a [Node GUI](https://github.com/d3x0r/sack.vfs/tree/sack-gui)

This is a control library build on registered callbacks of classes of controls.  The library is built on the image and renderer interfaces... it tracks higher level things like sliders, listboxes, buttons, and provides custom extensions based on each class.

# Dekware

Dekware build product is a Mud client/MUD.  Documentation and downloads of prebuilt versions are available at [d3x0r.org](https://d3x0r.org/dekware) or [www.d3x0r.org](https://www.d3x0r.org:444/dekware).

# SACK.vfs

Node addon that exposes core library support to Javascript.  Provides Websockets, HTTP, JSON, JSOX parsers, Sqlite/ODBC interface.

[NPM package](npmjs.com/package/sack.vfs) and [GIT Repository](https://github.com/d3x0r/sack.vfs)

# sack-gui

Node addon that includes all interfaces from sack.vfs, but also begins to implement interfafce to GUI subsystem.  

[NPM package](npmjs.com/package/sack-gui) and [GIT Repository](https://github.com/d3x0r/sack.vfs/tree/sack-gui).  
The GIT repository is a branch rooted in the master of SACK.vfs, and is just additions to existing interfaces; 
although it does change from using a sack.cc amaglamation, it uses cmake external project to download sack 
repository from github.


