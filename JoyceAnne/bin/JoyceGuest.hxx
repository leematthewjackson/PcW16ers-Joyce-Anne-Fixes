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


class JoyceGuest : public PcwGuest
{
public:
	JoyceGuest();
	virtual ~JoyceGuest();

	virtual unsigned char fetch_screen(unsigned short address);
	virtual unsigned char fetch_tpa(unsigned short address);
	virtual void store_screen(unsigned short address, unsigned char value);
	virtual void store_tpa(unsigned short address, unsigned char value);
	virtual void consoleWrite(unsigned char val);
};




