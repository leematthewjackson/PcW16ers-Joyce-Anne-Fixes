/************************************************************************

    JOYCE v2.5.2 - Amstrad PCW emulator

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

#include "Anne.hxx"
#include "UiContainer.hxx"
#include "UiLabel.hxx"
#include "UiSeparator.hxx"
#include "UiCommand.hxx"
#include "UiSetting.hxx"
#include "UiMenu.hxx"
#include "UiTextInput.hxx"
#include "UiScrollingMenu.hxx"


AnnePrinter::AnnePrinter() : PcwPrinter("parallel")
{
	m_sleeps = 0;
	m_speed = 63;
}


AnnePrinter::~AnnePrinter()
{

}




zbyte	AnnePrinter::in(zbyte port)
{
        unsigned busyline;
        ll_error_t e;
        zbyte r = 0;

        if (!isEnabled()) return 0xC0;

        if (!m_lldev) open();
        if (!m_lldev) return 0xC0;

	switch(port)
	{
		// For unidirectional port: Last value written
		case 0x38:	return m_latch;
		case 0x39:	// status
			e = ll_rctl_poll(m_lldev, &busyline);
 			showError(e);
			if (e != LL_E_OK) return 0xC0;
			if (!(busyline & LL_CTL_BUSY))  r |= 0x80;
			if (!(busyline & LL_CTL_ACK))   r |= 0x40;
			if (busyline & LL_CTL_NOPAPER)  r |= 0x20;
			if (busyline & LL_CTL_ISELECT)  r |= 0x10;
			if (!(busyline & LL_CTL_ERROR)) r |= 0x08;
/*
{ static int r0 = -1;
	if (r != r0) 
	{
		fprintf(stderr, "AnnePrinter::in(%02x) = %02x %s %s ->", 
			port, r & 0xC0,
			(r & 0x80) ? " " : " BUSY",
			(r & 0x40) ? " " : " ACK");
	switch (r & 0xc0)
	{
		case 0xc0: fprintf(stderr, "1\n"); break;
		case 0x80: fprintf(stderr, "3\n"); break;
		case 0x40: fprintf(stderr, "0\n"); break;
		case 0x00: fprintf(stderr, "2\n"); break;
		default:   fprintf(stderr, "?\n"); break;
	}
	r0 = r;
	}
	
}*/
/* Introduce an artificial delay when reading parallel port status. 
 *
 * I found this to be necessary when getting ANNE to have LocoLink 
 * conversations using the 'parshm' (shared memory) driver. The reason is 
 * down to how LocoLink works. While communicating, each end sends data 
 * alternately. When sending a byte, for example, one end sends bit 7, the 
 * other end acknowledges, one end sends bit 6, the other end acknowledges, 
 * and so on.
 *
 * Within Rosanne, the check for an acknowledgement retries 5000 times. On a 
 * real 16MHz PcW16, that gives a generous time window in which the computer
 * at the other end can process the bit it's received and sends the 
 * acknowledgement. But in ANNE (or JOYCE) things work a little differently.
 * All the CPU instructions for a frame are processed at the host computer's
 * top speed, and then a delay is inserted until the start of the next frame.
 * That means the 5000 retries can all happen and LocoLink can fail with a
 * timeout error, all while the system at the other end is still processing
 * the data (or waiting for the start of its next frame).
 *
 * The solution is to add a minimal delay to polling the printer status (akin
 * to a piece of slow hardware introducing wait states when accessed) so that
 * the time taken by the 5000 retries is long enough for the other end to 
 * process the data and send the acknowledgement.
 *
 * This one took a lot of time to track down!
 *
 * The count (delay every 63 reads) is arbitrary. Delaying every read is 
 * reliable but means the emulated PcW16 takes a long time to start up 
 * if there's no LocoLink connected. Increasing the number makes the 
 * startup faster but LocoLink less reliable.
 **/
			++m_sleeps;
			if (m_sleeps > m_speed)
			{
				SDL_Delay(1);
				m_sleeps = 0;
			}
			return r;
		case 0x3A:	return m_ctrl;
	}
	return 0xFF;
}


void	AnnePrinter::out(zbyte port, zbyte value)
{
	switch(port)
	{
		case 0x38:	m_latch = value;
/*
{ static int v0 = -1;
	if (value != v0)
		fprintf(stderr, "AnnePrinter::out(%02x,%02x)\n", port, value);
	v0 = value;
}*/
				writeChar(value);
				break;
		case 0x3A:	m_ctrl = value;
				writeCtrl();
				break;
	}
}



bool AnnePrinter::hasSettings(SDLKey *key, string &caption)
{
	*key = SDLK_p;
	caption = "  Printer  ";
	return true;
}



void AnnePrinter::writeCtrl()
{
        unsigned ctrls;
        ll_error_t e;

        if (!isEnabled()) return;

        if (!m_lldev) open();
        if (!m_lldev) return;

// Get current control line statuses */
	ll_sctl_get(m_lldev, &ctrls);

	if (m_ctrl & 1)    ctrls |= LL_CTL_STROBE; else ctrls &= ~LL_CTL_STROBE;
	if (m_ctrl & 2)    ctrls |= LL_CTL_AUFEED; else ctrls &= ~LL_CTL_AUFEED;
	if (!(m_ctrl & 4)) ctrls |= LL_CTL_INIT;   else ctrls &= ~LL_CTL_INIT;
	if (m_ctrl & 8)    ctrls |= LL_CTL_OSELECT;else ctrls &= ~LL_CTL_OSELECT;
// XXX Handle: Bidirectional (if can be bothered)
//             Enable IRQ
//             Bit 7 (if used)
	e = ll_sctl(m_lldev, ctrls);
 	showError(e);
}





bool AnnePrinter::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	if (!PcwPrinter::parseNode(doc, ns, cur)) return false;

	char *t = (char *)xmlGetProp(cur, (xmlChar *)"speed");
	if (t) 
	{
		m_speed = atoi(t);
		xmlFree(t);
	}
	return true;	
}


bool AnnePrinter::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	char buf[20];
	
	if (!PcwPrinter::storeNode(doc, ns, cur)) return false;

	sprintf(buf, "%d", m_speed);
	xmlSetProp(cur, (xmlChar *)"speed", (xmlChar *)buf);	
	return true;
}



UiEvent AnnePrinter::settings(UiDrawer *d)
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
			string ostat = "  Output is to ";
			if      (m_lldriver == "parpipe"  ) ostat += "pipe: " + displayName(m_llfilename, 40);
			else if (m_lldriver == "parfile"  ) ostat += "file: " + displayName(m_llfilename, 40);
			else if (m_lldriver == "parport"  ) ostat += "port: " + displayName(m_llfilename, 40);
			else if (m_lldriver == "parsocket") ostat += displaySocket(m_llfilename, 46);
			else if (m_lldriver == "parshm") ostat += displayShm(m_llfilename, 46);
			else if (m_lldriver == "llserv" ||
				 m_lldriver == "parllserv"  ) ostat += "LocoLink: " + displayName(m_llfilename, 40);
			else ostat += m_lldriver + ": " + displayName(m_llfilename, 40);
			uim.add(new UiSeparator(d));
			uim.add(new UiLabel  (ostat, d)); 
			uim.add(new UiCommand(SDLK_f,  "  Output to File...  ", d, UIG_SUBMENU));
			uim.add(new UiCommand(SDLK_l,  "  Use internal LocoLink server...", d, UIG_SUBMENU));
			uim.add(new UiCommand(SDLK_u,  "  Output to UNIX command... ",d, UIG_SUBMENU));
#ifdef HAVE_LINUX_PARPORT_H
			uim.add(new UiCommand(SDLK_p,  "  Output to Parallel port... ",d, UIG_SUBMENU));
#endif
			uim.add(new UiCommand(SDLK_m,  "  Output to shared Memory... ",d, UIG_SUBMENU));
			uim.add(new UiCommand(SDLK_s,  "  Output to Socket... ",d, UIG_SUBMENU));
#ifdef WIN32
			uim.add(new UiCommand(SDLK_1,  "  Output to LPT1:  ", d));
			uim.add(new UiCommand(SDLK_2,  "  Output to LPT2:  ", d));
			uim.add(new UiCommand(SDLK_3,  "  Output to LPT3:  ", d));
			uim.add(new UiCommand(SDLK_4,  "  Output to LPT4:  ", d));
#endif
			uim.add(new UiSeparator(d));
			inp_speed = new UiTextInput(SDLK_e,  "  Speed: _____  ", d);
			uim.add(inp_speed);
			sprintf(speed, "%d", m_speed);
			inp_speed->setText(speed);
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
			case SDLK_1: m_llfilename = "LPT1:"; m_lldriver = "parfile"; break;
			case SDLK_2: m_llfilename = "LPT2:"; m_lldriver = "parfile"; break;
			case SDLK_3: m_llfilename = "LPT3:"; m_lldriver = "parfile"; break;
			case SDLK_4: m_llfilename = "LPT4:"; m_lldriver = "parfile"; break;
			case SDLK_d: enable(false); break;
			case SDLK_c: enable();      break;
			case SDLK_e: s = atoi(inp_speed->getText().c_str());
				     if (s && s <= 5000) m_speed = s;
				     break;
			case SDLK_f:    uie = getFile(d);
					if (uie == UIE_QUIT) return uie;
					break;
			case SDLK_l:    uie = getLocoLink(d);
					if (uie == UIE_QUIT) return uie;
					break;
			case SDLK_u:    uie = getPipe(d);
					if (uie == UIE_QUIT) return uie;
					break;
			case SDLK_p:    uie = getPort(d);
					if (uie == UIE_QUIT) return uie;
					break;
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



