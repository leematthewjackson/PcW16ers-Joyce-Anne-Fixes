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

/* Set LapLink mode. This emulates the effect of a LapLink cable 
 *  * between two devices. */
ll_error_t ll_set_cable(LL_PDEV self, LL_CABLE cable) 
{
	if (!self || !self->lld_clazz) return LL_E_BADPTR;
	
	if (self->lld_clazz->llv_set_cable)
		return (*self->lld_clazz->llv_set_cable)(self, cable);

	self->lld_cable = cable;
	return LL_E_OK;
}

/* Get LapLink setting */
ll_error_t ll_get_cable(LL_PDEV self, LL_CABLE *cable)
{
	if (!self || !self->lld_clazz || !cable) return LL_E_BADPTR;
	
	if (self->lld_clazz->llv_get_cable)
		return (*self->lld_clazz->llv_get_cable)(self, cable);

	*cable = self->lld_cable;
	return LL_E_OK;
}


/* If we have received 'data' through a LapLink cable, set / clear the 
 * appropriate bits in 'ctl' */
void ll_laplink_setctl(unsigned char data, unsigned *control)
{
	if (!control) return;
	
/* FAULT is active low on the wire, active high within LibLink */
	if (data &  1) *control &= ~LL_CTL_ERROR; else *control |= LL_CTL_ERROR;
/* SEL is active high */
	if (data &  2) *control |= LL_CTL_ISELECT; else *control &= ~LL_CTL_ISELECT;
/* PAPER OUT is active high */
	if (data &  4) *control |= LL_CTL_NOPAPER; else *control &= ~LL_CTL_NOPAPER;
/* ACK is active low on the wire, active high within LibLink */
	if (data &  8) *control &= ~LL_CTL_ACK; else *control |= LL_CTL_ACK;
/* BUSY is active high */
	if (data & 16) *control |= LL_CTL_BUSY; else *control &= ~LL_CTL_BUSY;
}






