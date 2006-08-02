#include <winsock2.h>
#include "qcommon.h"

#if 0
// This library will require ws2_32.dll (WinSock2 update for Win95)
// Anyway, wsock32.dll will redirect most functions to ws2_32.dll
#pragma comment (lib, "ws2_32.lib")
#else
#pragma comment (lib, "wsock32.lib")
#endif


#if MAX_DEBUG
static const char *GetErrorString (int code)
{
	static const struct {
		int		code;
		char	*str;
	} info[] = {
		// list winsock2 error consts
		//?? linux: different set of errors? + no "WSA..." prefix
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

	const char *s = NULL;
	for (int i = 0; i < ARRAY_COUNT(info); i++)
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
#else
static const char *GetErrorString (int code)
{
	return va("WSA_%d: %s", code, appGetSystemErrorMessage (code));
}
#endif // MAX_DEBUG


/*-----------------------------------------------------------------------------
	Address manipulations
-----------------------------------------------------------------------------*/

static void NetadrToSockadr (netadr_t *a, SOCKADDR *s)
{
	memset (s, 0, sizeof(*s));
	SOCKADDR_IN	*s1 = (SOCKADDR_IN*)s;
	s1->sin_family      = AF_INET;
	s1->sin_addr.s_addr = (a->type == NA_BROADCAST) ? INADDR_BROADCAST : a->ip4;
	s1->sin_port        = a->port;
}

static void SockadrToNetadr (SOCKADDR *s, netadr_t *a)
{
	a->type = NA_IP;
	a->ip4  = ((SOCKADDR_IN *)s)->sin_addr.s_addr;
	a->port = ((SOCKADDR_IN *)s)->sin_port;
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
		return (a->ip4 == b->ip4) && (a->port == b->port);
	}

	return false;
}


// Compares without the port
bool NET_CompareBaseAdr (netadr_t *a, netadr_t *b)
{
	if (a->type != b->type)
		return false;

	switch (a->type)
	{
	case NA_LOOPBACK:		// no other address parts
		return true;
	case NA_IP:
		return a->ip4 == b->ip4;
	}

	return false;
}

const char *NET_AdrToString (netadr_t *a)
{
	return (a->type == NA_LOOPBACK)
		? "loopback"
		: va("%d.%d.%d.%d:%d", a->ip[0], a->ip[1], a->ip[2], a->ip[3], ntohs(a->port));
}


/*
localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
*/
static bool StringToSockaddr (const char *s, SOCKADDR *sadr, short defPort)
{
	memset (sadr, 0, sizeof(*sadr));

	SOCKADDR_IN *s1 = (SOCKADDR_IN*)sadr;		// s1 -> saddr
	s1->sin_family = AF_INET;
	s1->sin_port   = 0;

	TString<128> Copy; Copy = s;
	// find and process ":port"
	char *p = Copy.chr (':');
	if (p)
	{
		*p++ = 0;
		defPort = atoi (p);
	}
	s1->sin_port = htons (defPort);

	if (Copy[0] >= '0' && Copy[0] <= '9')
	{
		s1->sin_addr.s_addr = inet_addr (Copy);
		if (s1->sin_addr.s_addr == INADDR_NONE)	// invalid address
			return false;
	}
	else
	{
		HOSTENT *h;
		if (!(h = gethostbyname (Copy)))
			return false;
		s1->sin_addr.s_addr = *(unsigned *)h->h_addr_list[0];
	}

	return true;
}

/*
localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
*/
bool NET_StringToAdr (const char *s, netadr_t *a, short defPort)
{
	if (!strcmp (s, "localhost"))
	{
		memset (a, 0, sizeof(*a));
		a->type = NA_LOOPBACK;
		return true;
	}

	SOCKADDR sadr;
	if (!StringToSockaddr (s, &sadr, defPort))
		return false;
	SockadrToNetadr (&sadr, a);
	return true;
}


bool NET_IsLocalAddress (netadr_t *adr)
{
	return adr->type == NA_LOOPBACK;
}


/*-----------------------------------------------------------------------------
	Loopback communication
-----------------------------------------------------------------------------*/
//!! move to qcommon (netchan.cpp?)

#define	MAX_LOOPBACK	8

struct loopmsg_t
{
	byte	data[MAX_MSGLEN];
	int		datalen;
};

struct loopback_t
{
	loopmsg_t msgs[MAX_LOOPBACK];
	int		get, send;
};

static loopback_t loopbacks[2];


static bool GetLoopPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_msg)
{
	loopback_t &loop = loopbacks[sock];

	if (loop.send - loop.get > MAX_LOOPBACK)
		loop.get = loop.send - MAX_LOOPBACK;

	if (loop.get >= loop.send)
		return false;

	int i = loop.get++ & (MAX_LOOPBACK-1);

	memcpy (net_msg->data, loop.msgs[i].data, loop.msgs[i].datalen);
	net_msg->cursize = loop.msgs[i].datalen;

	memset (net_from, 0, sizeof(*net_from));
	net_from->type = NA_LOOPBACK;
	return true;
}


static void SendLoopPacket (netsrc_t sock, int length, void *data, netadr_t &to)
{
	loopback_t &loop = loopbacks[sock^1];

	int i = loop.send++ & (MAX_LOOPBACK-1);

	memcpy (loop.msgs[i].data, data, length);
	loop.msgs[i].datalen = length;
}


/*-----------------------------------------------------------------------------
	Socket communication
-----------------------------------------------------------------------------*/
//!! NOTE: NET_[Get|Send]Packet(): "sock" used for loopback only

static int netSocket;

bool NET_GetPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_msg)
{
	if (GetLoopPacket (sock, net_from, net_msg))
		return true;

	if (!netSocket) return false;

	SOCKADDR from;
	int fromlen = sizeof(from);
	int ret = recvfrom (netSocket, (char*)net_msg->data, net_msg->maxsize, 0, &from, &fromlen);

	SockadrToNetadr (&from, net_from);

	if (ret == SOCKET_ERROR)
	{
		int err = WSAGetLastError();

		if (err == WSAEWOULDBLOCK || err == WSAECONNRESET)	// WSAECONNRESET: see Q263823 in MSDN (applies to Win2k+)
			return false;
		if (err == WSAEMSGSIZE)
			appWPrintf ("Oversize packet from %s\n", NET_AdrToString(net_from));
		else
			Com_DPrintf ("NET_GetPacket(%s): %s\n", NET_AdrToString(net_from), GetErrorString (err));
		return false;
	}

	net_msg->cursize = ret;
	return true;
}


void NET_SendPacket (netsrc_t sock, int length, void *data, netadr_t to)
{
	if (to.type == NA_LOOPBACK)
	{
		SendLoopPacket (sock, length, data, to);
		return;
	}
	if (!netSocket) return;

	SOCKADDR addr;
	NetadrToSockadr (&to, &addr);

	if (sendto (netSocket, (char*)data, length, 0, &addr, sizeof(addr)) == SOCKET_ERROR)
	{
		int err = WSAGetLastError();

		if (err == WSAEWOULDBLOCK)	// block: silent
			return;

		if (err == WSAEADDRNOTAVAIL && to.type == NA_BROADCAST)	// broadcast not allowed
			return;

		Com_DPrintf ("NET_SendPacket(%s,%d): %s\n", NET_AdrToString (&to), length, GetErrorString (err));
	}
}


/*-----------------------------------------------------------------------------
	Socket initialization
-----------------------------------------------------------------------------*/

static int IPSocket (const char *net_interface, int port)
{
	int newsocket;

	if ((newsocket = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		if (err != WSAEAFNOSUPPORT)
			appWPrintf ("IPSocket: socket: %s", GetErrorString (err));
		return 0;
	}

	// make it non-blocking
	static unsigned long _true = 1;
	if (ioctlsocket (newsocket, FIONBIO, &_true) == SOCKET_ERROR)
	{
		appWPrintf ("IPSocket: nonblocking: %s\n", GetErrorString (WSAGetLastError ()));
		return 0;
	}

	// make it broadcast capable
	int i = 1;
	if (setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) == SOCKET_ERROR)
	{
		appWPrintf ("IPSocket: broadcast: %s\n", GetErrorString (WSAGetLastError ()));
		return 0;
	}

	SOCKADDR address;
	SOCKADDR_IN *address1 = (SOCKADDR_IN*) &address;
	if (!stricmp (net_interface, "localhost"))
	{
		address1->sin_family      = AF_INET;
		address1->sin_addr.s_addr = INADDR_ANY;
	}
	else
		StringToSockaddr (net_interface, &address, 0);
	address1->sin_port = htons (port);

	if (!bind (newsocket, &address, sizeof(address)))
		return newsocket;

	// error allocating socket -- try different port
	appPrintf ("IPSocket: unable to bind to port %d. Trying next port\n", port);
	for (i = 0; i < 32; i++)
	{
		port++;
		address1->sin_port = htons (port);
		if (!bind (newsocket, &address, sizeof(address)))
		{
			appPrintf ("IPSocket: allocated port %d\n", port);
			return newsocket;
		}
	}
	// this may happen when no TCP/IP support available, or when running 32 applications with
	// such port address
	appWPrintf ("IPSocket: unable to allocate address (%s)\n", GetErrorString (WSAGetLastError ()));
	closesocket (newsocket);
	return 0;
}


static bool netConfig = false;	// initial value is single-player

void NET_Config (bool multiplayer)
{
	if (netConfig == multiplayer) return;

	if (!multiplayer && netSocket)
	{
		// shut down socket
		closesocket (netSocket);
		netSocket = 0;
	}

	if (multiplayer)
	{
		// open socket
		cvar_t *ip   = Cvar_Get ("ip", "localhost", CVAR_NOSET);		//?? useless cvar
		cvar_t *port = Cvar_Get ("port", STR(PORT_SERVER), CVAR_NOSET);	//?? useless cvar
		netSocket = IPSocket (ip->string, port->integer);
		if (!netSocket) multiplayer = false;
	}

	netConfig = multiplayer;
}


void NET_Init ()
{
	guard(NET_Init);

#if _WIN32
	//?? may be, non-fatal error; when try to NET_Config(true) -> DropError("no network")
	WSADATA data;
	if (int err = WSAStartup (MAKEWORD(1,1), &data))
		appError ("WSAStartup failed: %s", GetErrorString (err));

	appPrintf ("WinSock: version %d.%d (%d.%d)\n",
		data.wVersion >> 8, data.wVersion & 255,
		data.wHighVersion >> 8, data.wHighVersion & 255);
//	appPrintf ("%s\n", data.szDescription);
#endif

	unguard;
}


void NET_Shutdown ()
{
	NET_Config (false);	// close sockets
#if _WIN32
	WSACleanup ();
#endif
}
