/************************************************************************

    JOYCE v2.5.0 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-2, 2022  John Elliott <seasip.webmaster@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*************************************************************************/

class GuestSettings
{
public:
	unsigned debug;	/* Debug flags */
	std::string drivepath[16];

	GuestSettings() { debug = 0; }
};

class Guest;

/* The host presents this interface for callbacks */
class Host
{

public:
	virtual void abort(Guest *caller, int err, const char *fmt, ...) = 0;
	virtual unsigned char fetch_screen(unsigned short address) = 0;
	virtual unsigned char fetch_tpa(unsigned short address) = 0;
	virtual void store_screen(unsigned short address, unsigned char value) = 0;
	virtual void store_tpa(unsigned short address, unsigned char value) = 0;
	virtual void yield() = 0;
	virtual void consoleWrite(unsigned char ch) = 0;
};

enum GuestState
{
	GS_Stopped = 0,
	GS_Running = 1,
	GS_Terminate = 2
};

/* External interface for guests */
class Guest
{
public:
	Host *m_host;

	virtual GuestState get_state() = 0;
	virtual void set_state(GuestState st) = 0;
	virtual int reset() = 0;
	virtual void set_xferbuf(unsigned short first, unsigned short last) = 0;
	virtual void on_timer() = 0;
	virtual unsigned char peek(unsigned int offset) = 0;
	virtual void poke(unsigned int offset, unsigned char value) = 0;	
	virtual void set_resourcedirs(const char *global, 
					const char *local) = 0;
	virtual void refresh_drives(GuestSettings &g) = 0;
protected:
	Guest(Host *host)
	{
		m_host = host;
	}

};

Guest *create_i86_guest(Host *host, const char *subsys);
int i86_guest_main(void *);
Guest *create_z80_guest(Host *host, const char *subsys);
int z80_guest_main(void *);

