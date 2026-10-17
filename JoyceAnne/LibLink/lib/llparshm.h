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

/* Note that all members are the size of a machine word, hopefully to stop 
 * simultaneous writes to client and server trampling each other */
typedef struct llparshm_state
{
	unsigned data;		/* State of data lines */
	unsigned ctl;		/* State of control lines */
	unsigned tick;		/* Ticker */	
} LLPARSHM_STATE;

/* This is the area of memory shared between the two systems */
typedef struct llparshm_common
{	
	unsigned version;
	LLPARSHM_STATE client;
	LLPARSHM_STATE server;	
	LL_CABLE cable;
} LLPARSHM_COMMON;

#define LLPSHM_VERSION 1

/* Simulating a parallel port using an area of shared memory */
typedef struct ll_parshm
{
	LL_DEVICE	 llpshm_super;
	char		 llpshm_clserver;
#ifdef HAVE_WINDOWS_H
	HANDLE		llpshm_handle;
#else
	int		llpshm_handle;	/* Handle to the shared memory */
#endif
	LLPARSHM_COMMON *llpshm_common;
	LLPARSHM_STATE	*llpshm_local;
	LLPARSHM_STATE	*llpshm_remote;
} LL_PARSHM;


/* Open connection */
ll_error_t llpshm_open(LL_PDEV self);
/* Close connection */
ll_error_t llpshm_close(LL_PDEV self);
/* Flush data */
ll_error_t llpshm_flush(LL_PDEV self);
/* Sending data */
ll_error_t llpshm_send(LL_PDEV self, unsigned char b);
/* Setting control lines */
ll_error_t llpshm_sctl(LL_PDEV self, unsigned b);
/* Setting control+data lines */
ll_error_t llpshm_sdctl(LL_PDEV self, unsigned char data, unsigned ctl);
/* Reading control lines */
ll_error_t llpshm_rctl_poll(LL_PDEV self, unsigned *b);
/* Reading control lines */
ll_error_t llpshm_rctl_wait(LL_PDEV self, unsigned *b);
/* Reading data lines */
ll_error_t llpshm_recv_poll(LL_PDEV self, unsigned char *b);
/* Reading data lines */
ll_error_t llpshm_recv_wait(LL_PDEV self, unsigned char *b);
/* Setting cable type */ 
ll_error_t llpshm_set_cable(LL_PDEV self, LL_CABLE cable);
/* Getting cable type */ 
ll_error_t llpshm_get_cable(LL_PDEV self, LL_CABLE *cable);


