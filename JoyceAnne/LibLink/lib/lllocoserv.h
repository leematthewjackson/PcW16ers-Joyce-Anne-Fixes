/* liblink: Simulate parallel / serial hardware for emulators

    Copyright (C) 2002, 2023  John Elliott <seasip.webmaster@gmail.com>

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


/* LocoLink file server. 
 *
 * Give it a LocoLink packet (received from whatever source) 
 * and it will return a suitable response, serving files
 * from a directory passed to it at startup.
 */ 

/* Current state: */
typedef enum fserver_state
{
	IDLE,		/* Not in the middle of anything */
	DOSX_CREATE,	/* In Create, have requested filename */
	DOSX_OPEN,	/* In Open, have requested filename */
	DOSX_READ,	/* In Read, returning data */
	DOSX_WRITE,	/* In Write, have requested data */
	DOSX_FINDFIRST,	/* In Findfirst, have requested filename */
	DOSX_MKDIR,	/* In Mkdir, have requested filename */
	DOSX_CHDIR,	/* In Chdir, have requested filename */
	DOSX_RMDIR,	/* In Rmdir, have requested filename */
	DOSX_UNLINK,	/* In Unlink, have requested filename */
	DOSX_CHMOD,	/* In Chmod, have requested filename */
	DOSX_REN1,	/* In Rename, have requested old filename */
	DOSX_REN2,	/* In Rename, have requested new filename */
	DOSX_CCREAT,	/* In conditional create, have requested filename */
	DOSX_ACKDATA,	/* Have returned data, waiting for ack */

} FSERVER_STATE;


#include "lldosfs.h"

/* I don't know how many simultaneous open files a LocoLink client may 
 * want -- possibly as few as one -- but let's say 20, the same as MSDOS */
#define MAXFILEHANDLE 20

/* The fileserver: */
typedef struct ll_fserver
{
	LL_PROTOCOL protocol;	/* LocoLink protocol currently in use */
	int drive;		/* Current drive (for LocoLink v3) */
	FILE *fp[MAXFILEHANDLE]; /* Open files */
	int   rwmode[MAXFILEHANDLE]; /* Access modes (0=R/O else R/W) */
	FILE *xfer_fp;		/* Currently active file (in a READ / WRITE) */
	char clientname[32];	/* Client program name */

	LL_FSCALLBACK callback;	/* Callback to verify filesystem access */
	void *cookie;		/* Cookie to pass to callback */

	LL_DOSFS *dosfs;	/* Emulates the DOS filesystem */
	FSERVER_STATE state;	/* Current state */
	unsigned short ffirst_attrs;	/* Search attribute for FindFirst */
	unsigned short open_mode;	/* Mode for file open */
	unsigned short xfer_req;	/* Requested bytes to read / write */
	unsigned short xfer_bytes;	/* Actual bytes read / written */
	unsigned short chmod_subfunc;	/* CHMOD subfunction */
	unsigned short chmod_attrs;	/* CHMOD new attributes */
	char ren_oldname[256];		/* Old filename for Rename */
	unsigned short dos3_result;	/* Result after last data ack */
} LL_FSERVER;

/* Set filesystem access callback */
ll_error_t ll_fs_set_callback(LL_FSERVER *self, LL_FSCALLBACK func, void *cookie);
/* Get filesystem access callback */
ll_error_t ll_fs_get_callback(LL_FSERVER *self, LL_FSCALLBACK *func, void **cookie);
 


