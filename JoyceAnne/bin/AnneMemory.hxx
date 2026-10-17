/************************************************************************

    JOYCE v2.1.0 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-2  John Elliott <seasip.webmaster@gmail.com>

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
//
// Memory definitions are global, for speed.
//
// The idea of the paging is that PCWRD[addr>>14][addr] will give the 
//   address of the byte at addr. Separate read and write pointers are 
//     maintained because of the CPC-like paging mode (which AFAIK is not 
//       used by any PCW software). 
//
#include "FlashChip.hxx"
extern zbyte *PCWFLASH;		/* The bottom half of memory */       
extern zbyte *anneMem[4];        /* Pointers to the current memory banks - reading */
extern zbyte anneMemIO[4];        
extern FlashChip gl_chip[2];

class AnneMemory : public PcwMemory
{
protected:
	long m_flashRequested;
	long m_flashActual;
	int m_flashBanks;

	int re_alloc(long &reqested, long &actual, int &nbanks, zbyte **ram);

public:
	AnneMemory();
	~AnneMemory();
	virtual void reset(void);
        virtual bool hasSettings(SDLKey *key, string &caption);
        virtual UiEvent settings(UiDrawer *d);
        virtual int alloc(PcwArgs *args);
        virtual int re_alloc(void);
//
// Page the memory
//
	void out(zbyte port, zbyte value);
	zbyte in(zbyte port);
	virtual zbyte getPage(zbyte port);

	inline long getFlashActual() { return m_flashActual; }

protected:
	bool parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	bool storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
};


