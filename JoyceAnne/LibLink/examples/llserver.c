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

    This exposes the internal LocoLink server over a parallel port.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"
#include "../lib/lltypes.h"	/* LocoLink packet types */

static LL_END end1 = LL_END_PCW;
static LL_END end2 = LL_END_PC;
static char *port1   = "/dev/parport0";
static char *driver1 = "parport";
//static char *port1   = "S:locolink.shmem";
//static char *driver1 = "parshm";
static char *port2   = "/tmp";
static char *driver2 = "parllserv";
static LL_CABLE cable1 = LL_CABLE_NORMAL;
static LL_CABLE cable2 = LL_CABLE_NORMAL; //LL_CABLE_LAPLINK;


ll_error_t callback(void *cookie, unsigned char *input, unsigned char *output)
{
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
			printf("       Client disconnected.\n");
			break;

	}
	fflush(stdout);
	return LL_E_FAIL;
}

/* Main loop. */
void monitor(LL_PDEV link1, LL_PDEV link2) 
{
	LL_PPLINK parser;
	ll_error_t e;
	unsigned char v1, v2;
	int v1_old = -1, v2_old = -1;

	e = ll_parlink_create(&parser, callback, NULL);

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
	ll_parlink_write(parser, v1);
			v1_old = v1;
			v2_old = v2;
		}
		if (v1 >= 2 && v2 >= 2) break;	
	}
	ll_parlink_write(parser, v1);

	printf("Server active\n"); fflush(stdout);
	while (1)
	{
		e = ll_lrecv_poll(link1, end1, &v1);
		if (!e) 
		{
			if (v1 != v1_old)
			{
// fprintf(stderr, "New v1: %d\n", v1);
				v1_old = v1;
				ll_parlink_write(parser, v1);
			}
			e = ll_lsend(link2, end2, v1);	
			
		}
		if (!e) e = ll_lrecv_poll(link2, end2, &v2);
		if (!e) 
		{
			if (v2 != v2_old)
			{
				v2_old = v2;
// fprintf(stderr, "New v2: %d\n", v2);
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
	fprintf(stderr, "    %s directory { -device <dev> } { -driver <drv> } { -laplink } { -pcw | -pc }\n", argv[0]);
	fprintf(stderr, "\n");
	fprintf(stderr, "For example:\n");
	fprintf(stderr, "    %s /home/user/pcwshare -device /dev/parport0 -driver parport -end pcw -laplink \n", argv[0]);
	fprintf(stderr, "       Shares /home/user/pcwshare to a PC connected by a LapLink cable to\n"
			"       parallel port 0.\n\n");

	fprintf(stderr, "    %s /tmp -device S:locolink.shmem -driver parshm -end pcw \n", argv[0]);
	fprintf(stderr, "       Shares /tmp to an emulated PC or PcW16 using shared memory.\n");
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

	if (argc < 2) return help(argv);

	port2 = argv[1];	/* Directory to share */	
/* Find out what device to open. */
	for (n = 2; n < argc; n++)
	{
		if (!strcmp(argv[n], "--help")) return help(argv);
		if (!strcmp(argv[n], "--version")) return version(argv);
		if (!strcmp(argv[n], "--laplink")){ cable1 = LL_CABLE_LAPLINK; }
		if (!strcmp(argv[n], "-laplink")) { cable1 = LL_CABLE_LAPLINK; }
		if (!strcmp(argv[n], "--pcw")) { end1 = LL_END_PCW; }
		if (!strcmp(argv[n], "-pcw"))  { end1 = LL_END_PCW; }
		if (!strcmp(argv[n], "--pc"))  { end1 = LL_END_PC; }
		if (!strcmp(argv[n], "-pc"))   { end1 = LL_END_PC; }

		if (n == (argc - 1)) continue;	
		if (!strcmp(argv[n], "--driver")) { ++n; driver1 = argv[n]; }
		if (!strcmp(argv[n], "-driver"))  { ++n; driver1 = argv[n]; }
		if (!strcmp(argv[n], "--device")) { ++n; port1   = argv[n]; }
		if (!strcmp(argv[n], "-device"))  { ++n; port1   = argv[n]; }
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
