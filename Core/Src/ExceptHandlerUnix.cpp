#include "CorePrivate.h"
#include <signal.h>
#include "ExceptInfoLinux.h"
#include "ExceptDumpers.h"


/* NOTES:
	- under cygwin we can use C++ "throw" from signal handler; under linux - can't; so - use setjmp/longjmp calls ...
	- when SIGABRT hooked, SIGSEGV (and others) will cause infinite loop with exceptions => stack overflow
	  (program will try to exit using abort() => raise(SIGABRT))
   CYGWIN NOTES:
	- cygwin headers have siginfo_t declarations, but support of this call is not implemented (when testing, received NULL
	  instead of siginfo_t* ...)
	- cygwin have no sigaltstack(), so we cannot catch stack overflow - app will exit with creation of empty "crash.log"
	- BUG: when GPF raised->hooked->ignored (inside TRY {exception} CATCH {do-nothing}), application stops reaction on Ctrl-C
	  (under linux - ok) - possibly bug around SetConsoleCtrlHandler() (not verified)
*/


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
	// at first: re-enable all signals
	sigset_t mask;
	sigemptyset (&mask);
	sigprocmask (SIG_SETMASK, &mask, NULL);

#if SETJMP_GUARD
	// check for disabled exceptions, THROW_AGAIN ...
	if (GNumDisabledContexts)
		THROW_AGAIN;
#endif

	//!! check for recurse, abort()

	//?? is it possible to detect stack overflow? (simple SIGSEGV, same sc_trapno  ...)
	const char *str = "(unknown signal)";
	bool dump = false;
	// when "dump == false", error context does not matters
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
//	case SIGABRT:
//		str = "Abort Request (SIGABRT)";
//		break;
	case SIGTRAP:
		str = "Illegal Instruction (SIGTRAP)";
		dump = true;
		break;
	case SIGSYS:
		str = "Illegal Syscall (SIGSYS)";
		dump = true;
		break;
	}
	// initiate error + log
#if __linux__
	TString<1024> Message;
	Message.sprintf ("%s in \"%s\"", str, appSymbolName (ctx.sc_eip));
	COutputDevice *Out = appBeginError (Message);
	if (dump)
	{
		// dump SIGCONTEXT
//		Out->Printf ("TrapNo: %d\n\n", ctx.sc_trapno);
		DumpRegs (Out, &ctx);
		Out->Printf ("  EFLAGS: %08X\n", ctx.sc_eflags);
		Out->Printf ("\nStack:\n");
		DumpMem (Out, (unsigned*) ctx.sc_esp, &ctx);
		Out->Printf ("\n");
#if UNWIND_EBP_FRAMES
		Out->Printf ("\nCall stack trace:\n");
		UnwindEbpFrame (Out, (unsigned*) ctx.sc_ebp);
#endif
	}
#else // __linux__
	// no context info - simply print error message
	appBeginError (str);
#endif // __linux__

	THROW_AGAIN;
}


#if __linux__ // have signal context

static void HookSignal (int signum, void (*handler)(int, SIGCONTEXT))
{
	// using cast to sighandler_t, because under linux out functions have additional argument: "SIGCONTEXT ctx"
	struct sigaction sa;
	sigemptyset (&sa.sa_mask);
	sa.sa_handler = (sighandler_t)handler;
	sa.sa_flags   = SA_ONSTACK;
	sigaction (signum, &sa, NULL);
}

#else

static void HookSignal (int signum, void (*handler)(int))
{
	struct sigaction sa;
	sigemptyset (&sa.sa_mask);
	sa.sa_handler = handler;
	sa.sa_flags   = 0;
	sigaction (signum, &sa, NULL);
}


#endif


void appHookExceptions ()
{
#if __linux__
	// to catch stack overflow we require to setup alternative stack for signal handlers
	static int altstack[4096];
	stack_t ss;
	ss.ss_sp    = &altstack;
	ss.ss_size  = sizeof(altstack);
	ss.ss_flags = 0;
	if (sigaltstack (&ss, NULL))
		appPrintf ("sigaltstack() failed\n");
#endif
	// hook ctrl-c (SIGINT)
	HookSignal (SIGINT, handle_ctrlc);
	// hook other signals
	static short signals[] = {
		SIGTERM, SIGHUP, SIGFPE, SIGILL,
		SIGSEGV, /*SIGABRT,*/ SIGTRAP, SIGSYS
	};
	for (int i = 0; i < ARRAY_COUNT(signals); i++)
		HookSignal (signals[i], handle_signal);
}
