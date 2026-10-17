/* liblink: Simulate parallel / serial hardware for emulators

    Copyright (C) 2002, 2024  John Elliott <seasip.webmaster@gmail.com>

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
#include "llparshm.h"

/* How does it work?
 *
 * One end is designated Client, the other end is designated Server. Using
 * shared memory, they both access an LLPARSHM_COMMON structure. 
 *
 * The filename input is x:shmname where x is C for client, S for server.
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


static void DUMP(LL_PARSHM *trueself, const char *txt)
{
	static LLPARSHM_COMMON olddata = { 0 };
	LLPARSHM_COMMON *newdata = trueself->llpshm_common;

	if (!memcmp(newdata, &olddata, sizeof(olddata))) return;

	logfunc("%s Version: %d  Client: {%02x %08x %08x} Server: {%02x %08x %08x} Cable: %d\n",
		txt, newdata->version,
		newdata->client.data, newdata->client.ctl, newdata->client.tick,
		newdata->server.data, newdata->server.ctl, newdata->server.tick,
		newdata->cable);
	olddata = *newdata;	
}

#define LOG(x) logfunc x
*/
#define LOG(x)
#define DUMP(x, y) 


/* 
   #ifndef HAVE_WINDOWS_H
   #define LOCK	sem_wait(&trueself->llpshm_common->semaphore);
   #define UNLOCK	sem_post(&trueself->llpshm_common->semaphore);
   #endif
*/

#define LOCK
#define UNLOCK

LL_VTBL llv_parshm =
{
	sizeof(LL_PARSHM),
	"parshm",
	"Parallel port emulated using shared memory",
	llpshm_open,
	llpshm_close,
	llpshm_flush,
	llpshm_send,
	NULL,		/* fast send byte */
	llpshm_recv_poll,
	NULL,		/* wait for change (data) */
	llpshm_sctl,
	llpshm_rctl_poll,
	NULL,		/* wait for change (control) */	
	llpshm_sdctl,
	llpshm_set_cable,
	llpshm_get_cable,
};


/* Open connection */
ll_error_t llpshm_open(LL_PDEV self)
{
	LL_PARSHM *trueself;
	LL_CABLE cable;
	char *fname;
	char opt;

	if (!self || self->lld_clazz != &llv_parshm || 
	     self->lld_filename == NULL) return LL_E_BADPTR;
	trueself = (LL_PARSHM *)self;

	/* Create or open a shared memory object. The filename passed in 
	 * will be either: 
	 *  'C:filename' or 'S:filename' (for 'client' / 'server')
	 */
	fname = self->lld_filename;
	opt = toupper(fname[0]);
	if (opt != 'S' && opt != 'C') return LL_E_BADNAME;
	++fname;
	if (fname[0] != ':') return LL_E_BADNAME;
	++fname;
	/* fname points at shared memory object name */

#ifdef HAVE_WINDOWS_H
	/* Try to open existing object first */
	trueself->llpshm_handle = OpenFileMapping(FILE_MAP_ALL_ACCESS,
			FALSE, fname);
	/* If that failed create a new one */
	if (trueself->llpshm_handle == NULL)
	{
		trueself->llpshm_handle = CreateFileMapping(
			INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 
			sizeof(LLPARSHM_COMMON), fname);
	}
	if (trueself->llpshm_handle == NULL) return LL_E_SYSERR;
	trueself->llpshm_common = MapViewOfFile(trueself->llpshm_handle,
		FILE_MAP_ALL_ACCESS, 0, 0, sizeof(LLPARSHM_COMMON));
	if (trueself->llpshm_common == NULL)
	{
		CloseHandle(trueself->llpshm_handle);
		return LL_E_SYSERR;
	}	
#else	/* Windows */
	LOG(("Shared memory name: %s\n", fname));
	trueself->llpshm_handle = shm_open(fname, O_RDWR | O_CREAT, 0600);
	if (trueself->llpshm_handle < 0) 
	{
		return LL_E_SYSERR;
	}	
	if (ftruncate(trueself->llpshm_handle, sizeof(LLPARSHM_COMMON)) < 0)
	{
		return LL_E_SYSERR;
	}
	trueself->llpshm_common = mmap(NULL,
		sizeof(LLPARSHM_COMMON), PROT_READ | PROT_WRITE, MAP_SHARED,
		trueself->llpshm_handle, 0);
	if (MAP_FAILED == trueself->llpshm_common)
	{
		return LL_E_SYSERR;
	}
#endif
	LOG(("Shared memory at %p\n", trueself->llpshm_common));
	if (trueself->llpshm_common->version == 0)	/* Not initialised */
	{
	LOG(("First device accessing shared memory\n"));
		trueself->llpshm_common->version = LLPSHM_VERSION;
	}
	/* Unknown future version at the other end */
	if (trueself->llpshm_common->version > LLPSHM_VERSION)
	{
		llpshm_close(self);
		return LL_E_INCOMPAT;		
	}
	trueself->llpshm_clserver = opt;
	if ('C' == opt)
	{
		trueself->llpshm_local = &trueself->llpshm_common->client;
		trueself->llpshm_remote = &trueself->llpshm_common->server;
	}
	else
	{
		trueself->llpshm_local = &trueself->llpshm_common->server;
		trueself->llpshm_remote = &trueself->llpshm_common->client;
	}
	llpshm_get_cable(self, &cable);
	if (cable != LL_CABLE_NORMAL && cable != LL_CABLE_LAPLINK)
		llpshm_set_cable(self, LL_CABLE_NORMAL);	

	return LL_E_OK;
}


/* Close connection */
ll_error_t llpshm_close(LL_PDEV self)
{
        LL_PARSHM *trueself;

        if (!self || self->lld_clazz != &llv_parshm) return LL_E_BADPTR;
        trueself = (LL_PARSHM *)self;

#ifdef HAVE_WINDOWS_H
	UnmapViewOfFile(trueself->llpshm_common);
	CloseHandle(trueself->llpshm_handle);
#else
	munmap(trueself->llpshm_common, sizeof(LLPARSHM_COMMON));
#endif
	return LL_E_OK;
}


/* Flush data */
ll_error_t llpshm_flush(LL_PDEV self)
{
	return LL_E_OK;
}


/* Sending data */
ll_error_t llpshm_send(LL_PDEV self, unsigned char b)
{
        LL_PARSHM *trueself;

        if (!self || (self->lld_clazz != &llv_parshm)) return LL_E_BADPTR;
        trueself = (LL_PARSHM *)self;

	LOCK
	
	trueself->llpshm_local->data = b;
	++trueself->llpshm_local->tick;
	UNLOCK

	DUMP(trueself, "llpshm_send ");
	return LL_E_OK;
}


/* Setting control lines. */
ll_error_t llpshm_sctl(LL_PDEV self, unsigned b)
{
        LL_PARSHM *trueself;

        if (!self || (self->lld_clazz != &llv_parshm)) return LL_E_BADPTR;
        trueself = (LL_PARSHM *)self;

	LOCK
	trueself->llpshm_local->ctl = b;
	++trueself->llpshm_local->tick;
	UNLOCK
	return LL_E_OK;
}

/* Setting control+data lines simultaneously.  */
ll_error_t llpshm_sdctl(LL_PDEV self, unsigned char data, unsigned ctl)
{
        LL_PARSHM *trueself;

        if (!self || (self->lld_clazz != &llv_parshm)) return LL_E_BADPTR;
        trueself = (LL_PARSHM *)self;
	
	LOCK
	trueself->llpshm_local->data = data;
	trueself->llpshm_local->ctl = ctl;
	++trueself->llpshm_local->tick;

	UNLOCK
	DUMP(trueself, "llpshm_sdctl");
	return LL_E_OK;
}



ll_error_t llpshm_rctl_poll(LL_PDEV self, unsigned *b)
{
        LL_PARSHM *trueself;

        if (!self || (self->lld_clazz != &llv_parshm)) return LL_E_BADPTR;
        trueself = (LL_PARSHM *)self;

	LOCK
	if (b) *b = trueself->llpshm_remote->ctl;
	UNLOCK
	DUMP(trueself, "llpshm_rctl ");
	return LL_E_OK;
}


ll_error_t llpshm_recv_poll(LL_PDEV self, unsigned char *b)
{
        LL_PARSHM *trueself;

        if (!b || !self || (self->lld_clazz != &llv_parshm)) return LL_E_BADPTR;
        trueself = (LL_PARSHM *)self;

	LOCK
	if (b) *b = trueself->llpshm_remote->data;
	UNLOCK
	DUMP(trueself, "llpshm_recv ");
	return LL_E_OK;
}


/* Setting the cable at one end sets it at the other */
ll_error_t llpshm_set_cable(LL_PDEV self, LL_CABLE cable)
{
        LL_PARSHM *trueself;

        if (!self || (self->lld_clazz != &llv_parshm)) return LL_E_BADPTR;
        trueself = (LL_PARSHM *)self;

	LOCK
	if (trueself->llpshm_common->cable != cable)
	{
		LOG(("LLPSHM: Cable changed from %d to %d\n", 
			trueself->llpshm_common->cable, cable));
	}
	trueself->llpshm_common->cable = cable;
	UNLOCK
	DUMP(trueself, "llpshm_scabl");
	return LL_E_OK;
}


ll_error_t llpshm_get_cable(LL_PDEV self, LL_CABLE *cable)
{
        LL_PARSHM *trueself;

        if (!self || (self->lld_clazz != &llv_parshm) || !cable) return LL_E_BADPTR;
        trueself = (LL_PARSHM *)self;
	
	LOCK
	*cable = trueself->llpshm_common->cable;
	UNLOCK
	DUMP(trueself, "llpshm_gcabl");
	return LL_E_OK;
}



