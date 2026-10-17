/* liblink: Simulate parallel / serial hardware for emulators

    Copyright (C) 2002, 2023-4  John Elliott <seasip.webmaster@gmail.com>

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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "liblink.h"
#include <limits.h>

#define END_OF_BLOCK	0x100
#define END_OF_TRANSFER 0x200


static LL_PDEV st_link;			/* The underlying link to the server */
static LL_PCWLINKIT st_pcwlinkit;


/* Display program syntax */
int syntax(char **argv)
{
	fprintf(stderr, "Syntax: \n");
	fprintf(stderr, "    %s { -device <dev> } { -driver <drv> } { -laplink } { -pcw }\n", argv[0]);
	fprintf(stderr, "\n");
	fprintf(stderr, "For example:\n");
	fprintf(stderr, "    %s -device unix:C:/tmp/linkit.socket -driver parsocket\n", argv[0]);
	fprintf(stderr, "       - Connect to an emulated PCW-Linkit server on /tmp/linkit.socket.\n\n");
	fprintf(stderr, "    %s -device unix:C:linkit.shmem -driver parshm\n", argv[0]);
	fprintf(stderr, "       - Connect to an emulated PCW-Linkit server using shared memory.\n\n");


// XXX enumerate drivers
	return 0;
}

/* Display program version */
int version(char **argv)
{
	fprintf(stderr, "liblink version %s\n", LIBLINK_VERSION);
	return 0;
}


static FILE *fp;

ll_error_t createfunc(void *cookie, const char *filename, unsigned long size)
{
	fp = fopen(filename, "wb");
	if (!fp) return LL_E_SYSERR;
	return LL_E_OK;
}


ll_error_t writefunc(void *cookie, unsigned char *data, unsigned long size)
{
	if (fwrite(data, 1, size, fp) < size) return LL_E_SYSERR;
	return LL_E_OK;
}


ll_error_t closefunc(void *cookie)
{
	if (fclose(fp)) return LL_E_SYSERR;
	fp = NULL;
	return LL_E_OK;
}


unsigned char buf[8388608];
unsigned long filesize;

ll_error_t loadfile(const char *filename)
{
	FILE *fp = fopen(filename, "rb");

	if (!fp) return LL_E_SYSERR;
	memset(buf, 0x1A, sizeof(buf));
	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	if (filesize >= 8388608)	/* File is too big to send */
	{
		fprintf(stderr, "File %s size is %lu\n", filename, filesize);
		fclose(fp);
		return LL_E_TOOBIG;
	}
	fseek(fp, 0, SEEK_SET);
	if (fread(buf, 1, filesize, fp) < filesize)
	{
		fclose(fp);
		return LL_E_SYSERR;
	}
	fclose(fp);
	return LL_E_OK;
}


int main(int argc, char **argv)
{
	ll_error_t e;
	int n;
	const char *driver = "parshm";
	const char *port   = "C:linkit.shmem";
	int nfirst = -1;

/* Find out what device to open. */
	for (n = 1; n < argc; n++)
	{
		if (!strcmp(argv[n], "--help")) return syntax(argv);
		if (!strcmp(argv[n], "--version")) return version(argv);
		if (n == (argc - 1)) 
		{
			if (argv[n][0] != '-' && nfirst < 0) nfirst = n;
			continue;
		}
		if (!strcmp(argv[n], "--driver")) { ++n; driver = argv[n]; }
		if (!strcmp(argv[n], "-driver"))  { ++n; driver = argv[n]; }
		if (!strcmp(argv[n], "--device")) { ++n; port   = argv[n]; }
		if (!strcmp(argv[n], "-device"))  { ++n; port   = argv[n]; }
		if (!strcmp(argv[n], "--")) 
		{
			nfirst = n + 1;
			break;
		}
		if (argv[n][0] != '-' && nfirst < 0) nfirst = n;
	}

	printf("Driver to use = %s\n", driver);
	printf("Device to use = %s\n", port);

	e = ll_open(&st_link, port, driver);
	if (e) 
	{
		const char *errmsg = ll_strerror(e);
		fprintf(stderr, "Error opening: %s\n", errmsg);
		return 1;
	}
	e = ll_linkit_create(&st_pcwlinkit, st_link);
	if (e) 
	{
		const char *errmsg = ll_strerror(e);
		fprintf(stderr, "Error opening: %s\n", errmsg);
		ll_close(&st_link);
		return 1;
	}
	printf("Connecting...\n");
	e = ll_linkit_connect(st_pcwlinkit);

	if (e) 
	{
		const char *errmsg = ll_strerror(e);
		fprintf(stderr, "Error connecting: %s\n", errmsg);
		ll_linkit_destroy(&st_pcwlinkit);
		ll_close(&st_link);
		return 1;
	}
	printf("... OK.\n");

	if (nfirst != -1)
	{
		sleep(1);
		e = ll_linkit_send_begin(st_pcwlinkit);
		if (!e) while (nfirst < argc)
		{
			printf("Sending: %s\n", argv[nfirst]); fflush(stdout);

			e = loadfile(argv[nfirst]);

			if (e) break;
			e = ll_linkit_send_file(st_pcwlinkit, argv[nfirst], buf, filesize, 0);
			++nfirst;
		}
		if (!e) e = ll_linkit_send_eot(st_pcwlinkit);
		if (e)
		{
			const char *errmsg = ll_strerror(e);
			fprintf(stderr, "Error sending: %s\n", errmsg);
		}
	}
	else
	{
		e = ll_linkit_receive(st_pcwlinkit, NULL, createfunc, writefunc, closefunc);
		if (e)
		{
			const char *errmsg = ll_strerror(e);
			fprintf(stderr, "Error receiving: %s\n", errmsg);
		}
	}
	ll_linkit_destroy(&st_pcwlinkit);
	ll_close(&st_link);
	if (e) 
	{
		return 1;
	}
	return 0;
}
