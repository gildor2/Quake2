#include "Core.h"


CErrorHandler GErr;


static char errMsg[256];		// to avoid stack-overflow dependency

void appFatalError (const char *fmt, ...)
{
	va_list argptr;

	GErr.fatalError = true;
	GErr.swError = true;

	va_start (argptr, fmt);
	vsnprintf (ARRAY_ARG(errMsg), fmt, argptr);
	va_end (argptr);

//??	if (debugLogged) DebugPrintf ("FATAL ERROR: %s\n", errMsg);
	appSprintf (ARRAY_ARG(GErr.history), "Error: %s\n\nHistory: ", errMsg);

	throw 1;
}


void appNonFatalError (const char *fmt, ...)
{
	va_list argptr;

	GErr.swError = true;

	if (fmt)
	{
		va_start (argptr, fmt);
		vsnprintf (ARRAY_ARG(errMsg), fmt, argptr);
		va_end (argptr);
//??		appPrintf (S_RED"ERROR: %s\n", errMsg);
	}

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
	strncat (GErr.history, buf, sizeof(GErr.history));
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

	strncat (GErr.history, buf, sizeof(GErr.history));

#if 1
	throw;
#else
	*((int*)NULL) &= 123;
#endif
}


void CErrorHandler::Reset ()
{
	swError = fatalError = wasError = false;
	strcpy (history, "General Protection Fault !\n\nHistory: ");	//?? check: is this message appears ?
}


static void Cmd_Error (int argc, char **argv)
{
	if (!stricmp (argv[1], "-gpf"))
		*((int*)NULL) = 0;						// this is not "throw" command, this is GPF
	else if (!stricmp (argv[1], "-drop"))		//?? "drop" error - Q2-style (drop server); global usage (and name) may be different
		appNonFatalError ("testing drop error");
	appFatalError ("%s", argv[1]);
}


//?? set GErr
void appInitError (void)
{
	GErr.Reset ();
	RegisterCommand ("error", Cmd_Error);
}
