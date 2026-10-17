#ifndef LIBLINK_H_INCLUDED

#define LIBLINK_H_INCLUDED 1

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

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    liblink is intended for use in emulators, to simulate ports that can 
    be used to interface with other hardware. The intention is that it 
    should be able to simulate:
    
    * Parallel ports:
    	> Real parallel ports
	> Printing to file
	> Printing to a pipeline
	> Simulated LocoLink (or LapLink?) with other processes
    * Serial ports:
    	> Real serial ports
	> I/O to file
	> Loopback (eg, to simulate a serial mouse)
	> Simulated serial links to other processes

    Using the LibDsk approach of opening (filename, drivername), we have:

	"/dev/parport0",   "parport"
	"/foo/bar",        "parfile"
	"/bin/lpr",	   "parpipe"
	"tcp:S:localhost:8256", "parsocket"
	"/dev/ttyS0",      "serport"  (or "COM1:", "serport")
	"/foo/bar",        "serfile"	// need 2 filenames
	NULL,		   "serloop"
	"tcp:C:localhost:8512", "sersocket"

    Using Java syntax, we have:

	class Device
	class ParDevice extends Device
	class SerDevice extends Device
*/
#ifdef __cplusplus
extern "C" {
#endif

typedef int ll_error_t;
struct ll_device;
struct ll_pardev;
struct ll_serdev;
struct ll_parlink;
struct ll_pcwlinkit;
typedef struct ll_device *LL_PDEV;
typedef struct ll_remote *LL_PREMOTE;
typedef struct ll_pardev *LL_PPAR;
typedef struct ll_serdev *LL_PSER;
typedef struct ll_parlink *LL_PPLINK;
typedef struct ll_fserver *LL_PFS;
typedef struct ll_pcwlinkit *LL_PCWLINKIT;

typedef enum ll_protocol
{
	NO_PROTOCOL = 0,
	LOCOLINK_V1 = 1,
	LOCOLINK_V2 = 2,
	LOCOLINK_V3 = 3
} LL_PROTOCOL;

/* When emulating LocoLink, some functions need to know if they are emulating:
 * > A LocoLink interface on a PCW expansion port
 * > A LocoLink interface on a PC parallel port
 */
typedef enum ll_end
{
	LL_END_PCW = 0,
	LL_END_PC = 1,	
} LL_END;

/* Open up a device. Returns LL_E_OK if OK, else an error. 
 * On return, "self" will be a device. */
ll_error_t ll_open(LL_PDEV *self, const char *filename, const char *devtype);

/* Close the link. */
ll_error_t ll_close(LL_PDEV *self);

/* Flush any pending bytes */
ll_error_t ll_flush(LL_PDEV self);

/* Read a byte from the link, asynchronously. */
ll_error_t ll_recv_poll(LL_PDEV self, unsigned char *value);
/* Read a byte from the link, synchronously. */
ll_error_t ll_recv_wait(LL_PDEV self, unsigned char *value);
/* Send a byte down the link. */
ll_error_t ll_send(LL_PDEV self, unsigned char value);
/* Send a byte down the link, toggling the STROBE line and delaying if 
 * necessary */
ll_error_t ll_send_byte(LL_PDEV self, unsigned char value);

/* Control lines */
/* Parallel port outgoing :*/
#define LL_CTL_STROBE     1	/* STROBE */
#define LL_CTL_AUFEED     2	/* AUTO FEED XT */
#define LL_CTL_OSELECT    4	/* SELECT */
#define LL_CTL_INIT	  8	/* INIT */
/* Parallel port incoming: */
/* In LibLink these are all treated as active high, even if on the actual 
 * wire they may be active low. */
#define LL_CTL_BUSY     256	/* BUSY */
#define LL_CTL_ACK      512	/* ACK */
#define LL_CTL_NOPAPER 1024	/* NO PAPER */
#define LL_CTL_ISELECT 2048	/* SELECT */
#define LL_CTL_ERROR   4096	/* ERROR */
/* Serial port outgoing */
#define LL_CTL_DTR    65536  
#define LL_CTL_RTS   131072 
/* Serial port incoming */
#define LL_CTL_CTS 16777216
#define LL_CTL_DSR 33554432

/* Read control lines */
ll_error_t ll_rctl_poll(LL_PDEV self, unsigned *ctl);
/* Wait for control lines to change */
ll_error_t ll_rctl_wait(LL_PDEV self, unsigned *ctl);
/* Set control lines */
ll_error_t ll_sctl(LL_PDEV self, unsigned ctl);
/* Set control and data lines simultaneously */
ll_error_t ll_sdctl(LL_PDEV self, unsigned char data, unsigned ctl);
/* Get last value written to control lines */
ll_error_t ll_sctl_get(LL_PDEV self, unsigned *ctl);
/* Get last value written to data lines */
ll_error_t ll_sdata_get(LL_PDEV self, unsigned char *data);

typedef enum { LL_CABLE_NORMAL, LL_CABLE_LAPLINK } LL_CABLE;
/* Set LapLink mode. This emulates the effect of a LapLink cable 
 * between two devices, by wiring the first 5 data lines to the control
 * lines. */
ll_error_t ll_set_cable(LL_PDEV self, LL_CABLE c);
/* Get LapLink setting */
ll_error_t ll_get_cable(LL_PDEV self, LL_CABLE *c);

/* Read LocoLink control lines*/
ll_error_t ll_lrecv_poll(LL_PDEV self, LL_END end, unsigned char *v);
/* Wait for control lines to change */
ll_error_t ll_lrecv_wait(LL_PDEV self, LL_END end, unsigned char *v);
/* Set LocoLink control lines */
ll_error_t ll_lsend(LL_PDEV self, LL_END end, unsigned char v);

/* Check that a file can be accessed */
typedef ll_error_t (*LL_FSCALLBACK)(void *cookie, const char *filename);

/* Set filesystem access callback (only currently used by internal server) */
ll_error_t ll_set_fs_callback(LL_PDEV self, LL_FSCALLBACK callback, void *cookie);
/* Get filesystem access callback (only currently used by internal server) */
ll_error_t ll_get_fs_callback(LL_PDEV self, LL_FSCALLBACK *callback, void **cookie);


/* Serial functions: Setting baudrates & protocol */
typedef enum { PE_NONE, PE_EVEN, PE_ODD } LL_PARITY;

ll_error_t ll_txbaud(LL_PSER self, unsigned baud);
ll_error_t ll_rxbaud(LL_PSER self, unsigned baud);
ll_error_t ll_txbits(LL_PSER self, unsigned bits);
ll_error_t ll_rxbits(LL_PSER self, unsigned bits);
ll_error_t ll_stopbits(LL_PSER self, unsigned bits);
ll_error_t ll_parity(LL_PSER self, LL_PARITY p);

const char *ll_strerror(ll_error_t error);

/* LocoLink packet parsing. This is used to convert a bitstream from a 
 * parallel port or LocoLink interface into a LocoLink packet, and vice
 * versa. 
 *
 * The parser communicates with the host using ll_parlink_read() and
 * ll_parlink_write(). When it receives a complete packet, it calls
 * the callback.
 */
typedef ll_error_t (*LL_PCALLBACK)(void *cookie, unsigned char *input, unsigned char *output);

/* Create the parser */
ll_error_t ll_parlink_create(LL_PPLINK *pself, LL_PCALLBACK callback, void *cookie);

/* Shut down LocoLink packet parser */
ll_error_t ll_parlink_close(LL_PPLINK *pself);

/* Reset packet parser, throw away any part-sent or part-received data */
ll_error_t ll_parlink_reset(LL_PPLINK pself);

/* Host writes a value to the server */
ll_error_t ll_parlink_write(LL_PPLINK pself, unsigned char value);

/* Host reads a value from the server */
ll_error_t ll_parlink_read(LL_PPLINK pself, unsigned char *value);

/* Calculate packet checksum */
unsigned short ll_parlink_crc(unsigned char *packet);

/* LocoLink file server. Give it a packet, it will return a packet or an 
 * error.*/

/* Create the file server */
ll_error_t ll_fs_create(LL_PFS *pself, const char *dirname); 

/* Shut down the file server */
ll_error_t ll_fs_close(LL_PFS *pself);

/* File server main dispatcher */
ll_error_t ll_fs_dispatch(LL_PFS self, unsigned char *input, unsigned char *output);
/* Overload with a compatible signature for ll_parlink_create() */
ll_error_t ll_fs_dispatch_v(void *self, unsigned char *input, unsigned char *output);


/* Send a packet down the link (using the standard LocoLink protocol) */
ll_error_t ll_send_llpacket(LL_PDEV self, LL_END end, const unsigned char packet[260]);

/* Receive a packet (using the standard LocoLink protocol) */
ll_error_t ll_recv_llpacket(LL_PDEV self, LL_END end, unsigned char packet[260]);

/* LocoLink RPC functions */

/* Connect to server. */
ll_error_t llc_connect(LL_PREMOTE *premote, LL_PDEV device, LL_END end, 
			const char *client_desc, LL_PROTOCOL protocol, 
		  	char *server_desc);

/* Terminate a session */
ll_error_t llc_hangup(LL_PREMOTE *premote);

/* Called when the server is busy to keep the connection alive. 'busy' will 
 * be 1 (processing) or 0 (finished). Return 0 to continue waiting, 1 to 
 * abort the operation. */
typedef int (*LL_PING_CALLBACK)(LL_PREMOTE self, int busy);
/* Set callback handler for pings */
ll_error_t llc_set_ping_callback(LL_PREMOTE premote, LL_PING_CALLBACK cbk);
/* Get callback handler for pings */
ll_error_t llc_get_ping_callback(LL_PREMOTE premote, LL_PING_CALLBACK *cbk);

/* Get the LL_END passed to llc_connect */
ll_error_t llc_get_end     (LL_PREMOTE remote, LL_END *end);
/* Get the protocol version in use */
ll_error_t llc_get_protocol(LL_PREMOTE remote, LL_PROTOCOL *proto);


/* LocoLink 1 functions */
/* Check space for file */
ll_error_t llc_check_space(LL_PREMOTE premote, unsigned filesize, const char *filename, unsigned short *result);


/* Reset drives */
ll_error_t llc_resetdisks(LL_PREMOTE premote, int mode, unsigned short *result);
/* Select drive */
ll_error_t llc_seldrive(LL_PREMOTE premote, unsigned char drive, 
						unsigned short *result);
/* Close label */
ll_error_t llc_closelabel(LL_PREMOTE premote, unsigned short *result);

/* Get label */
ll_error_t llc_findlabel(LL_PREMOTE premote, unsigned char *label, char drive,
						unsigned short *result);
/* Delete label */
ll_error_t llc_dellabel(LL_PREMOTE premote, unsigned short *result);

/* Set label */
ll_error_t llc_makelabel(LL_PREMOTE premote, const char *label,
					unsigned short *result);
/* Rename label */
ll_error_t llc_renlabel(LL_PREMOTE premote, const char *label,
				const char *newname, unsigned short *result);

/* Get free space */
ll_error_t llc_getfreespace(LL_PREMOTE self, unsigned char drive,
				unsigned short *sectors_per_cluster, 
			 	unsigned short *free_clusters,
				unsigned short *bytes_per_sector, 
				unsigned short *total_clusters);


/* Create directory */
ll_error_t llc_mkdir(LL_PREMOTE self, const char *path, unsigned short *result);
/* Remove directory */
ll_error_t llc_rmdir(LL_PREMOTE self, const char *path, unsigned short *result);
/* Set current directory */
ll_error_t llc_chdir(LL_PREMOTE self, const char *path, unsigned short *result);
/* Create file */
ll_error_t llc_creat(LL_PREMOTE premote, unsigned filesize, unsigned short attr, const char *filename, unsigned short *result);
/* Open file */
ll_error_t llc_openfile(LL_PREMOTE self, const char *path, unsigned char mode,
			unsigned short *result);
/* Close file */
ll_error_t llc_closefile(LL_PREMOTE premote, unsigned short handle, unsigned short *result);
/* Read bytes from file */
ll_error_t llc_read(LL_PREMOTE remote, unsigned short handle, void *data, size_t len, unsigned short *result);
/* Write bytes to file */
ll_error_t llc_write(LL_PREMOTE remote, unsigned short handle, const void *data, size_t len, unsigned short *result);

/* Remove directory */
ll_error_t llc_unlink(LL_PREMOTE self, const char *path, unsigned short *result);
/* Set file position */
ll_error_t llc_lseek(LL_PREMOTE self, unsigned short handle, long offset, unsigned char whence, long *result);
/* Get file attributes */
ll_error_t llc_getattr(LL_PREMOTE self, const char *path, 
			unsigned short *result, unsigned short *attr);
/* Set file attributes */
ll_error_t llc_chmod(LL_PREMOTE self, const char *path, unsigned short attr,
			unsigned short *result);
/* Get device information */
ll_error_t llc_getdevinfo(LL_PREMOTE self, unsigned short handle, unsigned short *result);
/* Get CWD */
ll_error_t llc_getcwd(LL_PREMOTE premote, unsigned char drive, char *cwd, unsigned short *result);

/* Find first file */
ll_error_t llc_findfirst(LL_PREMOTE premote,
		const char *path, unsigned short attr, unsigned char *ffblk,
		unsigned short *result);

/* Find next file */
ll_error_t llc_findnext(LL_PREMOTE premote,
		unsigned char *ffblk, unsigned short *result);

/* Rename a file */
ll_error_t llc_rename(LL_PREMOTE self, const char *from, const char *to, unsigned short *result);
/* Get timestamp */
ll_error_t llc_getfiletime(LL_PREMOTE self, unsigned short handle, unsigned short *result, unsigned short *tm, unsigned short *dt);
/* Set timestamp */
ll_error_t llc_setfiletime(LL_PREMOTE self, unsigned short handle, 
		unsigned short tm, unsigned short dt, unsigned short *result);
/* Create a file if it does not exist */
ll_error_t llc_ccreat(LL_PREMOTE self, unsigned short attr, 
		const char *path, unsigned short *result);


/* Convert a bitmap of signal IDs to text. Allow at least 160 chars in 's' */
void ll_dump_signals(char *s, unsigned ctl);


/* PCW-Linkit functions */
ll_error_t ll_linkit_create(LL_PCWLINKIT *pself, LL_PDEV device);
ll_error_t ll_linkit_destroy(LL_PCWLINKIT *pself);

/* Connect to the other end */
ll_error_t ll_linkit_connect(LL_PCWLINKIT pself);

/* Listen for a file transfer */
ll_error_t ll_linkit_receive(LL_PCWLINKIT pself,
			     void *cookie,
				ll_error_t (*createfunc)(void *cookie, const char *filename, unsigned long size),
				ll_error_t (*writefunc) (void *cookie, unsigned char *data, unsigned long size),
				ll_error_t (*closefunc) (void *coookie));

/* Signal start of sending */
ll_error_t ll_linkit_send_begin(LL_PCWLINKIT pself);

/* Send a file */
ll_error_t ll_linkit_send_file(LL_PCWLINKIT pself, const char *filename, 
		unsigned char *data, unsigned long size, int user);
/* Send End of Transfer */
ll_error_t ll_linkit_send_eot(LL_PCWLINKIT pself);

/* Errors */
#define LL_E_OK	      (0)		/* OK */
#define LL_E_UNKNOWN  (-1)		/* Unknown error */
#define LL_E_NOMEM    (-2)		/* No memory */
#define LL_E_BADPTR   (-3)		/* Bad pointer passed in */
#define LL_E_SYSERR   (-4)		/* See strerror() */
#define LL_E_EXIST    (-5)		/* File exists */
#define LL_E_NOMAGIC  (-6)		/* Not a LocoLink shared mem file */ 
#define LL_E_CLOSED   (-7)		/* Link is closed */ 
#define LL_E_OPEN     (-8)		/* Trying to open a link that's 
					 * already open */
#define LL_E_INUSE    (-9)		/* Master cannot link to server */ 
#define LL_E_TIMEOUT  (-10)		/* Operation timed out */ 
#define LL_E_BADPKT   (-11)		/* Packet length must be 1-255 bytes */
#define LL_E_CRC      (-12)		/* Received packet has bad CRC */
#define LL_E_UNEXPECT (-13)		/* Other machine sent an unexpected */
					/* value.  */
#define LL_E_NODRIVER (-14)		/* Unknown driver name */
#define LL_E_NOTIMPL  (-15)		/* Function not implemented */
#define LL_E_NOTDEV   (-16)		/* Trying to open a file as a device */
#define LL_E_BADNAME  (-17)		/* Bad filename */
#define LL_E_PASSWORD (-18)		/* Password required & not supplied 
					 * (not yet implemented) */
#define LL_E_AGAIN    (-19)		/* Non-blocking I/O not completed */
#define LL_E_SOCKERR  (-20)		/* See hstrerror() */
#define LL_E_NOTFOUND (-21)		/* File not found */
#define LL_E_BADFUNC  (-22)		/* Function number invalid */
#define LL_E_BADHANDLE (-23)		/* Invalid handle */
#define LL_E_DENIED   (-24)		/* Access denied */
#define LL_E_TOOMANY  (-25)		/* Too many open files */
#define LL_E_NOTREADY (-26)		/* Drive not ready */
#define LL_E_FAIL     (-27)		/* LocoLink RPC reports failure */
#define LL_E_BADPROTO (-28)		/* Not possible in this version of 
					 * LocoLink protocol */
#define LL_E_NOMOREFILES (-29)		/* No more files to find */
#define LL_E_ABORT    (-30)		/* User abort */
#define LL_E_INCOMPAT (-31)		/* Trying to connect to incompatible LibLink */
#define LL_E_TOOBIG   (-32)		/* File is too big to transmit */
#define LL_E_REJECTED (-33)		/* Other machine rejected the file */

#ifdef __cplusplus
}
#endif                                                                          

#define LIBLINK_VERSION "0.0.4"

#endif /* ndef LIBLINK_H_INCLUDED */



