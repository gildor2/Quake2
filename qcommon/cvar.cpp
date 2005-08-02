#include "qcommon.h"

int		cvar_initialized = 0;			// 0 - before config read, 1 - before system finished init, 2 - after init
cvar_t	*cvar_vars;
static CMemoryChain *cvar_chain;

bool userinfo_modified;

static bool cheats = true;				// will be disabled on multiplayer game start
	// NOTE: this is CVAR cheats, not server cheats


#define HASH_BITS		8
#define HASH_SIZE		(1 << HASH_BITS)
#define HASH_MASK		(HASH_SIZE - 1)

static cvar_t	*hashTable[HASH_SIZE];


static int ComputeHash (const char *name)
{
	int h = 0;
	while (char c = *name++)
		h = (h ^ 0x25) + (c & 0xDF);	// ignore case for hash computation
	return h & HASH_MASK;
}


static void Cvar_SetString (cvar_t *var, const char *str)
{
	if (var->string && !strcmp (var->string, str))
		return;		// not changed

//	Com_Printf (S_MAGENTA"set %s = %s\n", var->name, str);//!!

	// update non-string fields
	var->value = atof (str);
	var->integer = atoi (str);
	var->modified = true;

	// update "string" field
	int len = strlen (str) + 1;

	if (!var->stringLength && len <= CVAR_BUFFER_SIZE)
	{	// buffer is not allocated and cvar_t.buf is enough for holding str
		memcpy (var->buf, str, len);
		var->string = var->buf;
		return;
	}

	// buffer is already allocated or internal buffer is too small
	if (len > var->stringLength)
	{	// buffer is too small - reallocate it
		if (var->stringLength)
			appFree (var->string);	// free old buffer
		len = Align (len, 16);
		var->string = (char*) appMalloc (len);
		var->stringLength = len;
	}
	strcpy (var->string, str);
}


/*
============
Cvar_InfoValidate
============
*/
static bool Cvar_InfoValidate (const char *s)
{
	if (strchr (s, '\\') || strchr (s, '\"') || strchr (s, ';'))
		return false;

	return true;
}

/*
============
Cvar_FindVar
============
*/
static cvar_t *Cvar_FindVar (const char *var_name)
{
	cvar_t	*var;
	int		hash;

	hash = ComputeHash (var_name);
	for (var = hashTable[hash]; var; var = var->hashNext)
	{
		if (!stricmp (var_name, var->name))
			return var;
	}

	return NULL;
}

/*
============
Cvar_VariableValue
============
*/
float Cvar_VariableValue (const char *var_name)
{
	cvar_t *var = Cvar_FindVar (var_name);
	if (!var) return 0;
	return var->value;
}

/*
============
Cvar_VariableValue
============
*/
int Cvar_VariableInt (const char *var_name)
{
	cvar_t *var = Cvar_FindVar (var_name);
	if (!var) return 0;
	return var->integer;
}

/*
============
Cvar_VariableString
============
*/
const char *Cvar_VariableString (const char *var_name)
{
	cvar_t *var = Cvar_FindVar (var_name);
	if (!var) return "";
	return var->string;
}


void CL_WriteConfiguration (const char *filename);

static void Cvar_SetHardcoded (cvar_t *var, const char *value)
{
	if (!stricmp (var->name, "game"))
	{
		CL_WriteConfiguration (Cvar_VariableString ("cfgfile"));
		if (FS_SetGamedir (value))
		{
			Cvar_SetString (var, value);
			FS_LoadGameConfig ();
		}
	}
	// add another vars here
	else
		Cvar_SetString (var, value);
}

static void FilterValue (cvar_t *var, const char **value)
{
	if (!stricmp (var->name, "game") && !strcmp (*value, BASEDIRNAME))
		*value = "";
}


/*
============
Cvar_Get

If the variable already exists, the value will not be set
The flags will be or'ed in if the variable exists.
============
*/
static void Cvar_Init();

cvar_t *Cvar_Get (const char *var_name, const char *var_value, int flags)
{
	EXEC_ONCE(Cvar_Init())

	if (flags & (CVAR_USERINFO|CVAR_SERVERINFO))
	{
		if (!Cvar_InfoValidate (var_name))
		{
			Com_WPrintf("Invalid info cvar name\n");
			return NULL;
		}
	}

	cvar_t *var = Cvar_FindVar (var_name);
	if (var)
	{
		// keep non-user var
		if (!(flags & CVAR_USER_CREATED))
		{
			if (!(flags & CVAR_NODEFAULT))
			{
				if (!var->resetString)
					var->resetString = CopyString (var_value, cvar_chain);	// save "default" value
				else if (stricmp (var->resetString, var_value))
					Com_WPrintf ("Different default value for cvar %s: %s != %s\n", var_name, var_value, var->resetString);
			}

			if (flags & CVAR_CHEAT && !cheats)					// reset cheatvar
				Cvar_SetString (var, var_value);

			if (var->flags & CVAR_USER_CREATED)
			{
				if (flags & CVAR_GAME_CREATED)
					var->flags |= CVAR_GAME_CREATED;			// user->game
				var->flags &= ~CVAR_USER_CREATED;				// reset user flag
			}
			strcpy (const_cast<char*>(var->name), var_name);	// update name (may be different case)
		}

		// keep engine var
		if (!(var->flags & (CVAR_USER_CREATED|CVAR_GAME_CREATED)))
			flags &= ~(CVAR_USER_CREATED|CVAR_GAME_CREATED);	// cvar WAS engine-created (keep this flags)

		// user or game -> engine
//		if (!(flags & (CVAR_USER_CREATED|CVAR_GAME_CREATED)))
//			var->flags &= ~(CVAR_USER_CREATED|CVAR_GAME_CREATED); // cvar BECOMES engine-created (reset flags and keep it later)

		var->flags |= flags & CVAR_FLAG_MASK;

		// CVAR_LATCH vars updated only with Cvar_Get() calls ...
		if (var->latchedString)
		{
			Cvar_SetHardcoded (var, var->latchedString);
			appFree (var->latchedString);
			var->latchedString = NULL;
		}

		return var;
	}

	if (!var_value)
		return NULL;

	if (flags & (CVAR_USERINFO|CVAR_SERVERINFO))
	{
		if (!Cvar_InfoValidate (var_value))
		{
			Com_WPrintf ("Invalid info cvar value\n");
			return NULL;
		}
	}

	var = new (cvar_chain) cvar_t;
	var->name = CopyString (var_name, cvar_chain);

	if (cvar_initialized < 2 && Com_CheckCmdlineVar (var_name))
	{	// variable overriden with commandline as "1"
		Cvar_SetString (var, "1");
	}
	else
	{	// ordinary variable
		FilterValue (var, &var_name);
		Cvar_SetString (var, var_value);
	}

	// link the variable in
	var->next = cvar_vars;
	cvar_vars = var;

	// insert into a hash table
	int hash = ComputeHash (var_name);
	var->hashNext = hashTable[hash];
	hashTable[hash] = var;

	if (!(flags & (CVAR_NODEFAULT|CVAR_USER_CREATED)))
		var->resetString = CopyString (var_value, cvar_chain);

	var->flags = flags & CVAR_FLAG_MASK;

	return var;
}


void Cvar_GetVars (const cvarInfo_t *vars, int count)
{
	for (int i = 0; i < count; i++, vars++)
	{
		char	name[256];

		char *value = strchr (vars->string, '=');
		if (!value)	// should not happens
		{
			strcpy (name, vars->string);
			value = "";
		}
		else
		{
			// copy var name till delimiter
			appStrncpyz (name, vars->string, value - vars->string + 1);
			// skip delimiter
			value++;
		}

		cvar_t	*var;
		if (var = Cvar_Get (name, value, vars->flags))
		{
			if (vars->flags & CVAR_UPDATE)
				var->modified = true;
			else if (vars->flags & CVAR_NOUPDATE)
				var->modified = false;
		}

		if (vars->var)
			*vars->var = var;
	}
}


/*
============
Cvar_Set2
============
*/
static cvar_t *Cvar_Set2 (const char *var_name, const char *value, int flags, bool force)
{
	cvar_t *var = Cvar_FindVar (var_name);
	if (!var)
	{	// create it
		return Cvar_Get (var_name, value, flags|CVAR_NODEFAULT);
	}

	if (var->flags & CVAR_CMDLINE && !cvar_initialized && !force)
	{
		// prevent from modification of cvars, created from command line
//		Com_DPrintf ("Cvar_Set: ignore var %s\n", var->name);
		return var;
	}

	if (var->flags & (CVAR_USERINFO|CVAR_SERVERINFO))
	{
		if (!Cvar_InfoValidate (value))
		{
			Com_WPrintf ("Invalid info cvar value\n");
			return var;
		}
	}

	FilterValue (var, &value);
	if (!force)
	{
		if (var->flags & CVAR_NOSET)
		{
			if (strcmp (var->string, value))						// no error when value is not changed
				Com_WPrintf ("%s is write protected\n", var_name);
			return var;
		}

		if (var->flags & CVAR_CHEAT && !cheats)
		{
			if (!strcmp (value, var->string))						// no message when not modified
				return var;
			if (flags & CVAR_USER_CREATED)
				Com_WPrintf ("%s is cheat protected\n", var_name);
			else
				Com_DPrintf ("%s is cheat protected\n", var_name);	// trying to modify cheat cvar by engine without "force"
			return var;
		}

		if (var->flags & CVAR_LATCH)
		{
			if (var->latchedString)
			{
				if (!strcmp (value, var->latchedString))			// not modified
					return var;
				// free old latched string
				appFree (var->latchedString);
				var->latchedString = NULL;
			}
			if (!strcmp (value, var->string))
			{	// latched var have switched back to its value
				var->modified = false;
				return var;
			}
			// here: var->latchedString is unset (pointer released)
			if (Com_ServerState ())
			{
				if (Com_ServerState () == ss_demo)
					return var;										// disable changing of latched cvars from demos

				Com_Printf ("%s will be changed after restart\n", var_name);
				var->latchedString = CopyString (value);
			}
			else
				Cvar_SetHardcoded (var, value);

			return var;
		}
	}
	else
	{
		if (var->latchedString)
		{
			appFree (var->latchedString);
			var->latchedString = NULL;
		}
	}

	if (!strcmp (value, var->string))
		return var;					// not changed

	Cvar_SetString (var, value);
	if (var->modified && var->flags & CVAR_USERINFO)
		userinfo_modified = true;	// transmit at next oportunity

	return var;
}

/*
============
Cvar_FullSet
============
*/

void Cvar_FullSet (cvar_t *var, const char *value, unsigned flags)
{
	Cvar_SetString (var, value);
	if (var->modified && var->flags & CVAR_USERINFO)
		userinfo_modified = true;	// transmit at next oportunity

	var->flags = flags & CVAR_FLAG_MASK;
}

cvar_t *Cvar_FullSet (const char *var_name, const char *value, unsigned flags)
{
	cvar_t *var = Cvar_FindVar (var_name);
	if (!var)
	{	// create it
		return Cvar_Get (var_name, value, flags|CVAR_NODEFAULT);
	}

	Cvar_FullSet (var, value, flags);
	return var;
}

/*
============
Cvar_SetXXX
============
*/
cvar_t *Cvar_SetValue (const char *var_name, float value)
{
	return Cvar_Set2 (var_name, va("%g",value), 0, false);	// old: %f (will add trailing zeros: 1.000000)
}

cvar_t *Cvar_SetInteger (const char *var_name, int value)
{
	return Cvar_Set2 (var_name, va("%d", value), 0, false);
}

cvar_t *Cvar_Set (const char *var_name, const char *value)
{
	return Cvar_Set2 (var_name, value, 0, false);
}

cvar_t *Cvar_ForceSet (const char *var_name, const char *value)
{
	return Cvar_Set2 (var_name, value, 0, true);
}

float Cvar_Clamp (cvar_t *cvar, float low, float high)
{
	if (low > high)
		return 0;	// invalid arguments

	if (cvar->value < low)
	{
		Cvar_SetValue (cvar->name, low);
		return low;
	}
	if (cvar->value > high)
	{
		Cvar_SetValue (cvar->name, high);
		return high;
	}
	return cvar->value;
}

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
bool Cvar_Command (int argc, char **argv)
{
	// check variables
	cvar_t *var = Cvar_FindVar (argv[0]);
	if (!var) return false;

	// perform a variable print or set
	if (argc == 1)
	{
		const char *def;

		// get string for latched value
		const char *latch = var->latchedString ? va("-> \"%s\" ", var->latchedString) : "";
		// get string for default value
		if (var->resetString)
		{
			if (var->IsDefault())
				def = "(default)";
			else
				def = va("default:\"%s\"", var->resetString);
		}
		else
			def = "(no default)";
		// print info
		Com_Printf ("\"%s\" is:\"%s\" %s%s\n", var->name, var->string, latch, def);
		return true;
	}

	Cvar_Set2 (var->name, argv[1], CVAR_USER_CREATED|CVAR_NODEFAULT, false);
	return true;
}


/*
============
Cvar_Set_f

Allows setting and defining of arbitrary cvars from console
============
*/

static void Cvar_SetWithFlags (const char *name, const char *value, unsigned set, unsigned reset)
{
	unsigned flags;

	cvar_t *var = Cvar_FindVar (name);
	if (var)
	{
		flags = var->flags;
		if (flags & CVAR_CMDLINE && !cvar_initialized)
		{
			// prevent from modification of cvars, created from command line
//			Com_DPrintf ("Cvar_SetWithFlags: ignore var %s\n", var->name);
			return;
		}
	}
	else
		flags = CVAR_USER_CREATED|CVAR_NODEFAULT;
	// var may be NULL here!
	Cvar_FullSet (name, value, flags & ~reset | set);
}


static void Cvar_Set_f (bool usage, int argc, char **argv)
{
	if (argc != 3 && argc != 4 || usage)
	{
		Com_Printf ("Usage: set <variable> <value> [a|u|s]\n  (archive|userinfo|serverinfo)\n");
		return;
	}

	if (argc == 4)
	{
		switch (char flag = argv[3][0])
		{
		case 'a':
			Cvar_SetWithFlags (argv[1], argv[2], CVAR_ARCHIVE, 0);					// +a
			break;
		case 'u':
			Cvar_SetWithFlags (argv[1], argv[2], CVAR_USERINFO, CVAR_SERVERINFO);	// -s +u
			break;
		case 's':
			Cvar_SetWithFlags (argv[1], argv[2], CVAR_SERVERINFO, CVAR_USERINFO);	// +s -u
			break;
		default:
			Com_WPrintf ("Invalid flag for set: %c\n", flag);
			return;
		}
	}
	else
		Cvar_Set2 (argv[1], argv[2], CVAR_USER_CREATED|CVAR_NODEFAULT, false);
}


static void Cvar_Seta_f (bool usage, int argc, char **argv)
{
	if (argc != 3 || usage)
	{
		Com_Printf ("Usage: seta <variable> <value>\n");
		return;
	}
	Cvar_SetWithFlags (argv[1], argv[2], CVAR_ARCHIVE, 0);
}


static void Cvar_Setu_f (bool usage, int argc, char **argv)
{
	if (argc != 3 || usage)
	{
		Com_Printf ("Usage: setu <variable> <value>\n");
		return;
	}
	Cvar_SetWithFlags (argv[1], argv[2], CVAR_USERINFO, CVAR_SERVERINFO);
}


static void Cvar_Sets_f (bool usage, int argc, char **argv)
{
	if (argc != 3 || usage)
	{
		Com_Printf ("Usage: sets <variable> <value>\n");
		return;
	}
	Cvar_SetWithFlags (argv[1], argv[2], CVAR_SERVERINFO, CVAR_USERINFO);
}


static void Cvar_Reset_f (bool usage, int argc, char **argv)
{
	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: reset <mask>\n");
		return;
	}
	const char *mask = argv[1];
	for (cvar_t *var = cvar_vars; var; var = var->next)
	{
		if (appMatchWildcard (var->name, mask, true) && !(var->flags & (CVAR_NOSET|CVAR_LATCH|CVAR_GAME_CREATED|CVAR_USER_CREATED))
			&& (var->flags & CVAR_ARCHIVE) && var->resetString)
			Cvar_Set2 (var->name, var->resetString, 0, false);	//!! var->Reset(), when Reset() will be optimized
	}
}


/*
============
Cvar_WriteVariables

Appends lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables (FILE *f, int includeMask, int excludeMask, const char *header)
{
	bool logged = false;
	for (cvar_t *var = cvar_vars; var; var = var->next)
	{
		if (!(var->flags & CVAR_ARCHIVE))
			continue;
		if (!(var->flags & includeMask) || (var->flags & excludeMask))
			continue;

		if (!(var->flags & CVAR_GAME_CREATED) && var->IsDefault())
			continue;		// holds default value

		char type;
		if (!(var->flags & (CVAR_SERVERINFO|CVAR_USERINFO)))
			type = 'a';
		else if (var->flags & CVAR_SERVERINFO)
			type = 's';
		else
			type = 'u';
		if (!logged)
		{
			fprintf (f, header);
			logged = true;
		}
		fprintf (f, "set%c %s \"%s\"\n", type, var->name, var->string);
	}
}

/*
============
Cvar_List_f
============
*/
static void Cvar_List_f (bool usage, int argc, char **argv)
{
	if (argc > 2 || usage)
	{
		Com_Printf ("Usage: cvarlist [<mask>]\n");
		return;
	}

	const char *mask = argc == 2 ? argv[1] : NULL;

	int n, total;
	n = total = 0;
	for (cvar_t *var = cvar_vars; var; var = var->next)
	{
		total++;
		if (mask && !appMatchWildcard (var->name, mask, true)) continue;
		n++;
		unsigned flags = var->flags;

		char	s[7];
		strcpy (s, "      ");
		const char *color = "";
		if (flags & CVAR_USER_CREATED)
		{
			s[0] = '*';
			color = S_CYAN;
		}
		else if (flags & CVAR_GAME_CREATED)
		{
			s[0] = 'G';
			color = S_MAGENTA;
		}
		if (flags & CVAR_ARCHIVE)		s[1] = 'A';
		if (flags & CVAR_USERINFO)		s[2] = 'U';
		if (flags & CVAR_SERVERINFO)	s[3] = 'S';
		if (flags & CVAR_NOSET)
		{
			s[4] = '-';
			color = S_RED;
		}
		else if (flags & CVAR_LATCH)
			s[4] = 'L';
		if (flags & CVAR_CHEAT)
			s[5] = 'C';
		Com_Printf ("%s %s%s = \"%s\"\n", s, color, var->name, var->string);
	}
	Com_Printf ("Displayed %d/%d cvars\n", n, total);
}


static const char *Cvar_BitInfo (unsigned bit)
{
	char info[MAX_INFO_STRING];
	info[0] = 0;

	for (cvar_t *var = cvar_vars; var; var = var->next)
	{
		if (var->flags & bit)
			Info_SetValueForKey (info, var->name, var->string);
	}
	return va("%s", info);
}

// returns an info string containing all the CVAR_USERINFO cvars
const char *Cvar_Userinfo (void)
{
	return Cvar_BitInfo (CVAR_USERINFO);
}

// returns an info string containing all the CVAR_SERVERINFO cvars
const char *Cvar_Serverinfo (void)
{
	return Cvar_BitInfo (CVAR_SERVERINFO);
}

/*
============
Cvar_Add_f
============
*/
static void Cvar_Add_f (bool usage, int argc, char **argv)
{
	cvar_t	*var;
	char	*varName;
	bool	wrap, force;							//?? rot[ate] -> wr[ap]

	if (argc != 3 && argc != 5 && argc != 6 || usage)
	{
		Com_Printf ("Usage: add <variable> <increment> [<min> <max> [w][f]]\n");
		return;
	}

	wrap = force = false;
	if (argc == 6)
	{
		char *flags = argv[5];
		while (*flags)
		{
			switch (char flag = *flags++)
			{
			case 'w':
				wrap = true;
				break;
			case 'f':
				force = true;
				break;
			default:
				Com_WPrintf ("Invalid flag for add: %c\n", flag);
				return;
			}
		}
	}

	varName = argv[1];
	var = Cvar_FindVar (varName);
	if (!var)
	{
		if (force)
			var = Cvar_Get (varName, "0", 0);
		else
		{
			Com_WPrintf ("%s is not found\n", varName);
			return;
		}
	}

	float value = var->value + atof (argv[2]);
	if (argc == 5 || argc == 6)
	{
		float min = atof (argv[3]);
		float max = atof (argv[4]);
		if (min >= max)
		{
			Com_WPrintf ("add: MIN >= MAX\n");
			return;
		}
		// check bounds with a small epsilon to avoid FP precision bugs with small numbers
		if (wrap)
		{	// rotate value
			if (value > max + 0.0001f)
				value = min;
			else if (value < min - 0.0001f)
				value = max;
		}
		else
		{	// limit value
			if (value > max + 0.0001f)
				value = max;
			else if (value < min - 0.0001f)
				value = min;
		}
	}
	// make new string value from float
	Cvar_Set2 (varName, va("%g", value), CVAR_USER_CREATED|CVAR_NODEFAULT, false);
}


static void Cvar_Scale_f (bool usage, int argc, char **argv)
{
	cvar_t	*var;
	char	*varName;

	if (argc != 3 || usage)
	{
		Com_Printf ("Usage: scale <variable> <scale>\n");
		return;
	}

	varName = argv[1];
	var = Cvar_FindVar (varName);
	if (!var)
	{
		Com_WPrintf ("%s is not found\n", varName);
		return;
	}

	Cvar_Set2 (varName, va("%g", var->value * atof (argv[2])), CVAR_USER_CREATED|CVAR_NODEFAULT, false);
}


static void Cvar_Toggle_f (bool usage, int argc, char **argv)
{
	cvar_t	*var;

	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: toggle <variable>\n");
		return;
	}

	var = Cvar_FindVar (argv[1]);
	if (!var || !var->value)
		Cvar_Set2 (argv[1], "1", CVAR_USER_CREATED|CVAR_NODEFAULT, false);
	else
		Cvar_Set2 (argv[1], "0", CVAR_USER_CREATED|CVAR_NODEFAULT, false);
}


static void Cvar_Cycle_f (bool usage, int argc, char **argv)
{
	cvar_t	*var;
	int		i, numValues, firstValue;
	bool	revert;

	if (!stricmp (argv[1], "-r"))
	{
		revert = true;
		firstValue = 3;
	}
	else
	{
		revert = false;
		firstValue = 2;
	}
	numValues = argc - firstValue;
	if (numValues < 2 || usage)
	{
		Com_Printf ("Usage: cycle [-r] <variable> <value1> <value2> [<value3> ...]\n");
		return;
	}

	if (!(var = Cvar_FindVar (argv[firstValue-1])))
	{
		Cvar_Set2 (argv[firstValue-1], argv[firstValue], CVAR_USER_CREATED|CVAR_NODEFAULT, false);
		return;
	}

	for (i = 0; i < numValues; i++)
		if (!stricmp (var->string, argv[firstValue+i])) break;

	if (i == numValues)		// not found
		i = 0;
	else if (!revert)
	{	// next value
		i++;
		if (i > numValues - 1) i = 0;
	}
	else
	{	// previous value
		i--;
		if (i < 0) i = numValues - 1;
	}

	Cvar_Set2 (argv[firstValue-1], argv[firstValue+i], CVAR_USER_CREATED|CVAR_NODEFAULT, false);
}


void Cvar_Cheats (bool enable)
{
	if (enable == cheats) return;
	cheats = enable;
	if (enable) return;

	for (cvar_t *var = cvar_vars; var; var = var->next)
		if (var->flags & CVAR_CHEAT && !var->IsDefault())
		{
			Com_DPrintf ("fixing cheat cvar %s\n", var->name);
			var->Reset();
		}
}


/*
============
Cvar_Init

Reads in all archived cvars
============
*/
static void Cvar_Init ()
{
	cvar_chain = new CMemoryChain;

	RegisterCommand ("set", Cvar_Set_f);
	RegisterCommand ("seta", Cvar_Seta_f);
	RegisterCommand ("setu", Cvar_Setu_f);
	RegisterCommand ("sets", Cvar_Sets_f);
	RegisterCommand ("reset", Cvar_Reset_f);
	RegisterCommand ("cvarlist", Cvar_List_f);
	RegisterCommand ("add", Cvar_Add_f);
	RegisterCommand ("scale", Cvar_Scale_f);
	RegisterCommand ("toggle", Cvar_Toggle_f);
	RegisterCommand ("cycle", Cvar_Cycle_f);
}
