#include "CorePrivate.h"
#include <signal.h>

#if __linux__
#include "ExceptInfoLinux.h"
#endif

/* BUG:	when use following before MainLoop:
		TRY {
			*((int*)NULL)=1;
		} CATCH {
		} END_CATCH
	app will ignore Ctrl-C (may be, this is because Ctrl-C will be disabled in SIGSEGV handler, which is not finished?)
	1sr solution: avoid such constructions under Unix; but: this construction is required for some error recovery:
		Memory.cpp::BuildAllocatorsTable() etc
	2nd solution: use sigsetjmp()/siglongjmp(); check effectiveness under Linux ... (same code for sig/non-sig funcs ?)
*/

// NOTE: under cygwin we can "throw" from signal handler; under linux - can't; so - use setjmp/longjmp calls ...

//!! ERRORS: "error -stack" will not be handled under cygwin (linux untested)
//!!		 when SIGABRT hooked, SIGSEGV (and others) will cause infinite loop with exceptions => stack overflow

#if __CYGWIN__
#define sighandler_t	void(*)(int)
#endif

#if __linux__
static void handle_ctrlc (int signum, SIGCONTEXT ctx)
#else
static void handle_ctrlc (int signum)
#endif
{
	if (GIsRequestingExit)			// exit requested, but not yet executed (application is hang?)
	{
		//!! should dump context here (context of hang app)
		appError ("Ctrl-C");
	}
	appWPrintf ("Ctrl-C has been pressed\n");
	GIsRequestingExit = true;
}


#if __linux__
static void handle_signal (int signum, SIGCONTEXT ctx)
#else
static void handle_signal (int signum)
#endif
{
	const char *str = "(unknown signal)";
	bool dump = false;
	switch (signum)
	{
	case SIGTERM:
		str = "Termination Request (SIGTERM)";
		break;
	case SIGHUP:
		str = "Terminal Hangup (SIGHUP)";
		break;
	case SIGFPE:
		str = "FPU Exception (SIGFPE)";
		dump = true;
		break;
	case SIGILL:
		str = "Illegal Instruction (SIGILL)";
		dump = true;
		break;
	case SIGSEGV:
		str = "General Protection Fault (SIGSEGV)";
		dump = true;
		break;
/*	case SIGABRT:
		str = "Abort Request (SIGABRT)";
		break; */
	case SIGTRAP:
		str = "Illegal Instruction (SIGTRAP)";
		dump = true;
		break;
	case SIGSYS:
		str = "Illegal Syscall (SIGSYS)";
		dump = true;
		break;
	}
#if __linux__
	if (dump)
	{
		//!! dump CONTEXT
		appPrintf ("EXCEPTION: %08X\n", ctx.sc_eip);
	}
#endif
	appError (str);
}


void appHookExceptions ()
{
	// using cast to sighandler_t, because under linux out functions have additional argument: "SIGCONTEXT ctx"
	// hook ctrl-c (SIGINT)
	struct sigaction sa;
	sigemptyset (&sa.sa_mask);
	sa.sa_handler = (sighandler_t)handle_ctrlc;
	sa.sa_flags   = 0;
	sigaction (SIGINT, &sa, NULL);
	// hook other signals
	static short signals[] = {
		SIGTERM, SIGHUP, SIGFPE, SIGILL,
		SIGSEGV, /*SIGABRT,*/ SIGTRAP, SIGSYS
	};
	for (int i = 0; i < ARRAY_COUNT(signals); i++)
	{
		sigemptyset (&sa.sa_mask);
		sa.sa_handler = (sighandler_t)handle_signal;
		sa.sa_flags   = 0;
		sigaction (signals[i], &sa, NULL);
	}
}
