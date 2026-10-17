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

/* Get last value written to control lines */
ll_error_t ll_sctl_get(LL_PDEV self, unsigned *ctl)
{
	if (!self) return LL_E_BADPTR;
	if (ctl) *ctl = self->lld_lastsctl;
	return LL_E_OK;
}

/* Get last value written to data lines */
ll_error_t ll_sdata_get(LL_PDEV self, unsigned char *data)
{
	if (!self) return LL_E_BADPTR;
	if (data) *data = self->lld_lastsdata;
	return LL_E_OK;
}

/* Write a new value. 
 * If we are in LapLink mode, then data output additionally sets the other 
 * end's control lines.  */

ll_error_t ll_send(LL_PDEV self, unsigned char value)
{
	unsigned ctrl = 0;
	ll_error_t err = LL_E_OK;
	LL_CABLE cable;

	if (!self) return LL_E_BADPTR;
	
	err = ll_get_cable(self, &cable);
	if (err) return err;

	self->lld_lastsdata = value;

	ctrl = self->lld_lastsctl;
	switch(cable)
	{
		case LL_CABLE_LAPLINK:	
/* LapLink: Echo data bits to the control lines. */
			ll_laplink_setctl(value, &ctrl);
			self->lld_lastsctl = ctrl;
			return ll_isdctl(self, value, ctrl);
		case LL_CABLE_NORMAL:
			err = ll_isend(self, value);
			if (err) return err;
			break;
	}
	return LL_E_OK;
}


/* Set control and data lines. */
ll_error_t ll_sdctl(LL_PDEV self, unsigned char data, unsigned ctrl)
{
	ll_error_t err = LL_E_OK;
	LL_CABLE cable;

	if (!self) return LL_E_BADPTR;
	
	err = ll_get_cable(self, &cable);
	if (err) return err;

	switch(cable)
	{
		case LL_CABLE_LAPLINK:	
/* LapLink: Echo data bits to the control lines. */
			ll_laplink_setctl(data, &ctrl);
			self->lld_lastsctl = ctrl;
			break;
		case LL_CABLE_NORMAL:
			break;
	}
	self->lld_lastsdata = data;
	self->lld_lastsctl = ctrl;
	return ll_isdctl(self, data, ctrl);
}



/* Set control lines. */
ll_error_t ll_sctl(LL_PDEV self, unsigned ctrl)
{
	ll_error_t err = LL_E_OK;
	LL_CABLE cable;

	if (!self) return LL_E_BADPTR;
	
	err = ll_get_cable(self, &cable);
	if (err) return err;

	switch(cable)
	{
		case LL_CABLE_LAPLINK:	
/* LapLink: Echo data bits to the control lines. */
			ll_laplink_setctl(self->lld_lastsdata, &ctrl);
			self->lld_lastsctl = ctrl;
			break;
		case LL_CABLE_NORMAL:
			self->lld_lastsctl = ctrl;
			return ll_isctl(self, ctrl);
	}
	return ll_isctl(self, ctrl);
}


/* Send a byte to a device */
ll_error_t ll_send_byte(LL_PDEV self, unsigned char value)
{
	ll_error_t err;
        if (!self || !self->lld_clazz) return LL_E_BADPTR;

        if (self->lld_clazz->llv_send_byte)
        {
                return (*self->lld_clazz->llv_send_byte)(self, value);
        }
	/* Here is a default implementation. It sets the data, then
	 * strobes the port, with delays that should work for most printers. */

	err = ll_send(self, value); if (err) return err;
	microsleep(1);	// sleep 1uS 
	err = ll_sctl(self, self->lld_lastsctl | LL_CTL_STROBE); if (err) return err;
	microsleep(1);	// sleep 1uS 
	err = ll_sctl(self, self->lld_lastsctl &~ LL_CTL_STROBE); if (err) return err;
	microsleep(1);  // sleep 1uS
	return LL_E_OK;
}


void microsleep(int ms)
{
#ifdef HAVE_WINDOWS_H
	int XXX_how_is_this_done_in_windows;
	Sleep((ms + 999) / 1000);
#else
	usleep(ms);
#endif
}

