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

// qcommon.h -- definitions common between client and server, but not game.dll

#ifndef QCOMMON_H
#define QCOMMON_H

#include "Core.h"

//!!!! temp:
#define Com_FatalError	appFatalError
#define Com_DropError	appNonFatalError
#define Sys_Error		appFatalError

#include "q_shared2.h"
#include "qfiles.h"


#define	VERSION		4.10

#define APPNAME		"Quake2"
#define	BASEDIRNAME	"baseq2"
#define CONFIGNAME	"config.cfg"
// if CD_PATH is not defined, CD checking and adding its paths will not be performed
#define CD_PATH		"install/data"
#define CD_CHECK	"install/data/quake2.exe"	// file used for CD validation

#define DO_GUARD		1
#define DO_GUARD_SLOW	0

#define NEW_PROTOCOL_ID "gildor"

#ifdef WIN32

//?????? DEBUG:
//#ifdef NDEBUG
#define BUILDSTRING "Win32 RELEASE"
//#else
//#define BUILDSTRING "Win32 DEBUG"
//#endif

#ifdef _M_IX86
#define	CPUSTRING	"x86"
#elif defined _M_ALPHA
#define	CPUSTRING	"AXP"
#endif

#elif defined __linux__

#define BUILDSTRING "Linux"

#ifdef __i386__
#define CPUSTRING "i386"
#elif defined __alpha__
#define CPUSTRING "axp"
#else
#define CPUSTRING "Unknown"
#endif

#elif defined __sun__

#define BUILDSTRING "Solaris"

#ifdef __i386__
#define CPUSTRING "i386"
#else
#define CPUSTRING "sparc"
#endif

#else	// unknown platform

#define BUILDSTRING "Unknown"
#define	CPUSTRING	"Unknown"

#endif


#define SAVEGAME_DIRECTORY			"save"
#define SAVEGAME_SERVER_EXTENSION	"sv2"
#define SAVEGAME_GAME_EXTENSION		"sav"
#define SAVEGAME_VARS_EXTENSION		"ssv"


#ifndef DEDICATED_ONLY
#define DEDICATED	dedicated->integer
#else
#define DEDICATED	1
#endif

//============================================================================

typedef struct sizebuf_s
{
	bool	allowoverflow;		// if false, do a error when overflowed
	bool	overflowed;			// set to true if the buffer size failed
	byte	*data;
	int		maxsize;
	int		cursize;
	int		readcount;
} sizebuf_t;

void	SZ_Init (sizebuf_t *buf, void *data, int length);
void	SZ_Clear (sizebuf_t *buf);
void	*SZ_GetSpace (sizebuf_t *buf, int length);
void	SZ_Write (sizebuf_t *buf, void *data, int length);
void	SZ_Insert (sizebuf_t *buf, void *data, int length, int pos);	// used for Cbuf_InsertText() only
void	SZ_Print (sizebuf_t *buf, char *data);	// strcats onto the sizebuf

//============================================================================


void	MSG_WriteChar (sizebuf_t *sb, int c);
void	MSG_WriteByte (sizebuf_t *sb, int c);
void	MSG_WriteShort (sizebuf_t *sb, int c);
void	MSG_WriteLong (sizebuf_t *sb, int c);
void	MSG_WriteFloat (sizebuf_t *sb, float f);
void	MSG_WriteString (sizebuf_t *sb, char *s);
void	MSG_WriteCoord (sizebuf_t *sb, float f);
void	MSG_WritePos (sizebuf_t *sb, vec3_t pos);
void	MSG_WriteAngle (sizebuf_t *sb, float f);
void	MSG_WriteAngle16 (sizebuf_t *sb, float f);
void	MSG_WriteDeltaUsercmd (sizebuf_t *sb, usercmd_t *from, usercmd_t *cmd);
void	MSG_WriteDeltaEntity (entity_state_t *from, entity_state_t *to, sizebuf_t *msg, bool force, bool newentity);
void	MSG_WriteDir (sizebuf_t *sb, vec3_t vector);


void	MSG_BeginReading (sizebuf_t *sb);

int		MSG_ReadChar (sizebuf_t *sb);
int		MSG_ReadByte (sizebuf_t *sb);
int		MSG_ReadShort (sizebuf_t *sb);
int		MSG_ReadLong (sizebuf_t *sb);
float	MSG_ReadFloat (sizebuf_t *sb);
char	*MSG_ReadString (sizebuf_t *sb);

float	MSG_ReadCoord (sizebuf_t *sb);
void	MSG_ReadPos (sizebuf_t *sb, vec3_t pos);
float	MSG_ReadAngle (sizebuf_t *sb);
float	MSG_ReadAngle16 (sizebuf_t *sb);
void	MSG_ReadDeltaUsercmd (sizebuf_t *sb, usercmd_t *from, usercmd_t *cmd);

void	MSG_ReadDir (sizebuf_t *sb, vec3_t vector);

void	MSG_ReadData (sizebuf_t *sb, void *buffer, int size);

//============================================================================


//
// key / value info strings
//
#define	MAX_INFO_KEY		64
#define	MAX_INFO_VALUE		64
#define	MAX_INFO_STRING		512

char*	Info_ValueForKey (char *s, char *key);
void	Info_RemoveKey (char *s, char *key);
void	Info_SetValueForKey (char *s, char *key, char *value);
void	Info_Print (char *s);


/* crc.h */

void	CRC_Init(unsigned short *crcvalue);
void	CRC_ProcessByte(unsigned short *crcvalue, byte data);
unsigned short CRC_Value(unsigned short crcvalue);
unsigned short CRC_Block (byte *start, int count);



/*
==============================================================

PROTOCOL

==============================================================
*/

// declaration from server.h
typedef enum {
	ss_dead,
	ss_loading,
	ss_game,
	ss_cinematic,
	ss_demo,
	ss_pic
} server_state_t;
// some qc commands are only valid before the server has finished
// initializing (precache commands, static sounds / objects, etc)


// protocol.h -- communications protocols

#define	PROTOCOL_VERSION	34

//=========================================

#define	PORT_MASTER	27900
#define	PORT_CLIENT	27901
#define	PORT_SERVER	27910

//=========================================

#define	UPDATE_BACKUP	16	// copies of entity_state_t to keep buffered
							// must be power of two
#define	UPDATE_MASK		(UPDATE_BACKUP-1)



//==================
// the svc_strings[] array in cl_parse.c should mirror this
//==================

//
// server to client
//
enum svc_ops_e
{
	svc_bad,

	// these ops are known to the game dll
	svc_muzzleflash,
	svc_muzzleflash2,
	svc_temp_entity,
	svc_layout,
	svc_inventory,

	// the rest are private to the client and server
	svc_nop,
	svc_disconnect,
	svc_reconnect,
	svc_sound,					// <see code>
	svc_print,					// [byte] id [string] null terminated string
	svc_stufftext,				// [string] stuffed into client's console buffer, should be \n terminated
	svc_serverdata,				// [long] protocol ...
	svc_configstring,			// [short] [string]
	svc_spawnbaseline,
	svc_centerprint,			// [string] to put in center of the screen
	svc_download,				// [short] size [size bytes]
	svc_playerinfo,				// variable
	svc_packetentities,			// [...]
	svc_deltapacketentities,	// [...]
	svc_frame,

	svc_last					// number of commands
};

//==============================================

//
// client to server
//
enum clc_ops_e
{
	clc_bad,
	clc_nop,
	clc_move,					// [[usercmd_t]
	clc_userinfo,				// [[userinfo string]
	clc_stringcmd				// [string] message
};

//==============================================

// plyer_state_t communication

#define	PS_M_TYPE			(1<<0)
#define	PS_M_ORIGIN			(1<<1)
#define	PS_M_VELOCITY		(1<<2)
#define	PS_M_TIME			(1<<3)
#define	PS_M_FLAGS			(1<<4)
#define	PS_M_GRAVITY		(1<<5)
#define	PS_M_DELTA_ANGLES	(1<<6)

#define	PS_VIEWOFFSET		(1<<7)
#define	PS_VIEWANGLES		(1<<8)
#define	PS_KICKANGLES		(1<<9)
#define	PS_BLEND			(1<<10)
#define	PS_FOV				(1<<11)
#define	PS_WEAPONINDEX		(1<<12)
#define	PS_WEAPONFRAME		(1<<13)
#define	PS_RDFLAGS			(1<<14)

//==============================================

// user_cmd_t communication

// ms and light always sent, the others are optional
#define	CM_ANGLE1 			(1<<0)
#define	CM_ANGLE2		 	(1<<1)
#define	CM_ANGLE3 			(1<<2)
#define	CM_FORWARD			(1<<3)
#define	CM_SIDE				(1<<4)
#define	CM_UP				(1<<5)
#define	CM_BUTTONS			(1<<6)
#define	CM_IMPULSE			(1<<7)

//==============================================

// a sound without an ent or pos will be a local only sound
#define	SND_VOLUME			(1<<0)		// a byte
#define	SND_ATTENUATION		(1<<1)		// a byte
#define	SND_POS				(1<<2)		// three coordinates
#define	SND_ENT				(1<<3)		// a short 0-2: channel, 3-12: entity
#define	SND_OFFSET			(1<<4)		// a byte, msec offset from frame start

#define DEFAULT_SOUND_PACKET_VOLUME			1.0
#define DEFAULT_SOUND_PACKET_ATTENUATION	1.0

//==============================================

// entity_state_t communication

#define U_MODEL_N	(U_MODEL|U_MODEL2|U_MODEL3|U_MODEL4)
#define U_ORIGIN_N	(U_ORIGIN1|U_ORIGIN2|U_ORIGIN3)
#define U_ANGLE_N	(U_ANGLE1|U_ANGLE2|U_ANGLE3)

// try to pack the common update flags into the first byte
#define	U_ORIGIN1			(1<<0)
#define	U_ORIGIN2			(1<<1)
#define	U_ANGLE2			(1<<2)
#define	U_ANGLE3			(1<<3)
#define	U_FRAME8			(1<<4)		// frame is a byte
#define	U_EVENT				(1<<5)
#define	U_REMOVE			(1<<6)		// REMOVE this entity, don't add it
#define	U_MOREBITS1			(1<<7)		// read one additional byte

// second byte
#define	U_NUMBER16			(1<<8)		// NUMBER8 is implicit if not set
#define	U_ORIGIN3			(1<<9)
#define	U_ANGLE1			(1<<10)
#define	U_MODEL				(1<<11)
#define U_RENDERFX8			(1<<12)		// fullbright, etc
#define	U_EFFECTS8			(1<<14)		// autorotate, trails, etc
#define	U_MOREBITS2			(1<<15)		// read one additional byte

// third byte
#define	U_SKIN8				(1<<16)
#define	U_FRAME16			(1<<17)		// frame is a short
#define	U_RENDERFX16		(1<<18)		// 8 + 16 = 32
#define	U_EFFECTS16			(1<<19)		// 8 + 16 = 32
#define	U_MODEL2			(1<<20)		// weapons, flags, etc
#define	U_MODEL3			(1<<21)
#define	U_MODEL4			(1<<22)
#define	U_MOREBITS3			(1<<23)		// read one additional byte

// fourth byte
#define	U_OLDORIGIN			(1<<24)		// FIXME: get rid of this ??
#define	U_SKIN16			(1<<25)
#define	U_SOUND				(1<<26)
#define	U_SOLID				(1<<27)


/*
==============================================================

CMD

Command text buffering and command execution

==============================================================
*/

/*

Any number of commands can be added in a frame, from several different sources.
Most commands come from either keybindings or console line input, but remote
servers can also send across commands and entire text files can be execed.

The + command line options are also added to the command buffer.

*/

void	Cbuf_Init (void);
// allocates an initial text buffer that will grow as needed

void	Cbuf_AddText (char *text);
// as new commands are generated from the console or keybindings,
// the text is added to the end of the command buffer.

void	Cbuf_InsertText (char *text);
// when a command wants to issue other commands immediately, the text is
// inserted at the beginning of the buffer, before any remaining unexecuted
// commands.

void	Cbuf_Execute (void);
// Pulls off \n terminated lines of text from the command buffer and sends
// them through Cmd_ExecuteString.  Stops when the buffer is empty.
// Normally called once per frame, but may be explicitly invoked.
// Do not call inside a command function!

void	Cbuf_CopyToDefer (void);
void	Cbuf_InsertFromDefer (void);
// These two functions are used to defer any pending commands while a map
// is being loaded (NOTE: cannot simply disable Cbuf_Execute(): while map is loading,
// there can be executed different commands)

//===========================================================================

//!! here for command completion (current version) only
class CAlias : public CStringItem
{
public:
	char	*value;
};
extern TList<CAlias> AliasList;

class CCommand : public CStringItem
{
public:
	int		flags;
	void (*func) ();
};
extern TList<CCommand> CmdList;


void	Cmd_Init (void);

//--bool	RegisterCommand (char *cmd_name, void (*func)(void), int flags);
// called by the init functions of other parts of the program to
// register commands and functions to call for them.
// The cmd_name is referenced later, so it should not be in temp memory
// if function is NULL, the command will be forwarded to the server
// as a clc_stringcmd instead of executed locally
//--void	UnregisterCommand (const char *cmd_name);

// The functions that execute commands get their parameters with these
// functions. Cmd_Argv () will return an empty string, not a NULL
// if arg > argc, so string operations are always safe.

bool	Cmd_ExecuteString (const char *text);
// Parses a single line of text into arguments and tries to execute it
// as if it was typed at the console; if command is unknown, will return false

void	Cmd_ForwardToServer (int argc, char **argv);
// adds the current command line as a clc_stringcmd to the client message.
// things like godmode, noclip, etc, are commands directed to the server,
// so when they are typed in at the console, they will need to be forwarded.
// NOTE: implemented in client

void	Cmd_WriteAliases (FILE *f);

/*
==============================================================

CVAR

==============================================================
*/

typedef struct
{
	cvar_t	**var;				// destination, may be NULL
	const char *string;			// name[/value]
	int		flags;
} cvarInfo_t;

// NOTES:
//	CVAR_FULL() useful for any case (especially for vars with value == "")
//	CVAR_VAR() useful for vars, which C name is the same as cvar.name
//	CVAR_VAR2() useful for vars, which C name differs from cvar.name
//	CVAR_GET() useful for retreiving var pointer without setting var parameters
//	CVAR_NULL() useful for setting var parameters without retreiving its pointer
#define CVAR_BEGIN(name)				static const cvarInfo_t name[] = {
#define CVAR_FULL(pvar,name,value,flags)	{pvar, name"="value, flags}	// any parameters, value not stringified
#define CVAR_VAR(name,value,flags)			CVAR_FULL(&name, #name, #value, flags)	// var == name, non-empty value, flags
#define CVAR_VAR2(var,name,value,flags)		CVAR_FULL(&var,  #name, #value, flags)	// var != name, non-empty value, flags
#define CVAR_GET(name)						CVAR_FULL(&name, #name, "",		CVAR_NODEFAULT)	// just set var
#define CVAR_NULL(name,value,flags)			CVAR_FULL(NULL,  #name, #value, flags)	// register cvar with
#define CVAR_END						};


extern cvar_t *cvar_vars;
extern int	cvar_initialized;

//--cvar_t *Cvar_Get (char *var_name, char *value, int flags);
// creates the variable if it doesn't exist, or returns the existing one
// if it exists, the value will not be changed, but flags will be ORed in
// that allows variables to be unarchived without needing bitflags

cvar_t 	*Cvar_FullSet (const char *var_name, const char *value, int flags);

//--cvar_t	*Cvar_SetValue (char *var_name, float value);
//--cvar_t	*Cvar_SetInteger (char *var_name, int value);
//--cvar_t 	*Cvar_Set (char *var_name, char *value);				// will create the variable if it doesn't exist
cvar_t	*Cvar_ForceSet (const char *var_name, const char *value);	// will set the variable even if NOSET or LATCH

// expands value to a string and calls Cvar_Set
//--float	Cvar_Clamp (cvar_t *cvar, float low, float high);
//--float	Cvar_ClampName (char *name, float low, float high);

//--float	Cvar_VariableValue (char *var_name);
//--int		Cvar_VariableInt (char *var_name);
// returns 0 if not defined or non numeric

//--char	*Cvar_VariableString (char *var_name);
// returns an empty string if not defined

void	Cvar_GetLatchedVars (void);
// any CVAR_LATCHED variables that have been set will now take effect

bool	Cvar_Command (int argc, char **argv);
// called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
// command.  Returns true if the command was a variable reference that
// was handled. (print or change)

void	Cvar_WriteVariables (FILE *f, int includeMask, int excludeMask, const char *header);
// appends lines containing "set variable value" for all variables with the archive flag set.

void	Cvar_Cheats (bool enable);
void	Cvar_Init (void);

char	*Cvar_Userinfo (void);
// returns an info string containing all the CVAR_USERINFO cvars

char	*Cvar_Serverinfo (void);
// returns an info string containing all the CVAR_SERVERINFO cvars

extern bool userinfo_modified;
// this is set each time a CVAR_USERINFO variable is changed
// so that the client knows to send it to the server

/*
==============================================================

NET

==============================================================
*/

// net.h -- quake's interface to the networking layer

#define	PORT_ANY		-1

#define	MAX_MSGLEN		16384		// max length of a message
#define MAX_MSGLEN_OLD	1400		// MAX_MSGLEN for old clients and for IPX protocol

typedef enum {NA_LOOPBACK, NA_BROADCAST, NA_IP, NA_IPX, NA_BROADCAST_IPX} netadrtype_t;

typedef enum {NS_CLIENT, NS_SERVER} netsrc_t;

typedef struct
{
	netadrtype_t type;
	byte	ip[4];
	byte	ipx[10];
	unsigned short port;
} netadr_t;

bool	IPWildcard (netadr_t *a, char *mask);

void	NET_Init (void);
void	NET_Shutdown (void);

void	NET_Config (bool multiplayer);

bool	NET_GetPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message);
void	NET_SendPacket (netsrc_t sock, int length, void *data, netadr_t to);

bool	NET_CompareAdr (netadr_t *a, netadr_t *b);
bool	NET_CompareBaseAdr (netadr_t *a, netadr_t *b);
bool	NET_IsLocalAddress (netadr_t *adr);
char	*NET_AdrToString (netadr_t *a);
bool	NET_StringToAdr (const char *s, netadr_t *a);

//============================================================================

typedef struct
{
	bool	fatal_error;

	netsrc_t sock;

	int		dropped;			// between last packet and previous

	int		last_received;		// for timeouts
	int		last_sent;			// for retransmits

	netadr_t remote_address;
	int		qport;				// qport value to write when transmitting

	// sequencing variables
	int		incoming_sequence;
	int		incoming_acknowledged;
	int		incoming_reliable_acknowledged;	// single bit

	int		incoming_reliable_sequence;		// single bit, maintained local

	int		outgoing_sequence;
	int		reliable_sequence;				// single bit
	int		last_reliable_sequence;			// sequence number of last send

	// reliable staging and holding areas
	sizebuf_t message;		// writing buffer to send to server
	byte	message_buf[MAX_MSGLEN-16];		// leave space for header

	// message is copied to this buffer when it is first transfered
	int		reliable_length;
	byte	reliable_buf[MAX_MSGLEN-16];	// unacked reliable message
} netchan_t;

extern netadr_t	net_from;
extern sizebuf_t net_message;
extern byte		net_message_buffer[MAX_MSGLEN];


void	Netchan_Init (void);
void	Netchan_Setup (netsrc_t sock, netchan_t *chan, netadr_t adr, int qport);

bool	Netchan_NeedReliable (netchan_t *chan);
void	Netchan_Transmit (netchan_t *chan, int length, void *data);
void	Netchan_OutOfBand (netsrc_t net_socket, netadr_t adr, int length, void *data);
void	Netchan_OutOfBandPrint (netsrc_t net_socket, netadr_t adr, const char *format, ...);
bool	Netchan_Process (netchan_t *chan, sizebuf_t *msg);

bool	Netchan_CanReliable (netchan_t *chan);


/*
==============================================================

CMODEL

==============================================================
*/


// cmodel_t.flags
#define CMODEL_ALPHA	1
#define CMODEL_MOVABLE	2

typedef struct
{
	vec3_t	mins, maxs;
	float	radius;
	int		headnode;
	int		firstface, numfaces;
	int		flags;
} cmodel_t;

extern char map_name[];
extern bool map_clientLoaded;

cmodel_t *CM_LoadMap (char *name, bool clientload, unsigned *checksum);
cmodel_t *CM_InlineModel (char *name);	// *1, *2, etc

int		CM_NumClusters (void);
int		CM_NumInlineModels (void);
char	*CM_EntityString (void);

// creates a clipping hull for an arbitrary box
int		CM_HeadnodeForBox (vec3_t mins, vec3_t maxs);


// returns an ORed contents mask
int		CM_PointContents (vec3_t p, int headnode);
int		CM_TransformedPointContents (vec3_t p, int headnode, vec3_t origin, vec3_t angles);
int		CM_TransformedPointContents2 (vec3_t p, int headnode, vec3_t origin, vec3_t *axis);

//--void	CM_BoxTrace (trace_t *tr, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int headnode, int brushmask);
//--void	CM_TransformedBoxTrace (trace_t *tr, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs,	int headnode, int brushmask, vec3_t origin, vec3_t angles);

byte	*CM_ClusterPVS (int cluster);
byte	*CM_ClusterPHS (int cluster);

int		CM_PointLeafnum (vec3_t p, int num = 0);

// call with topnode set to the headnode, returns with topnode
// set to the first node that splits the box
int		CM_BoxLeafnums (vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode = NULL, int headnode = 0);

int		CM_LeafContents (int leafnum);
int		CM_LeafCluster (int leafnum);
int		CM_LeafArea (int leafnum);

void	CM_SetAreaPortalState (int portalnum, bool open);
bool	CM_AreasConnected (int area1, int area2);

int		CM_WriteAreaBits (byte *buffer, int area);
bool	CM_HeadnodeVisible (int headnode, byte *visbits);

void	CM_WritePortalState (FILE *f);
void	CM_ReadPortalState (FILE *f);

/*
==============================================================

PLAYER MOVEMENT CODE

Common between server and client so prediction matches

==============================================================
*/

extern float pm_airaccelerate;

void	Pmove (pmove_t *pmove);


/*----------------- Debugging ------------------*/


void	DebugPrintf (const char *fmt, ...);


/*-------------- Memory management -------------*/

/* Layout of fields:
	1. char *name
	2. self_t *next
	3. all othen fields
	4. char name[] (pointed by field #1)
*/
typedef struct basenamed_s
{
	char *name;
	struct basenamed_s *next;
	// place for other fields
} basenamed_t;


// Strings
char	*ChainCopyString (const char *in, void *chain);

// Named structure lists
basenamed_t *AllocNamedStruc (int size, char *name);
#define FreeNamedStruc(s)   appFree(s)
basenamed_t *ChainAllocNamedStruc (int size, char *name, void *chain);

basenamed_t *FindNamedStruc (char *name, basenamed_t *first, basenamed_t **where);
basenamed_t *FindNamedStrucByIndex (basenamed_t *first, int index);
int IndexOfNamedStruc (char *name, basenamed_t *first);

basenamed_t *AddToNamedList (char *name, basenamed_t *list);
basenamed_t *ChainAddToNamedList (char *name, basenamed_t *list, void *chain);
void	FreeNamedList (basenamed_t *list);


/*----------------- File system ----------------*/

extern cvar_t	*fs_gamedirvar;

#define LIST_FILES	1
#define LIST_DIRS	2

void	FS_InitFilesystem (void);
bool	FS_SetGamedir (const char *dir);
//--char	*FS_Gamedir (void);
char	*FS_NextPath (const char *prevpath);
void	FS_LoadGameConfig (void);

int		FS_FOpenFile (const char *filename, FILE **file);
//--bool FS_FileExists (char *filename);
void	FS_FCloseFile (FILE *f);
void	FS_Read (void *buffer, int len, FILE *f);
// properly handles partial reads

//--void*	FS_LoadFile (const char *path, unsigned *len = NULL);
// a null buffer will just return the file length without loading
// a -1 length is not present
//--void	FS_FreeFile (void *buffer);

//--void	FS_CreatePath (char *path);

void	Sys_Mkdir (char *path);

char	*Sys_FindFirst (char *path, int flags);
char	*Sys_FindNext (void);
void	Sys_FindClose (void);


/*------------- Miscellaneous -----------------*/

extern	int	curtime;		// time returned by last Sys_Milliseconds; remove ??
extern	int linewidth;		// for functions, which wants to perform advanced output formatting

void	Mem_Init (void);

void	Com_BeginRedirect (char *buffer, int buffersize, void (*flush)(char*));
void	Com_EndRedirect (void);

void	NORETURN Com_Quit (void);

server_state_t Com_ServerState (void);		// this should have just been a cvar...
void	Com_SetServerState (server_state_t state);

unsigned Com_BlockChecksum (void *buffer, int length);
byte	COM_BlockSequenceCRCByte (byte *base, int length, int sequence);

#ifdef DYNAMIC_REF
// Using inline version will grow executable with ~2Kb (cl_fx.cpp uses a lots of [c|f]rand() calls)
// BUT: require this for dynamic renderer (or move to ref/ref.in)
// 0 to 1
inline float frand (void)
{
	return rand() * (1.0f/RAND_MAX);
}

// -1 to 1
inline float crand (void)
{
	return rand() * (2.0f/RAND_MAX) - 1;
}
#else
float frand (void);
float crand (void);
#endif


extern cvar_t	*developer;
extern cvar_t	*dedicated;
extern cvar_t	*com_speeds;
extern cvar_t	*timedemo;
extern cvar_t	*timescale;
extern cvar_t	*sv_cheats;

// com_speeds times
extern int	time_before_game;
extern int	time_after_game;
extern int	time_before_ref;
extern int	time_after_ref;

bool	COM_CheckCmdlineVar (const char *name);

void	QCommon_Init (char *cmdline);
void	QCommon_Frame (int msec);
void	QCommon_Shutdown (void);

#define NUMVERTEXNORMALS	162
extern	vec3_t	bytedirs[NUMVERTEXNORMALS];

// this is in the client code, but can be used for debugging from server
void	SCR_DebugGraph (float value, int color);


/*-------- Non-portable system services --------*/


void	Sys_Init (void);

void	Sys_UnloadGame (void);
void	*Sys_GetGameAPI (void *parms);
// loads the game dll and calls the api init function

char	*Sys_ConsoleInput (void);
void	Sys_ConsoleOutput (char *string);
void	Sys_SendKeyEvents (void);
void	NORETURN Sys_Error (const char *error, ...);
void	NORETURN Sys_Quit (void);
void	Sys_CopyProtect (void);


/*------- Client / server systems ---------*/


void	CL_Init (void);
void	CL_Drop (bool fromError = false);
void	CL_Shutdown (bool error);
void	CL_Frame (float msec, int realMsec);
void	Con_Print (const char *text);
void	SCR_BeginLoadingPlaque (void);

void	SV_Init (void);
void	SV_Shutdown (char *finalmsg, bool reconnect);
void	SV_Frame (float msec);


/*------------- Image loading -------------*/


//--void LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height);
//--void LoadTGA (char *name, byte **pic, int *width, int *height);
//--void LoadJPG (char *name, byte **pic, int *width, int *height);

#define IMAGE_PCX	1
#define IMAGE_WAL	2
#define IMAGE_TGA	4
#define IMAGE_JPG	8

#define IMAGE_8BIT	(IMAGE_PCX|IMAGE_WAL)
#define IMAGE_32BIT	(IMAGE_TGA|IMAGE_JPG)
#define IMAGE_ANY	(IMAGE_8BIT|IMAGE_32BIT)

//--int ImageExists (char *name, int stop_mask);


/*---------- Map and model stuff ----------*/


typedef struct lightFlare_s
{
	vec3_t	origin;
	float	size;
	float	radius;					// -1 for sunflare
	byte	color[4];
	byte	style;
	int		model;
	struct lightFlare_s *next;
} lightFlare_t;


// static map light

typedef enum {sl_linear, sl_inverse, sl_inverse2} slightType_t;

typedef struct slight_s
{
	slightType_t type;
	byte	spot;					// bool
	vec3_t	origin;
	vec3_t	color;
	byte	style;
	float	intens;
	// arghrad fields
	float	focus;
	float	fade;					// for linear lights only: scale the distance
	// for spotlights
	float	spotDot;
	vec3_t	spotDir;
	struct slight_s *next;
} slight_t;

// static map effects

typedef struct splash_s
{
	vec3_t	origin;
	struct splash_s *next;
} splash_t;


typedef enum {map_q2, map_kp, map_hl} mapType_t;
typedef enum {fog_no, fog_linear, fog_exp, fog_exp2} fogMode_t;


typedef struct
{
	char		name[MAX_QPATH];	// mapname
	void		*file;				// buffer, returned by FS_LoadFile()
	mapType_t	type;
	unsigned	checksum;
	unsigned	length;
	CMemoryChain *extraChain;

	// entstring
	int			entDataSize;
	char		*entities;

	// geometry
	int			numPlanes;
	dplane_t	*planes;

	int			numVertexes;
	dvertex_t	*vertexes;

	// BSP
	int			visDataSize;
	dvis_t		*visibility;

	int			numNodes;
	dnode_t		*nodes;

	int			numLeafs;
	dleaf_t		*leafs;

	int			numLeaffaces;
	unsigned short *leaffaces;

	int			numLeafbrushes;
	unsigned short *leafbrushes;

	int			numEdges;
	dedge_t		*edges;

	int			numSurfedges;
	int			*surfedges;

	int			numBrushes;
	dbrush_t	*brushes;

	int			numBrushsides;
	dbrushside_t *brushsides;

	int			numAreas;
	darea_t		*areas;

	int			numAreaportals;
	dareaportal_t *areaportals;

	int			numModels;
	cmodel_t	*models;

	// faces
	int			numTexinfo;
	texinfo_t	*texinfo;

	int			numFaces;
	dface_t		*faces;

	// lighting
	int			lightDataSize;
	byte		*lighting;

	int			numFlares;
	lightFlare_t *flares;

	int			numSlights;
	slight_t	*slights;

	// static effects
	int			numSplashes;			// target_splash entities
	splash_t	*splashes;

	// fog
	//?? remove
	fogMode_t	fogMode;
	float		fogColor[3];
	union {
		struct {	// exp/exp2
			float	fogDens;
		};
		struct {	// linear
			float	fogStart;
			float	fogEnd;
		};
	};

	float	sunLight;		// 0 if no sun
	vec3_t	sunColor;
	vec3_t	sunVec;
	vec3_t	sunAmbient;		// ambient light from sky surfaces
	vec3_t	sunSurface;
	vec3_t	ambientLight;	// global ambient light
} bspfile_t;

extern bspfile_t *map_bspfile;


//--bspfile_t *LoadBspFile (char *filename, bool clientload, unsigned *checksum);
char *ProcessEntstring (char *entString);


/*-----------------------------------------------------------------------------
	Staff from new engine
-----------------------------------------------------------------------------*/

#include "../client/ref_decl.h"
#include "../client/ref_defs.h"

#endif // QCOMMON_H
