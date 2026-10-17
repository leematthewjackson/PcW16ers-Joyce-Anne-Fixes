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

class GuestImpl : public Guest
{
protected:
	FILE *m_drive[16];
	long m_filesize[16];
	bool m_ro[16];
	
public:
	GuestImpl(Host *host);
	~GuestImpl();

	GuestSettings m_settings;

	virtual void refresh_drives(GuestSettings &g);
	void closeall();
	void openall();
	unsigned short drives();
	int read(int drive, unsigned offset, unsigned char *buf);
	int write(int drive, unsigned offset, unsigned char *buf);
	bool has_drive(int drive);
};


