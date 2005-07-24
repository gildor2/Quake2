/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// common.cpp -- misc functions used in client and server
#include "qcommon.h"
#include <time.h>

#define	MAXPRINTMSG	4096

int		realtime;

#include "../client/ref.h"	// using RE_DrawTextXxx () for com_speeds


cvar_t	*com_speeds;
cvar_t	*developer;
cvar_t	*timescale;
cvar_t	*timedemo;
cvar_t	*sv_cheats;

#ifndef DEDICATED_ONLY
static cvar_t dummyDedicated;
cvar_t	*dedicated = &dummyDedicated;
	// dummy cvar; may be used before initialized - when error happens before
	// Com_Init()::Cvar_GetVars(); after this, will point to actual cvar
#else
cvar_t	*dedicated;
#endif

static cvar_t	*logfile_active;		// 1 = buffer log, 2 = flush after each print
static FILE	*logfile;

static server_state_t server_state;

// com_speeds times
unsigned time_before_game, time_after_game, time_before_ref, time_after_ref;

int		linewidth = 80;

/*
============================================================================

CLIENT / SERVER interactions

============================================================================
*/

static char	*rd_buffer;
static int	rd_buffersize;
static void	(*rd_flush)(char *buffer);


void Com_BeginRedirect (char *buffer, int buffersize, void (*flush)(char*))
{
	if (!buffer || !buffersize || !flush)
		return;
	rd_buffer = buffer;
	rd_buffersize = buffersize;
	rd_flush = flush;

	*rd_buffer = 0;
}

void Com_EndRedirect (void)
{
	rd_flush (rd_buffer);

	rd_buffer = NULL;
	rd_buffersize = 0;
	rd_flush = NULL;
}


/*
=============
Com_Printf

Both client and server can use this, and it will output
to the apropriate place.
=============
*/

static bool console_logged = false;
static char log_name[MAX_OSPATH];


void Com_Printf (const char *fmt, ...)
{
	va_list	argptr;
	char	msg[MAXPRINTMSG];

	guard(Com_Printf);

	va_start (argptr,fmt);
	vsnprintf (ARRAY_ARG(msg),fmt,argptr);
	va_end (argptr);

	if (rd_buffer)
	{
		if ((strlen (msg) + strlen(rd_buffer)) > rd_buffersize - 1)
		{
			rd_flush (rd_buffer);
			*rd_buffer = 0;
		}
		strcat (rd_buffer, msg);
		return;
	}

	if (!DEDICATED)
		Con_Print (msg);

	// also echo to debugging console (in dedicated server mode only, when win32)
	Sys_ConsoleOutput (msg);

	// logfile
	if (logfile_active && logfile_active->integer)
	{
		const char *s = strchr (msg, '\r');		// find line-feed
		if (s)	s++;
		else	s = msg;
		if (!s[0]) return;						// nothing to print
		if (!logfile)
		{
			logfile = fopen ("console.log", logfile_active->integer > 2 || console_logged ? "a" : "w");
			if (!logfile)
			{
				Cvar_SetInteger ("logfile", 0);
				Com_WPrintf ("Unable to create console.log\n");
			}
		}

		if (logfile)
		{
			appUncolorizeString (msg, msg);
			fprintf (logfile, "%s", msg);
			if (logfile_active->integer > 1)	// force to save every message
			{
				fclose (logfile);
				logfile = NULL;
			}
		}
		console_logged = true;
	}

	unguard;
}


#ifndef NO_DEVELOPER
/*
================
Com_DPrintf

A Com_Printf that only shows up if the "developer" cvar is set
(prints on a screen with a different color)
When developer is set to 2, logging the message.
When developer is set to 256, do not colorize message.
================
*/
void Com_DPrintf (const char *fmt, ...)
{
	va_list	argptr;
	char	msg[MAXPRINTMSG];

	if (!DEVELOPER) return;

	va_start (argptr,fmt);
	vsnprintf (ARRAY_ARG(msg),fmt,argptr);
	va_end (argptr);
	Com_Printf (S_BLUE"%s", msg);
	if (developer->integer == 2)
	{
		appUncolorizeString (msg, msg);
		DebugPrintf ("%s", msg);
	}
}
#endif


bool debugLogged = false;

void DebugPrintf (const char *fmt, ...)
{
	va_list	argptr;
	char	msg[1024], ctime[256];
	FILE	*log;
	time_t	itime;

	va_start (argptr,fmt);
	vsnprintf (ARRAY_ARG(msg),fmt,argptr);
	va_end (argptr);

	log = fopen ("debug.log", "a+");
	if (!debugLogged)
	{
		time (&itime);
		strftime (ARRAY_ARG(ctime), "%a %b %d, %Y (%H:%M:%S)", localtime (&itime));
		fprintf (log, "\n\n----- " APPNAME " debug log on %s -----\n", ctime);
		debugLogged = true;
	}
	fprintf (log, "%s", msg);
	fclose (log);
}


/*
================
Com_WPrintf

Printf non-fatal error message (warning) using a different color
When developer is set to 2, logging the message
================
*/

void Com_WPrintf (const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	va_start (argptr,fmt);
	vsnprintf (ARRAY_ARG(msg),fmt,argptr);
	va_end (argptr);

	Com_Printf (S_YELLOW"%s", msg);
#ifndef NO_DEVELOPER
	if (developer->integer == 2) DebugPrintf ("%s", msg);
#endif
}


/*
=============
Com_Quit

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void Com_Quit (void)
{
	SV_Shutdown ("Server quit\n", false);
	CL_Shutdown (false);
	Com_Shutdown ();
	Sys_Quit ();
}


static unsigned GetInt (char **str)
{
	unsigned r;
	char	c, *s;

	r = 0;
	s = *str;
	while (1)
	{
		c = *s;
		if (c < '0' || c > '9') break;
		r = r * 10 + c - '0';
		s++;
	}
	*str = s;
	return r;
}

bool IPWildcard (netadr_t *a, char *mask)
{
	int		i, n;
	char	*m;

	if (a->type != NA_IP) return false;
	m = mask;

	for (i = 0; i < 4; i++)
	{
		if (m[0] == '*')
			m++;			// skip '*'
		else if (m[0] >= '0' && m[0] <= '9')
		{
			n = GetInt (&m);
			if (m[0] == '.' || m[0] == 0)
			{
				if (a->ip[i] != n) return false;
			}
			else if (m[0] == '-')
			{
				if (a->ip[i] < n) return false;
				m++;
				n = GetInt (&m);
				if (a->ip[i] > n) return false;
			}
			else
			{
				Com_DPrintf ("IPWildcard: bad char in \"%s\"\n", mask);
				return false;
			}
		}
		else
		{
			Com_DPrintf ("IPWildcard: bad char in \"%s\"\n", mask);
			return false;
		}
		if (m[0] == 0 && i < 3)
		{
			Com_DPrintf ("IPWildcard: short mask \"%s\"\n", mask);
			return true;
		}
		m++;
	}
	return true;
}


/*
==================
Com_ServerState
==================
*/
server_state_t Com_ServerState (void)
{
	return server_state;
}

/*
==================
Com_SetServerState
==================
*/
void Com_SetServerState (server_state_t state)
{
	server_state = state;
}


#if 1
// 0 to 1
float frand (void)
{
	return rand() * (1.0f/RAND_MAX);
}

// -1 to 1
float crand (void)
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
const char *Info_ValueForKey (const char *s, const char *key)
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

		if (!strcmp (key, pkey))
			return va("%s", value);

		if (!*s) return "";
		s++;
	}
}

static void Info_RemoveKey (char *s, const char *key)
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

		if (!strcmp (key, pkey))
		{
			strcpy (start, s);	// remove this part
			return;
		}

		if (!*s) return;
		s++;
	}

}


void Info_SetValueForKey (char *s, const char *key, const char *value)
{
	char	newi[MAX_INFO_STRING], *v;
	int		maxsize = MAX_INFO_STRING;

	if (strchr (key, '\\') || strchr (value, '\\'))
	{
		Com_WPrintf ("Info_Set(\"%s\",\"%s\"): can't use keys or values with a '\\'\n", key, value);
		return;
	}

#if 0
	if (strchr (key, '\"') || strchr (value, '\"'))
	{
		Com_WPrintf ("Can't use keys or values with a '\"'\n");
		return;
	}

	if (strchr (key, ';'))
	{
		Com_WPrintf ("Can't use keys or values with a semicolon\n");
		return;
	}
#endif

	if (strlen (key) > MAX_INFO_KEY - 1 || strlen (value) > MAX_INFO_KEY - 1)
	{
		Com_WPrintf ("Keys and values must be < " STR(MAX_INFO_KEY) " characters\n");
		return;
	}
	Info_RemoveKey (s, key);
	if (!value || !value[0])
		return;

	int size = appSprintf (ARRAY_ARG(newi), "\\%s\\%s", key, value);
	if (strlen (s) + size > maxsize)
	{
		Com_WPrintf ("Info string length exceeded\n");
		return;
	}

	// only copy ASCII values
	s += strlen (s);
	v = newi;
	while (*v)
	{
		char c = *v++ & 127;		// strip high bits
		if (c >= 32) *s++ = c;
	}
	*s = 0;
}


void Info_Print (const char *s)
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
		Com_Printf (S_GREEN"%-22s", key);

		if (!*s)
		{
			Com_WPrintf ("missing value\n");
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;
		Com_Printf ("%s\n", value);

		if (!*s) return;
		s++;
	}
}


/*-----------------------------------------------------------------------------
	Commandline parsing
-----------------------------------------------------------------------------*/


#define MAX_CMDLINE_PARTS	64

static char *cmdlineParts[MAX_CMDLINE_PARTS];
static int cmdlineNumParts;

static void ParseCmdline (const char *cmdline)
{
	char	c, *dst;
	static char buf[512];
	int		i, quotes;

	guard(ParseCmdline);

	// parse command line; fill cmdlineParts[] array
	buf[0] = 0;						// starts with 0 (needs for trimming trailing spaces)
	dst = &buf[1];
	while (true)
	{
		while ((c = *cmdline) == ' ') cmdline++;	// skip spaces
		if (!c) break;				// end of commandline

		if (c != '-')
		{
			// bad argument
			Com_Printf ("ParseCmdline: bad argument \"");
			do
			{
				Com_Printf ("%c", c);
				c = *++cmdline;
			} while (c != ' ' && c != 0);
			Com_Printf ("\"\n");
			continue;				// try next argument
		}

		// cmdline points to start of possible command
		if (cmdlineNumParts == MAX_CMDLINE_PARTS)
		{
			Com_WPrintf ("ParseCmdline: overflow\n");
			break;
		}
		cmdlineParts[cmdlineNumParts++] = dst;
		quotes = 0;
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
//		Com_Printf("arg[%d] = <%s>\n", cmdlineNumParts-1, cmdlineParts[cmdlineNumParts-1]);
	}

	// immediately exec strings of type "var=value"
	for (i = 0; i < cmdlineNumParts; i++)
	{
		char	*cmd, *s1, *s2;

		cmd = cmdlineParts[i];
		s1 = strchr (cmd, '=');
		s2 = strchr (cmd, '\"');
		if (s1 && (!s2 || s2 > s1))		// a=b, but '=' not inside quotes
		{
			char	varName[64], varValue[256];
			char	*value;
			int		len;
			cvar_t	*var;

			// convert to "set a b"
			appStrncpyz (varName, cmd, s1 - cmd + 1);	// copy "a"
			appStrncpyz (varValue, s1 + 1, sizeof(varValue));
			len = strlen (varValue);
			if (varValue[0] == '\"' && varValue[len-1] == '\"')
			{
				// remove quotes
				value = varValue + 1;
				varValue[len-1] = 0;
			}
			else
				value = varValue;
			var = Cvar_Set (varName, value);
			if (var)
				var->flags |= CVAR_CMDLINE;
			else
				Com_WPrintf ("ParseCmdline: unable to set \"%s\"\n", varName);

			cmdlineParts[i] = NULL;		// remove command
		}
	}

	unguard;
}


bool Com_CheckCmdlineVar (const char *name)
{
	int		i;
	char	*cmd;

	for (i = 0; i < cmdlineNumParts; i++)
	{
		cmd = cmdlineParts[i];
		if (!cmd || strchr (cmd, ' ')) continue;	// already removed or contains arguments
		if (!stricmp (name, cmd))
		{
			cmdlineParts[i] = NULL;
			return true;
		}
	}
	return false;
}


static void PushCmdline (void)
{
	int		i;
	char	*cmd;

	for (i = 0; i < cmdlineNumParts; i++)
	{
		cmd = cmdlineParts[i];
		if (!cmd) continue;				// already removed
		Cbuf_AddText (va("%s\n", cmd));
		cmdlineParts[i] = NULL;			// just in case
	}
}


/*-----------------------------------------------------------------------------
	Initialization
-----------------------------------------------------------------------------*/

static cvar_t	*nointro;


void Com_Init (const char *cmdline)
{
CVAR_BEGIN(vars)
	CVAR_VAR(com_speeds, 0, 0),
#ifndef NO_DEVELOPER
	CVAR_VAR(developer, 0, 0),
#else
	CVAR_VAR(developer, 0, CVAR_NOSET),
#endif
	CVAR_VAR(timescale, 1, CVAR_CHEAT),
	CVAR_VAR(timedemo, 0, CVAR_CHEAT),
	CVAR_VAR(nointro, 0, CVAR_NOSET),
	CVAR_FULL(&logfile_active, "logfile", "0", 0),
	CVAR_FULL(&sv_cheats, "cheats", "0", CVAR_SERVERINFO|CVAR_LATCH),
#ifdef DEDICATED_ONLY
	CVAR_VAR(dedicated, 1, CVAR_NOSET)
#else
	CVAR_VAR(dedicated, 0, CVAR_NOSET)
#endif
CVAR_END

	guard(Com_Init);

	Swap_Init ();
	Cvar_Init ();

	ParseCmdline (cmdline);			// should be executed before any cvar creation
	Cvar_GetVars (ARRAY_ARG(vars));
	Cvar_Get ("version", VERSION_STR, CVAR_SERVERINFO|CVAR_NOSET);

	Cmd_Init ();
	Key_Init ();

	InitByteDirs ();

	FS_InitFilesystem ();

	FS_LoadGameConfig ();
	Cbuf_Execute ();
	cvar_initialized = 1;			// config executed -- allow cmdline cvars to be modified

	if (DEDICATED)
	{
		RegisterCommand ("quit", Com_Quit);
		linewidth = 80;
	}

	NET_Init ();
	Netchan_Init ();

	SV_Init ();
	if (!DEDICATED) CL_Init ();

	// initialize rand() functions
	srand (appMilliseconds ());

	if (nointro->integer == 0)
	{	// if the user didn't give any commands, run default action
		if (!DEDICATED)
		{
			Cbuf_AddText ("d1\n");
			Cbuf_Execute ();
			if (!Com_ServerState ())		// this alias not exists or not starts demo
				Cvar_ForceSet ("nointro", "1");
		}
		else
		{
			Cbuf_AddText ("dedicated_start\n");
			Cbuf_Execute ();
		}
	}

	cvar_initialized = 2;
	PushCmdline ();
	Com_Printf (S_GREEN"\n====== " APPNAME " Initialized ======\n\n");

	unguard;
}


extern	int	c_traces, c_pointcontents;


#define SV_PROFILE		//?????

void Com_Frame (float msec)
{
	unsigned time_before, time_between, time_after;

	guard(Com_Frame);

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
			const char *s = Sys_ConsoleInput ();
			if (!s) break;
			Cbuf_AddText (va("%s\n",s));
		}
	}
	Cbuf_Execute ();

	if (com_speeds->integer)
	{
		time_before = appCycles ();
		time_before_game = 0;				// for detection of frames with game frame
	}

//if (!Cvar_VariableInt("sv_block"))
	SV_Frame (smsec);

	if (!DEDICATED)
	{
		if (com_speeds->integer)
		{
			time_between = appCycles ();
			time_before_ref = 0;			// for detection of frames with renderer frame (may be dropped with cl_maxfps)
		}

		CL_Frame (smsec, msec);

		if (com_speeds->integer)
		{
			static float old_gm;
			static int old_tr, old_pc;

			time_after = appCycles ();

			float rf = (time_before_ref) ? appDeltaCyclesToMsecf (time_after_ref - time_before_ref) : 0;
			float gm = (time_before_game) ? appDeltaCyclesToMsecf (time_after_game - time_before_game) : 0;
			float sv = appDeltaCyclesToMsecf (time_between - time_before) - gm;
			float cl = appDeltaCyclesToMsecf (time_after - time_between) - rf;
			float all = appDeltaCyclesToMsecf (time_after - time_before);
			if (time_before_game)			// have a valid game frame
			{
				old_gm = gm;
				old_tr = c_traces;
				old_pc = c_pointcontents;
			}
			if (time_before_ref)
				RE_DrawTextRight (va("sv:%.2f gm:%.2f (%.2f) cl:%.2f rf:%.2f all:%.2f\n"
									 "tr: %4d (%4d) pt: %4d (%4d)",
									sv, gm, old_gm, cl, rf, all,
									c_traces, old_tr, c_pointcontents, old_pc),
									RGB(1, 0.8, 0.3));

#ifdef SV_PROFILE
			if (1)	//?? cvar; com_speeds==2 ? g_speeds ?
			{
				extern unsigned prof_times[256];
				extern unsigned prof_counts[256];
				static char *names[] = {"LinkEdict", "UnlinkEdict", "AreaEdicts", "Trace",
					"PtContents", "Pmove", "ModIndex", "ImgIndex", "SndIndex",
					"Malloc", "Free", "FreeTags"};
				static unsigned counts[12], times[12];
				static unsigned cyc, lastCycles, lastMsec;

				cyc = appCycles ();
				for (int i = 0; i < ARRAY_COUNT(names); i++)
				{
					if (time_before_ref)
						RE_DrawTextLeft (va("%11s: %3d %.3f", names[i], counts[i], appDeltaCyclesToMsecf (times[i])), RGB(1, 0.8, 0.3));
					if (time_before_game)
					{
						counts[i] = prof_counts[i];
						times[i] = prof_times[i];
						prof_counts[i] = prof_times[i] = 0;
					}
				}
				lastCycles = cyc;
			}
#endif
		}
	}

	unguard;
}


void Com_Shutdown (void)
{
	if (logfile)
	{
		fclose (logfile);
		logfile = NULL;
	}
}
