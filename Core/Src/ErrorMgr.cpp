#include "Core.h"

#if !DO_GUARD
//?? may be, can do NonFatalError() without DO_GUARD ? (to be exact, without GUARD_BEGIN/GUARD_CATCH)
#error Current implementation not allow appNonFatalError() without DO_GUARD
#endif


CErrorHandler GErr;

void appFatalError (const char *fmt, ...)
{
	va_list argptr;

	GErr.swError = true;

	va_start (argptr, fmt);
	vsnprintf (ARRAY_ARG(GErr.message), fmt, argptr);
	va_end (argptr);

//??	if (debugLogged) DebugPrintf ("FATAL ERROR: %s\n", errMsg);

	throw 1;
}


//?? try to call via FatalError() (handle situation when fmt==NULL)
void appNonFatalError (const char *fmt, ...)
{
	va_list argptr;

	GErr.swError = true;
	GErr.nonFatalError = true;

	if (fmt)
	{
		va_start (argptr, fmt);
		vsnprintf (ARRAY_ARG(GErr.message), fmt, argptr);
		va_end (argptr);
		appPrintf (S_RED"ERROR: %s\n", GErr.message);
	}
	else
		GErr.message[0] = 0;

	throw 1;
}


void appUnwindPrefix (const char *fmt)
{
	char	buf[512];

	if (GErr.wasError)
		appSprintf (buf, sizeof(buf), " <- %s:", fmt);
	else
		appSprintf (buf, sizeof(buf), "%s:", fmt);

	GErr.wasError = false;		// will not insert "<-" next appUnwindThrow()
	appStrcatn (ARRAY_ARG(GErr.history), buf);
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

	appStrcatn (ARRAY_ARG(GErr.history), buf);

#if 1
	throw;
#else
	*((int*)NULL) &= 123;
#endif
}


void CErrorHandler::Reset ()
{
	swError = nonFatalError = wasError = false;
	strcpy (message, "General Protection Fault !");	//?? check: is this message appears ?
	history[0] = 0;
}


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
