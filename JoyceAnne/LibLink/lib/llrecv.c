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





/* Read data lines. */
ll_error_t ll_recv_poll(LL_PDEV self, unsigned char *value)
{
	LL_CABLE cable;
	ll_error_t err;
	
	err = ll_get_cable(self, &cable);
	if (err) return err;

	err = ll_irecv_poll(self,value);
	if (err) return err;

	if (self->lld_data0 != *value) LOG(("ll_recv_poll (%p): data=%02x\n", 
		self, *value));
	self->lld_data0 = *value;
	return LL_E_OK;
}

ll_error_t ll_recv_wait(LL_PDEV self, unsigned char *value)
{
	LL_CABLE cable;
	ll_error_t err;
	
	err = ll_get_cable(self, &cable);
	if (err) return err;

	err = ll_irecv_wait(self,value);
	if (err) return err;
LOG(("ll_recv_wait: data=%02x\n", *value));
	return LL_E_OK;
}



/* Read control lines. In LapLink mode, five control lines are driven 
 * from the last byte read from the data lines. Not all drivers implement
 * reading the data lines, so be prepared for the ll_irecv() to fail. */
ll_error_t ll_rctl_poll(LL_PDEV self, unsigned *value)
{
	unsigned char data = 0;
	ll_error_t err;
	LL_CABLE cable;
	
	err = ll_get_cable(self, &cable);
	if (err) return err;

	err = ll_irctl_poll(self,value);
	if (err) return err;

if (self->lld_ctrl0 != *value) LOG(("ll_rctl_poll(%p): control=%08x\n", 
	self, *value));
self->lld_ctrl0 = *value;

	switch(cable)
	{
		case LL_CABLE_LAPLINK:
		err = ll_irecv_poll(self, &data);

		if (err != LL_E_OK && err != LL_E_NOTIMPL) return err;	
{ static int d0 = -1;
if (d0 != *value) LOG(("ll_rctl_poll: data   =%02x\n", data));
d0 = *value; }
		if (err == LL_E_OK) ll_laplink_setctl(data, value);
{ static int v0 = -1;
if (v0 != *value) LOG(("ll_rctl_poll: result =%08x\n", *value));
v0 = *value; }
		break;
	
		case LL_CABLE_NORMAL:
		break;
	}
	return LL_E_OK;
}


/* Read control lines. In LapLink mode, five control lines are driven 
 * from the last byte read from the data lines. Not all drivers implement
 * reading the data lines, so be prepared for the ll_irecv() to fail. */
ll_error_t ll_rctl_wait(LL_PDEV self, unsigned *value)
{
	unsigned char data = 0;
	ll_error_t err;
	LL_CABLE cable;
	
	err = ll_get_cable(self, &cable);
	if (err) return err;

	err = ll_irctl_wait(self,value);
	if (err) return err;
LOG(("ll_rctl_wait: control=%08x\n", *value));
	switch(cable)
	{
		case LL_CABLE_LAPLINK:
		err = ll_irecv_poll(self, &data);

		if (err != LL_E_OK && err != LL_E_NOTIMPL) return err;	
LOG(("ll_rctl_wait: data   =%02x\n", data));
		if (err == LL_E_OK) ll_laplink_setctl(data, value);
LOG(("ll_rctl_wait: result =%08x\n", *value));
		break;
	
		case LL_CABLE_NORMAL:
		break;
	}
	return LL_E_OK;
}





