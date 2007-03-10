#include "qcommon.h"
#include "../client/ref.h"	// using RE_DrawTextXxx() for com_speeds
#include "OutputDeviceFile.h"


static cvar_t *com_speeds;
cvar_t	*developer;
cvar_t	*timescale;
cvar_t	*timedemo;
cvar_t	*sv_cheats;
static cvar_t *nointro;


#if !DEDICATED_ONLY && !NO_DEDICATED
static cvar_t dummyDedicated;
cvar_t	*dedicated = &dummyDedicated;
	// dummy cvar; may be used before initialized - when error happens before
	// Com_Init()->Cvar_GetVars(); after this, will point to actual cvar
#else
cvar_t	*dedicated;
#endif

static cvar_t	*logfile_active;		// 1 = buffer log, 2 = flush after each print
static COutputDeviceFile *FileLog;

static server_state_t server_state;

// com_speeds times
unsigned time_before_game, time_after_game, time_before_ref, time_after_ref;


#if !NO_DEVELOPER
/*
================
Com_DPrintf

A appPrintf that only shows up if the "developer" cvar is set
(prints on a screen with a different color)
When developer is set to 2, logging the message.
When developer is set to 256, do not colorize message.
================
*/
void Com_DPrintf(const char *fmt, ...)
{
	if (!DEVELOPER) return;

	va_list	argptr;
	char	msg[4096];
	va_start(argptr,fmt);
	vsnprintf(ARRAY_ARG(msg),fmt,argptr);
	va_end(argptr);
	appPrintf(S_BLUE"%s", msg);
	if (developer->integer == 2)
	{
		appUncolorizeString(msg, msg);
		DebugPrintf("%s", msg);
	}
}
#endif


COutputDevice *debugLog;	// can initialize with GNull (COutputDeviceNull) ?

void DebugPrintf(const char *fmt, ...)
{
	va_list	argptr;
	char	msg[4096];
	va_start(argptr,fmt);
	vsnprintf(ARRAY_ARG(msg),fmt,argptr);
	va_end(argptr);

	if (!debugLog)
	{
		debugLog = new COutputDeviceFile("debug.log");
		debugLog->Printf("\n\n----- %s debug log, %s -----\n", appPackage(), appTimestamp());
	}
	debugLog->Write(msg);
	debugLog->Flush();
}


static unsigned GetInt(const char **str)
{
	unsigned r = 0;
	const char *s = *str;
	while (true)
	{
		int c = *s - '0';
		if (c < 0 || c > 9) break;
		r = r * 10 + c;
		s++;
	}
	*str = s;
	return r;
}

bool IPWildcard(netadr_t *a, const char *mask)
{
	if (a->type != NA_IP) return false;
	const char *m = mask;

	for (int i = 0; i < 4; i++)
	{
		if (m[0] == '*')
			m++;			// skip '*'
		else if (m[0] >= '0' && m[0] <= '9')
		{
			int n = GetInt(&m);
			if (m[0] == '.' || m[0] == 0)
			{
				if (a->ip[i] != n) return false;
			}
			else if (m[0] == '-')
			{
				if (a->ip[i] < n) return false;
				m++;
				n = GetInt(&m);
				if (a->ip[i] > n) return false;
			}
			else
			{
				Com_DPrintf("IPWildcard: bad char in \"%s\"\n", mask);
				return false;
			}
		}
		else
		{
			Com_DPrintf("IPWildcard: bad char in \"%s\"\n", mask);
			return false;
		}
		if (m[0] == 0 && i < 3)
		{
			Com_DPrintf("IPWildcard: short mask \"%s\"\n", mask);
			return true;
		}
		m++;
	}
	return true;
}


server_state_t Com_ServerState()
{
	return server_state;
}

void Com_SetServerState(server_state_t state)
{
	server_state = state;
}


#if 1
// -1 to 1
float crand()
{
	return rand() * (2.0f/RAND_MAX) - 1;
}
#endif


/*
=====================================================================

  INFO STRINGS

=====================================================================
*/

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
===============
*/
const char *Info_ValueForKey(const char *s, const char *key)
{
	char	pkey[512];
	char	value[512];
								// work without stomping on each other
	if (*s == '\\') s++;
	while (true)
	{
		char *o = pkey;
		while (*s != '\\')
		{
			if (!*s) return "";
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (!strcmp(key, pkey))
			return va("%s", value);

		if (!*s) return "";
		s++;
	}
}

static void Info_RemoveKey(char *s, const char *key)
{
	char	pkey[512];
	char	value[512];

	if (*s == '\\') s++;
	while (true)
	{
		char *start = s;
		char *o = pkey;
		while (*s != '\\')
		{
			if (!*s) return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (!strcmp(key, pkey))
		{
			strcpy(start, s);	// remove this part
			return;
		}

		if (!*s) return;
		s++;
	}

}


void Info_SetValueForKey(char *s, const char *key, const char *value)
{
	if (strchr(key, '\\') || strchr(value, '\\'))
	{
		appWPrintf("Info_Set(\"%s\",\"%s\"): can't use keys or values with a '\\'\n", key, value);
		return;
	}

#if 0
	if (strchr(key, '\"') || strchr(value, '\"'))
	{
		appWPrintf("Can't use keys or values with a '\"'\n");
		return;
	}

	if (strchr(key, ';'))
	{
		appWPrintf("Can't use keys or values with a semicolon\n");
		return;
	}
#endif

	Info_RemoveKey(s, key);
	if (!value || !value[0]) return;	// will not add variable with empty value, for example, when game=""

	char newi[MAX_INFO_STRING];
	int size = appSprintf(ARRAY_ARG(newi), "\\%s\\%s", key, value);
	if (strlen(s) + size >= MAX_INFO_STRING)
	{
		appWPrintf("Info string length exceeded\n");
		return;
	}

	// only copy ASCII values
	s += strlen(s);
	char *v = newi;
	while (*v)
	{
		char c = *v++ & 127;		// strip high bits (note: q3 does not perform this!)
		if (c >= 32) *s++ = c;
	}
	*s = 0;
}


void Info_Print(const char *s)
{
	char	key[512];
	char	value[512];

	if (*s == '\\') s++;
	while (true)
	{
		char *o = key;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;
		appPrintf(S_GREEN"%-22s", key);

		if (!*s)
		{
			appWPrintf("missing value\n");
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;
		appPrintf("%s\n", value);

		if (!*s) return;
		s++;
	}
}


/*-----------------------------------------------------------------------------
	Angle functions (taken from Quake3)
-----------------------------------------------------------------------------*/

//!! incorrect in most cases (should be slerp); used by cl_ents.cpp only
float LerpAngle(float a1, float a2, float frac)
{
	if (a2 - a1 > 180)
		a2 -= 360;
	if (a2 - a1 < -180)
		a2 += 360;
	return Lerp(a1, a2, frac);
}

float ReduceAngle(float a)
{
	a = (360.0f/65536) * (appRound(a*(65536.0f/360)) & 65535);
	return a;
}

float AngleSubtract(float a1, float a2)
{
	float a = a1 - a2;
	while (a > 180)
		a -= 360;
	while (a < -180)
		a += 360;
	return a;
}

void AnglesSubtract(const CVec3 &v1, const CVec3 &v2, CVec3 &v3)
{
	v3[0] = AngleSubtract(v1[0], v2[0]);
	v3[1] = AngleSubtract(v1[1], v2[1]);
	v3[2] = AngleSubtract(v1[2], v2[2]);
}


/*
============================================================================

					TEXT PARSER

============================================================================
*/

static char com_token[MAX_STRING_CHARS];
static int	com_lines;

static const char *SkipWhitespace(const char *data, bool *hasNewLines)
{
	char	c;
	while ((c = *data) <= ' ')
	{
		if (!c) return NULL;
		if (c == '\n')
		{
			com_lines++;
			*hasNewLines = true;
		}
		data++;
	}

	return data;
}

char *COM_Parse(const char *&data_p, bool allowLineBreaks)
{
	const char *data = data_p;
	com_token[0] = 0;

	if (!data)					// all data is out
		return com_token;		// ""

	int len = 0;
	char c = 0;
	bool hasNewLines = false;

	while (true)
	{
		// skip whitespace
		data = SkipWhitespace(data, &hasNewLines);
		if (!data)
		{
			data_p = NULL;
			return com_token;
		}
		if (hasNewLines && !allowLineBreaks)
		{
			data_p = data;
			return com_token;
		}

		c = *data;

		// skip double slash comments
		if (c == '/' && data[1] == '/')
		{
			data += 2;
			while (*data && *data != '\n') data++;
		}
		// skip /* */ comments
		else if (c=='/' && data[1] == '*')
		{
			data += 2;
			while (*data && (*data != '*' || data[1] != '/'))
				data++;
			if (*data)
				data += 2;
		}
		else
			break;
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (true)
		{
			c = *data++;
			if (c == '\"' && *data == '\"')
			{
				// doubled quotes
				data++;				// skip both quotes
				if (len < sizeof(com_token))
					com_token[len++] = c;
				continue;
			}

			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				data_p = (char *) data;
				return com_token;
			}

			if (len < sizeof(com_token))
				com_token[len++] = c;
		}
	}

	// parse a regular word
	do
	{
		if (len < sizeof(com_token))
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
		if (c == '\n')
			com_lines++;
	} while (c > 32);

	if (len == sizeof(com_token))
	{
//		appWPrintf("Token exceeded %d chars, discarded.\n", sizeof(com_token));
		len = 0;
	}
	com_token[len] = 0;

	data_p = data;
	return com_token;
}

//?? its my own function - move, rename ...
const char *COM_QuoteString(const char *str, bool alwaysQuote)
{
	char	*dst, c;

	if (!alwaysQuote)
	{
		const char *s;

		s = str;
		while (c = *s++)
			if (c == ' ' || c == '\"' || c == ';') break;
		if (!c) return str;				// line have no chars, which requires quoting
	}

	dst = com_token;
	*dst++ = '\"';
	while (c = *str++)
	{
		*dst++ = c;
		if (c == '\"') *dst++ = c;
	}
	*dst++ = '\"';
	*dst = 0;

	return com_token;
}


/*-----------------------------------------------------------------------------
	Commandline parsing
-----------------------------------------------------------------------------*/


#define MAX_CMDLINE_PARTS	64

static char *cmdlineParts[MAX_CMDLINE_PARTS];
static int cmdlineNumParts;

static void ParseCmdline(const char *cmdline)
{
	guard(ParseCmdline);

	// parse command line; fill cmdlineParts[] array
	static char buf[512];
	buf[0] = 0;						// starts with 0 (needs for trimming trailing spaces)
	char *dst = &buf[1];
	while (true)
	{
		char c;
		while ((c = *cmdline) == ' ') cmdline++;	// skip spaces
		if (!c) break;				// end of commandline

		if (c != '-')
		{
			// bad argument
			appPrintf("ParseCmdline: bad argument \"");
			do
			{
				appPrintf("%c", c);
				c = *++cmdline;
			} while (c != ' ' && c != 0);
			appPrintf("\"\n");
			continue;				// try next argument
		}

		// cmdline points to start of possible command
		if (cmdlineNumParts == MAX_CMDLINE_PARTS)
		{
			appWPrintf("ParseCmdline: overflow\n");
			break;
		}
		cmdlineParts[cmdlineNumParts++] = dst;
		int quotes = 0;
		while (c = *++cmdline)
		{
			if (c == '\"')
				quotes ^= 1;
			else if (c == '-' && !quotes)
			{
				char prev = *(dst-1);
				if (prev == 0 || prev == ' ')
					break;
			}
			*dst++ = c;
		}
		while (*(dst-1) == ' ') dst--;

		*dst++ = 0;
//		appPrintf("arg[%d] = <%s>\n", cmdlineNumParts-1, cmdlineParts[cmdlineNumParts-1]);
	}

	// immediately exec strings of type "var=value"
	for (int i = 0; i < cmdlineNumParts; i++)
	{
		const char *cmd = cmdlineParts[i];
		const char *s1 = strchr(cmd, '=');
		const char *s2 = strchr(cmd, '\"');
		if (s1 && (!s2 || s2 > s1))		// a=b, but '=' not inside quotes
		{
			char	varName[64], varValue[256];
			// convert to "set a b"
			appStrncpyz(varName, cmd, s1 - cmd + 1);	// copy "a"
			appStrncpyz(varValue, s1 + 1, sizeof(varValue));
			int len = strlen(varValue);
			const char *value;
			if (varValue[0] == '\"' && varValue[len-1] == '\"')
			{
				// remove quotes
				value = varValue + 1;
				varValue[len-1] = 0;
			}
			else
				value = varValue;
			cvar_t *var = Cvar_Set(varName, value);
			if (var)
				var->flags |= CVAR_CMDLINE;
			else
				appWPrintf("ParseCmdline: unable to set \"%s\"\n", varName);

			cmdlineParts[i] = NULL;		// remove command
		}
	}

	unguard;
}


bool Com_CheckCmdlineVar(const char *name)
{
	for (int i = 0; i < cmdlineNumParts; i++)
	{
		const char *cmd = cmdlineParts[i];
		if (!cmd || strchr(cmd, ' ')) continue;	// already removed or contains arguments
		if (!stricmp(name, cmd))
		{
			cmdlineParts[i] = NULL;
			return true;
		}
	}
	return false;
}


static void PushCmdline()
{
	for (int i = 0; i < cmdlineNumParts; i++)
	{
		const char *cmd = cmdlineParts[i];
		if (!cmd) continue;				// already removed
		Cbuf_AddText(va("%s\n", cmd));
		cmdlineParts[i] = NULL;			// just in case
	}
}


/*-----------------------------------------------------------------------------
	Initialization
-----------------------------------------------------------------------------*/

void Com_Init(const char *cmdline)
{
CVAR_BEGIN(vars)
	CVAR_VAR(com_speeds, 0, 0),
#if !NO_DEVELOPER
	CVAR_VAR(developer, 0, 0),
#else
	CVAR_VAR(developer, 0, CVAR_NOSET),
#endif
	CVAR_VAR(timescale, 1, CVAR_CHEAT),
	CVAR_VAR(timedemo, 0, CVAR_CHEAT),
	CVAR_VAR(nointro, 0, CVAR_NOSET),
	CVAR_FULL(&logfile_active, "logfile", "0", 0),
	CVAR_FULL(&sv_cheats, "cheats", "0", CVAR_SERVERINFO|CVAR_LATCH),
#if DEDICATED_ONLY
	CVAR_VAR(dedicated, 1, CVAR_NOSET)
#else
	CVAR_VAR(dedicated, 0, CVAR_NOSET)
#endif
CVAR_END

	guard(Com_Init);

	ParseCmdline(cmdline);			// should be executed before any cvar creation
	Cvar_GetVars(ARRAY_ARG(vars));
	Cvar_Get("version", VERSION_STR, CVAR_SERVERINFO|CVAR_NOSET);

	Key_Init();

	InitByteDirs();

	InitFileSystem();

	Cbuf_Execute();
	cvar_t::initialized = 1;		// config executed -- allow cmdline cvars to be modified

	if (DEDICATED)
	{
		// require this for dedicated server, when launching common executable (!DEDICATED_ONLY)
		// (executed Con_Print() -> Con_CheckResize() function):
		GScreenWidth = 80;
	}

	NET_Init();
	Netchan_Init();

	SV_Init();
	if (!DEDICATED) CL_Init();

	// initialize rand() functions
	srand(appMilliseconds());

	if (nointro->integer == 0)
	{	// if the user didn't give any commands, run default action
		if (!DEDICATED)
		{
			Cbuf_AddText("d1\n");
			Cbuf_Execute();
			if (!Com_ServerState())		// this alias not exists or not starts demo
				Cvar_ForceSet("nointro", "1");
		}
		else
		{
			Cbuf_AddText("dedicated_start\n");
			Cbuf_Execute();
		}
	}

	cvar_t::initialized = 2;
	PushCmdline();
	appPrintf(S_GREEN"\n====== " APPNAME " Initialized ======\n\n");

	unguard;
}


extern	int	c_traces, c_pointcontents;


#define SV_PROFILE		1	//?????

bool com_isClient, com_isServer;

void Com_Frame(float msec)
{
	guard(Com_Frame);

	// logfile
	//?? move to Core, to appPrintf() ?
	//?? appUncolorizeString() for file log
	if (logfile_active && logfile_active->modified)
	{
		logfile_active->modified = false;
		if (logfile_active->integer)
		{
			if (!FileLog)
			{
				FileLog = new COutputDeviceFile(APPNAME".log", true);
				FileLog->Register();
			}
			FileLog->FlushEveryTime = logfile_active->integer > 1;
		}
		else
		{
			if (FileLog)
			{
				FileLog->Unregister();
				delete FileLog;
				FileLog = NULL;
			}
		}
	}

	//?? ignore timescale in multiplayer and timedemo in non-demo mode
	float smsec;								// scaled msec (for timescale or timedemo)
	if (timedemo->integer)
		smsec = 100.0f / timedemo->integer;		// sv_fps ?
	else // if (timescale->value)
		smsec = msec * timescale->value;
	if (smsec < 0) smsec = 0;					// no reverse time

	c_traces = 0;
	c_pointcontents = 0;

	if (DEDICATED)
	{
		while (true)
		{
			const char *s = Sys_ConsoleInput();
			if (!s) break;
			Cbuf_AddText(va("%s\n",s));
		}
	}
	Cbuf_Execute();
	FS_Tick();

	unsigned time_before, time_between, time_after;
	time_before = appCycles();
	time_before_game = 0;					// for detection of frames with game frame

	com_isServer = true;
	SV_Frame(smsec);
	com_isServer = false;

	if (!DEDICATED)
	{
		time_between = appCycles();
		time_before_ref = 0;				// for detection of frames with renderer frame (may be dropped with cl_maxfps)

		com_isClient = true;
		CL_Frame(smsec, msec);
		com_isClient = false;

		if (com_speeds->integer)
		{
			static float old_gm;
			static int old_tr, old_pc;

			time_after = appCycles();

			float rf  = (time_before_ref) ? appCyclesToMsecf(time_after_ref - time_before_ref) : 0;
			float gm  = (time_before_game) ? appCyclesToMsecf(time_after_game - time_before_game) : 0;
			float sv  = appCyclesToMsecf(time_between - time_before) - gm;
			float cl  = appCyclesToMsecf(time_after - time_between) - rf;
			float all = appCyclesToMsecf(time_after - time_before);
			if (time_before_game)			// have a valid game frame
			{
				old_gm = gm;
				old_tr = c_traces;
				old_pc = c_pointcontents;
			}
			if (time_before_ref)
				RE_DrawTextRight(va("sv:%5.2f gm:%5.2f (%5.2f) cl:%5.2f rf:%5.2f all:%5.2f\n"
									 "tr: %4d (%4d) pt: %4d (%4d)",
									sv, gm, old_gm, cl, rf, all,
									c_traces, old_tr, c_pointcontents, old_pc),
									RGB(1, 0.8, 0.3));

#if SV_PROFILE
			if (1)	//?? cvar; com_speeds==2 ? g_speeds ?
			{
				extern unsigned prof_times[256];
				extern unsigned prof_counts[256];
				static char *names[] = {"LinkEdict", "UnlinkEdict", "AreaEdicts", "Trace",
					"PtContents", "Pmove", "ModIndex", "ImgIndex", "SndIndex",
					"Malloc", "Free", "FreeTags"};
				static unsigned counts[12], times[12];

				for (int i = 0; i < ARRAY_COUNT(names); i++)
				{
					if (time_before_ref)
						RE_DrawTextLeft(va("%11s: %3d %.3f", names[i], counts[i], appCyclesToMsecf(times[i])), RGB(1, 0.8, 0.3));
					if (time_before_game)
					{
						counts[i] = prof_counts[i];
						times[i]  = prof_times[i];
						prof_counts[i] = prof_times[i] = 0;
					}
				}
			}
#endif
		}
	}

	unguard;
}
