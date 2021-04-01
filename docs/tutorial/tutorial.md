# SACK - System Abstraction Component Kit

These tutorials should help get you up to speed to be able to explore and 
use SACK.  Even though it's a large collection of code, it's still mostly
just components that can be used alone or incombination with other components.

## What is the abstraction?

The system that this presents to the developer.  This list is approximately the layers of the code, and items later in the list often depend on one or more of those above them.  

 - Has a memory allocation system which allows sharing while still maintaining ownership of objects.  Heap allocation MAY be shared between proceses, if they are able to map the same memory to the same physical location.  
 - Standard container classes or `PLIST`, `PDATALIST`, `PLINKSTACK`, `PDATASTACK`, `PLINKQUEUE`, and `PDATAQUEUE`; which behave as dynamically expanding containers of other values.  Those that have `DATA` in their name store a slab of structures packed as an 
 array of those structures.  (BinaryTree, GenericSets, ...)
 - `PTEXT` and `PVARTEXT` for string and token handling.  The library is internally meant to be utf8; some support for Windows UNICODE compilation was done, and some still exists for file names which allows longer pathnames.  `TEXTRUNE` and UTF8 conversion utiltiies.
 - Threads are explicitly available, and the extended container types provided are thread aware by default.
 - The system is interrupt driven.  This is more colloquially called 'event driven', that when events happen, the appropriate code is invoked as directly as possible in reponse to the triggering event.  Interrupts imply that the current threads process might be suspended to do alternate work, but rather the task's own thread is used leaving the current one free to execute. Creation of threads has had many different interfaces for various sytems and compiler platforms; this simplifies to a simple `ThreadTo( proc, userData)`. 
 - Timers are scheduled via a single background thread to dispatch events; timers measure how much time was spent in the time handler so that the next tick can be scheduled close to the expected tick, from the start of the previous dispatched event.
 - Networking is non-Berkley.  Networking is a good example of the above event driven code, when sockets are created, the obligatory poll for events is done in background threads, and completed packets are dispatched to the users code without requing back to the main thread; allowing the network packet to be processed out of band of the main thread.
 - The 'main thread' is the thread that runs the program entry point, `main()` in C, for example.  
 - Programs have the ability to specify initializers which run before main().  These initializers may be scheduled with a priority to make sure some code runs before some other code, regardless of link order.  With modern languages, including the favorite C++, program initization code is easier to have distributed; but there's potential that one class may expect another to have already been initialized, and would have to take special care itself to wait.
 - Images - High performance images by virtue of not having a variety of supported formats.  There is only 32 bit colors with 8 bits per channel for a Red, Blue, Green and Alpha Channel.  There are no 4 or 15 bit modes.
 - Rendering Images - If you have images, you need a way to show them, and showing them is just giving them a window.
 - GUI/Control library - a whole slew of standard UI controls one might expect, buttons, lists, text, etc.
 - Dynamic loading - one size fits all get procedure from a library.
 - Dynamic code discovery - Functions may be registered into a tree of available methods.   Interfaces, or structures of function pointers can also be registered; this is how display
 and image interfaces are handled.  The image and render modules register an interface structure filled with their own methods.
 - File Systems aren't that different between systems; and really this was to provide common interfaces for things like directory scanning (`opendir()` or `findfirst()` or ...).  A 'recent' addition is abstract file systems.  File system interfaces can be registered for file system providers; these file system interfaces can be mounted in a heiarchy along with the existing native file interface; allowing for softare flie drivers which can be dynamically loaded and used.
 - COM Ports - low level devices are often accessed very differently between different operating sytems.
 - SQL Abstraction - Layers on ODBC and Sqlite native interafce to make a simple query interface for issuing SQL Commands.  Internally adds a stack of connections (where required) in order to handle nested queries; or `for( "Select * from table" ) {  for( "select * from otherTable where valuefrom first table..." )   }` sort of thing.  Automatically handles disconnects and reconnects.
 
 

### Layers on Layers...

In the beginning, in the year 1992, there was a `NanoRTOS` I wrote for DOS, which loaded `.exe` files using command.com executive services to read the data.  A system of dynamic linking was done that allowed multple executables to be loaded dynamically; but being on DOS there was no memory protection, so each process was essentially a thread.  Functions and drivers could be registered in the system and requested dynamically later.  One of the core features that enabled this project to work was a custom memory allocator.  The memory allocator had features like reference counting with `Hold( memory_pointer )` which for every extra hold, an extra `Release( memory_pointer )` would have to be called to release the memory, this allowed processes to pass memory directly to other processes, expecting that the other processes would comsume and `Release()` it, but still maintain a reference to it for its own purposes.  Memory is of course merged as free spaces are found to be near each other, and memory free blocks were sorted according to physical address to keep heap growth down, and generally fragment free.

This also resulted with a `GetSizeOfMemBlock( pointer )` sort of function which could return the size of a block allocated;  in practice this feature is rarely useful, and is better tracked with your own 'available' and 'used' sort of counters.

Experimentally, I added a `Defragment(&pointer)` which could find a 'better' place for the memory block and move it there, a realloc without resizing.

Threads in `NanoRTOS` were created using `fork()` which split the current stack and returned a different value for each version; much like posix `fork()` but didn't create a process.  Threads were also cooperative, such that when they didn't have any work to do they could `Relinquish()` which is the equivalent of `sched_yield()` or `Sleep(0)` in Linux and Windows.

'Some Assembly Required' would be a huge sticker on the side of `NanoRTOS`, and assembly is very unportable.  So wanting to continue developing on this system, I implemented the shared memory allocator, which was previously able to allocate for multiple proesses.  I implemented the ability to share a heap between multiple processes, and even persist the heap directly on disk to be reloaded as a permenant program state.

### Side Projects

As s first significant development for SACK as a library vs just a few files copied as boiler plate, I developed '(Dekware)[http://dekware.sf.net]'.  Dekware is essentially an extendable interactive terminal scripting language.  The language was used to make Dekware into a MUD client, although the intent is for Dekware to be a MUD itself.  Text processing was intrinsic to a MUD client; having the ability to parse and react to phrases and variable content is a handy feature.  'You discvoer *so and so*'s hand in your pocket' : Auto respond, `shout "*so and so* is a bloody theif!"; charge *so-and-so*'.

Taking apart and reassembling text for display especially when handling positional ANSI terminal escape seqeuences become an important bottleneck to optimize.  Text segments are single allocations for the string content and string information.  Text sgemtns may be linked into a list, with indirect text segments able to refer to a 'phrase' or another linked list of text segments.

Plugins, and method registration was developed for this particular project.

Image and Display hooks started to become a thing, because there was no standard avaiable between all platforms.  (Mind you this is in the mid to late 90's when there was very little internet).

## How To dotdotdot

What tutorials?

### First a big DO NOT

Please do not attempt to 'make install' to the root of a posix-like system; this is NOT one of your standard stock system dependant libraries.   If there was found to be more than a handful of users of this, it could be refined to install on a linux system as a shared dependancy; but that doesn't help for windows where installing 'system shared dependancies' isn't as simple.

### Build

Building is scattered all over.  The current iteration uses CMake; which is configured with a variety of options.  This is one of the few projects where `cmake-gui` or `ccmake` might be useful for configuration for user preferences.  There are a lot of simple toggles already defined.

### Use SACK with another project

I just use cmake's `add_subdirectory()` command, and include sack as a project for another library.  You can specify a custom name for out-of-source projects like `add_subdirectory( ../../sack sack )` where the second parameter is the name to use in the 'build' product directory cmake uses.

Since this has its own idea of what 'systems' are, and the 'install' features of existing operating systems is so varied, this is not configured to install as a system-wide library

### Use SACK in sources

SACK implements a file called `stdhdrs.h` that, when included, includes most common headers that one uses for a program with simple file and user IO.  Depending on the system, various other headers are included.   This also includes the basic SACK types and functionality.

```
#include <stdhdrs.h>
```

### Allocate and Release Memory

For example purposes, this will refer to allocating a structure; any 'type' may be allocated.

Any memory allocated will remain allocated until released.  It's a good rule that for every allocation you do, you should make sure there's a way to deallocate the said thing.

When compiled in 'debug' mode, memory allocation functions pass extra parameters that are the source file and line doing the allocation.  Blocks that have been allocated are marked with what code allocated them, and what code released them the last time (until multiple free blocks get aggregated, which loses some information).  The structure of memory blocks is also checked to make sure that blocks have not overflowed boundaries, or maybe updated a released block after it was released.  Also, in `debug` enabled modes, blocks are deliberately set as non-zero when allocated, and cleared to a known constant when released; the update on release allows checking for modifications after a release.  Memory allocation stats may also be queried, the count and size of free and used blocks can be queried.

Blocks have a reference count, so double-releasing memroy is caught as an error (and ignored in release mode).  Because `Hold()` may increment the use counter of a block, multiple `Release()` calls may be required to actually de-allocate the memory.  `Release()` does return `NULL` when the block is finally freed, and non-NULL if there is an outstanding `Hold()` on the block.


``` C
// also refered to as 'thing'
struct typeToAllocate {
    int data; // some sort of structure to allocate
};
```

``` C
// allocate one thing
struct typeToAllocate* p = Allocate( sizeof(*p) );

// alternate allocation method
struct typeToAllocate* q = New( struct typeToAllocate );

// allocate an array of things
struct typeToAllocate* r = NewArray( struct typeToAllocate, count ); 

// the above are similar to using malloc()...
//struct typeToAllocate* r = malloc( sizeof(*s)) ;

```

### Deallocate or Release

When you are done with a block of memory, it's good to return it to the pool for other code to use later...

Microsoft COM headers use 'Release' in a way that conflicts with actually using 'Release'.  There is an alternative `Dallocate()`, which takes the type of the block to release; which enforcs that the thing you're freeing is the type you think.

<spoiler> This was also considered to implement as a compliment to `New( type)` where `Deallocate( type, p)` could call a dynamically registered initializer and finalizer based on the type name specified.</spoiler>

``` C
// allocate one thing
Release( p );
Deallocate( struct typetoAllocate, p );

```

###