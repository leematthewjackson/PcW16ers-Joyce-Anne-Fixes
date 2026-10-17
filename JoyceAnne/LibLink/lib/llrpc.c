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
#include "lltypes.h"
#include "crc16.h"

#define REG_FUNC 2
#define REG_AL   3
#define REG_BL   4
#define REG_BH   5
#define REG_CL   6
#define REG_CH   7
#define REG_DL   8
#define REG_DH   9

extern unsigned char ll_crc_tab[512];

typedef struct ll_remote
{
	LL_PDEV device;
	LL_END end;
	LL_PROTOCOL protocol;
	LL_PING_CALLBACK ping_callback;
	char client[33];
	char server[33];
	unsigned short wcrc;
} LL_REMOTE;

static inline unsigned short peek2(unsigned char *src)
{
	return (((unsigned short)src[1]) << 8) | src[0];
}

ll_error_t llc_get_end     (LL_PREMOTE remote, LL_END *end)
{
	if (!remote || !end) return LL_E_BADPTR;
	*end = remote->end;
	return LL_E_OK;
}


ll_error_t llc_get_protocol(LL_PREMOTE remote, LL_PROTOCOL *proto)
{
	if (!remote || !proto) return LL_E_BADPTR;
	*proto = remote->protocol;
	return LL_E_OK;
}

#if 1
static inline ll_error_t llc_send_llpacket(LL_PREMOTE self, unsigned char *packet)
{
	return ll_send_llpacket(self->device, self->end, packet);
}

static ll_error_t llc_recv_llpacket(LL_PREMOTE self, unsigned char *packet)
{
	ll_error_t err;
	unsigned char response[3];

	while (1)
	{
		err = ll_recv_llpacket(self->device, self->end, packet);
		if (err) return err;
		if (packet[0] == PING_V2 || packet[0] == PING_V3)
		{
			if (self->ping_callback)
			{
				if (self->ping_callback(self, 1))
				{
					return LL_E_ABORT;
				}
			}

			response[0] = (packet[0] == PING_V2) ? ACK_V2 : ACK_V3;
			response[1] = 1;
			response[2] = packet[0];
			err = ll_send_llpacket(self->device, self->end, 
				response);
			if (err) return err;
		}
		else	
		{
			if (self->ping_callback)
			{
				if (self->ping_callback(self, 1))
				{
					return LL_E_ABORT;
				}
			}
			return err;
		}
	}
}
#else
static ll_error_t llc_send_llpacket(LL_PREMOTE self, unsigned char *packet)
{
	int n;

	fprintf(stderr, "Send: ");
	for (n = 0; n < 2 + packet[1]; n++)
	{
		fprintf(stderr, "%02x ", packet[n]);
	} 
	fprintf(stderr, "\n");
	return ll_send_llpacket(self->device, self->end, packet);
}

static ll_error_t llc_recv_llpacket(LL_PREMOTE self, unsigned char *packet)
{
	int n;

	ll_error_t err = ll_recv_llpacket(self->device, self->end, packet);
	if (!err)
	{
		fprintf(stderr, "Recv: ");
		for (n = 0; n < 2 + packet[1]; n++)
		{
			fprintf(stderr, "%02x ", packet[n]);
		} 
		fprintf(stderr, "\n");
	}	
	return err;
}
#endif



static ll_error_t log_unexpected(unsigned char *recvpkt)
{
	int n;

	fprintf(stderr, "LocoLink client received unexpected packet:\n");
	fprintf(stderr, "Type:   0x%02x\n", recvpkt[0]);
	fprintf(stderr, "Length: 0x%02x\n", recvpkt[1]);
	if (recvpkt[1])
	{
		fprintf(stderr, "Data:   ");
		for (n = 0; n < recvpkt[1]; n++)
		{
			fprintf(stderr, "0x%02x ", recvpkt[2 + n]);
		}
		fprintf(stderr, "\n");
	}
	return LL_E_UNEXPECT;
}

/* LocoLink client */

static ll_error_t llc_connect_v1(LL_PREMOTE self)
{
	unsigned char sendpkt[260];
	unsigned char recvpkt[260];
	ll_error_t err;

	sendpkt[0] = CONNECT_V1;
	sendpkt[1] = 2;
	sendpkt[2] = 0x43;
	sendpkt[3] = 0x31;
	
	err = llc_send_llpacket(self, sendpkt);
	if (err) return err;
	err  = llc_recv_llpacket(self, recvpkt);
	if (err) return err;
	if (recvpkt[0] != RESULT_V1 || recvpkt[1] != 0x01 || recvpkt[2] != CONNECT_V1)
		return LL_E_UNEXPECT;
	strcpy(self->server, "LocoLink 1");
	self->protocol = LOCOLINK_V1;
	return LL_E_OK;
}

static ll_error_t llc_connect_v2(LL_PREMOTE self)
{
	unsigned char sendpkt[260];
	unsigned char recvpkt[260];
	ll_error_t err;
	int clen = strlen(self->client);
	
	if (clen > 32) clen = 32;
	sendpkt[0] = CONNECT_V2;
	sendpkt[1] = 2 + clen;
	sendpkt[2] = 0x41;
	sendpkt[3] = 0x31;	
	strncpy((char *)sendpkt + 4, self->client, 32);
	
	err = llc_send_llpacket(self, sendpkt);
	if (err) return err;
	err  = llc_recv_llpacket(self, recvpkt);
	if (err) return err;
	if (recvpkt[0] != ACK_V2 || recvpkt[1] < 1 || recvpkt[2] != CONNECT_V2)
		return LL_E_UNEXPECT;

	if (recvpkt[1] > 1)
	{
		clen = recvpkt[1] - 1;
		if (clen > 32) clen = 32;	
		strncpy(self->server, (char *)recvpkt + 3, clen);
	}
	self->protocol = LOCOLINK_V2;
	return LL_E_OK;
}



static ll_error_t llc_connect_v3(LL_PREMOTE self)
{
	unsigned char sendpkt[260];
	unsigned char recvpkt[260];
	ll_error_t err;
	int clen = strlen(self->client);
	
	if (clen > 32) clen = 32;
	sendpkt[0] = CONNECT_V3;
	sendpkt[1] = 2 + clen;
	sendpkt[2] = 0x46;
	sendpkt[3] = 0x31;	
	strncpy((char *)sendpkt + 4, self->client, 32);
	
	err = llc_send_llpacket(self, sendpkt);
	if (err) return err;
	err  = llc_recv_llpacket(self, recvpkt);
	if (err) return err;
	if (recvpkt[0] != ACK_V3 || recvpkt[1] < 1 || recvpkt[2] != CONNECT_V3)
		return LL_E_UNEXPECT;
	if (recvpkt[1] > 1)
	{
		clen = recvpkt[1] - 1;
		if (clen > 32) clen = 32;	
		strncpy(self->server, (char *)recvpkt + 3, clen);
	}
	self->protocol = LOCOLINK_V3;
	return LL_E_OK;
}



ll_error_t llc_connect(LL_PREMOTE *premote, LL_PDEV dev, LL_END end, 
			const char *client_desc, LL_PROTOCOL protocol, 
			char *server_desc)
{
	ll_error_t err;
	LL_REMOTE *self;

	if (!dev || !premote) return LL_E_BADPTR;

	self = malloc(sizeof(LL_REMOTE));
	if (!self) return LL_E_NOMEM;	

	memset(self, 0, sizeof(LL_REMOTE));
	self->end = end;
	self->device = dev;
	self->protocol = NO_PROTOCOL;
	self->ping_callback = NULL;
	strncpy(self->client, client_desc, sizeof(self->client) - 1);

	if (protocol != NO_PROTOCOL)
	{
		switch (protocol)
		{
			case LOCOLINK_V1:
				err = llc_connect_v1(self);
				break;
			case LOCOLINK_V2:
				err = llc_connect_v2(self);
				break;
			case LOCOLINK_V3:
				err = llc_connect_v3(self);
				break;
			default: err = LL_E_BADPROTO;
				break;
		}
		if (err == LL_E_OK) /* That worked */
		{
			if (server_desc) strcpy(server_desc, self->server);
			*premote = self;
			return LL_E_OK;
		}
		*premote = NULL;
		free(self);
		return err;
	}
	/* Unknown protocol, let's try guessing. We have to start with v1
	 * first because the v1 server doesn't give a second chance. */
	err = llc_connect_v1(self);
	if (err) err = llc_connect_v2(self);
	if (err) err = llc_connect_v3(self);
	if (!err)
	{
		if (server_desc) strcpy(server_desc, self->server);
		*premote = self;
		return LL_E_OK;
	}
	*premote = NULL;
	free(self);
	return err;
}


/* Terminate a session */
ll_error_t llc_hangup(LL_PREMOTE *self)
{
	unsigned char sendpkt[260];
	ll_error_t err = LL_E_OK;

	if (self == NULL || *self == NULL) return LL_E_BADPTR;

	memset(sendpkt, 0, sizeof(sendpkt));
	switch (self[0]->protocol)
	{
		case LOCOLINK_V1: sendpkt[0] = HANGUP_V1; 
				  err = llc_send_llpacket(self[0], sendpkt);
				  break;
		case LOCOLINK_V2: sendpkt[0] = HANGUP_V2;
				  err = llc_send_llpacket(self[0], sendpkt);
				  break;
		case LOCOLINK_V3: sendpkt[0] = HANGUP_V3;
				  err = llc_send_llpacket(self[0], sendpkt);
				  break;
		case NO_PROTOCOL: err = LL_E_BADPROTO;
				  break;
	}
	free(*self);
	*self = NULL;
	return err;
}

/* Check if the packet is a result type. Returns LL_E_OK / LL_E_FAIL if it is,
 * LL_E_AGAIN otherwise */
static ll_error_t llc_check_success(unsigned char *recvpkt, unsigned short *result)
{
	switch (recvpkt[0])
	{
			case RESULT_V1:	/* v1 result */
				if (recvpkt[3] == 0) return LL_E_OK;
				*result = recvpkt[3];	
				return LL_E_FAIL;
			case SUCCESS_V2:	/* v2 success */
			case SUCCESS_V3:	/* v3 success */
				*result = peek2(recvpkt + 2);
				return LL_E_OK;
			case ERROR_V2:	/* v2 error */
			case ERROR_V3:	/* v3 error */
				*result = peek2(recvpkt + 2);
				return LL_E_FAIL;

	}
	return LL_E_AGAIN;
}


static ll_error_t llc_await_result(LL_PREMOTE self, unsigned short *result)
{
	unsigned char recvpkt[260];
	ll_error_t err;

	err  = llc_recv_llpacket(self, recvpkt);
	if (err) return err;

	err = llc_check_success(recvpkt, result);

	if (err != LL_E_OK && err != LL_E_FAIL)
		return LL_E_UNEXPECT;
	return err;
}


static ll_error_t llc_send_ack(LL_PREMOTE self,
				unsigned char type)
{
	unsigned char sendpkt[260];

	/* Acknowledge returned data */
	memset(sendpkt, 0, sizeof(sendpkt));
	sendpkt[0] = (self->protocol == LOCOLINK_V2) ? ACK_V2 : ACK_V3;
	sendpkt[1] = 1;
	sendpkt[2] = type;
	return llc_send_llpacket(self, sendpkt);
}


static ll_error_t llc_send_data(LL_PREMOTE self,
				REQTYPE type, const unsigned char *src, unsigned len)
{
	unsigned char sendpkt[260];

	memset(sendpkt, 0, sizeof(sendpkt));
	sendpkt[0] = (self->protocol == LOCOLINK_V2) ? DATASEND_V2 : DATASEND_V3;
	if (len > 255) len = 255;
	sendpkt[1] = len;
	memcpy(sendpkt + 2, src, len);
	return llc_send_llpacket(self, sendpkt);
}

static ll_error_t llc_send_asciiz(LL_PREMOTE self,
				REQTYPE type, unsigned maxlen, const char *str)
{
	if (maxlen > strlen(str)) maxlen = strlen(str);

	return llc_send_data(self, type, (unsigned char *)str, maxlen);
}

static unsigned char dos_pktsizes_v2[] =
{
/*		0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 0x */	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 1x */	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 2x */	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 3x */	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 2, 4, 6,
/* 4x */	6, 1, 8, 5, 6, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1,
/* 5x */	1, 1, 1, 1, 1, 1, 1, 8, 1, 1, 1, 5, 1, 1, 1, 1,
/* 6x */	1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, 7, 1, 1, 1,
};

static unsigned char dos_pktsizes_v3[] =
{
/*		0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 0x */	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 7, 7, 1,
/* 1x */	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 2x */	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 3x */	1, 1, 1, 1, 1, 1, 7, 1, 1, 1, 1, 1, 5, 2, 4, 6,
/* 4x */	6, 1, 8, 5, 6, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1,
/* 5x */	1, 1, 1, 1, 1, 1, 1, 8, 1, 1, 1, 5, 1, 1, 1, 1,
/* 6x */	1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, 7, 1, 1, 1,
};



static ll_error_t llc_send_doscall(LL_PREMOTE self,
		unsigned char func, unsigned char al, unsigned short bx,
		unsigned short cx, unsigned short dx)
{
	unsigned char sendpkt[260];

	if (self->protocol != LOCOLINK_V2 && self->protocol != LOCOLINK_V3) 
		return LL_E_BADPROTO;

	memset(sendpkt, 0, sizeof(sendpkt));
	sendpkt[0] = (self->protocol == LOCOLINK_V2) ? DOSCALL_V2 : DOSCALL_V3;
	sendpkt[1] = 8;
	sendpkt[2] = func;
	sendpkt[3] = al;
	sendpkt[4] = bx & 0xFF;
	sendpkt[5] = bx >> 8;
	sendpkt[6] = cx & 0xFF;
	sendpkt[7] = cx >> 8;
	sendpkt[8] = dx & 0xFF;
	sendpkt[9] = dx >> 8;

	if (self->protocol == LOCOLINK_V2 && func <= sizeof(dos_pktsizes_v2))
	{
		sendpkt[1] = dos_pktsizes_v2[func];
	}
	if (self->protocol == LOCOLINK_V3 && func <= sizeof(dos_pktsizes_v3))
	{
		sendpkt[1] = dos_pktsizes_v3[func];
	}

	return llc_send_llpacket(self, sendpkt);
}


static ll_error_t mkfcb(const char *label, unsigned char *fcb)
{
	int n;
	char name[14];
	char buf[12];
	char *p;

	strncpy(name, label, 13);
	name[13] = 0;
	for (n = 0; name[n]; n++)
	{
		if (islower(name[n])) name[n] = toupper(name[n]);
	}
	if (strchr(name, '/') || strchr(name, '\\') || strchr(name, ':'))
	{
		return LL_E_BADNAME;
	}
	p = strchr(name, '.');
	if (p)
	{
		*p = 0;
		sprintf(buf, "%-8.8s", name);
		sprintf(buf + 8, "%-3.3s", p + 1);
	}
	else
	{
		sprintf(buf, "%-11.11s", name);
	}
	memcpy(fcb, buf, 11);
	return LL_E_OK;
}


/************************* DOS function wrappers ************************/
/* Log out all drives */
ll_error_t llc_resetdisks(LL_PREMOTE self, int mode, 
				unsigned short *result)
{
	ll_error_t err;

	if (self == NULL || result == NULL) return LL_E_BADPTR;
	err = llc_send_doscall(self, DRV_ALLRESET, mode,0,0,0);
	if (err) return err;
	return llc_await_result(self, result);
}


/* Select drive */
ll_error_t llc_seldrive(LL_PREMOTE self,
		unsigned char drive, unsigned short *result)
{
	ll_error_t err;

	if (self == NULL || result == NULL) return LL_E_BADPTR;
	err = llc_send_doscall(self, DRV_SET, 0,0,0,drive);
	if (err) return err;
	return llc_await_result(self, result);
}

/* Close FCB (Disk label?) */
ll_error_t llc_closelabel(LL_PREMOTE self, unsigned short *result)
{
	ll_error_t err;

	if (self == NULL || result == NULL) return LL_E_BADPTR;
	err = llc_send_doscall(self, F_CLOSE, 0,0,0,0);
	if (err) return err;
	return llc_await_result(self, result);
}

/* Get disk label */
ll_error_t llc_findlabel(LL_PREMOTE self, 
		unsigned char *label, char drive, unsigned short *result)
{
	ll_error_t err;
	unsigned char recvpkt[260];
	int len;
	unsigned char fcb[43];

	if (self == NULL || label == NULL || result == NULL) return LL_E_BADPTR;
	memset(fcb, 0, sizeof(fcb));
/* LocoLink v3 requires the extended FCB to be passed in */
	fcb[0] = 0xFF;	
	fcb[6] = 0x08;	/* Volume label */
	fcb[7] = toupper(drive) - '@';
	memset(fcb+8, '?', 11);
	err = llc_send_doscall(self, F_SFIRST, 0,0,0,0);
	if (err) return err;
	while (1)
	{
		err  = llc_recv_llpacket(self, recvpkt);
		if (err) return err;

		err = llc_check_success(recvpkt, result);
		if (err == LL_E_OK || err == LL_E_FAIL) return err;
		switch (recvpkt[0])
		{
			case DATAREQ_V3:	/* v3 request data */
				if (recvpkt[2] != FIXED_DX)
					return LL_E_UNEXPECT;
				len = recvpkt[3]; 
				if (len > sizeof(fcb)) len = sizeof(fcb);
				err = llc_send_data(self, FIXED_DX,
						fcb, len);
				if (err) return err;
				break;	

			case DATARET_V2:	/* v2 return data */
			case DATARET_V3:	/* v3 return data */
				if (recvpkt[1] > 1) 
					memcpy(label, recvpkt + 3, recvpkt[1] - 1);
				/* Acknowledge returned data */
				err = llc_send_ack(self, recvpkt[0]);
				if (err) return err;
				break;	
			default:
				return log_unexpected(recvpkt);
		}
	}
}

/* Delete disk label */
ll_error_t llc_dellabel(LL_PREMOTE self, unsigned short *result)
{
	ll_error_t err;

	if (self == NULL || result == NULL) return LL_E_BADPTR;
	err = llc_send_doscall(self, F_DELETE, 0,0,0,0);
	if (err) return err;
	return llc_await_result(self, result);
}


/* Set disk label */
ll_error_t llc_makelabel(LL_PREMOTE self, const char *label, 
		unsigned short *result)
{
	unsigned char recvpkt[260];
	int len;
	ll_error_t err;
	unsigned char fcb[43];

	if (self == NULL || label == NULL || result == NULL) return LL_E_BADPTR;

	/* Convert the passed filename to FCB form */
	memset(fcb, 0, sizeof(fcb));
	fcb[0] = 0xFF;	/* Extended FCB */
	fcb[6] = 0x08;	/* Volume label */
	err = mkfcb(label, fcb + 8);
	if (err) return err;

	err = llc_send_doscall(self, F_MAKE, 0,0,0,0);
	if (err) return err;

	while (1)
	{
		err  = llc_recv_llpacket(self, recvpkt);
		if (err) return err;

		err = llc_check_success(recvpkt, result);
		if (err == LL_E_OK || err == LL_E_FAIL) return err;

		switch (recvpkt[0])
		{
			case DATAREQ_V2:	/* v2 request data */
			case DATAREQ_V3:	/* v3 request data */
				if (recvpkt[2] != FIXED_DX)
					return LL_E_UNEXPECT;
				len = recvpkt[3]; 
				if (len > sizeof(fcb)) len = sizeof(fcb);
				err = llc_send_data(self, FIXED_DX,
						fcb, len);
				if (err) return err;
				break;	
			default:
				return log_unexpected(recvpkt);
		}
	}

	return LL_E_OK;
}

/* Set disk label */
ll_error_t llc_renlabel(LL_PREMOTE self, const char *label, const char *newname,
		unsigned short *result)
{
	unsigned char recvpkt[260];
	int len;
	ll_error_t err;
	unsigned char fcb[43];

	if (self == NULL || label == NULL || 
	    newname == NULL || result == NULL) return LL_E_BADPTR;

	/* Convert the passed filename to FCB form */
	memset(fcb, 0, sizeof(fcb));
	fcb[0] = 0xFF;	/* Extended FCB */
	fcb[6] = 0x08;	/* Volume label */
	err = mkfcb(label, fcb + 8);
	if (err) return err;
	err = mkfcb(newname, fcb + 24);
	if (err) return err;

	err = llc_send_doscall(self, F_RENAME, 0,0,0,0);
	if (err) return err;

	while (1)
	{
		err  = llc_recv_llpacket(self, recvpkt);
		if (err) return err;

		err = llc_check_success(recvpkt, result);
		if (err == LL_E_OK || err == LL_E_FAIL) return err;

		switch (recvpkt[0])
		{
			case DATAREQ_V2:	/* v2 request data */
			case DATAREQ_V3:	/* v3 request data */
				if (recvpkt[2] != FIXED_DX)
					return LL_E_UNEXPECT;
				len = recvpkt[3]; 
				if (len > sizeof(fcb)) len = sizeof(fcb);
				err = llc_send_data(self, FIXED_DX,
						fcb, len);
				if (err) return err;
				break;	
			default:
				return log_unexpected(recvpkt);
		}
	}

	return LL_E_OK;
}


ll_error_t llc_getfreespace(LL_PREMOTE self, unsigned char drive,
				unsigned short *ax, unsigned short *bx,
				unsigned short *cx, unsigned short *dx)
{
	unsigned char recvpkt[260];
	ll_error_t err;

	if (self == NULL) return LL_E_BADPTR;
	err = llc_send_doscall(self, DOS_GETFREE, 0,0,0,drive);

	err  = llc_recv_llpacket(self, recvpkt);
	if (err) return err;

	/* Unusually, the result packet here may be longer than just AX. So
	 * the usual llc_check_success() won't cut it. */
	switch (recvpkt[0])
	{
		case SUCCESS_V2:	/* v2 success */
		case SUCCESS_V3:	/* v3 success */
			if (ax) *ax = peek2(recvpkt + 2);
			if (bx) *bx = peek2(recvpkt + 4);
			if (cx) *cx = peek2(recvpkt + 6);
			if (dx) *dx = peek2(recvpkt + 8);
			return LL_E_OK;
		case ERROR_V2:	/* v2 error */
		case ERROR_V3:	/* v3 error */
			if (ax) *ax = peek2(recvpkt + 2);
			return LL_E_FAIL;
	}
	return LL_E_UNEXPECT;
}



/* Get current directory */
ll_error_t llc_getcwd(LL_PREMOTE self,
		unsigned char drive, char *cwd, unsigned short *result)
{
	unsigned char recvpkt[260];
	int len;
	ll_error_t err;

	if (self == NULL || cwd == NULL || result == NULL) return LL_E_BADPTR;
	err = llc_send_doscall(self, DOS_GETCWD, 0,0,0,drive);

	while (1)
	{
		err  = llc_recv_llpacket(self, recvpkt);
		if (err) return err;

		err = llc_check_success(recvpkt, result);
		if (err == LL_E_OK || err == LL_E_FAIL) return err;
		switch (recvpkt[0])
		{
			case DATARET_V2:	/* v2 return data */
			case DATARET_V3:	/* v3 return data */
				len = recvpkt[1];
				if (len > 1)
				{
					memcpy(cwd, recvpkt + 3, len - 1);
					cwd[len - 1] = 0;
				}
				else	cwd[0] = 0;
				/* Acknowledge returned data */
				err = llc_send_ack(self, recvpkt[0]);
				if (err) return err;
				break;	
			default:
				return log_unexpected(recvpkt);
		}
	}

	return LL_E_OK;
}


/* Find first file */
static ll_error_t llc_xxdir(LL_PREMOTE self, unsigned function, const char *path, unsigned short *result)
{
	unsigned char recvpkt[260];
	ll_error_t err;

	if (self == NULL || path == NULL || result == NULL) return LL_E_BADPTR;

	err = llc_send_doscall(self, function, 0,0,0,0);
	if (err) return err;

	while (1)
	{
		err  = llc_recv_llpacket(self, recvpkt);
		if (err) return err;

		err = llc_check_success(recvpkt, result);
		if (err == LL_E_OK || err == LL_E_FAIL) return err;

		switch (recvpkt[0])
		{
			case DATAREQ_V2:	/* v2 request data */
			case DATAREQ_V3:	/* v3 request data */
				if (recvpkt[2] != ASCIIZ_DX)
					return LL_E_UNEXPECT;
				err = llc_send_asciiz(self, 
						recvpkt[2], recvpkt[3], path);
				if (err) return err;
				break;	
			default:
				return log_unexpected(recvpkt);
		}
	}

	return LL_E_OK;
}


ll_error_t llc_mkdir(LL_PREMOTE self, const char *path, unsigned short *result)
{
	if (self->protocol == LOCOLINK_V1)
	{
		ll_error_t err;
		unsigned char sendpkt[260];
		int len = strlen(path);

		if (len > 255) len = 255;
		memset(sendpkt, 0, sizeof(sendpkt));
		sendpkt[0] = MKDIR_V1;
		sendpkt[1] = len;
		strncpy((char *)sendpkt + 2, path, 255);
		err = llc_send_llpacket(self, sendpkt);
		if (err) return err;
		return llc_await_result(self, result);
	}
	return llc_xxdir(self, DOS_MKDIR, path, result);
}

ll_error_t llc_rmdir(LL_PREMOTE self, const char *path, unsigned short *result)
{
	return llc_xxdir(self, DOS_RMDIR, path, result);
}

ll_error_t llc_chdir(LL_PREMOTE self, const char *path, unsigned short *result)
{
	return llc_xxdir(self, DOS_CHDIR, path, result);
}

ll_error_t llc_unlink(LL_PREMOTE self, const char *path, unsigned short *result)
{
	return llc_xxdir(self, DOS_UNLINK, path, result);
}


ll_error_t llc_check_space(LL_PREMOTE self, unsigned filesize, const char *filename, unsigned short *result)
{
	unsigned char sendpkt[260];
	ll_error_t err;
	int len;

	if (self == NULL || filename == NULL || result == NULL) 
		return LL_E_BADPTR;

	if (self->protocol != LOCOLINK_V1) 
		return LL_E_BADPROTO;
	
	len = strlen(filename);
	if (len > 251) len = 251;

	sendpkt[0] = CKFREE_V1;
	sendpkt[1] = 4 + len;
	sendpkt[2] = filesize & 0xFF;		
	sendpkt[3] = (filesize >> 8) & 0xFF;		
	sendpkt[4] = (filesize >> 16) & 0xFF;		
	sendpkt[5] = (filesize >> 24) & 0xFF;		
	strncpy((char *)sendpkt + 6, filename, 251);
	err = llc_send_llpacket(self, sendpkt);
	if (err) return err;
	return llc_await_result(self, result);
}



/* Create a file */
ll_error_t llc_creat(LL_PREMOTE self, unsigned filesize, unsigned short attr, 
		const char *path, unsigned short *result)
{
	unsigned char sendpkt[260];
	unsigned char recvpkt[260];
	ll_error_t err;

	if (self == NULL || path == NULL || result == NULL) return LL_E_BADPTR;

	if (self->protocol == LOCOLINK_V1)
	{
		int len = strlen(path);
		if (len > 251) len = 251;

		sendpkt[0] = CREAT_V1;
		sendpkt[1] = 4 + len;
		sendpkt[2] = filesize & 0xFF;		
		sendpkt[3] = (filesize >> 8) & 0xFF;		
		sendpkt[4] = (filesize >> 16) & 0xFF;		
		sendpkt[5] = (filesize >> 24) & 0xFF;		
		self->wcrc = 0xFFFF;
		strncpy((char *)sendpkt + 6, path, len);
		err = llc_send_llpacket(self, sendpkt);
	}
	else
	{
		err = llc_send_doscall(self, DOS_CREAT,0,0,attr,0);
	}
	if (err) return err;

	while (1)
	{
		err  = llc_recv_llpacket(self, recvpkt);
		if (err) return err;

		err = llc_check_success(recvpkt, result);
		if (err == LL_E_OK || err == LL_E_FAIL) return err;

		switch (recvpkt[0])
		{
			case DATAREQ_V2:	/* v2 request data */
			case DATAREQ_V3:	/* v3 request data */
				if (recvpkt[2] != ASCIIZ_DX)
					return LL_E_UNEXPECT;
				err = llc_send_asciiz(self, 
						recvpkt[2], recvpkt[3], path);
				if (err) return err;
				break;	
			default:
				return log_unexpected(recvpkt);
		}
	}

	return LL_E_OK;
}


/* Open a file */
ll_error_t llc_openfile(LL_PREMOTE self, const char *path, unsigned char mode,
			unsigned short *result)
{
	unsigned char recvpkt[260];
	ll_error_t err;

	if (self == NULL || path == NULL || result == NULL) return LL_E_BADPTR;

	err = llc_send_doscall(self, DOS_OPEN, mode, 0, 0, 0);

	if (err) return err;

	while (1)
	{
		err  = llc_recv_llpacket(self, recvpkt);
		if (err) return err;

		err = llc_check_success(recvpkt, result);
		if (err == LL_E_OK || err == LL_E_FAIL) return err;

		switch (recvpkt[0])
		{
			case DATAREQ_V2:	/* v2 request data */
			case DATAREQ_V3:	/* v3 request data */
				if (recvpkt[2] != ASCIIZ_DX)
					return LL_E_UNEXPECT;
				err = llc_send_asciiz(self, 
						recvpkt[2], recvpkt[3], path);
				if (err) return err;
				break;	
			default:
				return log_unexpected(recvpkt);
		}
	}

	return LL_E_OK;
}


ll_error_t llc_closefile(LL_PREMOTE self, unsigned short handle, unsigned short *result)
{
	ll_error_t err;

	if (self == NULL || result == NULL) return LL_E_BADPTR;
	err = llc_send_doscall(self, DOS_CLOSE, 0,handle,0,0);
	if (err) return err;
	return llc_await_result(self, result);
}


/* Read data (v2/v3 protocol) - reads a block of up to 64k-1 bytes */
static ll_error_t llc_read3(LL_PREMOTE self, unsigned short handle, unsigned char *dest, unsigned short blocklen, unsigned short *result)
{
	ll_error_t err;
	int pktlen;
	unsigned char recvpkt[260];

	err = llc_send_doscall(self, DOS_READ, 0,handle,blocklen,0);
	if (err) return err;

	while (1)
	{
		err  = llc_recv_llpacket(self, recvpkt);
		if (err) return err;

		err = llc_check_success(recvpkt, result);
		if (err == LL_E_OK || err == LL_E_FAIL) return err;

		switch (recvpkt[0])
		{
			case DATARET_V2:	/* v2 return data */
			case DATARET_V3:	/* v3 return data */
				if (recvpkt[2] != FIXED_DX && recvpkt[2] != NEXTADDR)
					return LL_E_UNEXPECT;
				pktlen = recvpkt[1] - 1;
				if (pktlen > blocklen) pktlen = blocklen;
				memcpy(dest, recvpkt + 3, pktlen);
				err = llc_send_ack(self, recvpkt[0]);
				if (err) return err;
				blocklen -= pktlen;
				dest += pktlen;
				break;	
			default:
				return log_unexpected(recvpkt);
		}
	}
}


 
ll_error_t llc_read(LL_PREMOTE self, unsigned short handle, void *data, size_t len, unsigned short *result)
{
	ll_error_t err;
	unsigned char *dest = (unsigned char *)data;
	size_t blocklen;

	if (self == NULL || data == NULL || result == NULL) return LL_E_BADPTR;
	
	while (len)
	{
		blocklen = 32768;
		if (blocklen > len) blocklen = len;

		err = llc_read3(self, handle, dest, blocklen, result);
		if (err) return err;

		dest += blocklen;
		len -= blocklen;
	}
	return LL_E_OK;
}







/* Write data (v1 protocol) */
static ll_error_t llc_write1(LL_PREMOTE self, const unsigned char *data, size_t len, unsigned short *result)
{
	unsigned pktlen;
	unsigned char sendpkt[260];
	int n;
	ll_error_t err;

	if (ll_crc_tab[0] == 0 && ll_crc_tab[1] == 0)
	{
		LLCRC_Init(ll_crc_tab);
	}
	while (len != 0)
	{
		pktlen = 255;
		if (pktlen > len) pktlen = len;
		sendpkt[0] = WRITE_V1;
		sendpkt[1] = pktlen;
		memcpy(sendpkt, data, pktlen);
/* Update running checksum */
		LLCRC_ClearTo(self->wcrc);
		for (n = 0; n < pktlen; n++)
		{
			LLCRC_Update(sendpkt[n+2]);
		}
		self->wcrc = ~LLCRC_Done();
		err = llc_send_llpacket(self, sendpkt);
		if (err) return err;
		err = llc_await_result(self, result);
		if (err) return err;
		data += pktlen;
		len -= pktlen;
	}
	return LL_E_OK;
}


/* Write data (v2/v3 protocol) - writes a block of up to 64k-1 bytes */
static ll_error_t llc_write3(LL_PREMOTE self, unsigned short handle, const unsigned char *src, unsigned short blocklen, unsigned short *result)
{
	ll_error_t err;
	int pktlen;
	unsigned char recvpkt[260];

	err = llc_send_doscall(self, DOS_WRITE, 0,handle,blocklen,0);
	if (err) return err;

	while (1)
	{
		err  = llc_recv_llpacket(self, recvpkt);
		if (err) return err;

		err = llc_check_success(recvpkt, result);
		if (err == LL_E_OK || err == LL_E_FAIL) return err;

		switch (recvpkt[0])
		{
			case DATAREQ_V2:	/* v2 request data */
			case DATAREQ_V3:	/* v3 request data */
				if (recvpkt[2] != FIXED_DX && recvpkt[2] != NEXTADDR)
					return LL_E_UNEXPECT;
				pktlen = recvpkt[3];
				if (pktlen > blocklen) pktlen = blocklen;
				err = llc_send_data(self, recvpkt[0], src, pktlen);
				if (err) return err;
				blocklen -= pktlen;
				src += pktlen;
				break;	
			default:
				return log_unexpected(recvpkt);
		}
	}
}


 
ll_error_t llc_write(LL_PREMOTE self, unsigned short handle, const void *data, size_t len, unsigned short *result)
{
	ll_error_t err;
	const unsigned char *src = (const unsigned char *)data;
	size_t blocklen;

	if (self == NULL || data == NULL || result == NULL) return LL_E_BADPTR;
	
	if (self->protocol == LOCOLINK_V1)
	{
		return llc_write1(self, src, len, result);
	}
	while (len)
	{
		blocklen = 32768;
		if (blocklen > len) blocklen = len;

		err = llc_write3(self, handle, src, blocklen, result);
		if (err) return err;

		src += blocklen;
		len -= blocklen;
	}
	return LL_E_OK;


}


ll_error_t llc_lseek(LL_PREMOTE self, unsigned short handle, long offset,
			unsigned char whence, long *result)
{
	unsigned char recvpkt[260];
	ll_error_t err;

	if (self == NULL) return LL_E_BADPTR;
	err = llc_send_doscall(self, DOS_LSEEK, whence, handle, 
					(offset >> 16), (offset & 0xFFFF));

	err  = llc_recv_llpacket(self, recvpkt);
	if (err) return err;

	/* Unusually, the result packet here may be longer than just AX. So
	 * the usual llc_check_success() won't cut it. */
	switch (recvpkt[0])
	{
		case SUCCESS_V2:	/* v2 success */
		case SUCCESS_V3:	/* v3 success */
			*result = peek2(recvpkt + 2) + 
				  0x10000L * peek2(recvpkt + 8);
			return LL_E_OK;
		case ERROR_V2:	/* v2 error */
		case ERROR_V3:	/* v3 error */
			*result = peek2(recvpkt + 2);
			return LL_E_FAIL;
	}
	return LL_E_UNEXPECT;
}


/* Get file attributes */
ll_error_t llc_getattr(LL_PREMOTE self, const char *path, 
			unsigned short *result, unsigned short *attr)
{
	unsigned char recvpkt[260];
	ll_error_t err;

	if (self == NULL || path == NULL || result == NULL || attr == NULL) return LL_E_BADPTR;

	err = llc_send_doscall(self, DOS_CHMOD, 0, 0, 0, 0);

	if (err) return err;

	while (1)
	{
		err  = llc_recv_llpacket(self, recvpkt);
		if (err) return err;

		switch (recvpkt[0])
		{
			case SUCCESS_V2:	/* v2 success */
			case SUCCESS_V3:	/* v3 success */
				if (result) *result = peek2(recvpkt + 2);
				if (attr)   *attr = peek2(recvpkt + 6);
				return LL_E_OK;
			case ERROR_V2:	/* v2 error */
			case ERROR_V3:	/* v3 error */
				if (result) *result = peek2(recvpkt + 2);
				return LL_E_FAIL;
			case DATAREQ_V2:	/* v2 request data */
			case DATAREQ_V3:	/* v3 request data */
				if (recvpkt[2] != ASCIIZ_DX)
					return LL_E_UNEXPECT;
				err = llc_send_asciiz(self, 
						recvpkt[2], recvpkt[3], path);
				if (err) return err;
				break;	
			default:
				return log_unexpected(recvpkt);
		}
	}

	return LL_E_OK;
}


/* Set file attributes */
ll_error_t llc_chmod(LL_PREMOTE self, const char *path, unsigned short attr,
			unsigned short *result)
{
	unsigned char recvpkt[260];
	ll_error_t err;

	if (self == NULL || path == NULL || result == NULL) return LL_E_BADPTR;

	err = llc_send_doscall(self, DOS_CHMOD, 1, 0, attr, 0);

	if (err) return err;

	while (1)
	{
		err  = llc_recv_llpacket(self, recvpkt);
		if (err) return err;
		err = llc_check_success(recvpkt, result);
		if (err == LL_E_OK || err == LL_E_FAIL) return err;

		switch (recvpkt[0])
		{
			case DATAREQ_V2:	/* v2 request data */
			case DATAREQ_V3:	/* v3 request data */
				if (recvpkt[2] != ASCIIZ_DX)
					return LL_E_UNEXPECT;
				err = llc_send_asciiz(self, 
						recvpkt[2], recvpkt[3], path);
				if (err) return err;
				break;	
			default:
				return log_unexpected(recvpkt);
		}
	}

	return LL_E_OK;
}


/* Get device information */
ll_error_t llc_getdevinfo(LL_PREMOTE self, unsigned short handle, unsigned short *result)
{
	ll_error_t err;

	if (self == NULL || result == NULL) return LL_E_BADPTR;
	err = llc_send_doscall(self, DOS_IOCTL, 0,handle,0,0);
	if (err) return err;
	return llc_await_result(self, result);
}


/* Find first file (LocoLink v1) */
static ll_error_t llc_findfirst_v1(LL_PREMOTE self,
		const char *path, unsigned char isfile, unsigned short *result)
{
	unsigned char sendpkt[260];
	size_t len = strlen(path);
	ll_error_t err;

	if (len > 254) len = 254;
	memset(sendpkt, 0, sizeof(sendpkt));
	
	sendpkt[0] = FINDFIRST_V1;
	sendpkt[1] = 1 + len;
	sendpkt[2] = isfile;
	strncpy((char *)sendpkt + 3, path, len);
	err = llc_send_llpacket(self, sendpkt);
	if (err) return err;


	return llc_await_result(self, result);
}



/* Find first file */
ll_error_t llc_findfirst(LL_PREMOTE self,
		const char *path, unsigned short attr, unsigned char *ffblk,
		unsigned short *result)
{
	unsigned char recvpkt[260];
	int len;
	ll_error_t err;

	if (self == NULL || ffblk == NULL || result == NULL) return LL_E_BADPTR;

	if (self->protocol == LOCOLINK_V1)
	{
		return llc_findfirst_v1(self, path, (attr & 0x10) ? 0 : 1, result);
	}
	else if (self->protocol == LOCOLINK_V2 || self->protocol == LOCOLINK_V3)
	{
/* LocoLink v2/v3 passes the search attribute in AL, not CL */
		err = llc_send_doscall(self, DOS_SFIRST, attr,0,0,0);
	}
	if (err) return err;

	while (1)
	{
		err  = llc_recv_llpacket(self, recvpkt);
		if (err) return err;

		err = llc_check_success(recvpkt, result);
		if (err == LL_E_OK || err == LL_E_FAIL) return err;

		switch (recvpkt[0])
		{
			case DATAREQ_V2:	/* v2 request data */
			case DATAREQ_V3:	/* v3 request data */
				if (recvpkt[2] != ASCIIZ_DX)
					return LL_E_UNEXPECT;
				err = llc_send_asciiz(self, 
						recvpkt[2], recvpkt[3], path);
				if (err) return err;
				break;	

			case DATARET_V2:	/* v2 return data */
			case DATARET_V3:	/* v3 return data */
				len = recvpkt[1] - 1;
				memset(ffblk, 0, 0x2b);
				if (len > 0x2B) len = 0x2b;
				if (len > 0) memcpy(ffblk, recvpkt + 3, len);
				/* Acknowledge returned data */
				err = llc_send_ack(self, recvpkt[0]);
				if (err) return err;
				break;	
			default:
				return log_unexpected(recvpkt);
		}
	}

	return LL_E_OK;
}



/* Find next file */
ll_error_t llc_findnext(LL_PREMOTE self,
		unsigned char *ffblk, unsigned short *result)
{
	unsigned char recvpkt[260];
	int len;
	ll_error_t err;

	if (self == NULL || ffblk == NULL || result == NULL) return LL_E_BADPTR;

	err = llc_send_doscall(self, DOS_SNEXT, 0,0,0,0);
	if (err) return err;

	while (1)
	{
		err  = llc_recv_llpacket(self, recvpkt);
		if (err) return err;

/* For some reason this function returns the ffblk in the result packet 
 * rather than as a separate server -> client transfer.
 */
		if (self->protocol == LOCOLINK_V2 && recvpkt[0] == SUCCESS_V2)
		{
			*result = peek2(recvpkt + 2);
			memset(ffblk + 0x14, 0, 23);
			memcpy(ffblk + 0x14, recvpkt + 4, recvpkt[1] - 2);
			return LL_E_OK;
		}
		if (self->protocol == LOCOLINK_V3 && recvpkt[0] == SUCCESS_V3)
		{
			*result = peek2(recvpkt + 2);
			memset(ffblk + 0x13, 0, 24);
			memcpy(ffblk + 0x13, recvpkt + 4, recvpkt[1] - 2);
		}
		err = llc_check_success(recvpkt, result);
		if (err == LL_E_OK || err == LL_E_FAIL) return err;

		switch (recvpkt[0])
		{
			case DATARET_V2:	/* v2 return data */
			case DATARET_V3:	/* v3 return data */
				len = recvpkt[1] - 1;
				if (len > 0x2C) len = 0x2c;
				if (len > 0) memcpy(ffblk, recvpkt + 3, len);
				memcpy(ffblk, recvpkt + 3, len);
				/* Acknowledge returned data */
				err = llc_send_ack(self, recvpkt[0]);
				if (err) return err;
				break;	
			default:
				return log_unexpected(recvpkt);
		}
	}

	return LL_E_OK;
}




/* Rename a file */
ll_error_t llc_rename(LL_PREMOTE self, const char *from, const char *to, unsigned short *result)
{
	unsigned char recvpkt[260];
	ll_error_t err;

	if (self == NULL || from == NULL || to == NULL || result == NULL) return LL_E_BADPTR;

	err = llc_send_doscall(self, DOS_RENAME, 0,0,0,0);
	if (err) return err;

	while (1)
	{
		err  = llc_recv_llpacket(self, recvpkt);
		if (err) return err;

		err = llc_check_success(recvpkt, result);
		if (err == LL_E_OK || err == LL_E_FAIL) return err;

		switch (recvpkt[0])
		{
			case DATAREQ_V2:	/* v2 request data */
			case DATAREQ_V3:	/* v3 request data */
				if (recvpkt[2] == ASCIIZ_DX)
				{
					err = llc_send_asciiz(self, 
						recvpkt[2], recvpkt[3], from);
				}
				else if (recvpkt[2] == ASCIIZ_DI)
				{
					err = llc_send_asciiz(self, 
						recvpkt[2], recvpkt[3], to);
				}
				else	err = LL_E_UNEXPECT;
				if (err) return err;
				break;	
			default:
				return log_unexpected(recvpkt);
		}
	}
	return LL_E_OK;
}



/* Get timestamp */
ll_error_t llc_getfiletime(LL_PREMOTE self, unsigned short handle, unsigned short *result, unsigned short *tm, unsigned short *dt)
{
	ll_error_t err;
	unsigned char recvpkt[260];

	if (self == NULL || result == NULL) return LL_E_BADPTR;
	err = llc_send_doscall(self, DOS_FILETIME, 0,handle,0,0);
	if (err) return err;
	err  = llc_recv_llpacket(self, recvpkt);
	switch (recvpkt[0])
	{
		case SUCCESS_V2:	/* v2 success */
		case SUCCESS_V3:	/* v3 success */
			if (result) *result = peek2(recvpkt + 2);
			if (tm) *tm = peek2(recvpkt + 6);
			if (dt) *dt = peek2(recvpkt + 8);
			return LL_E_OK;
		case ERROR_V2:	/* v2 error */
		case ERROR_V3:	/* v3 error */
			if (result) *result = peek2(recvpkt + 2);
			return LL_E_FAIL;
	}
	return LL_E_UNEXPECT;
}

ll_error_t llc_setfiletime(LL_PREMOTE self, unsigned short handle, 
		unsigned short tm, unsigned short dt, unsigned short *result)
{
	ll_error_t err;

	if (self == NULL || result == NULL) return LL_E_BADPTR;
	err = llc_send_doscall(self, DOS_FILETIME, 1,handle,tm,dt);
	if (err) return err;
	return llc_await_result(self, result);
}


/* Create a file if it does not exist*/
ll_error_t llc_ccreat(LL_PREMOTE self, unsigned short attr, 
		const char *path, unsigned short *result)
{
	unsigned char recvpkt[260];
	ll_error_t err;

	if (self == NULL || path == NULL || result == NULL) return LL_E_BADPTR;

	err = llc_send_doscall(self, DOS_CCREAT,0,0,attr,0);

	if (err) return err;

	while (1)
	{
		err  = llc_recv_llpacket(self, recvpkt);
		if (err) return err;

		err = llc_check_success(recvpkt, result);
		if (err == LL_E_OK || err == LL_E_FAIL) return err;

		switch (recvpkt[0])
		{
			case DATAREQ_V2:	/* v2 request data */
			case DATAREQ_V3:	/* v3 request data */
				if (recvpkt[2] != ASCIIZ_DX)
					return LL_E_UNEXPECT;
				err = llc_send_asciiz(self, 
						recvpkt[2], recvpkt[3], path);
				if (err) return err;
				break;	
			default:
				return log_unexpected(recvpkt);
		}
	}

	return LL_E_OK;
}


/* Set callback handler for pings */
ll_error_t llc_set_ping_callback(LL_PREMOTE premote, LL_PING_CALLBACK cbk)
{
	if (!premote) return LL_E_BADPTR;
	premote->ping_callback = cbk;
	return LL_E_OK;
}

/* Get callback handler for pings */
ll_error_t llc_get_ping_callback(LL_PREMOTE premote, LL_PING_CALLBACK *cbk)
{
	if (!premote || !cbk) return LL_E_BADPTR;
	*cbk = premote->ping_callback;
	return LL_E_OK;
}

