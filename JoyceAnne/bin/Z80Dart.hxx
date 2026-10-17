/************************************************************************

    JOYCE v1.90 - Amstrad PCW emulator

    Copyright (C) 1996, 2001  John Elliott <seasip.webmaster@gmail.com>

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

#define REGMODE_REGISTER 1
#define REGMODE_DATA 2

class JoyceCPS;


class Z80Dart
{
protected:
	int m_regno;
	int m_regmode;	
public:
	zbyte m_latch;
	zbyte m_reg[8];
	zbyte m_rreg[2];	

	void reset(void);
	void outCtrl(zbyte b);	
	virtual void outData(zbyte b);
	zbyte inCtrl();
	virtual zbyte inData();
protected:
	virtual zbyte in(zbyte reg);
	virtual void out(zbyte reg, zbyte value);	
};

//
// In the CPS8256, the DART channel A is hooked up to the serial port
//
class Z80DartA : public Z80Dart
{
public:
	Z80DartA(PcwComms *c) : m_comms(c) { XLOG("Z80DartA::Z80DartA"); } 

	virtual void outData(zbyte b);
	virtual zbyte inData();
protected:
	virtual zbyte in(zbyte reg);
	virtual void out(zbyte reg, zbyte value);	

private:
	PcwComms *m_comms;
};

//
// In the CPS8256, the DART channel B is hooked up to the printer port
//
class Z80DartB : public Z80Dart
{
public:
	Z80DartB(JoyceCPS *p) : m_printer(p) { XLOG("Z80DartB::Z80DartB"); }
protected:
	virtual zbyte in(zbyte reg);
	virtual void out(zbyte reg, zbyte value);	

private:
	JoyceCPS *m_printer;
};



