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
#include "crc16.h"

/*
static void logfunc(const char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	vfprintf(stderr, s, ap);
	va_end(ap);
}
#define LOG(x) logfunc x
*/
#define LOG(x)

/* Given that the link is in state "o1" or "o2", wait for it to enter
 * state "n1" or "n2"
 *
 * This busy-waits; to try and minimise the problem it calls usleep() every
 * so often.
 *
 * If "o1" is -1, the link can be in any state. */

static int ll_expect2(LL_PDEV link, LL_END end, char o1, char o2, char n1, char n2, unsigned char *out)
{
	unsigned char c;
	int e;
	int first = 1;

	LOG(("ll_expect2 o1=%d o2=%d n1=%d n2=%d\n", o1, o2, n1, n2));
	do	
	{
/* First time in, don't wait for a change, just go with what is there */
		if (first)
		{
			e = ll_lrecv_poll(link, end, &c);
			if (e) return e;	/* Failed? */
			first = 0;
		}
		else
		{
/* Subsequent times, wait for a change */
			e = ll_lrecv_wait(link, end, &c);
		}
		if (e) return e;	/* Failed? */

		/* Got one of the new values. Return. */
		if (c == n1 || c == n2) 
		{
			*out = c;
			return LL_E_OK;
		}
		/* Still on the old values. Continue. */
		if (o1 >= 0 && c == o1) continue;
		if (o2 >= 0 && c == o2) continue;
		if (o1 <= 0) continue;
		/* Got a value that's neither new or old. */

LOG(("-- UNEXPECTED-- Old values: %d %d  Wanted: %d %d  Got: %d\n", o1, o2, n1, n2, c));
		return LL_E_UNEXPECT;
	}
	while (1);
	return LL_E_UNKNOWN;	// Can't happen
}

/* Given that the link is in state "old", wait for it to enter
 * state "n1". */

static int ll_expect(LL_PDEV link, LL_END end, char old, char n1)
{
	unsigned char o;
	return ll_expect2(link, end, old, old, n1, n1, &o);
}

/* Write a byte to the link. */
static int ll_wbyte(LL_PDEV link, LL_END end, unsigned char b)
{
	int n, e;
	unsigned char mask = 0x80;

	for (n = 0; n < 4; n++)
	{
LOG(("WBYTE tick sending %d\n", (b & mask) ? 3 : 2));
		e = ll_expect(link, end, 3,1); if (e) return e;
		e = ll_lsend (link, end, (b & mask) ? 3 : 2);
		mask = mask >> 1;
LOG(("WBYTE tock sending %d\n", (b & mask) ? 1 : 0));
		e = ll_expect(link, end, 1,3); if (e) return e;
		e = ll_lsend (link, end, (b & mask) ? 1 : 0);
		mask = mask >> 1;
	}	
	return LL_E_OK;
}



/* Read a byte from the link. */
int ll_rbyte(LL_PDEV link, LL_END end, unsigned char *b)
{
	int n, e;
	unsigned char mask = 0x80;
	unsigned char data;

	LOG(("Starting to receive byte\n"));
	*b = 0;
	for (n = 0; n < 4; n++)
	{
LOG(("RBYTE tick expecting {0,1} -> {2,3}\n"));
		e = ll_expect2(link, end, 0, 1 ,2, 3, &data); if (e) return e;
LOG(("RBYTE tick sending 3\n"));
		e = ll_lsend  (link, end, 3);
		if (data & 1) *b |= mask;
		mask = mask >> 1;
LOG(("RBYTE tock expecting {2,3} -> {0,1}\n"));
		e = ll_expect2(link, end, 2, 3, 0, 1, &data); if (e) return e;
LOG(("RBYTE tock sending 1\n"));
		e = ll_lsend  (link, end, 1);
		if (data & 1) *b |= mask;
		mask = mask >> 1;
	}
	LOG(("Byte received\n"));
	return LL_E_OK;
}


/* Send a packet down the link. */
int ll_send_llpacket(LL_PDEV link, LL_END end, const unsigned char *pkt)
{
	unsigned char packet[260];
	int e;
	int n;
	unsigned short sum;
	unsigned char v;

	LOG(("Starting to send packet\n"));
	if (pkt == NULL) return LL_E_BADPTR;
	memcpy(packet, pkt, 2 + pkt[1]);

	/* Generate the packet CRC */
	sum = ll_parlink_crc(packet);
	packet[2 + packet[1]] = sum & 0xFF;
	packet[3 + packet[1]] = (sum >> 8) & 0xFF;

LOG(("Packet to send: "));
for (n = 0; n < 4 + pkt[1]; n++)
{
	LOG(("%02x ", pkt[n]));
}
LOG(("\n"));


	/* Now try to send it. Wait for the input to become 2 (ready to 
	 * receive) or 3 (idle). */
	LOG(("Waiting for other end...\n"));
	e = ll_expect2(link, end, -1, -1, 2, 3, &v); if (e) return e;
	LOG(("... result=%d\n", v));
	if (v == 3)	/* Other end is idle. Send 3 to let it know we're here. */
	{
		LOG(("Sending 3\n"));
		e = ll_lsend(link, end, 3); if (e) return e;
	}

	/* Signal that a packet's starting */
	LOG(("Start of packet. Waiting for '2'\n"));
	e = ll_expect(link, end, -1, 2);	if (e) return e;
	LOG(("SOP: Sending '0'\n"));
	e = ll_lsend (link, end, 0    );	if (e) return e;
	LOG(("Start of packet. Waiting for '3'\n"));
	e = ll_expect(link, end, 2,  3);	if (e) return e;
	LOG(("SOP: Sending '1'\n"));
	e = ll_lsend (link, end, 1    );	if (e) return e;

	LOG(("Packet inited. Writing data.\n"));
	/* Write the data */
	for (n = 0; n < 4 + packet[1]; n++)
	{
		e = ll_wbyte (link, end, packet[n]);     if (e) return e;
	}
	/* Signal end of packet */
	LOG(("Sending end of packet.\n"));
	e = ll_expect(link, end, 3, 1);  if (e) return e;	
	e = ll_lsend (link, end, 3    );	if (e) return e;
	e = ll_expect(link, end, 1, 3);  if (e) return e;	
	e = ll_lsend (link, end, 2    );	if (e) return e;
	LOG(("Packet sent.\n"));
	return LL_E_OK;
}

/* Receive a packet from the other machine */
int ll_recv_llpacket(LL_PDEV link, LL_END end, unsigned char *pkt)
{
	unsigned char recvbuf[260];
	int e;
	int n;
	unsigned short sum, psum;

	if (pkt == NULL) return LL_E_BADPTR;

	LOG(("Starting to receive packet\n"));
	/* Receive packet prelude */
	e = ll_expect(link, end, 3,  0);	if (e) return e;
	LOG(("Receive SOP: Sending '3'\n"));
	e = ll_lsend (link, end, 3    );	if (e) return e;
	e = ll_expect(link, end, 0,  1);	if (e) return e;
	LOG(("Receive SOP: Sending '1'\n"));
	e = ll_lsend (link, end, 1    );	if (e) return e;

	/* Read first byte, length */
	e = ll_rbyte (link, end, &recvbuf[0]);    if (e) return e;
	e = ll_rbyte (link, end, &recvbuf[1]);    if (e) return e;
	/* Read other bytes */
	for (n = 0; n < 2 + recvbuf[1]; n++)
	{
		e = ll_rbyte(link, end, &recvbuf[n+2]);     if (e) return e;
	} 
	/* Check the packet checksum */
	sum = ll_parlink_crc(recvbuf);
	psum = (recvbuf[3 + recvbuf[1]] << 8) | recvbuf[2 + recvbuf[1]];

	/* Wait for end-of-packet */
	LOG(("Receiving end of packet\n"));
	e = ll_expect(link, end, -1, 3);     if (e) return e;	
	LOG(("Receive EOP: Sending '3'\n"));
	e = ll_lsend (link, end, 3    );	if (e) return e;
	e = ll_expect(link, end, 3, 2);       if (e) return e;	

LOG(("Packet received: "));
for (n = 0; n < 4 + recvbuf[1]; n++)
{
	LOG(("%02x ", recvbuf[n]));
}
LOG(("\n"));


	memcpy(pkt, recvbuf, 4 + recvbuf[1]);
	if (sum != psum) return LL_E_CRC;

	return LL_E_OK;
}


