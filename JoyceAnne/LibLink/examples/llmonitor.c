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

  LLMONITOR is intended to sit between a LocoLink client and server, and 
  output their conversation.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"
#include "../lib/lltypes.h"	/* LocoLink packet types */

static LL_END end1 = LL_END_PCW;
static LL_END end2 = LL_END_PC;
// static char *port1   = "/dev/parport0";
// static char *driver1 = "parport";
// static char *port2   = "/dev/parport1";
// static char *driver2 = "parport";
static char *port1   = "S:locolink2.shmem";
static char *driver1 = "parshm";
// static char *port2   = "C:locolink.shmem";
// static char *driver2 = "parshm";
static char *port2 = "/home/jce/C/LibLink/examples/testdir";
static char *driver2 = "parllserv";
static LL_CABLE cable1 = LL_CABLE_LAPLINK;
static LL_CABLE cable2 = LL_CABLE_NORMAL; //LL_CABLE_LAPLINK;


void show_dosregs(const unsigned char *pkt, int result)
{
	if (result)
	{
		if (pkt[1] >= 2) printf("AX=%04x ", pkt[2] + 256 * pkt[3]);
	}
	else
	{
		if (pkt[1] >= 1) 
		{
			printf("Function %02x ", pkt[2]);
			switch (pkt[2])
			{
				case DRV_ALLRESET: printf("(DRV_ALLRESET) "); break;
				case DRV_SET:	   printf("(DRV_SET) "); break;
				case F_CLOSE:	   printf("(F_CLOSE) "); break;
				case F_SFIRST:	   printf("(F_SFIRST) "); break;
				case F_DELETE:	   printf("(F_DELETE) "); break;
				case F_MAKE:	   printf("(F_MAKE) "); break;
				case F_RENAME:	   printf("(F_RENAME) "); break;
				case DOS_GETFREE: printf("(DOS_GETFREE) "); break;
				case DOS_MKDIR:   printf("(DOS_MKDIR) "); break;
				case DOS_RMDIR:   printf("(DOS_RMDIR) "); break;
				case DOS_CHDIR:   printf("(DOS_CHDIR) "); break;
				case DOS_CREAT:   printf("(DOS_CREAT) "); break;
				case DOS_OPEN:    printf("(DOS_OPEN) "); break;
				case DOS_READ:    printf("(DOS_READ) "); break;
				case DOS_WRITE:   printf("(DOS_WRITE) "); break;
				case DOS_UNLINK:  printf("(DOS_UNLINK) "); break;
				case DOS_LSEEK:   printf("(DOS_LSEEK) "); break;
				case DOS_CHMOD:   printf("(DOS_CHMOD) "); break;
				case DOS_IOCTL:   printf("(DOS_IOCTL) "); break;
				case DOS_GETCWD:  printf("(DOS_GETCWD) "); break;
				case DOS_SFIRST:   printf("(DOS_SFIRST) "); break;
				case DOS_SNEXT:   printf("(DOS_SNEXT) "); 
						  if (pkt[1] > 1)
						  {
							printf("... search data ...");
						  }
						  return;
				case DOS_RENAME:  printf("(DOS_RENAME) "); break;
				case DOS_FILETIME:printf("(DOS_FILETIME) "); break;
				case DOS_CCREAT:  printf("(DOS_CCREAT) "); break;
			}
	
		}
		if (pkt[1] >= 2) printf("AL=%02x ", pkt[3]);
	}
	if (pkt[1] < 3) return;
	if (pkt[1] == 3) printf("BL=%02x ", pkt[4]);
	else printf("BX=%04x ", pkt[4] + 256 * pkt[5]);
	if (pkt[1] < 5) return;
	if (pkt[1] == 5) printf("CL=%02x ", pkt[6]);
	else printf("CX=%04x ", pkt[6] + 256 * pkt[7]);
	if (pkt[1] < 7) return;
	if (pkt[1] == 7) printf("DL=%02x ", pkt[8]);
	else printf("DX=%04x ", pkt[8] + 256 * pkt[9]);
}


ll_error_t callback(void *cookie, unsigned char *input, unsigned char *output)
{
	char *caption = (char *)cookie;
	int n;
	char buf[80];
	char tmp[20];
	int offset;

	memset(buf, ' ', 79);
	buf[79] = 0;
	sprintf(tmp, "%-4.4s:", caption); memcpy(buf, tmp, 5);
	for (n = 0; n < 4 + input[1]; n++)
	{
		offset = (n % 16);

		sprintf(tmp, "%02x", input[n]);
		memcpy(buf + 7 + 3 * offset, tmp, 2);
		if (isprint(input[n])) buf[56+offset] = input[n];
		else		       buf[56+offset] = '.'; 

		if (offset == 15) 
		{
			printf("%s\n", buf);	
			memset(buf, ' ', 79);
		}
	}
	if (offset != 15) 
	{
		printf("%s\n", buf);	
		memset(buf, ' ', 5);
	}

/* Human-readable packet dump */	
	switch (input[0])
	{
		case CONNECT_V1: 
			printf("       Connect (LocoLink 1.x)\n");
			break;
		case CONNECT_V2: 
			printf("       Connect (LocoLink 2.x) client=%-*.*s\n",
				input[1] - 2, input[1] - 2, input + 4);
			break;
		case CONNECT_V3: 
			printf("       Connect (LocoLink 3.x) client=%-*.*s\n",
				input[1] - 2, input[1] - 2, input + 4);
			break;
		case HANGUP_V1:
		case HANGUP_V2:
		case HANGUP_V3:
			printf("       Hangup\n");
			break;
		case SUCCESS_V2:
		case SUCCESS_V3:
			printf("       DOS call succeeded: ");
			show_dosregs(input, 1);
			putchar('\n');
			break;
		case DOSCALL_V2:
		case DOSCALL_V3:
			printf("       DOS call: ");
			show_dosregs(input, 0);
			putchar('\n');
			break;
		case ERROR_V1:
			printf("       Error %d\n", input[2]);
			break;
		case ERROR_V2:
		case ERROR_V3:
			printf("       Error %d\n", input[2] + 256 *input[3]);
			break;
		case ACK_V2:
		case ACK_V3:
			printf("       Acknowledge 0x%02x\n", input[2]);
			break;
		case DATARET_V2:
		case DATARET_V3:
			printf("       Data returned: ");
			switch (input[2])
			{
				case ASCIIZ_DX: printf("ASCIIZ at DX"); break;
				case ASCIIZ_SI: printf("ASCIIZ at SI"); break;
				case ASCIIZ_DI: printf("ASCIIZ at DI"); break;
				case FIXED_DX : printf("buffer at DX"); break;
				case FIXED_DTA: printf("buffer at DTA"); break;
				case FIXED_SI : printf("buffer at SI"); break;
				case NEXTADDR : printf("continue last"); break;
			}
			putchar('\n');
			break;	
		case DATAREQ_V2:
		case DATAREQ_V3:
			printf("       Data request: ");
			switch (input[2])
			{
				case ASCIIZ_DX: printf("ASCIIZ at DX"); break;
				case ASCIIZ_SI: printf("ASCIIZ at SI"); break;
				case ASCIIZ_DI: printf("ASCIIZ at DI"); break;
				case FIXED_DX : printf("buffer at DX"); break;
				case FIXED_DTA: printf("buffer at DTA"); break;
				case FIXED_SI : printf("buffer at SI"); break;
				case NEXTADDR : printf("continue last"); break;
			}
			printf(" max len=%d\n", input[3]);
			break;
		case DATASEND_V2:
		case DATASEND_V3:
			printf("       Data upload\n");
			break;

	}
	fflush(stdout);
	return LL_E_FAIL;	/* Reset status for the next packet */
}

/* Main loop. */
void monitor(LL_PDEV link1, LL_PDEV link2) 
{
	LL_PPLINK parser1, parser2;
	ll_error_t e;
	unsigned char v1, v2;
	int v1_old = -1, v2_old = -1;

	e = ll_parlink_create(&parser1, callback, "1->2");
	if (!e) e = ll_parlink_create(&parser2, callback, "2->1");

	if (e)
	{
		fprintf(stderr, "Could not create packet parsers: %s\n", 
			ll_strerror(e));
		return;
	}
	printf("Waiting for ends to go idle...\n"); fflush(stdout);
	v1 = 0; v2 = 0;
	while (1)
	{
		e = ll_lrecv_poll(link1, end1, &v1);
		if (!e) e = ll_lrecv_poll(link2, end2, &v2);

		if (v1 != v1_old || v2 != v2_old)
		{
			if (!e) e = ll_lsend(link1, end1, v2);	
			if (!e) e = ll_lsend(link2, end2, v1);	
// fprintf(stderr, "v1=%d v2=%d\n", v1, v2);
			v1_old = v1;
			v2_old = v2;
//			ll_parlink_write(parser1, v1);
//			ll_parlink_write(parser2, v2);
		}
		if (v1 >= 2 && v2 >= 2) break;	
	}
	ll_parlink_write(parser1, v1);
	ll_parlink_write(parser2, v2);

	printf("Monitoring...\n"); fflush(stdout);
	while (1)
	{
		e = ll_lrecv_poll(link1, end1, &v1);
		if (!e) 
		{
			if (v1 != v1_old)
			{
// fprintf(stderr, "New v1: %d\n", v1);
				v1_old = v1;
				ll_parlink_write(parser1, v1);
			}
			e = ll_lsend(link2, end2, v1);	
			
		}
		if (!e) e = ll_lrecv_poll(link2, end2, &v2);
		if (!e) 
		{
			if (v2 != v2_old)
			{
// fprintf(stderr, "New v2: %d\n", v2);
				v2_old = v2;
				ll_parlink_write(parser2, v2);
			}
			e = ll_lsend(link1, end1, v2);	
		}
		if (e)
		{
			fprintf(stderr, "Comms failed: %s\n", 
				ll_strerror(e));
			return;
		}
#ifdef HAVE_WINDOWS_H
		Sleep(1);
#else
		usleep(1);	
#endif
	}
}

int help(char **argv)
{
	fprintf(stderr, "Syntax: \n");
	fprintf(stderr, "    %s { -device1 <dev> } { -driver1 <drv> } { -laplink1 } { -pcw1 | -pc1 }\n"
                        "       { -device2 <dev> } { -driver2 <drv> } { -lablink2 } { -pcw2 | -pc2 }\n", argv[0]);
	fprintf(stderr, "\n");
	fprintf(stderr, "For example:\n");
	fprintf(stderr, "    %s -device1 /dev/parport0 -driver1 parport -end1 pcw -laplink1 \\\n"
                        "    -device2 /dev/parport1 -driver2 parport -end2 pc\n", argv[0]);
	fprintf(stderr, "       Monitors traffic between a PC on parport0 and a PCW on parport1.\n\n");

	fprintf(stderr, "    %s -device1 /dev/parport0 -driver1 parport -end1 pcw -laplink1 \\\n"
                        "    -device2 C:locolink.shmem -driver2 parshm -end2 pc\n", argv[0]);
	fprintf(stderr, "       Monitors traffic between a PC on parport0 and an emulated PCW using\n"
                        "       shared memory.\n\n");
// XXX enumerate drivers
	return 0;
}

int version(char **argv)
{
	fprintf(stderr, "liblink version %s\n", VERSION);
	return 0;
}


int main(int argc, char **argv)
{
	LL_PDEV link1, link2;
	ll_error_t e;
	int n;
/* Find out what device to open. */
	for (n = 1; n < argc; n++)
	{
		if (!strcmp(argv[n], "--help")) return help(argv);
		if (!strcmp(argv[n], "--version")) return version(argv);
		if (!strcmp(argv[n], "--laplink1")){ cable1 = LL_CABLE_LAPLINK; }
		if (!strcmp(argv[n], "-laplink1")) { cable1 = LL_CABLE_LAPLINK; }
		if (!strcmp(argv[n], "--laplink2")){ cable2 = LL_CABLE_LAPLINK; }
		if (!strcmp(argv[n], "-laplink2")) { cable2 = LL_CABLE_LAPLINK; }
		if (!strcmp(argv[n], "--pcw1")) { end1 = LL_END_PCW; }
		if (!strcmp(argv[n], "--pcw2")) { end2 = LL_END_PCW; }
		if (!strcmp(argv[n], "-pcw1"))  { end1 = LL_END_PCW; }
		if (!strcmp(argv[n], "-pcw2"))  { end2 = LL_END_PCW; }
		if (!strcmp(argv[n], "--pc1"))  { end1 = LL_END_PC; }
		if (!strcmp(argv[n], "--pc2"))  { end2 = LL_END_PC; }
		if (!strcmp(argv[n], "-pc1"))   { end1 = LL_END_PC; }
		if (!strcmp(argv[n], "-pc2"))   { end2 = LL_END_PC; }

		if (n == (argc - 1)) continue;	
		if (!strcmp(argv[n], "--driver1")) { ++n; driver1 = argv[n]; }
		if (!strcmp(argv[n], "-driver1"))  { ++n; driver1 = argv[n]; }
		if (!strcmp(argv[n], "--device1")) { ++n; port1   = argv[n]; }
		if (!strcmp(argv[n], "-device1"))  { ++n; port1   = argv[n]; }
		if (!strcmp(argv[n], "--driver2")) { ++n; driver2 = argv[n]; }
		if (!strcmp(argv[n], "-driver2"))  { ++n; driver2 = argv[n]; }
		if (!strcmp(argv[n], "--device2")) { ++n; port2   = argv[n]; }
		if (!strcmp(argv[n], "-device2"))  { ++n; port2   = argv[n]; }
	}

	printf("Drivers to use = %s\t%s\n", driver1, driver2);
	printf("Devices to use = %s\t%s\n", port1, port2);
	printf("Emulate Laplink cable: %s\t%s\n", cable1 ? "yes" : "no", 
						cable2 ? "yes" : "no");
	printf("Cable ends: %s\t%s\n",
			(end1 == LL_END_PC) ? "PC" : "PCW",
			(end2 == LL_END_PC) ? "PC" : "PCW");
/* Open the links */
	e = ll_open(&link1, port1, driver1);
	if (e)
	{
		fprintf(stderr, "Failed to open link 1: %s\n", ll_strerror(e));	
		return 1;
	}
	e = ll_open(&link2, port2, driver2);
	if (e)
	{
		fprintf(stderr, "Failed to open link 2: %s\n", ll_strerror(e));	
		ll_close(&link1);
		return 1;
	}
	e = ll_set_cable(link1, cable1);
	if (!e) e = ll_set_cable(link2, cable2);
	if (e)
	{
		fprintf(stderr, "Failed to set cable types: %s\n", ll_strerror(e));	
		ll_close(&link2);
		ll_close(&link1);
		return 1;
	}
	
/* Enter main loop */
	monitor(link1, link2);

/* Close the links */
	e = ll_close(&link2);
	if (e) fprintf(stderr, "Failed to close link 2: %s\n", ll_strerror(e));
	e = ll_close(&link1);
	if (e) fprintf(stderr, "Failed to close link 1: %s\n", ll_strerror(e));

	return 0;
}
