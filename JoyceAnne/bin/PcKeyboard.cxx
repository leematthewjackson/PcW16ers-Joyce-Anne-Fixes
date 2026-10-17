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

#include "Pcw.hxx"
#include <list>
#include "PcKeyboard.hxx"

#define KB_PARITY 0x80
#define KB_STOP   0x40
#define KB_START  0x20
#define KB_BUSY   0x10
#define KB_RESET  0x04
#define KB_CLOCK  0x02
#define KB_TX     0x01

#define ST_NORMAL 0x00
#define ST_WRDATA 0x01
#define ST_STROBE 0x02
#define ST_OUTPUT 0x03

#include "PcKeyMap.hxx"   /* kc_maps[] and pcKeyScancodes() */

/* Translated scancodes */
/*
static int kc_maps[] = 
{
//	 SDL key 	  Codes 1 	   Codes 2 	    Codes 3 
	SDLK_ESCAPE,	  0x01,   0x81,	   0x01,   0x81,    0x01,   0x81,

	SDLK_TAB,	  0x0F,   0x8F,    0x0F,   0x8F,    0x0F,   0x8F,
	SDLK_q,		  0x10,   0x90,    0x10,   0x90,    0x10,   0x90,
	SDLK_w,		  0x11,   0x91,    0x11,   0x91,    0x11,   0x91,
	SDLK_e,		  0x12,   0x92,    0x12,   0x92,    0x12,   0x92,
	SDLK_r,		  0x13,   0x93,    0x13,   0x93,    0x13,   0x93,
	SDLK_t,		  0x14,   0x94,    0x14,   0x94,    0x14,   0x94,
	SDLK_LCTRL,	  0x1D,   0x9D,	   0x1D,   0x9D,    0x1D,   0x9D,
	SDLK_LALT,	  0x38,   0xB8,	   0x38,   0xB8,    0x38,   0xB8,
	SDLK_SPACE,	  0x39,   0xB9,    0x39,   0xB9,    0x39,   0xB9,
	SDLK_F1,	  0x3B,   0xBB,    0x3B,   0xBB,    0x3B,   0xBB,
	SDLK_F2,	  0x3C,   0xBC,    0x3C,   0xBC,    0x3C,   0xBC,
	SDLK_F3,	  0x3D,   0xBD,    0x3D,   0xBD,    0x3D,   0xBD,
	SDLK_F4,	  0x3E,   0xBE,    0x3E,   0xBE,    0x3E,   0xBE,
	SDLK_F5,	  0x3F,   0xBF,    0x3F,   0xBF,    0x3F,   0xBF,
	SDLK_F6,	  0x40,   0xC0,    0x40,   0xC0,    0x40,   0xC0,
	SDLK_F7,	  0x41,   0xC1,    0x41,   0xC1,    0x41,   0xC1,
	SDLK_F8,	  0x42,   0xC2,    0x42,   0xC2,    0x42,   0xC2,
	SDLK_F9,	  0x43,   0xC3,    0x43,   0xC3,    0x43,   0xC3,
	SDLK_F10,	  0x44,   0xC4,    0x44,   0xC4,    0x44,   0xC4,
	
	SDLK_F11,	  0x57,   0xD7,    0x57,   0xD7,    0x57,   0xD7,
	SDLK_F12,	  0x58,   0xD8,    0x58,   0xD8,    0x58,   0xD8,

	SDLK_RALT,	0xE038, 0xE0B8,  0xE038, 0xE0B8,  0xE038, 0xE0B8,
	SDLK_RCTRL,	0xE01D, 0xE09D,  0xE01D, 0xE09D,  0xE01D, 0xE09D,
};
*/



PcKeyboard::PcKeyboard(PcwSystem *s) :
        PcwDevice("keyboard", "pc102"), PcwInput(s)
{
        reset();
}


PcKeyboard::~PcKeyboard()
{

}

void PcKeyboard::reset(void)
{
	clearKeys();
	m_queue.clear();
	m_cmdActive = 0;
	m_scanSet = 1;
	m_typematic = 0x2C;
	m_enabled = true;
	memset(m_kpNumLock, 0, sizeof(m_kpNumLock));
}



int PcKeyboard::handleEvent(SDL_Event &e)
{
        bool b = false;
        int rv = 0;
	int n, kp;
	bool numlock;
	unsigned char codes[8];
        SDLKey keysym = SDLK_UNKNOWN;

        if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
        {
                b = (e.type == SDL_KEYDOWN);
                keysym = e.key.keysym.sym;
		numlock = (e.key.keysym.mod & KMOD_NUM) != 0;

		// [2.5.5-fix] Keypad keys send different codes depending on
		// NumLock. Release a key with the same meaning it was pressed
		// with, even if NumLock was toggled in between.
		kp = pcKeypadIndex(keysym);
		if (kp >= 0)
		{
			if (b) m_kpNumLock[kp] = numlock;
			else   numlock = m_kpNumLock[kp];
		}
		rv = pcKeyScancodes(keysym, b, numlock, codes);
		if (rv >= 0)
		{
			for (n = 0; n < rv; n++) pushKey(codes[n]);
			return 1;
		}
		rv = 0;
	}

        switch(e.type)
        {
                case SDL_QUIT: rv = -99; break;
                case SDL_KEYDOWN:
                	//m_keyPress = true;
                        rv = 1;
                        break;
                case SDL_KEYUP:
                        //m_keyPress = true;
                        rv = 1;
                        break;
        }
	return 0;
}
//
// Settings functions.
// See if this device has user-settable options. If it does, populates
// "key" and "caption" and returns true; else, returns false.
//
bool PcKeyboard::hasSettings(SDLKey *key, string &caption)
{
        *key     = SDLK_k;
        caption = "  Keyboard  ";
        return true;
}
//
// Display settings screen
// 
UiEvent PcKeyboard::settings(UiDrawer *d)
{
	return UIE_CONTINUE;
}
//
// Implement the JOYCE "Assign key" calls
//
void PcKeyboard::assignKey(Uint16 key, Uint8 l, Uint8 h)
{

}


void PcKeyboard::edfe(Z80 *R)
{

}


void PcKeyboard::clearKeys(void)
{
	m_queue.clear();
}


bool PcKeyboard::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	return true;
}

bool PcKeyboard::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	return true;
}

void PcKeyboard::setRange(int *R, zbyte a, zbyte b, zbyte c, zbyte d)
{

}


void PcKeyboard::keySet(void)
{

}


void PcKeyboard::pushKey(zbyte kc)
{
	m_queue.push_back(kc);
}

void PcKeyboard::onCommand()
{
	fprintf(stderr, "PcKeyboard::onCommand(%02x)\n", m_command);

	/* Are we in the middle of an existing command? */
	switch(m_cmdActive)
	{
		case 0xED:	m_cmdActive = 0;
				if (!(m_command & 0x80))
				{
					pushKey(0xFA);
					m_leds = m_command;
					updateLEDs();
					return;
				}
				break;
		case 0xF0:	m_cmdActive = 0;
				if (m_command < 4)
				{
					pushKey(0xFA);
					if (m_command == 0) pushKey(m_scanSet);
					else m_scanSet = m_command;
					return;	
				}
				pushKey(0xFE);
				return;
		case 0xF3:	m_cmdActive = 0;
				if (!(m_command & 0x80))
				{
					pushKey(0xFA);
					m_typematic = m_command;
					// XXX SDL_set_typematic	
					return;
				}
				break;
		
	}

	switch(m_command)
	{
		case 0xED:	pushKey(0xFA);	// Acknowledge
				m_cmdActive = 0xED;
				break;
		case 0xEE:	pushKey(0xEE);	// Diagnostic echo
				break;
		case 0xF0:	pushKey(0xFA);
				m_cmdActive = 0xF0;	// Set scancode set
				break;
		case 0xF2:	pushKey(0xFA);	// Get keyboard ID
				pushKey(0xAB);
				pushKey(0x83);
				break;	
		case 0xF3:	pushKey(0xFA);	// Set rate/delay
				m_cmdActive = 0xF3;
				break;
		case 0xF4:	m_queue.clear(); // Clear queue
				m_enabled = true;
				pushKey(0xFA);	// Acknowledge command
				break;
		case 0xF5:	m_enabled = false;	// Disable
				// Fall through to
		case 0xF6:	reset();		// Set defaults
				pushKey(0xFA);
				break;
		case 0xFF: // Full keyboard reset
				reset();
				pushKey(0xFA);	// Acknowledge command
				pushKey(0xAA);	// Self-test OK
				break;
		default:	pushKey(0xFE);	// Unknown command
				fprintf(stderr, "Unknown kbd command %02x\n", m_command);
				break;
		
	}
}



/* Default implementation doesn't touch the LEDs */
void PcKeyboard::updateLEDs(void)
{
}

