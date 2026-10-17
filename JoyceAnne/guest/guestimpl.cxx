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

#include <string>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include "guest.hxx"
#include "guestimpl.hxx"

GuestImpl::GuestImpl(Host *host) : Guest(host)
{
	for (int n = 0; n < 16; n++) 
	{
		m_filesize[n] = 0;
		m_drive[n] = NULL;
		m_ro[n] = false;
	}
}


GuestImpl::~GuestImpl()
{
	closeall();
}


void GuestImpl::closeall()
{
	for (int n = 0; n < 16; n++) 
	{
		if (m_drive[n])
		{
			fclose(m_drive[n]);
			m_drive[n] = NULL;
		}
		m_filesize[n] = 0;
	}
}



void GuestImpl::refresh_drives(GuestSettings &g)
{
	closeall();
	for (int n = 0; n < 16; n++)
	{
		m_settings.drivepath[n] = g.drivepath[n];
	}
	openall();
}

void GuestImpl::openall()
{
	unsigned char buf[256];

	for (int n = 0; n < 16; n++) 
	{
		if (m_settings.drivepath[n] == "") continue;

		m_drive[n] = fopen(m_settings.drivepath[n].c_str(), "r+b");
		if (!m_drive[n])
		{
			m_drive[n] = fopen(m_settings.drivepath[n].c_str(), "w+b");
			if (m_drive[n]) 
			{
				memset(buf, 0xE5, sizeof(buf));
				fwrite(buf, 1, sizeof(buf), m_drive[n]);
			}
		}
		if (!m_drive[n])
		{
			m_drive[n] = fopen(m_settings.drivepath[n].c_str(), "rb");
			if (m_drive[n]) m_ro[n] = true;
		}
		if (m_drive[n])
		{
			fseek(m_drive[n], 0, SEEK_END);
			m_filesize[n] = ftell(m_drive[n]);
		}
		else
		{
			m_filesize[n] = 0;
		}
	}
}

unsigned short GuestImpl::drives()
{
	unsigned short r = 0, mask = 1;

	for (int n = 0; n < 16; mask = mask << 1, n++)
	{
		if (m_drive[n]) r |= mask;
	}
	return r;
}


int GuestImpl::read(int drive, unsigned offset, unsigned char *buf)
{
	int nr;

	if (!m_drive[drive]) return 1;

//	fprintf(stderr, "Read from drive %d %c: at %08x\n", drive, drive + 'A', offset);
	fseek(m_drive[drive], offset + 256, SEEK_SET);
	nr = fread(buf, 1, 128, m_drive[drive]);
	if (nr < 128)
	{
		memset(buf + nr, 0xE5, 128 - nr);
	}
	return 0;
}



int GuestImpl::write(int drive, unsigned offset, unsigned char *buf)
{
	int nr;

	if (!m_drive[drive]) return 1;
	if (m_ro[drive]) return 2;

//	fprintf(stderr, "Write to drive %d %c: at %08x\n", drive, drive + 'A', offset);
	if (m_filesize[drive] < offset + 384)
	{
		fseek(m_drive[drive], 0, SEEK_END);	
		while (m_filesize[drive] < offset + 384)
		{
			if (fputc(0xE5, m_drive[drive]) == EOF) return 1;
			++m_filesize[drive];
		}
	}
	fseek(m_drive[drive], offset + 256, SEEK_SET);
	nr = fwrite(buf, 1, 128, m_drive[drive]);
	if (nr < 128)
	{
		return 1;
	}
	return 0;
}


bool GuestImpl::has_drive(int d)
{
	if (d < 0 || d >= 16) return false;
	return (m_drive[d] != NULL);
}
