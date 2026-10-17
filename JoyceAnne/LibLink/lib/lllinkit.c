/* liblink: Simulate parallel / serial hardware for emulators

    Copyright (C) 2002, 2024  John Elliott <seasip.webmaster@gmail.com>

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

/* Transmit or receive data through a PCW-Linkit link. */



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


#define END_OF_BLOCK	0x100
#define END_OF_TRANSFER 0x200


/* Characters encoded on base27 branch */
static unsigned char lookup_base27[27] = "\n\r0123456789!\"%'()*+-.&:;=?";
/* Characters encoded on base81 branch */
static unsigned char lookup_base81[81] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
	0x08, 0x0B, 0x0C, 0x0E, 0x0F, 0x10, 0x11, 0x12, 
	0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 
	0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x23, 0x24, 0x2F,
	0x3C, 0x3E, 0x40, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
	0x60, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F };
static unsigned char lookup_base243[243];


typedef struct ll_pcwlinkit
{
	LL_PDEV device;
	unsigned rle_byte;
	unsigned rle_len;
	unsigned short checksum;
	unsigned char lastread;
	unsigned char lastwrite;
} ll_pcwlinkit;


/* PCW-Linkit functions */
ll_error_t ll_linkit_create(LL_PCWLINKIT *pself, LL_PDEV device)
{
	LL_PCWLINKIT self;
	int n;

	if (!device || !pself) return LL_E_BADPTR;
	self = malloc(sizeof(ll_pcwlinkit));
	if (!self) return LL_E_NOMEM;
	memset(self, 0, sizeof(ll_pcwlinkit));
	for (n = 0; n < 128; n++) lookup_base243[n] = n | 0x80;

	self->device = device;
	*pself = self;
	return LL_E_OK;	
}

ll_error_t ll_linkit_destroy(LL_PCWLINKIT *pself)
{
	if (!pself) return LL_E_BADPTR;

	if (*pself)
	{
		free(*pself);
		*pself = NULL;
		return LL_E_OK;
	}
	return LL_E_BADPTR;
}


static ll_error_t send_bit0(LL_PCWLINKIT self, unsigned char c)
{
	self->lastwrite &= ~1;
	self->lastwrite |= (c ? 1 : 0);

/* We always send the inverse of the value we 'mean' to send; it will be 
 * negated on the wire, so the receiving end will see the value 'meant'. */

	return ll_send(self->device, self->lastwrite ^ 3);	
}

static ll_error_t send_bit1(LL_PCWLINKIT self, unsigned char c)
{
	self->lastwrite &= ~2;
	self->lastwrite |= (c ? 2 : 0);

/* We always send the inverse of the value we 'mean' to send; it will be 
 * negated on the wire, so the receiving end will see the value 'meant'. */

	return ll_send(self->device, self->lastwrite ^ 3);	
}



static ll_error_t toggle_bit1(LL_PCWLINKIT self)
{
	self->lastwrite ^= 2;
	return ll_send(self->device, self->lastwrite ^ 3);	
}


static ll_error_t recv_bit0(LL_PCWLINKIT self, unsigned char *c)
{
	unsigned char v;

	ll_error_t e = ll_recv_poll(self->device, &v);
	*c = (v & 1) ? 0 : 1;
	
	self->lastread = v;
	return e;
}

static ll_error_t recv_bit1(LL_PCWLINKIT self, unsigned char *c)
{
	unsigned char v;

	ll_error_t e = ll_recv_poll(self->device, &v);
	*c = (v & 2) ? 0 : 1;
	self->lastread = v;
	return e;
}

static ll_error_t wait_bit1(LL_PCWLINKIT self, unsigned char *c)
{
	unsigned char v;

	ll_error_t e = ll_recv_wait(self->device, &v);
	*c = (v & 2) ? 0 : 1;
	self->lastread = v;
	return e;
}



static ll_error_t recv_trit(LL_PCWLINKIT self, unsigned char *c)
{
	unsigned char v, v1;
	ll_error_t e;

	/* Remember current state of link */
	e = ll_recv_poll(self->device, &v); if (e) return e;
	/* Clock the other end for the next bit */
	e = toggle_bit1(self);	    if (e) return e;
	/* Wait until the value sent changes */
	do
	{
		e = ll_recv_poll(self->device, &v1); if (e) return e;
		self->lastread = v1;	
	} while (v1 == v);

	/* Return the XOR of the previous and current values */
	*c = ((v ^ v1) & 3);
	return e;
}

ll_error_t recv_base27(LL_PCWLINKIT self, unsigned char *v)
{
	unsigned char c1, c2, c3;
	ll_error_t e;

	e = recv_trit(self, &c1); if (e) return e;
	e = recv_trit(self, &c2); if (e) return e;
	e = recv_trit(self, &c3); if (e) return e;

	*v = (c3 - 1) + 3 * (c2 - 1) + 9 * (c1 - 1);
	return e;	
}

ll_error_t recv_base81(LL_PCWLINKIT self, unsigned char *v)
{
	unsigned char c1, c2, c3, c4;
	ll_error_t e;

	e = recv_trit(self, &c1); if (e) return e;
	e = recv_trit(self, &c2); if (e) return e;
	e = recv_trit(self, &c3); if (e) return e;
	e = recv_trit(self, &c4); if (e) return e;
	*v = (c4 - 1) + 3 * (c3 - 1) + 9 * (c2 - 1) + 27 * (c1 - 1);
	return e;	
}


ll_error_t recv_base243(LL_PCWLINKIT self, unsigned char *v)
{
	unsigned char c1, c2, c3, c4, c5;
	ll_error_t e;

	e = recv_trit(self, &c1); if (e) return e;
	e = recv_trit(self, &c2); if (e) return e;
	e = recv_trit(self, &c3); if (e) return e;
	e = recv_trit(self, &c4); if (e) return e;
	e = recv_trit(self, &c5); if (e) return e;
	*v = (c5 - 1) + 3 * (c4 - 1) + 9 * (c3 - 1) + 27 * (c2 - 1) + 81 * (c1 - 1);
	return e;
}



static ll_error_t recv_byte(LL_PCWLINKIT self, unsigned *result)
{
	unsigned char c;
	unsigned v1;
	ll_error_t e;

/* If we are currently in an RLE sequence, just keep on churning out the RLE */
	if (self->rle_len > 0)
	{
		*result = self->rle_byte;
		--self->rle_len;
		return LL_E_OK;
	}

	e = recv_trit(self, &c); if (e) return e;

	switch (c)
	{
		/* Lower-case letters */
		case 1:	
			e = recv_base27(self, &c); if (e) return e;
			if (c == 26) *result = END_OF_BLOCK;
			else	     *result = 'a' + c;
			return e;
		case 2:
			e = recv_trit(self, &c); if (e) return e;	
			switch (c)
			{
				case 1: /* Upper-case letters */
				e = recv_base27(self, &c); if (e) return e;
				if (c == 26) *result = END_OF_TRANSFER;
				else	     *result = 'A' + c;
				return e;
				
				case 2:	/* base-81 encoding */
				e = recv_base81(self, &c); if (e) return e;
				*result = lookup_base81[c];
				return e;	

				case 3:	/* base-243 encoding */
				e = recv_base243(self, &c); if (e) return e;
				*result = lookup_base243[c];
				return e;
			}
			return e;

		case 3:
			e = recv_trit(self, &c); if (e) return e;	
			switch (c)
			{
				case 1:	/* Space, tab or comma */
				e = recv_trit(self, &c); if (e) return e;	
				*result = "_ \t,"[c];
				return e;

				case 2: 
				e = recv_base27(self, &c); if (e) return e;
				*result = lookup_base27[c];
				return e;

				case 3:
				/* RLE run */
				e = recv_base27(self, &c);
				e = recv_byte(self, &v1);
				self->rle_byte = v1;	
				self->rle_len = c + 3;
				*result = self->rle_byte;
				return e;
		}
	}
	return e;

}

static ll_error_t send_trit(LL_PCWLINKIT self, unsigned vnew)
{
	unsigned char v, recv_hi;
	ll_error_t e;

	LOG(("Sending trit: %d lastread=%02x\n", vnew, (~self->lastread) & 3));
	/* Idle until incoming bit 1 changes */
	recv_hi = (self->lastread & 2) ? 0 : 1;
	LOG(("recv_hi = %d\n", recv_hi)); 
	do
	{
		e = wait_bit1(self, &v); if (e) return e;
/*		LOG(("v=%d\n", v)); */
	}
	while (v == recv_hi);
	LOG(("Toggled: lastread=%02x\n", (~self->lastread) & 3));

	/* Then send the trit */	
	self->lastwrite = (self->lastwrite ^ vnew) & 3;
	return ll_send(self->device, self->lastwrite ^ 3);	

}

static ll_error_t send_base27(LL_PCWLINKIT self, unsigned v)
{
	ll_error_t e;

	e = send_trit(self, ((v / 9) % 3) + 1); if (e) return e;
	e = send_trit(self, ((v / 3) % 3) + 1); if (e) return e;
	e = send_trit(self, (v % 3) + 1); 
	return e;
}


static ll_error_t send_base81(LL_PCWLINKIT self, unsigned v)
{
	ll_error_t e;

	e = send_trit(self, ((v / 27) % 3) + 1); if (e) return e;
	e = send_trit(self, ((v / 9) % 3) + 1); if (e) return e;
	e = send_trit(self, ((v / 3) % 3) + 1); if (e) return e;
	e = send_trit(self, (v % 3) + 1); 
	return e;
}


static ll_error_t send_base243(LL_PCWLINKIT self, unsigned v)
{
	ll_error_t e;

	e = send_trit(self, ((v / 81) % 3) + 1); if (e) return e;
	e = send_trit(self, ((v / 27) % 3) + 1); if (e) return e;
	e = send_trit(self, ((v / 9) % 3) + 1); if (e) return e;
	e = send_trit(self, ((v / 3) % 3) + 1); if (e) return e;
	e = send_trit(self, (v % 3) + 1); 
	return e;
}


static ll_error_t send_ubyte(LL_PCWLINKIT self, unsigned v)
{
	ll_error_t e;
	char *p;

	LOG(("send_ubyte %02x\n", v));
	switch(v)
	{
		case ' ':	e = send_trit(self, 3); if (e) return e;
				e = send_trit(self, 1); if (e) return e;
				return send_trit(self, 1);
		case '\t':	e = send_trit(self, 3); if (e) return e;
				e = send_trit(self, 1); if (e) return e;
				return send_trit(self, 2);
		case ',':	e = send_trit(self, 3); if (e) return e;
				e = send_trit(self, 1); if (e) return e;
				return send_trit(self, 3);
		case 0:		e = send_trit(self, 2); if (e) return e;
				e = send_trit(self, 2); if (e) return e;
				e = send_trit(self, 1); if (e) return e;
				e = send_trit(self, 1); if (e) return e;
				e = send_trit(self, 1); if (e) return e;
				return send_trit(self, 1);
		case END_OF_BLOCK:
				e = send_trit(self, 1); if (e) return e;
				e = send_trit(self, 3); if (e) return e;
				e = send_trit(self, 3); if (e) return e;
				return send_trit(self, 3); 
		case END_OF_TRANSFER:
				e = send_trit(self, 2); if (e) return e;
				e = send_trit(self, 1); if (e) return e;
				e = send_trit(self, 3); if (e) return e;
				e = send_trit(self, 3); if (e) return e;
				return send_trit(self, 3); 
	}
	p = strchr((char *)lookup_base27, v);
	if (p != NULL)
	{
		e = send_trit(self, 3); if (e) return e;
		e = send_trit(self, 2); if (e) return e;
		return send_base27(self, p - (char *)lookup_base27);
	}
/* The first byte of lookup_base81[] is NUL, which strchr can't cope with */
	p = strchr((char *)lookup_base81 + 1, v);
	if (p != NULL)
	{
		e = send_trit(self, 2); if (e) return e;
		e = send_trit(self, 2); if (e) return e;
		return send_base81(self, p - (char *)lookup_base81);
	}



	if (v >= 'a' && v <= 'z')
	{
		e = send_trit(self, 1); if (e) return e;
		return send_base27(self, v - 'a'); 
	}
	if (v >= 'A' && v <= 'Z')
	{
		e = send_trit(self, 2); if (e) return e;
		e = send_trit(self, 1); if (e) return e;
		return send_base27(self, v - 'A'); 
	}
	if (v >= 0x80) 
	{
		e = send_trit(self, 2); if (e) return e;
		e = send_trit(self, 3); if (e) return e;
		return send_base243(self, v & 0x7F); 
	}
	fprintf(stderr, "Unimplemented send for code 0x%02x\n", v);
	return LL_E_NOTIMPL;
}


static ll_error_t send_flush(LL_PCWLINKIT self)
{
	ll_error_t e;
	int n;

	if (self->rle_len == 0 || self->rle_len == 0xFF) return LL_E_OK;
	if (self->rle_len < 4)
	{
		for (n = 0; n < self->rle_len; n++)
		{
			e = send_ubyte(self, self->rle_byte);
			if (e) return e;
		}	
	}
	else
	{
LOG(("Flushing RLE sequence: len=%d -> %d  byte=%02x\n", 
		self->rle_len, self->rle_len - 4, self->rle_byte));

		e = send_trit(self, 3); if (e) return e;
		e = send_trit(self, 3); if (e) return e;
		e = send_base27(self, self->rle_len - 4); if (e) return e;
		e = send_ubyte(self, self->rle_byte); if (e) return e;
		self->rle_len = 0xFF;
	}
	return LL_E_OK;
}


static ll_error_t send_byte(LL_PCWLINKIT self, unsigned c)
{
	ll_error_t e;

	if (c == self->rle_byte && self->rle_len < 30)
	{
		++self->rle_len;
		return LL_E_OK;
	}
	e = send_flush(self);
	if (e) return e;

	self->rle_byte = c;
	self->rle_len = 1;
	return LL_E_OK;
}




/* Listen for a file transfer */
ll_error_t ll_linkit_receive(LL_PCWLINKIT self,
			     void *cookie,
				ll_error_t (*createfunc)(void *cookie, const char *filename, unsigned long size),
				ll_error_t (*writefunc) (void *cookie, unsigned char *data, unsigned long size),
				ll_error_t (*closefunc) (void *cookie))
{
	unsigned char v;
	unsigned char recv_hi = 0;
	unsigned vbyte, ckl, ckh;
	ll_error_t e;
	unsigned char header[15];
	unsigned long filesize, fptr;
	char *p;
	int n;
	char filename[20];
	char ext[5];
	unsigned char *buf;
	
	LOG(("Idle\n"));
	/* Idle until incoming bit 1 changes */
	e = recv_bit1(self, &v); if (e) return e;
	recv_hi = v;
	do
	{
		e = wait_bit1(self, &v); if (e) return e;
	}
	while (v == recv_hi);

	LOG(("Transfer initiated\n"));
	/* Now process incoming file(s) */
	while (1)	
	{
		self->rle_byte = 0;
		self->rle_len = 0;

		LOG(("Reading header\n"));
		/* Transfer initiated (or EOT received) */
		self->checksum = 0;
		/* Read the header */
		for (n = 0; n < 15; n++)
		{
			e = recv_byte(self, &vbyte);
			if (e) return e;
			if (vbyte == END_OF_TRANSFER)
			{
				LOG(("End of transfer\n"));
				return LL_E_OK;
			}
			header[n] = vbyte;
			self->checksum += vbyte;
		}
		e = recv_byte(self, &vbyte); if (e) return e;	/* Should be End of Block */
		if (vbyte != END_OF_BLOCK) 
		{
			/* Return an error */
			send_bit0(self, 1);
			recv_byte(self, &vbyte);
			return LL_E_UNEXPECT;
		}

		e = recv_byte(self, &ckh);   if (e) return e;	/* Checksum high */
		e = recv_byte(self, &ckl);   if (e) return e;	/* Checksum low */

		if (ckh * 256 + ckl != self->checksum)
		{
			/* Return an error */
			send_bit0(self, 1);
			recv_byte(self, &vbyte);
			return LL_E_CRC;
		} 
		filesize = header[0] * 32768 + header[1] * 128;

		sprintf(filename, "%-8.8s", header + 3);
		sprintf(ext, "%-4.4s", header + 11);
		p = strchr(filename, ' ');
		if (p) *p = 0;
		if (ext[1] != ' ')
		{
			strcat(filename, ext);
			p = strchr(filename, ' ');
			if (p) *p = 0;
		}	
		LOG(("Header received and checksum OK. Filename=%s size=%ld\n",
			filename, filesize));
		e = (*createfunc)(cookie, filename, filesize);	
		if (!e)
		{
			buf = malloc(filesize);
			if (!buf) e = LL_E_NOMEM;
		}
		if (e)
		{
			/* Return an error */
			send_bit0(self, 1);
			recv_byte(self, &vbyte);
			return e;
		}
		LOG(("File created\n"));
		/* Header was accepted */
		e = send_bit0(self, 0); if (e) return e;
		e = recv_byte(self, &vbyte); if (e) return e;
		self->checksum = 0;
		self->rle_len = 0;
		self->rle_byte = 0;
	
		LOG(("Reading %ld bytes\n", filesize));
		for (fptr = 0; fptr < filesize; fptr++)
		{
			e = recv_byte(self, &vbyte);
			if (e) 
			{
				free(buf);
				return e;
			}
			buf[fptr] = vbyte;
			self->checksum += vbyte;
		}
		e = recv_byte(self, &vbyte); if (e) return e;	/* Should be End of Block */
		LOG(("Read %ld bytes. Next byte %04x\n", filesize, vbyte));
		if (vbyte != END_OF_BLOCK) 
		{
			/* Return an error */
			send_bit0(self, 1);
			recv_byte(self, &vbyte);
			free(buf);
			return LL_E_UNEXPECT;
		}

		e = recv_byte(self, &ckh);   if (e) return e;	/* Checksum high */
		e = recv_byte(self, &ckl);   if (e) return e;	/* Checksum low */

		LOG(("Checksum=%04x expected %04x\n", self->checksum, ckh * 256 + ckl));
		if (ckh * 256 + ckl != self->checksum)
		{
			/* Return an error */
			send_bit0(self, 1);
			recv_byte(self, &vbyte);
			free(buf);
			return LL_E_CRC;
		} 
		LOG(("Writing data\n"));	

		e = (*writefunc)(cookie, buf, filesize);
		LOG(("Closing file\n"));	
		if (!e) e = (*closefunc)(cookie);
		if (e)
		{
			send_bit0(self, 1);
			recv_byte(self, &vbyte);
			free(buf);
			return e;
		}
		free(buf);
		e = send_bit0(self, 0); if (e) return e;
		e = recv_byte(self, &vbyte); if (e) return e;
	}
}


ll_error_t ll_linkit_send_file(LL_PCWLINKIT self, const char *filename, 
		unsigned char *data, unsigned long size, int user)
{
	char header[20];
	char pname[PATH_MAX];
	char bname[PATH_MAX];
	char *p, *q;
	unsigned short cksum;
	unsigned char c;
	int n;
	ll_error_t e;
	unsigned long recs;

	LOG(("Send file: name=%d:%s size=%ld\n", user, filename, size));
	/* Round up to CP/M record count */
	recs = (size + 127) / 128;
	if (recs > 0xFFFF)	/* The biggest that can be represented */
		return LL_E_TOOBIG;

	header[0] = (recs >> 8);
	header[1] = recs & 0xFF;
	header[2] = '0' + (user & 7);	/* User number */

#ifdef HAVE_WINDOWS_H
	GetShortPathName(filename, pname, PATH_MAX);
	q = strrchr(pname, '\\');
	if (q) q++;
	else q = (char *)filename;
#else
	strcpy(pname, filename);
	q = basename(pname);
	if (!q) q = (char *)filename;
#endif
	strcpy(bname, q);
	p = strchr(bname, '.');
	if (p)
	{
		*p = 0;
		sprintf(header + 3, "%-8.8s.%-3.3s", bname, p + 1);
	}
	else
	{
		sprintf(header + 3, "%-8.8s.   ", bname);
	}
	for (n = 3; n < 15; n++) header[n] = toupper(header[n]);
	cksum = 0;
	LOG(("FCB name: %c:%s\n", header[2], header + 3));

	LOG(("Header: "));
	for (n = 0; n < 15; n++) 
	{
		LOG(("%02x ", header[n]));
	}
	LOG(("\n"));


	self->rle_len = 0xff;
	self->rle_byte = 0;
	for (n = 0; n < 15; n++)
	{
		cksum += header[n];
		e = send_byte(self, header[n]); 
		if (e) return e;
	}
	LOG(("Sending END_OF_BLOCK with checksum=%04x\n", cksum));
	e = send_byte(self, END_OF_BLOCK); if (e) return e;
	e = send_byte(self, (cksum >> 8) & 0xFF); if (e) return e;
	e = send_byte(self, cksum & 0xFF); if (e) return e;
	e = send_byte(self, 0); if (e) return e;
	e = send_flush(self); if (e) return e;
	LOG(("Header sent. Ready to check status\n"));
/* The status will have been captured in lastread while transmitting that 
 * last byte */
	c = (~self->lastread) & 1;
	LOG(("Status received: %d lastread=%02x\n", c, (~self->lastread) & 3));

	if (c == 1)	/* Rejected */
	{
		return LL_E_REJECTED;
	}
	self->rle_len = 0xff;
	self->rle_byte = 0;
	cksum = 0;
	for (n = 0; n < 128 * recs; n++)
	{
	LOG(("Sending data %d / %d\n", n, 128 * recs));
		if (n < size)	c = data[n];
		else		c = 0x1A; 	
		cksum += c;
		e = send_byte(self, c);
		if (e) return e;
	}
	LOG(("Sending END_OF_BLOCK for data with checksum=%04x\n", cksum));
	e = send_byte(self, END_OF_BLOCK); if (e) return e;
	e = send_byte(self, (cksum >> 8) & 0xFF); if (e) return e;
	e = send_byte(self, cksum & 0xFF); if (e) return e;
	e = send_byte(self, 0); if (e) return e;
	e = send_flush(self); if (e) return e;
/* The status will have been captured in lastread while transmitting that 
 * last byte */
	c = (~self->lastread) & 1;
	LOG(("Status received: %d lastread=%02x\n", c, (~self->lastread) & 3));

	return (c == 0) ? LL_E_OK : LL_E_REJECTED;
}

ll_error_t ll_linkit_send_begin(LL_PCWLINKIT self)
{
	ll_error_t e;
	unsigned char c;

	LOG(("Requesting to transmit\n"));
	/* Capture current state to lastread */
	e = recv_bit1(self, &c);
	/* Then toggle the high bit */
	e = toggle_bit1(self); 
	LOG(("Request sent, result = %d\n", e));
	return e;
}


ll_error_t ll_linkit_send_eot(LL_PCWLINKIT self)
{
	ll_error_t e;

	LOG(("Sending End of Transfer\n"));
	e = send_ubyte(self, END_OF_TRANSFER);
	LOG(("End of Transfer sent, result = %d\n", e));
	return e;
}


ll_error_t ll_linkit_connect(LL_PCWLINKIT self)
{
	unsigned char sh = 0;
	unsigned char sl = 0;
	unsigned char v;
	ll_error_t e;
	int transitions = 0;
	int tries = 0;
	int complete = 0;

	e = send_bit1(self, sh); if (e) return e;
	e = send_bit0(self, sl); if (e) return e;

	while (1)
	{
		if (sh == 0)
		{
			e = recv_bit0(self, &v); if (e) return e;

			if (v == 0)	/* Sent 0 on H, got 0 on L: state A */
			{
				LOG(("a"));
			}
			else		/* Sent 0 on H, got 1 on L: state B */
			{
				LOG(("b"));	
				sh ^= 1;
				LOG(("Sending %d on line H\n", sh));
				e = send_bit1(self, sh); if (e) return e;
				LOG(("Sent    %d on line H\n", sh));
			}
		}
		else
		{
			e = recv_bit0(self, &v); if (e) return e;
			if (v == 0)
			{
				LOG(("c"));
				if (transitions >= 10) break;
				++transitions;
				sh ^= 1;
				LOG(("Sending %d on line H ++transitions\n", sh));
				e = send_bit1(self, sh); if (e) return e;
				LOG(("Sent    %d on line H ++transitions\n", sh));
			}
		}
		e = recv_bit1(self, &v); if (e) return e;
		if (sl != !v)
		{
			LOG(("%d on H -> %d on L\n", v, !v));
		}
		sl = !v;
		e = send_bit0(self, sl); if (e) return e;
	} 
	while (!complete)
	{
		e = recv_bit1(self, &v); if (e) return e;
		if (v == !sl)
		{
			if (tries < 100)
			{
				++tries;
			}
			else
			{
				complete = 1;
			}
		} 
		else
		{
			sl = !sl;
			e = send_bit0(self, sl); if (e) return e;
			tries = 0;
		}
	}
/* Assert that we have got 0 on the low bit, 1 on the high bit */
	e = recv_bit0(self, &v); if (e) return e;
	if (v != 0) return LL_E_UNEXPECT;
	e = recv_bit1(self, &v); if (e) return e;
	if (v != 1) return LL_E_UNEXPECT;
	return LL_E_OK;

}


