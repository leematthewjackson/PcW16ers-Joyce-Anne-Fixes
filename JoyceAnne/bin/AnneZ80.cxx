/************************************************************************

    JOYCE v2.1.2 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-3  John Elliott <seasip.webmaster@gmail.com>

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
#include "PcwFid.hxx"
#include "PcwBdos.hxx"
#include "inline.hxx"

void ResetPcw(void);
void LoadBoot(void);

#define GRANULARITY   100.0	// Check timing every 100ms
#define G_TICKS	       360 	// == GRANULARITY * 90% * 4

int gl_cputrace = 0;

/* These functions are used to display all daisywheel I/O */
AnneZ80::AnneZ80(AnneSystem *s)
{
	XLOG("AnneZ80::AnneZ80");
	m_msecs = (int)GRANULARITY;
	m_sys = s;	
}

void AnneZ80::main_every(void)
{
	if (m_sys->getResetFlag())
	{
		/* set Program Counter to 0000H */
		reset();
		m_sys->reset();
	}
/*
 // Uncomment & modify this code to monitor a memory location
 //
	{
		static word w = 0;
		word f2 = PCWRAM[0x1024] + 256 * PCWRAM[0x1025];
	
		if (w != f2)
		{
			fprintf(stderr, "1024: %04x to %04x\n", w, f2);
			w = f2;
		}
	
*/

 // Uncomment & modify this code for excruciating traces of what the Z80 is
 // doing.
	{
		static int id = 0;
		static int isr = 0;
		static zbyte bchk = 0;	

/*		switch(pc)
		{

			case 0x757A: fprintf(stderr, "ANNE LLK attempt connect\n"); break;
			case 0x75D4: fprintf(stderr, "ANNE LLK send packet\n"); break;
			case 0x7628: fprintf(stderr, "ANNE LLK send packet error\n"); break;
			case 0x7635: fprintf(stderr, "ANNE LLK send packet fail\n"); break;
			case 0x763B: fprintf(stderr, "ANNE LLK receive packet\n"); break;
			case 0x7962: fprintf(stderr, "ANNE LLK send packet prelude\n"); break;
			case 0x796B: fprintf(stderr, "ANNE LLK await 2->3\n"); break;
			case 0x7978: fprintf(stderr, "ANNE LLK write byte %02x\n", a); break;
			case 0x7990: fprintf(stderr, "ANNE LLK await 3->1\n"); break;
			case 0x79C8: fprintf(stderr, "ANNE LLK await 1->3\n"); break;
			case 0x7A07: fprintf(stderr, "ANNE LLK await change:\n"
				"%02x %02x %02x %02x\n"
				"%02x %02x %02x %02x\n"
				"%02x %02x %02x %02x\n"
				"%02x %02x %02x %02x\n",
			fetch(getHL()), fetch(getHL()+1), fetch(getHL()+2), fetch(getHL()+3), 
			fetch(getHL()+4), fetch(getHL()+5), fetch(getHL()+6), fetch(getHL()+7), 
			fetch(getHL()+8), fetch(getHL()+9), fetch(getHL()+10), fetch(getHL()+11), 
			fetch(getHL()+12), fetch(getHL()+13), fetch(getHL()+14), fetch(getHL()+15)); break;

			case 0x7A16: fprintf(stderr, "ANNE LLK got %02x HL->%02x %02x %02x %02x\n", a,
	fetch(getHL()), fetch(getHL()+1), fetch(getHL()+2), fetch(getHL()+3)); break;
			case 0x7A27: fprintf(stderr, "ANNE LLK await transkey=%d\n", a); break;
			case 0x7A2D: fprintf(stderr, "ANNE LLK await (on old value)\n"); break;
			case 0x7A51: fprintf(stderr, "ANNE LLK await (result) Cy=%d\n", f & 1); break;
		}

//		if (pc == 0x7962) gl_cputrace = 1;
//		if (pc == 0x7976) gl_cputrace = 0;
//		if (pc == 0x7166) gl_cputrace = 1;
//		if (pc == 0xF186 && fetch(0xFA00)==0xC3 && fetch(0xFA03)==0xC3
//		&&  gl_cputrace == 0) gl_cputrace = 1; 
//		if (pc == 0xF187 && fetch(0xFA00)==0xC3 && fetch(0xFA03)==0xC3
//		&&  gl_cputrace == 1) gl_cputrace = 0; 
//		if (pc == 0xF2A7 && fetch(0xFA00)==0xC3 && fetch(0xFA03)==0xC3
//		&&  gl_cputrace == 0) gl_cputrace = 1; 
//		if (pc == 0xF2B1 && fetch(0xFA00)==0xC3 && fetch(0xFA03)==0xC3
//		&&  gl_cputrace == 1) gl_cputrace = 0; 
		//if (pc == 0x62c7 && anneMemIO[1] == 0x11) 


		if (pc == 0xC349 && anneMemIO[3] == 7) 
		{
			++gl_cputrace;
		}

		if (pc == 0xb768 && anneMemIO[2] == 8) 
		{
			++gl_cputrace;
		}
		if (pc == 0xb789 && anneMemIO[2] == 8) 
		{
			--gl_cputrace;
		}

		if (pc == 0x38    && gl_cputrace > 0)
		{
			++isr;
			--gl_cputrace;
		}

		if (bchk != PCWRAM[0x2b919]) //ramA:3919	
		{
			printf("RamA:B919 PC=%04x %02x -> %02x\n",
				pc, bchk, PCWRAM[0x2b919]);
			bchk = PCWRAM[0x2b919];
		}
//		static byte b = 0;
//
*/
//		if (gl_cputrace >= 3) ++id;
		if (gl_cputrace >= 1) 
		{
		fprintf(stderr,"PC=%04x %02x AF=%02x:%02x BC=%04x DE=%04x HL=%04x IX=%04x IY=%04x SP=%04x %c |%02x%02x%02x%02x\n",
			pc, fetch(pc), a,f, getBC(), getDE(), getHL(), 
			ix, iy, sp, (f & 1) ? 'C' : ' ', 
			anneMemIO[0], anneMemIO[1], anneMemIO[2], anneMemIO[3]);
		}
/*
		if (pc == 0x187A  && isr > 0)
		{
			--isr;
			++gl_cputrace;
		}
		{
			static byte q[20];
			if (memcmp(PCWRAM + 0x29850, q, 20))
			{
			memcpy(q, PCWRAM + 0x29850, 20);
			printf("        pc=%04x ", pc);
			for (int p = 0; p < 20; p++)
				printf("%02x ", PCWRAM[p + 0x29850]);
			printf("\n");
			fflush(stdout);
			}
		} 
	//	{
	//		joyce_dprintf("%d: PC=%04x 1079: %02x -> %02x\n",
	//			id, pc,	b, PCWRAM[0x1079]);
	//		b = PCWRAM[0x1079];
	//	}
*/	}

	m_sys->m_fdc->tick();	// Check for FDC interrupt
	m_sys->m_com1.tick();	// Check for serial interrupts
	m_sys->m_com3.tick();
	m_sys->m_activeTerminal->tick();	
}

void AnneZ80::reset(void)
{
	m_avgSpeed = 0;
        m_cenCount = 0;
	m_ticksStarted = 0;
	m_ticksStopped = 0;
	m_ticksElapsed = 0;
	ImcZ80::reset();
	if (m_sys->getArgs()->m_warmStart) 
	{
		out(0, 0x00, 0xF0, 0x80);	// RAM into bottom 3rd
		sp = 0x4000;
		pc = 0x4018;
		memset(PCWRAM, 0, 1024);
		PCWRAM[6] = 0x80;
	}	
}


//
// Support the 0xED 0xFD pseudo-op
//
void AnneZ80::EDFD()
{
	/* Accelerate ROSANNE calls */
	if      (pc <  0x46 && pc >= 0x43)	m_sys->m_accel.edfd(this);
	else if (pc >= 0x80 && pc < 0x13D)	m_sys->m_accel.edfd(this);
	else if (pc == 0x01DC || pc == 0x41DC)	m_sys->m_accel.edfd(this);
	/* Accelerated printer I/O */
	else if (pc < 0xFC12 && pc >= 0xFC0F)     m_sys->m_activePrinter->writeChar(c); // LIST
	else if (pc < 0xFC30 && pc >= 0xFC2D) a = m_sys->m_activePrinter->isBusy() ? 0 : 1; // LISTST

	else if (pc < 0xFC51 && pc >= 0xFC4E) /* TIME */
	{
		zword temp = de;

		setDE(0xFBF4);
		jdos_time(this, 1);
		setDE(temp);
	}
	else 
	{
		fprintf(stderr, "Unknown EDFD call: pc=%x\n", pc);
	}
}




/****************************************************************/
/*** Extended JOYCE instructions. This is called on the ED FE ***/
/*** instruction to emulate anything that distinguishes JOYCE ***/
/*** from a real PCW					      ***/
/****************************************************************/

void AnneZ80::EDFE(void)
{
	switch(a)
	{
		case 0x00:	/* Get ANNE version */
			a = 0xFF;	/* Utility compatibility number. 
					 * 0  for a real Z80
					 * -1 for Anne 1.00+ */
			setHL(BCDVERS);	/* JOYCE version */
			setDE('A');	/* Emulator ID */
			break;

		case 0x01:
/*
       A=1:  Boot. This has no meaning on a PCW16, so we make it into a 
	    conventional reboot.
*/
			m_sys->setResetFlag();
			break;

		case 0x02: 
/*
	A=2: Set colour. BCD = 'white' RGB
			 EHL = 'black' RGB
*/
			m_sys->m_activeTerminal->setForeground(b,c,d);
			m_sys->m_activeTerminal->setBackground(e,h,l);	
			break;

/* 
	A=3 - A=7: Service a call from a disc driver FID

*/
		case 0x03:
			m_sys->m_hdc.fidLogin(this);
			break;
		case 0x04:
			m_sys->m_hdc.fidRead(this);
			break;
		case 0x05:
			m_sys->m_hdc.fidWrite(this);
			break;
		case 0x06:
			m_sys->m_hdc.fidFlush(this);
			break;
		case 0x07:
			m_sys->m_hdc.fidEms(this);
			break;
		case 0x08:		/* A=8: Save settings */
			m_sys->storeHardware();
			m_sys->saveHardware();
			break;
		case 0x09:		/* A=9: Switch which LPT we're using */
			setHL(0); 
			break;
		case 0x0A:		/* A=10: Access DOS filesystem */
			jdos(this, m_sys);
			break;
		case 0x0B:
			fid_char(this);	/* A=11: Screen control FID */
			break;
		case 0x0C:
			m_sys->m_activeKeyboard->edfe(this);
					/* A=12: low-level keyboard mapping */
			break;
		case 0x0D:
			fid_com(this);	/* (v1.21) A=13: COMPORT.FID */
			break;
		case 0x0E:
			m_sys->m_hdc.fidMessage(this); /* (v1.22) FID disc messages */
			break;
		case 0x0F:		/* (v1.22) LOGO */
			m_sys->m_termLogo.drLogo(this);
			break;
		case 0x10:		/* (v1.22) parse BOOT/BOOT.CFG */
			m_sys->getBootID(this);
			break;
		case 0x11:		/* (v2.5.0+) CP/M-86 emulation */
			m_sys->m_guest->service(this);
			break;

		case 0xFA:
			++gl_cputrace;
			break;
		case 0xFB:
			--gl_cputrace;
			break;
		case 0xFC:		/* (v1.30) CPMREDIR interface */
			cpmredir_intercept(this);
			break;
		case 0xFD:		/* Mouse intercept patch */
			m_sys->m_activeMouse->edfe(this);
			break;
		case 0xFE:		/* Set JOYCE timing parameters */
			setHL(m_cycles);
 			a = ((AnneTerminal *)m_sys->m_defaultTerminal)->getRefresh();
			if (c) ((AnneTerminal *)m_sys->m_defaultTerminal)->setRefresh(c);
			if (de) m_cycles = de;
			break;
		case 0xFF:		/* Halt & catch fire */
			goodbye(hl);
			break;

		default:		/* Invalid ED xx */
			joyce_dprintf("Unknown operation: ED FE with A=%02x at PC=%04x",
				a, pc);
			break;
	}
	return;
}


/****************************************************************/
/*** Write byte to given IO port.			      ***/
/****************************************************************/
unsigned AnneZ80::out(unsigned long time,
		  zbyte hi, zbyte lo, zbyte val)
{
	if ((lo & 0xF8) == 0x38) 
	{
		m_sys->m_lpt1.out(lo, val);
		return 0;
	}
	if ((lo & 0xF8) == 0x28) 
	{
		m_sys->m_com1.out(lo, val);
		return 0;
	}
	if ((lo & 0xF8) == 0x20) 
	{
		m_sys->m_com3.out(lo, val);
		return 0;
	}
	if ((lo & 0xF0) == 0xE0)
	{
		((AnneTerminal *)m_sys->m_defaultTerminal)->out(lo, val);
		return 0;
	}
	if ((lo & 0xFF) >= 0xF9)
	{
		m_sys->m_rtc.out(lo, val);
		return 0;
	}
	switch(lo & 0xFF)
	{
		case 0x1A: // FDC DOR
/*			   ++gl_cputrace;
			   {
			   FILE *fp = fopen("anne.ram", "wb");
			   for (int n = 0; n < 65536; n++)
			   {
				fputc(fetch(n), fp);	
			   }
			   fclose(fp); } */
			   m_sys->m_fdc->writeDOR(val); break;
		case 0x1C: break;
		case 0x1D: m_sys->m_fdc->writeData(val); break;
		case 0x1F: m_sys->m_fdc->writeDRR(val); break;

		case 0xF0: /* Memory paging */
		case 0xF1:
		case 0xF2:
		case 0xF3: m_sys->m_mem->out(lo, val); break;

		case 0xF4: 
		case 0xF5: m_sys->m_keyb.out(lo, val); break;
                case 0xF7: m_sys->m_activeTerminal->out(lo, val); break;
                case 0xF8: // System control
                           ((AnneAsic *)m_sys->m_asic)->outF8(val); break;

		default:
		//joyce_dprintf("OUT 0x%02x, 0x%02x at PC=%04x\n", lo, val, pc);
		printf("OUT 0x%02x, 0x%02x at PC=%04x\n", lo, val, pc);
		break;
	}
	return 0;
}

/****************************************************************/
/*** Read byte from given IO port.			      ***/
/****************************************************************/
unsigned AnneZ80::in(unsigned long time, unsigned long hi, unsigned long lo)
{
	if ((lo & 0xF8) == 0x38) 
	{
		return m_sys->m_lpt1.in(lo);
	}
	if ((lo & 0xF8) == 0x28) 
	{
// Allow the serial mouse to override the behaviour of a serial port
		if (m_sys->m_mouse.m_basePort == 0x28)
			return m_sys->m_mouse.in(lo);
		else	
/*			{
			byte b = m_sys->m_com1.in(lo);
        FILE *fp = fopen("serout.log", "a");
        fprintf(fp, "%04x: AnneSerial::in(%x)=%x\n", gl_sys->m_cpu->pc, lo, b);
        fclose(fp);
			return b;
			}
			*/return m_sys->m_com1.in(lo);
	}
	if ((lo & 0xF8) == 0x20) 
	{
// Allow the serial mouse to override the behaviour of a serial port
		if (m_sys->m_mouse.m_basePort == 0x20)
			return m_sys->m_mouse.in(lo);
		else	return m_sys->m_com3.in(lo);
	}
	if ((lo & 0xFF) >= 0xF9)
	{
		return m_sys->m_rtc.in(lo);
	}
	switch (lo)
	{
		case 0x1C: return m_sys->m_fdc->readControl(); // FDC status
		case 0x1D: return m_sys->m_fdc->readData();    // FDC data
		case 0x1F: return m_sys->m_fdc->readDIR()      // FDC digital input register
			// | m_sys->m_ide->readDIR();	       // IDE digital input register
			| 0x7F;				       // Value with no IDE
		case 0xF0: /* Memory paging */
		case 0xF1:
		case 0xF2:
		case 0xF3: return m_sys->m_mem->in(lo); 
		case 0xF4: 
		case 0xF5: return m_sys->m_keyb.in(lo); 
		case 0xF7: return ((AnneAsic *)m_sys->m_asic)->inF7(); 
		case 0xF8: return ((AnneAsic *)m_sys->m_asic)->inF8(); 
	}
//	printf("IN(%02x) at PC=%04x\n", lo, pc);
//	joyce_dprintf("IN(%02x) at PC=%04x\n", lo, pc);
	return 0xFF;
}


void AnneZ80::startwatch(int flag) 
{ 
	/* If restarted, add up running total */
	if (flag && (m_ticksStopped > m_ticksStarted))
	{
		m_ticksElapsed += (m_ticksStopped - m_ticksStarted);
	} 
	m_ticksStarted = SDL_GetTicks();
	m_ticksStopped = 0;	// Running
}


long AnneZ80::stopwatch(void) 
{
	m_ticksStopped = SDL_GetTicks(); 
	return 0; 
}

double AnneZ80::getSpeed(bool requested)
{
	if (requested)
	{
		if (!m_msecs) return 0;
		return ((GRANULARITY * 100.0) / m_msecs);
	} 
	return m_avgSpeed * 100.0;
}


void AnneZ80::setSpeed(double percent)
{
	if (!percent) 
	{
		m_msecs = 0;	// 0% speed means maximum.
		return;
	}
	double msecs = (GRANULARITY * 100.0) / percent;
	m_msecs = (int)msecs;
}



/****************************************************************/
/*** This function is called at 900Hz. It has to implement    ***/
/*** all PCW features which rely on time.		      ***/
/****************************************************************/
int AnneZ80::interrupt(void)
{
	if (extern_stop) return Z80_quit;

	++m_cenCount;

	// Handle any waiting SDL events.
	m_sys->eventLoop();

	// Note that these events are all included in startwatch() / 
	// stopwatch() checks. If they are going to do anything time-
	// consuming it is up to them to do their own stopwatch() or 
	// startwatch().
	//
	m_sys->m_activeTerminal->onScreenTimer();
	if (m_sys->getDebug()) m_sys->m_termDebug.onScreenTimer();

	// Poll the keyboard
	m_sys->m_activeKeyboard->poll(this);

	// Do ASICy things
	((AnneAsic *)m_sys->m_asic)->onAsicTimer();

	// Make the RTC tick
	m_sys->m_rtc.tick();

	if (m_cenCount > 27000) 
	{
		m_cenCount = 0;
		m_sys->m_lpt1.libLinkTick();
	}
	if ((m_cenCount % G_TICKS) == G_TICKS-1)	// Every second 
	{
		// We should have taken (GRANULARITY)ms since last check. 
		// Calculate if that is so.
		m_ticksStopped = SDL_GetTicks(); 
		if (m_ticksStopped > m_ticksStarted)
		{
			m_ticksElapsed += (m_ticksStopped - m_ticksStarted);
		}	 
		m_ticksStarted = m_ticksStopped;
		m_ticksStopped = 0;	// Running
		if (m_ticksElapsed < m_msecs)
		{
			SDL_Delay(m_msecs - m_ticksElapsed);
		}
		if (m_ticksElapsed)
		{
			m_avgSpeed = (GRANULARITY / m_ticksElapsed);
		}
		else	m_avgSpeed = 9999.99;
		m_ticksElapsed = 0;

	}
	return 0;	/* No interrupt */
}





zbyte AnneZ80::fetchB(zword addr) { return fetch(addr); }
zword AnneZ80::fetchW(zword addr) { return fetch2(addr); }
void AnneZ80::storeB(zword addr, zbyte v) { store(addr, v); }
void AnneZ80::storeW(zword addr, zword v) { store2(addr, v); }

void AnneZ80::setCycles(int c)
{
	m_cycles = c;
}
