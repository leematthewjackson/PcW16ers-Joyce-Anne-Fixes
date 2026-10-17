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
#include "lldosfs.h"
#include "crc16.h"
#include "lllocoserv.h"
#include "lltypes.h"

typedef unsigned char zbyte;

/* LocoLink file server. 
 *
 * Give it a LocoLink packet (received from whatever source) 
 * and it will return a suitable response, serving files
 * from a directory passed to it at startup.
 */ 

/* Forward declarations... */
static ll_error_t ll_fs_dispatch_1(LL_PFS self, zbyte *input, zbyte *output);
static ll_error_t ll_fs_dispatch_2(LL_PFS self, zbyte *input, zbyte *output);
static ll_error_t ll_fs_dispatch_3(LL_PFS self, zbyte *input, zbyte *output);
static ll_error_t ll_fs_dispatch_x_idle(LL_PFS self, zbyte *input, zbyte *output);
static ll_error_t ll_fs_dispatch_x_findfirst(LL_PFS self, zbyte *input, zbyte *output);
static ll_error_t ll_fs_dispatch_x_open(LL_PFS self, zbyte *input, zbyte *output);
static ll_error_t ll_fs_dispatch_x_create(LL_PFS self, zbyte *input, zbyte *output);
static ll_error_t ll_fs_dispatch_x_read(LL_PFS self, zbyte *input, zbyte *output);
static ll_error_t ll_fs_dispatch_x_write(LL_PFS self, zbyte *input, zbyte *output);
static ll_error_t ll_fs_dispatch_x_mkdir(LL_PFS self, zbyte *input, zbyte *output);
static ll_error_t ll_fs_dispatch_x_rmdir(LL_PFS self, zbyte *input, zbyte *output);
static ll_error_t ll_fs_dispatch_x_chdir(LL_PFS self, zbyte *input, zbyte *output);
static ll_error_t ll_fs_dispatch_x_unlink(LL_PFS self, zbyte *input, zbyte *output);
static ll_error_t ll_fs_dispatch_x_chmod(LL_PFS self, zbyte *input, zbyte *output);
static ll_error_t ll_fs_dispatch_x_ren1(LL_PFS self, zbyte *input, zbyte *output);
static ll_error_t ll_fs_dispatch_x_ren2(LL_PFS self, zbyte *input, zbyte *output);
static ll_error_t ll_fs_dispatch_x_ccreat(LL_PFS self, zbyte *input, zbyte *output);
static ll_error_t ll_fs_dispatch_ackdata(LL_PFS self, zbyte *input, zbyte *output);
static ll_error_t ll_fs_doscall_x(LL_PFS self, zbyte func, word16 ax, word16 bx, word16 cx, word16 dx, zbyte *output);


static ll_error_t ll_fs_error(LL_PFS self, zbyte *output, ll_error_t err);
static ll_error_t ll_fs_result(LL_PFS self, zbyte *input, zbyte *output, int result);
static ll_error_t ll_fs_request(LL_PFS self, zbyte *output, 
				REQTYPE type, int max);
static ll_error_t ll_fs_retdata(LL_PFS self, zbyte *output, 
				REQTYPE type, const void *source, int len);

/* Helpers to read little-endian bytes and words */
static word16 peek2(zbyte *c)
{
	return c[0] | (((word16)c[1]) << 8);
}

static unsigned long peek4(zbyte *c)
{
	return c[0] | (((unsigned long)c[1]) << 8) | 
		      (((unsigned long)c[2]) << 16) |
		      (((unsigned long)c[3]) << 24);
}



/*
static void logfunc(const char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	vfprintf(stderr, s, ap);
	va_end(ap);
	fflush(stderr);
}
#define LOG(x) logfunc x
*/
#define LOG(x)

/* File access callback. Default is to allow everything. */
static ll_error_t default_callback(void *cookie, const char *filename)
{
	return LL_E_OK;	
}



/* Create a fileserver, serving up files in 'dirname' and subdirectories */
ll_error_t ll_fs_create(LL_PFS *pself, const char *dirname)
{
	LL_PFS p;
	ll_error_t e;
	int nh;

	if (!pself || !dirname) return LL_E_BADPTR;

	p = malloc(sizeof(LL_FSERVER));
	if (!p) return LL_E_NOMEM;

	memset(p, 0, sizeof(LL_FSERVER));

	/* Create an LL_DOSFS to emulate the DOS filesystem */
	e = ll_dosfs_construct(&p->dosfs, dirname);
	if (e)
	{
		free(p);
		return e;
	}
	*pself = p;
	/* Initialise everything to idle */
	if (p->callback == NULL)
	{
		p->callback = default_callback;
	}
	for (nh = 0; nh < MAXFILEHANDLE; nh++)
	{
		p->fp[nh] = NULL;
		p->rwmode[nh] = 0;
	}
	p->xfer_fp = NULL;
	p->state = IDLE;
	return LL_E_OK;
}

/* Shut down file server */
ll_error_t ll_fs_close(LL_PFS *pself)
{	
	int nh;
	if (pself == NULL || *pself == NULL) return LL_E_BADPTR;

	/* Close any open files */
	for (nh = 0; nh < MAXFILEHANDLE; nh++)
	{
		if (pself[0]->fp[nh])
		{
			fclose(pself[0]->fp[nh]);
		}
	}
	/* Destroy the DOS fileserver */
	ll_dosfs_destruct(&(pself[0]->dosfs));
	free(pself[0]);
	*pself = NULL;
	return LL_E_OK;
}

/* Set / get the file access callback */
ll_error_t ll_fs_set_callback(LL_PFS pself, LL_FSCALLBACK func, void *cookie)
{
	if (!pself) return LL_E_BADPTR;

	pself->callback = func;
	pself->cookie   = cookie;
	if (pself->callback == NULL)
	{
		pself->callback = default_callback;
	}
	return ll_dosfs_set_callback(pself->dosfs, func, cookie);
}

ll_error_t ll_fs_get_callback(LL_PFS pself, LL_FSCALLBACK *func, void **cookie)
{
	if (!pself) return LL_E_BADPTR;

	if (func) 
	{
		*func = pself->callback;
		if (pself->callback == default_callback)
		{
			*func = NULL;	
		}
	}
	if (cookie) *cookie = pself->cookie;
	return LL_E_OK;
}


/* This is to wrap ll_fs_dispatch() so that it matches the signature 
 * expected by ll_set_fs_callback().
 * 
 * It also logs the packets sent and received, if logging is enabled */
ll_error_t ll_fs_dispatch_v(void *self, zbyte *input, zbyte *output)
{
	unsigned pkt_len = 4 + input[1];
	int n;
	ll_error_t err;

	LOG(("   Packet received "));
	for (n = 0; n < pkt_len; n++)
	{
		LOG(("%02x ", input[n]));
	}
	LOG(("\n"));
	err = ll_fs_dispatch((LL_PFS)self, input, output);
	pkt_len = 4 + output[1];
	if (!err)
	{
		LOG(("   Packet returned "));
		for (n = 0; n < pkt_len; n++)
		{
			LOG(("%02x ", output[n]));
		}
		LOG(("\n"));
	}
	return err;
}


/* This function is called for each LocoLink packet received. It will 
 * parse and act upon it, generating a matching result packet. 
 *
 * In practice this function checks only for the three types of 'connect'
 * packet it knows and sets the protocol accordingly. Any other type of 
 * packet is processed according to the currently active protocol.
 */
ll_error_t ll_fs_dispatch(LL_PFS self, zbyte *input, zbyte *output)
{
/* v1 "start" packet */
	if (input[0] == CONNECT_V1 && input[1] == 2 && 
	    input[2] == 0x43 && input[3] == 0x31)
	{
		output[0] = 0x55;	/* Acknowledge */
		output[1] = 0x01;	/* 1 byte */
		output[2] = input[0];
		self->protocol = LOCOLINK_V1;	
		strcpy(self->clientname, "Unknown");
		LOG(("Connection from LocoLink v1 client\n"));
		return LL_E_OK;
	}
/* v2 "start" packet */
	if (input[0] == CONNECT_V2 && input[1] >= 2 && input[1] <= 32 &&
	    input[2] == 0x41 && input[3] == 0x31)
	{
		memcpy(self->clientname, input + 4, input[1] - 2);
		self->clientname[input[1] - 2] = 0;
		strcpy((char*)output + 3, "LibLink " LIBLINK_VERSION);
		output[0] = ACK_V2;	/* Acknowledge */
		output[1] = 1 + strlen((char *)output + 3);
		output[2] = input[0];
		self->protocol = LOCOLINK_V2;	
		LOG(("Connection from LocoLink v2 client: %s\n", self->clientname));
		return LL_E_OK;
	}


/* v3 "start" packet */
	if (input[0] == CONNECT_V3 && input[1] >= 2 && input[1] <= 32 &&
	    input[2] == 0x46 && input[3] == 0x31)
	{
		memcpy(self->clientname, input + 4, input[1] - 2);
		self->clientname[input[1] - 2] = 0;
		strcpy((char*)output + 3, "LibLink " LIBLINK_VERSION);
		output[0] = ACK_V3;	/* Acknowledge */
		output[1] = 1 + strlen((char *)output + 3);
		output[2] = input[0];
		self->protocol = LOCOLINK_V3;	
		LOG(("Connection from LocoLink v3 client: %s\n", self->clientname));

		return LL_E_OK;
	}


	switch(self->protocol)
	{
		case LOCOLINK_V1: return ll_fs_dispatch_1(self, input, output);
		case LOCOLINK_V2: return ll_fs_dispatch_2(self, input, output);
		case LOCOLINK_V3: return ll_fs_dispatch_3(self, input, output);
		default:
		return LL_E_UNEXPECT;	/* Don't know what to do with this packet */
	}
}


/* Map LL_E_ codes to DOS codes */
static zbyte dos_error_map[] = 
{
	0x00,	/* LL_E_OK		No error */
	0x14,	/* LL_E_UNKNOWN		Unknown error */
	0x08,	/* LL_E_NOMEM		Out of memory */
	0x09,	/* LL_E_BADPTR		Bad pointer */
	0x07,	/* LL_E_SYSERR		System error (try to map this better) */
	0x50,	/* LL_E_EXIST		File exists */
	0x1A,	/* LL_E_NOMAGIC		Not a LocoLink shared mem file */
	0x15,	/* LL_E_CLOSED		Link is closed */
	0x16,	/* LL_E_OPEN		Link is open */
	0x20,	/* LL_E_INUSE		Link in use */
	0x79,	/* LL_E_TIMEOUT		Timeout */
	0x0D,	/* LL_E_BADPKT		Data invalid */
	0x0D,	/* LL_E_CRC		Bad CRC */
	0x0D,	/* LL_E_UNEXPECT	Unexpected value */
	0x0F,	/* LL_E_NODRIVER	Unknown driver name */
	0x0E,	/* LL_E_NOTIMPL		Not implemented */
	0x1F,	/* LL_E_NOTDEV		Trying to open a file as a device */
	0x03,	/* LL_E_BADNAME		Path not found */
	0x05,	/* LL_E_PASSWORD	Password required */
	0x36,	/* LL_E_AGAIN		Network busy */
	0x3B,	/* LL_E_SOCKERR		Unexpected network error */
	0x02,	/* LL_E_NOTFOUND	File not found */
	0x01,	/* LL_E_BADFUNC		Bad function */
	0x06,	/* LL_E_BADHANDLE	Invalid handle */
	0x05,	/* LL_E_DENIED		Access denied */
	0x04,	/* LL_E_TOOMANY		Too many open files */
	0x15,	/* LL_E_NOTREADY	Drive not ready */
	0x01,	/* LL_E_FAIL		Bad function */
	0x01,	/* LL_E_BADPROTO	Bad function */
	0x12,	/* LL_E_NOMOREFILES	No more results */
};

/****************************************************************************
 *                       Multi-version protocol handlers                    *
 ****************************************************************************/

/* Return an 'error' packet for the current protocol version */
static ll_error_t ll_fs_error(LL_PFS self, zbyte *output, ll_error_t err)
{
	int e;

	switch (self->protocol)
	{
		case LOCOLINK_V1:
			output[0] = ERROR_V1;	/* v1: Return error packet */
			output[1] = 0;
			return LL_E_OK;	
		case LOCOLINK_V2:
			output[0] = ERROR_V2;
			break;
		case LOCOLINK_V3:
			output[0] = ERROR_V3;
			break;
		default:
			return LL_E_BADPROTO;
	}
/* v2 / v3 error packet */
	output[1] = 2;
	e = -err;
	if (e >= 0 && e < sizeof(dos_error_map)) output[2] = dos_error_map[e];
	else output[2] = dos_error_map[-LL_E_UNKNOWN];		
	output[3] = 0;
	return LL_E_OK;	
}


/* Return a 'result' packet for the current protocol version containing a
 * single word */
static ll_error_t ll_fs_result(LL_PFS self, zbyte *input, zbyte *output, 
					int result)
{
	switch (self->protocol)
	{
		case LOCOLINK_V1:
			output[0] = RESULT_V1;	/* v1: Return result packet */
			output[1] = 2;
			if (input) output[2] = input[0]; else output[2] = 0;
			output[3] = result;
			return LL_E_OK;	
		case LOCOLINK_V2:
			output[0] = SUCCESS_V2;	/* v3 success packet */
			break;

		case LOCOLINK_V3:
			output[0] = SUCCESS_V3;	/* v3 success packet */
			break;
		default:
			return LL_E_BADPROTO;
	}
	/* v2 / v3 success packet */
	output[1] = 2;
	output[2] = result & 0xFF;
	output[3] = result >> 8;
	return LL_E_OK;	
}


/* Return a 'result' packet for the current protocol version containing
   4 words */
static ll_error_t ll_fs_longresult(LL_PFS self, zbyte *input, zbyte *output, 
					unsigned short ax, unsigned short bx,
					unsigned short cx, unsigned short dx)
{
	switch (self->protocol)
	{
		case LOCOLINK_V1:
			output[0] = RESULT_V1;	/* v1: Return result packet */
			output[1] = 2;
			if (input) output[2] = input[0]; else output[2] = 0;
			output[3] = ax;
			return LL_E_OK;	
		case LOCOLINK_V2:
			output[0] = SUCCESS_V2;	/* v3 success packet */
			break;

		case LOCOLINK_V3:
			output[0] = SUCCESS_V3;	/* v3 success packet */
			break;
		default:
			return LL_E_BADPROTO;
	}
	/* v2 / v3 success packet */
	output[1] = 8;
	output[2] = ax & 0xFF;
	output[3] = ax >> 8;
	output[4] = bx & 0xFF;
	output[5] = bx >> 8;
	output[6] = cx & 0xFF;
	output[7] = cx >> 8;
	output[8] = dx & 0xFF;
	output[9] = dx >> 8;
	return LL_E_OK;	
}




/****************************************************************************
 *                          LocoLink v1 protocol handlers                   *
 ****************************************************************************/

static ll_error_t get_filename(char *filename, zbyte *input, int offset)
{
	int n;

	/* Buffer smaller than filename */
	if (input[1] < (offset+1)) 
	{
		return LL_E_BADNAME;
	}
	memcpy(filename, input + offset + 2, input[1] - offset);
	filename[input[1] - offset] = 0;
	for (n = 0; filename[n]; n++)
	{
		filename[n] = tolower(filename[n]);
	}
	return LL_E_OK;
}

/* LocoLink v1 protocol handler */
static ll_error_t ll_fs_dispatch_1(LL_PFS self, zbyte *input, zbyte *output)
{
	unsigned long l1;
	char filename[PATH_MAX];
	ll_error_t err;
	zbyte fresult[0x2C];

	switch(input[0])
	{
		case HANGUP_V1:			/* Hang up */
			LOG(("v1 Hangup\n"));
			self->protocol = NO_PROTOCOL;
			return LL_E_CLOSED;

		case CKFREE_V1:			/* Check free space */
			if (get_filename(filename, input, 4))
			{
				return ll_fs_error(self, output, 0);
			}
			l1 = peek4(input + 2);
			LOG(("Check free space v1: L=%ld S=%s\n", l1, filename));
/* Returns 0 if there is space, 1 if not. Currently assumes there is space */
			return ll_fs_result(self, input, output, 0);

		case CREAT_V1:			/* Create file */
			if (get_filename(filename, input, 4))
			{
				return ll_fs_error(self, output, 0);
			}
			l1 = peek4(input + 2);
			LOG(("Create v1: L=%ld S=%s\n", l1, filename));
			if (self->fp[0])
			{
				LOG(("Create v1 fail: Already open"));
				return ll_fs_result(self, input, output, 1);
			}
/* Returns 0 if file created, 1 if not */
			err = ll_dosfs_creat(self->dosfs, filename, 0, &self->fp[0]);
			if (err) 
			{
				LOG(("ll_dosfs_creat failed: %s", ll_strerror(err)));
				return ll_fs_result(self, input, output, 1);
			}
			self->rwmode[0] = 1;
			return ll_fs_result(self, input, output, 0);
		case WRITE_V1:	/* Write data */
			LOG(("Write data v1: len=%d", input[1]));
			if (!self->fp[0])
				return ll_fs_result(self, input, output, 1);
			if (fwrite(input + 2, 1, input[1], self->fp[0]) < input[1])
			{
				fclose(self->fp[0]);
				self->fp[0] = NULL;
				return ll_fs_result(self, input, output, 1);
			}
			return ll_fs_result(self, input, output, 0);

		case CLOSE_V1:
			LOG(("Close file v1: checksum=0x%02x%02x", input[3], input[2]));
			if (!self->fp[0] || fclose(self->fp[0]))
			{
				self->fp[0] = NULL;
				return ll_fs_result(self, input, output, 1);
			}
			self->fp[0] = NULL;
			return ll_fs_result(self, input, output, 0);
		case MKDIR_V1:
			if (get_filename(filename, input, 0))
			{
				return ll_fs_error(self, output, 0);
			}
			err = ll_dosfs_mkdir(self->dosfs, filename);
			if (err != LL_E_OK)
			{
				return ll_fs_error(self, output, 0);
			}
			return ll_fs_result(self, input, output, 1);

		case FINDFIRST_V1:
			if (get_filename(filename, input, 1))
			{
				return ll_fs_error(self, output, 0);
			}
			l1 = input[2];
			LOG(("Findfirst v1: L=%ld S=%s\n", l1, filename));

			err = ll_dosfs_findfirst(self->dosfs, filename, 
				l1 ? 0x07 : 0x20, fresult);
/* Returns 0 if file exists, 1 if not */
			if (err == LL_E_OK)
			{
				return ll_fs_result(self, input, output, 0);
			}
			return ll_fs_result(self, input, output, 1);

	}
	LOG(("Unknown packet type v1: %02x\n", input[0]));
	return ll_fs_error(self, output, 0);
}







/****************************************************************************
 *                          LocoLink v2 protocol handlers                   *
 ****************************************************************************/

ll_error_t ll_fs_dispatch_2(LL_PFS self, zbyte *input, zbyte *output)
{
	switch (self->state)
	{
		case IDLE: 
		return ll_fs_dispatch_x_idle(self, input, output);

		case DOSX_OPEN:
		return ll_fs_dispatch_x_open(self, input, output);

		case DOSX_CREATE:
		return ll_fs_dispatch_x_create(self, input, output);

		case DOSX_READ:
		return ll_fs_dispatch_x_read(self, input, output);

		case DOSX_WRITE:
		return ll_fs_dispatch_x_write(self, input, output);

		case DOSX_FINDFIRST:
		return ll_fs_dispatch_x_findfirst(self, input, output);

		case DOSX_MKDIR:
		return ll_fs_dispatch_x_mkdir(self, input, output);

		case DOSX_RMDIR:
		return ll_fs_dispatch_x_rmdir(self, input, output);

		case DOSX_CHDIR:
		return ll_fs_dispatch_x_chdir(self, input, output);

		case DOSX_UNLINK:
		return ll_fs_dispatch_x_unlink(self, input, output);

		case DOSX_CHMOD:
		return ll_fs_dispatch_x_chmod(self, input, output);

		case DOSX_REN1:
		return ll_fs_dispatch_x_ren1(self, input, output);

		case DOSX_REN2:
		return ll_fs_dispatch_x_ren2(self, input, output);

		case DOSX_CCREAT:
		return ll_fs_dispatch_x_ccreat(self, input, output);

		case DOSX_ACKDATA:
		return ll_fs_dispatch_ackdata(self, input, output);
	}

	LOG(("Server v2: Unknown state %d\n", self->state));
	return LL_E_UNEXPECT;
}




/****************************************************************************
 *                          LocoLink v3 protocol handlers                   *
 ****************************************************************************/


ll_error_t ll_fs_dispatch_3(LL_PFS self, zbyte *input, zbyte *output)
{
	switch (self->state)
	{
		case IDLE: 
		return ll_fs_dispatch_x_idle(self, input, output);

		case DOSX_OPEN:
		return ll_fs_dispatch_x_open(self, input, output);

		case DOSX_CREATE:
		return ll_fs_dispatch_x_create(self, input, output);

		case DOSX_READ:
		return ll_fs_dispatch_x_read(self, input, output);

		case DOSX_WRITE:
		return ll_fs_dispatch_x_write(self, input, output);

		case DOSX_FINDFIRST:
		return ll_fs_dispatch_x_findfirst(self, input, output);

		case DOSX_MKDIR:
		return ll_fs_dispatch_x_mkdir(self, input, output);

		case DOSX_RMDIR:
		return ll_fs_dispatch_x_rmdir(self, input, output);

		case DOSX_CHDIR:
		return ll_fs_dispatch_x_chdir(self, input, output);

		case DOSX_UNLINK:
		return ll_fs_dispatch_x_unlink(self, input, output);

		case DOSX_CHMOD:
		return ll_fs_dispatch_x_chmod(self, input, output);

		case DOSX_REN1:
		return ll_fs_dispatch_x_ren1(self, input, output);

		case DOSX_REN2:
		return ll_fs_dispatch_x_ren2(self, input, output);

		case DOSX_CCREAT:
		return ll_fs_dispatch_x_ccreat(self, input, output);

		case DOSX_ACKDATA:
		return ll_fs_dispatch_ackdata(self, input, output);
	}

	LOG(("Server v3: Unknown state %d\n", self->state));
	return LL_E_UNEXPECT;
}

/****************************************************************************
 *                     LocoLink v2 / v3 protocol handlers                   *
 ****************************************************************************/


/* In the IDLE state, we expect either a HANGUP or a DOSCALL */
ll_error_t ll_fs_dispatch_x_idle(LL_PFS self, zbyte *input, zbyte *output)
{
	unsigned func, ax, bx, cx, dx;

	switch(input[0])
	{

		case HANGUP_V2:	LOG(("v2 Hangup\n"));
				self->protocol = NO_PROTOCOL;
				return LL_E_CLOSED;
		case HANGUP_V3:	LOG(("v3 Hangup\n"));
				self->protocol = NO_PROTOCOL;
				return LL_E_CLOSED;
		case ECHO_V3:	/* For testing: Echo input */
				output[0] = ACK_V3;
				output[1] = input[1] + 1;
				output[2] = ECHO_V3;
				memcpy(output + 3, input + 2, input[1]);
				return LL_E_OK;
		case DOSCALL_V2:
		case DOSCALL_V3:
				func = input[2];
				ax = input[3];
				bx = peek2(input + 4);
				cx = peek2(input + 6);
				dx = peek2(input + 8);

/* Special case for FINDNEXT: The 'long' packet contains a search handle */
				if (func == DOS_SNEXT && self->protocol == LOCOLINK_V2)
				{
					if (input[1] >= 0x0F) 
					{
						cx = peek2(input + 0x04);
						dx = peek2(input + 0x02);
					}
					else { cx = dx = 0xFFFF; }
				}
				if (func == DOS_SNEXT && self->protocol == LOCOLINK_V3)
				{
					if (input[1] >= 0x11) 
					{
						cx = peek2(input + 0x05);
						dx = peek2(input + 0x03);
					}
					else { cx = dx = 0xFFFF; }
				}

				LOG(("DOSCALL func=%02x AL=%02x BX=%04x CX=%04x DX=%04x\n",
					func, ax, bx, cx, dx));
				return ll_fs_doscall_x(self, func, ax, bx, cx, dx, output);
	}
	LOG(("Unknown packet type v2/v3: %02x\n", input[0]));
	return LL_E_UNEXPECT;
}


/* Generate a data request packet of type 'type', asking for up to 'max' 
 * bytes */
static ll_error_t ll_fs_request(LL_PFS self, zbyte *output, REQTYPE type, int max)
{
	switch (self->protocol)
	{
		case LOCOLINK_V2:
			output[0] = DATAREQ_V2;	/* v2 data request packet */
			break;
		case LOCOLINK_V3:
			output[0] = DATAREQ_V3;	/* v3 data request packet */
			break;
		default:
			return LL_E_BADPROTO;
	}

	output[1] = 2;
	output[2] = type;
	output[3] = max;
	return LL_E_OK;	
}


/* Generate a data return packet of type 'type', containing 'len' bytes 
 * from 'source' */
static ll_error_t ll_fs_retdata(LL_PFS self, zbyte *output, REQTYPE type, 
				const void *source, int len)
{
	switch (self->protocol)
	{
		case LOCOLINK_V2:
			output[0] = DATARET_V2;	/* v2 data result packet */
			break;
		case LOCOLINK_V3:
			output[0] = DATARET_V3;	/* v3 data result packet */
			break;
		default:
			return LL_E_BADPROTO;
	}
	output[1] = len + 1;
	output[2] = type;
	memcpy(output + 3, source, len);
	return LL_E_OK;	

}


/* Handle a DOSCALL packet - 'func' gives the DOS function, AX/BX/CX/DX are
 * parameters. */
static ll_error_t ll_fs_doscall_x(LL_PFS self, zbyte func, word16 ax, word16 bx, word16 cx, word16 dx, zbyte *output)
{
	ll_error_t err;
	zbyte fresult[0x2C];
	zbyte readbuf[0xFE];
	int count, rcount;
	long lpos;

	switch (func)
	{
		case DRV_ALLRESET:
			LOG(("DOSCALL: DRV_ALLRESET\n"));
/* Only v3 protocol supports multiple drives */
			if (self->protocol == LOCOLINK_V3)
			{
				self->drive = dx & 0xFF;	
				if (self->drive != 0) 
					return ll_fs_error(self, output, LL_E_NOTREADY);
			}
			return ll_fs_result(self, NULL, output, 0);
		case DRV_SET:
			LOG(("DOSCALL: DRV_SET %d\n", dx & 0xFF));
/* Only v3 protocol supports multiple drives */
			if (self->protocol == LOCOLINK_V3)
			{
				self->drive = dx & 0xFF;	
			}
			else
			{
				self->drive = 0;
			}
			return ll_fs_result(self, NULL, output, 1);
		case F_CLOSE:
			LOG(("DOSCALL: F_CLOSE\n"));
			return ll_fs_result(self, NULL, output, 0);
		case F_SFIRST:
			LOG(("DOSCALL: F_SFIRST\n"));
			if (self->drive != 0) return ll_fs_error(self, output, LL_E_NOTREADY);
			err = ll_dosfs_getlabel(self->dosfs, fresult);
			if (err)
			{
				self->state = IDLE;
				return ll_fs_error(self, output, LL_E_NOTFOUND);
			}
			/* Return data and wait for an ack */
			self->state = DOSX_ACKDATA;
			self->dos3_result = 0;
			return ll_fs_retdata(self, output, FIXED_DX, fresult, 19);
		case F_DELETE:
			/* Don't allow the volume label to be set or changed */
			LOG(("DOSCALL: F_DELETE\n"));
			return ll_fs_error(self, output, LL_E_DENIED);
		case F_MAKE:
			/* Don't allow the volume label to be set or changed */
			LOG(("DOSCALL: F_MAKE\n"));
			return ll_fs_error(self, output, LL_E_DENIED);
		case F_RENAME:
			/* Don't allow the volume label to be set or changed */
			LOG(("DOSCALL: F_RENAME\n"));
			return ll_fs_error(self, output, LL_E_DENIED);
		case DOS_GETFREE:
			LOG(("DOSCALL: GETFREE\n"));
			if (self->protocol == LOCOLINK_V3 && (self->drive != 0 || (dx & 0xFF) > 1))
				return ll_fs_error(self, output, LL_E_NOTREADY);
			err = ll_dosfs_getfree(self->dosfs, 
					&ax, &bx, &cx, &dx);
			LOG(("DOSCALL: ll_dosfs_getfree returned %d\n", err));
			if (err)
			{
				self->state = IDLE;
				return ll_fs_error(self, output, LL_E_NOTFOUND);
			}
			/* Return an extended result packet giving free space */
			return ll_fs_longresult(self, NULL, output, ax, bx, cx, dx);
	
		case DOS_MKDIR:
			LOG(("DOSCALL: MKDIR\n"));
			if (self->drive != 0) return ll_fs_error(self, output, LL_E_NOTREADY);
			self->state = DOSX_MKDIR;
			/* Request the filename */
			return ll_fs_request(self, output, ASCIIZ_DX, 255);
		case DOS_CHDIR:
			LOG(("DOSCALL: CHDIR\n"));
			if (self->drive != 0) return ll_fs_error(self, output, LL_E_NOTREADY);
			self->state = DOSX_CHDIR;
			/* Request the filename */
			return ll_fs_request(self, output, ASCIIZ_DX, 255);
		case DOS_RMDIR:
			LOG(("DOSCALL: RMDIR\n"));
			if (self->drive != 0) return ll_fs_error(self, output, LL_E_NOTREADY);
			self->state = DOSX_RMDIR;
			/* Request the filename */
			return ll_fs_request(self, output, ASCIIZ_DX, 255);
		case DOS_CREAT:
			LOG(("DOSCALL: CREATE al=%02x\n", ax));
			if (self->drive != 0) return ll_fs_error(self, output, LL_E_NOTREADY);
			self->state = DOSX_CREATE;
			self->open_mode = ax;
			/* Request the filename */
			return ll_fs_request(self, output, ASCIIZ_DX, 255);
		case DOS_CCREAT:
			LOG(("DOSCALL: CCREAT al=%02x\n", ax));
			if (self->drive != 0) return ll_fs_error(self, output, LL_E_NOTREADY);
			self->state = DOSX_CCREAT;
			self->open_mode = ax;
			/* Request the filename */
			return ll_fs_request(self, output, ASCIIZ_DX, 255);
		case DOS_OPEN:
			LOG(("DOSCALL: OPEN al=%02x\n", ax));
			if (self->drive != 0) return ll_fs_error(self, output, LL_E_NOTREADY);
			self->state = DOSX_OPEN;
			self->open_mode = ax;
			/* Request the filename */
			return ll_fs_request(self, output, ASCIIZ_DX, 255);
		case DOS_CLOSE:
			LOG(("DOSCALL: CLOSE bx=%04x\n", bx));
			if (bx < 1 || bx > MAXFILEHANDLE)
			{
				return ll_fs_error(self, output, LL_E_BADHANDLE);
			}
			if (self->fp[bx - 1])
			{
				if (fclose(self->fp[bx - 1]))
				{
					self->fp[bx - 1] = NULL;
					return ll_fs_error(self, output, LL_E_SYSERR);
				}
				else
				{	
					self->fp[bx - 1] = NULL;
					return ll_fs_result(self,NULL,output, 0);
				}
			}
			else
			{
				return ll_fs_error(self, output, LL_E_BADHANDLE);
			}

		case DOS_READ:
			LOG(("DOSCALL: READ bx=%04x cx=%04x\n", bx, cx));
			if (self->drive != 0) return ll_fs_error(self, output, LL_E_NOTREADY);
			if (bx < 1 || bx > MAXFILEHANDLE)
			{
				return ll_fs_error(self, output, LL_E_BADHANDLE);
			}
			if (!self->fp[bx - 1])
			{
				return ll_fs_error(self, output, LL_E_BADHANDLE);
			}
			self->xfer_fp = self->fp[bx - 1];
			self->xfer_req = cx;
			self->xfer_bytes = 0;

			/* Vacuous */
			if (self->xfer_req == 0)
			{
				self->xfer_fp = NULL;
				return ll_fs_result(self, NULL, output, 0);
			}
			count = (cx < sizeof(readbuf)) ? cx : sizeof(readbuf);
			rcount = fread(readbuf, 1, count, self->xfer_fp);
			if (rcount < count)
			{
				self->xfer_req   = rcount;
			}			
			self->xfer_bytes = rcount;	
			self->state = DOSX_READ;
			return ll_fs_retdata(self, output, FIXED_DX, readbuf, rcount);
		case DOS_WRITE:
			LOG(("DOSCALL: WRITE bx=%04x cx=%04x\n", bx, cx));
			if (self->drive != 0) return ll_fs_error(self, output, LL_E_NOTREADY);
			if (bx < 1 || bx > MAXFILEHANDLE)
			{
				LOG(("WRITE fails: Handle out of range\n"));
				return ll_fs_error(self, output, LL_E_BADHANDLE);
			}
			if (!self->fp[bx - 1])
			{
				LOG(("WRITE fails: Handle not open\n"));
				return ll_fs_error(self, output, LL_E_BADHANDLE);
			}
			/* File is open read-only */
			if (!self->rwmode[bx - 1])
			{
				LOG(("WRITE fails: File is read-only\n"));
				return ll_fs_error(self, output, LL_E_DENIED);
			}
			self->xfer_fp = self->fp[bx - 1];
			self->xfer_req = cx;
			self->xfer_bytes = 0;

			/* Vacuous */
			if (self->xfer_req == 0)
			{
				self->xfer_fp = NULL;
				return ll_fs_result(self, NULL, output, 0);
			}
			/* Request the data */
			self->state = DOSX_WRITE;
			return ll_fs_request(self, output, FIXED_DX, 
					(cx < 254) ? cx : 254);

		case DOS_UNLINK:
			LOG(("DOSCALL: UNLINK\n"));
			if (self->drive != 0) return ll_fs_error(self, output, LL_E_NOTREADY);
			self->state = DOSX_UNLINK;
			/* Request the filename */
			return ll_fs_request(self, output, ASCIIZ_DX, 255);

		case DOS_LSEEK:
			if (self->drive != 0) return ll_fs_error(self, output, LL_E_NOTREADY);
			if (bx < 1 || bx > MAXFILEHANDLE)
			{
				return ll_fs_error(self, output, LL_E_BADHANDLE);
			}
			if (!self->fp[bx - 1])
			{
				return ll_fs_error(self, output, LL_E_BADHANDLE);
			}
			lpos = (((long)cx) << 16) | (dx & 0xFFFF);
			switch(ax & 0xFF)
			{
				case 0: lpos = fseek(self->fp[bx - 1], lpos, SEEK_SET); break;
				case 1: lpos = fseek(self->fp[bx - 1], lpos, SEEK_CUR); break;
				case 2: lpos = fseek(self->fp[bx - 1], lpos, SEEK_END); break;
				default: return ll_fs_error(self, output, LL_E_BADFUNC);
			}
			if (lpos < 0) return ll_fs_error(self, output, LL_E_SYSERR);
			lpos = ftell(self->fp[bx - 1]); 
			LOG(("lseek(%ld, %d) -> %ld\n", 
				(((long)cx) << 16) | (dx & 0xFFFF), ax & 0xFF,
				lpos));
			ax = (lpos & 0xFFFF);
			dx = (lpos >> 16);
			return ll_fs_longresult(self, NULL, output, ax, bx, cx, dx);
		case DOS_CHMOD:
			LOG(("DOSCALL: CHMOD\n"));
			if (self->drive != 0) return ll_fs_error(self, output, LL_E_NOTREADY);
			if ((ax & 0xFF) > 1) return ll_fs_error(self, output, LL_E_BADFUNC);
			self->state = DOSX_CHMOD;
			self->chmod_subfunc = ax & 0xFF;
			self->chmod_attrs = cx;
			/* Request the filename */
			return ll_fs_request(self, output, ASCIIZ_DX, 255);
		case DOS_IOCTL:
			if (self->drive != 0) return ll_fs_error(self, output, LL_E_NOTREADY);
			return ll_fs_longresult(self, NULL, output, 0, 0, 0, 0);

		case DOS_GETCWD:
			LOG(("DOSCALL: GETCWD\n"));
			if (self->drive != 0) return ll_fs_error(self, output, LL_E_NOTREADY);
			/* Return (dummy) data and wait for an ack */
			self->state = DOSX_ACKDATA;
			self->dos3_result = 0;
			return ll_fs_retdata(self, output, FIXED_DX, fresult, 0);
		case DOS_SFIRST:
			LOG(("DOSCALL: FINDFIRST al=%02x cx=%04x\n", ax, cx));
			if (self->drive != 0) return ll_fs_error(self, output, LL_E_NOTREADY);
			self->state = DOSX_FINDFIRST;
			self->ffirst_attrs = ax;
			/* Request the filename */
			return ll_fs_request(self, output, ASCIIZ_DX, 255);
		case DOS_SNEXT:
			LOG(("DOSCALL: FINDNEXT\n"));
			if (self->drive != 0) return ll_fs_error(self, output, LL_E_NOTREADY);
			if (self->protocol == LOCOLINK_V3)
			{
				unsigned id = cx;

				id = (id << 16) | (dx & 0xFFFF);
				err = ll_dosfs_findnext(self->dosfs, id, fresult);
			}
			else
			{
/* XXX LocoLink v2 also has a long-form search packet - need to support it too */
				err = ll_dosfs_findnext(self->dosfs, 0xFFFFFFFF, fresult);
			}
			if (err)
			{
				self->state = IDLE;
				return ll_fs_error(self, output, LL_E_NOTFOUND);
			}
/* LocoLink v2/v3 return FINDNEXT output in the result packet, not as a 
 * separate transfer */
			if (self->protocol == LOCOLINK_V2)
			{
				self->state = IDLE;
				output[0] = SUCCESS_V2;
				output[1] = 12 + strlen((char *)fresult + 0x1E);
				memcpy(output + 2, fresult + 0x12, output[1]);
				return LL_E_OK;	
			}
			else
			{
				self->state = IDLE;
				output[0] = SUCCESS_V3;
				output[1] = 13 + strlen((char *)fresult + 0x1E);
				memcpy(output + 2, fresult + 0x11, output[1]);
				return LL_E_OK;	
			}
			break;
		case DOS_RENAME:
			LOG(("DOSCALL: RENAME\n"));
			if (self->drive != 0) return ll_fs_error(self, output, LL_E_NOTREADY);
			self->state = DOSX_REN1;
			/* Request the first filename */
			return ll_fs_request(self, output, ASCIIZ_DX, 255);

		case DOS_FILETIME:
			LOG(("DOSCALL: FILETIME AL=%02x BX=%04x\n", ax & 0xFF, bx));
			if ((ax & 0xFF) > 1)
			{
				return ll_fs_error(self, output, LL_E_BADFUNC);
			}
			if (bx < 1 || bx > MAXFILEHANDLE)
			{
				return ll_fs_error(self, output, LL_E_BADHANDLE);
			}
			if (!self->fp[bx - 1])
			{
				return ll_fs_error(self, output, LL_E_BADHANDLE);
			}
			if ((ax & 0xFF) == 1)
			{
				err = ll_dosfs_set_ftime(self->dosfs, self->fp[bx - 1], dx, cx);
				if (err) return ll_fs_error(self, output, err);
				return ll_fs_result(self, NULL, output, 0);	
			}
			else
			{
				err = ll_dosfs_get_ftime(self->dosfs, self->fp[bx - 1], &dx, &cx);
				if (err) return ll_fs_error(self, output, err);
				return ll_fs_longresult(self, NULL, output, ax, bx, cx, dx);
			}
			break;
	}	
	LOG(("DOSCALL: Unknown function %02x\n", func));
	return ll_fs_error(self, output, LL_E_BADFUNC);
}


ll_error_t ll_fs_dispatch_x_findfirst(LL_PFS self, zbyte *input, zbyte *output)
{
	char filename[PATH_MAX];
	zbyte fresult[0x2C];
	ll_error_t err;

	if (input[0] != DATASEND_V2 && input[0] != DATASEND_V3)
	{
		self->state = IDLE;
		LOG(("DOSCALL: findfirst did not receive expected packet\n"));
		return ll_fs_error(self, output, LL_E_UNEXPECT);
	}
	if (get_filename(filename, input, 0))
	{
		self->state = IDLE;
		return ll_fs_error(self, output, LL_E_BADNAME);
	}
	err = ll_dosfs_findfirst(self->dosfs, filename, self->ffirst_attrs, fresult);
	if (err)
	{
		self->state = IDLE;
		return ll_fs_error(self, output, err);
	}
/* Return data and wait for an ack */
	self->state = DOSX_ACKDATA;
	self->dos3_result = 0;
	return ll_fs_retdata(self, output, FIXED_DTA, fresult, sizeof(fresult));
}


ll_error_t ll_fs_dispatch_x_open(LL_PFS self, zbyte *input, zbyte *output)
{
	char filename[PATH_MAX];
	ll_error_t err;
	int nh;

	if (input[0] != DATASEND_V2 && input[0] != DATASEND_V3)
	{
		self->state = IDLE;
		LOG(("DOSCALL: open did not receive expected packet\n"));
		return ll_fs_error(self, output, LL_E_UNEXPECT);
	}
	if (get_filename(filename, input, 0))
	{
		self->state = IDLE;
		return ll_fs_error(self, output, LL_E_BADNAME);
	}
	for (nh = 0; nh < MAXFILEHANDLE; nh++)
	{
		if (self->fp[nh] == NULL)
		{
			err = ll_dosfs_open(self->dosfs, filename, 
					self->open_mode, &self->fp[nh]);
			if (err)
			{
				self->rwmode[nh] = 0;
				self->state = IDLE;
				return ll_fs_error(self, output, err);
			}
			else
			{
				self->rwmode[nh] = self->open_mode;
				self->state = IDLE;
				return ll_fs_result(self, NULL, output, nh + 1);
			}
		}
	}
	return ll_fs_error(self, output, LL_E_TOOMANY);
}

ll_error_t ll_fs_dispatch_x_create(LL_PFS self, zbyte *input, zbyte *output)
{
	char filename[PATH_MAX];
	ll_error_t err;
	int nh;

	if (input[0] != DATASEND_V2 && input[0] != DATASEND_V3)
	{
		self->state = IDLE;
		LOG(("DOSCALL: create did not receive expected packet\n"));
		return ll_fs_error(self, output, LL_E_UNEXPECT);
	}
	if (get_filename(filename, input, 0))
	{
		self->state = IDLE;
		return ll_fs_error(self, output, LL_E_BADNAME);
	}
	for (nh = 0; nh < MAXFILEHANDLE; nh++)
	{
		if (self->fp[nh] == NULL)
		{
			err = ll_dosfs_creat(self->dosfs, filename, 
					self->open_mode, &self->fp[nh]);
			if (err)
			{
				self->rwmode[nh] = 0;
				self->state = IDLE;
				return ll_fs_error(self, output, err);
			}
			else
			{
				self->rwmode[nh] = 1;
				self->state = IDLE;
				return ll_fs_result(self, NULL, output, nh + 1);
			}
		}
	}
	return ll_fs_error(self, output, LL_E_TOOMANY);
}

ll_error_t ll_fs_dispatch_x_ccreat(LL_PFS self, zbyte *input, zbyte *output)
{
	char filename[PATH_MAX];
	ll_error_t err;
	int nh;

	if (input[0] != DATASEND_V2 && input[0] != DATASEND_V3)
	{
		self->state = IDLE;
		LOG(("DOSCALL: ccreate did not receive expected packet\n"));
		return ll_fs_error(self, output, LL_E_UNEXPECT);
	}
	if (get_filename(filename, input, 0))
	{
		self->state = IDLE;
		return ll_fs_error(self, output, LL_E_BADNAME);
	}
	for (nh = 0; nh < MAXFILEHANDLE; nh++)
	{
		if (self->fp[nh] == NULL)
		{
			err = ll_dosfs_ccreat(self->dosfs, filename, 
					self->open_mode, &self->fp[nh]);
			if (err)
			{
				self->rwmode[nh] = 0;
				self->state = IDLE;
				return ll_fs_error(self, output, err);
			}
			else
			{
				self->rwmode[nh] = 1;
				self->state = IDLE;
				return ll_fs_result(self, NULL, output, nh + 1);
			}
		}
	}
	return ll_fs_error(self, output, LL_E_TOOMANY);
}




ll_error_t ll_fs_dispatch_x_read(LL_PFS self, zbyte *input, zbyte *output)
{
	zbyte readbuf[0xFE];
	int count, rcount;

	if ((input[0] != ACK_V2 && input[0] != ACK_V3) ||
	    (input[2] != DATARET_V2 && input[2] != DATARET_V3))
	{
		self->state = IDLE;
		self->xfer_fp = NULL;
		LOG(("DOSCALL: read did not receive expected packet\n"));
		return ll_fs_error(self, output, LL_E_UNEXPECT);
	}
	if (!self->xfer_fp)
	{
		self->state = IDLE;
		return ll_fs_error(self, output, LL_E_BADHANDLE);
	}
	count = self->xfer_req - self->xfer_bytes;

	if (count <= 0)	/* No more to come */
	{
		self->state = IDLE;
		self->xfer_fp = NULL;
		return ll_fs_result(self, NULL, output, self->xfer_bytes);	
	}
	if (count > sizeof(readbuf)) count = sizeof(readbuf);

	rcount = fread(readbuf, 1, count, self->xfer_fp);
	if (rcount < count)
	{
		self->xfer_req  = self->xfer_bytes + rcount;
	}			
	self->xfer_bytes += rcount;	
	self->state = DOSX_READ;
	return ll_fs_retdata(self, output, NEXTADDR, readbuf, count);
}





ll_error_t ll_fs_dispatch_x_write(LL_PFS self, zbyte *input, zbyte *output)
{
	int remaining;
	size_t res;

	if (input[0] != DATASEND_V2 && input[0] != DATASEND_V3)
	{
		self->state = IDLE;
		self->xfer_fp = NULL;
		LOG(("DOSCALL: write did not receive expected packet\n"));
		return ll_fs_error(self, output, LL_E_UNEXPECT);
	}
	if (!self->xfer_fp)
	{
		LOG((" WRITE fails: Bad handle\n"));
		self->state = IDLE;
		return ll_fs_error(self, output, LL_E_BADHANDLE);
	}
	res = fwrite(input + 2, 1, input[1], self->xfer_fp);
	if (res < input[1])
	{
		LOG((" WRITE fails: Short write (expected %d, wrote %d) errno=%d %s\n", input[1], res, errno, strerror(errno)));
		self->state = IDLE;
		self->xfer_fp = NULL;
		return ll_fs_error(self, output, LL_E_SYSERR);
	}
	self->xfer_bytes += input[1];
	/* End of the request */
	if (self->xfer_bytes >= self->xfer_req)
	{
		self->state = IDLE;
		self->xfer_fp = NULL;
		return ll_fs_result(self, NULL, output, self->xfer_bytes);
	
	}
	/* More to come */
	remaining = self->xfer_req - self->xfer_bytes;
	self->state = DOSX_WRITE;
	return ll_fs_request(self, output, NEXTADDR, (remaining < 254) ? remaining : 254);
}


ll_error_t ll_fs_dispatch_x_mkdir(LL_PFS self, zbyte *input, zbyte *output)
{
	char filename[PATH_MAX];
	ll_error_t err;

	if (input[0] != DATASEND_V2 && input[0] != DATASEND_V3)
	{
		self->state = IDLE;
		LOG(("DOSCALL: mkdir did not receive expected packet\n"));
		return ll_fs_error(self, output, LL_E_UNEXPECT);
	}
	if (get_filename(filename, input, 0))
	{
		self->state = IDLE;
		return ll_fs_error(self, output, LL_E_BADNAME);
	}
	err = ll_dosfs_mkdir(self->dosfs, filename);
	if (err)
	{
		self->state = IDLE;
		return ll_fs_error(self, output, err);
	}
	else
	{
		self->state = IDLE;
		return ll_fs_result(self, NULL, output, 0);
	}
}


ll_error_t ll_fs_dispatch_x_rmdir(LL_PFS self, zbyte *input, zbyte *output)
{
	char filename[PATH_MAX];
	ll_error_t err;

	if (input[0] != DATASEND_V2 && input[0] != DATASEND_V3)
	{
		self->state = IDLE;
		LOG(("DOSCALL: rmdir did not receive expected packet\n"));
		return ll_fs_error(self, output, LL_E_UNEXPECT);
	}
	if (get_filename(filename, input, 0))
	{
		self->state = IDLE;
		return ll_fs_error(self, output, LL_E_BADNAME);
	}
	err = ll_dosfs_rmdir(self->dosfs, filename);
	if (err)
	{
		self->state = IDLE;
		return ll_fs_error(self, output, err);
	}
	else
	{
		self->state = IDLE;
		return ll_fs_result(self, NULL, output, 0);
	}
}


ll_error_t ll_fs_dispatch_x_chdir(LL_PFS self, zbyte *input, zbyte *output)
{
	char filename[PATH_MAX];
	ll_error_t err;

	if (input[0] != DATASEND_V2 && input[0] != DATASEND_V3)
	{
		self->state = IDLE;
		LOG(("DOSCALL: chdir did not receive expected packet\n"));
		return ll_fs_error(self, output, LL_E_UNEXPECT);
	}
	if (get_filename(filename, input, 0))
	{
		self->state = IDLE;
		return ll_fs_error(self, output, LL_E_BADNAME);
	}
	err = ll_dosfs_chdir(self->dosfs, filename);
	if (err)
	{
		self->state = IDLE;
		return ll_fs_error(self, output, err);
	}
	else
	{
		self->state = IDLE;
		return ll_fs_result(self, NULL, output, 0);
	}
}




ll_error_t ll_fs_dispatch_x_unlink(LL_PFS self, zbyte *input, zbyte *output)
{
	char filename[PATH_MAX];
	ll_error_t err;

	if (input[0] != DATASEND_V2 && input[0] != DATASEND_V3)
	{
		self->state = IDLE;
		LOG(("DOSCALL: unlink did not receive expected packet\n"));
		return ll_fs_error(self, output, LL_E_UNEXPECT);
	}
	if (get_filename(filename, input, 0))
	{
		self->state = IDLE;
		return ll_fs_error(self, output, LL_E_BADNAME);
	}
	err = ll_dosfs_unlink(self->dosfs, filename);
	if (err)
	{
		self->state = IDLE;
		return ll_fs_error(self, output, err);
	}
	else
	{
		self->state = IDLE;
		return ll_fs_result(self, NULL, output, 0);
	}
}


ll_error_t ll_fs_dispatch_x_chmod(LL_PFS self, zbyte *input, zbyte *output)
{
	char filename[PATH_MAX];
	ll_error_t err;
	unsigned short cx = 0;

	if (input[0] != DATASEND_V2 && input[0] != DATASEND_V3)
	{
		self->state = IDLE;
		LOG(("DOSCALL: chdir did not receive expected packet\n"));
		return ll_fs_error(self, output, LL_E_UNEXPECT);
	}
	if (get_filename(filename, input, 0))
	{
		self->state = IDLE;
		return ll_fs_error(self, output, LL_E_BADNAME);
	}
	if (self->chmod_subfunc == 0)
	{
		err = ll_dosfs_getattr(self->dosfs, filename, &cx);
	}
	else
	{
		err = ll_dosfs_setattr(self->dosfs, filename, self->chmod_attrs);
	}
	if (err)
	{
		self->state = IDLE;
		return ll_fs_error(self, output, err);
	}
	else
	{
		self->state = IDLE;
		return ll_fs_longresult(self, NULL, output, cx, 0, cx, 0);
	}
}


ll_error_t ll_fs_dispatch_x_ren1(LL_PFS self, zbyte *input, zbyte *output)
{
	if (input[0] != DATASEND_V2 && input[0] != DATASEND_V3)
	{
		self->state = IDLE;
		LOG(("DOSCALL: rename did not receive expected packet\n"));
		return ll_fs_error(self, output, LL_E_UNEXPECT);
	}
	if (get_filename(self->ren_oldname, input, 0))
	{
		self->state = IDLE;
		return ll_fs_error(self, output, LL_E_BADNAME);
	}
/* Request the second filename */
	self->state = DOSX_REN2;
	return ll_fs_request(self, output, ASCIIZ_DI, 255);
}


ll_error_t ll_fs_dispatch_x_ren2(LL_PFS self, zbyte *input, zbyte *output)
{
	ll_error_t err;
	char newname[PATH_MAX];

	if (input[0] != DATASEND_V2 && input[0] != DATASEND_V3)
	{
		self->state = IDLE;
		LOG(("DOSCALL: rename did not receive expected packet\n"));
		return ll_fs_error(self, output, LL_E_UNEXPECT);
	}
	if (get_filename(newname, input, 0))
	{
		self->state = IDLE;
		return ll_fs_error(self, output, LL_E_BADNAME);
	}
	err = ll_dosfs_rename(self->dosfs, self->ren_oldname, newname);
	
	if (err)
	{
		self->state = IDLE;
		return ll_fs_error(self, output, err);
	}
	else
	{
		self->state = IDLE;
		return ll_fs_result(self, NULL, output, 0);
	}
}


ll_error_t ll_fs_dispatch_ackdata(LL_PFS self, zbyte *input, zbyte *output)
{
/* Is this the Ack we were expecting? */
	switch (self->protocol)
	{
		case LOCOLINK_V2:
		if (input[0] != ACK_V2 || input[2] != DATARET_V2)
		{
			self->state = IDLE;
			LOG(("DOSCALL2: ackdata %02x %02x %02x did not receive expected packet\n", input[0], input[1], input[2]));
			return ll_fs_error(self, output, LL_E_UNEXPECT);
		}
		break;
		case LOCOLINK_V3:
		if (input[0] != ACK_V3 || input[2] != DATARET_V3)
		{
			self->state = IDLE;
			LOG(("DOSCALL3: ackdata %02x %02x %02x did not receive expected packet\n", input[0], input[1], input[2]));
			return ll_fs_error(self, output, LL_E_UNEXPECT);
		}
		break;
		default:
			return LL_E_BADPROTO;
	}
	self->state = IDLE;
	return ll_fs_result(self, NULL, output, self->dos3_result);
}


