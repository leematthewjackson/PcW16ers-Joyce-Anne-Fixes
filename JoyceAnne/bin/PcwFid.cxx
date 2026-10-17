/************************************************************************

    JOYCE v2.5.2 - Amstrad PCW emulator

    Copyright (C) 1996, 2001, 2023  John Elliott <seasip.webmaster@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*************************************************************************/

#include "Pcw.hxx"
#include "PcwFid.hxx"
#include "inline.hxx"

extern void gsx_cmd(zword pb);

static void fid_puts(Z80 *R, zword addr, const char *s)
{
        while (*s) R->storeB(addr++, *s++);
        R->storeB(addr++, 0x0D);
        R->storeB(addr++, 0x0A);
        R->storeB(addr++, 0xFF);
}


void fid_cems(Z80 *R)
{
/* This is called with: 

	A = 11
	B = flags, currently 0
	C = 5
	D = error:  0 => a device has been got
		    1 => hook failure, no more devices
		    2 => hook failure, device name in use
		    3 => version control error
	HL -> 93-byte message buffer

    returns: A=0 => ret NC
	     A=1 => ret C
*/

	switch (R->d)
	{
		case 3:			/* Version control */
		fid_puts(R, R->getHL(), "VGA.FID: Incorrect FID environment");
		R->a = 0;
		return;

		case 2:			/* Drive letter in use */
		fid_puts(R, R->getHL(), "VGA.FID: VGA driver already loaded");
		R->a = 0;
		return;

		case 1:			/* No free memory */
		fid_puts(R, R->getHL(), "VGA.FID: No character devices available");
		R->a = 0;
		return;

		case 0:			/* Device hooked OK */
		fid_puts(R, R->getHL(), "VGA.FID loaded OK");
		R->a = 1;
		return;
	}

}


int jfastcrt_doinit(void)
{
	int found = 0;
	unsigned scr_reset = 0;
	unsigned scr_blinkcount = 0;
	unsigned te_rvon = 0;
	unsigned te_attrs = 0;
	unsigned te_showcur = 0;
	unsigned te_showcursor = 0;
	unsigned msg_ric_xpos = 0;

//
// This is where we need to locate the host system's SCR and TE drivers and
// 'suck out their brains' as Raymond Chen might say.
//
// The TE data segment is the same in all versions of PCW CP/M except that 
// te_pagemode goes in before te_width in (roughly) 1.8+
//
// The SCR data segment similarly gains an (unused) 'right-to-left text' flag
// but otherwise all is the same.
//
// Note that this search may find two or more copies of the code, because 
// there may be copies of the EMS file in disk buffers. However the offsets
// in load and store instructions ought still to point to the right place.
//

// Firstly, however, check that there's a CP/M BIOS jumpblock present */
	for (int n = 0x1FC00; n <= 0x1FC60; n += 3)
	{
		if (PCWRAM[n] != 0xC3)
		{
// (we allow the BIOS to contain the JOYCE shortcuts ED FD) 
			if (PCWRAM[n] == 0xED && PCWRAM[n+1] == 0xFD) 
				continue;
			return 0;
		}
	}

	for (int n = 0; n < 0xC000; n++)
	{
/* To locate SCR, search for the function that initialises blink speed */
		if (PCWRAM[n] == 0x21 && PCWRAM[n+1] == 0x0A && 
			PCWRAM[n+2] == 0x0A && PCWRAM[n+3] == 0xCD &&
			PCWRAM[n+6] == 0xAF && PCWRAM[n+7] == 0x32)
		{
			scr_reset = n - 11;
			scr_blinkcount = PCWRAM[n+8] + 256 * PCWRAM[n+9];
			found |= 1;
		}
/* To locate TE, look for the reverse video on/off sequence */
		if (PCWRAM[n] == 0x01 && PCWRAM[n+1] == 0x04 && 
			PCWRAM[n+2] == 0xFB && PCWRAM[n+3] == 0x18 &&
			PCWRAM[n+4] == 0x03 && PCWRAM[n+5] == 0x01 &&
			PCWRAM[n+6] == 0x00 && PCWRAM[n+7] == 0xFB &&
			PCWRAM[n+8] == 0x21)
		{
			te_rvon = n;
			te_attrs = PCWRAM[n+9] + 256 * PCWRAM[n+10];
			found |= 2;
		}
/* Also look for the show cursor sequence */
		if (PCWRAM[n] == 0x2A && PCWRAM[n+3] == 0x7C && 
		    PCWRAM[n+4] == 0x2F && PCWRAM[n+5] == 0xA5 && 
		    PCWRAM[n+6] == 0xC8 && PCWRAM[n+7] == 0x32)
		{
			te_showcur = n;
			te_showcursor = PCWRAM[n+1] + 256 * PCWRAM[n+2];
			found |= 4;
		} 
/* To locate MSG, look for the code that draws Retry/Ignore/Cancel messages */
		if (PCWRAM[n] == 0x2A && PCWRAM[n+3] == 0x3A && 
		    PCWRAM[n+6] == 0x2C && PCWRAM[n+7] == 0xBD &&
		    PCWRAM[n+8] == 0xD0 && PCWRAM[n+9] == 0x54 &&
		    PCWRAM[n+10] == 0x5F && PCWRAM[n+11] == 0x3C && 	
		    PCWRAM[n+12] == 0x32)
		{
			msg_ric_xpos = PCWRAM[n+4] + 256 * PCWRAM[n+5];
			found |= 8;	
		}
	}

	if (found == 15 && gl_sys->m_fastCRT)
	{
		gl_sys->m_fastCRT->setVars(scr_blinkcount, te_attrs, te_showcursor, msg_ric_xpos);
		return 1;
	}
	return 0;
}


void jfastcrt_putchar(Z80 *R, int message)
{
	if (gl_sys->m_fastCRT)
	{
		if (message)	gl_sys->m_fastCRT->writeSys(R->e);
		else		gl_sys->m_fastCRT->write(R->e);
	}
}

void jfastcrt_init(Z80 *R)
{
/* This is called with: 

	A = 11
	B = flags, currently 0
	C = 8
	HL -> 93-byte message buffer

    returns: A=0 => ret NC, buffer contains error
	     A=1 => ret C
*/
	if (jfastcrt_doinit())
	{
		R->a = 1;
		fid_puts(R, R->getHL(), "Fast CRT driver: Compatible CP/M version");
	}
	else
	{
		fid_puts(R, R->getHL(), "Fast CRT driver: Incompatible CP/M version");
		R->a = 0;

	}
}



void jfastcrt_cems(Z80 *R)
{
/* This is called with: 

	A = 11
	B = flags, currently 0
	C = 7
	D = error:  0 => a device has been got
		    1 => hook failure, no more devices
		    2 => hook failure, device name in use
		    3 => version control error
	HL -> 93-byte message buffer

    returns: A=0 => ret NC
	     A=1 => ret C
*/

	switch (R->d)
	{
		case 3:			/* Version control */
		fid_puts(R, R->getHL(), "JFASTCRT.FID: Incorrect FID environment");
		R->a = 0;
		return;

		case 2:			/* Drive letter in use */
		fid_puts(R, R->getHL(), "JFASTCRT.FID: Driver already loaded");
		R->a = 0;
		return;

		case 1:			/* No free memory */
		fid_puts(R, R->getHL(), "JFASTCRT.FID: No character devices available");
		R->a = 0;
		return;

		case 4:			/* Device hooked OK replacing CRT */
		fid_puts(R, R->getHL(), "JOYCE fast CRT driver installed (as device CRT)");
		R->a = 1;
		return;

		case 0:			/* Device hooked OK as standalone */
		fid_puts(R, R->getHL(), "JOYCE fast CRT driver installed (as device UC1)");
		R->a = 1;
		return;
	}

}




void vga_capabilities(Z80 *R)	/* This function new in v1.20 */
{
	/* Sets the following bits in HL: */
	/* Bit 0: If 800x600 screen is available */
	/* Bit 1: If GSX is available */
	/* Bit 2: (v1.22) If LIOS is available */
	/* Bit 3: (v2.5.2+) If FASTCRT is available */
	/* Bit 4: (v2.5.5+) If libGSX features are available */
	
	R->h = 0;
	R->l = 0x1F;	/* All features */
	R->b = R->c = R->d = R->e = 0;
	R->ix = 0;
	R->iy = 0;
}


/* Character device handlers (called with A=0Bh, C=subfunction) */
void fid_char(Z80 *R)
{
	PcwVgaTerm &t = gl_sys->m_termVga;
	switch(R->c)
	{
		case 0: gl_sys->selectTerminal(gl_sys->m_defaultTerminal);
			break;
		case 1: gl_sys->selectTerminal(&gl_sys->m_termVga);
			break;
		case 2:	t.getVgaTerm() << R->e; 
			break;
		case 3: t.getVgaTerm().writeSys(R->e);  break;
/* v2.1.7: Reintroduce GSX */
 		case 4: gl_sys->m_termGSX.command(R->getDE()); break;
		case 5: fid_cems(R); break;
		case 6: vga_capabilities(R); break;	/* v1.20 */
		case 7: jfastcrt_cems(R); break;	/* v1.5.2 */
		case 8: jfastcrt_init(R); break;
		case 9: jfastcrt_putchar(R, 0); break; 
		case 10: jfastcrt_putchar(R, 1); break;
		case 11: R->a = gl_sys->m_termGSX.gsxlib(R->b, R->getDE(), R->getHL());
			break;
	}
}

/* FIDEMS for the COM port driver */

void com_ems(Z80 *R)
{
	char str[90];

/* This is called with: 

	A = 0Dh
	C = 7
	D = error:  0 => a device has been got
		    1 => hook failure, no more devices
		    2 => hook failure, device name in use
		    3 => version control error
	HL -> 93-byte message buffer

    returns: A=0 => ret NC
	     A=1 => ret C
*/
	switch (R->d)
	{
		case 3:			/* Version control */
		fid_puts(R, R->getHL(), "COMPORT.FID: Incorrect FID environment");
		R->a = 0;
		return;

		case 2:			/* Device name in use*/
		fid_puts(R, R->getHL(), "COMPORT.FID: A COM port driver is already loaded");
		R->a = 0;
		return;

		case 1:			/* No free memory */
		fid_puts(R, R->getHL(), "COMPORT.FID: No character devices available");
		R->a = 0;
		return;

		case 0:			/* Device hooked OK */
		sprintf(str,"COMPORT.FID is obsolete. Delete it. "); /* Using port COM%d", jset.comport + 1); */
		fid_puts(R, R->getHL(), str);
		R->a = 1;
		return;
	}
}
