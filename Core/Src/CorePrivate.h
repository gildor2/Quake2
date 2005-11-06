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
