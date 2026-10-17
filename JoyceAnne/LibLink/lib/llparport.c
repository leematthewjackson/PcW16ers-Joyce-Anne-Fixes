/* liblink: Simulate parallel / serial hardware for emulators

    Copyright (C) 2002  John Elliott <seasip.webmaster@gmail.com>

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

#ifdef HAVE_LINUX_PPDEV_H

/* Directly drive a parallel port. 
 * This requires a Linux system that supports userspace parallel port drivers;
 * for example, Linux v2.4 with the CONFIG_PPDEV 
 * ("Support for user-space parallel port device drivers") option set.
 */

#include "linux/ppdev.h"
#include "linux/parport.h"

#ifdef MAJOR_IN_MKDEV
#include <sys/mkdev.h>
#endif
#ifdef MAJOR_IN_SYSMACROS
#include <sys/sysmacros.h>
#endif

#include "llparport.h"

LL_VTBL llv_parport =
{
	sizeof(LL_PARPORT),
	"parport",
	"Parallel port (Linux)",
	llpd_open,
	llpd_close,
	llpd_flush,
	llpd_send,
	NULL,	/* Fast transmit byte */
	NULL,	/* recv_poll */
	NULL,	/* recv_wait */
	llpd_sctl,
	llpd_rctl_poll,
	llpd_rctl_wait,
	llpd_sdctl,
};




/* Open connection */
ll_error_t llpd_open(LL_PDEV self)
{
	LL_PARPORT *trueself;
	struct stat st;
	int e, err;

	if (!self || self->lld_clazz != &llv_parport || 
	     self->lld_filename == NULL) return LL_E_BADPTR;
	trueself = (LL_PARPORT *)self;
	trueself->llpd_fd = -1;

	e = stat(self->lld_filename, &st);
	if (e) return LL_E_SYSERR;

        if (!S_ISCHR(st.st_mode)) return LL_E_NOTDEV;
        if (major(st.st_rdev) != 99) return LL_E_NOTDEV;
	trueself->llpd_fd = open(self->lld_filename, O_RDWR);
	if (trueself->llpd_fd == -1) return LL_E_SYSERR;

	err = LL_E_OK;
/*	if      (ioctl(trueself->llpd_fd, PPEXCL,  0)) err = LL_E_SYSERR;
	else */if (ioctl(trueself->llpd_fd, PPCLAIM, 0)) err = LL_E_SYSERR; 

	if (err) { close(trueself->llpd_fd); trueself->llpd_fd = -1; }		
	return err;
}


/* Close connection */
ll_error_t llpd_close(LL_PDEV self)
{
        LL_PARPORT *trueself;

        if (!self || self->lld_clazz != &llv_parport) return LL_E_BADPTR;
        trueself = (LL_PARPORT *)self;
        if (trueself->llpd_fd < 0) return LL_E_OK;

	ioctl(trueself->llpd_fd, PPRELEASE, 0);
	close(trueself->llpd_fd);	
	trueself->llpd_fd = -1;
	return LL_E_OK;
}


/* Flush data */
ll_error_t llpd_flush(LL_PDEV self)
{
	return LL_E_OK;
}


/* Sending data */
ll_error_t llpd_send(LL_PDEV self, unsigned char b)
{
        LL_PARPORT *trueself;

        if (!self || self->lld_clazz != &llv_parport) return LL_E_BADPTR;
        trueself = (LL_PARPORT *)self;
        if (trueself->llpd_fd < 0) return LL_E_CLOSED;

	if (ioctl(trueself->llpd_fd, PPWDATA, &b)) return LL_E_SYSERR;		

	return LL_E_OK;
}


/* Setting control lines. Twiddling STROBE sends the byte. */
ll_error_t llpd_sctl(LL_PDEV self, unsigned b)
{
        LL_PARPORT *trueself;
	unsigned char ch;

        if (!self || self->lld_clazz != &llv_parport) return LL_E_BADPTR;
        trueself = (LL_PARPORT *)self;
        if (!trueself->llpd_fd) return LL_E_CLOSED;

	ch = 0;
	if (b & LL_CTL_STROBE)  ch |= PARPORT_CONTROL_STROBE;
	if (b & LL_CTL_AUFEED)  ch |= PARPORT_CONTROL_AUTOFD;
	if (b & LL_CTL_OSELECT) ch |= PARPORT_CONTROL_SELECT;
	if (b & LL_CTL_INIT)    ch |= PARPORT_CONTROL_INIT;

	if (ioctl(trueself->llpd_fd, PPWCONTROL, &ch)) return LL_E_SYSERR;
	return LL_E_OK;
}

/* Can't change both data and control lines in one call, but this is the
 * closest we can get */
ll_error_t llpd_sdctl(LL_PDEV self, unsigned char data, unsigned ctl)
{
	ll_error_t e = llpd_send(self, data);
	if (e) return e;
	return llpd_sctl(self, ctl);
}


/* Reading control lines */
ll_error_t llpd_rctl_poll(LL_PDEV self, unsigned *b)
{
        LL_PARPORT *trueself;
        unsigned char ch;

        if (!self || self->lld_clazz != &llv_parport) return LL_E_BADPTR;
        trueself = (LL_PARPORT *)self;
        if (!trueself->llpd_fd) return LL_E_CLOSED;

        if (ioctl(trueself->llpd_fd, PPRSTATUS, &ch)) return LL_E_SYSERR;

	if (b)
	{
		*b = 0;
/* BUSY is active high on the wire, active low when read from the PC I/O 
 * port, so flip it back to active high.
 * ACK and ERROR are active low on the wire and remain active low when read 
 * from the PC I/O port, so flip them to active high as well.
 * SEL and PAPER OUT are active high on the wire and when read from the 
 * I/O port, so no need to change them.
 */

		if (!(ch & PARPORT_STATUS_ERROR))  *b |= LL_CTL_ERROR;
		if (ch & PARPORT_STATUS_SELECT)    *b |= LL_CTL_ISELECT;
		if (ch & PARPORT_STATUS_PAPEROUT)  *b |= LL_CTL_NOPAPER;
		if (!(ch & PARPORT_STATUS_BUSY))   *b |= LL_CTL_BUSY;
		if (!(ch & PARPORT_STATUS_ACK))    *b |= LL_CTL_ACK;
	}
        return LL_E_OK;
}
 

ll_error_t llpd_rctl_wait(LL_PDEV self, unsigned *b)
{
        LL_PARPORT *trueself;
	fd_set fds;
	int rv;
	struct timeval timeout = { 0 };

        if (!self || self->lld_clazz != &llv_parport) return LL_E_BADPTR;
        trueself = (LL_PARPORT *)self;
        if (!trueself->llpd_fd) return LL_E_CLOSED;

/* Unfortunately we can't just wait on the port, because changes to the 
 * control lines received from a real LocoLink does not cause select()
 * to return. So every 1ms, poll the lines to see if anything happened. */
	while (1)
	{
		FD_ZERO(&fds);
		FD_SET(trueself->llpd_fd, &fds);
		timeout.tv_sec = 0;
		timeout.tv_usec = 1000;
		rv = select(1, &fds, NULL, NULL, &timeout);	
		if (rv >= 0) return llpd_rctl_poll(self, b);
	}
	return LL_E_SYSERR;	
}

#endif // def HAVE_LINUX_PPDEV_H
