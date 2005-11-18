#if __UNIX__
#	define CONTEXT		SIGCONTEXT
#endif


//#define LOG_FUNCS_ONLY		1
#define LOG_STRINGS				1
#define UNWIND_EBP_FRAMES		1

#define STACK_UNWIND_DEPTH		80


void DumpRegs (COutputDevice *Out, CONTEXT *ctx);
void DumpMem (COutputDevice *Out, const unsigned *data, CONTEXT *ctx);
#if UNWIND_EBP_FRAMES
void UnwindEbpFrame (COutputDevice *Out, const unsigned *data);
#endif
