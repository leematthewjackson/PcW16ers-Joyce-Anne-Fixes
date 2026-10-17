
#include "z80inc.hxx"
#include "Z80.hxx"
#include "ImcZ80.hxx"
#include "GuestZ80.hxx"
#include "z80guest.hxx"

unsigned guestBank;
unsigned guestRAMsize;
zbyte *guestRAM;


GuestZ80::GuestZ80(Z80Guest *sys) : m_sys(sys)
{
/* The memory banking mechanism can support up to 16M, but for this 
 * particular use case we require only 64k */
	guestRAM = new zbyte[65536];
	guestRAMsize = 65536;
	guestBank = 0;
	m_fromBank = 0;
	m_toBank = 0;

	reset();
}

GuestZ80::~GuestZ80()
{
	delete [] guestRAM;
	guestRAM = NULL;
	guestRAMsize = 0;
	guestBank = 0;
}

void GuestZ80::reset()
{
	guestBank = 0;
	m_fromBank = 0;
	m_toBank = 0;
	ImcZ80g::reset();
}

double GuestZ80::getSpeed(bool requested) 
{
	return 0.0; 
}

void GuestZ80::setSpeed(double percent) { }

zbyte GuestZ80::gfetchB(zword addr) { return ::gfetch(addr); }
zword GuestZ80::gfetchW(zword addr) { return ::gfetch2(addr); }
void GuestZ80::gstoreB(zword addr, zbyte b) { ::gstore(addr, b); }
void GuestZ80::gstoreW(zword addr, zword w) { ::gstore2(addr, w); }
void GuestZ80::setCycles(int cycles) { }

int GuestZ80::interrupt(void) 
{ 
	return 0;
}

unsigned int GuestZ80::out(unsigned long time, zbyte hi, zbyte lo, zbyte val) 
{
	zword count, source, dest;
	zbyte tmpBank, tmp;

	switch (lo)
	{
		case 0xAB:	guestBank = val; break;
		case 0xAC:	m_fromBank = val; break;
		case 0xAD:	m_toBank = val;
				count = getBC();
				source = getDE();
				dest = getHL();
				tmpBank = guestBank;
				while (count)
				{
					guestBank = m_fromBank;
					tmp = gfetch(source);
					guestBank = m_toBank;	
					gstore(dest, tmp);
					++source;
					++dest;
					--count;
					
				}
				setBC(0);
				setDE(source);
				setHL(dest);
				guestBank = tmpBank;
				break;
		case 0xB0:	/* Load CCP */
				memcpy(guestRAM + 256 * val, m_ccp, 0x800);
				break;
		case 0xB4:	/* Fast console output */
				m_sys->m_host->consoleWrite(val);
				break;
		default:	fprintf(stderr, "GUEST OUT %02x,%02x\n",
					lo, val);
				
	}  
	return 0;
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


unsigned int GuestZ80::in(unsigned long time, unsigned long hi, unsigned long lo)
{
	switch(lo)
	{
		case 0xAB: 	return guestBank;
		case 0xAC:	return m_fromBank;
		case 0xAD:	return m_toBank;
		case 0xAE:	return m_sys->drives() & 0xFF;
		case 0xAF:	return (m_sys->drives() >> 8) & 0xFF;
/* SELDSK */
		case 0xB1:	m_curdrive = c;
				if (m_sys->has_drive(m_curdrive))
				{
// fprintf(stderr, "Emulated seldsk on drive %d %c:  DPB at %04x\n", m_curdrive, m_curdrive + 'A', getDE());

					memcpy(guestRAM + getDE(), dpb_8m, 15);
					setHL(iy);
					return 1;
				}
				return 0;
/* READ */
		case 0xB2:	if (m_sys->has_drive(m_curdrive))
				{
					unsigned offset = (getBC() * 128 + getDE()) * 128;
					unsigned addr = getHL();
// fprintf(stderr, "Emulated read from drive %d %c: at offset 0x%08x from 0x%04x\n", m_curdrive, m_curdrive + 'A', offset, addr);
					b = m_sys->read(m_curdrive, offset, &guestRAM[addr]);
					return 1;
				}
				return 0;
		case 0xB3:	if (m_sys->has_drive(m_curdrive))
				{
					unsigned offset = (getBC() * 128 + getDE()) * 128;
					unsigned addr = getHL();
// fprintf(stderr, "Emulated write to drive %d %c: at offset 0x%08x from 0x%04x\n", m_curdrive, m_curdrive + 'A', offset, addr);

					b = m_sys->write(m_curdrive, offset, &guestRAM[addr]);
					return 1;
				}
				return 0;
				
		default:	fprintf(stderr, "GUEST IN %02lx\n", lo);
	}
	return 0; 
}

void GuestZ80::EDFE(void) { }
void GuestZ80::EDFD(void) 
{
}

void GuestZ80::startwatch(int flag) { }
long GuestZ80::stopwatch(void) { return 0; }

 
