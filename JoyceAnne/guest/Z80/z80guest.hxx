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


class Z80Guest : public GuestImpl
{
	string	m_res_global, m_res_local;
	short m_first, m_last;

	int loadres(std::string filename, unsigned addr, bool bPRL);
	int loadprl(FILE *fp, unsigned addr);
public:
	Z80Guest(Host *host);
	~Z80Guest();

	virtual GuestState get_state();
	virtual void set_state(GuestState st);
	virtual int reset();
	virtual void set_xferbuf(unsigned short first, unsigned short last);
	virtual void on_timer();
	virtual zbyte peek(unsigned offset);
	virtual void poke(unsigned offset, zbyte value);
	virtual void set_resourcedirs(const char *global, const char *local);
	int execute(void);
	
	GuestZ80 m_z80;	

};


