#include "qcommon.h"

int		cvar_initialized = 0;			// 0 - before config read, 1 - before system finished init, 2 - after init (2 is unused ??)
cvar_t	*cvar_vars;
static void *cvar_chain;

qboolean userinfo_modified;

static bool cheats = true;				// will be disabled on multiplayer game start
	// NOTE: this is CVAR cheats, not server cheats


#define HASH_BITS		8
#define HASH_SIZE		(1 << HASH_BITS)
#define HASH_MASK		(HASH_SIZE - 1)

static cvar_t	*hashTable[HASH_SIZE];


static int ComputeHash (char *name)
{
	int		h;
	char	*s, c;

	h = 0;
	s = name;
	while (c = *s++)
		h = (h ^ 0x25) + (c & 0xDF);	// ignore case for hash computation
	return h & HASH_MASK;
}


static void Cvar_SetString (cvar_t *var, char *str)
{
	int		len;

	if (var->string && !strcmp (var->string, str))
		return;		// not changed

//	Com_Printf ("^5set %s = %s\n", var->name, str);//!!

	// update non-string fields
	var->value = atof (str);
	var->integer = atoi (str);
	var->modified = true;

	// update "string" field
	len = strlen (str) + 1;

	if (!var->string_length && len <= CVAR_BUFFER_SIZE)
	{	// buffer is not allocated and cvar_t.buf is enough for holding str
		strcpy (var->buf, str);
		var->string = var->buf;
		return;
	}

	// buffer is already allocated or internal buffer is too small
	if (len > var->string_length)
	{	// buffer is too small - reallocate it
		if (var->string_length)
			Z_Free (var->string);	// free old buffer
		len = (len + 15) & ~15;		// align buffer size to 16 bytes
		var->string = Z_Malloc (len);
		var->string_length = len;
	}
	strcpy (var->string, str);
}


/*
============
Cvar_InfoValidate
============
*/
static bool Cvar_InfoValidate (char *s)
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
static cvar_t *Cvar_FindVar (char *var_name)
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
float Cvar_VariableValue (char *var_name)
{
	cvar_t	*var;

	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
	return var->value;
}

/*
============
Cvar_VariableValue
============
*/
int Cvar_VariableInt (char *var_name)
{
	cvar_t	*var;

	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
	return var->integer;
}

/*
============
Cvar_VariableString
============
*/
char *Cvar_VariableString (char *var_name)
{
	cvar_t *var;

	var = Cvar_FindVar (var_name);
	if (!var)
		return "";
	return var->string;
}


void CL_WriteConfiguration (char *filename);

static void Cvar_SetHardcoded (cvar_t *var, char *value)
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

static void FilterValue (cvar_t *var, char **value)
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
cvar_t *Cvar_Get (char *var_name, char *var_value, int flags)
{
	cvar_t	*var;
	int		hash;

	if (flags & (CVAR_USERINFO|CVAR_SERVERINFO))
	{
		if (!Cvar_InfoValidate (var_name))
		{
			Com_WPrintf("Invalid info cvar name\n");
			return NULL;
		}
	}

	var = Cvar_FindVar (var_name);
	if (var)
	{
		// keep non-user var
		if (!(flags & CVAR_USER_CREATED))
		{
			if (!(flags & CVAR_NODEFAULT))
			{
				if (!var->reset_string)
					var->reset_string = ChainCopyString (var_value, cvar_chain);	// save "default" value
				else if (stricmp (var->reset_string, var_value))
					Com_WPrintf ("Different default value for cvar %s: %s != %s\n", var_name, var_value, var->reset_string);
			}

			if (flags & CVAR_CHEAT && !cheats)					// reset cheatvar
				Cvar_SetString (var, var_value);

			if (var->flags & CVAR_USER_CREATED)
			{
				if (flags & CVAR_GAME_CREATED)
					var->flags |= CVAR_GAME_CREATED;			// user->game
				var->flags &= ~CVAR_USER_CREATED;				// reset user flag
			}
			strcpy (var->name, var_name);						// update name (may be different case)
		}

		// keep engine var
		if (!(var->flags & (CVAR_USER_CREATED|CVAR_GAME_CREATED)))
			flags &= ~(CVAR_USER_CREATED|CVAR_GAME_CREATED);	// cvar WAS engine-created (keep this flags)

		// user or game -> engine
//		if (!(flags & (CVAR_USER_CREATED|CVAR_GAME_CREATED)))
//			var->flags &= ~(CVAR_USER_CREATED|CVAR_GAME_CREATED); // cvar BECOMES engine-created (reset flags and keep it later)

		var->flags |= flags & CVAR_FLAG_MASK;
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

	var = AllocChainBlock (cvar_chain, sizeof(*var));
	var->name = AllocChainBlock (cvar_chain, strlen (var_name) + 1);
	strcpy (var->name, var_name);

	if (cvar_initialized < 2 && COM_CheckCmdlineVar (var_name))
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
	hash = ComputeHash (var_name);
	var->hashNext = hashTable[hash];
	hashTable[hash] = var;

	if (!(flags & (CVAR_NODEFAULT|CVAR_USER_CREATED)))
		var->reset_string = ChainCopyString (var_value, cvar_chain);

	var->flags = flags & CVAR_FLAG_MASK;

	return var;
}


void Cvar_GetVars (const cvarInfo_t *vars, int count)
{
	int		i;

	for (i = 0; i < count; i++, vars++)
	{
		cvar_t	*var;

		var = Cvar_Get (vars->name, vars->value, vars->flags);
		if (var)
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
static cvar_t *Cvar_Set2 (char *var_name, char *value, int flags, bool force)
{
	cvar_t	*var;

	var = Cvar_FindVar (var_name);
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
			if (var->latched_string)
			{
				if (!strcmp (value, var->latched_string))
					return var;
				Z_Free (var->latched_string);
				var->latched_string = NULL;

				if (!strcmp (value, var->string))
				{	// latched var have switched back to its value
					var->modified = false;
					return var;
				}
			}
			else
			{
				if (!strcmp (value, var->string))
					return var;
			}

			// here: var->latched string is unset (pointer released)
			if (Com_ServerState ())
			{
				if (Com_ServerState () == ss_demo)
					return var;										// disable changing of latched cvars from demos

				Com_Printf ("%s will be changed for next game.\n", var_name);
				var->latched_string = CopyString (value);
			}
			else
				Cvar_SetHardcoded (var, value);

			return var;
		}
	}
	else
	{
		if (var->latched_string)
		{
			Z_Free (var->latched_string);
			var->latched_string = NULL;
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
cvar_t *Cvar_FullSet (char *var_name, char *value, int flags)
{
	cvar_t	*var;

	var = Cvar_FindVar (var_name);
	if (!var)
	{	// create it
		return Cvar_Get (var_name, value, flags|CVAR_NODEFAULT);
	}

	Cvar_SetString (var, value);
	if (var->modified && var->flags & CVAR_USERINFO)
		userinfo_modified = true;	// transmit at next oportunity

	var->flags = flags & CVAR_FLAG_MASK;

	return var;
}

/*
============
Cvar_SetXXX
============
*/
cvar_t *Cvar_SetValue (char *var_name, float value)
{
	return Cvar_Set2 (var_name, va("%g",value), 0, false);	// old: %f (will add trailing zeros: 1.000000)
}

cvar_t *Cvar_SetInteger (char *var_name, int value)
{
	return Cvar_Set2 (var_name, va("%d", value), 0, false);
}

cvar_t *Cvar_Set (char *var_name, char *value)
{
	return Cvar_Set2 (var_name, value, 0, false);
}

cvar_t *Cvar_ForceSet (char *var_name, char *value)
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

float Cvar_ClampName (char *name, float low, float high)
{
	cvar_t	*var;

	var = Cvar_FindVar (name);

	if (!var)
	{
		Cvar_SetValue (name, low);
		return low;
	}
	return Cvar_Clamp (var, low, high);
}

/*
============
Cvar_GetLatchedVars

Any variables with latched values will now be updated
============
*/
void Cvar_GetLatchedVars (void)
{
	cvar_t	*var;

	for (var = cvar_vars; var; var = var->next)
	{
		if (!var->latched_string)
			continue;

		Cvar_SetHardcoded (var, var->latched_string);

		Z_Free (var->latched_string);
		var->latched_string = NULL;
	}
}

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qboolean Cvar_Command (void)
{
	cvar_t	*var;

	// check variables
	var = Cvar_FindVar (Cmd_Argv(0));
	if (!var)
		return false;

	// perform a variable print or set
	if (Cmd_Argc() == 1)
	{
		char	*def, *latch;

		// get string for latched value
		if (var->latched_string)
			latch = va("-> \"%s\" ", var->latched_string);
		else
			latch = "";
		// get string for default value
		if (var->reset_string)
		{
			if (!strcmp (var->string, var->reset_string))
				def = "(default)";
			else
				def = va("default:\"%s\"", var->reset_string);
		}
		else
			def = "(no default)";
		// print info
		Com_Printf ("\"%s\" is:\"%s\" %s%s\n", var->name, var->string, latch, def);
		return true;
	}

	Cvar_Set2 (var->name, Cmd_Argv(1), CVAR_USER_CREATED|CVAR_NODEFAULT, false);
	return true;
}


/*
============
Cvar_Set_f

Allows setting and defining of arbitrary cvars from console
============
*/

static void Cvar_SetWithFlags (int set, int reset)
{
	cvar_t	*var;
	int		flags;

	var = Cvar_FindVar (Cmd_Argv(1));
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
	flags = flags & ~reset | set;
	Cvar_FullSet (Cmd_Argv(1), Cmd_Argv(2), flags);
}


static void Cvar_Set_f (void)
{
	int			c;

	c = Cmd_Argc();
	if (c != 3 && c != 4)
	{
		Com_Printf ("Usage: set <variable> <value> [u|s|a]\n  (userinfo|serverinfo|archive)\n");
		return;
	}

	if (c == 4)
	{
		char	flag;

		switch (flag = *Cmd_Argv(3))
		{
		case 'u':
			Cvar_SetWithFlags (CVAR_USERINFO, CVAR_SERVERINFO);	// -s +u
			break;
		case 's':
			Cvar_SetWithFlags (CVAR_SERVERINFO, CVAR_USERINFO);	// +s -u
			break;
		case 'a':
			Cvar_SetWithFlags (CVAR_ARCHIVE, 0);				// +a
			break;
		default:
			Com_WPrintf ("Invalid flag for set: %c\n", flag);
			return;
		}
	}
	else
		Cvar_Set2 (Cmd_Argv(1), Cmd_Argv(2), CVAR_USER_CREATED|CVAR_NODEFAULT, false);
}


static void Cvar_Seta_f (void)
{
	if (Cmd_Argc() != 3)
	{
		Com_Printf ("Usage: seta <variable> <value>\n");
		return;
	}
	Cvar_SetWithFlags (CVAR_ARCHIVE, 0);
}


static void Cvar_Setu_f (void)
{
	if (Cmd_Argc() != 3)
	{
		Com_Printf ("Usage: setu <variable> <value>\n");
		return;
	}
	Cvar_SetWithFlags (CVAR_USERINFO, CVAR_SERVERINFO);
}


static void Cvar_Sets_f (void)
{
	if (Cmd_Argc() != 3)
	{
		Com_Printf ("Usage: sets <variable> <value>\n");
		return;
	}
	Cvar_SetWithFlags (CVAR_SERVERINFO, CVAR_USERINFO);
}


static void Cvar_Reset_f (void)
{
	cvar_t	*var;
	char	*mask;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("Usage: reset <mask>\n");
		return;
	}
	mask = Cmd_Argv(1);
	for (var = cvar_vars; var; var = var->next)
	{
		if (MatchWildcard2 (var->name, mask, true) && !(var->flags & (CVAR_NOSET|CVAR_LATCH|CVAR_GAME_CREATED|CVAR_USER_CREATED))
			&& (var->flags & CVAR_ARCHIVE) && var->reset_string)
			Cvar_Set2 (var->name, var->reset_string, 0, false);
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
	cvar_t	*var;
	char	type;
	bool	logged;

	logged = false;
	for (var = cvar_vars; var; var = var->next)
	{
		if (!(var->flags & CVAR_ARCHIVE))
			continue;
		if (!(var->flags & includeMask) || (var->flags & excludeMask))
			continue;

		if (var->reset_string && !(var->flags & CVAR_GAME_CREATED) && !strcmp (var->string, var->reset_string))
			continue;		// holds default value

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
static void Cvar_List_f (void)
{
	cvar_t	*var;
	int		n, total;
	char	*mask;

	if (Cmd_Argc() > 2)
	{
		Com_Printf ("Usage: cvarlist [<mask>]\n");
		return;
	}

	if (Cmd_Argc () == 2)
		mask = Cmd_Argv (1);
	else
		mask = NULL;

	n = total = 0;
	for (var = cvar_vars; var; var = var->next)
	{
		char	s[6], *color;
		int		flags;

		total++;
		if (mask && !MatchWildcard2 (var->name, mask, true)) continue;
		n++;
		flags = var->flags;
		strcpy (s, "      ");
		color = "";
		if (flags & CVAR_USER_CREATED)
		{
			s[0] = '*';
			color = "^6";
		}
		else if (flags & CVAR_GAME_CREATED)
		{
			s[0] = 'G';
			color = "^5";
		}
		if (flags & CVAR_ARCHIVE)		s[1] = 'A';
		if (flags & CVAR_USERINFO)		s[2] = 'U';
		if (flags & CVAR_SERVERINFO)	s[3] = 'S';
		if (flags & CVAR_NOSET)
		{
			s[4] = '-';
			color = "^1";
		}
		else if (flags & CVAR_LATCH)
			s[4] = 'L';
		if (flags & CVAR_CHEAT)
			s[5] = 'C';
		Com_Printf ("%s %s%s \"%s\"\n", s, color, var->name, var->string);
	}
	Com_Printf ("Displayed %d/%d cvars\n", n, total);
}


char *Cvar_BitInfo (int bit)
{
	static char	info[MAX_INFO_STRING];
	cvar_t	*var;

	info[0] = 0;

	for (var = cvar_vars; var; var = var->next)
	{
		if (var->flags & bit)
			Info_SetValueForKey (info, var->name, var->string);
	}
	return info;
}

// returns an info string containing all the CVAR_USERINFO cvars
char *Cvar_Userinfo (void)
{
	return Cvar_BitInfo (CVAR_USERINFO);
}

// returns an info string containing all the CVAR_SERVERINFO cvars
char *Cvar_Serverinfo (void)
{
	return Cvar_BitInfo (CVAR_SERVERINFO);
}

/*
============
Cvar_Add_f
============
*/
static void Cvar_Add_f (void)
{
	int		c;
	cvar_t	*var;
	char	*varName, *flags, flag;
	float	min, max, value;
	bool	wrap, force;							//?? rot[ate] -> wr[ap]

	c = Cmd_Argc();
	if (c != 3 && c != 5 && c != 6)
	{
		Com_Printf ("Usage: add <variable> <increment> [<min> <max> [w][f]]\n");
		return;
	}

	wrap = force = false;
	if (c == 6)
	{
		flags = Cmd_Argv(5);
		while (*flags)
		{
			switch (flag = *flags++)
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

	varName = Cmd_Argv(1);
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

	value = var->value + atof (Cmd_Argv(2));
	if (c == 5 || c == 6)
	{
		min = atof (Cmd_Argv(3));
		max = atof (Cmd_Argv(4));
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


static void Cvar_Scale_f (void)
{
	cvar_t	*var;
	char	*varName;

	if (Cmd_Argc() != 3)
	{
		Com_Printf ("Usage: scale <variable> <scale>\n");
		return;
	}

	varName = Cmd_Argv(1);
	var = Cvar_FindVar (varName);
	if (!var)
	{
		Com_WPrintf ("%s is not found\n", varName);
		return;
	}

	Cvar_Set2 (varName, va("%g", var->value * atof (Cmd_Argv(2))), CVAR_USER_CREATED|CVAR_NODEFAULT, false);
}


static void Cvar_Toggle_f (void)
{
	cvar_t	*var;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("Usage: toggle <variable>\n");
		return;
	}

	var = Cvar_FindVar (Cmd_Argv(1));
	if (!var || !var->value)
		Cvar_Set2 (Cmd_Argv(1), "1", CVAR_USER_CREATED|CVAR_NODEFAULT, false);
	else
		Cvar_Set2 (Cmd_Argv(1), "0", CVAR_USER_CREATED|CVAR_NODEFAULT, false);
}


static void Cvar_Cycle_f (void)
{
	cvar_t	*var;
	int		i, numValues, firstValue;
	bool	revert;

	if (!stricmp (Cmd_Argv (1), "-r"))
	{
		revert = true;
		firstValue = 3;
	}
	else
	{
		revert = false;
		firstValue = 2;
	}
	numValues = Cmd_Argc () - firstValue;
	if (numValues < 2)
	{
		Com_Printf ("Usage: cycle [-r] <variable> <value1> <value2> [<value3> ...]\n");
		return;
	}

	if (!(var = Cvar_FindVar (Cmd_Argv (firstValue-1))))
	{
		Cvar_Set2 (Cmd_Argv (firstValue-1), Cmd_Argv (firstValue), CVAR_USER_CREATED|CVAR_NODEFAULT, false);
		return;
	}

	for (i = 0; i < numValues; i++)
		if (!stricmp (var->string, Cmd_Argv (firstValue+i))) break;

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

	Cvar_Set2 (Cmd_Argv (firstValue-1), Cmd_Argv (firstValue+i), CVAR_USER_CREATED|CVAR_NODEFAULT, false);
}


void Cvar_Cheats (qboolean enable)
{
	cvar_t	*var;

	if (enable == cheats) return;
	cheats = enable;
	if (enable) return;

	for (var = cvar_vars; var; var = var->next)
		if (var->flags & CVAR_CHEAT && strcmp (var->string, var->reset_string))
		{
			Com_DPrintf ("fixing cheat cvar %s\n", var->name);
			Cvar_SetString (var, var->reset_string);
		}
}


/*
============
Cvar_Init

Reads in all archived cvars
============
*/
void Cvar_Init (void)
{
	cvar_chain = CreateMemoryChain ();

	Cmd_AddCommand ("set", Cvar_Set_f);
	Cmd_AddCommand ("seta", Cvar_Seta_f);
	Cmd_AddCommand ("setu", Cvar_Setu_f);
	Cmd_AddCommand ("sets", Cvar_Sets_f);
	Cmd_AddCommand ("reset", Cvar_Reset_f);
	Cmd_AddCommand ("cvarlist", Cvar_List_f);
	Cmd_AddCommand ("add", Cvar_Add_f);
	Cmd_AddCommand ("scale", Cvar_Scale_f);
	Cmd_AddCommand ("toggle", Cvar_Toggle_f);
	Cmd_AddCommand ("cycle", Cvar_Cycle_f);
}
