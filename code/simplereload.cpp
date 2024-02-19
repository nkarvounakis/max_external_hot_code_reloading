#include <stdio.h>
#include <math.h>
#include "simplereload_platform.h"

// Platform / OS Independent Code

#if defined __cplusplus
	// Disables C++ Name Mangling
	#define DLLEXPORT extern "C"
#else
	#define DLLEXPORT
#endif


static int
AscendingRampWrapped(double Ramp, double PrevSample) {
	return (Ramp - PrevSample < 0.0) ? 1 : 0;
}

DLLEXPORT
RESET_COUNTER(ResetCounterSignal) {
	SimpleReloadState->Counter = 0;
}

DLLEXPORT
DSP_ROUTINE(DspPerform) {
	double *In1 = Inputs[0];
	double *In2 = Inputs[1];
	double *Out0 = Outputs[0];
	double *Out1 = Outputs[1];
	while(SampleFrames--) {
		double newRampV = 0;
		double CounterV = 0;
		if(AscendingRampWrapped(*In1, SimpleReloadState->PrevSample)) {
			SimpleReloadState->Counter = SimpleReloadState->Counter + 1;
		}
		if(In2 && *In2) {
			ResetCounterSignal(SimpleReloadState);
		}

		newRampV = cos(*In1 * TWOPI);
		CounterV = SimpleReloadState->Counter;

		SimpleReloadState->PrevSample = *In1;

		In1++; In2++;
		*Out0++ = newRampV;
		*Out1++ = CounterV;
	}
}