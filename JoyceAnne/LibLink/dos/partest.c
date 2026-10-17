/* liblink: Simulate parallel / serial hardware for emulators
 
    Copyright (C) 2002, 2023  John Elliott <jce@seasip.demon.co.uk>

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

*****************************************************************************

This is a PCDOS-hosted equivalent of the LibLink PARTEST utility, to view 
and toggle data / control lines on a parallel port.

*/


#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include <ctype.h>
#include <string.h>

static unsigned lpt_port;			/* Printer port base */
static unsigned char data, status, ctrl;	/* Copies of LPT registers */
static unsigned vid_seg = 0xB800;		/* Video RAM base segment */
static unsigned char vid_mode;			/* Current video mode */
static unsigned char far *framebuf = MK_FP(0xB800, 0); /* Video RAM address */


/* Get current video mode */
unsigned char getmode()
{
	union REGS rg;

	rg.h.ah = 0x0F;
	int86(0x10, &rg, &rg);
	return rg.h.al;
}

/* Clear screen (by redoing mode set) */
void cls()
{
	union REGS rg;

	rg.x.ax = vid_mode;

	int86(0x10, &rg, &rg);
}

/* Write a string to video RAM with a given attribute */
void putstr(int row, int col, const char *s, int attr)
{
	unsigned addr = row * 160 + col * 2;

	while (*s)
	{
		framebuf[addr++] = *s++;
		framebuf[addr++] = attr;
	}
}


/* Output the binary breakdown of a byte */
void showbyte(char *buf, unsigned char val)
{
	sprintf(buf, " %d     %d     %d     %d     %d     %d     %d     %d",
			(val & 0x80) != 0,
			(val & 0x40) != 0,
			(val & 0x20) != 0,
			(val & 0x10) != 0,
			(val & 0x08) != 0,
			(val & 0x04) != 0,
			(val & 0x02) != 0,
			(val & 0x01) != 0);
}

/* Render a byte as a series of words */
void bytetext(char *buf, unsigned char val, char *txt)
{
	sprintf(buf, " %-5.5s %-5.5s %-5.5s %-5.5s %-5.5s %-5.5s %-5.5s %-5.5s",
			(val & 0x80) ? txt : "",
			(val & 0x40) ? txt+5 : "",
			(val & 0x20) ? txt+10 : "",
			(val & 0x10) ? txt+15 : "",
			(val & 0x08) ? txt+20 : "",
			(val & 0x04) ? txt+25 : "",
			(val & 0x02) ? txt+30 : "",
			(val & 0x01) ? txt+35 : "");
}

/* Show current status of data and control lines */
void showstatus()
{
	char buf[81];

	status = inportb(lpt_port + 1);
	ctrl   = inportb(lpt_port + 2);	

	showbyte(buf, data);
	putstr(6, 10, buf, 11);

	showbyte(buf, status);
	putstr(11, 10, buf, 11);
	bytetext(buf, status, "~BUSY~ACK PAPER SEL ~ERR ~IRQ ---- ----");
	putstr(12, 8, buf, 10);

	showbyte(buf, ctrl);
	putstr(16, 10, buf, 11);
	bytetext(buf, ctrl, "---- ---- BiDi ENIRQOSEL RESETAU.LFSTROB");
	putstr(17, 8, buf, 10);
}


void mainloop(void)
{
	char c;

	while (1)
	{
		showstatus();
		if (kbhit())
		{
			c = getch();
			if (islower(c)) c = toupper(c);
		
			switch (c)
			{
				case '7': data ^= 0x80; outportb(lpt_port, data); break;
				case '6': data ^= 0x40; outportb(lpt_port, data); break;
				case '5': data ^= 0x20; outportb(lpt_port, data); break;
				case '4': data ^= 0x10; outportb(lpt_port, data); break;
				case '3': data ^= 0x08; outportb(lpt_port, data); break;
				case '2': data ^= 0x04; outportb(lpt_port, data); break;
				case '1': data ^= 0x02; outportb(lpt_port, data); break;
				case '0': data ^= 0x01; outportb(lpt_port, data); break;
				case 'H': ctrl ^= 0x80; outportb(lpt_port + 2, ctrl); break;
				case 'G': ctrl ^= 0x40; outportb(lpt_port + 2, ctrl); break;
				case 'F': ctrl ^= 0x20; outportb(lpt_port + 2, ctrl); break;
				case 'E': ctrl ^= 0x10; outportb(lpt_port + 2, ctrl); break;
				case 'D': ctrl ^= 0x08; outportb(lpt_port + 2, ctrl); break;
				case 'C': ctrl ^= 0x04; outportb(lpt_port + 2, ctrl); break;
				case 'B': ctrl ^= 0x02; outportb(lpt_port + 2, ctrl); break;
				case 'A': ctrl ^= 0x01; outportb(lpt_port + 2, ctrl); break;

				case 'Q': return;
			}
		}
	}
}


int main(int argc, char **argv)
{
	int lpt = 1;
	unsigned far *bda = MK_FP(0x40, 8);
	char buf[81];
	
	vid_mode = getmode();
	if (vid_mode == 7)
	{
		vid_seg = 0xB000;
	}
	framebuf = MK_FP(vid_seg, 0);	
	if (argc >= 2)
	{
		lpt = atoi(argv[1]);
		if (lpt < 1 || lpt > 4)
		{
			fprintf(stderr, "LPT port must be 1-4\n");
			return 1;
		}
	}
	lpt_port = bda[lpt - 1];
	cls();
	data   = inportb(lpt_port);

	putstr(0, 30, "Parallel port tester", 9);
	sprintf(buf, "Using LPT%d at address 0x%04x", lpt, lpt_port);
	putstr(2, 0, buf, (vid_mode == 7) ? 7 : 12);
	putstr(5, 0, "Output:   D7    D6    D5    D4    D3    D2    D1    D0", (vid_mode == 7) ? 7 : 15);
	putstr(10, 0, "Input:    D7    D6    D5    D4    D3    D2    D1    D0",(vid_mode == 7) ? 7 : 15);
	putstr(15, 0, "Control:  D7    D6    D5    D4    D3    D2    D1    D0",(vid_mode == 7) ? 7 : 15);
	showstatus();

	sprintf(buf, "%-10.10s%-70.70s", 
		"", "7-0 toggle data bits. H-A toggle control bits. Q quits.");
	putstr(24, 0, buf, (vid_mode == 7) ? 0x70 : 0x31);

	mainloop();
	cls();
	return 0;	
}
