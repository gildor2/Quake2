
#define COMMAND_USAGE	1
#define COMMAND_ARGS	2


/*-----------------------------------------------------------------------------
	Generic commands
-----------------------------------------------------------------------------*/


CORE_API bool RegisterCommand (const char *name, void(*func)(), int flags);
CORE_API bool UnregisterCommand (const char *name);
CORE_API void ExecuteCommand (const char *str); //?? make it local for Commands.cpp


inline bool RegisterCommand (const char *name, void(*func)(void))
{
	return RegisterCommand (name, func, 0);
}

inline bool RegisterCommand (const char *name, void(*func)(bool))
{
	return RegisterCommand (name, (void(*)())func, COMMAND_USAGE);
}

inline bool RegisterCommand (const char *name, void(*func)(int,char**))
{
	return RegisterCommand (name, (void(*)())func, COMMAND_ARGS);
}

inline bool RegisterCommand (const char *name, void(*func)(bool,int,char**))
{
	return RegisterCommand (name, (void(*)())func, COMMAND_USAGE|COMMAND_ARGS);
}


/*-----------------------------------------------------------------------------
	Special commands
-----------------------------------------------------------------------------*/


struct CSimpleCommand
{
	const char	*name;
	void (*func)(int argc, char **argv);
};

CORE_API bool ExecuteCommand (const char *str, const CSimpleCommand *CmdList, int numCommands);
