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
// net_wins.c

#include <winsock2.h>
#include <wsipx.h>
#include "../qcommon/qcommon.h"

#define	MAX_LOOPBACK	8

typedef struct
{
	byte	data[MAX_MSGLEN];
	int		datalen;
} loopmsg_t;

typedef struct
{
	loopmsg_t msgs[MAX_LOOPBACK];
	int		get, send;
} loopback_t;


cvar_t		*net_shownet;
static cvar_t	*noudp;
static cvar_t	*noipx;

static loopback_t	loopbacks[2];
static int			ip_socket;
static int			ipx_socket;


/*
====================
NET_ErrorString
====================
*/
static char *NET_ErrorString (void)
{
	int		i, code;
	char	*s;
	static struct {
		int		code;
		char	*str;
	} info[] = {
		// list winsock2 error consts
#define T(str)	{WSA##str, #str}
		T(EINTR), T(EBADF), T(EACCES), T(EDISCON), T(EFAULT), T(EINVAL), T(EMFILE),
		T(EWOULDBLOCK), T(EINPROGRESS), T(EALREADY), T(ENOTSOCK), T(EDESTADDRREQ),
		T(EMSGSIZE), T(EPROTOTYPE), T(ENOPROTOOPT), T(EPROTONOSUPPORT), T(ESOCKTNOSUPPORT),
		T(EOPNOTSUPP), T(EPFNOSUPPORT), T(EAFNOSUPPORT), T(EADDRINUSE), T(EADDRNOTAVAIL),
		T(ENETDOWN), T(ENETUNREACH), T(ENETRESET), T(ECONNABORTED), T(ECONNRESET), T(ENOBUFS),
		T(EISCONN), T(ENOTCONN), T(ESHUTDOWN), T(ETOOMANYREFS), T(ETIMEDOUT), T(ECONNREFUSED),
		T(ELOOP), T(ENAMETOOLONG), T(EHOSTDOWN), T(EHOSTUNREACH), T(ENOTEMPTY), T(EPROCLIM),
		T(EUSERS), T(EDQUOT), T(ESTALE), T(EREMOTE), T(SYSNOTREADY), T(VERNOTSUPPORTED),
		T(NOTINITIALISED), T(EDISCON), T(ENOMORE), T(ECANCELLED), T(EINVALIDPROCTABLE),
		T(EINVALIDPROVIDER), T(EPROVIDERFAILEDINIT), T(SYSCALLFAILURE), T(SERVICE_NOT_FOUND),
		T(TYPE_NOT_FOUND), T(_E_NO_MORE), T(_E_CANCELLED), T(EREFUSED), T(HOST_NOT_FOUND),
		T(TRY_AGAIN), T(NO_RECOVERY), T(NO_DATA)
#undef ERR
	};

	code = WSAGetLastError ();
	s = NULL;
	for (i = 0; i < ARRAY_COUNT(info); i++)
		if (info[i].code == code)
		{
			s = info[i].str;
			break;
		}
	if (s)
		return va("WSA%s", s);
	else
		return va("WSA_ERR_%d", code);
}

//=============================================================================

void NetadrToSockadr (netadr_t *a, SOCKADDR *s)
{
	memset (s, 0, sizeof(*s));

	switch (a->type)
	{
	case NA_BROADCAST:
		{
			SOCKADDR_IN	*s1 = (SOCKADDR_IN*)s;
			s1->sin_family = AF_INET;
			s1->sin_port = a->port;
			s1->sin_addr.s_addr = INADDR_BROADCAST;
		}
		break;
	case NA_IP:
		{
			SOCKADDR_IN	*s1 = (SOCKADDR_IN*)s;
			s1->sin_family = AF_INET;
			s1->sin_addr.s_addr = *(int *)&a->ip;
			s1->sin_port = a->port;
		}
		break;
	case NA_IPX:
		{
			SOCKADDR_IPX *s1 = (SOCKADDR_IPX*)s;
			s1->sa_family = AF_IPX;
			memcpy(s1->sa_netnum, &a->ipx[0], 4);
			memcpy(s1->sa_nodenum, &a->ipx[4], 6);
			s1->sa_socket = a->port;
		}
		break;
	case NA_BROADCAST_IPX:
		{
			SOCKADDR_IPX *s1 = (SOCKADDR_IPX*)s;
			s1->sa_family = AF_IPX;
			memset(s1->sa_netnum, 0, 4);
			memset(s1->sa_nodenum, 0xff, 6);
			s1->sa_socket = a->port;
		}
		break;
	}
}

void SockadrToNetadr (SOCKADDR *s, netadr_t *a)
{
	if (s->sa_family == AF_INET)
	{
		a->type = NA_IP;
		*(int *)&a->ip = ((SOCKADDR_IN *)s)->sin_addr.s_addr;
		a->port = ((SOCKADDR_IN *)s)->sin_port;
	}
	else if (s->sa_family == AF_IPX)
	{
		a->type = NA_IPX;
		memcpy(&a->ipx[0], ((SOCKADDR_IPX *)s)->sa_netnum, 4);
		memcpy(&a->ipx[4], ((SOCKADDR_IPX *)s)->sa_nodenum, 6);
		a->port = ((SOCKADDR_IPX *)s)->sa_socket;
	}
}


bool NET_CompareAdr (netadr_t *a, netadr_t *b)
{
	if (a->type != b->type)
		return false;

	switch (a->type)
	{
	case NA_LOOPBACK:
		return true;
	case NA_IP:
		if (a->ip[0] == b->ip[0] && a->ip[1] == b->ip[1] && a->ip[2] == b->ip[2] && a->ip[3] == b->ip[3] && a->port == b->port)
			return true;
		break;
	case NA_IPX:
		if ((memcmp(a->ipx, b->ipx, 10) == 0) && a->port == b->port)
			return true;
		break;
	}

	return false;
}

/*
===================
NET_CompareBaseAdr

Compares without the port
===================
*/
bool NET_CompareBaseAdr (netadr_t *a, netadr_t *b)
{
	if (a->type != b->type)
		return false;

	if (a->type == NA_LOOPBACK)
		return true;
	else if (a->type == NA_IP)
	{
		if (a->ip[0] == b->ip[0] && a->ip[1] == b->ip[1] && a->ip[2] == b->ip[2] && a->ip[3] == b->ip[3])
			return true;
	}
	else if (a->type == NA_IPX)
	{
		if ((memcmp(a->ipx, b->ipx, 10) == 0))
			return true;
	}

	return false;
}

char *NET_AdrToString (netadr_t *a)
{
	static	char	s[64];

	if (a->type == NA_LOOPBACK)
		Com_sprintf (ARRAY_ARG(s), "loopback");
	else if (a->type == NA_IP)
		Com_sprintf (ARRAY_ARG(s), "%d.%d.%d.%d:%d", a->ip[0], a->ip[1], a->ip[2], a->ip[3], ntohs(a->port));
	else
		Com_sprintf (ARRAY_ARG(s), "%02x%02x%02x%02x:%02x%02x%02x%02x%02x%02x:%i", a->ipx[0], a->ipx[1], a->ipx[2], a->ipx[3], a->ipx[4], a->ipx[5], a->ipx[6], a->ipx[7], a->ipx[8], a->ipx[9], ntohs(a->port));

	return s;
}


/*
=============
NET_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
bool NET_StringToSockaddr (char *s, SOCKADDR *sadr)
{
	struct hostent	*h;
	char	*colon;
	int		val;
	char	copy[128];

	memset (sadr, 0, sizeof(*sadr));

	if ((strlen(s) >= 23) && (s[8] == ':') && (s[21] == ':'))	// check for an IPX address
	{
		((SOCKADDR_IPX *)sadr)->sa_family = AF_IPX;
		copy[2] = 0;
#define DO(src,dest)	\
	copy[0] = s[src];	\
	copy[1] = s[src + 1];	\
	sscanf (copy, "%x", &val);	\
	((SOCKADDR_IPX *)sadr)->dest = val
		DO(0, sa_netnum[0]);
		DO(2, sa_netnum[1]);
		DO(4, sa_netnum[2]);
		DO(6, sa_netnum[3]);
		DO(9, sa_nodenum[0]);
		DO(11, sa_nodenum[1]);
		DO(13, sa_nodenum[2]);
		DO(15, sa_nodenum[3]);
		DO(17, sa_nodenum[4]);
		DO(19, sa_nodenum[5]);
#undef DO
		sscanf (&s[22], "%u", &val);
		((SOCKADDR_IPX *)sadr)->sa_socket = htons((unsigned short)val);
	}
	else
	{
		SOCKADDR_IN *s1;

		s1 = (SOCKADDR_IN*)sadr;
		s1->sin_family = AF_INET;
		s1->sin_port = 0;

		strcpy (copy, s);
		// strip off a trailing :port if present
		for (colon = copy ; *colon ; colon++)
			if (*colon == ':')
			{
				*colon = 0;
				s1->sin_port = htons((short)atoi(colon+1));
			}

		if (copy[0] >= '0' && copy[0] <= '9')
		{
			*(int *) &s1->sin_addr = inet_addr(copy);
		}
		else
		{
			if (!(h = gethostbyname (copy)))
				return 0;
			*(int *) &s1->sin_addr = *(int *)h->h_addr_list[0];
		}
	}

	return true;
}

/*
=============
NET_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
bool NET_StringToAdr (char *s, netadr_t *a)
{
	SOCKADDR sadr;

	if (!strcmp (s, "localhost"))
	{
		memset (a, 0, sizeof(*a));
		a->type = NA_LOOPBACK;
		return true;
	}

	if (!NET_StringToSockaddr (s, &sadr))
		return false;

	SockadrToNetadr (&sadr, a);

	return true;
}


bool NET_IsLocalAddress (netadr_t *adr)
{
	return adr->type == NA_LOOPBACK;
}

/*
=============================================================================

LOOPBACK BUFFERS FOR LOCAL PLAYER

=============================================================================
*/

bool NET_GetLoopPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message)
{
	int		i;
	loopback_t	*loop;

	loop = &loopbacks[sock];

	if (loop->send - loop->get > MAX_LOOPBACK)
		loop->get = loop->send - MAX_LOOPBACK;

	if (loop->get >= loop->send)
		return false;

	i = loop->get & (MAX_LOOPBACK-1);
	loop->get++;

	memcpy (net_message->data, loop->msgs[i].data, loop->msgs[i].datalen);
	net_message->cursize = loop->msgs[i].datalen;

	memset (net_from, 0, sizeof(*net_from));
	net_from->type = NA_LOOPBACK;
	return true;
}


void NET_SendLoopPacket (netsrc_t sock, int length, void *data, netadr_t to)
{
	int		i;
	loopback_t	*loop;

	loop = &loopbacks[sock^1];

	i = loop->send & (MAX_LOOPBACK-1);
	loop->send++;

	memcpy (loop->msgs[i].data, data, length);
	loop->msgs[i].datalen = length;
}

//=============================================================================

bool NET_GetPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message)
{
	int 	ret, fromlen;
	SOCKADDR from;
	int		net_socket, protocol, err;

	if (NET_GetLoopPacket (sock, net_from, net_message))
		return true;

	for (protocol = 0 ; protocol < 2 ; protocol++)
	{
		net_socket = protocol == 0 ? ip_socket : ipx_socket;
		if (!net_socket) continue;

		fromlen = sizeof(from);
		ret = recvfrom (net_socket, (char*)net_message->data, net_message->maxsize, 0, (SOCKADDR *)&from, &fromlen);

		SockadrToNetadr (&from, net_from);

		if (ret == SOCKET_ERROR)
		{
			err = WSAGetLastError();

			if (err == WSAEWOULDBLOCK || err == WSAECONNRESET)	// WSAECONNRESET: see Q263823 in MSDN (applies to Win2k)
				continue;
			if (err == WSAEMSGSIZE)
				Com_WPrintf ("Oversize packet from %s\n", NET_AdrToString(net_from));
			else
				Com_DPrintf ("NET_GetPacket(%s): %s\n", NET_AdrToString(net_from), NET_ErrorString());
			continue;
		}

		if (ret == net_message->maxsize)
		{
			Com_WPrintf ("Oversize packet from %s\n", NET_AdrToString (net_from));
			continue;
		}

		net_message->cursize = ret;
		return true;
	}

	return false;
}

//=============================================================================

void NET_SendPacket (netsrc_t sock, int length, void *data, netadr_t to)
{
	int		ret;
	SOCKADDR addr;
	int		net_socket;

	switch (to.type)
	{
	case NA_LOOPBACK:
		NET_SendLoopPacket (sock, length, data, to);
		return;
	case NA_BROADCAST:
		net_socket = ip_socket;
		break;
	case NA_IP:
		net_socket = ip_socket;
		break;
	case NA_IPX:
		net_socket = ipx_socket;
		break;
	case NA_BROADCAST_IPX:
		net_socket = ipx_socket;
		break;
	default:
		Com_FatalError ("NET_SendPacket: bad address type %d", to.type);
	}
	if (!net_socket) return;

	NetadrToSockadr (&to, &addr);

	ret = sendto (net_socket, (char*)data, length, 0, &addr, sizeof(addr) );
	if (ret == SOCKET_ERROR)
	{
		int err = WSAGetLastError();

		// wouldblock is silent
		if (err == WSAEWOULDBLOCK)
			return;

		// some PPP links dont allow broadcasts
		if ((err == WSAEADDRNOTAVAIL) && ((to.type == NA_BROADCAST) || (to.type == NA_BROADCAST_IPX)))
			return;

		Com_DPrintf ("NET_SendPacket(%s,%d): %s\n", NET_AdrToString (&to), length, NET_ErrorString ());
	}
}


//=============================================================================


/*
====================
NET_Socket
====================
*/
int NET_IPSocket (char *net_interface, int port)
{
	int		newsocket;
	SOCKADDR_IN address;
	static unsigned long _true = 1;
	int		i, err;

	i = 1;
	if ((newsocket = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
	{
		err = WSAGetLastError();
		if (err != WSAEAFNOSUPPORT)
			Com_WPrintf ("UDP_OpenSocket: socket: %s", NET_ErrorString());
		return 0;
	}

	// make it non-blocking
	if (ioctlsocket (newsocket, FIONBIO, &_true) == SOCKET_ERROR)
	{
		Com_WPrintf ("UDP_OpenSocket: ioctl FIONBIO: %s\n", NET_ErrorString());
		return 0;
	}

	// make it broadcast capable
	if (setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) == SOCKET_ERROR)
	{
		Com_WPrintf ("UDP_OpenSocket: setsockopt SO_BROADCAST: %s\n", NET_ErrorString());
		return 0;
	}

	if (!net_interface || !net_interface[0] || !stricmp(net_interface, "localhost"))
		address.sin_addr.s_addr = INADDR_ANY;
	else
		NET_StringToSockaddr (net_interface, (SOCKADDR *)&address);

	if (port == PORT_ANY)
		address.sin_port = 0;
	else
		address.sin_port = htons((short)port);

	address.sin_family = AF_INET;

	if (bind (newsocket, (SOCKADDR*)&address, sizeof(address)) == SOCKET_ERROR)
	{
		Com_WPrintf ("UDP_OpenSocket: bind: %s\n", NET_ErrorString());
		closesocket (newsocket);
		return 0;
	}

	return newsocket;
}


/*
====================
NET_OpenIP
====================
*/

#define NUM_TRIES	10

void NET_OpenIP (void)
{
	cvar_t	*ip;
	int		port, tries;

	ip = Cvar_Get ("ip", "localhost", CVAR_NOSET);
	port = Cvar_Get ("port", va("%d", PORT_SERVER), CVAR_NOSET)->integer;

	ip_socket = NET_IPSocket (ip->string, port);

	for (tries = 0; !ip_socket && tries < NUM_TRIES; tries++)
		ip_socket = NET_IPSocket (ip->string, PORT_ANY);
	if (!ip_socket)
		Com_WPrintf ("OpenIP: Unable to allocate IP socket\n");
}


/*
====================
IPX_Socket
====================
*/
static int NET_IPXSocket (int port)
{
	int		newsocket;
	SOCKADDR_IPX address;
	static unsigned long _true = 1;
	int		err;

	if ((newsocket = socket (PF_IPX, SOCK_DGRAM, NSPROTO_IPX)) == INVALID_SOCKET)
	{
		err = WSAGetLastError();
		if (err != WSAEAFNOSUPPORT)
			Com_WPrintf ("IPX_Socket: socket: %s\n", NET_ErrorString());
		return 0;
	}

	// make it non-blocking
	if (ioctlsocket (newsocket, FIONBIO, &_true) == SOCKET_ERROR)
	{
		Com_WPrintf ("IPX_Socket: ioctl FIONBIO: %s\n", NET_ErrorString());
		closesocket (newsocket);
		return 0;
	}

	// make it broadcast capable
	if (setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&_true, sizeof(_true)) == SOCKET_ERROR)
	{
		Com_WPrintf ("IPX_Socket: setsockopt SO_BROADCAST: %s\n", NET_ErrorString());
		closesocket (newsocket);
		return 0;
	}

	address.sa_family = AF_IPX;
	memset (address.sa_netnum, 0, 4);
	memset (address.sa_nodenum, 0, 6);
	if (port == PORT_ANY)
		address.sa_socket = 0;
	else
		address.sa_socket = htons((short)port);

	if (bind (newsocket, (SOCKADDR*)&address, sizeof(address)) == SOCKET_ERROR)
	{
		Com_WPrintf ("IPX_Socket: bind: %s\n", NET_ErrorString());
		closesocket (newsocket);
		return 0;
	}

	return newsocket;
}


/*
====================
NET_OpenIPX
====================
*/
void NET_OpenIPX (void)
{
	int		port;

	port = Cvar_Get ("port", va("%i", PORT_SERVER), CVAR_NOSET)->integer;
	ipx_socket = NET_IPXSocket (port);
	if (!ipx_socket)
	{
		//!! try different sockets
		Com_Printf ("OpenIPX: unable to allocate socket. IPX disabled\n");
		Cvar_ForceSet ("noipx", "1");
	}
}


/*
====================
NET_Config

A single player game will only use the loopback code
====================
*/
static bool configured = false;
static bool netConfig;

void NET_Config (bool multiplayer)
{
	if (configured && netConfig == multiplayer)
		return;

	netConfig = multiplayer;
	configured = true;

	if (!multiplayer || noipx->modified || noudp->modified)
	{	// shut down any existing sockets
		if (ip_socket)
		{
			closesocket (ip_socket);
			ip_socket = 0;
		}
		if (ipx_socket)
		{
			closesocket (ipx_socket);
			ipx_socket = 0;
		}
	}

	if (multiplayer)
	{	// open sockets
		if (!noudp->integer)
			NET_OpenIP ();
		if (!noipx->integer)
			NET_OpenIPX ();
	}

	noipx->modified = noudp->modified = false;
}

static void Cmd_NetRestart_f (void)
{
	if (!configured)
	{
		Com_WPrintf ("Network not started\n");
		return;
	}
	NET_Config (netConfig);
}


//===================================================================


static WSADATA		winsockdata;

/*
====================
NET_Init
====================
*/
void NET_Init (void)
{
	int		r;

CVAR_BEGIN(vars)
	CVAR_VAR(noudp, 0, 0),
	CVAR_VAR(noipx, 0, 0),
	CVAR_VAR(net_shownet, 0, 0)
CVAR_END

	RegisterCommand ("net_restart", Cmd_NetRestart_f);
	Cvar_GetVars (ARRAY_ARG(vars));

	r = WSAStartup (MAKEWORD(1, 1), &winsockdata);
	if (r) Com_FatalError ("Winsock initialization failed.");

	Com_Printf("Winsock Initialized\n");
}


/*
====================
NET_Shutdown
====================
*/
void NET_Shutdown (void)
{
	NET_Config (false);	// close sockets
	WSACleanup ();
}
