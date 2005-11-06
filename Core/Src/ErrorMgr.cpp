#include "CorePrivate.h"
#include "OutputDeviceFile.h"


//?? add "bool GSilentErrors": when true, appError() etc will not set GErr.swError/GIsFatalError +
//??  appGetErrorLog() will return GNull (incorrect: should avoid ErrLog places at all to allow use of
//??  logging in a case of later error with GSilentErrors==false)
//?? use GSilentErrors=1 with some TRY/CATCH blocks; Win32 analog:
//?? "__try { /*code1*/ } __except(1) { /*code2*/ }" =>
//?? "GSilentErrors=true;TRY { /*code1*/ } CATCH { /*code2*/ } END_CATCH GSilentErrors=false;"


#define CRASH_LOG		"crash.log"


bool			GIsFatalError;
CErrorHandler	GErr;


/*-----------------------------------------------------------------------------
	Error log file
-----------------------------------------------------------------------------*/

static COutputDeviceFile ErrLog;
static bool ErrLogOpened;				// cannot use ErrLog.IsOpened(): may be failed to open

COutputDevice *appGetErrorLog ()
{
	if (!ErrLogOpened)
	{
		ErrLogOpened = true;
		ErrLog.Open (CRASH_LOG);
		ErrLog.FlushEveryTime = true;
		ErrLog.NoColors       = false;	// do not modify output
		ErrLog.Printf ("\n\n");
		ErrLog.Printf ("-------------------------------------------\n");
		ErrLog.Printf ("%s crash, %s\n", appPackage (), appTimestamp ());
		ErrLog.Printf ("-------------------------------------------\n");
		ErrLog.Printf ("OS:  %s\nCPU: %s\n", GMachineOS, GMachineCPU);
		ErrLog.Printf ("\nERROR: %s\n\n", *GErr.Message);
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
		// fatal error, but log is not started
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

//?? when detect numContexts>sizeof(array), should add to history "(overflow)..." in appUnwind...()
static CGuardContext *contexts[1024];
static int numContexts = 0;


CGuardContext::CGuardContext (const char *msg)
: text(msg)
, jumped(false)
{
//	printf("%d-> %s\n", numContexts, msg);//!!
	// register context
	if (numContexts < ARRAY_COUNT(contexts))
		contexts[numContexts] = this;
	numContexts++;
}


CGuardContext::~CGuardContext ()
{
	if (jumped) return;
	// unregister context
	numContexts--;
//	printf("...(%d) %s\n", numContexts, text);
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
//	printf ("#%d  %s\n", numContexts-1, contexts[numContexts-1]->text);
	CGuardContext *ctx = contexts[numContexts-1];
	numContexts--;
	ctx->jumped = true;
	longjmp (ctx->jmp, 1);
}

#endif

void appFatalError (const char *fmt, ...)
{
	GErr.swError  = true;
	GIsFatalError = true;

	GLogHook = NULL;

	char buf[1024];
	va_list argptr;
	va_start (argptr, fmt);
	vsnprintf (ARRAY_ARG(buf), fmt, argptr);
	va_end (argptr);

//??	if (debugLog) debugLog->Printf ("FATAL ERROR: %s\n", buf);
	// open log
	if (!ErrLogOpened)
	{
		GErr.Message = buf;
		appGetErrorLog ();
	}
	else
	{
		// NOTE: this will work for appError() only; GPF not handled!
		ErrLog.Printf ("\n\nRecursive error: %s\n", buf);
		GErr.History[0] = 0;
		GErr.wasError = false;
	}

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

	if (!GErr.nonFatalError)
		GIsFatalError = true;

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
	GErr.Reset ();
#if !NO_DEBUG
	RegisterCommand ("error", Cmd_Error);
#endif
}
