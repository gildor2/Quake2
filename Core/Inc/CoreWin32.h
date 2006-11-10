/*=============================================================================

	Core definitions for Win32 platform

=============================================================================*/

// if windows.h is not included ...
#ifndef WINAPI
#	define HANDLE		void*
#	define HINSTANCE	void*
#endif // WINAPI

#if __GNUC__ && defined(FIELD_OFFSET)
// redefine FIELD_OFFSET macro to allow some macros to work in switch() as case label
#	undef FIELD_OFFSET
#	define FIELD_OFFSET(type,field)		offsetof(type,field)
#endif

// Package implementation
#define IMPLEMENT_PACKAGE(version,build,date)		\
	extern const char GPackage[]   = STR(PACKAGE);	\
	extern const char PkgVersion[] = STR(PACKAGE) " version " STR(version) " build " STR(build) " (" date ")";\
	HINSTANCE hInstance;							\
	int __stdcall DllMain(HINSTANCE hInst, int Reason, void *Reserved)	\
	{ hInstance = hInst; return 1; }


/*-----------------------------------------------------------------------------
	Win32 global variables
-----------------------------------------------------------------------------*/

extern bool GIsWinNT, GIsWin2K;
extern HINSTANCE hInstance;


/*-----------------------------------------------------------------------------
	Dynamic libraries
-----------------------------------------------------------------------------*/

#define DLLEXT		".dll"

// some declarations from kernel32.dll (to avoid use of <windows.h>)
#ifndef WINAPI
extern "C" {
	DLL_IMPORT HINSTANCE __stdcall LoadLibraryA(const char *name);
	DLL_IMPORT void*	 __stdcall GetProcAddress(HINSTANCE dllHandle, const char *name);
	DLL_IMPORT int		 __stdcall FreeLibrary(HINSTANCE dllHandle);
}
#endif

// portable wrapped for dynamic libraries
class CDynamicLib
{
private:
	HINSTANCE hDll;
public:
	inline bool Load(const char *name)
	{
		hDll = LoadLibraryA(name);
		return hDll != NULL;
	}
	inline void Free()
	{
		if (!hDll) return;			// not loaded
		FreeLibrary(hDll);
		hDll = NULL;
	}
	inline bool GetProc(const char *name, void *func)
	{
		*(address_t*)func = (address_t) GetProcAddress(hDll, name);
		return (*(address_t*)func != (address_t)NULL);
	}
	inline operator bool()
	{
		return hDll != NULL;
	}
};
