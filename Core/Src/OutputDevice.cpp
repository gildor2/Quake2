#include "Core.h"


class COutputDeviceNull : public COutputDevice
{
public:
	void Write (const char *str)
	{}
};

static COutputDeviceNull NullDevice;


COutputDevice *GLog  = &NullDevice;
COutputDevice *GNull = &NullDevice;


void COutputDevice::Printf (const char *fmt, ...)
{
	va_list	argptr;
	va_start (argptr, fmt);
	char buf[4096];
	int len = vsnprintf (ARRAY_ARG(buf), fmt, argptr);
	va_end (argptr);
	if (len < 0 || len >= sizeof(buf) - 1)
		return;

	Write (buf);
}


void appWPrintf (const char *fmt, ...)
{
	va_list	argptr;
	va_start (argptr, fmt);
	char buf[4096];
	int len = vsnprintf (ARRAY_ARG(buf), fmt, argptr);
	va_end (argptr);

	if (len < 0 || len >= sizeof(buf) - 1) return;
//??	GLog->Write (buf);
	appPrintf (S_YELLOW"%s", buf);
//??#ifndef NO_DEVELOPER
//??	if (developer->integer == 2) DebugPrintf ("%s", msg);
//??#endif
}
