#if DBG_SYMBOLS

CORE_API bool appSymbolName (address_t addr, char *buffer, int size);
CORE_API const char *appSymbolName (address_t addr);

#else // DBG_SYMBOLS

inline bool appSymbolName (address_t, char*, int)
{
	return false;
}

inline const char *appSymbolName (address_t addr)
{
	return va("%08X", addr);
}

#endif // DBG_SYMBOLS
