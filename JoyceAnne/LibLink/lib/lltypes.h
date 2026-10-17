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

/* LocoLink constants: */

/* LocoLink packet types: */
#define CONNECT_V1  0x07	/* Connect */
#define CONNECT_V2  0x0B
#define CONNECT_V3  0x5A

#define HANGUP_V1   0x14	/* Disconnect */
#define HANGUP_V2   0x22
#define HANGUP_V3   0x6D

#define CKFREE_V1   0x21	/* Check free space */
#define CREAT_V1    0x2E	/* Create file */
#define WRITE_V1    0x3B	/* Write file */
#define CLOSE_V1    0x48	/* Close file */
#define MKDIR_V1    0x6F	/* Create directory */
#define FINDFIRST_V1 0x7C	/* Find first file / dir */

#define DOSCALL_V2  0x39	/* DOS function dispatcher */
#define DOSCALL_V3  0x80

#define RESULT_V1   0x55	/* Success / fail result */
#define SUCCESS_V2  0x50	/* Success result */
#define SUCCESS_V3  0x93	
#define ERROR_V1    0x62	/* Error result */
#define ERROR_V2    0x67
#define ERROR_V3    0xA6	

#define DATAREQ_V2  0x7E	/* Server requests data */
#define DATAREQ_V3  0xB9

#define DATARET_V2  0xAC	/* Server returns data */
#define DATARET_V3  0xDF

#define DATASEND_V2 0x95	/* Client sends data */
#define DATASEND_V3 0xCC

#define ACK_V2      0xC3	/* Acknowledgement packet */
#define ACK_V3      0x18

#define PING_V2     0xDA	/* Ping (keep link alive) */
#define PING_V3     0x2B	

#define ECHO_V3	    0x05	/* Echo request (for testing) */

/* Data request / return packet types */
typedef enum reqtype
{
	ASCIIZ_DX = 0x80,	/* ASCIIZ passed in DX */
	ASCIIZ_SI = 0xA5,	/* ASCIIZ passed in SI */
	ASCIIZ_DI = 0xCA,	/* ASCIIZ passed in DI */
	FIXED_DX  = 0x11,	/* Fixed data block passed in DX */
	FIXED_DTA = 0x36,	/* Fixed data block at DTA */
	FIXED_SI  = 0x5B,	/* Fixed data block passed in SI */
	NEXTADDR  = 0xEF,	/* Next address after previous transfer */
} REQTYPE;

/* DOS functions supported by LocoLink 2.x */
#define DRV_ALLRESET 0x0D	/* Reset discs */
#define DRV_SET      0x0E	/* Select drive */
#define F_CLOSE      0x10	/* Close disk label */
#define F_SFIRST     0x11	/* Get disk label */
#define F_DELETE     0x13	/* Delete disk label */
#define F_MAKE	     0x16	/* Set disk label */
#define F_RENAME     0x17	/* Rename disk label */
#define DOS_GETFREE  0x36	/* Get free space */
#define DOS_MKDIR    0x39	/* Create directory */
#define DOS_RMDIR    0x3A	/* Remove directory */
#define DOS_CHDIR    0x3B	/* Change directory */
#define DOS_CREAT    0x3C	/* Create file */
#define DOS_OPEN     0x3D	/* Open file */
#define DOS_CLOSE    0x3E	/* Close file */
#define DOS_READ     0x3F	/* Read from file */
#define DOS_WRITE    0x40	/* Write to file */
#define DOS_UNLINK   0x41	/* Erase file */
#define DOS_LSEEK    0x42	/* Seek in file */
#define DOS_CHMOD    0x43	/* Get / set file attributes */
#define DOS_IOCTL    0x44	/* IOCTL (naturally) */
#define DOS_GETCWD   0x47	/* Get CWD */
#define DOS_SFIRST   0x4E	/* Find first file */
#define DOS_SNEXT    0x4F	/* Find next file */
#define DOS_RENAME   0x56	/* Rename a file */
#define DOS_FILETIME 0x57	/* Get/set file time */
#define DOS_CCREAT   0x5B	/* Create file if not present */
