# SACK

[![Gitter](https://badges.gitter.im/d3x0r/SACK.svg)](https://gitter.im/d3x0r/SACK?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

A document published from header document information - https://sack.sf.net  ( https://sourceforge.net/projects/sack/ )

Git is often more up to date.  Internal mercurial server is often used.

Monotone would have been best; but; well... maybe they were too closed.

---------------

#System Abstraction Component Kit

Souces are generally separate, requiring fewest dependancies of others.
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
n+1) Intershell; THis is a generic application layout handler.  It handles high level plugins with generic controls that can be placed easily even after deployment.

n+10) Dekware - all of the above; it can load intershell, then extend buttons to provide scripting.  Terminal, general script processing utility... 


---

Image and Render libraries are connected via Interfaces.  An Interface is a struct of function pointers that is requested by name.  Interfaces may be aliased, so if video loads as 'video.opengl' an alias called 'video' can be created to select which of several are provided by default.

Images are 32 bit color; There are functions to provide platform ordered dwords to create colors from components or get components from colors.   Internally there are just a few tight loops optimized for 32 bit color transfers and operations.  Back in 2000 I searched for '64 bit color' which exists internally on video cards for higher precision (fewer lost decimals of precision)... but generally the user cannot differentiate every level of 256 colors so it's more than sufficient.

Linux support through navtive X now
Android can use GLES2 or Native Framebuffer (ANativeWindow?)
Windows can use Win32 Windows, OpenGL, d3d (roughly), in OpenGL, windows/controls created turn into surfaces in a 3d Space, which can be rendered in and around other 3d objects.   (This is why the image layer is a pluggable interface, because draw operations turn into opengl calls).
All can use the proxy which provides a network interafce that a browser can connect to, the draw commands are fowarded to the brwser to perform... 

Started a vulkan layer, which would simplify some things.  The image opengl interface for shaders could really use vulkan as a backend... but they're somewhat flexible now... so maybe implementing deeper layers of API In a Vulkan sort of way will be done.

#Panther's Slick Interface (psi) 

This is a control library build on registered callbacks of classes of controls.  The library is built on the image and renderer interfaces... it tracks higher level things like sliders, listboxes, buttons, and provides custom extensions based on each class.

