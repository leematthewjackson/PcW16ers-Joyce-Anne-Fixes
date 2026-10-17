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

/* Transmit bytes through a LocoLink link. The link can either be a genuine
 * LocoLink system, or a LapLink cable. 
 *
 * Starting with a stock LocoLink setup: The values written by the PCW are
 * perceived by the PC as follows:
 *
 *       On the PCW          |  In LibLink |       On the PC
 * PCW writes  Bit 1  Bit 0  | BUSY   ACK  | ~BUSY  ~ACK  PC reads
 * ==========================|=============|======================
 *          0      0      0  |    0    0   |     1     1  0C0h (3)
 *          1      0      1  |    1    0   |     0     1  040h (1)
 *          2      1      0  |    0    1   |     1     0  080h (2)
 *          3      1      1  |    1    1   |     0     0  000h (0)
 *
 * If instead the two ends are connected by a LapLink cable, then 
 * the BUSY and ~ACK lines on the wire are controlled by bits 3 and 4
 * written to the data port:
 *
 *   On the PCW-emulating PC |  In LibLink |       On the PC
 *    To send  Bit 3  Bit 4  | BUSY   ACK  | ~BUSY  ~ACK  PC reads
 * ==========================|=============|======================
 *          0      1      0  |    0    0   |     1     1  0C0h (3)
 *          1      1      1  |    1    0   |     0     1  040h (1)
 *          2      0      0  |    0    1   |     1     0  080h (2)
 *          3      0      1  |    1    1   |     0     0  000h (0)
 *
 * And data bits 1 and 0 written by the PC go to the SEL and ~FAULT
 * lines:
 *
 *        On the PC          |  In LibLink | On the PCW-emulating PC 
 *    To send  Bit 1  Bit 0  | SEL   ERROR |   SEL ~FAULT PC reads
 * ==========================|=============|========================
 *          0      0      0  |    0    1   |     0     0   00h (0)
 *          1      0      1  |    0    0   |     0     1   08h (1)
 *          2      1      0  |    1    1   |     1     0  010h (2)
 *          3      1      1  |    1    0   |     1     0  018h (3)
 * 
 */



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

static unsigned char unscramble(unsigned ctl, LL_END end)
{
	unsigned char data = 0;

	/* We are the PC. Have received data on the BUSY and ACK lines */
	if (LL_END_PC == end)
	{
		if (!(ctl & LL_CTL_BUSY)) data |= 1;
		if (ctl & LL_CTL_ACK)  data |= 2; 
		LOG(("PC Unscramble %s %s -> %d\n", 
			(ctl & LL_CTL_BUSY) ? "BUSY" : "    ",
			(ctl & LL_CTL_ACK)  ? "ACK " : "    ", data));
	}	
	else	/* We are the "PCW". Have received data on the ERROR and  */
	{	/* ISELECT lines [Verified with physical hardware] */
		if (!(ctl & LL_CTL_ERROR)) data |= 1;
		if (ctl & LL_CTL_ISELECT)  data |= 2;
		LOG(("PCW Unscramble %s %s -> %d\n", 
			(ctl & LL_CTL_ERROR)    ? "ERROR"   : "     ",
			(ctl & LL_CTL_ISELECT)  ? "ISELECT" : "       ", data));
	}
	return data;
}


/* Read LocoLink control lines*/
ll_error_t ll_lrecv_poll(LL_PDEV self, LL_END end, unsigned char *v)
{
	unsigned ctl;
	ll_error_t err;
	LL_CABLE cable;

	err = ll_get_cable(self, &cable);
	if (err) return err;
	/* If this is a true LocoLink PCW<-->PC, then the data will be on the
	 * actual data lines. */
	if (LL_CABLE_NORMAL == cable && LL_END_PCW == end)
	{
		err = ll_recv_poll(self, v);
		if (err) return err;
		if (v) *v &= 3;
		LOG(("ll_lrecv_poll v=%d\n", (*v) & 0xFF));
	}
	else	/* This is either the PC end of a true LocoLink, or an */
	{	/* emulation over a LapLink cable. Either way the information */
		/* required has to be parsed from the control lines. */
		err = ll_rctl_poll(self, &ctl);
		if (err) return err;
		if (v) *v = (unscramble(ctl, end) & 3);
		LOG(("ll_lrecv_poll ctl=%04x -> v=%d\n", ctl, *v));
	}
	return err;
}


/* Wait for control lines to change */
ll_error_t ll_lrecv_wait(LL_PDEV self, LL_END end, unsigned char *v)
{
	unsigned ctl;
	ll_error_t err;
	LL_CABLE cable;

	err = ll_get_cable(self, &cable);
	if (err) return err;
	/* If this is a true LocoLink PCW<-->PC, then the data will be on the
	 * actual data lines. */
	if (LL_CABLE_NORMAL == cable && LL_END_PCW == end)
	{
		err = ll_recv_wait(self, v);
		if (err) return err;
		LOG(("ll_lrecv_wait v=%d\n", (*v) & 0xFF));
		if (v) *v &= 3;
		return err;
	}	
	else	/* This is either the PC end of a true LocoLink, or an */
	{	/* emulation over a LapLink cable. Either way the information */
		/* required has to be parsed from the control lines. */
		err = ll_rctl_wait(self, &ctl);
		if (err) return err;
		if (v) *v = (unscramble(ctl, end) & 3);
		LOG(("ll_lrecv_wait ctl=%04x -> v=%d\n", ctl, *v));
		return err;
	}
}


/* Set LocoLink control lines */
ll_error_t ll_lsend(LL_PDEV self, LL_END end, unsigned char v)
{
	ll_error_t err = LL_E_OK;
	unsigned ctl = 0;
	unsigned char data = 0;

	LOG(("ll_lsend  end=%d v=%02x\n", end, v));
	if (LL_END_PC == end) 
	{
/* This end of the connection is the "PC". It has a LocoLink interface or 
 * LapLink cable plugged into its parallel port. 
 * Write the value to data lines 0,1, where LocoLink will expect to see it
 * Also mirror the value on ~FAULT and SEL, where a LapLink cable will 
 * send it to bits 0 and 1 */
		ctl |= LL_CTL_STROBE;

/* This combination of ERROR and ISELECT works on llparport between llclient
 * and JOYCE, and the data lines work on a real LocoLink interface. */
		if (!(v & 1)) ctl |= LL_CTL_ERROR;
		if (v & 2) ctl |= LL_CTL_ISELECT;
		err = ll_sdctl(self, v & 3, ctl | LL_CTL_STROBE);
		if (!err) err = ll_sdctl(self, v & 3, ctl &~ LL_CTL_STROBE);
		LOG(("ll_llsend PC %d -> ctl = %04x\n", v, ctl));
	}
	else	
	{
	/* This end of the connection is the "PCW". The value being transmitted
	 * is written to:
	 * - Data lines 0 and 1 (internal LocoLink server)
	 * - Data lines 3 and 4 (real or emulated LapLink cable to PC)
	 *			[Verified with physical hardware]
	 * - ACK and BUSY	(emulating LocoLink interface to PC) */

		if (!(v & 1)) { ctl |= LL_CTL_BUSY; data |= 16; } 
		if (v & 2) { ctl |= LL_CTL_ACK;} else { data |= 8; } 
		err = ll_sdctl(self, data | (v & 3), ctl);

		LOG(("ll_llsend PCW %d -> ctl = %04x data=%02x\n", v, ctl,
			data | (v & 3)));
	}
	return err;
}






