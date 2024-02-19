@echo off
setlocal
cd /D "%~dp0"

:: Required Paths
:: Relative to /build in case of relative paths
set MaxSDKPath=..\..\max-sdk
set OutputPath=..\

:: Include Paths for Headers
set IncludePaths=/I %MaxSDKPath%\source\max-sdk-base\c74support\max-includes  
set IncludePaths=%IncludePaths% /I %MaxSDKPath%\source\max-sdk-base\c74support\msp-includes


:: Compiler Switches
set CommonCompilerFlags=-nologo /DEBUG /Zi /MP -Od /Ob0
set CommonCompilerFlags=%CommonCompilerFlags% -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -FC  /EHsc
set CommonCompilerFlags=%CommonCompilerFlags% %IncludePaths%
set MaxPlatformCompilerFlags=%CommonCompilerFlags% -DWIN_VERSION -DWIN_EXT_VERSION -D_CRT_SECURE_NO_DEPRECATE -DMAXAPI_USE_MSCRT

:: Linker Switches
set CommonLinkerFlags=-incremental:no -opt:ref /DEBUG
set MaxPlatformLinkerFlags=%CommonLinkerFlags%  %MaxSDKPath%\source\max-sdk-base\c74support\max-includes\x64\MaxAPI.lib
set MaxPlatformLinkerFlags=%MaxPlatformLinkerFlags% %MaxSDKPath%\source\max-sdk-base\c74support\msp-includes\x64\MaxAudio.lib
set MaxPlatformLinkerFlags=%MaxPlatformLinkerFlags% %MaxSDKPath%\source\max-sdk-base\c74support\jit-includes\x64\jitlib.lib

:: DLL Linker Switches
::  __declspec(dllexport) could also be used at the function declaration stage instead of specifying the exports at here but it has several drawbacks.
::  The attribute is not standardized, so different operating systems (and even different compilers!) have their own designations. 
::  Using a specific one inside the code file, I'd be tying myself to the Microsoft's compiler,  and that's not necessarily something that I'd like to do.
set dll_link=               /EXPORT:ResetCounterSignal
set dll_link=%dll_link%     /EXPORT:DspPerform

IF NOT EXIST build mkdir build
pushd build

:: Microsoft locks the pdb (sigh), we'll have to create a new one every single time if we want debuggers like visual studio to properly work
del *.pdb > NUL 2> NUL
echo WAITING FOR PDB > lock.tmp
set  RandomPDBName=simplereload_%random%.pdb

:: Max Translation Unit
cl %MaxPlatformCompilerFlags%  /LD ..\code\max_simplereload.cpp -Fmsimplereload.map /Fe: %OutputPath%simplereload.mxe64 /link  %MaxPlatformLinkerFlags% /IMPLIB:max_simplereload.lib -PDB:max_simplereload.pdb

:: Platform Independent Translation Unit
cl %CommonCompilerFlags%  /LD ..\code\simplereload.cpp    -Fmsimplereload_dll.map  /Fe: %OutputPath%simplereload.dll /link %CommonLinkerFlags%  /IMPLIB:simplereload.lib  -PDB:%RandomPDBName% %dll_link%

echo Compilation Completed

del lock.tmp

popd
