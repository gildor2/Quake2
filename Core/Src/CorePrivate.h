/*=============================================================================

	Core private functions

=============================================================================*/


#define PACKAGE		Core
#define CORE_API	PACKAGE_EXPORT

#include "Core.h"


/*-----------------------------------------------------------------------------
	Startup/shutdown some subsystems
-----------------------------------------------------------------------------*/

void appInitMemory ();
void appInitError ();
void appInitPlatform ();

void appShutdownPlatform ();
void appShutdownError ();

void appDetectCPU ();


/*-----------------------------------------------------------------------------
	Error management
-----------------------------------------------------------------------------*/

// initiate error (will set GErr.Message and GIsFatalError + start logging)
COutputDevice *appBeginError (const char *msg);
// will return correct value even when log cannot be opened; should be called after GErr.Message is
// set, otherwise log file will contain "General Protection Fault" string!
COutputDevice *appGetErrorLog ();
#if SETJMP_GUARD
// when > 0, throwing exceptions will not produce application error
// (used for 'signal()+setjmp()'-driven exception handler only)
extern int GNumDisabledContexts;
#endif


/*-----------------------------------------------------------------------------
	Debug symbols internals
-----------------------------------------------------------------------------*/

#if DBG_SYMBOLS

void appLoadDebugSymbols ();

// platform-specific code
bool osGetAddressPackage (address_t address, char *pkgName, int bufSize, int &offset);
bool osGetAddressSymbol (address_t address, char *exportName, int bufSize, int &offset);

#else

#define appLoadDebugSymbols()

#endif


/*-----------------------------------------------------------------------------
	Unix private stuff
-----------------------------------------------------------------------------*/

#if __UNIX__

void appHookExceptions ();

#endif // __UNIX__
