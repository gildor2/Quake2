/*=============================================================================

	Core definitions for Unix/Linux platforms

=============================================================================*/


// Package implementation
//?? check
#define IMPLEMENT_PACKAGE(version,build,date)		\
	extern const char GPackage[]   = STR(PACKAGE);	\
	extern const char PkgVersion[] = STR(PACKAGE) " version " STR(version) " build " STR(build) " (" date ")";
//?? check "dlopen" man: have info about exported func "_init"


/*-----------------------------------------------------------------------------
	Dynamic libraries
-----------------------------------------------------------------------------*/

#include <dlfcn.h>					//?? another place
#define DLLEXT		".so"			//?? cygwin uses .dll (may be, .so works too?)

// portable wrapped for dynamic libraries
class CDynamicLib
{
private:
	void	*hDll;
public:
	inline bool Load (const char *name)
	{
		hDll = dlopen (name, RTLD_LAZY|RTLD_GLOBAL);
		return hDll != NULL;
	}
	inline void Free ()
	{
		if (!hDll) return;			// not loaded
		dlclose (hDll);
		hDll = NULL;
	}
	inline bool GetProc (const char *name, void *func)
	{
		func = dlsym (hDll, name);
		return (*(address_t*)func != NULL);
	}
	inline operator bool ()
	{
		return hDll != NULL;
	}
};
