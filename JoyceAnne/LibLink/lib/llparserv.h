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

/* Simulating a parallel port using a file */
typedef struct ll_parserv
{
	LL_DEVICE	llpls_super;
	LL_PPLINK	llpls_link;	
	LL_PFS		llpls_server;
} LL_PARSERV;


/* Open connection */
ll_error_t llpls_open(LL_PDEV self);
/* Close connection */
ll_error_t llpls_close(LL_PDEV self);
/* Flush data */
ll_error_t llpls_flush(LL_PDEV self);
/* Sending data */
ll_error_t llpls_send(LL_PDEV self, unsigned char b);
/* Sending data */
ll_error_t llpls_send_byte(LL_PDEV self, unsigned char b);
/* Setting control lines */
ll_error_t llpls_sctl(LL_PDEV self, unsigned b);
/* Setting data and control lines */
ll_error_t llpls_sdctl(LL_PDEV self, unsigned char data, unsigned ctl);
/* Reading data */
ll_error_t llpls_recv_poll(LL_PDEV self, unsigned char *b);
/* Reading control lines */
ll_error_t llpls_rctl_poll(LL_PDEV self, unsigned *b);
/* Set filesystem access callback */
ll_error_t llpls_set_callback(LL_PDEV self, LL_FSCALLBACK func, void *cookie);
/* Get filesystem access callback */
ll_error_t llpls_get_callback(LL_PDEV self, LL_FSCALLBACK *func, void **cookie);
 


