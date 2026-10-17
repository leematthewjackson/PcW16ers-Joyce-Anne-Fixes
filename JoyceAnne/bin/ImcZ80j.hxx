
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


class ImcZ80 : public Z80
{
public:
   zbyte *memptr;

   int frames;
   int int_line, nmi_line;	// INT and NMI lines
   int m_cycles;		// Cycles per frame 

   ImcZ80();

/*#ifdef DEBUG
	word breakpoint=0;
	unsigned int breaks=0;

	inline void log(FILE *fp, char *name, word val)
	{
		int i;
		fprintf(fp,"%s=%04X ",name,val);
		for(i=0;i<8;i++,val++)fprintf(fp," %02X",fetch(val));
		fputc('\n',fp);
	}
#endif*/
	void clear_regs();
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

extern zbyte *PCWRD[], *PCWWR[];	
/*
 * extern byte snapa,snapf,snapb,snapc,snapd,snape,snaph,snapl,
             snapa1,snapf1,snapb1,snapc1,snapd1,snape1,snaph1,snapl1,
             snapi,snapr,snapiff1,snapiff2,snapim;
extern word snapix,snapiy,snapsp,snappc;
extern int load_snap;
extern char snapname[];
extern char snapsavename[];
extern int hsize,vsize;
extern byte keyports[9];
extern int bordercr;
extern int borderchange;
extern int soundfd;
*/

inline zbyte fetch (zword x) { return PCWRD[(zword)(x)>>14][x]; }
inline zword fetch2(zword x) { return ((fetch((x)+1)<<8)|fetch(x));    }
inline void store(zword x, zbyte y) { (PCWWR[(zword)(x)>>14][x]) = y; }
inline void store2b(zword x, zbyte hi, zbyte lo)
{
	(PCWWR[(zword)(x)>>14][x]) = lo;
	(PCWWR[(zword)(x+1)>>14][(x+1)&65535]) = hi;
}
inline void store2(zword x, zword y) { store2b(x, (y)>>8, y & 0xFF); }


