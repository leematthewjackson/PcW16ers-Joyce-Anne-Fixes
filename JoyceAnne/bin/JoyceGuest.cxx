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

#include "Joyce.hxx"

JoyceGuest::JoyceGuest()
{
}

JoyceGuest::~JoyceGuest()
{
}


unsigned char JoyceGuest::fetch_screen(unsigned short address)
{
	if (address > 0x7800 && address < 0x8000)
	{
		return PCWRAM[0xB800 + (address & 0x7FF)];
	}
	unsigned row = (address / 90) & 0xFF;
	unsigned col = (address % 90);

	return ((JoycePcwTerm *)gl_sys->m_defaultTerminal)->fetch_screen(row, col);
}

unsigned char JoyceGuest::fetch_tpa(unsigned short address)
{
//	fprintf(stderr, "Peek TPA[0x%04x] = %02x\n", address, 
//		PCWRAM[0x10000 + (address & 0xFFFF)]);
		
	return PCWRAM[0x10000 + (address & 0xFFFF)];
}

void JoyceGuest::store_screen(unsigned short address, unsigned char value)
{
	if (address > 0x7800 && address < 0x8000)
	{
		PCWRAM[0xB800 + (address & 0x7FF)] = value;
		return;
	}
	unsigned row = (address / 90) & 0xFF;
	unsigned col = (address % 90);

	((JoycePcwTerm *)gl_sys->m_defaultTerminal)->store_screen(row, col, value);
}


void JoyceGuest::store_tpa(unsigned short address, unsigned char value)
{
//	fprintf(stderr, "TPA[0x%04x] := %02x\n", address, value);
	PCWRAM[0x10000 + (address & 0xFFFF)] = value;
}


void JoyceGuest::consoleWrite(unsigned char val)
{
	/* If VGA is active, it would be nice to send output
	 * to it. The problem is that we're not running on the
	 * main thread at this point, and so any SDL operation
	 * will crash. 
	if (gl_sys->m_activeTerminal == &gl_sys->m_termVga)
	{
		gl_sys->m_termVga.getVgaTerm() << val;
	}
        else  */
	if (gl_sys->m_fastCRT)
        {
                gl_sys->m_fastCRT->write(val);
        }
}


