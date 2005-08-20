#include <winsock2.h>
#if 0
// this will require ws2_32.dll (WinSock2 update for Win95)
#pragma comment (lib, "ws2_32.lib")
#else
#pragma comment (lib, "wsock32.lib")
#endif

#include "qcommon.h"


static const char *NET_ErrorString ()
{
	static const struct {
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

	int code = WSAGetLastError ();
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


/*-----------------------------------------------------------------------------
	Address manipulations
-----------------------------------------------------------------------------*/

static void NetadrToSockadr (netadr_t *a, SOCKADDR *s)
{
	memset (s, 0, sizeof(*s));

	switch (a->type)
	{
	case NA_BROADCAST:
		{
			SOCKADDR_IN	*s1 = (SOCKADDR_IN*)s;
			s1->sin_family      = AF_INET;
			s1->sin_port        = a->port;
			s1->sin_addr.s_addr = INADDR_BROADCAST;
		}
		break;
	case NA_IP:
		{
			SOCKADDR_IN	*s1 = (SOCKADDR_IN*)s;
			s1->sin_family      = AF_INET;
			s1->sin_port        = a->port;
			s1->sin_addr.s_addr = *(int *)&a->ip; // copy 4 bytes as DWORD
		}
		break;
	}
}

static void SockadrToNetadr (SOCKADDR *s, netadr_t *a)
{
	if (s->sa_family == AF_INET)
	{
		a->type        = NA_IP;
		*(int *)&a->ip = ((SOCKADDR_IN *)s)->sin_addr.s_addr;
		a->port        = ((SOCKADDR_IN *)s)->sin_port;
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
		return (memcmp (a->ip, b->ip, 4) == 0) && (a->port == b->port);
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
		return memcmp (a->ip, b->ip, 4) == 0;
	}

	return false;
}

char *NET_AdrToString (netadr_t *a)
{
	static TString<64> s;

	if (a->type == NA_LOOPBACK)
		s = "loopback";
	else // if (a->type == NA_IP)
		s.sprintf ("%d.%d.%d.%d:%d", a->ip[0], a->ip[1], a->ip[2], a->ip[3], ntohs(a->port));

	return s;
}


/*
localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
*/
static bool StringToSockaddr (const char *s, SOCKADDR *sadr)
{
	memset (sadr, 0, sizeof(*sadr));

	SOCKADDR_IN *s1 = (SOCKADDR_IN*)sadr;
	s1->sin_family = AF_INET;
	s1->sin_port   = 0;

	char	copy[128];
	strcpy (copy, s);
	// strip off a trailing :port if present
	for (char *colon = copy; *colon; colon++)
		if (*colon == ':')
		{
			*colon = 0;
			s1->sin_port = htons (atoi (colon+1));
			break;
		}

	if (copy[0] >= '0' && copy[0] <= '9')
	{
		*(int *) &s1->sin_addr = inet_addr (copy);
	}
	else
	{
		struct hostent *h;
		if (!(h = gethostbyname (copy)))
			return 0;
		*(int *) &s1->sin_addr = *(int *)h->h_addr_list[0];
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
bool NET_StringToAdr (const char *s, netadr_t *a)
{
	if (!strcmp (s, "localhost"))
	{
		memset (a, 0, sizeof(*a));
		a->type = NA_LOOPBACK;
		return true;
	}

	SOCKADDR sadr;
	if (!StringToSockaddr (s, &sadr)) return false;

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


bool NET_GetLoopPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_msg)
{
	loopback_t *loop = &loopbacks[sock];

	if (loop->send - loop->get > MAX_LOOPBACK)
		loop->get = loop->send - MAX_LOOPBACK;

	if (loop->get >= loop->send)
		return false;

	int i = loop->get & (MAX_LOOPBACK-1);
	loop->get++;

	memcpy (net_msg->data, loop->msgs[i].data, loop->msgs[i].datalen);
	net_msg->cursize = loop->msgs[i].datalen;

	memset (net_from, 0, sizeof(*net_from));
	net_from->type = NA_LOOPBACK;
	return true;
}


void NET_SendLoopPacket (netsrc_t sock, int length, void *data, netadr_t to)
{
	loopback_t *loop = &loopbacks[sock^1];

	int i = loop->send & (MAX_LOOPBACK-1);
	loop->send++;

	memcpy (loop->msgs[i].data, data, length);
	loop->msgs[i].datalen = length;
}


/*-----------------------------------------------------------------------------
	Socket communication
-----------------------------------------------------------------------------*/

static int netSocket;

bool NET_GetPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_msg)
{
	if (NET_GetLoopPacket (sock, net_from, net_msg))
		return true;

	if (!netSocket) return false;

	SOCKADDR from;
	int fromlen = sizeof(from);
	int ret = recvfrom (netSocket, (char*)net_msg->data, net_msg->maxsize, 0, (SOCKADDR *)&from, &fromlen);

	SockadrToNetadr (&from, net_from);

	if (ret == SOCKET_ERROR)
	{
		int err = WSAGetLastError();

		if (err == WSAEWOULDBLOCK || err == WSAECONNRESET)	// WSAECONNRESET: see Q263823 in MSDN (applies to Win2k+)
			return false;
		if (err == WSAEMSGSIZE)
			Com_WPrintf ("Oversize packet from %s\n", NET_AdrToString(net_from));
		else
			Com_DPrintf ("NET_GetPacket(%s): %s\n", NET_AdrToString(net_from), NET_ErrorString());
		return false;
	}

	if (ret == net_msg->maxsize)
	{
		Com_WPrintf ("Oversize packet from %s\n", NET_AdrToString (net_from));
		return false;
	}

	net_msg->cursize = ret;
	return true;
}


void NET_SendPacket (netsrc_t sock, int length, void *data, netadr_t to)
{
	if (to.type == NA_LOOPBACK)
	{
		NET_SendLoopPacket (sock, length, data, to);
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

		Com_DPrintf ("NET_SendPacket(%s,%d): %s\n", NET_AdrToString (&to), length, NET_ErrorString ());
	}
}


/*-----------------------------------------------------------------------------
	Socket initialization
-----------------------------------------------------------------------------*/

static int IPSocket (char *net_interface, int port)
{
	int newsocket;

	if ((newsocket = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
	{
		if (WSAGetLastError() != WSAEAFNOSUPPORT)
			Com_WPrintf ("IPSocket: socket: %s", NET_ErrorString());
		return 0;
	}

	// make it non-blocking
	static unsigned long _true = 1;
	if (ioctlsocket (newsocket, FIONBIO, &_true) == SOCKET_ERROR)
	{
		Com_WPrintf ("IPSocket: nonblocking: %s\n", NET_ErrorString());
		return 0;
	}

	// make it broadcast capable
	int i = 1;
	if (setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) == SOCKET_ERROR)
	{
		Com_WPrintf ("IPSocket: broadcast: %s\n", NET_ErrorString());
		return 0;
	}

	SOCKADDR_IN address;
	if (!stricmp(net_interface, "localhost"))
		address.sin_addr.s_addr = INADDR_ANY;
	else
		StringToSockaddr (net_interface, (SOCKADDR *)&address);

	address.sin_port   = htons (port);
	address.sin_family = AF_INET;
	if (!bind (newsocket, (SOCKADDR*)&address, sizeof(address)))
		return newsocket;

	// error allocating socket -- try different port
	Com_Printf ("IPSocket: unable to bind to port %d. Trying next port\n", port);
	for (i = 1; i < 32; i++)
	{
		port++;
		address.sin_port = htons (port);
		if (!bind (newsocket, (SOCKADDR*)&address, sizeof(address)))
		{
			Com_Printf ("IPSocket: allocated port %d\n", port);
			return newsocket;
		}
	}
	// this may happen when no TCP/IP support available, or when running 32 applications with
	// such port address
	Com_WPrintf ("IPSocket: unable to allocate address (%s)\n", NET_ErrorString ());

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
		cvar_t *ip = Cvar_Get ("ip", "localhost", CVAR_NOSET);
		int port = Cvar_Get ("port", va("%d", PORT_SERVER), CVAR_NOSET)->integer;
		netSocket = IPSocket (ip->string, port);
		if (!netSocket) multiplayer = false;
	}

	netConfig = multiplayer;
}

void NET_Init ()
{
	guard(NET_Init);

	WSADATA winsockdata;
	if (WSAStartup (MAKEWORD(1,1), &winsockdata))
		Com_FatalError ("Winsock initialization failed");

	Com_Printf ("Winsock Initialized\n");

	unguard;
}


void NET_Shutdown (void)
{
	NET_Config (false);	// close sockets
	WSACleanup ();
}
