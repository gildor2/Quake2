#include "Core.h"


char	GErrorHistory[4096];


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
	char	buf[4096];
	va_list	argptr;
	int		len;

	va_start (argptr, fmt);
	len = vsnprintf (buf, sizeof(buf), fmt, argptr);
	va_end (argptr);
	if (len < 0 || len >= sizeof(buf) - 1)
		return;

	Write (buf);
}


//???
void appWPrintf (const char *fmt, ...)
{
	char	buf[4096];
	va_list	argptr;
	int		len;

	va_start (argptr, fmt);
	buf[0] = COLOR_ESCAPE; buf[1] = C_YELLOW + '0';		// make string colored
	len = vsnprintf (buf+2, sizeof(buf)-2, fmt, argptr);
	va_end (argptr);
	if (len < 0 || len >= sizeof(buf) - 1 - 2)
		return;

//??	GLog->Write (buf);
	appPrintf (buf);
}
