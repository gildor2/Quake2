#include "CorePrivate.h"

#define MAX_LOGGERS		32
#define BUFFER_LEN		4096

COutputDevice *GLogHook = NULL;

/*-----------------------------------------------------------------------------
	NULL device
-----------------------------------------------------------------------------*/

class COutputDeviceNull : public COutputDevice
{
public:
	void Write (const char *str)
	{ /* empty */ }
};

static COutputDeviceNull GNullDevice;
COutputDevice *GNull = &GNullDevice;

/*-----------------------------------------------------------------------------
	Log device: all Write() -> appPrintf()
-----------------------------------------------------------------------------*/

class COutputDeviceLog : public COutputDevice
{
private:
	int recurse;
public:
	void Write (const char *str)
	{
		// do not allow GLog to be used in appPrintf() output device chains
		// in this case, GLog.Write -> appPrintf -> GLog.Write
		if (recurse)
			appError ("GLog: recurse");
		// print data
		recurse++;
		appPrintf ("%s", str);
		recurse--;
	}
};

static COutputDeviceLog GLogDevice;
COutputDevice *GLog = &GLogDevice;


/*-----------------------------------------------------------------------------
	COutputDevice
-----------------------------------------------------------------------------*/

static int numDevices = 0;
static COutputDevice *loggers[MAX_LOGGERS];

void COutputDevice::Printf (const char *fmt, ...)
{
	va_list	argptr;
	va_start (argptr, fmt);
	char buf[BUFFER_LEN];
	int len = vsnprintf (ARRAY_ARG(buf), fmt, argptr);
	va_end (argptr);
	if (len < 0 || len >= sizeof(buf) - 1)
		return;

	if (NoColors) appUncolorizeString (buf, buf); // in-place
	Write (buf);
	if (FlushEveryTime) Flush ();
}


void COutputDevice::Register ()
{
	if (numDevices)
	{
		for (int i = 0; i < numDevices; i++)
			if (loggers[i] == this) return;		// already registered
		if (numDevices >= MAX_LOGGERS)
			appError ("COutputDevice::Register: too much loggers");
		// new device will be first
		memmove (loggers+1, loggers, sizeof(COutputDevice*) * numDevices);
	}
	loggers[0] = this;
	numDevices++;
}


void COutputDevice::Unregister ()
{
	for (int i = 0; i < numDevices; i++)
		if (loggers[i] == this)
		{
			// found
			numDevices--;
			if (i < numDevices)			// not last
				memcpy (loggers+i, loggers+i+1, sizeof(COutputDevice*) * (numDevices - i)); // compact list
			return;
		}
}


/*-----------------------------------------------------------------------------
	Printing
-----------------------------------------------------------------------------*/

void appPrintf (const char *fmt, ...)
{
	guard(appPrintf);
	if (!numDevices) return;			// no loggers registered

	va_list	argptr;
	va_start (argptr, fmt);
	char buf[BUFFER_LEN];
	int len = vsnprintf (ARRAY_ARG(buf), fmt, argptr);
	va_end (argptr);
	if (len < 0 || len >= sizeof(buf) - 1) return;		//?? may be, write anyway ...

	if (GIsFatalError)
		GLogHook = NULL;

	char buf2[BUFFER_LEN];				// buffer for string with removed color codes
	buf2[0] = 0;
	for (int i = 0; i < numDevices; i++)
	{
		COutputDevice *out = GLogHook ? GLogHook : loggers[i];
		if (out->NoColors && !buf2[0])	// lazy operation: when needed only
			appUncolorizeString (buf2, buf);
		out->Write (out->NoColors ? buf2 : buf);
		if (out->FlushEveryTime) out->Flush ();
		// allow single device to grab all output
		if (GLogHook)
			break;
	}
	unguard;
}


void appWPrintf (const char *fmt, ...)
{
	va_list	argptr;
	va_start (argptr, fmt);
	char buf[BUFFER_LEN];
	int len = vsnprintf (ARRAY_ARG(buf), fmt, argptr);
	va_end (argptr);

	if (len < 0 || len >= sizeof(buf) - 1) return;		//?? may be, write anyway ...
	appPrintf (S_YELLOW"%s", buf);
}
