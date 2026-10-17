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

/* This is an example client program with an ftp-like interface. Its 
 * intended use is to access LocoLink servers (running on real PCWs or 
 * under emulation) for the purposes of testing their behaviour.
 *
 * There are a number of places where the implementation is not particularly
 * rigorous. For example, it is possible to set the current directory to a
 * path that does not exist. Use this on live data at your own risk!
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "liblink.h"
#include <limits.h>

const char *client_version = "LibLink client " LIBLINK_VERSION;

LL_PREMOTE remote;		/* The RPC wrapper for the remote server */
LL_PDEV link;			/* The underlying link to the server */
static char st_drive = 'Z';	/* Drive letter in use on the server */
static char st_path[PATH_MAX];	/* Current server directory */
static int  st_mode;		/* Disk mode (CP/M or LocoScript) */
static LL_PROTOCOL st_protocol = NO_PROTOCOL;	/* Active protocol */
static LL_END st_end = LL_END_PC;	/* Default to being the PC */

/* This program does not use the "Get CWD / Set CWD" calls to change directory
 * on the server. (As far as I can see, the value set by 'Set CWD' is returned
 * by 'Get CWD' but otherwise unused, at least within the LocoLink 2 server.
 *
 * Therefore, this program has to maintain the "CWD" locally and convert all 
 * filenames into absolute pathnames before handing them to the server. This 
 * matches the behaviour of the known PCW LocoLink clients. */

void do_reset();		/* Forward declaration */

/* DOS errors per Ralf Brown Interrupt List, INT 21h/AH=59h */
static const char *dos_err[] =
{
	"No error",				/*  0 */
	"Function number invalid",		/*  1 */
	"File not found",			/*  2 */
	"Path not found",			/*  3 */
	"Too many open files",			/*  4 */
	"Access denied (try reset)",		/*  5 */
	"Invalid handle",			/*  6 */
	"Memory control block destroyed",	/*  7 */
 	"Insufficient memory",			/*  8 */
	"Memory block address invalid",		/*  9 */
	"Environment invalid",			/* 10 */
	"Format invalid",			/* 11 */
	"Access code invalid",			/* 12 */
	"Data invalid",				/* 13 */
	"Fixup overflow",			/* 14 */
	"Invalid drive",			/* 15 */
	"Attempted to remove current directory",/* 16 */
	"Not same device",			/* 17 */
	"No more files",			/* 18 */
	"Disk write-protected",			/* 19 */
	"Unknown unit",				/* 20 */
	"Drive not ready",			/* 21 */
	"Unknown command",			/* 22 */
	"CRC data error",			/* 23 */
	"Bad request structure length",		/* 24 */
	"Seek error",				/* 25 */
	"Unknown media type",			/* 26 */
	"Sector not found",			/* 27 */
	"Printer out of paper",			/* 28 */
	"Write fault",				/* 29 */
	"Read fault",				/* 30 */
	"General failure",			/* 31 */
	"Sharing violation",			/* 32 */
	"Lock violation",			/* 33 */
	"Disk change invalid",			/* 34 */
	"FCB unavailable",			/* 35 */	
	"Sharing buffer overflow",		/* 36 */
	"Code page mismatch",			/* 37 */
	"Cannot complete file operation",	/* 38 */
	"Insufficient disk space",		/* 39 */
};

#define DOS_ERR_COUNT (sizeof(dos_err) / sizeof(dos_err[0]))

/* LocoScript file types */
static const char *loco_ids[] = 
{
	"JOY document",
	"DOC PC document",
	"BMP scalable font",
	"CHR printer font",
	"CMB dot-matrix printer driver",
	"DMN Disc Manager data",
	"DRV driver",
	"EDC spellchecker dictionary",
	"HLP help file",
	"KBD keyboard layout",
	"KNO settings",
	"OML mail merge overlay",
	"OSP spell check overlay",
	"OVL overlay",
	"PHR phrase list",
	"PRI printer driver",
	"SCR screen characters",
	"SDC spellchecker dictionary",
	"UDC spellchecker dictionary",
	"XCH scalable font"
};
#define LOCO_ID_COUNT (sizeof(loco_ids) / sizeof(loco_ids[0]))

 

/* Display program syntax */
int syntax(char **argv)
{
	fprintf(stderr, "Syntax: \n");
	fprintf(stderr, "    %s { -device <dev> } { -driver <drv> } { -laplink } { -pcw }\n", argv[0]);
	fprintf(stderr, "\n");
	fprintf(stderr, "For example:\n");
	fprintf(stderr, "    %s -device /dev/parport0 -driver parport\n", argv[0]);
	fprintf(stderr, "       - Connect to a LocoLink server on the first parallel port.\n\n");
	fprintf(stderr, "    %s -device unix:C:/tmp/locolink.socket -driver parsocket\n", argv[0]);
	fprintf(stderr, "       - Connect to an emulated LocoLink server on /tmp/locolink.socket.\n\n");
	fprintf(stderr, "    %s -device unix:C:locolink.shm -driver parshm -pcw -v1\n", argv[0]);
	fprintf(stderr, "       - Connect to an emulated LocoLink v1 server using shared memory.\n\n");


// XXX enumerate drivers
	return 0;
}

/* Display program version */
int version(char **argv)
{
	fprintf(stderr, "liblink version %s\n", LIBLINK_VERSION);
	return 0;
}

/* Handle an error from a LibLink operation. There are two possibilities:
 * 
 * LL_E_FAIL: Successfully called the server, and got a result, but 
 *           the result indicated a DOS error. 'result' is the DOS error
 *           code. Report the error and return.
 * Other:     Link-level error. The connection to the server has failed,
 *           or an unexpected or corrupt packet was received. Report the
 *           error and terminate llclient.
 */
void diewith(ll_error_t err, const char *operation, unsigned short result)
{
	const char *errmsg;

	if (err == LL_E_FAIL)
	{
		fprintf(stderr, "%s failed: DOS error = 0x%04x", operation,
			result);
		if (result >= 0 && result < DOS_ERR_COUNT)
			fprintf(stderr, " %s\n", dos_err[result]);
		else	fprintf(stderr, " (Unknown)\n");
		return;
	}

 	errmsg = ll_strerror(err);
	fprintf(stderr, "Error in %s: %s\n", operation, errmsg);

	llc_hangup(&remote);
	ll_close(&link);
	exit(1);
}

/* This callback is triggered when the server sends a keep-alive packet. It
 * displays a spinning baton, just to let the user know the link has not
 * hung. */
static int spinner(LL_PREMOTE self, int busy)
{
	static int count = 0;
	static char *spin = "\\-/|";
	if (!busy)
	{
		printf(" \b"); fflush(stdout);
	}
	else
	{
		printf("%c\b", spin[count]); fflush(stdout);
		count = (count + 1) % 4;
	}
	return 0;
}


/* Show drive label
ll_error_t showlabel(void)
{
	unsigned char ffblk[256];
	unsigned short result;
	ll_error_t e;

	e = llc_findlabel(remote, ffblk, &result);
	if (!e)
	{
		printf("Disk label : %-8.8s.%-3.3s",
				ffblk + 8, ffblk + 16);
		putchar('\n');
		return LL_E_OK;
	}
	else if (e == LL_E_FAIL && result == 2)
	{
		printf("No disk label.\n");
		return LL_E_OK;
	}
	return e;	
}
*/

/* Display a directory entry in a form similar to that used by PCDOS DIR. */
void show_file(unsigned char *ffblk)
{
	unsigned short date, time;
	unsigned long size;

	/* Parse the file date and time */
	time = ffblk[0x16] | (ffblk[0x17] << 8);
	date = ffblk[0x18] | (ffblk[0x19] << 8);
	size = ffblk[0x1A] | (ffblk[0x1B] << 8) | (ffblk[0x1C] << 16) | (ffblk[0x1D] << 24);

	/* Print the filename */
	printf("%-14.14s ", ffblk + 0x1E);

	/* Print the file size (or <DIR> if it's a directory) */
	if (ffblk[0x15] & 0x10)
		printf("%-8.8s ", "<DIR>");
	else	printf("%8ld ", size);

	/* Print date and time */
	printf(" %04d-%02d-%02d ", (date >> 9) + 1980, (date >> 5) & 0x0F, 
				date & 0x1F);
	printf(" %02d:%02d:%02d ", (time >> 11), (time >> 5) & 0x3F, 
					(time << 1) & 0x3F);

	/* Print attributes */
	putchar((ffblk[0x15] & 0x10) ? 'd' : '-');	
	putchar((ffblk[0x15] & 0x01) ? 'r' : '-');	
	putchar((ffblk[0x15] & 0x02) ? 'h' : '-');	
	putchar((ffblk[0x15] & 0x04) ? 's' : '-');	
	putchar((ffblk[0x15] & 0x08) ? 'v' : '-');	
	putchar((ffblk[0x15] & 0x20) ? 'a' : '-');	
	putchar('\n');
}

/* Remove trailing spaces from a string 
static void trim(char *s)
{
	int n = strlen(s) - 1;

	while (n >= 0 && isspace(s[n]))
	{
		s[n] = 0;
		--n;
	}
}*/

/* Convert a string to uppercase. The LocoLink server (at least in v2) is 
 * case-sensitive, so ensure all filenames are passed in upper case */
static void makeupper(char *search)
{
	char *p;

	for (p = search; *p; ++p)
	{
		*p = toupper(*p);
	}
}



/* Convert an entered filename to a full path on the remote system */
static void makeremotename(const char *name, char *namebuf)
{
	/* If path is not absolute, make it absolute */
	if (name[1] != ':' || name[2] != '\\')
	{
		if (st_path[0])
			sprintf(namebuf, "%c:\\%s\\%s", st_drive, st_path, name);
		else	sprintf(namebuf, "%c:\\%s", st_drive, name);
	}
	else 	sprintf(namebuf, "%s", name);
	namebuf[PATH_MAX - 1] = 0;
	/* We're talking to an (emulated) DOS system in which filenames 
	 * are in upper case */
	makeupper(namebuf);

	printf("%s -> %s\n", name, namebuf);
}


/* Set the current directory on the remote system (held locally) */
void change_dir(const char *path)
{
	if (LOCOLINK_V1 == st_protocol)
	{
		printf("Not supported in LocoLink v1.\n");
		return;
	}
	if (path == NULL || path[0] == 0 || (path[1] == ':' && path[2] == 0))
	{
		printf("Current directory: %c:\\%s\n", st_drive, st_path);
		return;
	}
	if (!strcmp(path, ".")) return;	/* No-op */
	if (path[0] == '.' && path[1] == '.')
	{
		char *p = strrchr(st_path, '\\');
		if (p) *p = 0;
		else	st_path[0] = 0;
		if (path[2] == 0)
		{
			printf("Current directory: %c:\\%s\n", st_drive, st_path);
			return;
		}
		else if (path[2] == '\\')
		{
			path += 3;
		}
		else path += 2;
	}
	if (path[1] == ':' && path[2] == '\\')
	{
		strcpy(st_path, path + 3);	
		printf("Current directory: %c:\\%s\n", st_drive, st_path);
		return;
	}
	else if (path[0] == '\\') strcpy(st_path, path + 1);
	else strcpy(st_path, path);
	printf("Current directory: %c:\\%s\n", st_drive, st_path);
}


void do_label(const char *path)
{
	int haslabel = 0;
	ll_error_t e;
	unsigned char ffblk[256];
	unsigned short result; 

	if (LOCOLINK_V1 == st_protocol)
	{
		printf("Not supported in LocoLink v1.\n");
		return;
	}
	do
	{
		e = llc_findlabel(remote, ffblk, st_drive, &result);
		if (e == LL_E_FAIL && result == 5)
		{
			do_reset();
		}
	}
	while (e == LL_E_FAIL && result == 5);

	if (!e) haslabel = 1;

	if (path == NULL)	/* Get label */
	{
		if (haslabel)
		{
			printf(" Volume in drive %c is %-8.8s.%-3.3s", st_drive,
					ffblk + 8, ffblk + 16);
			putchar('\n');
		}
		else if (e == LL_E_FAIL && result == 2)
		{
			printf(" Volume in drive %c has no label\n", st_drive);
		}
		else diewith(e, "Get volume label", result);
		return;
	}
	if (path[0] == '-')	/* Erase label */
	{
		if (!haslabel)
		{
			fprintf(stderr, "No volume label to remove\n");
			return;
		}
		e = llc_dellabel(remote, &result);
		if (e) diewith(e, "Remove volume label", result);
		return;

	}
	else	/* Set label */
	{
/* The LocoLink 2 server does not appear to care whether the label exists or 
 * not, so there's no need to use llc_renlabel on an existing label but
 * llc_makelabel on a new one. And the LocoLink 3 server doesn't seem to 
 * implement disk labels properly anyway */
		e = llc_makelabel(remote, path, &result);
/* This would have the same effect:
		e = llc_renlabel(remote, "*", path, &result);
*/
		if (!e) e = llc_closelabel(remote, &result);
		if (e) diewith(e, "Set volume label", result);
		do_label(NULL);
	}
}


/* Set the current directory on the remote system (held remotely) */
void rchange_dir(const char *path)
{
	unsigned short result;
	ll_error_t e;
	char buf[PATH_MAX];

	if (LOCOLINK_V1 == st_protocol)
	{
		printf("Not supported in LocoLink v1.\n");
		return;
	}
	if (path != NULL)
	{
		e = llc_chdir(remote, path, &result);

		if (e)
		{
			diewith(e, "Change directory", result);
			return;
		}
	}
	e = llc_getcwd(remote, st_drive, buf, &result);
	if (e)
	{
		diewith(e, "Get current directory", result);
		return;
	}	
	printf("Current directory [remote]: %s\n", buf);
}





/* List a directory on the remote system */
void rlist_dir(const char *search)
{
	unsigned char ffblk[256];
	unsigned short result;
	ll_error_t e;
	int lines = 0;
	unsigned short ax,bx,cx,dx;
	unsigned long freespace;
	/* Start by getting volume label */

	if (LOCOLINK_V1 == st_protocol)
	{
		printf("Not supported in LocoLink v1.\n");
		return;
	}
	if (search == NULL) search = "*.*";
	do
	{
		e = llc_findlabel(remote, ffblk, st_drive, &result);
		if (e == LL_E_FAIL && result == 5)
		{
			do_reset();
		}
	}
	while (e == LL_E_FAIL && result == 5);

	if (!e)
	{
		printf(" Volume in drive %c is %-8.8s.%-3.3s", st_drive,
				ffblk + 8, ffblk + 16);
		putchar('\n');
	}
	else if (e == LL_E_FAIL && result == 2)
	{
		printf(" Volume in drive %c has no label\n", st_drive);
	}
	else if (st_protocol != LOCOLINK_V3)
	{
/* The get volume label call always fails in LocoLink 3.00, so don't bother
 * reporting the failure in that version */
		diewith(e, "Get volume label", result);
	}

	printf(" Directory of  %c:\\%s\n\n", st_drive, st_path);
	e = llc_findfirst(remote, search, 0x16, ffblk, &result);
	if (e == LL_E_FAIL)
	{
		diewith(e, "Find first file", result);
	}
	while (!e) 
	{
		show_file(ffblk);
		e = llc_findnext(remote, ffblk, &result);
		++lines;
	}
	if (e && e != LL_E_FAIL)
	{
		diewith(e, "Find next file", result);
		return;
	}
	printf("%8d File(s) ", lines);

/* Files listed. Now show free space */
	e = llc_getfreespace(remote, 1, &ax, &bx, &cx, &dx);
	if (!e)
	{
		freespace = (unsigned long)ax * (unsigned long)bx * 
				(unsigned long)cx;
		printf("%ld bytes free", freespace);
	}
	putchar('\n');
	if (e && e != LL_E_FAIL)
	{
		diewith(e, "Get free space", ax);
	}
	fflush(stdout);
}


void list_dir(const char *wildcard)
{
	char search[PATH_MAX];

	if (wildcard == NULL || wildcard[0] == 0) 
				makeremotename("*.*", search);
	else		      	makeremotename(wildcard, search);
	rlist_dir(search);	
}



/* Display the contents of a remote file */
void view_file(const char *name)
{
	char buf[256];
	char namebuf[PATH_MAX];
	unsigned short result = 0;
	ll_error_t e = LL_E_OK;
	int n;

	if (LOCOLINK_V1 == st_protocol)
	{
		printf("Not supported in LocoLink v1.\n");
		return;
	}
	if (name == NULL || name[0] == 0)
	{
		fprintf(stderr, "This command must be followed by a remote filename.\n");
		return;
	}
	/* Convert passed name to remote pathname */
	makeremotename(name, namebuf);

	/* Open the file */
	e = llc_openfile(remote, namebuf, 1, &result);
	if (e)
	{
		diewith(e, "Open remote file", result);
		return;
	}
	else
	{
		unsigned short handle = result;

		do	
		{
			e = llc_read(remote, handle, buf, sizeof(buf), 
					&result);
			if (e)
			{
				diewith(e, "Read remote file", result);
				break;
			}	
			for (n = 0; n < result; n++)
			{
				if (buf[n] == 0x1A) 	/* CP/M EOF */
				{
					result = n;
					break;
				}
				putchar(buf[n]);
			}
		}
		while (result == sizeof(buf));

		e = llc_closefile(remote, handle, &result);
		if (e)
		{
			diewith(e, "Close remote file", result);
		}	
		
	}
}

void erase_file(const char *remotename)
{
	char namebuf[PATH_MAX];
	unsigned short result = 0;
	ll_error_t e = LL_E_OK;

	if (LOCOLINK_V1 == st_protocol)
	{
		printf("Not supported in LocoLink v1.\n");
		return;
	}
	if (remotename == NULL || remotename[0] == 0)
	{
		fprintf(stderr, "This command must be followed by a remote filename.\n");
		return;
	}

	/* Convert passed name to remote pathname */
	makeremotename(remotename, namebuf);

	e = llc_unlink(remote, namebuf, &result);
	if (e)
	{
		diewith(e, "Delete remote file", result);
	}
}



void rename_file(const char *remote1, const char *remote2)
{
	char namebuf1[PATH_MAX];
	char namebuf2[PATH_MAX];
	unsigned short result = 0;
	ll_error_t e = LL_E_OK;

	if (LOCOLINK_V1 == st_protocol)
	{
		printf("Not supported in LocoLink v1.\n");
		return;
	}
	if (remote1 == NULL || remote1[0] == 0 ||
	    remote2 == NULL || remote2[0] == 0)
	{
		fprintf(stderr, "This command must be followed by a two filenames.\n");
		return;
	}

	makeremotename(remote1, namebuf1);
	makeremotename(remote2, namebuf2);

	e = llc_rename(remote, namebuf1, namebuf2, &result);
	if (e)
	{
		diewith(e, "Rename file", result);
	}
}




/* Download a remote file */
void get_file(const char *remotename, const char *localname)
{
	char buf[256];
	char namebuf[PATH_MAX];
	unsigned short result = 0;
	ll_error_t e = LL_E_OK;
	FILE *fp;

	if (LOCOLINK_V1 == st_protocol)
	{
		printf("Not supported in LocoLink v1.\n");
		return;
	}
	if (remotename == NULL || remotename[0] == 0)
	{
		fprintf(stderr, "This command must be followed by a remote filename.\n");
		return;
	}
	if (localname == NULL) localname = remotename;

	/* Convert passed name to remote pathname */
	makeremotename(remotename, namebuf);

	printf("Remote file: %s\n"
	       "Local file:  %s\n", namebuf, localname);
	/* Open the files */
	fp = fopen(localname, "wb");
	if (!fp)
	{
		perror(localname);
		return;
	}
	e = llc_openfile(remote, namebuf, 1, &result);
	if (e)
	{
		diewith(e, "Open remote file", result);
		return;
	}
	else
	{
		unsigned short handle = result;

		do	
		{
			e = llc_read(remote, handle, buf, sizeof(buf), 
					&result);
			if (e)
			{
				diewith(e, "Read remote file", result);
				break;
			}
			if (!result) break;
			if (fwrite(buf, 1, result, fp) < result)
			{
				perror(localname);
				break;
			}	
		}
		while (result == sizeof(buf));

		e = llc_closefile(remote, handle, &result);
		if (e)
		{
			diewith(e, "Close remote file", result);
		}	
		if (fclose(fp))
		{
			perror(localname);
		}
		
	}
}


/* Upload a file to the remote system */
void put_file(const char *localname, const char *remotename)
{
	char buf[128];
	char namebuf[PATH_MAX];
	unsigned short result = 0;
	ll_error_t e = LL_E_OK;
	FILE *fp;
	long lsize;
	int len;

	if (localname == NULL || localname[0] == 0)
	{
		fprintf(stderr, "This command must be followed by a local filename.\n");
		return;
	}
	if (remotename == NULL)
	{
		const char *p = localname + strlen(localname) - 1;

		remotename = localname;
		while (p >= localname)
		{
			if (*p == '/'
#ifdef _WINDOWS
		 	|| *p == '\\' || *p == ':'
#endif
			)
			{
				remotename = p + 1;
				break;
			}
			--p;
		}
	}
/* XXX LocoLink v1 version */

	/* Convert passed name to remote pathname */
	makeremotename(remotename, namebuf);

	printf("Local file:  %s\n"
	       "Remote file: %s\n", localname, namebuf);
	/* Open the files */
	fp = fopen(localname, "rb");
	if (!fp)
	{
		perror(localname);
		return;
	}
	fseek(fp, 0, SEEK_END);
	lsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
/* XXX Convert attributes? */
	e = llc_creat(remote, lsize, 0, namebuf, &result);
	if (e)
	{
		diewith(e, "Create remote file", result);
		return;
	}
	else
	{
		unsigned short handle = result;

		do	
		{
/* Pad the last record with 0x1A, CP/M EOF */
			memset(buf, 0x1A, sizeof(buf));
			len = fread(buf, 1, sizeof(buf), fp);
			if (!len) break;

			e = llc_write(remote, handle, buf, sizeof(buf), &result);
			if (e)
			{
				diewith(e, "Write remote file", result);
				break;
			}
		}
		while (result == sizeof(buf));

		e = llc_closefile(remote, handle, &result);
		if (e)
		{
			diewith(e, "Close remote file", result);
		}	
		if (fclose(fp))
		{
			perror(localname);
		}
		
	}
}




/* Like view_file, but only loads the first 128 bytes and tries to parse
 * them as a LocoScript or +3DOS header */
void head_file(const char *name)
{
	unsigned char buf[128];
	char namebuf[PATH_MAX];
	unsigned short result = 0;
	ll_error_t e = LL_E_OK;
	int n;
	unsigned char cksum;

	if (LOCOLINK_V1 == st_protocol)
	{
		printf("Not supported in LocoLink v1.\n");
		return;
	}
	if (name == NULL || name[0] == 0)
	{
		fprintf(stderr, "This command must be followed by a remote filename.\n");
		return;
	}
	makeremotename(name, namebuf);
	/* Ensure path is absolute */

	e = llc_openfile(remote, namebuf, 1, &result);
	if (e)
	{
		diewith(e, "Open remote file", result);
		return;
	}
	else
	{
		unsigned short handle = result;

		memset(buf, 0, sizeof(buf));
		e = llc_read(remote, handle, buf, sizeof(buf), &result);
		if (e)
		{
			diewith(e, "Read remote file", result);
			return;
		}	
		e = llc_closefile(remote, handle, &result);
		if (e)
		{
			diewith(e, "Close remote file", result);
			return;
		}	
	}
	/* buf holds file header */
	for (cksum = 0, n = 0; n < 127; n++)
	{
		cksum += buf[n];
	}
	if (cksum == buf[127])
	{
		for (n = 0; n < LOCO_ID_COUNT; n++)
		{
			if (!memcmp(loco_ids[n], buf, 3))
			{
				printf("LocoScript %s (v%d.%d)\n",
					loco_ids[n] + 4, buf[3], buf[4]);
				printf("    %-30.30s\n", buf + 5);
				printf("    %-30.30s\n", buf + 35);
				printf("    %-30.30s\n", buf + 65);
				return;
			}
		}
		if (!memcmp("PLUS3DOS\032", buf, 9))
		{
			printf("+3DOS (v%d.%d) file\n", buf[9], buf[10]);
			switch (buf[15])
			{
				case 0: printf("    BASIC\n"); break;
				case 1: printf("    Number array\n"); break;
				case 2: printf("    Character array\n"); break;
				case 3: printf("    CODE %d,%d\n",
					buf[18]+256*buf[19],
					buf[16]+256*buf[17]); break;
			}
			return;
		}
	}
/* Header not recognised */
	for (n = 0; n < 128; n++)
		if (buf[n] == '\r' || buf[n] == '\n' || 
			isprint(buf[n])) putchar(buf[n]);
	putchar('\n');
}



void help()
{
	printf("Commands supported:\n"
		"    drive <a-p>          Select drive [LocoLink v3 only]\n"
		"    dir { <filespec> }   List files\n"
		"    erase <remotefile>   Delete remote file\n"
		"    get <remotefile> { <localfile> }\n"
		"                         Download a file from the server\n"
		"    head <remotefile>    Show file header\n"
		"    label                Show drive label\n"
		"    label <newlabel>     Set drive label\n"
		"    label -              Remove drive label\n"
		"    put <localfile> { <remotefile> }\n"
		"                         Upload a file to the server\n"
		"    quit                 Close the connection and leave the program\n"
		"    ren <oldname> <newname>\n"
                "                         Rename a file on the server\n"
		"    reset { <mode> }     Acknowledge disk change\n"
		"                       [LocoLink 3] mode should be C or L (CP/M or LocoScript)\n"
		"    type <remotefile>  Show contents of file\n"
	);
}

void setdrive(const char *arg)
{
	ll_error_t e;
	char drive;
	unsigned short result;

	if (LOCOLINK_V1 == st_protocol)
	{
		printf("Not supported in LocoLink v1.\n");
		return;
	}
	if (arg == NULL)
	{
		printf("Current drive: %c:\n", toupper(st_drive));
		return;
	}
 	drive = tolower(arg[0]);
	if (drive < 'a' || drive > 'p')
	{
		fprintf(stderr, "Drive must be A to P\n");
		return;
	}
	e = llc_seldrive(remote, drive - 'a', &result);
	if (e) 
	{
		diewith(e, "Select drive", result);
	}
	else
	{
		st_drive = toupper(drive);
	}
}


void do_reset()
{
	ll_error_t e;
	unsigned short result;

	if (st_protocol == LOCOLINK_V1) return;	/* There is no reset for v1 */
	
	e = llc_resetdisks(remote, st_mode, &result);
	if (e) 
	{
		diewith(e, "Reset disks", result);
	}
	else
	{
		st_path[0] = 0;
		printf("Disks reset: result=0x%04x\n", result);
	}
}

void do_reset_arg(const char *s)
{
	if (s == NULL || s[0] == 0)
	{
		do_reset();
	}
	else if (isdigit(s[0]))
	{
		st_mode = atoi(s);
		printf("[Mode %d]\n", st_mode);
		do_reset();
	}
	else if (toupper(s[0]) == 'L')
	{
		printf("[LocoScript mode]\n");
		st_mode = 0xFF;
		do_reset();
	}
	else if (toupper(s[0]) == 'C')
	{
		printf("[CP/M mode]\n");
		st_mode = 0;
		do_reset();
	}
	else	fprintf(stderr, "Unknown argument to reset command: %s\n", s);
}

#define MAXARG 10

void command_loop()
{
	char cmdbuf[81];
	char *args[MAXARG];
	int na;
	char *p;

	while (1)
	{
		printf("LocoLink> ");
		fflush(stdout);
		if (!fgets(cmdbuf, sizeof(cmdbuf)-1, stdin))
		{
			return;
		}
		/* Skip initial spaces */
		p = cmdbuf;
		while (p[0] && isspace(p[0])) ++p;
		for (na = 0; na < MAXARG; na++)
		{
			args[na] = strtok(p, " \t\n");
			p = NULL;
		} 
/*		for (na = 0; na < MAXARG; na++)
		{
			printf("%d: %s\n", na, args[na]);
		}	
*/
		if (args[0] == NULL) continue;	/* Nothing entered */
		if (!strcmp(args[0], "drive"))
		{
			setdrive(args[1]);
			continue;
		}	
		if (!strcmp(args[0], "dir") || !strcmp(args[0], "ls"))
		{
			list_dir(args[1]);
			continue;
		}
		if (!strcmp(args[0], "get"))
		{
			get_file(args[1], args[2]);
			continue;
		}
		if (!strcmp(args[0], "put"))
		{
			put_file(args[1], args[2]);
			continue;
		}
		if (!strcmp(args[0], "type") || !strcmp(args[0], "cat"))
		{
			view_file(args[1]);
			continue;
		}
		if (!strcmp(args[0], "erase") || !strcmp(args[0], "era") ||
		    !strcmp(args[0], "del")   || !strcmp(args[0], "rm"))
		{
			erase_file(args[1]);
			continue;
		}
		if (!strcmp(args[0], "ren") || !strcmp(args[0], "mv"))
		{
			rename_file(args[1], args[2]);
			continue;
		}
		if (!strcmp(args[0], "cd") || !strcmp(args[0], "chdir"))
		{
			change_dir(args[1]);
			continue;
		}
		if (!strcmp(args[0], "head"))
		{
			head_file(args[1]);
			continue;
		}
		if (!strcmp(args[0], "label"))
		{
			do_label(args[1]);
			continue;
		}
		if (!strcmp(args[0], "reset"))
		{
			do_reset_arg(args[1]);
			continue;
		}	
		if (!strcmp(args[0], "quit"))
		{
			return;
		}
		if (!strcmp(args[0], "help"))
		{
			help();
			continue;
		}
/* The "R" functions ('raw x') are intended for testing operations on the
 * remote system rather than for use in practice. */
		if (!strcmp(args[0], "rcd") || !strcmp(args[0], "rchdir"))
		{
			rchange_dir(args[1]);
			continue;
		}
		if (!strcmp(args[0], "rdir") || !strcmp(args[0], "rls"))
		{
			rlist_dir(args[1]);
			continue;
		}

		printf("Unknown command: '%s'\n", args[0]);	
	}
}



int main(int argc, char **argv)
{
	ll_error_t e;
	char server[33];
	int n;
	const char *driver = "parport";
	const char *port   = "/dev/parport0";

	LL_CABLE cable = LL_CABLE_NORMAL;
/* Find out what device to open. */
	for (n = 1; n < argc; n++)
	{
		if (!strcmp(argv[n], "--help")) return syntax(argv);
		if (!strcmp(argv[n], "--version")) return version(argv);
		if (!strcmp(argv[n], "--pcw")) { st_end = LL_END_PCW; }
		if (!strcmp(argv[n], "-pcw")) { st_end = LL_END_PCW; }
		if (!strcmp(argv[n], "--pc")) { st_end = LL_END_PC; }
		if (!strcmp(argv[n], "-pc")) { st_end = LL_END_PC; }
		if (!strcmp(argv[n], "--v1")) { st_protocol = LOCOLINK_V1; }
		if (!strcmp(argv[n], "-v1")) { st_protocol = LOCOLINK_V1; }
		if (!strcmp(argv[n], "--v2")) { st_protocol = LOCOLINK_V2; }
		if (!strcmp(argv[n], "-v2")) { st_protocol = LOCOLINK_V2; }
		if (!strcmp(argv[n], "--v3")) { st_protocol = LOCOLINK_V3; }
		if (!strcmp(argv[n], "-v3")) { st_protocol = LOCOLINK_V3; }
		if (!strcmp(argv[n], "--laplink")) { cable = LL_CABLE_LAPLINK; }
		if (!strcmp(argv[n], "-laplink"))  { cable = LL_CABLE_LAPLINK; }
		if (!strcmp(argv[n], "--nolaplink")) { cable = LL_CABLE_NORMAL; }
		if (!strcmp(argv[n], "-nolaplink"))  { cable = LL_CABLE_NORMAL; }

		if (n == (argc - 1)) continue;	
		if (!strcmp(argv[n], "--driver")) { ++n; driver = argv[n]; }
		if (!strcmp(argv[n], "-driver"))  { ++n; driver = argv[n]; }
		if (!strcmp(argv[n], "--device")) { ++n; port   = argv[n]; }
		if (!strcmp(argv[n], "-device"))  { ++n; port   = argv[n]; }
	}

	printf("Driver to use = %s\n", driver);
	printf("Device to use = %s\n", port);
	printf("Emulate Laplink cable: %s\n", cable ? "yes" : "no");
	printf("System type:           %s\n", 
			(st_end == LL_END_PCW) ? "PCW" : "PC");
	printf("Protocol:              ");
	switch(st_protocol)
	{
		case NO_PROTOCOL: printf("Autodetect\n");  break;
		case LOCOLINK_V1: printf("LocoLink v1\n"); break;
		case LOCOLINK_V2: printf("LocoLink v2\n"); break;
		case LOCOLINK_V3: printf("LocoLink v3\n"); break;
		default:	  printf("Unknown %d\n", st_protocol); break;	
	}


	e = ll_open(&link, port, driver);
	if (e) 
	{
		const char *errmsg = ll_strerror(e);
		fprintf(stderr, "Error opening: %s\n", errmsg);
		return 1;
	}
	e = ll_set_cable(link, cable);
	if (e) 
	{
		const char *errmsg = ll_strerror(e);
		fprintf(stderr, "Error setting cable type: %s\n", errmsg);
		ll_close(&link);
		return 1;
	}
	printf("Connecting...\n");
	e = llc_connect(&remote, link, st_end, client_version, st_protocol, server);
	if (e) 
	{
		const char *errmsg = ll_strerror(e);
		fprintf(stderr, "Error connecting: %s\n", errmsg);
		ll_close(&link);
		return 1;
	}
	llc_set_ping_callback(remote, spinner);
	llc_get_protocol(remote, &st_protocol);

	if (st_protocol == LOCOLINK_V3) st_drive = 'A'; else st_drive = 'Z';
	printf("Connected: Protocol version=%d  Server=%s\n", 
			st_protocol, server);

	/* Start with a RESET */
	do_reset();
	/* And then prompt for command */
	command_loop();
	llc_hangup(&remote);
	ll_close(&link);
	return 0;
}
