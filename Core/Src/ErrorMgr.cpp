#include "CorePrivate.h"
#include "OutputDeviceFile.h"
#include "OutputDeviceMemTail.h"

//?? add "bool GSilentErrors": when true, appError() etc will not set GErr.swError/GIsFatalError +
//??  appGetErrorLog() will return GNull (incorrect: should avoid ErrLog places at all to allow use of
//??  logging in a case of later error with GSilentErrors==false)
//?? use GSilentErrors=1 with some TRY/CATCH blocks; Win32 analog:
//?? "__try { /*code1*/ } __except(1) { /*code2*/ }" =>
//?? "GSilentErrors=true;TRY { /*code1*/ } CATCH { /*code2*/ } END_CATCH GSilentErrors=false;"


#define CRASH_LOG		"crash.log"

#if 0
// debug SETJMP_GUARD; remove later
#define DBG(msg)		msg
#else
#define DBG(msg)
#endif


bool			GIsFatalError;
CErrorHandler	GErr;
static COutputDeviceMemTail LogTail (1024);


/*-----------------------------------------------------------------------------
	Error log file
-----------------------------------------------------------------------------*/

static COutputDeviceFile ErrLog;
static bool ErrLogOpened;				// cannot use ErrLog.IsOpened(): may be failed to open
static bool IsRecursiveError;


COutputDevice *appBeginError (const char *msg)
{
	DBG(printf("appBeginError() from %s\n", appSymbolName (GET_RETADDR(msg))));
	if (GIsFatalError)
	{
		// recursive error
		ErrLog.Printf ("\n\nRecursive error: %s\n", msg);
		IsRecursiveError = true;
		return GNull;
	}
	GIsFatalError   = true;
	GLogHook        = NULL;
	GErr.Message    = msg;
	GErr.History[0] = 0;
	LogTail.Unregister ();				// freeze log tail

	// start log
	appGetErrorLog ();
	ErrLog.Printf ("-------------------------------------------\n");
	ErrLog.Printf ("%s crash, %s\n", appPackage (), appTimestamp ());
	ErrLog.Printf ("-------------------------------------------\n");
	ErrLog.Printf ("OS:  %s\nCPU: %s\n", GMachineOS, GMachineCPU);
	ErrLog.Printf ("\nERROR: %s\n\n", msg);

	return &ErrLog;
}


// real function is BeginError, but later - get error log; should be a different name!
COutputDevice *appGetErrorLog ()
{
	if (IsRecursiveError)
		return GNull;

	if (!ErrLogOpened)
	{
		// init log
		ErrLogOpened = true;
		ErrLog.Open (CRASH_LOG);
		ErrLog.FlushEveryTime = true;
		ErrLog.NoColors       = false;	// do not modify output
		ErrLog.Printf ("\n\n");
		// detect logging w/o error
		if (!GIsFatalError)
			ErrLog.Printf ("WARNING: log opened without error!\n");
	}
	return &ErrLog;
}


#if DO_GUARD
static void LogHistory (const char *part)
{
	if (GErr.nonFatalError)
	{
		GErr.History += part;
		return;
	}

	if (!ErrLogOpened)
	{
		// fatal error, but log is not started ...
		// this may happen on Win32 when compiled with WIN32_USE_SEH=0
		appGetErrorLog ();
	}
	// HERE: ErrLog is opened
	if (!GErr.History[0])
		ErrLog.Write ("History:\n  ");
	GErr.History += part;
	ErrLog.Write (part);
}
#endif


/*-----------------------------------------------------------------------------
	Error throwing
-----------------------------------------------------------------------------*/

#if SETJMP_GUARD

int GNumDisabledContexts;
static CGuardContext *contexts[512];
static int numContexts = 0;


CGuardContext::CGuardContext (const char *msg)
:	text(msg)
,	jumped(false)
{
	DBG(printf("%d-> %s\n", numContexts, msg));
	// register context
	if (numContexts < ARRAY_COUNT(contexts))
		contexts[numContexts] = this;
	numContexts++;
}


CGuardContext::~CGuardContext ()
{
	if (jumped)
	{
		DBG(printf("*%d\n", numContexts));
		return;
	}
	// unregister context
	numContexts--;
	if (numContexts < GNumDisabledContexts)
	{
		GNumDisabledContexts = 0;
		DBG(printf("...enable:%d\n", numContexts));
	}
	DBG(printf("...(%d) %s\n", numContexts, text));
}


CGuardContext2::CGuardContext2 ()
:	CGuardContext (NULL)
{
	DBG(printf("...disable:%d(%d)\n", numContexts-1, GNumDisabledContexts));
	if (!GNumDisabledContexts)
		GNumDisabledContexts = numContexts;
}


void appThrowException ()
{
	if (!numContexts)
	{
		// no guard/unguard or TRY/CATCH blocks
		appGetErrorLog ();			// force open
		ErrLog.Printf ("\n\nUncatched appThrowException()\n");
		abort ();
	}
	// check for stack overflow
	if (numContexts > ARRAY_COUNT(contexts))
	{
		//?? may be, add to log "... (recurse)"
		GNumDisabledContexts = 0;					// just in case; seems, stack overflow ... (always error)
		numContexts = ARRAY_COUNT(contexts);
	}
	DBG(printf ("#%d  %s\n", numContexts-1, contexts[numContexts-1]->text));
	// remove context
	CGuardContext *ctx = contexts[numContexts-1];
	numContexts--;
	if (numContexts < GNumDisabledContexts)
	{
		DBG(printf("...enable:%d\n", numContexts));
		GNumDisabledContexts = 0;
	}
	ctx->jumped = true;
	longjmp (ctx->jmp, 1);
}

#endif


void appFatalError (const char *fmt, ...)
{
	char buf[1024];
	va_list argptr;
	va_start (argptr, fmt);
	vsnprintf (ARRAY_ARG(buf), fmt, argptr);
	va_end (argptr);

//??	if (debugLog) debugLog->Printf ("FATAL ERROR: %s\n", buf);
	// initiate error + open log
	appBeginError (buf);
	if (!IsRecursiveError)
		GErr.swError = true;

	THROW;
}


void appNonFatalError (const char *fmt, ...)
{
	GErr.swError       = true;
	GErr.nonFatalError = true;

	GLogHook = NULL;

	if (fmt)
	{
		va_list argptr;
		va_start (argptr, fmt);
		vsnprintf (ARRAY_ARG(GErr.Message), fmt, argptr);
		va_end (argptr);
		appPrintf (S_RED"ERROR: %s\n", *GErr.Message);
	}
	else
		GErr.Message[0] = 0;	// no message at all

	THROW;
}


#if DO_GUARD

void appUnwindPrefix (const char *fmt)
{
	TString<512> Buf;
	Buf.sprintf (GErr.wasError ? " <- %s:" : "%s:", fmt);
	LogHistory (Buf);
	GErr.wasError = false;		// will not insert "<-" next appUnwindThrow()
}


void appUnwindThrow (const char *fmt, ...)
{
	char	buf[512];
	va_list	argptr;

	va_start (argptr, fmt);
	if (GErr.wasError)
	{
		*((unsigned*)buf) = BYTES4(' ','<','-',' ');
//		buf[0] = ' '; buf[1] = '<'; buf[2] = '-'; buf[3] = ' ';
		vsnprintf (buf+4, sizeof(buf)-4, fmt, argptr);
	}
	else
		vsnprintf (buf, sizeof(buf), fmt, argptr);
	va_end (argptr);
	GErr.wasError = true;
	LogHistory (buf);

	THROW;
}

#endif


void CErrorHandler::Reset ()
{
	swError = nonFatalError = wasError = false;
	Message = "General Protection Fault !";		// on Win32 this message will appears only when compiled with WIN32_USE_SEH=1
	History[0] = 0;
}


/*-----------------------------------------------------------------------------
	Initialization
-----------------------------------------------------------------------------*/

#if !NO_DEBUG
static void Cmd_Error (int argc, char **argv)
{
	guard(Cmd_Error);
	if (!stricmp (argv[1], "-gpf"))
		*((int*)NULL) = 0;						// this is not "throw" command, this is GPF
	else if (!stricmp (argv[1], "-drop"))		//?? "drop" error - Q2-style (drop server); global usage (and name) may be different
		appNonFatalError ("testing drop error");
	else if (!stricmp (argv[1], "-stack"))
		Cmd_Error (argc, argv);					// infinite recurse
	else if (!stricmp (argv[1], "-throw"))
		THROW;									//?? use C++ throw?
	appFatalError ("%s", argv[1]);
	unguard;
}
#endif


//?? set GErr
void appInitError ()
{
	LogTail.NoColors = true;
	LogTail.Register ();
	GErr.Reset ();
#if !NO_DEBUG
	RegisterCommand ("error", Cmd_Error);
#endif
}


void appShutdownError ()
{
	if (ErrLogOpened)
	{
		// finish error log and close it
		ErrLog.Write ("\n\nLast log lines:\n");
		ErrLog.Write (LogTail.GetTail (10));
		ErrLog.Write ("\n\n");
		ErrLog.Close ();
	}
	if (GIsFatalError)
	{
		// print error message to global log
		appPrintf ("\n\n"S_RED"--------------------\n%s fatal error: %s\n", appPackage (), *GErr.Message);
#if DO_GUARD
		appPrintf ("History: %s\n\n", *GErr.History);
#endif
		// display popup
		appDisplayError ();
	}
}
