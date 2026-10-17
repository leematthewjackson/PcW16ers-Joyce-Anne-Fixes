/* liblink: Simulate parallel / serial hardware for emulators

    Copyright (C) 2002, 2023  John Elliott <seasip.webmaster@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "ll.h"
#include "llparsocket.h"
#include "llspacket.h"

#ifdef HAVE_WINSOCK_H
#define AGAIN (WSAGetLastError() == WSAEWOULDBLOCK)
#else
#define AGAIN (errno == EWOULDBLOCK)
#define ioctlsocket ioctl
#endif

static ll_error_t handle_incoming(LL_PARSOCKET *trueself, LLS_PACKET **result);
static ll_error_t llps_set_async(LL_PARSOCKET *trueself, int nonblocking);
static ll_error_t send_connect(LL_PARSOCKET *trueself);
static ll_error_t send_hangup(LL_PARSOCKET *trueself);

/* How does it work?
 *
 * When a port is set up, it operates either as a client or as a server.
 * This is specified by the filename input, which is of the form
 *
 * domain:x:name
 *
 * where x is C for client, S for server. domain is "unix" or "tcp". 
 * "name" is a filename (UNIX domain sockets) or a hostname+port (TCP/IP).
 *
 * Servers have two states: "listening" and "connected". If a server is in
 * "listening" mode, then any read() or rctl() command will attempt to do 
 * an accept(); and any send() or sctl() command will fail silently. 
 *
 * Clients are always connected.
 *
 * The packets sent backwards and forwards across the link are formed:
 * DB type	;1 byte
 * DB payload	;0-4 bytes, depending on type
 *
 * Any packet sent expects a reply.
 *
 * Types are:
 * 00	Result.	4 bytes payload - an LL_E_ number. LL_E_SYSERR is forbidden.
 * 01	Connect. Sent by the client when its open() is called. Payload is
 *              4 bytes; an integer that must be 0.
 * 02	Hang up. Sent by the client when its close() is called. Payload is
 * 		likewise an integer that must be 0.
 * 03	Abort.	 Can be sent by either end to shut down the link.
 *
 * 08	Set data lines.    1 byte payload  - data.
 * 09	Set control lines. 4 bytes payload - control lines.
 * 0A	Set cable type.    4 bytes payload. LL_CABLE_*: 0=normal, 1=LapLink.
 * 0B   Set data&control.  7 bytes payload. length=5: 4 bytes control, 1 data.
 *
 * Note: ANY future packet types will be at least 3 bytes - a command code,
 * and then the packet length (16bit).
 *
 */

/*
static void logfunc(const char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	vfprintf(stderr, s, ap);
	va_end(ap);
}

#define LOG(x) logfunc x


static const char *LOGTYPE(int t)
{
	static char buf[30];
	static const char *pkt_type[] = 
	{
		"PT_RESULT", "PT_CONNECT", "PT_HANGUP",
		"PT_ABORT",  "0x04",       "0x05",
		"0x06",      "0x07",       "PT_SETDATA",
		"PT_SETCTRL","PT_SETCABLE","PT_SETCDATA"
	};

	if (t >= PT_RESULT && t <= PT_SETCDATA) return pkt_type[t];

	sprintf(buf, "0x%02x", t & 0xFF);
	return buf;
}
*/
#define LOG(x)


/* Read a packet & parse it into a structure 
 */ 

static ll_error_t read_packet(LL_PARSOCKET *trueself, LLS_PACKET **ppkt)
{
	LLS_PACKET *pkt2, *pkt;
	int plen;
	unsigned char buf[7];
	unsigned char *rptr;
	ll_error_t e;

/*	LOG(("read_packet begins\n")); */
	if (!ppkt) return LL_E_BADPTR;
	pkt = malloc(sizeof(LLS_PACKET));
	if (!pkt) return LL_E_NOMEM;
	*ppkt = pkt;

	/* Read 1st packet byte */
	do
	{
		e = recv(trueself->llps_sockfd2, &pkt->type, 1, 0);
		if (e < 0) 
		{
			free(pkt); 
			*ppkt = NULL;
			if (AGAIN) 
			{
/* LOG(("read_packet fails: LL_E_AGAIN\n")); */
				return LL_E_AGAIN;
			}
/* LOG(("read_packet fails: LL_E_SYSERR\n")); */
			return LL_E_SYSERR; 
		} 
	} while (e < 1);
/*	LOG(("read_packet receives type byte %02x: %s\n", pkt->type, LOGTYPE(pkt->type))); */

	plen = 0;	
	switch(pkt->type)	
	{
		case PT_RESULT:   plen = 4; break;
		case PT_CONNECT:  plen = 4; break;
		case PT_HANGUP:   plen = 4; break;
		case PT_ABORT:    plen = 0; break;
		case PT_SETCABLE: plen = 4; break;
		case PT_SETDATA:  plen = 1; break;
		case PT_SETCTRL:  plen = 4; break;
		case PT_SETCDATA: plen = 7; break;
		default: plen = 2; break;
	}
	/* Read payload */
/*	LOG(("read_packet reading payload of %d bytes\n", plen)); */
	rptr = buf;
	while (plen)
	{
                e = recv(trueself->llps_sockfd2, rptr, plen, 0);
		if (e < 0) 
		{
			if (!AGAIN)
			{
				free(pkt); 
				*ppkt = NULL;
				return LL_E_SYSERR; 
			}
			/* else: we're in non-blocking mode and have a partial
			 * packet; the rest should be here soon. Just keep
			 * trying, to avoid having to save that partial state.
			 * (XXX This is a busy-wait, but not one that ought
			 * to happen much if everything's working correctly.)
			 */
		}
		else
		{
			plen -= e;
			rptr += e;
		}
        }
	plen = 0;
	switch (pkt->type)
	{
		case PT_RESULT: case PT_CONNECT: 
		case PT_HANGUP: case PT_SETCTRL:	
		case PT_SETCABLE:
			pkt->payload.i4 = buf[0] << 24 | buf[1] << 16 |
				          buf[2] <<  8 | buf[3];
			break;
		case PT_SETCDATA:
			pkt->payload.i4 = buf[2] << 24 | buf[3] << 16 |
				          buf[4] <<  8 | buf[5];
			pkt->data       = buf[6];
			break;
		case PT_SETDATA:
			pkt->payload.i1 = buf[0];
			break;
		case PT_ABORT:
			break;
		default:
			pkt->payload.i2 = buf[0] << 8 | buf[1];
			plen = pkt->payload.i2;
			break;
	}

	if (pkt->type == PT_SETCDATA)
	{
     	   LOG(("Incoming: %02x Type=%s Payload=%08x %02x ",  
		        ((unsigned char *)trueself)[0],  
                       LOGTYPE(pkt->type), pkt->payload.i4, pkt->data)); 
	}
	else
	{
     	   LOG(("Incoming: %02x Type=%s Payload=%08x ",  
		        ((unsigned char *)trueself)[0],  
                       LOGTYPE(pkt->type), pkt->payload.i4)); 
	}
	if (pkt->type == PT_SETCTRL)
	{
		char buf[161];
		
		ll_dump_signals(buf, pkt->payload.i4);
		LOG(("%s", buf));
	}
	LOG(("\n"));

	/* Read trailer, if any. Currently none is supported */
	if (!plen) return LL_E_OK;	/* No trailer */


/*	LOG(("read_packet reading trailer of %d bytes\n", plen); */
	pkt2 = malloc(plen + sizeof(LLS_PACKET));
	if (!pkt2) 
	{
		free(pkt);
		*ppkt = NULL;
		return LL_E_NOMEM;
	}
	pkt2->type       = pkt->type;
	pkt2->payload.i2 = pkt->payload.i2;
	free(pkt);
	*ppkt = pkt2;
	rptr = pkt2->trailer;
	while (plen)
	{
                e = recv(trueself->llps_sockfd2, rptr, plen, 0);
		if (e < 0) 
		{
			if (!AGAIN)
			{
				free(pkt2); 
				*ppkt = NULL;
				return LL_E_SYSERR; 
			} /* else, as above: just keep trying */
		}
		else
		{
			plen -= e;
			rptr += e;
		}
	}
	return LL_E_OK;	
}



static ll_error_t send_packet(LL_PARSOCKET *trueself, unsigned char *pkt, int len)
{
	int e;
	unsigned char *p = pkt;

        LOG(("Outgoing: %02x %s:", 
		        ((unsigned char *)trueself)[0],  
			LOGTYPE(pkt[0])));
	for (e = 1; e < len; e++) LOG(("%02x ", pkt[e]));
	LOG(("\n"));
   
	while (len)
	{
        	e = send(trueself->llps_sockfd2, p, len, 0);
		if (e < 0) return LL_E_SYSERR;
		len -= e;
		p += e;
	}
/*	LOG(("send_packet complete\n")); */
	return LL_E_OK;
}


static ll_error_t send_connect(LL_PARSOCKET *trueself)
{
	unsigned char pkt[5];

	pkt[0] = PT_CONNECT;
	memset(pkt + 1, 0, 4);
	return send_packet(trueself, pkt, 5);
}



static ll_error_t send_hangup(LL_PARSOCKET *trueself)
{
	unsigned char pkt[5];

	pkt[0] = PT_HANGUP;
	memset(pkt + 1, 0, 4);
	return send_packet(trueself, pkt, 5);
}




static ll_error_t send_result(LL_PARSOCKET *trueself, ll_error_t err)
{
	unsigned char pkt[5];

	pkt[0] = PT_RESULT;
	pkt[1] = (err >> 24) & 0xFF;
	pkt[2] = (err >> 16) & 0xFF;
	pkt[3] = (err >>  8) & 0xFF;
	pkt[4] = (err      ) & 0xFF;

	return send_packet(trueself, pkt, 5);
}

static ll_error_t read_result(LL_PARSOCKET *trueself)
{
	LLS_PACKET *pkt = NULL;
	int nreqs = 0;
	ll_error_t err;
	int async = trueself->llps_async;	


/* Deal with crossover: if the other end happened to send a request at the
 * same time we did, handle_incoming() will act on it and respond to it.
 * It'll only return pkt non-NULL if it's a reply to us. */
	do {
/* Switch to synchronous mode to read results */
/*		LOG(("Switch to synchronous mode to read results @\n")); */
		err = llps_set_async(trueself, 0);
		if (err) return err;
		err = handle_incoming(trueself, &pkt);
/*		LOG(("Restore async mode to %d\n", async)); */
/* And switch back to whatever mode was in use before */
		llps_set_async(trueself, async);
		if (err) return err;
/* Only tolerate one request, since they should have seen and acted on our
 * request before seeing our reply to theirs, which they should have been
 * waiting for */
		if (!pkt) { nreqs++; }
	} while (!pkt && nreqs <= 1);

/* TODO: Check for PT_ABORT here */
	if (!pkt || pkt->type != PT_RESULT) 
	{
		free(pkt);
		return LL_E_UNEXPECT;
	}
	err = pkt->payload.i4;
	free(pkt);
	return err;
}


/* Check for an incoming packet. 
 * Return LL_E_AGAIN if there is no packet (nonblocking mode)
 * Return LL_E_OK    if packet was handled successfully
 * Other errors as appropriate
 * If 'result' pointer non-NULL, and incoming packet was a reply, return
 * it in pointer (caller must free it) and return LL_E_OK. */
static ll_error_t handle_incoming(LL_PARSOCKET *trueself, LLS_PACKET **result)
{
	LLS_PACKET *pkt;
	ll_error_t err;

	err = read_packet(trueself, &pkt);
	if (err) return err;

	switch(pkt->type)
	{


		case PT_SETCABLE:
			if (pkt->payload.i4 == LL_CABLE_NORMAL ||
			    pkt->payload.i4 == LL_CABLE_LAPLINK)
			{
		LOG(("Received PT_SETCABLE; setting cable type to %ld\n", pkt->payload.i4));
				trueself->llps_super.lld_cable = pkt->payload.i4;
				free(pkt);
/*		LOG(("Switch to synchronous mode to send results A\n")); */
				err = llps_set_async(trueself, 0);
				if (err) return err;
				return  send_result(trueself, LL_E_OK);
			}
			free(pkt);
/*		LOG(("Switch to synchronous mode to send results B\n")); */
			err = llps_set_async(trueself, 0);
			if (err) return err;
			return  send_result(trueself, LL_E_UNEXPECT);
			
		case PT_SETDATA: 
			trueself->llps_ibyte = pkt->payload.i1;
/* Switch to blocking mode to send result packet */
			free(pkt);
/*		LOG(("Switch to synchronous mode to send results C\n")); */
			err = llps_set_async(trueself, 0);
			if (err) return err;
			return  send_result(trueself, LL_E_OK);
		case PT_SETCDATA:
			trueself->llps_ibyte = pkt->data;
			/* And fall through to PT_SETCTRL */
		case PT_SETCTRL:
			trueself->llps_istat = pkt->payload.i4;
			LOG(("trueself->llps_istat := %04x\n", trueself->llps_istat));
/* Switch to blocking mode to send result packet */
			free(pkt);
/*		LOG(("Switch to synchronous mode to send results D\n")); */
			err = llps_set_async(trueself, 0);
			if (err) return err;
			return  send_result(trueself, LL_E_OK);
		case PT_HANGUP:
			if (pkt->payload.i4 != 0) 
			{
				err = send_result(trueself, LL_E_UNEXPECT);
				if (err) return err;
			}
			else
			{
				err = send_result(trueself, LL_E_OK);
				if (err) return err;
			}
			free(pkt);
			if (trueself->llps_clserver == 'S')
			{
				close(trueself->llps_sockfd2);
				trueself->llps_sockfd2 = INVALID_SOCKET;
				trueself->llps_listening = 1;
			}
/* Report that the connection was shut down */
			return LL_E_CLOSED;
		case PT_RESULT:
			if (result)
			{
/* Caller was expecting this reply. Don't process it, pass it to them. */
				*result = pkt;
			}
/* XXX we should probably complain about getting PT_RESULT when we didn't
 * expect it. For now, fall through. */
			break;

	}
	return LL_E_OK;
}





LL_VTBL llv_parsocket =
{
	sizeof(LL_PARSOCKET),
	"parsocket",
	"Parallel port emulated using sockets",
	llps_open,
	llps_close,
	llps_flush,
	llps_send,
	NULL,	/* fast send byte */
	llps_recv_poll,
	llps_recv_wait,
	llps_sctl,
	llps_rctl_poll,
	llps_rctl_wait,
	llps_sdctl,
	llps_set_cable,
	NULL
};


/* Open connection */
ll_error_t llps_open(LL_PDEV self)
{
	LL_PARSOCKET *trueself;
	int mode;
	char *fname, *sport, *namecopy;
	char opt;
	int iport;
	ll_error_t err;
	struct hostent *hent;

	if (!self || self->lld_clazz != &llv_parsocket || 
	     self->lld_filename == NULL) return LL_E_BADPTR;
	trueself = (LL_PARSOCKET *)self;

	/* Create a socket. The filename passed in will be either:
	 *  'unix:L:filename' or 
	 *  'tcp:L:hostname:port'; since Windows doesn't have Unix domain
	 *  sockets, we don't check for them 
	 *
	 * L is either 'S' (server) or 'C' (client). Hence the end result
	 * will look something like:
	 * 	unix:S/
	 *  */
#ifndef HAVE_WINSOCK_H
	if      (strstr(self->lld_filename, "unix:") == self->lld_filename) mode = 1;
	else 
#endif
	if (strstr(self->lld_filename, "tcp:")  == self->lld_filename) mode = 2;
	else return LL_E_BADNAME; 

	fname = strchr(self->lld_filename, ':') + 1;
	opt = toupper(fname[0]);
	if (opt != 'S' && opt != 'C') return LL_E_BADNAME;
	++fname;
	if (fname[0] != ':') return LL_E_BADNAME;
	++fname;
	/* Now, here we go. In UNIX, use a unix domain socket. In Windows,
	 * there aren't such things so use a TCP/IP socket. */
	trueself->llps_listening = 0;
	trueself->llps_clserver = opt;
	trueself->llps_istat = 0;
	switch(mode)
	{
#ifndef HAVE_WINSOCK_H	/* WinSock doesn't have UNIX domain sockets */
		case 1:
		trueself->llps_sockfd = socket(PF_LOCAL, SOCK_STREAM, 0);
		if (trueself->llps_sockfd == INVALID_SOCKET) return LL_E_SYSERR;
		trueself->llps_s_unix.sun_family = PF_UNIX;
		strncpy(trueself->llps_s_unix.sun_path, fname, sizeof(trueself->llps_s_unix.sun_path) - 1);
		trueself->llps_s_unix.sun_path[sizeof(trueself->llps_s_unix.sun_path) - 1] = 0;	
		if (opt == 'S')	/* Server */
		{
			int e = unlink(trueself->llps_s_unix.sun_path);
			if (e && errno == ENOENT) e = 0;
			if (e) return LL_E_SYSERR;		
			if (bind(trueself->llps_sockfd, (struct sockaddr *)(&trueself->llps_s_unix), sizeof(trueself->llps_s_unix)))
				return LL_E_SYSERR;	
/* One at a time, please, lads... */
			if (listen(trueself->llps_sockfd, 1)) return LL_E_SYSERR;
			trueself->llps_listening = 1;
		}
		else
		{
			if (connect(trueself->llps_sockfd, (struct sockaddr *)(&trueself->llps_s_unix), sizeof(trueself->llps_s_unix)))
				return LL_E_SYSERR;	
			trueself->llps_sockfd2 = trueself->llps_sockfd;
/* TODO: Failures here to disconnect() */
			err = send_connect(trueself); if (err) return err;
			err = read_result(trueself); if (err) return err;
/* Send the current state of our data and control lines */
			return llps_sdctl(&trueself->llps_super, trueself->llps_super.lld_lastsdata, trueself->llps_super.lld_lastsctl);
			
		}
		break;
#endif	/* ndef HAVE_WINSOCK_H */
		case 2:	/* For TCP/IP */
		/* Open up an IP socket */
		trueself->llps_sockfd = socket(PF_INET,  SOCK_STREAM, IPPROTO_TCP);
#ifndef HAVE_WINSOCK_H
		{
			/*
			 * On Unix, SO_REUSEADDR is what you want to get
			 * around unnecessary restrictions on binding ports
			 * in the TIME_WAIT state (without it, you can get
			 * failures if you try to use the same listening
			 * port twice in quick succession).
			 * Windows does not have those restrictions, and
			 * SO_REUSEADDR does something different (and daft)
			 * there, so don't do this on Windows.
			 */
			int on = 1;
			setsockopt(trueself->llps_sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
		}
#endif	/* ndef HAVE_WINSOCK_H */
		/* Initialise address */
		memset(&trueself->llps_s_inet, 0, sizeof(trueself->llps_s_inet));
		trueself->llps_s_inet.sin_family = AF_INET;
/* Work out host and port from fname */
		namecopy = malloc(1 + strlen(fname));
		if (!namecopy) return LL_E_NOMEM;
		strcpy(namecopy, fname);

		iport = 0;
		sport = strchr(namecopy, ':');
		if (sport) 
		{
			*sport = 0;
			iport = atoi(sport + 1);	
		}
		if (iport == 0) iport = 8256;	
		hent = gethostbyname(namecopy);
		if (!hent)  { free(namecopy); return LL_E_SOCKERR; }
/* Does not support IPv6 */
		if (hent->h_addrtype != AF_INET) return LL_E_BADNAME;	
		trueself->llps_s_inet.sin_port        = htons(iport);
		memcpy(&trueself->llps_s_inet.sin_addr.s_addr, hent->h_addr_list[0], 4);
		if (opt == 'S')
		{
			if (bind(trueself->llps_sockfd, (struct sockaddr *)(&trueself->llps_s_inet), sizeof(trueself->llps_s_inet)))
				return LL_E_SYSERR;	
			if (listen(trueself->llps_sockfd, 1)) return LL_E_SYSERR;
			trueself->llps_listening = 1;

		}
		else
		{
			if (connect(trueself->llps_sockfd, (struct sockaddr *)(&trueself->llps_s_inet), sizeof(trueself->llps_s_inet)))
				return LL_E_SYSERR;	
			trueself->llps_sockfd2 = trueself->llps_sockfd;
/* TODO: Failures here to disconnect() */
			err = send_connect(trueself); if (err) return err;
			err = read_result(trueself); if (err) return err;
/* Send the current state of our data and control lines */
			return llps_sdctl(&trueself->llps_super, trueself->llps_super.lld_lastsdata, trueself->llps_super.lld_lastsctl);
			
		}

		break;
	}
	return LL_E_OK;
}


/* Close connection */
ll_error_t llps_close(LL_PDEV self)
{
        LL_PARSOCKET *trueself;
	ll_error_t err;


        if (!self || self->lld_clazz != &llv_parsocket) return LL_E_BADPTR;
        trueself = (LL_PARSOCKET *)self;

/* If server & serving, send the 'hangup' packet */
	if (trueself->llps_clserver == 'S' && !(trueself->llps_listening))
	{
		err = send_hangup(trueself);  if (err) return err;
		err = read_result(trueself); if (err) return err; 
		close(trueself->llps_sockfd2);
		trueself->llps_sockfd2 = INVALID_SOCKET;
	}
/* If client, send the 'hangup' packet */
	if (trueself->llps_clserver == 'C')
	{
		err = send_hangup(trueself);  if (err) return err;
		err = read_result(trueself); if (err) return err; 
	}
	close(trueself->llps_sockfd);
	trueself->llps_sockfd = INVALID_SOCKET;

	return LL_E_OK;
}


/* Flush data */
ll_error_t llps_flush(LL_PDEV self)
{
        LL_PARSOCKET *trueself;
        ll_error_t err = LL_E_OK;

        if (!self || (self->lld_clazz != &llv_parsocket)) return LL_E_BADPTR;
        trueself = (LL_PARSOCKET *)self;

/* There's no need to flush sent data, but we should service any requests
 * _from_ the other end, to reduce the time they are stuck waiting for our
 * reply.
 * (XXX: unless this is called very frequently, it's still possible for
 * the other end to get delayed if the local software isn't paying attention
 * to the relevant I/O.) */
	if (!trueself->llps_listening)
	{
		do {
/*		LOG(("Switch to asynchronous mode to flush\n")); */
			err = llps_set_async(trueself, 1);
			if (err) return err;
			err = handle_incoming(trueself, NULL);
		} while (!err);
	}

	return err == LL_E_AGAIN ? LL_E_OK : err;
}


/* Sending data */
ll_error_t llps_send(LL_PDEV self, unsigned char b)
{
        LL_PARSOCKET *trueself;
	ll_error_t e;
	unsigned char pkt[2];

        if (!self || (self->lld_clazz != &llv_parsocket)) return LL_E_BADPTR;
        trueself = (LL_PARSOCKET *)self;

	/* If not connected to the other end, then fail silently */
	if (trueself->llps_listening) return LL_E_OK;

	do
	{
/*		LOG(("Switch to asynchronous mode to send data\n")); */
		e = llps_set_async(trueself, 1); if (e) return e;
		e = handle_incoming(trueself, NULL);
		if (e && e != LL_E_AGAIN) return e;
	}
	while (e != LL_E_AGAIN);

	pkt[0] = PT_SETDATA;
	pkt[1] = b;

        e = send_packet(trueself, pkt, 2);
	if (e) return LL_E_SYSERR;
	return read_result(trueself);
}


/* Setting control lines. Twiddling STROBE sends the byte. */
ll_error_t llps_sctl(LL_PDEV self, unsigned b)
{
        LL_PARSOCKET *trueself;
	ll_error_t e;
	unsigned char pkt[5];

        if (!self || (self->lld_clazz != &llv_parsocket)) return LL_E_BADPTR;
        trueself = (LL_PARSOCKET *)self;

/* Nothing at the other end; fail silently */
	if (trueself->llps_listening) return LL_E_OK;

/* Swallow any packets the other end is sending (eg: SETCABLE) */
	do
	{
/*		LOG(("Switch to asynchronous mode to set control lines\n")); */
		e = llps_set_async(trueself, 1); if (e) return e;
		e = handle_incoming(trueself, NULL);
		if (e && e != LL_E_AGAIN) return e;
	}
	while (e != LL_E_AGAIN);

	pkt[0] = PT_SETCTRL;
	pkt[1] = (b >> 24) & 0xFF;
	pkt[2] = (b >> 16) & 0xFF;
	pkt[3] = (b >>  8) & 0xFF;
	pkt[4] = (b      ) & 0xFF;

        e = send_packet(trueself, pkt, 5);
	if (e) return LL_E_SYSERR;
	return read_result(trueself);
}

/* Setting control+data lines simultaneously.  */
ll_error_t llps_sdctl(LL_PDEV self, unsigned char data, unsigned ctl)
{
        LL_PARSOCKET *trueself;
	ll_error_t e;
	unsigned char pkt[8];

	LOG(("llps_sdctl(%02x,%08x)\n", data, ctl));

        if (!self || (self->lld_clazz != &llv_parsocket)) return LL_E_BADPTR;
        trueself = (LL_PARSOCKET *)self;

/* Nothing at the other end; fail silently */
	if (trueself->llps_listening) return LL_E_OK;

/* Swallow any packets the other end is sending (eg: SETCABLE) */
	do
	{
/*		LOG(("Switch to asynchronous mode to set control lines\n")); */
		e = llps_set_async(trueself, 1); if (e) return e;
		e = handle_incoming(trueself, NULL);
		if (e && e != LL_E_AGAIN) return e;
	}
	while (e != LL_E_AGAIN);

	pkt[0] = PT_SETCDATA;
	pkt[1] = 0;
	pkt[2] = 5;	/* Length of payload */
	pkt[3] = (ctl >> 24) & 0xFF;
	pkt[4] = (ctl >> 16) & 0xFF;
	pkt[5] = (ctl >>  8) & 0xFF;
	pkt[6] = (ctl      ) & 0xFF;
	pkt[7] = data;

        e = send_packet(trueself, pkt, 8);
	if (e) return LL_E_SYSERR;
	return read_result(trueself);
}






/*----------------------------------------------------------------------
 Portable function to set a socket into nonblocking mode.
 Calling this on a socket causes all future read() and write() calls on
 that socket to do only as much as they can immediately, and return 
 without waiting.
 If no data can be read or written, they return -1 and set errno
 to EAGAIN (or EWOULDBLOCK).
 Thanks to Bjorn Reese for this code.
----------------------------------------------------------------------*/
static int setNonblocking(SOCKET fd, int nonblocking)
{
    int flags;

    /* If they have O_NONBLOCK, use the Posix way to do it */
#if defined(O_NONBLOCK)
    /* Fixme: O_NONBLOCK is defined but broken on SunOS 4.1.x and AIX 3.2.5. */
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    if (nonblocking) flags |= O_NONBLOCK;
    else             flags &= ~O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags);
#else
    /* Otherwise, use the old way of doing it */
    flags = nonblocking ? 1 : 0;
    return ioctlsocket(fd, FIONBIO, &flags);
#endif
}     

/* Switch the socket between blocking & nonblocking mode */

static ll_error_t llps_set_async(LL_PARSOCKET *trueself, int nonblocking)
{
	int e;

/*	LOG(("llps_set_async(%d)\n", nonblocking)); */

	if (trueself->llps_listening) 
	     e = setNonblocking(trueself->llps_sockfd, nonblocking);
	else e = setNonblocking(trueself->llps_sockfd2, nonblocking);
	if (e) return LL_E_SYSERR;
	trueself->llps_async = nonblocking;
	return LL_E_OK;
}

/* Listening? */
static ll_error_t llps_wait_answer(LL_PARSOCKET *trueself)
{
	ll_error_t err;
	LLS_PACKET *pkt;
#ifdef HAVE_WINDOWS_H
	size_t len = sizeof(trueself->llps_client);
#else
	socklen_t len = sizeof(trueself->llps_client);
#endif
	while (trueself->llps_listening)
	{
		trueself->llps_sockfd2 = accept(trueself->llps_sockfd, &trueself->llps_client, &len);
		if (trueself->llps_sockfd2 < 0) 
		{
			if (AGAIN) return LL_E_AGAIN;
			return LL_E_SYSERR;
		}
		err = read_packet(trueself, &pkt);
		if (err) return err;

/* First packet must be a Connect packet */
		if (pkt->type != PT_CONNECT ||
		    pkt->payload.i4 != 0)
		{
			err = send_result(trueself, LL_E_UNEXPECT);
			if (err) { free(pkt); return err; }
/* Go back to sleep */
			close(trueself->llps_sockfd2);
			trueself->llps_sockfd2 = INVALID_SOCKET;
			continue;
		}
/* TODO: Password checking would go here */
		free(pkt);
		err = send_result(trueself, LL_E_OK);
		if (err)  return err;
/* If we are using a cable type other than the default, tell the client */
		trueself->llps_listening = 0;
		err = llps_set_cable(&trueself->llps_super, trueself->llps_super.lld_cable);
		if (err)  return err;
/* Tell the other end the current state of our output lines */
		err = llps_sdctl(&trueself->llps_super, trueself->llps_super.lld_lastsdata, trueself->llps_super.lld_lastsctl);
		if (err)  return err;
	}
	return LL_E_OK;
}


static ll_error_t llps_rctl(LL_PARSOCKET *trueself, unsigned *b)
{
	int e;

	if (trueself->llps_listening) 
	{
		e = llps_wait_answer(trueself);	
		if (e) 
		{
			*b = trueself->llps_istat;
			return e;
		}
	}
	e = handle_incoming(trueself, NULL);
	*b = trueself->llps_istat;
	return e;
}

/* Reading control lines */
ll_error_t llps_rctl_poll(LL_PDEV self, unsigned *b)
{
        LL_PARSOCKET *trueself;
	ll_error_t e;

        if (!b || !self || (self->lld_clazz != &llv_parsocket)) return LL_E_BADPTR;
        trueself = (LL_PARSOCKET *)self;
/*		LOG(("Switch to asynchronous mode to read control lines\n"));*/
	e = llps_set_async(trueself, 1); if (e) return e;
	
	e = llps_rctl(trueself, b);
	if (e == LL_E_AGAIN) e = LL_E_OK;
/*	LOG(("llps_rctl_poll returns %04x\n", *b)); */
	return e;
}
 

/* Reading control lines */
ll_error_t llps_rctl_wait(LL_PDEV self, unsigned *b)
{
        LL_PARSOCKET *trueself;

        if (!b || !self || (self->lld_clazz != &llv_parsocket)) return LL_E_BADPTR;
        trueself = (LL_PARSOCKET *)self;
/*	LOG(("Switch to synchronous mode to wait on control lines\n")); */
	llps_set_async(trueself, 0);
	return llps_rctl(trueself, b);
}
 

static ll_error_t llps_recv(LL_PARSOCKET *trueself, unsigned char *b)
{
	int e;

	if (trueself->llps_listening) 
	{
		e = llps_wait_answer(trueself);	
		if (e) return e;
	}
	e = handle_incoming(trueself, NULL);
	*b = trueself->llps_ibyte;
	if (e) return e;
	return 0;
}

/* Reading control lines */
ll_error_t llps_recv_poll(LL_PDEV self, unsigned char *b)
{
        LL_PARSOCKET *trueself;
	ll_error_t e;

        if (!b || !self || (self->lld_clazz != &llv_parsocket)) return LL_E_BADPTR;
        trueself = (LL_PARSOCKET *)self;
/*	LOG(("Switch to asynchronous mode to poll data lines\n")); */
	e = llps_set_async(trueself, 1); if (e) return e;
	
	e = llps_recv(trueself, b);
	if (e == LL_E_AGAIN) e = LL_E_OK;
	return e;
}
 

/* Reading control lines */
ll_error_t llps_recv_wait(LL_PDEV self, unsigned char *b)
{
        LL_PARSOCKET *trueself;

        if (!b || !self || (self->lld_clazz != &llv_parsocket)) return LL_E_BADPTR;
        trueself = (LL_PARSOCKET *)self;
/*	LOG(("Switch to synchronous mode to wait on data lines\n")); */
	llps_set_async(trueself, 0);
	return llps_recv(trueself, b);
}
 


/* Setting the cable at one end sets it at the other */
ll_error_t llps_set_cable(LL_PDEV self, LL_CABLE cable)
{
        LL_PARSOCKET *trueself;
	ll_error_t e;
	unsigned char pkt[5];

        if (!self || (self->lld_clazz != &llv_parsocket)) return LL_E_BADPTR;
        trueself = (LL_PARSOCKET *)self;

	/* If not connected to the other end, then fail silently */
	if (!trueself->llps_listening)
	{
		pkt[0] = PT_SETCABLE;
		pkt[1] = (cable >> 24) & 0xFF;
		pkt[2] = (cable >> 16) & 0xFF;
		pkt[3] = (cable >>  8) & 0xFF;
		pkt[4] =  cable        & 0xFF;

       		e = send_packet(trueself, pkt, 5);
		if (e) return LL_E_SYSERR;
		e = read_result(trueself);
		if (e) return e;
	}
	self->lld_cable = cable;
	return LL_E_OK;
}

