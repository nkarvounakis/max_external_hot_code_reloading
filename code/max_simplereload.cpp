#include "ext.h"
#include "ext_obex.h"
#include "ext_systhread.h"
#include "z_dsp.h"
#include "simplereload_platform.h"

#if defined(_WIN32)
	#include "win32_simplereload.cpp"
	#define CallbackRef &Win32DLLData.callbacks
	#define CheckAndReloadDynamicCode Win32CheckAndReloadDynamicCode
	#define BuildDLLPaths(X) Win32BuildDLLFilePaths((HMODULE)X)
#elif defined(__APPLE__)
	#include "osx_simplereload.cpp"
	#define CallbackRef &OSXDLLData.callbacks
	#define CheckAndReloadDynamicCode OSXCheckAndReloadDynamicCode
	#define BuildDLLPaths(X) OSXBuildDLLPaths(X)
#endif


struct t_simplereload {
	t_pxobject	ob;			// the object itself (must be first)
	dll_callbacks *Callbacks;
	simplereload_state State;


	t_systhread		x_systhread;						// thread reference
	int				x_systhread_cancel;					// thread cancel flag
	void				*x_qelem;							// for message passing between threads
	int				x_sleeptime;						// how many milliseconds to sleep
};

static t_class *simplereload_class;

static void 
StopDLLObserverThread(t_simplereload *x)
{
	unsigned int ret;

	if (x->x_systhread) {
		post("stopping our thread");
		x->x_systhread_cancel = true;						// tell the thread to stop
		systhread_join(x->x_systhread, &ret);					// wait for the thread to stop
		x->x_systhread = NULL;
	}
}

static void 
DLLObserverThreadSleep(t_simplereload *x, long sleeptime)
{
	if (sleeptime<10)
		sleeptime = 10;
	x->x_sleeptime = sleeptime;														// no need to lock since we are readonly in worker thread
}


static int 
DLLObserverThread(t_simplereload *x)
{
	for (;;) {
		if (x->x_systhread_cancel) break;
		
		int ret = CheckAndReloadDynamicCode();
		// post("%d", ret);
		if(ret) post("Reloading DLL");


		qelem_set(x->x_qelem);
		systhread_sleep(x->x_sleeptime);
	}

	x->x_systhread_cancel = false;					
	systhread_exit(0);															
	return 0;
}

static void simplethread_qfn(t_simplereload *x){}

static void 
CreateDLLObserverThread(t_simplereload *x)
{
	post("task!\n");
	StopDLLObserverThread(x);
	if(!x->x_systhread) {
		post("starting a new thread");
		systhread_create((method) DLLObserverThread, x, 0, 0, 0, &x->x_systhread);
	}

}

static int
AscendingRampWrapped(double Ramp, double PrevSample) {
	return (Ramp - PrevSample < 0.0) ? 1 : 0;
}


static void 
MaxDSP64Perform(t_simplereload *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)
{
	x->Callbacks->ObjectDspRoutine(&x->State, ins, outs, sampleframes);
}

// registers a function for the signal chain in Max
static void 
MaxDSP64Callback(t_simplereload *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
	object_method(dsp64, gensym("dsp_add64"), x, MaxDSP64Perform, 0, 0);
}

static void 
MaxFreeClassCallback(t_simplereload *x)
{
	StopDLLObserverThread(x);
	qelem_free(x->x_qelem);
	z_dsp_free(&x->ob);
}

static void 
MaxBangCallback(t_simplereload *x) {
	x->Callbacks->ResetCounterRoutine(&x->State);
}

static void*
MaxInitClassCallback(t_symbol *s, long argc, t_atom *argv)
{
	t_simplereload *x = 0;
 	x  = (t_simplereload *)object_alloc(simplereload_class);
	if (x) {
		x->Callbacks = CallbackRef;
		x->State.PrevSample = 0;
		x->State.Counter = 0;
		x->x_systhread = 0;
		x->x_sleeptime = 50;
		dsp_setup((t_pxobject *)x, 2);
		outlet_new((t_object *)x, "signal");
		outlet_new((t_object *)x, "signal");

		x->x_qelem = qelem_new(x,(method)simplethread_qfn);

		CreateDLLObserverThread(x);
	}
	
	return (x);
}

static void 
MaxAssistCallback(t_simplereload *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_INLET) {
		switch (a) {
			case 0: {sprintf(s, "Unipolar Ramp (Signal)"); break;}
			case 1: {sprintf(s, "Counter Reset Impulse (Signal)"); break;}
		}
	}
	else {
		switch (a) {
			case 0: {sprintf(s, "Cosine Wave (Signal)"); break;}
			case 1: {sprintf(s, "Counter (Signal)"); break;}
		}
	}
}

extern "C" void ext_main(void *moduleRef)
{
	t_class *c;
	BuildDLLPaths(moduleRef);

	c = class_new("simplereload", (method)MaxInitClassCallback, (method)MaxFreeClassCallback, sizeof(t_simplereload), 0, A_GIMME, 0);

	class_addmethod(c, (method)MaxAssistCallback,	"assist",A_CANT, 0);
	class_addmethod(c, (method)MaxBangCallback,	"bang",	 0);
	class_addmethod(c, (method)DLLObserverThreadSleep,	"sleeptime",	A_DEFLONG, 0);
	class_addmethod(c, (method)MaxDSP64Callback,		"dsp64",	A_CANT, 0);

	class_dspinit(c);
	class_register(CLASS_BOX, c); /* CLASS_NOBOX */
	simplereload_class = c;
}