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

#ifndef QCOMMON_H
#define QCOMMON_H

#include "Core.h"

//!!!! temp:
#define Com_DropError	appNonFatalError

//#define TRACE_DEBUG		1


//-------- declarations, required for ref_xxxx.h --------------

// forward declarations; should remove!!
struct cvar_t;
struct cvarInfo_t;
struct trace_t;
struct bspfile_t;

// cmodel interface
struct CBspNode;
struct CBspLeaf;

// game interface/communication strucs
struct entity_state_t;
struct entityStateEx_t;
struct player_state_t;
struct usercmd_t;
struct pmove_t;

// empty declarations
struct CBrush;


// image flags
#define IMAGE_PCX	1
#define IMAGE_WAL	2
#define IMAGE_TGA	4
#define IMAGE_JPG	8

#define IMAGE_8BIT	(IMAGE_PCX|IMAGE_WAL)
#define IMAGE_32BIT	(IMAGE_TGA|IMAGE_JPG)
#define IMAGE_ANY	(IMAGE_8BIT|IMAGE_32BIT)

// exports for renderer
#include "../client/engine_intf.h"


//--------------- some constants ------------------------------

// version number
#define	VERSION			4.16

#if _WIN32

#define BUILDSTRING		"Win32"
#if _M_IX86				// VisualC std define for x86
#	define CPUSTRING	"x86"
#elif _M_ALPHA
#	define CPUSTRING	"AXP"
#elif __MINGW32__
#	define CPUSTRING	"x86"
#endif

#elif __linux__

#define BUILDSTRING		"Linux"
#if __i386__
#	define CPUSTRING	"i386"
#elif __alpha__
#	define CPUSTRING	"axp"
#endif

#elif __CYGWIN__

#define BUILDSTRING		"Cygwin"
#define CPUSTRING		"x86"

#elif __UNIX__

#define BUILDSTRING		"*nix"

#else	// unknown platform

#define BUILDSTRING		"Unknown"

#endif	// platform

// default CPUSTRING
#ifndef CPUSTRING
#define	CPUSTRING		"Unknown"
#endif


#define VERSION_STR		STR(VERSION) " " CPUSTRING " " __DATE__ " " BUILDSTRING

#if !DEDICATED_ONLY
#define DEDICATED		dedicated->integer
#else
#define DEDICATED		1
#endif


#if NO_DEBUG && !defined(NO_DEVELOPER)
#define NO_DEVELOPER	1
#endif

#if !NO_DEVELOPER
#define DEVELOPER		developer->integer
#else
#define DEVELOPER		0
inline void Com_DPrintf (const char*, ...) {}
#endif


// some configuration

#define APPNAME			"Quake2"
#define	BASEDIRNAME		"baseq2"
#define CONFIGNAME		"config.cfg"
// if CD_PATH is not defined, CD checking and adding its paths will not be performed
#define CD_PATH			"install/data"
#define CD_CHECK		"install/data/quake2.exe"	// file used for CD validation

#define SAVEGAME_DIRECTORY			"save"
#define SAVEGAME_SERVER_EXTENSION	"sv2"
#define SAVEGAME_GAME_EXTENSION		"sav"
#define SAVEGAME_VARS_EXTENSION		"ssv"

#define	MAX_STRING_CHARS	1024			// max length of a string passed to TokenizeString()
#define	MAX_STRING_TOKENS	80				// max tokens resulting from TokenizeString()

#define	MAX_QPATH			64				// max length of a quake game pathname
#define	MAX_OSPATH			128				// max length of a filesystem pathname



/*-----------------------------------------------------------------------------
	msg.cpp
-----------------------------------------------------------------------------*/

class sizebuf_t
{
public:
	//!! should be private:
	byte	*data;
	int		maxsize;						// size of buffer
	bool	overflowed;						// set to true if the buffer size failed
	//!! should be public:
	bool	allowoverflow;					// if false, do a error when overflowed
	int		cursize;						// writting cursor
	int		readcount;						// reading cursor

	// Functions
	void Init (void *data, int length);		// setup buffer
	inline void Clear ()					// clear writting
	{
		cursize = 0;
		overflowed = false;
	}
	void Write (const void *data, int length);
	inline void Write (sizebuf_t &from)
	{
		Write (from.data, from.cursize);
	}
	inline void BeginReading ()
	{
		readcount = 0;
	}
};

void	MSG_WriteChar (sizebuf_t *sb, int c);
void	MSG_WriteByte (sizebuf_t *sb, int c);
void	MSG_WriteShort (sizebuf_t *sb, int c);
void	MSG_WriteLong (sizebuf_t *sb, int c);
void	MSG_WriteFloat (sizebuf_t *sb, float f);
void	MSG_WriteString (sizebuf_t *sb, const char *s);
void	MSG_WritePos (sizebuf_t *sb, const CVec3 &pos);
void	MSG_WriteAngle (sizebuf_t *sb, float f);
void	MSG_WriteAngle16 (sizebuf_t *sb, float f);
void	MSG_WriteDir (sizebuf_t *sb, const CVec3 &vector);


int		MSG_ReadChar (sizebuf_t *sb);
int		MSG_ReadByte (sizebuf_t *sb);
int		MSG_ReadShort (sizebuf_t *sb);
int		MSG_ReadLong (sizebuf_t *sb);
float	MSG_ReadFloat (sizebuf_t *sb);
char	*MSG_ReadString (sizebuf_t *sb);
void	MSG_ReadPos (sizebuf_t *sb, CVec3 &pos);
float	MSG_ReadAngle (sizebuf_t *sb);
float	MSG_ReadAngle16 (sizebuf_t *sb);
void	MSG_ReadDir (sizebuf_t *sb, CVec3 &vector);

void	MSG_ReadData (sizebuf_t *sb, void *buffer, int size);

/*-----------------------------------------------------------------------------
	Delta compression for quake2 protocol (entdelta.cpp)
-----------------------------------------------------------------------------*/

// entity_state_t communication

void	MSG_WriteDeltaEntity (sizebuf_t *msg, const entityStateEx_t *from, entityStateEx_t *to, bool force, bool newentity, bool isExt);
void	MSG_WriteRemoveEntity (sizebuf_t *msg, int num);

int		MSG_ReadEntityBits (sizebuf_t *msg, unsigned *bits, bool *remove);
void	MSG_ReadDeltaEntity (sizebuf_t *msg, const entityStateEx_t *from, entityStateEx_t *to, unsigned bits);

void	MSG_ReadDeltaUsercmd (sizebuf_t *smg, const usercmd_t *from, usercmd_t *to);
void	MSG_WriteDeltaUsercmd (sizebuf_t *msg, const usercmd_t *from, usercmd_t *to);

void	MSG_ReadDeltaPlayerstate (sizebuf_t *msg, const player_state_t *oldState, player_state_t *newState);
void	MSG_WriteDeltaPlayerstate (sizebuf_t *msg, const player_state_t *oldState, player_state_t *newState);


/*-----------------------------------------------------------------------------
	anorms.cpp
-----------------------------------------------------------------------------*/

#define NUMVERTEXNORMALS	162
// used by:
// 1. common.cpp: MSG_WriteDir(), MSG_ReadDir()
// 2. cl_fx.cpp:  CL_FlyParticles(), CL_BfgParticles()
extern	/*const*/ CVec3 bytedirs[NUMVERTEXNORMALS];

void	InitByteDirs ();


/*-----------------------------------------------------------------------------
	key/value info strings
-----------------------------------------------------------------------------*/

#define	MAX_INFO_KEY		64
#define	MAX_INFO_VALUE		64
#define	MAX_INFO_STRING		512

const char * Info_ValueForKey (const char *s, const char *key);
//?? Info_SetValueForKey: used for Cvar_BitInfo() and 1 place for value("ip")=ip only!
void	Info_SetValueForKey (char *s, const char *key, const char *value);
void	Info_Print (const char *s);


/*-----------------------------------------------------------------------------
	Text parser
-----------------------------------------------------------------------------*/

// data is an in/out parm, returns a parsed out token
char *COM_Parse (const char * &data_p, bool allowLineBreaks = true);
const char *COM_QuoteString (const char *str, bool alwaysQuote);


/*-----------------------------------------------------------------------------
	misc math
-----------------------------------------------------------------------------*/

// macro version for any types (used 2 times)
#define VectorSubtract2(a,b,c)	(c[0]=a[0]-b[0],c[1]=a[1]-b[1],c[2]=a[2]-b[2])

float ReduceAngle (float a);
float LerpAngle (float a1, float a2, float frac);
float AngleSubtract (float a1, float a2);
void AnglesSubtract (const CVec3 &v1, const CVec3 &v2, CVec3 &v3);


/*-----------------------------------------------------------------------------
	crc.cpp
-----------------------------------------------------------------------------*/


byte	Com_BlockSequenceCRCByte (byte *base, int length, int sequence);


/*-----------------------------------------------------------------------------
	Collision detection (??)
-----------------------------------------------------------------------------*/

struct csurface_t
{
	// standard csurface_t fields (do not change this - used in trace
	// functions, which are exported to game !)
	char		shortName[16];
	unsigned	flags;
	int			value;
	// field from mapsurface_t (so, csurface_t now contains old
	//  csurface_t and mapsurface_t)
	char		fullName[32];		// shortName[] is too short ...
	// fields added since 4.00
	int			material;
	color_t		color;				// color of texture (HL lighting surface, SURF_LIGHT)
};

// a trace is returned when a box is swept through the world
// 'trace0_t' used for compatibility with original Q2 mods; engine uses
// extended structure 'trace_t' (additional fields will be stripped, when
// passing this structure to game)
struct trace0_t
{
	bool		allsolid;			// if true, plane is not valid; if 'true', startsolid is 'true' too
	byte		pad1[3];			// qboolean pad
	bool		startsolid;			// if true, the initial point was in a solid area
	byte		pad2[3];			// qboolean pad
	float		fraction;			// time completed, 1.0 = didn't hit anything
	CVec3		endpos;				// final position
	CPlane		plane;				// surface normal at impact
	csurface_t	*surface;			// surface hit
	int			contents;			// contents on other side of surface hit
	struct edict_s *ent;			// not set by CM_*() functions; unknown type at this point !
};

// extended trace info
struct trace_t : trace0_t
{
	int			brushNum;			// number of brush, which caused collision (uninitialized == -1)
};


/*
==============================================================

PROTOCOL

==============================================================
*/

// declaration from server.h
enum server_state_t
{
	ss_dead,
	ss_loading,
	ss_game,
	ss_cinematic,
	ss_demo,
	ss_pic
};


// protocol.h -- communications protocols

#define	PROTOCOL_VERSION	34

//=========================================

#define	PORT_MASTER			27900
#define	PORT_CLIENT			27901		//?? unused
#define	PORT_SERVER			27910

//=========================================

#define	UPDATE_BACKUP		16			// copies of entity_state_t to keep buffered; must be power of two
#define	UPDATE_MASK			(UPDATE_BACKUP-1)

// protocol extensions
#define NEW_PROTOCOL_ID 		"gildor"
#define NEW_PROTOCOL_VERSION	1

/* Protocol extensions history:
 * ----------------------------
 *	0 (no version)
 *		- surface-dependent footsteps (EV_FOOTSTEPn, EV_FALLSHORTn)
 *		- camper sounds (EV_CAMPERn)
 *		- increased max network packet size
 *		- increased download packet size (faster downloading)
 *	1 compatible with v0 (can read demos, recorded with v0 extensions)
 *		-
 */


// player_state_t->rdflags (refdef flags)
#define	RDF_UNDERWATER		1		// warp the screen as apropriate
#define RDF_NOWORLDMODEL	2		// used for player configuration screen
//ROGUE
#define	RDF_IRGOGGLES		4
#define RDF_UVGOGGLES		8		//?? unused
// new since 4.00
#define RDF_THIRD_PERSON	0x10


#define	ANGLE2SHORT(x)	(appRound ((x)*65535.0f/360) & 65535)
#define	SHORT2ANGLE(x)	((x)*(360.0f/65536))



// the svc_strings[] array in cl_parse.cpp should mirror this
// server to client
enum
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

// client to server
enum
{
	clc_bad,
	clc_nop,
	clc_move,					// [usercmd_t]
	clc_userinfo,				// [userinfo string]
	clc_stringcmd				// [string message]
};


// a sound without an ent or pos will be a local only sound
#define	SND_VOLUME			(1<<0)		// a byte
#define	SND_ATTENUATION		(1<<1)		// a byte
#define	SND_POS				(1<<2)		// three coordinates
#define	SND_ENT				(1<<3)		// a short 0-2: channel, 3-12: entity
#define	SND_OFFSET			(1<<4)		// a byte, msec offset from frame start

#define DEFAULT_SOUND_PACKET_VOLUME			1.0
#define DEFAULT_SOUND_PACKET_ATTENUATION	1.0


/*-----------------------------------------------------------------------------
	Command text buffering and command execution (cmd.cpp)
-----------------------------------------------------------------------------*/

/*

Any number of commands can be added in a frame, from several different sources.
Most commands come from either keybindings or console line input, but remote
servers can also send across commands and entire text files can be execed.

The + command line options are also added to the command buffer.

*/


void	Cbuf_AddText (const char *text);
// as new commands are generated from the console or keybindings,
// the text is added to the end of the command buffer.

void	Cbuf_Execute ();
// Pulls off \n terminated lines of text from the command buffer and sends
// them through Cmd_ExecuteString.  Stops when the buffer is empty.
// Normally called once per frame, but may be explicitly invoked.
// Do not call inside a command function!

void	Cbuf_CopyToDefer ();
void	Cbuf_InsertFromDefer ();
// These two functions are used to defer any pending commands while a map
// is being loaded (NOTE: cannot simply disable Cbuf_Execute(): while map is loading,
// there can be executed different commands)


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
	unsigned flags;
	void (*func) ();
};
extern TList<CCommand> CmdList;


// adds the current command line as a clc_stringcmd to the client message.
// things like godmode, noclip, etc, are commands directed to the server,
// so when they are typed in at the console, they will need to be forwarded.
// NOTE: implemented in client
void	Cmd_ForwardToServer (int argc, char **argv);

void	Cmd_WriteAliases (COutputDevice *Out);


/*-----------------------------------------------------------------------------
	cvar.cpp
-----------------------------------------------------------------------------*/

#define	CVAR_ARCHIVE		0x00001	// set to cause it to be saved to config file (config.cfg)
#define	CVAR_USERINFO		0x00002	// added to userinfo when changed, then sent to senver
#define	CVAR_SERVERINFO		0x00004	// added to serverinfo when changed; mostly used with NOSET or LATCH flag
#define	CVAR_NOSET			0x00008	// don't allow change from console at all, but can be set from the command line
#define	CVAR_LATCH			0x00010	// save changes until server restart
// added since 4.00
#define	CVAR_USER_CREATED	0x00020	// created by a "set" command -- this cvar have no pointers (cvar_t*) from any code
#define CVAR_GAME_CREATED	0x00040	// created from game library
#define CVAR_CHEAT			0x00080	// will be reset to its default value when cheat protection active
#define CVAR_CMDLINE		0x00100	// variable was set from command line

#define CVAR_FLAG_MASK		0x0FFFF	// mask of stored cvar flags

// not stored flags:
#define CVAR_NODEFAULT		0x10000	// do not store "default" value from this Cvar_Get() call
#define CVAR_UPDATE			0x20000	// set "modified" field after Cvar_Get() call
#define CVAR_NOUPDATE		0x40000	// reset "modified" field ...

#define	CVAR_BUFFER_SIZE	16		// size of buffer for var->string inside cvar_t

struct cvar_t
{
	const char *name;
	char	*string;
	char	*latchedString;			// for CVAR_LATCH vars
	unsigned flags;
	bool	modified;				// set each time the cvar is changed; originally - qboolean
	byte	pad[3];					// qboolean pad (4 bytes)
	float	value;					// atof(string)
	cvar_t	*next;
	// added since 4.00
	char	*resetString;			// default cvar value (unset for user-created vars)
	int		integer;				// atoi(string)
	int		stringLength;			// size of buffer, allocated for holding var->string (or 0 if var->buf used)
	char	buf[CVAR_BUFFER_SIZE];
	cvar_t	*hashNext;
	// functions
	inline bool IsDefault ()
	{
		return resetString && strcmp (string, resetString) == 0;
	}
	inline void Reset ()
	{
		//!! not optimal - should use cvar_t struc instead of "name"
		Cvar_Set (name, resetString);
	}
	inline float Clamp (float low, float high)
	{
		return Cvar_Clamp (this, low, high);
	}
//	inline void FullSet (const char *value, unsigned flags)
//	{
//		Cvar_FullSet (this, value, flags);
//	}
	// statics
	static cvar_t	*vars;
	static int		initialized;
	// this is set each time a cvar variable is changed (or'ed with cvar flags)
	static unsigned modifiedFlags;
};


struct cvarInfo_t
{
	cvar_t	**var;				// destination, may be NULL
	const char *string;			// name[/value]
	unsigned flags;
};

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
#define CVAR_NULL(name,value,flags)			CVAR_FULL(NULL,  #name, #value, flags)	// register cvar without saving pointer
#define CVAR_END						};


//--cvar_t *Cvar_Get (char *var_name, char *value, int flags);
// creates the variable if it doesn't exist, or returns the existing one
// if it exists, the value will not be changed, but flags will be ORed in
// that allows variables to be unarchived without needing bitflags

// used for CVAR_SERVERINFO|(CVAR_LATCH/CVAR_NOSET)
// and inside cvar.cpp for "set[s|u|a]" commands
cvar_t	*Cvar_FullSet (const char *var_name, const char *value, unsigned flags);

//--cvar_t	*Cvar_SetValue (char *var_name, float value);
//--cvar_t	*Cvar_SetInteger (char *var_name, int value);
//--cvar_t 	*Cvar_Set (char *var_name, char *value);				// will create the variable if it doesn't exist
cvar_t	*Cvar_ForceSet (const char *var_name, const char *value);	// will set the variable even if NOSET or LATCH

// expands value to a string and calls Cvar_Set
//--float	Cvar_Clamp (cvar_t *cvar, float low, float high);

//--float	Cvar_VariableValue (char *var_name);
//--int		Cvar_VariableInt (char *var_name);
// returns 0 if not defined or non numeric

//--const char	*Cvar_VariableString (char *var_name);
// returns an empty string if not defined

bool	Cvar_Command (int argc, char **argv);
// called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
// command.  Returns true if the command was a variable reference that
// was handled. (print or change)

void	Cvar_WriteVariables (COutputDevice *Out, int includeMask, int excludeMask, const char *header);
// appends lines containing "set variable value" for all variables with the archive flag set.

void	Cvar_Cheats (bool enable);

const char *Cvar_BitInfo (unsigned bit);


/*-----------------------------------------------------------------------------
	net_chan.cpp, net_{platform}.cpp
-----------------------------------------------------------------------------*/

#define	MAX_MSGLEN		16384		// max length of a message
#define MAX_MSGLEN_OLD	1400		// MAX_MSGLEN for old clients (and for IPX protocol - gone)

enum netadrtype_t
{
	NA_LOOPBACK, NA_BROADCAST, NA_IP
};
	// NOTE: NA_BROADCAST used for serrching game servers only
enum netsrc_t
{
	NS_CLIENT, NS_SERVER
};

struct netadr_t
{
	netadrtype_t type;
	unsigned short port;	// big-endian value
	union {
		byte	ip[4];
		unsigned ip4;
	};
};

bool	IPWildcard (netadr_t *a, const char *mask);

void	NET_Init ();
void	NET_Shutdown ();

void	NET_Config (bool multiplayer);

bool	NET_GetPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_msg);
void	NET_SendPacket (netsrc_t sock, int length, void *data, netadr_t to);

bool	NET_CompareAdr (netadr_t *a, netadr_t *b);
bool	NET_CompareBaseAdr (netadr_t *a, netadr_t *b);
bool	NET_IsLocalAddress (netadr_t *adr);
const char *NET_AdrToString (netadr_t *a);
bool	NET_StringToAdr (const char *s, netadr_t *a, short defPort = 0);


class netchan_t
{
public:
	netsrc_t sock;				// NS_CLIENT|NS_SERVER -- used only in Process()/Transmit() for "qport" insertion
								//	and for NET_SendPacket(sock, ...)
	//?? physically, "sock" used only in NET_[Get|Send]Packet() => loopback transfer; to remove this, we
	//?? should insert loopback buffers here?

	int		dropped;			// between last packet and previous

	int		last_received;		// for timeouts
	int		last_sent;			// for retransmits

	netadr_t remote_address;
	int		port;				// qport value to write when transmitting

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

	// methods

	void Setup (netsrc_t sock, netadr_t adr, int qport);
	// returns true if the last reliable message has acked; UNUSED??
	inline bool CanReliable ()
	{
		return (reliable_length == 0);	// if != 0 -- waiting for ack
	}
	// returns true, when have reliable data to (re)transmit
	bool NeedReliable ();
	// combine reliable+unreliable data, send; retransmit reliable, when needed
	// reliable data taken from "message"+"reliable_buf", unreliable - "data"+"length"
	void Transmit (void *data, int length);
	// process/skip incoming message header
	bool Process (sizebuf_t *msg);
};

extern netadr_t	net_from;
extern sizebuf_t net_message;


void	Netchan_Init ();
void	Netchan_OutOfBandPrint (netsrc_t net_socket, netadr_t adr, const char *format, ...);


/*-----------------------------------------------------------------------------
	cmodel.cpp
-----------------------------------------------------------------------------*/

struct cmodel_t;		// full declaration in cmodel.h

#if TRACE_DEBUG
extern bool cm_showTrace;
#endif

extern char map_name[];
extern bool map_clientLoaded;

cmodel_t *CM_LoadMap (const char *name, bool clientload, unsigned *checksum);
cmodel_t *CM_InlineModel (const char *name);	// *1, *2, etc
cmodel_t *CM_InlineModel (int index);

int		CM_NumClusters ();
int		CM_NumInlineModels ();

// creates a clipping hull for an arbitrary box
int		CM_HeadnodeForBox (const CBox &box);

// returns an OR'ed contents mask
int		CM_PointContents (const CVec3 &p, int headnode);
int		CM_TransformedPointContents (const CVec3 &p, int headnode, const CVec3 &origin, const CVec3 &angles);
int		CM_TransformedPointContents (const CVec3 &p, int headnode, const CVec3 &origin, const CAxis &axis);
int		CM_PointModelContents (const CVec3 &p);
bool	CM_CombineTrace (trace_t &trace1, const trace_t &trace2);

const byte *CM_ClusterPVS (int cluster);

void	CM_SetAreaPortalState (int portalnum, bool open);
bool	CM_AreasConnected (int area1, int area2);

int		CM_WriteAreaBits (byte *buffer, int area);
bool	CM_HeadnodeVisible (int headnode, const byte *visbits);

void	CM_WritePortalState (FILE *f);
void	CM_ReadPortalState (FILE *f);

/*-----------------------------------------------------------------------------
	Player movement code (pmove.cpp)
	Common between server and client so prediction matches
-----------------------------------------------------------------------------*/

extern float pm_airaccelerate;
void	Pmove (pmove_t *pmove);


/*-----------------------------------------------------------------------------
	File system (files.cpp)
-----------------------------------------------------------------------------*/

void	InitFileSystem ();
void	FS_Tick ();

const char *FS_NextPath (const char *prevpath);


/*------------- Miscellaneous -----------------*/

// debugging
void DebugPrintf (const char *fmt, ...) PRINTF(1,2);

server_state_t Com_ServerState ();		// this should have just been a cvar...
void	Com_SetServerState (server_state_t state);

// md4.cpp
unsigned Com_BlockChecksum (void *buffer, int length);

// 0 to 1
inline float frand ()
{
	return rand() * (1.0f/RAND_MAX);
}

#if !STATIC_BUILD && IS_RENDERER
// Using inline version will grow executable by ~2Kb (cl_fx.cpp uses a lots of [c|f]rand() calls)
// BUT: require this for dynamic renderer (or move to "engine.h" or to Core)
// Most frequent use:
//	CVec3 dir;
//	dir.Set (crand(), crand(), crand());
//	dir.NormalizeFast ();

// -1 to 1
inline float crand ()
{
	return rand() * (2.0f/RAND_MAX) - 1;
}
#else
float crand ();
#endif


extern cvar_t	*developer;
extern cvar_t	*dedicated;
extern cvar_t	*com_speeds;
extern cvar_t	*timedemo;
extern cvar_t	*timescale;
extern cvar_t	*sv_cheats;

// com_speeds times
extern unsigned time_before_game, time_after_game, time_before_ref, time_after_ref;

bool	Com_CheckCmdlineVar (const char *name);

void	Com_Init (const char *cmdline);
void	Com_Frame (float msec);

extern bool com_isClient, com_isServer;

// this is in the client code, but can be used for debugging from server
void	SCR_DebugGraph (float value, int color);


/*-----------------------------------------------------------------------------
	Non-portable system services (sys_*.cpp)
-----------------------------------------------------------------------------*/

char	*Sys_ConsoleInput ();
void	Sys_ProcessMessages ();


/*-----------------------------------------------------------------------------
	Client / server systems
-----------------------------------------------------------------------------*/

void	CL_Init ();
void	CL_Drop (bool fromError = false);
void	CL_Shutdown ();
void	CL_Frame (float msec, float realMsec);

void	Key_Init ();
void	SCR_BeginLoadingPlaque (bool server = false);

void	SV_Init ();
void	SV_Shutdown (const char *finalmsg, bool reconnect = false);
void	SV_Frame (float msec);


/*-----------------------------------------------------------------------------
	Quake2 map format consts
-----------------------------------------------------------------------------*/

// from FileFormats.h
#define MAX_MAP_AREAS			256
#define	MAX_MAP_AREAPORTALS		1024
#define	MAX_MAP_LEAFS			0x20000		// 128k for Q3, 64k for Q2, 8k for Q1
#define	MAX_MAP_FACES			0x20000 	// 128k for Q3, 64k for Q1/Q2

// contents flags are seperate bits
// a given brush can contribute multiple content bits
// multiple brushes can be in a single leaf

// lower bits are stronger, and will eat weaker brushes completely

#define	CONTENTS_SOLID			0x00000001	// an eye is never valid in a solid
#define	CONTENTS_WINDOW			0x00000002	// translucent, but not watery
#define	CONTENTS_AUX			0x00000004	// for design-time only
#define	CONTENTS_LAVA			0x00000008
#define	CONTENTS_SLIME			0x00000010
#define	CONTENTS_WATER			0x00000020
#define	CONTENTS_MIST			0x00000040	// for design-time only
#define CONTENTS_ALPHA			0x00000080	// from Kingping - can shoot through this

// remaining contents are non-visible, and don't eat brushes

#define	CONTENTS_AREAPORTAL		0x00008000

#define	CONTENTS_PLAYERCLIP		0x00010000
#define	CONTENTS_MONSTERCLIP	0x00020000

// currents can be added to any other contents, and may be mixed
#define	CONTENTS_CURRENT_0		0x00040000
#define	CONTENTS_CURRENT_90		0x00080000
#define	CONTENTS_CURRENT_180	0x00100000
#define	CONTENTS_CURRENT_270	0x00200000
#define	CONTENTS_CURRENT_UP		0x00400000
#define	CONTENTS_CURRENT_DOWN	0x00800000

#define	CONTENTS_ORIGIN			0x01000000	// removed before bsping an entity

#define	CONTENTS_MONSTER		0x02000000	// should never be on a brush, only in game
#define	CONTENTS_DEADMONSTER	0x04000000
#define	CONTENTS_DETAIL			0x08000000	// brushes to be added after vis leafs
#define	CONTENTS_TRANSLUCENT	0x10000000	// auto set if any surface has trans
#define	CONTENTS_LADDER			0x20000000

// content masks
#define	MASK_ALL				(-1)
#define	MASK_SOLID				(CONTENTS_SOLID|CONTENTS_WINDOW)
#define	MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define	MASK_DEADSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW)
#define	MASK_MONSTERSOLID		(CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define	MASK_WATER				(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define	MASK_SHOT				(CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEADMONSTER)
#define MASK_CURRENT			(CONTENTS_CURRENT_0|CONTENTS_CURRENT_90|CONTENTS_CURRENT_180|CONTENTS_CURRENT_270|	\
								CONTENTS_CURRENT_UP|CONTENTS_CURRENT_DOWN)

// surface flags
#define	SURF_LIGHT				0x0001		// value will hold the light strength

#define	SURF_SLICK				0x0002		// effects game physics

#define	SURF_SKY				0x0004		// don't draw, but add to skybox
#define	SURF_WARP				0x0008		// turbulent water warp
#define	SURF_TRANS33			0x0010
#define	SURF_TRANS66			0x0020
#define	SURF_FLOWING			0x0040		// scroll towards angle
#define	SURF_NODRAW				0x0080		// don't bother referencing the texture

// added since 4.00
// Kingpin (used for non-KP maps from scripts too)
#define SURF_ALPHA				0x1000
#define	SURF_SPECULAR			0x4000		// have a bug in KP's headers: SPECULAR and DIFFUSE consts are 0x400 and 0x800
#define	SURF_DIFFUSE			0x8000

#define SURF_AUTOFLARE			0x2000		// just free flag (should use extra struc for dBsp2Texinfo_t !!)



#endif // QCOMMON_H
