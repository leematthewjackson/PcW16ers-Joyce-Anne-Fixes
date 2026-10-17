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

#include "z80inc.hxx"
#include "Z80.hxx"
#include "ImcZ80.hxx"
#include "GuestZ80.hxx"
#include "z80guest.hxx"

#define BIOS_BASE 0xE600

Host *gl_host;

Z80Guest::Z80Guest(Host *host) : GuestImpl(host), m_z80(this)
{
	m_first = 0;
	m_last = 0;
}


Z80Guest::~Z80Guest()
{
}



Guest *create_z80_guest(Host *host, const char *subsys)
{
	return new Z80Guest(host);
}

int Z80Guest::execute()
{
	gl_host = m_host;
	m_z80.mainloop(BIOS_BASE, 0);
	return 0;
}


int z80_guest_main(void *data)
{
	Z80Guest *self = (Z80Guest *)data;

	return self->execute();	
}



GuestState Z80Guest::get_state()
{
	return m_z80.m_state;
}

void Z80Guest::set_state(GuestState st)
{
	m_z80.set_state(st);
}

void Z80Guest::set_resourcedirs(const char *global, const char *local)
{
	m_res_global = global;
	m_res_local = local;
}

int Z80Guest::loadprl(FILE *fp, unsigned addr)
{
	zbyte prl_header[256];
	zword len;
	unsigned n, a, mask;
	int c;

	/* Load the PRL header */
	if (fread(prl_header, 1, 256, fp) < 256) return -2;
	len = prl_header[1] + 256 * prl_header[2];
	/* Load the bytes */
	if (fread(guestRAM + addr, 1, len, fp) < len)
	{
		return -2;
	}
	/* Load and apply the relocation map */
	for (n = 0; n < len; n += 8)
	{
		c = fgetc(fp);
		if (c == EOF) return -2;
		for (a = 0, mask = 0x80; a < 8; a++, mask = mask >> 1)
		{
			if ((c & mask) && (n + a < len))
			{
				guestRAM[addr + n + a] += ((addr >> 8) - 1);
			}
		}
	}	
	return 0;
}



int Z80Guest::loadres(std::string filename, unsigned addr, bool bPRL)
{
	std::string path;
	FILE *fp = NULL;
	long pos;
	unsigned len;

	if (m_res_local == "" && m_res_global == "")
	{	
		return -2;	/* Don't even bother */
	}

	if (m_res_local != "")
	{
		path = m_res_local;
		path += "/";
		path += filename;
		fp = fopen(path.c_str(), "rb");
	}
	if (fp == NULL && m_res_global != "")
	{
		path = m_res_global;
		path += "/";
		path += filename;
		fp = fopen(path.c_str(), "rb");
	}
	if (fp == NULL)
	{
		fprintf(stderr, "Could not locate resource: %s\n",
				filename.c_str());
		return -2;
	}
	if (bPRL)
	{
		int r = loadprl(fp, addr);
		fclose(fp);
		return r;
	}
	else
	{
		fseek(fp, 0, SEEK_END);
		pos = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		len = pos;
		if (addr + len > 0x10000) 
		{
			len = 0x10000 - addr;
			fprintf(stderr, "Truncating BIOS load at 0x%04x bytes\n", len);
		}	
		fread(guestRAM + addr, 1, len, fp);
		fclose(fp);
		return 0;
	}
}



int Z80Guest::reset()
{
	m_z80.reset();
	m_z80.m_curdrive = 0;
	set_state(GS_Stopped);
	m_host->yield();
// * Clear memory
	memset(guestRAM, 0xC7, guestRAMsize);
	memset(guestRAM, 0, 0x5C);	/* Blank the Zero Page */
	guestRAM[3] = 0x95;		/* LST:=LPT CON:=CRT PUN:=PTP RDR:=PTR*/
	m_z80.pc = BIOS_BASE;
	m_z80.sp = 0;

// * Load BIOS, BDOS and CCP

	if (loadres("os2ccp.prl", BIOS_BASE - 0x1600, true)) return -2;
	if (loadres("os3bdos.prl", BIOS_BASE - 0xE00, true)) return -2;
	if (loadres("cpm2bios.bin", BIOS_BASE, false)) return -2;

	/* Save a copy of the CCP for later */
	memcpy(m_z80.m_ccp, guestRAM + (BIOS_BASE - 0x1600), 0x800);

	/* Set transfer buffer address */
        guestRAM[BIOS_BASE + 0x64] = m_first & 0xFF;
        guestRAM[BIOS_BASE + 0x65] = m_first >> 8;

	FILE *fp = fopen("z80.ram", "wb");
	fwrite(guestRAM, 1, 0x10000, fp);
	fclose(fp);
	return 0;
}


void Z80Guest::set_xferbuf(unsigned short first, unsigned short last)
{
	m_first = first;
	m_last = last;
}

void Z80Guest::on_timer()
{

}

zbyte Z80Guest::peek(unsigned offset)
{
	if (offset < guestRAMsize)
	{
		return guestRAM[offset];
	}	
	else if (offset >= 0xFE0000 && offset < 0xFF0000) 	
	{
		return gl_host->fetch_screen(offset & 0xFFFF);
	}
	else if (offset >= 0xFF0000 && offset < 0x1000000) 	
	{
		return gl_host->fetch_tpa(offset & 0xFFFF);
	}
	return 0xFF;
}

void Z80Guest::poke(unsigned offset, zbyte value)
{
	if (offset < guestRAMsize)
	{
fprintf(stderr, "Z80Guest::poke(%04x,%02x)\n", offset, value);
		guestRAM[offset] = value;
	}	
	else if (offset >= 0xFE0000 && offset < 0xFF0000) 	
	{
		gl_host->store_screen(offset & 0xFFFF, value);
	}
	else if (offset >= 0xFF0000 && offset < 0x1000000) 	
	{
		return gl_host->store_tpa(offset & 0xFFFF, value);
	}
}



