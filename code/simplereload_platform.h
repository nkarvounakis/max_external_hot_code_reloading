#if !defined(DSP_PLATFORM_H)

struct simplereload_state {
	double PrevSample;
	double Counter;
};

#define PI 3.14159265358979323846
#define TWOPI 6.28318530717958647692

#define DSP_ROUTINE(name) void name(simplereload_state* SimpleReloadState, double **Inputs, double **Outputs, long SampleFrames)
typedef DSP_ROUTINE(custom_dsp_perform);

#define RESET_COUNTER(name) void name(simplereload_state* SimpleReloadState)
typedef RESET_COUNTER(reset_counter);

#define RELOAD_DLL(name) static int name()

struct dll_callbacks {
	custom_dsp_perform *ObjectDspRoutine;
	reset_counter *ResetCounterRoutine;
};

#define DSP_PLATFORM_H
#endif