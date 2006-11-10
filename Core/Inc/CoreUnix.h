/*=============================================================================

	Core definitions for Unix/Linux platforms

=============================================================================*/

#ifndef __UNIX__
#define __UNIX__	1
#endif

#if !__CYGWIN__
#define stricmp		strcasecmp
#define strnicmp	strncasecmp
#endif


// Package implementation
//?? check
#define IMPLEMENT_PACKAGE(version,build,date)		\
	extern const char GPackage[]   = STR(PACKAGE);	\
	extern const char PkgVersion[] = STR(PACKAGE) " version " STR(version) " build " STR(build) " (" date ")";
//?? check "dlopen" man: have info about exported func "_init"
//?? doc about "dlopen" says: "_init" and "_fini" are obsolete, use __attribute__((constructor|destructor)) instead


/*-----------------------------------------------------------------------------
	Dynamic libraries
-----------------------------------------------------------------------------*/

#define GNU_SOURCE					// require some GNU extensions (dladdr())
#include <dlfcn.h>					//?? another place
#undef GNU_SOURCE

#define DLLEXT		".so"			//?? cygwin uses .dll (may be, .so works too?)

// portable wrapped for dynamic libraries
class CDynamicLib
{
private:
	void	*hDll;
public:
	inline bool Load(const char *name)
	{
		hDll = dlopen(name, RTLD_LAZY|RTLD_GLOBAL);
		return hDll != NULL;
	}
	inline void Free()
	{
		if (!hDll) return;			// not loaded
		dlclose(hDll);
		hDll = NULL;
	}
	inline bool GetProc(const char *name, void *func)
	{
		func = dlsym(hDll, name);
		return (*(address_t*)func != (address_t)NULL);
	}
	inline operator bool()
	{
		return hDll != NULL;
	}
};
