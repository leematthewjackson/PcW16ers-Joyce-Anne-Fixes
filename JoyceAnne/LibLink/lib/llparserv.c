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
#include "llparserv.h"
#include "lllocoserv.h"

LL_VTBL llv_parllserv =
{
	sizeof(LL_PARSERV),
	"parllserv",
	"Local LocoLink server (parallel port)",
	llpls_open,
	llpls_close,
	llpls_flush,
	llpls_send,
	llpls_send_byte,
	llpls_recv_poll,
	NULL,	/* recv_wait */
	llpls_sctl,
	llpls_rctl_poll,
	NULL,	/* No handy "sleep on control signal change" */
	llpls_sdctl,
	NULL,	/* Set cable type */
	NULL,	/* Get cable type */
	llpls_set_callback,
	llpls_get_callback
};



/* Open connection */
ll_error_t llpls_open(LL_PDEV self)
{
	LL_PARSERV *trueself;
	ll_error_t err;

	if (!self || self->lld_clazz != &llv_parllserv || 
	     self->lld_filename == NULL) return LL_E_BADPTR;
	trueself = (LL_PARSERV *)self;
/* Create the server */
	err = ll_fs_create(&trueself->llpls_server, self->lld_filename);
	if (err) return err;
/* And the packet parser */
	err = ll_parlink_create(&trueself->llpls_link, ll_fs_dispatch_v, 
				trueself->llpls_server);
	if (err)
	{
		ll_fs_close(&trueself->llpls_server);
		return err;
	}			
	return LL_E_OK;
}


/* Close connection */
ll_error_t llpls_close(LL_PDEV self)
{
        LL_PARSERV *trueself;
	int err;

        if (!self || self->lld_clazz != &llv_parllserv) return LL_E_BADPTR;
        trueself = (LL_PARSERV *)self;
        if (!trueself->llpls_server) return LL_E_CLOSED;
	err = ll_parlink_close(&trueself->llpls_link);
	if (err) return err;
	err = ll_fs_close(&trueself->llpls_server);
	return err;
}


/* Flush data */
ll_error_t llpls_flush(LL_PDEV self)
{
	return LL_E_OK;
}


/* Sending data */
ll_error_t llpls_send(LL_PDEV self, unsigned char b)
{
        LL_PARSERV *trueself;

        if (!self || (self->lld_clazz != &llv_parllserv)) return LL_E_BADPTR;
        trueself = (LL_PARSERV *)self;

	ll_parlink_write(trueself->llpls_link, b);

	return LL_E_OK;
}


/* Setting control lines. Twiddling STROBE sends the byte. */
ll_error_t llpls_sctl(LL_PDEV self, unsigned b)
{
	return LL_E_OK;
}

ll_error_t llpls_sdctl(LL_PDEV self, unsigned char data, unsigned ctl)
{
/*	fprintf(stderr, "llpls_sdctl  data=%d  ctl=%08x\n", data & 3, ctl); */
	return llpls_send(self, data);
}


/* Reading control lines */
ll_error_t llpls_rctl_poll(LL_PDEV self, unsigned *b)
{
	LL_PARSERV *trueself;
	unsigned char value;

	if (!self || (self->lld_clazz != &llv_parllserv)) return LL_E_BADPTR;
	if (!b) return LL_E_BADPTR;
	trueself = (LL_PARSERV *)self;

	*b = 0;
	ll_parlink_read(trueself->llpls_link, &value);
/* Set the control lines as if we are reading from either a LocoLink or a 
 * LapLink cable */

	if (value & 2) 
	{
		*b |= LL_CTL_ACK;
		*b |= LL_CTL_ISELECT;
	}
/* WHICH WAY ROUND IS BUSY?! */
/*	if (value & 1)
	{
		*b |= LL_CTL_BUSY;
		*b |= LL_CTL_ERROR;
	}*/
/* Seems to work with ANNE?  */
	if (!(value & 1))	  
	{
		*b |= LL_CTL_BUSY;
/* 20230327: Moved ERROR into here so it works with JOYCE internal server
	    and verified PCW Unscramble */
		*b |= LL_CTL_ERROR;
	}
	else
	{
	}
/**/	
//fprintf(stderr, "llparserv encode %d -> %08x\n", value, *b);
	return LL_E_OK;
}

/* Return the same info on the data lines as on the control lines */
ll_error_t llpls_recv_poll(LL_PDEV self, unsigned char *b)
{
	LL_PARSERV *trueself;
	unsigned char value;

	if (!self || (self->lld_clazz != &llv_parllserv)) return LL_E_BADPTR;
	if (!b) return LL_E_BADPTR;
	trueself = (LL_PARSERV *)self;

	ll_parlink_read(trueself->llpls_link, &value);
	*b = value;
	return LL_E_OK;
}
	

 

/* Sending a whole byte */
ll_error_t llpls_send_byte(LL_PDEV self, unsigned char b)
{
	LL_PARSERV *trueself;

	if (!self || (self->lld_clazz != &llv_parllserv)) return LL_E_BADPTR;

	trueself = (LL_PARSERV *)self;
	return ll_parlink_write(trueself->llpls_link, b);
}



ll_error_t llpls_set_callback(LL_PDEV self, LL_FSCALLBACK func, void *cookie)
{
	LL_PARSERV *trueself;

	if (!self || (self->lld_clazz != &llv_parllserv)) return LL_E_BADPTR;
	trueself = (LL_PARSERV *)self;
	
	return ll_fs_set_callback(trueself->llpls_server, func, cookie);
}

ll_error_t llpls_get_callback(LL_PDEV self, LL_FSCALLBACK *func, void **cookie)
{
	LL_PARSERV *trueself;

	if (!self || (self->lld_clazz != &llv_parllserv)) return LL_E_BADPTR;
	trueself = (LL_PARSERV *)self;
	
	return ll_fs_get_callback(trueself->llpls_server, func, cookie);
}


