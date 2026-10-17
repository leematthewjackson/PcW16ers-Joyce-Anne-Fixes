/************************************************************************

    JOYCE v2.5.3 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-2, 2024  John Elliott <seasip.webmaster@gmail.com>

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

class JoycePcwLinkit: public PcwLibLink
{
public:
	JoycePcwLinkit();
	virtual ~JoycePcwLinkit();

	zbyte	in();
	void	out(zbyte value);	
	virtual string getTitle();
	
	virtual bool hasSettings(SDLKey *key, string &caption);
	virtual UiEvent settings(UiDrawer *d);
private:
	int m_sleeps;
};




