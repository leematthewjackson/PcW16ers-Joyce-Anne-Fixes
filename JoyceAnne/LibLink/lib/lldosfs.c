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

#include "ll.h"
#include "lldosfs.h"

#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

/* This file attempts to simulate a DOS filesystem within a directory on the
 * host system. */

/* Forward declarations: */

/* Free any resources being used for a findfirst / findnext in progress */
static void ll_dosfs_findclose(LL_DOSFS *self, LL_FINDDATA *search);

/* Convert a DOS filename to a host system path */
static ll_error_t ll_dosfs_makename(LL_DOSFS *self, const char *dosname, char *filename);

/* See if the passed filename conforms to DOS 8.3 name and character set 
 * standards. If check_case is true, rejects filenames containing upper-case
 * characters */
static int conforms_pcdos(const char *filename, int check_case);

/* Check for characters that DOS considers to be path separators */
static inline int ll_issep(char c) { return (c == '/' || c == '\\'); }



/*
static void logfunc(const char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	vfprintf(stderr, s, ap);
	va_end(ap);
	fflush(stderr);
}
#define LOG(x) logfunc x
*/
#define LOG(x)

/* Ensure that buffer ends with a directory separator */
static void cat_separator(char *buffer)
{
	if (strlen(buffer) == 0)
	{
		strcpy(buffer, "./");
	}
	else
	{
		char *p = buffer + strlen(buffer) - 1;
		if (*p != '/'
#ifdef HAVE_WINDOWS_H
			&& *p != '\\' && *p != ':'
#endif	
			) strcat(buffer, "/");
	}	
}


/* Construct an LL_DOSFS object, with its filesystem root at rootdir */
ll_error_t ll_dosfs_construct(LL_DOSFS **pself, const char *rootdir)
{
	LL_DOSFS *self;

/* Create the object and initialise it to known values */
	if (!pself) return LL_E_BADPTR;
	self = malloc(sizeof(LL_DOSFS));
	if (!self) return LL_E_NOMEM;

	memset(self, 0, sizeof(LL_DOSFS));
/* Expand the path from relative to absolute */
#ifdef HAVE_WINDOWS_H
	if (!GetFullPathName(rootdir, PATH_MAX, self->rootdir, NULL))
	{
		strcpy(self->rootdir, rootdir);
	}

#else
	if (!realpath(rootdir, self->rootdir))
	{
		strcpy(self->rootdir, rootdir);
	}
#endif
	/* Ensure there's a directory separator at the end */
	cat_separator(self->rootdir);

	*pself = self;
/* Set CWD to root directory */
	strcpy(self->cwd, self->rootdir);
	sprintf(self->cwd_request, "A:\\");
	LOG(("ll_dosfs_create: rootdir='%s'\n", self->rootdir));
	return LL_E_OK;
}


/* Destroy a DOSFS. If there is a find operation in progress, shut it down. */
ll_error_t ll_dosfs_destruct(LL_DOSFS **pself)
{
	LL_DOSFS *self;
	LL_FINDDATA *search;

	LOG(("ll_dosfs_close\n"));
	if (!pself) return LL_E_BADPTR;
	self = *pself;
	if (!self) return LL_E_BADPTR;

	/* Close down all searches */
	while (self->searches)
	{
		search = self->searches;
		self->searches = search->next;
		ll_dosfs_findclose(self, search);
		free(search);
	}
	free(self);
	return LL_E_OK;
}

/* Convert a filename into the fixed NNNNNNNTTT form used by an FCB */
static void mkfcbname(char *name, char *fcb)
{
	char *p = strchr(name, '.');
	int n;

	/* If there is no dot then the name populates the first 8 characters */
	if (!p)
	{
		sprintf(fcb, "%-8.8s   ", name);
	}
	else	/* Otherwise the name gets the bit before the dot and the */
	{	/* type gets the bit after */
		*p = 0;
		sprintf(fcb, "%-8.8s%-3.3s", name, p + 1);
		*p = '.';
	}
	/* Expand '*' to '???' */
	for (n = 0; n < 8; n++)
	{
		if (fcb[n] == '*') 
		{
			while (n < 8) fcb[n++] = '?';
			break;
		}
	}
	for (n = 0; n < 3; n++)
	{
		if (fcb[8 + n] == '*') 
		{
			while (n < 3) fcb[8 + n++] = '?';
			break;
		}
	}
}


/* See if a file matches the search template. For Win32 FindFirst() / FindNext()
 * has handled the wildcard so we just need to check attribute. For Posix we 
 * need to check both */
static ll_error_t ll_dosfs_match(LL_DOSFS *self, LL_FINDDATA *search, unsigned char *result)
{
	struct stat st;
	struct tm *ptm;
	char *filename;
	char pathname[PATH_MAX];
	int n;
#ifdef HAVE_WINDOWS_H
	/* Prefer the short name as it's already in 8.3 format */
	filename = search->wfd.cAlternateFileName;
	if (strlen(filename) == 0 && strlen(search->wfd.cFileName) < 13)
	{
		filename = search->wfd.cFileName;
	}
	if (!(search->findattr & 0x10))	/* Omit directories */
	{
		if (search->wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			return LL_E_NOMOREFILES;
	}
#else
	filename = search->entry->d_name;

	LOG(("Found file: %s ", filename));
	if (!conforms_pcdos(filename, 1))
	{
		LOG((" not conformant\n", filename));
		return LL_E_NOMOREFILES;
	}
	LOG((" conformant\n", filename));

/* Compare the filename against the search template */
	if (strcmp(search->findfile, "*.*"))	// Shortcut: *.* matches 
	{					// everything, so no need to
		char fcbname[13];		// do the comparison
		char fcbmatch[13];

		mkfcbname(filename, fcbname);
		mkfcbname(search->findfile, fcbmatch);

		for (n = 0; n < 11; n++)
		{
			if (fcbname[n] != fcbmatch[n] && fcbmatch[n] != '?')
				return LL_E_NOMOREFILES;
		}
	}
	
#endif
	LOG(("ll_dosfs_match(%s : %s)\n", filename, search->findfile));
	/* Reject any file whose name is longer than 12 characters, as DOS
	 * 8.3 filenames have to fit in 12. */
	if (strlen(filename) > 12) return LL_E_NOMOREFILES;
	/* Generate the path to the found file */
	if (strlen(search->finddir))
	{	
		/* If a path was passed to the search, the file is there */
		strcpy(pathname, search->finddir);
	}
	else
	{
		/* Otherwise it's in the CWD */
		strcpy(pathname, self->cwd);
	}
	/* Append filename to the search path */
	cat_separator(pathname);
	strcat(pathname, filename);

	/* Get its details */	
	if (stat(pathname, &st))
	{
		LOG(("ll_dosfs_match: stat(%s) failed\n", pathname));
		return LL_E_NOMOREFILES;
	}
	LOG(("ll_dosfs_match: stat(%s) OK\n", pathname));
	if (!(search->findattr & 0x10))	/* Omit directories? */
	{
		if (S_ISDIR(st.st_mode))
		{
			LOG(("Searching for file but found dir\n"));
			return LL_E_NOMOREFILES;	
		}
	}
	/* If findattr & 0x10, returns both directories and files */

	/* Generate a FINDFIRST / FINDNEXT results buffer */
	memset(result, 0, 0x2C);

	/* Populate the buffer with a pattern so I can see what is being 
	 * sent to and fro */
	for (n = 0; n < 0x15; n++) result[n] = n + 0xE0;
	/* Store a 'handle' in bytes 0x06-0x09 of the result */
	result[0x06] = (search->id & 0xFF);
	result[0x07] = (search->id >> 8) & 0xFF;
	result[0x08] = (search->id >> 16) & 0xFF;
	result[0x09] = (search->id >> 24) & 0xFF;

	result[0x11] = (search->id & 0xFF);
	result[0x12] = (search->id >> 8) & 0xFF;
	result[0x13] = (search->id >> 16) & 0xFF;
	result[0x14] = (search->id >> 24) & 0xFF;

	ptm = localtime(&st.st_mtime);
	/* File attributes. Windows returns these, but for POSIX we have
	 * to invent them */
#ifdef HAVE_WINDOWS_H
	result[0x15] = search->wfd.dwFileAttributes & 0x3F;
#else
	if (S_ISDIR(st.st_mode))
	{
		result[0x15] |= 0x10;	/* Directory? */
	}
	if (!(st.st_mode & S_IWUSR))
	{
		result[0x15] |= 0x01;	/* Read-only? */
	}
#endif
	/* Time stamp */
	result[0x16] = (ptm->tm_sec >> 1) | (ptm->tm_min << 5);
	result[0x17] = (ptm->tm_min >> 3) | (ptm->tm_hour << 3);
	result[0x18] = (ptm->tm_mday)     | ((ptm->tm_mon + 1) << 5);
	result[0x19] = ((ptm->tm_mon + 1) >> 3)  | ((ptm->tm_year - 80) << 1);
	/* Size */
	result[0x1A] = st.st_size & 0xFF;
	result[0x1B] = (st.st_size >> 8) & 0xFF;
	result[0x1C] = (st.st_size >> 16) & 0xFF;
	result[0x1D] = (st.st_size >> 24) & 0xFF;
	/* Filename */
	strcpy((char *)result + 0x1E, filename);
	/* Force filename to upper case */
	for (n = 0x1E; result[n]; n++)
	{
		if (islower(result[n])) result[n] = toupper(result[n]);
	}	

	return LL_E_OK;
}

/* Get volume label */
ll_error_t ll_dosfs_getlabel(LL_DOSFS *self, unsigned char *result)
{
	char label[PATH_MAX];
	char fcbname[13];
	char *p;
	int n;

	if (!self || !result) return LL_E_BADPTR;

	/* On Windows, could conceivably use GetVolumeInformation() to return
	 * the actual drive label. But for now, just return the last 
  	 * component of the path */	
	strcpy(label, self->rootdir);
	/* Lop off the directory separator */
	label[strlen(label) - 1] = 0;

	/* Limit to last path component */
	p = strrchr(label, '/');
	if (p)
	{
		memmove(label, p + 1, strlen(p));
	}
#ifdef HAVE_WINDOWS_H
	p = strrchr(label, '\\');
	if (p)
	{
		memmove(label, p + 1, strlen(p));
	}
	p = strrchr(label, ':');
	if (p)
	{
		memmove(label, p + 1, strlen(p));
	}

#endif
	LOG(("Return volume label: %s\n", label));

	if (label[0] == 0 || label[0] == '.') return LL_E_NOTFOUND;

	memset(result, 0, 19);
	mkfcbname(label, fcbname);

	result[0] = 0xFF;	/* Extended FCB */
	result[6] = 0x08;	/* Volume label */
	memcpy(result + 8, fcbname, 11); 
	for (n = 8; n < 19; n++)
	{
		if (islower(result[n])) result[n] = toupper(result[n]);
	}
	return LL_E_OK;
}


/* Get free space.  */
ll_error_t ll_dosfs_getfree(LL_DOSFS *self,
				unsigned short *sectors_per_cluster, 
			 	unsigned short *free_clusters,
				unsigned short *bytes_per_sector, 
				unsigned short *total_clusters)
{
#ifdef HAVE_WINDOWS_H
	char drivename[PATH_MAX];
	int n;
	DWORD dwSectorsPerCluster;
	DWORD dwBytesPerSector;
	DWORD dwFreeClusters;
	DWORD dwTotalClusters;

	strcpy(drivename, self->cwd);
	if (drivename[0] == '\\' && drivename[1] == '\\') /* UNC path */
	{
		for (n = 2; drivename[n]; n++)
		{
			if (drivename[n] == '/' || drivename[n] == '\\')
			{
				drivename[n+1] = 0;
				break;
			}
		}
	}
	else	/* Drive and rootdir */
	{
		if (drivename[1] == ':' && 
			(drivename[2] == '\\' || drivename[2] == '/'))
		{
			drivename[3] = 0;
		}
	}
	if (GetDiskFreeSpace(drivename, &dwSectorsPerCluster, &dwBytesPerSector,
				&dwFreeClusters, &dwTotalClusters))
	{
		*bytes_per_sector = dwBytesPerSector;
		*sectors_per_cluster = dwSectorsPerCluster;

		if (dwTotalClusters <= 65535) *total_clusters = dwTotalClusters;
		else			  *total_clusters = 65535;		

		if (dwFreeClusters <= 65535)  *free_clusters = dwFreeClusters;
		else			  *free_clusters = 65535;		
		return LL_E_OK;
	}
	return LL_E_SYSERR;
#else
	struct statvfs stats;

	LOG(("Performing statvfs: self->cwd=%s\n", self->cwd));
	if (!statvfs(self->cwd, &stats))
	{
		/* Assume 512 bytes / sector rather than going to the 
		 * trouble of determining whether it's actually 4k */
		*bytes_per_sector = 512;
		*sectors_per_cluster = stats.f_bsize / (*bytes_per_sector);

		if (stats.f_blocks <= 65535) *total_clusters = stats.f_blocks;
		else			     *total_clusters = 65535;		

		if (stats.f_bfree <= 65535)  *free_clusters = stats.f_bfree;
		else			     *free_clusters = 65535;		
		return LL_E_OK;
	}
	else LOG(("statvfs_failed: error=%s\n", strerror(errno)));
	return LL_E_SYSERR;
#endif
}


static ll_error_t ll_dosfs_getsearch(LL_DOSFS *self, const char *finddir, const char *findfile, unsigned short attr, LL_FINDDATA **psearch)
{
	LL_FINDDATA *p = self->searches;

	LOG(("Searching finddir=%s findfile=%s attr=%02x\n", 
		finddir, findfile, attr));
	while (p)
	{
		if (p->findattr == attr && 
			!strcmp(finddir, p->finddir) &&
			!strcmp(findfile, p->findfile))
		{
			*psearch = p;
LOG(("Found existing search, id=%02x\n", p->id));
			return LL_E_OK;
		}
		p = p->next;
	}
	/* Not an existing search; create a new one */
	p = malloc(sizeof(LL_FINDDATA));
	if (!p) return LL_E_NOMEM;
	memset(p, 0, sizeof(*p));
#ifdef HAVE_WINDOWS_H
	p->hFind = INVALID_HANDLE_VALUE;
#endif
	strcpy(p->finddir, finddir);
	strcpy(p->findfile, findfile);
	p->findattr = attr;
	++self->idnext;
/* Don't allow -1 as a valid id */
	if (self->idnext == 0xFFFFFFFF) self->idnext = 0;
	p->id = self->idnext;
	p->next = self->searches;
	self->searches = p;
LOG(("Allocated new search, id=%02x\n", p->id));
	*psearch = p;
	return LL_E_OK;	
}



/* Find first file */
ll_error_t ll_dosfs_findfirst(LL_DOSFS *self, const char *filename, unsigned short attr, unsigned char *result)
{	
	ll_error_t err;
	char findpath[PATH_MAX];
	LL_FINDDATA *search = NULL;

	if (!self || !filename || !result) return LL_E_BADPTR;

	LOG(("ll_dosfs_findfirst(%s,0x%04x)\n", filename, attr));
	self->lsearch = NULL;

	err = ll_dosfs_makename(self, filename, findpath);
	if (err) return err;

#ifdef HAVE_WINDOWS_H
	err = ll_dosfs_getsearch(self, findpath, "", attr, &search);
	if (err) return err;
	/* Close off any previous find */
	ll_dosfs_findclose(self, search);
	search->findattr = attr;

	search->hFind = FindFirstFile(findpath, &search->wfd);

	/* No files found? */
	if (search->hFind == INVALID_HANDLE_VALUE)
	{
		return LL_E_NOTFOUND;
	}
	/* See if this file is suitable */
	err = ll_dosfs_match(self, search, result);
	if (err != LL_E_NOMOREFILES) return err;
	/* If not, chain to findnext() to find the next file */
	self->lsearch = search;
	return ll_dosfs_findnext(self, 0xFFFFFFFF, result);
#else
	{
		char *p;
		char finddir[PATH_MAX];
		char findfile[PATH_MAX];
		
		strcpy(finddir, findpath);
		p = strrchr(finddir, '/'); /* Find the last dir sep */
		if (p)	/* Which should always be the case */
		{
			*p = 0;
			/* Set findfile to be just the filename template */
			strcpy(findfile, p + 1);
		}
		else
		{
			strcpy(findfile, filename);
		}
		err = ll_dosfs_getsearch(self, finddir, findfile, attr, &search);
		if (err) return err;
		/* Close off any previous find */
		ll_dosfs_findclose(self, search);
		search->findattr = attr;

		LOG(("Searching '%s' for '%s'\n", search->finddir, search->findfile));
		search->direct = opendir(search->finddir);
		if (!search->direct)
		{
			/* Directory does not exist */
			return LL_E_NOTFOUND;
		}
		self->lsearch = search;
		return ll_dosfs_findnext(self, 0xFFFFFFFF, result);
	}
#endif
}


/* Find next file */
ll_error_t ll_dosfs_findnext(LL_DOSFS *self, unsigned id, unsigned char *result)
{
	ll_error_t err;
	LL_FINDDATA *search, *search2;
	
	if (!self || !result) return LL_E_BADPTR;

	/* If ID of -1 is passed in, use the search we were already on */
	if (id == 0xFFFFFFFF)
	{
		search = self->lsearch;
	}
	else
	{
		search = self->searches;
		while (search && search->id != id)
		{
			search = search->next;	
		}
	}
	if (!search)
	{
		LOG(("Search id %08x not found\n"));
		return LL_E_NOTFOUND;
	}
	self->lsearch = search;

	LOG(("<%d>ll_dosfs_findnext(%s/%s)\n", 
			search->id, search->finddir, search->findfile));

/* Check that there is a search in progress */
#ifdef HAVE_WINDOWS_H
	if (search->hFind == INVALID_HANDLE_VALUE)
	{
		return LL_E_NOTFOUND;
	}
	while (FindNextFile(search->hFind, &search->wfd))
	{
		err = ll_dosfs_match(self, search, result);
		if (err != LL_E_NOMOREFILES) return err;
	}
#else
	if (!search->direct)
	{
		return LL_E_NOTFOUND;
	}
	while ((search->entry = readdir(search->direct)))	
	{
		err = ll_dosfs_match(self, search, result);
		if (err != LL_E_NOMOREFILES) return err;
	}
#endif
	LOG(("<%d>ll_dosfs_findnext: No more files. Closing search.\n",
		search->id));
	/* No matching entry was found */
	ll_dosfs_findclose(self, search);

/* Remove this search from the linked list */	
	if (self->searches == search)
	{
		self->searches = search->next;
	}
	else
	{
		search2 = self->searches;
		while (search2)
		{
			if (search2->next == search)
			{
				search2->next = search->next;	
			}
		}
	}
/* And free it */
	free(search);
	self->lsearch = NULL;	
	return LL_E_NOMOREFILES;

}


static void ll_dosfs_findclose(LL_DOSFS *self, LL_FINDDATA *search)
{
	LOG(("ll_dosfs_findclose(id=%d)\n", search->id));
#ifdef HAVE_WINDOWS_H
	if (search->hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(search->hFind);
		search->hFind = INVALID_HANDLE_VALUE;
	}
#else
	if (search->direct)
	{
		closedir(search->direct);
		search->direct = NULL;
	}
#endif

}

ll_error_t ll_dosfs_open(LL_DOSFS *self, const char *filename, unsigned short mode, FILE **fp)
{
	ll_error_t err;
	char pathname[PATH_MAX];

	if (!self || !filename || !fp) return LL_E_BADPTR;
	
	err = ll_dosfs_makename(self, filename, pathname);
	if (err) return err;

	LOG(("Open file: %s -> %s  mode=%d\n", filename, pathname, mode & 7));

	if (self->callback)
	{
		err = (*self->callback)(self->cookie, pathname);
		if (err) return err;
	}
	if (mode & 7)
	{
		*fp = fopen(pathname, "r+b");
	}
	else
	{
		*fp = fopen(pathname, "rb");
	}
	if (!*fp) return LL_E_SYSERR;
	else return LL_E_OK;
}


ll_error_t ll_dosfs_creat(LL_DOSFS *self, const char *filename, unsigned short attrs, FILE **fp)
{
	ll_error_t err;
	char pathname[PATH_MAX];

	if (!self || !filename || !fp) return LL_E_BADPTR;
	
	err = ll_dosfs_makename(self, filename, pathname);
	if (err) return err;

	LOG(("Create file: %s -> %s  attrs=%d\n", filename, pathname, attrs));

	if (self->callback)
	{
		err = (*self->callback)(self->cookie, pathname);
		if (err) return err;
	}
	*fp = fopen(pathname, "w+b");
	if (!*fp) return LL_E_SYSERR;
	else return LL_E_OK;
}


/* Create a file if it doesn't already exist. */
ll_error_t ll_dosfs_ccreat(LL_DOSFS *self, const char *filename, unsigned short attrs, FILE **fp)
{
	ll_error_t err;
	char pathname[PATH_MAX];
	int fd;

	if (!self || !filename || !fp) return LL_E_BADPTR;
	
	err = ll_dosfs_makename(self, filename, pathname);
	if (err) return err;

	LOG(("Conditionally create file: %s -> %s  attrs=%d\n", filename, pathname, attrs));

	if (self->callback)
	{
		err = (*self->callback)(self->cookie, pathname);
		if (err) return err;
	}
	fd = open(pathname, O_RDWR | O_CREAT | O_EXCL, S_IRWXU);
	if (fd < 0) return LL_E_SYSERR;
	*fp = fdopen(fd, "w+b");
	if (!*fp) 
	{
		close(fd);
		return LL_E_SYSERR;
	}
	return LL_E_OK;
}


ll_error_t ll_dosfs_mkdir(LL_DOSFS *self, const char *filename)
{
	ll_error_t err;
	char pathname[PATH_MAX];

	if (!self || !filename) return LL_E_BADPTR;
	
	err = ll_dosfs_makename(self, filename, pathname);
	if (err) return err;

	LOG(("Create directory: %s -> %s\n", filename, pathname));

	if (self->callback)
	{
		err = (*self->callback)(self->cookie, pathname);
		if (err) return err;
	}
#ifdef HAVE_WINDOWS_H
	if (mkdir(pathname)) return LL_E_SYSERR;
#else
	if (mkdir(pathname, 0777)) return LL_E_SYSERR;
#endif
	else return LL_E_OK;
}


ll_error_t ll_dosfs_rmdir(LL_DOSFS *self, const char *filename)
{
	ll_error_t err;
	char pathname[PATH_MAX];

	if (!self || !filename) return LL_E_BADPTR;
	
	err = ll_dosfs_makename(self, filename, pathname);
	if (err) return err;

	LOG(("Remove directory: %s -> %s\n", filename, pathname));

	if (self->callback)
	{
		err = (*self->callback)(self->cookie, pathname);
		if (err) return err;
	}
	if (rmdir(pathname)) return LL_E_SYSERR;
	else return LL_E_OK;
}


ll_error_t ll_dosfs_chdir(LL_DOSFS *self, const char *filename)
{
/* It turns out that existing LocoLink servers don't actually implement 
 * chdir. Or rather, they let you set a notional CWD that is returned by 
 * GetCWD(), but this does not have any effect on other operations, since
 * they all are designed to work only with absolute paths. 
 *
 * Do likewise: Update self->cwd_request (requested CWD) but don't touch 
 * self->cwd (real CWD).
 */
#if 0
	ll_error_t err;
	char pathname[PATH_MAX];
	char *p;
	struct stat st;
#endif
	if (!self || !filename) return LL_E_BADPTR;

	/* Short-circuit chdir(".") to do nothing */
	if (!strcmp(filename, "."))
	{
		return LL_E_OK;
	}
#if 0
	/* Short-circuit chdir("..") to go up one */
	if (!strcmp(filename, ".."))
	{
		if (!strcmp(self->cwd, self->rootdir)) 
			return LL_E_OK;	/* Can't chdir .. from the root */

		/* Point to end of CWD (should be separator) */
		p = self->cwd + strlen(self->cwd) - 1;
		if (ll_issep(*p)) /* which it should be */
		{
			*p = 0;
			--p;
		}
		p = strrchr(self->cwd, '/'); 
		if (!p) /* Path was something like foo/ or even /, and we */
		{	/* just hacked off the last /. Put it back. */
			cat_separator(self->cwd);
			return LL_E_BADNAME;	
		}
/* Similarly chop the last path component off cwd_request */
		p[1] = 0;
		p = strrchr(self->cwd_request, '\\');
		if (p) *p = 0;
		if (strlen(self->cwd_request) == 2 && self->cwd_request[1] == ':')
			strcat(self->cwd_request, "\\");
		return LL_E_OK;
	}
	if (!strcmp(filename, "\\") || !strcmp(filename + 1, ":\\"))
	{
/* Request to chdir to root */
		strcpy(self->cwd, self->rootdir);
		strcpy(self->cwd_request, "A:\\");
		return LL_E_OK;	
	}
	
	err = ll_dosfs_makename(self, filename, pathname);
	if (err) return err;

	cat_separator(pathname);
	LOG(("Change directory: %s -> %s\n", filename, pathname));

/* Ensure we can stat the new path */
	if (stat(pathname, &st)) return LL_E_SYSERR;	
	if (!S_ISDIR(st.st_mode))
	{
		return LL_E_BADNAME;	/* Can't chdir to a file */
	}

	strcpy(self->cwd, pathname);
#endif
	if (ll_issep(filename[0]))	/* Requested absolute path? */
	{
		sprintf(self->cwd_request, "A:%s", filename);
	}
	else if (filename[1] == ':' && ll_issep(filename[2]))
	{
		strcpy(self->cwd_request, filename);
	}
	else if (filename[1] == ':')	/* Requested relative path */
	{
		strcat(self->cwd_request, "\\");
		strcat(self->cwd_request, filename + 2);		
	}
	else
	{
		strcat(self->cwd_request, "\\");
		strcat(self->cwd_request, filename);		
	}
	return LL_E_OK;
}


/* Unlink a file */
ll_error_t ll_dosfs_unlink(LL_DOSFS *self, const char *filename)
{
	ll_error_t err;
	char pathname[PATH_MAX];

	if (!self || !filename) return LL_E_BADPTR;
	
	err = ll_dosfs_makename(self, filename, pathname);
	if (err) return err;

	LOG(("Remove file: %s -> %s\n", filename, pathname));

	if (self->callback)
	{
		err = (*self->callback)(self->cookie, pathname);
		if (err) return err;
	}
	if (unlink(pathname)) return LL_E_SYSERR;
	else return LL_E_OK;
}


/* Get file attributes */
ll_error_t ll_dosfs_getattr(LL_DOSFS *self, const char *filename, 
				unsigned short *attr)
{
	ll_error_t err;
	char pathname[PATH_MAX];

	if (!self || !filename || !attr) return LL_E_BADPTR;
	
	err = ll_dosfs_makename(self, filename, pathname);
	if (err) return err;

	LOG(("Get file attributes: %s -> %s\n", filename, pathname));

#ifdef HAVE_WINDOWS_H
	{
		DWORD dwAttr = GetFileAttributes(pathname);
		
		if (dwAttr == INVALID_FILE_ATTRIBUTES)
			return LL_E_SYSERR;
		*attr = dwAttr & 0x3F;
	}
#else
	{
		struct stat st;

		if (stat(pathname, &st))
		{
			LOG(("ll_dosfs_getattr: stat(%s) failed\n", pathname));
			return LL_E_SYSERR;
		}
		*attr = 0;
		if (S_ISDIR(st.st_mode))
		{
			*attr |= 0x10;
		}
		if (!(st.st_mode & S_IWUSR))
		{
			*attr |= 0x01;
		}
	}
#endif
	return LL_E_OK;
}


/* Set file attributes */
ll_error_t ll_dosfs_setattr(LL_DOSFS *self, const char *filename, 
				unsigned short attr)
{
	ll_error_t err;
	char pathname[PATH_MAX];

	if (!self || !filename) return LL_E_BADPTR;
	
	err = ll_dosfs_makename(self, filename, pathname);
	if (err) return err;

	LOG(("Set file attributes: %s -> %s attr=%04x\n", filename, pathname,
			attr));
	if (self->callback)
	{
		err = (*self->callback)(self->cookie, pathname);
		if (err) return err;
	}

#ifdef HAVE_WINDOWS_H
	{
		DWORD dwAttr = GetFileAttributes(pathname);
		
		if (dwAttr == INVALID_FILE_ATTRIBUTES)
			return LL_E_SYSERR;
		dwAttr &= ~0x3F;
		dwAttr |= (attr & 0x3F);

		if ((attr & 0x3F) == 0) dwAttr |= FILE_ATTRIBUTE_NORMAL;

		if (!SetFileAttributes(pathname, dwAttr))
			return LL_E_SYSERR;
	}
#else
	{
		struct stat st;

		if (stat(pathname, &st))
		{
			LOG(("ll_dosfs_getattr: stat(%s) failed\n", pathname));
			return LL_E_SYSERR;
		}
		if (attr & 0x01) st.st_mode &= ~S_IWUSR;
		else		 st.st_mode |=  S_IWUSR;
	
		if (chmod(pathname, st.st_mode))
			return LL_E_SYSERR;
	}
#endif
	return LL_E_OK;
}



/* Get current directory */
ll_error_t ll_dosfs_getcwd(LL_DOSFS *self, char *filename)
{
	strcpy(filename, self->cwd_request);
	return LL_E_OK;
}


/* Rename a file */
ll_error_t ll_dosfs_rename(LL_DOSFS *self, const char *oldname, const char *newname)
{
	ll_error_t err;
	char oldpath[PATH_MAX];
	char newpath[PATH_MAX];

	if (!self || !oldname || !newname) return LL_E_BADPTR;
	
	err = ll_dosfs_makename(self, oldname, oldpath);
	if (err) return err;
	err = ll_dosfs_makename(self, newname, newpath);
	if (err) return err;

	LOG(("Rename file: %s [%s] -> %s [%s]\n", oldname, oldpath, newname, newpath));

	if (self->callback)
	{
		err = (*self->callback)(self->cookie, oldname);
		if (err) return err;
	}
	if (rename(oldpath, newpath)) return LL_E_SYSERR;
	else return LL_E_OK;
}


/* Get the timestamp of an open file */
ll_error_t ll_dosfs_get_ftime(LL_DOSFS *self, FILE *fp, unsigned short *date, unsigned short *time)
{
	int fd;
	struct stat st;
	struct tm *ptm;

	if (!self || !fp) return LL_E_BADPTR;

	fd = fileno(fp);
	if (fd < 0) return LL_E_BADHANDLE;

	if (fstat(fd, &st)) return LL_E_SYSERR;

	ptm = localtime(&st.st_mtime);
	*time = (ptm->tm_sec >> 1) | (ptm->tm_min << 5) | (ptm->tm_hour << 11);
	*date = (ptm->tm_mday)     | ((ptm->tm_mon + 1) << 5) | ((ptm->tm_year - 80) << 9);
	return LL_E_OK;
}


/* Set the timestamp on an open file */
ll_error_t ll_dosfs_set_ftime(LL_DOSFS *self, FILE *fp, unsigned short date, unsigned short time)
{
#ifdef HAVE_WINDOWS_H
	int fd;
	HANDLE hFile;
	FILETIME ft;

	if (!self || !fp) return LL_E_BADPTR;

	/* Get the underlying Windows file handle */
	fd = fileno(fp);
	if (fd < 0) return LL_E_BADHANDLE;
	hFile = (HANDLE)_get_osfhandle(fd);
	if (hFile == (HANDLE)-1) return LL_E_BADHANDLE;
	
	/* Convert the timestamp to Windows format */
	DosDateTimeToFileTime(date, time, &ft);
	if (!SetFileTime(hFile, NULL, NULL, &ft))
	{
		return LL_E_SYSERR;
	}
	return LL_E_OK;

#else
	int fd;
	struct timespec times[2];
	struct tm objtm;

	if (!self || !fp) return LL_E_BADPTR;

	fd = fileno(fp);
	if (fd < 0) return LL_E_BADHANDLE;

	memset(times, 0, sizeof(times));
	/* Convert DOS timestamp to UNIX timespec */
	objtm.tm_sec = 2 * (time & 0x1F);
	objtm.tm_min = (time >> 5) & 0x3F;
	objtm.tm_hour = (time >> 11) & 0x1F;
	objtm.tm_mday = (date & 0x1F);
	objtm.tm_mon  = ((date >> 5) & 0x0F) - 1;
	objtm.tm_year = ((date >> 9) & 0x7F) + 80;
	times[0].tv_sec = mktime(&objtm);

	if (futimens(fd, times)) return LL_E_SYSERR;
	return LL_E_OK;
#endif
}

#ifndef HAVE_WINDOWS_H
/* Check that a filename is valid under PCDOS rules */
static int conforms_pcdos(const char *filename, int check_case)
{
	const char *p;
	int n;

	/* In a directory search, skip over files with upper case names */
	if (check_case)
	{
		for (n = 0; filename[n]; n++)
		{
			if (isupper(filename[n])) return 0;
		}
	}
 	p = strchr(filename, '.');
	if (p)
	{
		if (p == filename) return 0;
		if ((p - filename) > 8) return 0;
		if (strlen(p + 1) > 3) return 0;
		if (strchr(p + 1, '.')) return 0;
	}
	else
	{
		if (filename[0] == 0 || strlen(filename) > 8) return 0;
	}
	if (strchr(filename, '\\') || strchr(filename, ':') || 
	    strchr(filename, '/')) return 0;
	return 1;
}
#endif


/* Given a DOS filename, convert it to a path under the root of the emulated
 * filesystem. */
static ll_error_t ll_dosfs_makename(LL_DOSFS *self, const char *dosname, char *outpath)
{
	int n;
	char *p;
	LOG(("ll_dosfs_makename(%s)\n", dosname));

	/* Reject UNC paths */
	if (ll_issep(dosname[0]) && ll_issep(dosname[1]))
	{
		return LL_E_BADNAME;
	}	
	/* Don't allow directory traversals */
	if (strstr(dosname, ".."))
	{
		return LL_E_BADNAME;
	}	

	/* If it starts with a drive, nip that off */
	if (dosname[0] != 0 && dosname[1] == ':') dosname += 2;

	/* If it now starts with a directory separator, nip that off too */
	/* and start processing at the root directory */	
	if (dosname[0] == '\\' || dosname[0] == '/') 
	{
		while(dosname[0] == '\\' || dosname[0] == '/')
			++dosname;
		strcpy(outpath, self->rootdir);	
	}
	else	/* If not, start at the CWD */
	{
		strcpy(outpath, self->cwd);	
	}
	p = outpath + strlen(outpath);	/* Point to where filename will be */
	strcat(outpath, dosname);
	/* Force all filename elements to lower case */
	while (*p)
	{
		if (isupper(*p)) *p = tolower(*p);	
		++p;
	}
	/* Convert to POSIX path separators */
	for (n = 0; outpath[n]; n++)
	{
		if (outpath[n] == '\\') outpath[n] = '/';
	}
	LOG(("ll_dosfs_makename(%s -> %s)\n", dosname, outpath));

	return LL_E_OK;
}



/* Set filesystem access callback */
ll_error_t ll_dosfs_set_callback(LL_DOSFS *self, LL_FSCALLBACK func, void *cookie)
{
	if (!self) return LL_E_BADPTR;
	self->callback = func;
	self->cookie = cookie;
	return LL_E_OK;
}

/* Get filesystem access callback */
ll_error_t ll_dosfs_get_callback(LL_DOSFS *self, LL_FSCALLBACK *func, void **cookie)
{
	if (!self) return LL_E_BADPTR;
	if (func) *func = self->callback;
	if (cookie) *cookie = self->cookie;
	return LL_E_OK;
}


 
