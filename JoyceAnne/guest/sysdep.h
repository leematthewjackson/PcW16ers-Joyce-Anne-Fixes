
/* 
 * Dos/PC Emulator
 * Copyright (C) 1991 Jim Hudgens
 * 
 * 
 * The file is part of GDE.
 * 
 * GDE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 * 
 * GDE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GDE; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  
 *
 */

#ifndef SYSDEP_H
#define SYSDEP_H

static char sysdep_h_rcsid[]=
  "$Id: $";


/* $Log: $
 *
 */

#define DEBUG 1	/* Build an unoptimised GDE core */


#ifndef DEBUG
/* TESTED SPEEDUPS */
#define INLINE_DECODE_MODRM
#define QUICK_FETCH
#define QUICK_DECODE_RM
#define UNPACKED_FLAGS
#endif

#include "../../config.h"
/* Replaced the multiple-stanza structure of this file with an autoconf-ish
 * version. */

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#define MAXPATHLEN _MAX_PATH
#define A_DRIVE "A:"
#define B_DRIVE "B:"
#else
#define A_DRIVE "/dev/fd0"
#define B_DRIVE "/dev/fd1"
#endif

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_DIR_H
#include <dir.h>
#endif
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

// STL headers
#include <string>
#include <vector>

// SDL for output
#include <SDL.h>
#include <SDL_thread.h>

// libdsk
#include <libdsk.h>
#include <765.h>

// XML parser
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include "../../bin/UiTypes.hxx"

using namespace std;	/* ministl is in the global namespace. If we're 
			   using the proper STL, bring it in as well. */

#define STRERROR(y)  strerror(y)

typedef   char  int8;
typedef   long  int32;
typedef   short int16;   
typedef   unsigned long  u_int32;
typedef   unsigned short u_int16;
typedef   unsigned char  u_int8;

#define IS_VOLATILE volatile

#ifndef NULL
#define NULL  0
#endif

#endif	/* def SYSDEP_H */
