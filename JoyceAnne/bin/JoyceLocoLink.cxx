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

// The PCW (slave) LocoLink interface

#include "Joyce.hxx"
#include "UiContainer.hxx"
#include "UiLabel.hxx"
#include "UiSeparator.hxx"
#include "UiCommand.hxx"
#include "UiSetting.hxx"
#include "UiMenu.hxx"
#include "UiTextInput.hxx"
#include "UiScrollingMenu.hxx"
#include "PcwFileList.hxx"
#include "PcwFileChooser.hxx"

JoyceLocoLink::JoyceLocoLink() : PcwLibLink("locolink", "slave") 
{
	XLOG("JoyceLocoLink::JoyceLocoLink()");
	m_sleeps = 0;
}


JoyceLocoLink::~JoyceLocoLink()
{
	XLOG("JoyceLocoLink::~JoyceLocoLink()");
}
// Settings functions.
// See if this device has user-settable options. If it does, populates
// "key" and "caption" and returns true; else, returns false.
//
bool JoyceLocoLink::hasSettings(SDLKey *key, string &caption)
{
	*key = SDLK_l;
	caption = "  LocoLink  ";	
	return true;
}





void JoyceLocoLink::out(zbyte b)
{
	if (!isEnabled()) return;

        if (!m_lldev) open();
        if (!m_lldev) return;

//	static zbyte last = 0xFF;
//	if (b != last)
//	{
//		fprintf(stderr, "JoyceLocoLink::out(%d)\n", b);
//		last = b;
//	}
	ll_error_t e = ll_lsend(m_lldev, LL_END_PCW, b);
	showError(e);
}


zbyte JoyceLocoLink::in(void)
{
	zbyte b;

	if (!isEnabled()) return 0xFF;
        if (!m_lldev) open();
        if (!m_lldev) return 0xFF;

	
	ll_error_t e = ll_lrecv_poll(m_lldev, LL_END_PCW, &b);
	showError(e);

//	static zbyte last = 0xFF;
//
//	if (b != last)
//	{
//		fprintf(stderr, "JoyceLocoLink::in = %d\n", b);
//		last = b;
//	}
	/* Introduce an artifical delay when reading LocoLink status (cf 
 	 * AnnePrinter::in() in AnnePrinter.cxx). I don't know if it's as
	 * necessary as it was at the PcW16 end, but in theory the same 
	 * issue (timeout errors caused by how quickly the emulated CPU 
	 * can execute a tight polling loop) could arise */

	++m_sleeps;
	if (m_sleeps > 2000)
	{
		m_sleeps = 0;
		SDL_Delay(1);
	}
	return b;
}


string JoyceLocoLink::getTitle()
{
	return "  LocoLink interface  ";
}
