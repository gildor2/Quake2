
#if DBG_SYMBOLS

#define DBG_SYMBOLS_FILE	"symbols.dbg"

void appLoadDebugSymbols (void);		//?? CoreLocal
CORE_API bool appSymbolName (address_t addr, char *buffer, int size);
CORE_API char *appSymbolName (address_t addr);

// Platform-specific code
bool osAddressInfo (address_t address, char *pkgName, int bufSize, int *offset);	//?? CoreLocal
bool osModuleInfo (address_t address, char *exportName, int bufSize, int *offset);	//?? CoreLocal

#else // DBG_SYMBOLS

// empty declarations
#define appLoadDebugSymbols()			//?? CoreLocal

inline appSymbolName (address_t, char*, int)
{
	return false;
}

inline char *appSymbolName (address_t addr)
{
	return va("%08X", addr);
}

#endif // DBG_SYMBOLS
