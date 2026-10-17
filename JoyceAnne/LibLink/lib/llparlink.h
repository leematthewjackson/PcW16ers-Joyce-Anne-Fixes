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


/* LocoLink packet parser / constructor 
 *
 * Give it a stream of LocoLink I/O reads/writes, and it will assemble 
 * packets, pass them to a callback function, and convert the results back
 * to I/O reads/writes.
 */

typedef enum
{
	RIDLE,	/* Waiting to receive packet */
	READ1,	/* Start of packet detected */
	READ2,	/* Reading packet (odd bit) */
	READ3,	/* Reading packet (even bit) */
	READ4,	/* End of packet read */
	SIDLE,	/* Waiting to send packet */
	SEND1,	/* Sending start of packet */
	SEND2,	/* Sending packet (even bit) */
	SEND3,	/* Sending packet (odd bit) */
	SEND4,	/* End of packet written */
	HANGUP1,/* Hanging up */
	HANGUP2,/* Hanging up */
} LLPL_STATE;

typedef struct ll_parlink
{
	LLPL_STATE state;		/* State machine for receive / send */
	unsigned char recvd;		/* Last byte written to link */
	unsigned char sent;		/* Byte to return to host */
	unsigned rdptr;			/* Next bit to read in recv_packet */
	unsigned wrptr;			/* Next bit to write in send_packet */
	LL_PROTOCOL protocol;		/* LocoLink protocol version 1-3 */
	void *cookie;			/* Cookie for callback function */

	unsigned char recv_packet[260];	/* Last packet received */
	unsigned char send_packet[260];	/* Packet to return to caller */

	LL_PCALLBACK callback;		/* Callback function to process pkts */

} LL_PARLINK;

