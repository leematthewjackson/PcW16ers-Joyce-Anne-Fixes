/************************************************************************

    JOYCE v2.5.3 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-2, 2024  John Elliott <seasip.webmaster@gmail.com>

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

#include "Joyce.hxx"
#include "UiContainer.hxx"
#include "UiLabel.hxx"
#include "UiSeparator.hxx"
#include "UiCommand.hxx"
#include "UiSetting.hxx"
#include "UiMenu.hxx"
#include "UiTextInput.hxx"
#include "UiScrollingMenu.hxx"


JoycePcwLinkit::JoycePcwLinkit() : PcwLibLink("pcwlinkit", "0")
{
}


JoycePcwLinkit::~JoycePcwLinkit()
{

}




zbyte	JoycePcwLinkit::in()
{
        unsigned busyline;
        ll_error_t e;
        zbyte r = 0;

        if (!isEnabled()) return 0xFF;

        if (!m_lldev) open();
        if (!m_lldev) return 0xFF;

	e = ll_recv_poll(m_lldev, &r);
 	showError(e);
	if (e != LL_E_OK) return 0xFF;
	return r ^ 3;
}


void	JoycePcwLinkit::out(zbyte value)
{
	ll_error_t e;

        if (!isEnabled()) return;

        if (!m_lldev) open();
        if (!m_lldev) return;
	e = ll_send(m_lldev, value);
 	showError(e);
}



bool JoycePcwLinkit::hasSettings(SDLKey *key, string &caption)
{
	*key = SDLK_w;
	caption = "  PCW Linkit  ";
	return true;
}




UiEvent JoycePcwLinkit::settings(UiDrawer *d)
{
	int x,y,sel;
	UiEvent uie;
	char speed[50];
	int s;
	UiTextInput *inp_speed;
	
	x = y = 0;
	sel = -1;
	while (1)
	{
		UiMenu uim(d);
		bool bEnabled;

		bEnabled = isEnabled();
		uim.add(new UiLabel(getTitle(), d));
		uim.add(new UiSeparator(d));
		uim.add(new UiSetting(SDLK_d, !bEnabled, "  Disconnected  ", d));
		uim.add(new UiSetting(SDLK_c,  bEnabled, "  Connected  ",    d));

		if (bEnabled)
		{
			string ostat = "  Connection is to ";
			if (m_lldriver == "parsocket") ostat += displaySocket(m_llfilename, 46);
			else if (m_lldriver == "parshm") ostat += displayShm(m_llfilename, 46);
			else ostat += m_lldriver + ": " + displayName(m_llfilename, 40);
			uim.add(new UiSeparator(d));
			uim.add(new UiLabel  (ostat, d)); 
			uim.add(new UiCommand(SDLK_m,  "  Connect using shared Memory... ",d, UIG_SUBMENU));
			uim.add(new UiCommand(SDLK_s,  "  Connect using Socket... ",d, UIG_SUBMENU));
		}

		d->centreContainer(uim);
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_ESCAPE,  "  EXIT  ", d));
		uim.setSelected(sel);

		uie =  uim.eventLoop();
	
		if (uie == UIE_QUIT) return uie;
		sel = uim.getSelected();	
		switch(uim.getKey(sel))
		{
			case SDLK_d: enable(false); break;
			case SDLK_c: enable();      break;
			case SDLK_m:    uie = getSharedMem(d);
					if (uie == UIE_QUIT) return uie;
					break;
			case SDLK_s:    uie = getSocket(d);
					if (uie == UIE_QUIT) return uie;
					break;

			case SDLK_ESCAPE: close();  return UIE_OK;
			default: break;
		}

	}
	return UIE_OK;
}


string JoycePcwLinkit::getTitle()
{
	return "PCW Linkit interface";
}

