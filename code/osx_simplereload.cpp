#include "simplereload_platform.h"

struct osx_dll_data {
  dll_callbacks callbacks;
};

static osx_dll_data OSXDLLData = {};

RELOAD_DLL(OSXCheckAndReloadDynamicCode) {
  return 0;
}

static void OSXBuildDLLPaths(void *ModuleRef) {
  
}