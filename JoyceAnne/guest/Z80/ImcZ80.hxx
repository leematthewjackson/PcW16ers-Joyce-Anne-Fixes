
/* C++ Z80 emulation, based on Ian Collier's xz80 v0.1d
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


class ImcZ80g : public Z80
{
public:
   zbyte *memptr;

   int frames;
   int int_line, nmi_line;	// INT and NMI lines
   int m_cycles;		// Cycles per frame 
   GuestState m_state;

   GuestState get_state();
   void set_state(GuestState g);
/*#ifdef DEBUG
	word breakpoint=0;
	unsigned int breaks=0;

	inline void log(FILE *fp, char *name, word val)
	{
		int i;
		fprintf(fp,"%s=%04X ",name,val);
		for(i=0;i<8;i++,val++)fprintf(fp," %02X",gfetch(val));
		fputc('\n',fp);
	}
#endif*/
	virtual void mainloop(zword initpc = 0, zword initsp = 0);
	virtual void reset(void);
	virtual void Int(int status);
	virtual void Nmi(int status);

        virtual void main_every(void);
	virtual int interrupt(void) = 0;
/* Output "val" to the I/O port whose high address byte is "hi" and
 * whose low address byte is "lo".  The output occurred "time" clock
 * cycles after the start of the current frame.  The result from the
 * function is the number of wait states introduced by the I/O device.*/

	virtual unsigned int out(unsigned long time,
				 zbyte hi, zbyte lo,
				 zbyte val) = 0;
/* Input from the I/O port whose high address byte is "hi" and whose
 * low address byte is "lo".  The input occurred "time" clock cycles
 * after the start of the current frame.  The result from the function
 * is the 8-bit unsigned input from the port, plus (t<<8) where t is
 * the number of wait states introduced by the I/O device. */
	virtual unsigned int in(unsigned long time,
			        unsigned long hi, unsigned long lo) = 0;


	virtual void EDFE(void) = 0;
	virtual void EDFD(void) = 0;
	virtual void startwatch(int flag) = 0;
	virtual long stopwatch(void) = 0;
};

extern Host *gl_host;
extern unsigned guestBank;
extern unsigned guestRAMsize;
extern zbyte *guestRAM;

inline zbyte gfetch (zword x) 
{
	if (x >= 0xE000) 	/* Top 8k is common memory */
	{
		return guestRAM[x];
	}
	else if (guestBank < 0xFE) 
	{
		return guestRAM[(guestBank << 16 | x) % guestRAMsize];
	}
	else if (guestBank == 0xFE)
	{
		return gl_host->fetch_screen(x);
	}
	else if (guestBank == 0xFF)
	{
		return gl_host->fetch_tpa(x);
	}
	return 0xFF;
}

/*
inline zbyte gfetch(zword x)
{
	zbyte y = Xgfetch(x);
	fprintf(stderr, "gfetch(%04x)=%02x\n", x, y);
	return y;
}*/

inline zword gfetch2(zword x) { return ((gfetch((x)+1)<<8)|gfetch(x));    }

inline void gstore(zword x, zbyte y)
{
	if (x >= 0xE000) 	/* Top 8k is common memory */
	{ 	
		guestRAM[x] = y; 
		return; 
	}
	if (guestBank < 0xFE)
	{
		guestRAM[(guestBank << 16 | x) % guestRAMsize] = y;
		return;
	}
	else if (guestBank == 0xFE)
	{
		gl_host->store_screen(x, y);
	}
	else if (guestBank == 0xFF)
	{
		gl_host->store_tpa(x, y);
	}
}

inline void gstore2b(zword x, zbyte hi, zbyte lo)
{
        gstore(x,  lo);
        gstore(x+1,hi);
}

inline void gstore2(zword x, zword y) { gstore2b(x, (y)>>8, y & 0xFF); }

