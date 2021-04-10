
# Building Revisited

## Prerequsites 

- Git
- A working build system for C/C++
- CMake (also ccmake or other cmake-gui)
- (Optional) ninja for CMake faster builds
- 

- Linux
   - unixodbc-devel - (optional, but requires reconfiguration before doing build)
   - (optional)freetype2-devel
   - (optional)libpng-devel
   - (optional)jpeg-devel
   - mesa-devel
   - wayland-devel
   - X11-devel
   - zlib-devel
   - (optional)OpenSSL/LibreSSL-devel
   - Sqlite-devel
   - OpenAL-devel

### Ubuntu example

``` bash
sudo apt-get update -y
sudo apt-get install cmake
sudo apt-get install -y ninja-build
sudo apt-get install uuid-dev unixodbc-dev
```

and

``` bash
sudo apt-get install -y  libfreetype6-dev libpng-dev zlib1g-dev libjpeg-dev wayland-protocols libwayland-dev libxkbcommon-dev libgl1-mesa-dev libglew-dev libxext-dev
```

or

``` bash
sudo apt-get install -y libfreetype6-dev
sudo apt-get install libpng-dev zlib1g-dev libjpeg-dev
sudo apt-get install -y wayland-protocols
sudo apt-get install -y libwayland-dev
sudo apt-get install -y libxkbcommon-dev
sudo apt-get install -y libgl1-mesa-dev
sudo apt-get install -y libglew-dev
sudo apt-get install -y libxext-dev
```
    
- Windows
   - none 

```
git clone https://github.com/d3x0r/sack
mkdir build
cd build
../
```

### Configure top level options

- Windows

```
cmake-gui .
```

- Linux

```
ccmake .
```

## Wait... what's happening?

At this point, any errors in configuration should be resolved before starting a build.
Based on what options you chose the current structure will look like

```
build/
   debug_solution/
   debug_out/
   <cmake configuration files>
```

Or instead of 'debug' may be 'Debug', 'Release', 'RelWithDebInfo', 'MinRel', which are standard cmake configuration types;  The choice of `CMAKE_BUILD_TYPE` should be presented as a list of options in the configuration utility.  

- `*_solution/`
    - This is where the builds for subprojects within sack are located.
- `*_out/`
    - This is where output from the build's `install` rule are sent.

Within these will be directories with several build targets which may depend on other build targets; they are meant as high level applications which utilize SACK.

# Subprojects

This is a summary of subprojects which get built.

All projects attempt to have a `info` directory which attempts to capture the current build date and information for what particular build this is.

## core
  
  Core is the SACK core system build.  It installes to

  - `*_out/core/CMakePackage` - This is included in other projects to use this build target.  It contains the configuration for where includes, libraries, binaries, etc are located.  Also defines some convenience rules for making libaries and executables with SACK.
  - `*_out/core/include/SACK/`
  - `*_out/core/include/AL/`  (On Windows the OpenAL Headers)
  - `*_out/core/lib/`
    - On Windows  static library targets go here, the export library for Windows DLL linkages.
    - On Linux shared and static libarary targets go here.  To use in place use `LD_LIBRARY_PATH=.../*_out/core/lib`; or from the `bin` directory, just `LD_LIBRARY_PATH=../lib` which may be `../lib64`.
  - `*_out/core/bin/`
    - On Windows, executables, and DLLs, and other dynamic modules go here. 
    - On Linux only executable targets go here; internal utilities to interact with programs built using SACK, or simply small utilities which demonstrate functionality.
  - `*_out/core/resources/`
    - `fonts/`
    - `images/`
  - `*_out/core/share/`
     - on windows, OpenAL targets end up here.
  - `*_out/core/src/`
     - This is additional sources, which may be utliized to link SACK to programs, depending on the target compiler and operating system.

  

## binary

This is built referncing the previously built `*_out/core/CMakePackage` information, and is a bunch of small programs from `sack/src/utils`.  Which are mostly just small command line utilities which may or may not have a use.   They were mostly all useful to me at one point or another; and probably will be again.

The directory structure is similar to the above `core` structure with a `bin`.  

(There are currently some include and lib targets installed from Intershell crossover, which should be opted out.)


## interShell

InterShell is a interchangable shell; it loads a configuration based on the name of the application it is, and allows mocking up quick interfaces.

This is the dynamic application builder for SACK.

(should dig up a link to some high level documatnion on this target.)

This directory structure has a `bin/`, but also should be noted that inside `bin/` is a `plugins/` directory which contains additional InterShell modules which can be loaded.

Additional plugins may be developed and linked using the included `include/` and `lib/` directories; for which there is a `CMakePackage` target in the root.

## (Optional targets)

Many additional target projects are built if the top level is enabled with `BUILD_EXTRAS`.

Also at a top level there is an option for `BUILD_TESTS` which is propagated to other projects, but then each project has a configuration.  The `core` project uses this flag to build many additional small unit-test type appications which are simple demonstrations.  There is no formal framework verifying correctness, other than manual inspection at this time.

By going to `*_solution/extraTarget` and running the cmake configuration utility, and redoing the build.  

