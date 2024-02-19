#include <Windows.h>
#include "simplereload_platform.h"

struct win32_dll_data {
  dll_callbacks callbacks;
  FILETIME LastWriteTime;
  HMODULE DynamicCodeDLL;
};

static char SourceDLLPath[MAX_PATH];
static char TempDLLPath[MAX_PATH];
static win32_dll_data Win32DLLData = {};

static DSP_ROUTINE(DspPerformStub) {}
static RESET_COUNTER(ResetCounterStub) {}

static FILETIME
Win32GetLastWriteTime(char *FileName) {
	FILETIME LastWriteTime = {};

	WIN32_FIND_DATA FindData;
	HANDLE FileHandle = FindFirstFileA(FileName, (LPWIN32_FIND_DATAA)&FindData);
	if(FileHandle != INVALID_HANDLE_VALUE) {
		LastWriteTime = FindData.ftLastWriteTime;
		FindClose(FileHandle);
	} 
	return LastWriteTime;
}

static void 
WIN32UnloadDynamicCode() {
	if(Win32DLLData.DynamicCodeDLL) {
		FreeLibrary(Win32DLLData.DynamicCodeDLL);
		Win32DLLData.DynamicCodeDLL = 0;
	}
	Win32DLLData.callbacks.ObjectDspRoutine = DspPerformStub;
	Win32DLLData.callbacks.ResetCounterRoutine = ResetCounterStub;
}


static void
WIN32LoadDynamicCode() {
		Win32DLLData.LastWriteTime = Win32GetLastWriteTime(SourceDLLPath);
		CopyFileA(SourceDLLPath, TempDLLPath, FALSE);
		HMODULE  DynamicCodeDLL = LoadLibraryA(TempDLLPath);
		if(DynamicCodeDLL) {
			Win32DLLData.DynamicCodeDLL = (DynamicCodeDLL);

			Win32DLLData.callbacks.ObjectDspRoutine = (custom_dsp_perform*)GetProcAddress(DynamicCodeDLL, "DspPerform");
			Win32DLLData.callbacks.ResetCounterRoutine = (reset_counter*)GetProcAddress(DynamicCodeDLL, "ResetCounterSignal");

			if(!Win32DLLData.callbacks.ObjectDspRoutine) Win32DLLData.callbacks.ObjectDspRoutine = DspPerformStub;
		}
}

RELOAD_DLL(Win32CheckAndReloadDynamicCode) {
  FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceDLLPath);

  if(CompareFileTime(&NewDLLWriteTime, &Win32DLLData.LastWriteTime) != 0) {
    WIN32UnloadDynamicCode();
    WIN32LoadDynamicCode();
    return 1;
  }
  return 0;
}

// DLL File Path Building Stuff
static void
CatStrings(size_t SourceACount, char *SourceA,
           size_t SourceBCount, char *SourceB,
           size_t DestCount, char *Dest)
{    
    for(int Index = 0;
        Index < SourceACount;
        ++Index)
    {
        *Dest++ = *SourceA++;
    }

    for(int Index = 0;
        Index < SourceBCount;
        ++Index)
    {
        *Dest++ = *SourceB++;
    }

    *Dest++ = 0;
}

static int
StringLength(char *String)
{
    int Count = 0;
    while(*String++)
    {
        ++Count;
    }
    return(Count);
}


static void
Win32BuildDLLFilePaths(HMODULE ModuleRef) {
	char Mxe64FilePath[MAX_PATH];

	DWORD SizeOfFilename =  GetModuleFileNameA(ModuleRef, Mxe64FilePath, sizeof(Mxe64FilePath));
	char *OnePastLastMxe64FileNameSlash = Mxe64FilePath;
	char *OneBeforeFileExtention = Mxe64FilePath;
	for(char *Scan = Mxe64FilePath; *Scan; ++Scan) {
		if(*Scan == '\\') {
			OnePastLastMxe64FileNameSlash = Scan + 1;
		}
		if(*Scan == '.') {
			OneBeforeFileExtention = Scan - 1;
		}
	}

	char Mxe64Name[MAX_PATH];
	long long ExternalNameSizeWithoutExt = OneBeforeFileExtention - OnePastLastMxe64FileNameSlash;
	char *Out = Mxe64Name;
	char *Scan = OnePastLastMxe64FileNameSlash;
	for(int Index = 0; Index <= ExternalNameSizeWithoutExt; ++Index) *Out++ = *Scan++;

	char DllName[MAX_PATH];
	CatStrings(ExternalNameSizeWithoutExt + 1, Mxe64Name,
						StringLength(".dll"), ".dll",
						sizeof(DllName), DllName);
	CatStrings(OnePastLastMxe64FileNameSlash - Mxe64FilePath, Mxe64FilePath,
							StringLength(DllName), DllName,
							sizeof(SourceDLLPath), SourceDLLPath);

	char TempDllName[MAX_PATH];
	CatStrings(ExternalNameSizeWithoutExt + 1, Mxe64Name,
						StringLength("_temp.dll"), "_temp.dll",
						sizeof(TempDllName), TempDllName);
	CatStrings(OnePastLastMxe64FileNameSlash - Mxe64FilePath, Mxe64FilePath,
							StringLength(TempDllName), TempDllName,
							sizeof(TempDLLPath), TempDLLPath);

	post("DLL Path: %s", SourceDLLPath);
	post("Temp Path: %s", TempDLLPath);
}