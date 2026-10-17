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
#ifndef HOSTZ80_HXX_INCLUDED

class Z80Guest;

class GuestZ80 : public ImcZ80g
{
public:
	GuestZ80(Z80Guest *sys);
	~GuestZ80();

	Z80Guest *m_sys;
	zbyte	m_curdrive;
	zbyte	m_fromBank;
	zbyte	m_toBank;
	zbyte	m_ccp[2048];

        virtual double getSpeed(bool requested);
        virtual void setSpeed(double percent);

/* vtable wrappers for the inline gstore/gfetch instructions. The CPU emulator
 * itself doesn't use these, but other system components do. */
	virtual zbyte gfetchB(zword addr);
	virtual zword gfetchW(zword addr);
	virtual void gstoreB(zword addr, zbyte b);
	virtual void gstoreW(zword addr, zword w);
	virtual void setCycles(int cycles);

	virtual int interrupt(void);
	virtual unsigned out(unsigned long time, zbyte hi, zbyte lo, zbyte val);
	virtual unsigned int in(unsigned long time, unsigned long hi, 
		unsigned long lo);
	virtual void EDFE(void);
	virtual void EDFD(void);
	virtual void startwatch(int flag);
	virtual long stopwatch(void);
	virtual void reset();
};


#define HOSTZ80_HXX_INCLUDED 1
#endif // ndef HOSTZ80_HXX_INCLUDED
 
