#include <time.h>						// for logging time funcs
#include "Core.h"
#include "OutputDeviceFile.h"

#if !DO_GUARD
//?? may be, can do NonFatalError() without DO_GUARD ? (to be exact, without GUARD_BEGIN/GUARD_CATCH)
#error Current implementation not allow appNonFatalError() without DO_GUARD
#endif

#define CRASH_LOG		"crash.log"


bool GIsFatalError;


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
		// log date/time (header)
		time_t itime;
		time (&itime);
		char ctime[256];
		strftime (ARRAY_ARG(ctime), "%b %d, %Y (%H:%M:%S)", localtime (&itime));
		ErrLog.Printf ("\n\n");
		ErrLog.Printf ("-------------------------------------------\n");
		ErrLog.Printf ("%s: %s crash\n", appPackage (), ctime);
		ErrLog.Printf ("-------------------------------------------\n");
		ErrLog.Printf ("\nERROR: %s\n\n", *GErr.Message);
	}
	return &ErrLog;
}


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


/*-----------------------------------------------------------------------------
	Error throwing
-----------------------------------------------------------------------------*/

CErrorHandler GErr;

void appFatalError (const char *fmt, ...)
{
	GErr.swError = true;
	GIsFatalError = true;

	char buf[1024];
	va_list argptr;
	va_start (argptr, fmt);
	vsnprintf (ARRAY_ARG(buf), fmt, argptr);
	va_end (argptr);

//??	if (debugLogged) DebugPrintf ("FATAL ERROR: %s\n", buf);
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

	throw 1;
}


void appNonFatalError (const char *fmt, ...)
{
	GErr.swError       = true;
	GErr.nonFatalError = true;

	if (fmt)
	{
		va_list argptr;
		va_start (argptr, fmt);
		vsnprintf (ARRAY_ARG(GErr.Message), fmt, argptr);
		va_end (argptr);
		appPrintf (S_RED"ERROR: %s\n", *GErr.Message);
	}
	else
		GErr.Message[0] = 0;

	throw 1;
}


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
		buf[0] = ' '; buf[1] = '<'; buf[2] = '-'; buf[3] = ' ';
		vsnprintf (buf+4, sizeof(buf)-4, fmt, argptr);
	}
	else
		vsnprintf (buf, sizeof(buf), fmt, argptr);
	va_end (argptr);
	GErr.wasError = true;
	LogHistory (buf);

	if (!GErr.nonFatalError)
		GIsFatalError = true;

	throw;
}


void CErrorHandler::Reset ()
{
	swError = nonFatalError = wasError = false;
	Message = "General Protection Fault !";		// on Win32 this message will appears only when compiled with WIN32_USE_SEH=1
	History[0] = 0;
}


/*-----------------------------------------------------------------------------
	Initialization
-----------------------------------------------------------------------------*/

static void Cmd_Error (int argc, char **argv)
{
	guard(Cmd_Error);
	if (!stricmp (argv[1], "-gpf"))
		*((int*)NULL) = 0;						// this is not "throw" command, this is GPF
	else if (!stricmp (argv[1], "-drop"))		//?? "drop" error - Q2-style (drop server); global usage (and name) may be different
		appNonFatalError ("testing drop error");
	else if (!stricmp (argv[1], "-stack"))
		Cmd_Error (argc, argv);					// infinite recurse
	appFatalError ("%s", argv[1]);
	unguard;
}


//?? set GErr
void appInitError ()
{
	GErr.Reset ();
	RegisterCommand ("error", Cmd_Error);
}
