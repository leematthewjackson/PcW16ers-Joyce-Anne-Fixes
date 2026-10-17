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

#ifndef LLDOSFS_H_INCLUDED
#define LLDOSFS_H_INCLUDED

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

typedef struct ll_finddata
{
	unsigned id;
	struct ll_finddata *next;
	char finddir[PATH_MAX];		/* Directory currently being searched */
					/* (for findfirst / findnext) */
	char findfile[PATH_MAX];	/* File template in search directory */
	unsigned short findattr;	/* Search includes directories? */
#ifdef HAVE_WINDOWS_H
	HANDLE	hFind;			/* FindFirst / FindNext handle */
	WIN32_FIND_DATA wfd;		/* Most recently found file */
#else
        DIR *direct;    		/* Directory being searched */
        struct dirent *entry;		/* Most recently found file */
#endif
} LL_FINDDATA;

/* This file attempts to simulate a DOS filesystem within a directory on the
 * host system. */
typedef struct ll_dosfs
{
	unsigned idnext;		/* Next free ID for file search */
	char rootdir[PATH_MAX];		/* Root directory (host path) */
	char cwd[PATH_MAX];		/* Current directory (real path) */
	char cwd_request[PATH_MAX];	/* Current directory (as DOS sees it) */
	LL_FSCALLBACK callback;		/* File access callback */
	void *cookie;			/* Cookie to pass to callback */
	LL_FINDDATA *searches;		/* Findfirsts / findnexts */
	LL_FINDDATA *lsearch;		/* Last search used */
} LL_DOSFS;


/* Construct a DOSFS */
ll_error_t ll_dosfs_construct(LL_DOSFS **pself, const char *rootdir);
/* Delete DOSFS */
ll_error_t ll_dosfs_destruct(LL_DOSFS **pself);

/* These roughly correspond to the operations exported by LocoLink */

/* Get drive label */
ll_error_t ll_dosfs_getlabel(LL_DOSFS *self, unsigned char *result);

/* Get free space */
ll_error_t ll_dosfs_getfree(LL_DOSFS *self,
				unsigned short *sectors_per_cluster, 
			 	unsigned short *free_clusters,
				unsigned short *bytes_per_sector, 
				unsigned short *total_clusters);

/* Find first file */
ll_error_t ll_dosfs_findfirst(LL_DOSFS *self, const char *filename, unsigned short attr, unsigned char *result);
/* Find next file */
ll_error_t ll_dosfs_findnext(LL_DOSFS *self, unsigned id, unsigned char *result);

/* Create directory */
ll_error_t ll_dosfs_mkdir(LL_DOSFS *self, const char *dirname);
/* Remove directory */
ll_error_t ll_dosfs_rmdir(LL_DOSFS *self, const char *dirname);
/* Change CWD */
ll_error_t ll_dosfs_chdir(LL_DOSFS *self, const char *dirname);
/* Unlink file */
ll_error_t ll_dosfs_unlink(LL_DOSFS *self, const char *dirname);
/* Get attributes */
ll_error_t ll_dosfs_getattr(LL_DOSFS *self, const char *dirname, unsigned short *attr);
/* Set attributes */
ll_error_t ll_dosfs_setattr(LL_DOSFS *self, const char *dirname, unsigned short attr);
/* Get CWD */
ll_error_t ll_dosfs_getcwd(LL_DOSFS *self, char *dirname);


/* Open file */
ll_error_t ll_dosfs_open(LL_DOSFS *self, const char *filename, unsigned short mode, FILE **fp);

/* Create file */
ll_error_t ll_dosfs_creat(LL_DOSFS *self, const char *filename, unsigned short mode, FILE **fp);

/* Conditionally create file */
ll_error_t ll_dosfs_ccreat(LL_DOSFS *self, const char *filename, unsigned short mode, FILE **fp);

/* Rename file */
ll_error_t ll_dosfs_rename(LL_DOSFS *self, const char *oldname, const char *newname);

/* Get / set file timestamp */
ll_error_t ll_dosfs_get_ftime(LL_DOSFS *self, FILE *fp, unsigned short *date, unsigned short *time);
ll_error_t ll_dosfs_set_ftime(LL_DOSFS *self, FILE *fp, unsigned short date, unsigned short time);



/* Set filesystem access callback */
ll_error_t ll_dosfs_set_callback(LL_DOSFS *self, LL_FSCALLBACK func, void *cookie);
/* Get filesystem access callback */
ll_error_t ll_dosfs_get_callback(LL_DOSFS *self, LL_FSCALLBACK *func, void **cookie);
 
#endif /* ndef LLDOSFS_H_INCLUDED */


