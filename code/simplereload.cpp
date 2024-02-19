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

static
double fold(double v, double lo1, double hi1){
	double lo;
	double hi;
	if(lo1 == hi1){ return lo1; }
	if (lo1 > hi1) {
		hi = lo1; lo = hi1;
	} else {
		lo = lo1; hi = hi1;
	}
	double range = hi - lo;
	long numWraps = 0;
	if(v >= hi){
		v -= range;
		if(v >= hi){
			numWraps = (long)((v - lo)/range);
			v -= range * (double)numWraps;
		}
		numWraps++;
	} else if(v < lo){
		v += range;
		if(v < lo){
			numWraps = (long)((v - lo)/range) - 1;
			v -= range * (double)numWraps;
		}
		numWraps--;
	}
	if(numWraps & 1) v = hi + lo - v;
	return v;
}

DLLEXPORT
DSP_ROUTINE(DspPerform) {
	double *In1 = Inputs[0];
	double *In2 = Inputs[1];
	double *Out0 = Outputs[0];
	double *Out1 = Outputs[1];
	double *Out2 = Outputs[2];
	while(SampleFrames--) {

		double newRampV = 0;
		double CounterV = 0;
		if(AscendingRampWrapped(*In1, SimpleReloadState->PrevSample)) {
			SimpleReloadState->Counter = SimpleReloadState->Counter + 1;
		}
		if(In2 && *In2) {
			ResetCounterSignal(SimpleReloadState);
		}
		double phaseInp = *In1;
		double SOut = 0;

#define TEST 0
#if TEST == 0
		newRampV = cos(phaseInp * TWOPI);
		SOut = cos(phaseInp * 200 * TWOPI);
#else
		newRampV = fold(phaseInp * 4, -1, 1);
		SOut = fold(phaseInp * 250 * 4, -1, 1);

#endif
		double modz = 1.0;
		CounterV = SimpleReloadState->Counter;

		SimpleReloadState->PrevSample = *In1;

		In1++; In2++;
		*Out0++ = newRampV;
		*Out1++ = SOut;
		*Out2++ = CounterV;



	}
}