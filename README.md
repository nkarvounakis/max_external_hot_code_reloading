
# Max External Hot Code Reloading


https://github.com/NickKarvounakis/max_external_hot_code_reloading/assets/41171766/0c9e2b4c-122d-4995-a4f0-921d490a9188



### The Issue
  Externals in Max, face a limitation: Max locks them upon loading, preventing modifications unless the entire Max environment is reloaded.

For small externals  this is most of the time fine as the time spent reloading is often times not all that significant.
This becomes a major hindrance for larger, more dynamic projects where the time spent reloading significantly increases.

### The Solution
This example achieves the desired behaviour by instead of compiling the external monolithically, splitting it's functionality into two translation units.

* Max Platform Layer (max_simplereload.cpp):
  The .mxe64 external to be read and locked by Max.
  Uses only Max platform code for OS independence.
* Platform Independent Layer (simplereload.cpp):
  the .dll to be read by the external.
  Holds platform-independent code, freely modifiable without Max interference.

#### Key Mechanisms
* Memory Reservation: The Max platform layer reserves memory in its object structure and passes necessary data to the platform-independent layer.
* Unloading and Swapping: When platform-independent code re-compiles, the .dll is unloaded, rebuilt, and reloaded, seamlessly replacing the previous code with the new version while using the same reserved memory.

##### Caveats
  * Max Object Class Changes: If the Max object class itself undergoes structural changes (added or removed members), the memory layout might get affected, requiring a Max restart.
  * Static: You can't use static variables globally or in functions in the platform-independent file
  * Function Pointers: You can't use function pointers to directly reference functions defined in DLL. The Whole referencing has to happen through GetProcAddress  (as demonstrated in the example), and never directly.


#### Benefits and Versatility:
  * Instantaneous Recompiling: Changes in the platform-independent code take effect immediately without requiring Max restarts.
  * Real-Time Algorithm Modification: This enables experimentation and fine-tuning algorithms in real time, much like in a scripting languages.
  * Platform Independence: Multiple operating systems support with appropriate os-specific implementations of the reloading mechanism.
  * Code Reusability: By creating other platform-specific versions (ex: juce_simplereload.cpp), the core platform-independent code becomes reusable across multiple platforms.

## Development Setup Instructions
### 1. Installing the Required Tools (MSVC & Windows SDK)

In order to work with the codebase, you'll need the [Microsoft C/C++ Build Tools
v15 (2017) or later](https://aka.ms/vs/17/release/vs_BuildTools.exe), for both
the Windows SDK and the MSVC compiler and linker.

You could use OSX too, but you'll have to implement the OSX layer / build script yourself.

Before Compiling, you have to set the following variables in the build.bat file
a) the max SDK path
```
  set MaxSDKPath=D:\patches\max-sdk
```
b) the path that the external will be compiled to (default being the build folder in the root directory of this project)
```
  set OutputPath=./
  or
  set OutputPath=D:\patches\Externals\SimpleReload
```

### 2. Build Environment Setup

Building the codebase can be done in a terminal which is equipped with the
ability to call either MSVC  from command line.

This is generally done by calling `vcvarsall.bat x64`, which is included in the
Microsoft C/C++ Build Tools. This script is automatically called by the `x64
Native Tools Command Prompt for VS <year>` variant of the vanilla `cmd.exe`. If you've installed the build tools, this command prompt may be easily located by
searching for `Native` from the Windows Start Menu search.


You can ensure that the MSVC compiler is accessible from your command line by
running:

```
cl
```

If everything is set up correctly, you should have output very similar to the
following:

```
Microsoft (R) C/C++ Optimizing Compiler Version 19.29.30151 for x64
Copyright (C) Microsoft Corporation.  All rights reserved.

usage: cl [ option... ] filename... [ /link linkoption... ]
```

### 3. Building
Within this terminal, `cd` to the root directory of the codebase, and just run
the `build.bat` script:

```
build
```

You should see the following output:

```
max_simplereload.cpp
   Creating library max_simplereload.lib and object max_simplereload.exp
simplereload.cpp
   Creating library simplereload.lib and object simplereload.exp
Compilation Completed
```

If everything worked correctly, there will be a `build` folder in the root
level of the codebase, and will contain the output files `simplereload.mxe64` `simplereload.dll` `simplereload_temp.dll` (unless you have specified another output path). 

These three files have to be visible by max and in the same path at all times for the external to work. Any changes to simplereload.cpp should be instantly reflected in the running external (upon re-compiling).
