
#define COMMAND_USAGE	1		// have "bool usage"
#define COMMAND_ARGS	2		// uses "int argc, char **argv"
#define COMMAND_ARGS2	4		// uses "const char *args"


/*-----------------------------------------------------------------------------
	Generic commands
-----------------------------------------------------------------------------*/


CORE_API bool RegisterCommand (const char *name, void(*func)(), unsigned flags = 0);
CORE_API bool UnregisterCommand (const char *name);
CORE_API bool ExecuteCommand (const char *str);


inline bool RegisterCommand (const char *name, void(*func)(bool), unsigned flags = 0)
{
	return RegisterCommand (name, (void(*)())func, COMMAND_USAGE|flags);
}

inline bool RegisterCommand (const char *name, void(*func)(int,char**), unsigned flags = 0)
{
	return RegisterCommand (name, (void(*)())func, COMMAND_ARGS|flags);
}

inline bool RegisterCommand (const char *name, void(*func)(bool,int,char**), unsigned flags = 0)
{
	return RegisterCommand (name, (void(*)())func, COMMAND_USAGE|COMMAND_ARGS|flags);
}

inline bool RegisterCommand (const char *name, void(*func)(const char*), unsigned flags = 0)
{
	return RegisterCommand (name, (void(*)())func, COMMAND_ARGS2|flags);
}

inline bool RegisterCommand (const char *name, void(*func)(bool,const char*), unsigned flags = 0)
{
	return RegisterCommand (name, (void(*)())func, COMMAND_USAGE|COMMAND_ARGS2|flags);
}


/*-----------------------------------------------------------------------------
	Special commands
-----------------------------------------------------------------------------*/


struct CSimpleCommand
{
	const char	*name;
	void (*func)(int argc, char **argv);	// NULL to ignore command
};

CORE_API bool ExecuteCommand (const char *str, const CSimpleCommand *CmdList, int numCommands);
