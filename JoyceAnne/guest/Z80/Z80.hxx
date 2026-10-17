
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
#ifndef Z80_HXX_INCLUDED

typedef unsigned char zbyte;	/* C++17 has a native byte type */
typedef unsigned short zword;

#define bc ((b<<8)|c)
#define de ((d<<8)|e)
#define hl ((h<<8)|l)
#define bc1 ((b1<<8)|c1)
#define de1 ((d1<<8)|e1)
#define hl1 ((h1<<8)|l1)


class Z80
{
public:
	zbyte a, f, b, c, d, e, h, l;
	zbyte r, a1, f1, b1, c1, d1, e1, h1, l1, i, iff1, iff2, im;
	zword pc;
	zword ix, iy, sp;

	virtual void Int(int status) = 0;
	virtual void Nmi(int status) = 0;

	inline zword getBC() { return bc; }
	inline zword getDE() { return de; }
	inline zword getHL() { return hl; }
	inline void setBC(zword v) { c = v & 0xFF; b = v >> 8; }
        inline void setDE(zword v) { e = v & 0xFF; d = v >> 8; }
	inline void setHL(zword v) { l = v & 0xFF; h = v >> 8; } 
        inline zword getBC1() { return bc1; }
        inline zword getDE1() { return de1; }
        inline zword getHL1() { return hl1; }
        inline void setBC1(zword v) { c1 = v & 0xFF; b1 = v >> 8; }
        inline void setDE1(zword v) { e1 = v & 0xFF; d1 = v >> 8; }
        inline void setHL1(zword v) { l1 = v & 0xFF; h1 = v >> 8; }                 

        virtual double getSpeed(bool requested) = 0;
        virtual void setSpeed(double percent)   = 0;
	virtual void mainloop(zword startpc=0, zword startsp=0) = 0;

/* vtable wrappers for the inline gstore/gfetch instructions. The CPU emulator
 * itself doesn't use these, but other system components do. */
	virtual zbyte gfetchB(zword addr) = 0;
	virtual zword gfetchW(zword addr) = 0;
	virtual void gstoreB(zword addr, zbyte b) = 0;
	virtual void gstoreW(zword addr, zword w) = 0;
	virtual void reset() = 0;
	virtual void setCycles(int cycles) = 0;
};


#define Z80_quit  1
#define Z80_NMI   2
#define Z80_reset 3
#define Z80_load  4
#define Z80_save  5
#define Z80_log   6


#define FLG_C   0x01    /* Carry */
#define FLG_H   0x10    /* Halfcarry */
#define FLG_Z   0x40    /* Zero */


#define Z80_HXX_INCLUDED 1
#endif // ndef Z80_HXX_INCLUDED
 
