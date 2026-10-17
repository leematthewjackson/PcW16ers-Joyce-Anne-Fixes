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

#include "Anne.hxx"

AnneGuest::AnneGuest()
{
	memset(m_membank, 0, sizeof(m_membank));
}

AnneGuest::~AnneGuest()
{
}


void AnneGuest::service(Z80 *R)
{
	if (R->c == 5)
	{
		int n;
		for (n = 0; n < 10; n++)
		{
			m_membank[n] = R->fetchB(R->getHL() + n);
		}
		return;
	}

	PcwGuest::service(R);
}


unsigned char AnneGuest::fetch_screen(unsigned short address)
{
	/* The screen is at offset 4000h in the PcW16 RAM.*/ 
	return PCWRAM[address + 0x4000];	
}

unsigned char AnneGuest::fetch_tpa(unsigned short address)
{
//	unsigned bank = (m_membank[5 + (address >> 14)] & 0x7F);
//	fprintf(stderr, "fetch_tpa: %04x -> %02x:%04x -> %02x:%04x -> %06x -> [%02x]\n", 
//				address, 
//				address >> 14, address & 0x3FFF,
//				bank, address & 0x3FFF,
//				tpa_address(address), PCWRAM[tpa_address(address)]);
	return PCWRAM[tpa_address(address)];
}

void AnneGuest::store_screen(unsigned short address, unsigned char value)
{
	if (address < 0x9600)
	{
		PCWRAM[0x4000 + address] = value;	
	}
}
 

void AnneGuest::store_tpa(unsigned short address, unsigned char value)
{

//	unsigned bank = (m_membank[5 + (address >> 14)] & 0x7F);
//
//	fprintf(stderr, "store_tpa: %04x -> %02x:%04x\n", 
//				address, bank, address & 0x3FFF);
	PCWRAM[tpa_address(address)] = value;
}


void AnneGuest::consoleWrite(unsigned char value)
{
	// XXX Not implemented in ANNE yet
}


