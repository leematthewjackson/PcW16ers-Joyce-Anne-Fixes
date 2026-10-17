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

#include "i86int.hxx"

IS_VOLATILE int intr;

PC_ENV::PC_ENV(Host *host) : GuestImpl(host) 
{
	m_state = GS_Stopped;
	m_timers = 0;
	m_ticks = 0;
	m_delay = 0;
	m_ram = new unsigned char[1048576];
	m_first = 0;
	m_last = 0;
	m_curdrive = -1;
	memset(&Gn_regs, 0, sizeof(Gn_regs));
	memset(&Sp_regs, 0, sizeof(Sp_regs));
	memset(&Sg_regs, 0, sizeof(Sg_regs));
	m_cputicks = 0;
	sysmode = 0;
	m_intNo = -1;
	m_intDev = NULL;
	m_mem_base = NULL;
#ifdef DEBUG
	check = 0;
	saved_ip = 0;
	saved_cs = 0;
	enc_pos = 0;
	enc_str_pos = 0;
	memset(decode_buf, 0, sizeof(decode_buf));
	memset(decoded_buf, 0, sizeof(decoded_buf));
#endif
	dbg_stream = NULL;

	reset();
	debug_init();
}

PC_ENV::~PC_ENV()
{
	delete [] m_ram;
}


GuestState PC_ENV::get_state()
{
	return m_state;
}


void PC_ENV::set_state(GuestState ps)
{
	m_state = ps;
}


u_int8 PC_ENV::fetch(u_int32 address)
{
	address &= 0xFFFFF;

	if (address < (SEG_TPA << 4) || address >= 0xFFFF0)
	{
		return m_ram[address];
	}
	if (address >= (SEG_TPA << 4) && address <= ((SEG_TPA << 4) | 0xFFFF))
	{
		return m_host->fetch_tpa(address & 0xFFFF);
	}
	if (address >= (SEG_VIDEO << 4) && address < ((SEG_VIDEO << 4) | 0xFFFF))
	{
		return m_host->fetch_screen(address & 0xFFFF);
	}
	return 0xFF;
}


void PC_ENV::store(u_int32 address, u_int8 value)
{
	address &= 0xFFFFF;

	if (address < (SEG_TPA << 4) || address >= 0xFFFF0)
	{
		m_ram[address] = value;
	}
/* Do not allow writes to the TPA outside the transfer buffer area - we don't
  want the guest to overwrite important bits of the host! */
	if (address >= ( (SEG_TPA << 4) | m_first) && 
	    address <  ( (SEG_TPA << 4) | m_last))
	{
		m_host->store_tpa(address & 0xFFFF, value);
	}
	if (address >= (SEG_VIDEO << 4) && address < (SEG_VIDEO << 4) | 0xFFF0)
	{
		m_host->store_screen(address & 0xFFFF, value);
	}
}


void PC_ENV::sys_out8(u_int16 port, u_int8 value)
{
	if (port == EMU_UC1)
	{
		m_host->consoleWrite(value);
	}
}

void PC_ENV::sys_out16(u_int16 port, u_int16 value)
{
	if (port == EMU_UC1)
	{
		m_host->consoleWrite(value);
	}
}

u_int16 PC_ENV::sys_in16(u_int16 port)
{
	switch (port)
	{
		case EMU_DRV:
			return drives();
	}
	return 0xFFFF;
}

static unsigned char dpb_8m[] = 
{
	0x80, 0x00,	/* SPT */
	0x05,		/* BSH */
	0x1F,		/* BLM */
	0x01,		/* EXM */
	0xFF, 0x07,	/* DSM */
	0xFF, 0x03,	/* DRM */
	0xFF,		/* AL0 */
	0x00,		/* AL1 */
	0x00, 0x00,	/* CKS */
	0x00, 0x00	/* OFF */
};

u_int8 PC_ENV::sys_in8(u_int16 port)
{
	unsigned short dpb;
	int n;
	unsigned char buf[128];

	switch (port)
	{
		case EMU_SD:
			m_curdrive = R_CL;
			if (has_drive(m_curdrive))
			{
//fprintf(stderr, "Emulated seldsk on drive %d %c:  DPB at %05x\n", m_curdrive, m_curdrive + 'A', R_DS * 16 + R_DI);
//	m_settings.debug |= DEBUG_DECODE_F | DEBUG_TRACE_F;
				dpb = R_DS * 16 + R_DI;
				for (n = 0; n < 15; n++)
				{
					store(dpb + n, dpb_8m[n]);
				}	
				return 1;
			}			
			return 0;	/* Use host */
		case EMU_RD:
			if (has_drive(m_curdrive))
			{
				unsigned offset = (R_BX*128+R_CX) * 128;
				unsigned addr =   (R_DI *16+R_SI);
//fprintf(stderr, "Emulated read from drive %d %c: at offset 0x%08x to %04x:%04x 0x%05x\n", m_curdrive, m_curdrive + 'A', offset, addr);
//	m_settings.debug |= DEBUG_DECODE_F | DEBUG_TRACE_F;
				R_AH = read(m_curdrive, offset, buf);
				for (int n = 0; n < 128; n++)
				{
					store(addr + n, buf[n]);
				}
				return 1;	
			}
			return 0;	/* Use host */
		case EMU_WR:
			if (has_drive(m_curdrive))
			{
				unsigned offset = (R_BX*128+R_CX) * 128;
				unsigned addr =   (R_DI *16+R_SI);
//fprintf(stderr, "Emulated write to drive %d %c: at offset 0x%08x from 0x%05x\n", m_curdrive, m_curdrive + 'A', offset, addr);
//	m_settings.debug |= DEBUG_DECODE_F | DEBUG_TRACE_F;
				for (int n = 0; n < 128; n++)
				{
					buf[n] = fetch(addr + n);
				}
				R_AH = write(m_curdrive, offset, buf);
				return 1;
			}
			return 0;	/* Use host */
	
	}

	return 0xFF;
}


void PC_ENV::sys_lock()
{
}
Guest *create_i86_guest(Host *host, const char *subsys)
{
	return new PC_ENV(host);
}

int i86_guest_main(void *data)
{
	PC_ENV *self = (PC_ENV *)data;

	return self->i86_exec();	
}

void PC_ENV::set_resourcedirs(const char *global, const char *local)
{
	m_res_global = global;
	m_res_local = local;
}

int PC_ENV::loadres(std::string filename, unsigned addr)
{
	std::string path;
	FILE *fp = NULL;

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
	fread(m_ram + addr, 1, 0xFFFF, fp);
	fclose(fp);
	return 0;
}

int PC_ENV::reset()
{	
	int x;

	m_curdrive = 0;
	m_state = GS_Stopped;
	m_host->yield();
	R_CS = 0xFFFF;
	R_IP = 0x0000;
/* Initialise RAM to 0xCDCD (INT CD) which should trigger the 'unknown 
 * interrupt' error if hit */
	memset(m_ram, 0xCD, 1048576);

	m_ram[0xFFFF0] = 0xEA;
	m_ram[0xFFFF1] = 0x00;
	m_ram[0xFFFF2] = 0x25;
	m_ram[0xFFFF3] = 0x51;
	m_ram[0xFFFF4] = 0x00;	/* JMP 51:2500 */

	x = loadres("cpm86.sys", 0x490);
	if (x) return x;
	x = loadres("cpm86bios.bin", 0x2A10);
	if (x) return x;
	memset(m_ram + 0x490, 0, 0x80);	/* Zap the CMD header */

	/* Store the transfer buffer addresses in the BIOS */
	m_ram[0x510 + 0x255A] = m_first & 0xFF;
	m_ram[0x510 + 0x255B] = m_first >> 8;	
	m_ram[0x510 + 0x255C] = SEG_TPA & 0xFF;
	m_ram[0x510 + 0x255D] = SEG_TPA >> 8;	
	m_ram[0x510 + 0x255E] = m_last & 0xFF;
	m_ram[0x510 + 0x255F] = m_last >> 8;	

//	m_settings.debug |= DEBUG_DECODE_F | DEBUG_TRACE_F;
	return 0;
}

u_int8 PC_ENV::peek(unsigned offset)
{
	return fetch(offset);	
}

void PC_ENV::poke(unsigned offset, u_int8 value)
{
	store(offset, value);
}

void PC_ENV::set_xferbuf(unsigned short first, unsigned short last)
{
	m_first = first;
	m_last = last;
}


PC_ENV *PC_ENV::clone()
{
	PC_ENV *other = new PC_ENV(m_host);

	copy_to(other);
	return other;
}


/* Note that this is a shallow copy! */
void PC_ENV::copy_to(PC_ENV *other)
{
	other->Gn_regs = Gn_regs;
	other->Sp_regs = Sp_regs;
	other->Sg_regs = Sg_regs;
	other->m_settings = m_settings;
	other->sysmode = sysmode;
	other->m_intNo = m_intNo;
	other->m_intDev = m_intDev;
	other->m_mem_base = m_mem_base;
	other->dbg_stream = dbg_stream;
#ifdef DEBUG
	other->check = check;
	other->saved_ip = saved_ip;
	other->saved_cs = saved_cs;
	other->enc_pos = enc_pos;
	other->enc_str_pos = enc_str_pos;
	memcpy(other->decode_buf, decode_buf, sizeof(decode_buf));
	memcpy(other->decoded_buf, decoded_buf, sizeof(decoded_buf));
#endif
	other->m_first = m_first;
	other->m_last = m_last;
}


void PC_ENV::clock_tick()
{
	m_host->yield();
}


bool PC_ENV::illegal_op()
{
	i86_intr_raise(0xD2);
	return false;
}

void PC_ENV::on_timer()
{
	i86_intr_raise(0xD1);
}


bool PC_ENV::halted()
{
/* Call the host and ask its opinion? */
	return false;
}





