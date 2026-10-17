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
#include "llparlink.h"
#include "crc16.h"

unsigned char ll_crc_tab[512];

/* LocoLink packet parser / constructor 
 *
 * Give it a stream of LocoLink I/O reads/writes, and it will assemble 
 * packets, pass them to a callback function, and convert the results back
 * to I/O reads/writes.
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
#define SHOWCOOKIE "%p"		// If I know the cookies are strings I can 
				// change this to "%s" 
*/
#define LOG(x)


/* The default callback just knows enough to say Hello and Goodbye, and 
 * logs everything to standard error */

static ll_error_t default_callback(void *cookie, unsigned char *input, unsigned char *output)
{
	unsigned pkt_len = 4 + input[1];
	int n;
	LL_PARLINK *self = (LL_PARLINK *)cookie;

	LOG(("   Packet received "));
	for (n = 0; n < pkt_len; n++)
	{
		LOG(("%02x ", input[n]));
	}
	LOG(("\n"));

/* v1 "start" packet */
	if (input[0] == CONNECT_V1 && input[1] == 2 && 
	    input[2] == 0x43 && input[3] == 0x31)
	{
		output[0] = RESULT_V1;	/* Acknowledge */
		output[1] = 0x01;	/* 1 byte */
		output[2] = input[0];
		self->protocol = LOCOLINK_V1;	

		return LL_E_OK;
	}
	if (self->protocol == LOCOLINK_V1 && input[0] == HANGUP_V1)
	{
		LOG(("v1 Hangup\n"));
		self->protocol = NO_PROTOCOL;
		return LL_E_CLOSED;
	}
	if (self->protocol == LOCOLINK_V2 && input[0] == HANGUP_V2)
	{
		LOG(("v2 Hangup\n"));
		self->protocol = NO_PROTOCOL;
		return LL_E_CLOSED;
	}
	if (self->protocol == LOCOLINK_V3 && input[0] == HANGUP_V3)
	{
		LOG(("v3 Hangup\n"));
		self->protocol = NO_PROTOCOL;
		return LL_E_CLOSED;
	}
	LOG(("Unknown packet\n"));
	switch (self->protocol)
	{
		case LOCOLINK_V1:
			output[0] = 0x62;	/* v1: Return error packet */
			output[1] = 0;
			return LL_E_OK;	
		case LOCOLINK_V2:
			output[0] = 0x67;	/* v2: Return error packet */
			output[1] = 2;
			output[2] = 1;		/* Invalid function number */
			output[3] = 0;
			return LL_E_OK;	
		case LOCOLINK_V3:
			output[0] = 0xA6;	/* v3: Return error packet */
			output[1] = 2;
			output[2] = 1;		/* Invalid function number */
			output[3] = 0;
		default: 
			return LL_E_UNEXPECT;	/* Don't know what to do with this packet */
	}
}



/* Create a LocoLink packet parser */
ll_error_t ll_parlink_create(LL_PPLINK *pself, LL_PCALLBACK pcallback, void *cookie)
{
	LL_PARLINK *p;

	if (!pself) return LL_E_BADPTR;

	p = malloc(sizeof(LL_PARLINK));
	if (!p) return LL_E_NOMEM;

	memset(p, 0, sizeof(LL_PARLINK));

	ll_parlink_reset(p);
	*pself = p;
	p->callback = pcallback;
	p->cookie = cookie;
	if (p->callback == NULL)
	{
		p->callback = default_callback;
		p->cookie = p;
	}
	return LL_E_OK;
}

/* Shut down LocoLink packet parser */
ll_error_t ll_parlink_close(LL_PPLINK *pself)
{
	if (pself == NULL || *pself == NULL) return LL_E_BADPTR;
	free(*pself);
	*pself = NULL;
	return LL_E_OK;
}

/* Reset link */
ll_error_t ll_parlink_reset(LL_PPLINK pself)
{
	if (pself == NULL) return LL_E_BADPTR;

	LOG((SHOWCOOKIE " ll_parlink_reset\n", pself->cookie));

	pself->state = RIDLE;
	pself->sent = 2;
	pself->rdptr = 0;
	pself->wrptr = 0;
	return LL_E_OK;
}

static void storebit(LL_PPLINK pself, unsigned char bit)
{
	unsigned short crc, pkt_crc;
	unsigned char mask = 0x80 >> (pself->wrptr & 7);
	unsigned offset = (pself->wrptr / 8);
	unsigned pkt_len = 0xffff;
	ll_error_t err;
	int n;
	
	/* Overflow */
	if (offset > sizeof(pself->recv_packet)) 
	{
		LOG(("offset=%d overflow\n", offset));
		return;
	}
	if (bit) pself->recv_packet[offset] |= mask;	
	else     pself->recv_packet[offset] &= ~mask;	

	++pself->wrptr;
	if ((pself->wrptr & 7) == 0)
	{	
		LOG((SHOWCOOKIE " Byte received %02x offset=%d wrptr=%d\n", 
			pself->cookie, 
			pself->recv_packet[offset], offset, pself->wrptr));
		/* Complete packet read? */
		if (pself->wrptr >= 16)
		{	
			pkt_len = pself->recv_packet[1] + 4;

			if (pself->wrptr == 8 * pkt_len)
			{
/* Packet complete. Check the CRC. */
				crc = ll_parlink_crc(pself->recv_packet);
				pkt_crc = ((pself->recv_packet[pkt_len - 1]) << 8)
					  | pself->recv_packet[pkt_len - 2];

				if (crc != pkt_crc)
				{
					LOG((SHOWCOOKIE " CRC error: %04x vs %04x\n", pself->cookie, crc, pkt_crc));
					err = LL_E_CRC;	
				}
				else
				{
					err = (*pself->callback)(pself->cookie, pself->recv_packet, pself->send_packet);
					if (err == LL_E_CLOSED)
					{
						pself->state = HANGUP1;
						pself->sent  = 1;
						pself->rdptr = 0;
						LOG (("  storebit -> HANGUP1\n"));
						return;
					}
				}
				if (!err)
				{
					crc = ll_parlink_crc(pself->send_packet);
					pself->send_packet[2 + pself->send_packet[1]] = crc & 0xFF;
					pself->send_packet[3 + pself->send_packet[1]] = crc >> 8;
					pself->state = READ4;			
					pself->sent  = 1;
					pself->rdptr = 0;
		LOG(("   Packet returned "));
		for (n = 0; n < 4 + pself->send_packet[1]; n++)
		{
			LOG(("%02x ", pself->send_packet[n]));
		}
		LOG(("\n"));

				}
				else
				{
					ll_parlink_reset(pself);
				}
			}
		}
	}
}

static unsigned char fetchbit(LL_PPLINK pself)
{
	unsigned char mask = 0x80 >> (pself->rdptr & 7);
	unsigned offset = (pself->rdptr / 8);
	
	/* Overflow */
	if (offset > sizeof(pself->send_packet)) return 0;

	LOG(("fetchbit rdptr=%d offset=%d mask=0x%02x val=%02x bit=%d\n",
		pself->rdptr, offset, mask, 
		pself->send_packet[offset],
		(pself->send_packet[offset] & mask) ? 1 : 0));
	++pself->rdptr;
	return (pself->send_packet[offset] & mask) ? 1 : 0;
}





/* Host writes a value to the server */
ll_error_t ll_parlink_write(LL_PPLINK pself, unsigned char value)
{
	if (pself == NULL) return LL_E_BADPTR;

	LOG((SHOWCOOKIE " LLK write: %d -> %d\n", pself->cookie, pself->recvd, value & 3));

	pself->recvd = value & 3;

	switch (pself->state)
	{
		case RIDLE:
/* Receiving a packet: Wait until received value is 0, send 3. */
			switch(pself->recvd)
			{
				case 0: 
					pself->wrptr = 0;
					pself->state = READ1;
					pself->sent = 3;
					LOG((SHOWCOOKIE "  RIDLE -> READ1\n", pself->cookie));
					break;	
/* Anything else -- stay idle */
			}
			break;
		case READ1:
			switch(pself->recvd)
			{
				case 0:	break;	/* Ignore */
				case 1: /* Start of packet */
					pself->state = READ2;
					pself->sent = 1;
					LOG((SHOWCOOKIE "  READ1 -> READ2\n", pself->cookie));
					break;	
/* Anything else is an error */
				default:LOG((SHOWCOOKIE "  READ1 -> RIDLE\n", pself->cookie));
					ll_parlink_reset(pself);
					break;
			}
			break;
		case READ2:
			switch(pself->recvd)
			{
				case 0: 	 /* Previous bit: Ignore */
				case 1: break;

				case 2:
				case 3: pself->state = READ3;
					pself->sent = 3;
					/* storebit may change state & sent */
					storebit(pself, pself->recvd & 1);
					LOG((SHOWCOOKIE "  READ2 -> READ3\n", pself->cookie));
					break;
			}
			break;
		case READ3:
			switch(pself->recvd)
			{
				case 0:
				case 1: pself->state = READ2;
					pself->sent = 1;
					LOG((SHOWCOOKIE "  READ3 -> READ2\n", pself->cookie));
					/* storebit may change state & sent */
					storebit(pself, pself->recvd & 1);
					break;

				case 2:
				case 3: break; /* Previous bit: ignore */
			}
			break;
		case READ4:
			switch(pself->recvd)
			{
				case 0:
				case 1: break;	/* Previous bit: ignore */
				case 2: LOG((SHOWCOOKIE "  READ4 -> RIDLE\n", pself->cookie));
					ll_parlink_reset(pself);
					break;
				case 3: pself->sent = 3;
					pself->state = SIDLE;
					LOG((SHOWCOOKIE "  READ4 -> SIDLE\n", pself->cookie));
					break;
			}
			break;

		case SIDLE:
			switch (pself->recvd)
			{
				case 0:
				case 1: LOG((SHOWCOOKIE "  SIDLE -> RIDLE\n", pself->cookie));
					ll_parlink_reset(pself);
					break;
				case 2: LOG((SHOWCOOKIE "  SIDLE -> SEND1\n", pself->cookie));
					pself->rdptr = 0;
					pself->sent = 0;
					pself->state = SEND1;
					break;
				case 3: pself->sent = 3;
					break;
			}
			break;
		case SEND1:
			switch (pself->recvd)
			{
				case 0:
				case 1: LOG((SHOWCOOKIE "  SEND1 -> RIDLE\n", pself->cookie));
					ll_parlink_reset(pself);
					break;
				case 2: break;
				case 3: LOG((SHOWCOOKIE "  SEND1 -> SEND2\n", pself->cookie));
					pself->sent = 1;
					pself->state = SEND2;
					break;
			}
			break;
		case SEND2:
			switch (pself->recvd)
			{
				case 1:	
					if (8 * (4 + pself->send_packet[1]) == pself->rdptr)
					{
						LOG((SHOWCOOKIE " All packet sent  SEND2 -> SEND4 send_packet[1]=0x%02x len=%d rdptr=%d\n", pself->cookie,
pself->send_packet[1], 8 * (4 + pself->send_packet[1]), pself->rdptr));
						pself->state = SEND4;
						pself->sent = 3;
					}
					else
					{
						pself->state = SEND3;
						pself->sent = fetchbit(pself) ? 3 : 2;
						LOG((SHOWCOOKIE "  SEND2 -> SEND3 %d\n", pself->cookie, pself->sent));
					}
					break;
				case 0:	
				case 2: LOG((SHOWCOOKIE "  SEND2 -> RIDLE\n", pself->cookie));
					ll_parlink_reset(pself);
					break;
				case 3: break;
			}
			break;
		case SEND3:
			switch (pself->recvd)
			{
				case 3:	pself->state = SEND2;
					pself->sent = fetchbit(pself) ? 1 : 0;
					LOG((SHOWCOOKIE "  SEND3 -> SEND2 %d\n", pself->cookie, pself->sent));
					break;
				case 1: break;
				case 0:	
				case 2: LOG((SHOWCOOKIE "  SEND3 -> RIDLE\n", pself->cookie));
					ll_parlink_reset(pself);
					break;
			}
			break;
		case SEND4:
			switch (pself->recvd)
			{
				case 2:
				case 0:	LOG((SHOWCOOKIE "  SEND4 -> RIDLE err\n", pself->cookie));
					ll_parlink_reset(pself);
					break;
				case 1: break;
				case 3: LOG((SHOWCOOKIE "  SEND4 -> RIDLE ok\n", pself->cookie));
					pself->sent = 2;
					pself->state = RIDLE;
					break;
			}
			break;

		case HANGUP1:
			switch(pself->recvd)
			{
				case 0:
				case 1: break;	/* Previous bit: ignore */
				case 2: LOG (("  HANGUP1 -> RIDLE\n"));
					ll_parlink_reset(pself);
					break;
				case 3:	LOG (("  HANGUP1 -> HANGUP2\n"));
					pself->sent =3;
					pself->state = HANGUP2;
					break;
			}
			break;
		case HANGUP2:
			switch(pself->recvd)
			{
				case 0:
				case 1: LOG (("  HANGUP2 -> RIDLE err\n"));
					ll_parlink_reset(pself);
					break;
				case 2: LOG (("  HANGUP2 -> RIDLE ok\n"));
					pself->sent = 2; /* Listening */
					pself->state = RIDLE;
					break;
				case 3:	break;	/* Previous bit: ignore */
			}
	}

	return LL_E_OK;
}

/* Host reads a value from the server */
ll_error_t ll_parlink_read(LL_PPLINK pself, unsigned char *value)
{
	if (pself == NULL || value == NULL) return LL_E_BADPTR;

	LOG(("LLK read:     %d\n", pself->sent));
	*value = pself->sent;
	return LL_E_OK;
}



unsigned short ll_parlink_crc(unsigned char *packet)
{	
	unsigned n;

	if (ll_crc_tab[0] == 0 && ll_crc_tab[1] == 0)
	{
		LLCRC_Init(ll_crc_tab);
	}
	LLCRC_ClearTo(0xFFFF);
	for (n = 0; n < 2 + packet[1]; n++)
	{
		LLCRC_Update(packet[n]);
	}
	return ~LLCRC_Done();
}



