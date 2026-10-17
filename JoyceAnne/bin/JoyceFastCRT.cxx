/************************************************************************

    JOYCE v2.5.2 - Amstrad PCW emulator

    Copyright (C) 1996, 2001, 2023  John Elliott <seasip.webmaster@gmail.com>

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
#include "UiCommand.hxx"
#include "UiSeparator.hxx"
#include "UiSetting.hxx"
#include "UiMenu.hxx"

static zword stretch[256] = 
{
	0x0000, 0x0003, 0x000C, 0x000F, 0x0030, 0x0033, 0x003C, 0x003F,
	0x00C0, 0x00C3, 0x00CC, 0x00CF, 0x00F0, 0x00F3, 0x00FC, 0x00FF,
	0x0300, 0x0303, 0x030C, 0x030F, 0x0330, 0x0333, 0x033C, 0x033F,
	0x03C0, 0x03C3, 0x03CC, 0x03CF, 0x03F0, 0x03F3, 0x03FC, 0x03FF,
	0x0C00, 0x0C03, 0x0C0C, 0x0C0F, 0x0C30, 0x0C33, 0x0C3C, 0x0C3F,
	0x0CC0, 0x0CC3, 0x0CCC, 0x0CCF, 0x0CF0, 0x0CF3, 0x0CFC, 0x0CFF,
	0x0F00, 0x0F03, 0x0F0C, 0x0F0F, 0x0F30, 0x0F33, 0x0F3C, 0x0F3F,
	0x0FC0, 0x0FC3, 0x0FCC, 0x0FCF, 0x0FF0, 0x0FF3, 0x0FFC, 0x0FFF,
	0x3000, 0x3003, 0x300C, 0x300F, 0x3030, 0x3033, 0x303C, 0x303F,
	0x30C0, 0x30C3, 0x30CC, 0x30CF, 0x30F0, 0x30F3, 0x30FC, 0x30FF,
	0x3300, 0x3303, 0x330C, 0x330F, 0x3330, 0x3333, 0x333C, 0x333F,
	0x33C0, 0x33C3, 0x33CC, 0x33CF, 0x33F0, 0x33F3, 0x33FC, 0x33FF,
	0x3C00, 0x3C03, 0x3C0C, 0x3C0F, 0x3C30, 0x3C33, 0x3C3C, 0x3C3F,
	0x3CC0, 0x3CC3, 0x3CCC, 0x3CCF, 0x3CF0, 0x3CF3, 0x3CFC, 0x3CFF,
	0x3F00, 0x3F03, 0x3F0C, 0x3F0F, 0x3F30, 0x3F33, 0x3F3C, 0x3F3F,
	0x3FC0, 0x3FC3, 0x3FCC, 0x3FCF, 0x3FF0, 0x3FF3, 0x3FFC, 0x3FFF,
	0xC000, 0xC003, 0xC00C, 0xC00F, 0xC030, 0xC033, 0xC03C, 0xC03F,
	0xC0C0, 0xC0C3, 0xC0CC, 0xC0CF, 0xC0F0, 0xC0F3, 0xC0FC, 0xC0FF,
	0xC300, 0xC303, 0xC30C, 0xC30F, 0xC330, 0xC333, 0xC33C, 0xC33F,
	0xC3C0, 0xC3C3, 0xC3CC, 0xC3CF, 0xC3F0, 0xC3F3, 0xC3FC, 0xC3FF,
	0xCC00, 0xCC03, 0xCC0C, 0xCC0F, 0xCC30, 0xCC33, 0xCC3C, 0xCC3F,
	0xCCC0, 0xCCC3, 0xCCCC, 0xCCCF, 0xCCF0, 0xCCF3, 0xCCFC, 0xCCFF,
	0xCF00, 0xCF03, 0xCF0C, 0xCF0F, 0xCF30, 0xCF33, 0xCF3C, 0xCF3F,
	0xCFC0, 0xCFC3, 0xCFCC, 0xCFCF, 0xCFF0, 0xCFF3, 0xCFFC, 0xCFFF,
	0xF000, 0xF003, 0xF00C, 0xF00F, 0xF030, 0xF033, 0xF03C, 0xF03F,
	0xF0C0, 0xF0C3, 0xF0CC, 0xF0CF, 0xF0F0, 0xF0F3, 0xF0FC, 0xF0FF,
	0xF300, 0xF303, 0xF30C, 0xF30F, 0xF330, 0xF333, 0xF33C, 0xF33F,
	0xF3C0, 0xF3C3, 0xF3CC, 0xF3CF, 0xF3F0, 0xF3F3, 0xF3FC, 0xF3FF,
	0xFC00, 0xFC03, 0xFC0C, 0xFC0F, 0xFC30, 0xFC33, 0xFC3C, 0xFC3F,
	0xFCC0, 0xFCC3, 0xFCCC, 0xFCCF, 0xFCF0, 0xFCF3, 0xFCFC, 0xFCFF,
	0xFF00, 0xFF03, 0xFF0C, 0xFF0F, 0xFF30, 0xFF33, 0xFF3C, 0xFF3F,
	0xFFC0, 0xFFC3, 0xFFCC, 0xFFCF, 0xFFF0, 0xFFF3, 0xFFFC, 0xFFFF,
};


void CRTVars::reset()
{
	static zbyte defpalette[] = { 1,1,1,1,60,60};

	memset(defaults, 0, sizeof(defaults));

/* In normal use, these variables would all be pointed at PCW memory. But
 * in case this is called without being mapped to PCW memory, default it 
 * to pointing at an internal state buffer */

	top		= &defaults[0];
	left		= &defaults[1];
	width		= &defaults[2];
	height		= &defaults[3];
	curx		= &defaults[4];
	cury		= &defaults[5];	
	language	= &defaults[8];	/* 6 & 7 are saved position */
	attrs		= &defaults[9];
	wrap		= &defaults[10];
	pagemode	= &defaults[11];
	showcur		= &defaults[12];
	curshown	= &defaults[13];
	curevent	= &defaults[14]; /* 14 and 15 - this is a word */
	palette		= &defaults[16]; /* 16-21 - six bytes */
	ric_xpos	= &defaults[22];
	adrive		= &defaults[23];
	stl_on		= &defaults[24];
	ul_mask		= &defaults[25]; /* Underline mask */
	reqwidth	= &defaults[26];
	reqheight	= &defaults[27];
	is_24x80	= &defaults[28];
	*width = 24;
	*height = 80;
	and_mask = -1;
	or_mask = -1;
	xor_mask = -1;
	inited = 0;
	memcpy(palette, defpalette, 6);
}




JoyceFastCRT::JoyceFastCRT(JoyceSystem *s) : PcwFastCRT(s)
{
	s->m_fastCRT = this;
	m_colour = false;
	m_beepBuffer = NULL;
	m_beepLen = 0;
}

JoyceFastCRT::~JoyceFastCRT()
{
	if (m_beepBuffer) 
	{
		free(m_beepBuffer);
		m_beepBuffer = NULL;
	}
}

void JoyceFastCRT::reset(void)
{
	m_vars.reset();	
}

void JoyceFastCRT::tick(void)
{
}

bool JoyceFastCRT::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
        char *s = (char *)xmlGetProp(cur, (xmlChar *)"colour");
	if (s && !strcmp(s, "yes")) m_colour = true;
	else			    m_colour = false;
	if (s) xmlFree(s);

	return true;
}

bool JoyceFastCRT::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	xmlSetProp(cur, (xmlChar *)"colour", (xmlChar *)(m_colour ? "yes" :"no"));
	return true;
}

string JoyceFastCRT::getTitle(void)
{
	return "  Accelerated display  ";
}

bool JoyceFastCRT::hasSettings(SDLKey *key, string &caption)
{
	*key = SDLK_a;
	caption = "  Accelerated display  ";
	return true;
}

UiEvent JoyceFastCRT::settings(UiDrawer *d)
{
	int sel = -1;
	UiEvent uie;

	do
	{
		UiMenu uim(d);
		uim.add(new UiLabel("  Accelerated display  ", d))
		   .add(new UiSeparator(d))
		   .add(new UiSetting(SDLK_c, m_colour, "  Colour changes allowed? ", d))
		   .add(new UiSeparator(d))
		   .add(new UiCommand(SDLK_ESCAPE, "  EXIT  ", d));
		d->centreContainer(uim);
		uim.setSelected(sel);
		uie = uim.eventLoop();
		if (uie == UIE_QUIT) return uie;
		sel = uim.getSelected();
		switch (uim[sel].getKey())
		{
			case SDLK_ESCAPE: return UIE_OK;
			case SDLK_c: m_colour = !m_colour;
			break;
		}
	}
	while (1);
	return UIE_OK;
}


void JoyceFastCRT::setVars(unsigned scr_blinkcount, unsigned te_attrs,
			unsigned te_showcursor, unsigned msg_ric_xpos)
{
	m_vars.width	= &PCWRAM[te_attrs - 11];
	m_vars.height	= &PCWRAM[te_attrs - 10];
	m_vars.reqwidth = &PCWRAM[te_attrs - 9];
	m_vars.reqheight= &PCWRAM[te_attrs - 8];
	m_vars.left	= &PCWRAM[te_attrs - 7];
	m_vars.top	= &PCWRAM[te_attrs - 6];
	m_vars.wrap	= &PCWRAM[te_attrs - 5];
	m_vars.curx	= &PCWRAM[te_attrs - 4];
	m_vars.cury	= &PCWRAM[te_attrs - 3];
	m_vars.attrs	= &PCWRAM[te_attrs];
	m_vars.language	= &PCWRAM[te_attrs + 1];

	m_vars.curevent	= &PCWRAM[te_showcursor - 2];
	m_vars.showcur	= &PCWRAM[te_showcursor];
	m_vars.curshown	= &PCWRAM[te_showcursor + 1];
	m_vars.is_24x80	= &PCWRAM[te_showcursor + 2];
/* Not all CP/Ms have this variable, so use the default storage if necessary */
	if (te_showcursor < te_attrs - 15)
	{
		m_vars.pagemode = &PCWRAM[te_showcursor + 3];
	}
	m_vars.palette  = &PCWRAM[scr_blinkcount + 3];
	m_vars.ric_xpos = &PCWRAM[msg_ric_xpos];	
	m_vars.adrive   = &PCWRAM[msg_ric_xpos - 1];	
	m_vars.stl_on   = &PCWRAM[msg_ric_xpos - 2];	
	m_vars.inited   = 1;
}


bool JoyceFastCRT::isInitialised()
{
	return m_vars.inited;
}


bool JoyceFastCRT::isPageMode()
{
	return (m_vars.pagemode[0] != 0);
}

void JoyceFastCRT::setPageMode(bool b)
{
	m_vars.pagemode[0] = b ? 0xFF : 0;
}


bool JoyceFastCRT::isWrap()
{
	return (m_vars.wrap[0] != 0);
}

void JoyceFastCRT::setWrap(bool b)
{
	m_vars.wrap[0] = b ? 0xFF : 0;
}




bool JoyceFastCRT::isReverse()
{
	return ((m_vars.attrs[0]) & 4) != 0;
}

void JoyceFastCRT::setReverse(bool b)
{
	if (b)	m_vars.attrs[0] |= 4;
	else	m_vars.attrs[0] &= ~4;
}


bool JoyceFastCRT::isUnderline()
{
	return ((m_vars.attrs[0]) & 2) != 0;
}

void JoyceFastCRT::setUnderline(bool b, zbyte mask)
{
	if (b)	{ m_vars.attrs[0] |= 2; m_vars.ul_mask[0] = mask; }
	else	m_vars.attrs[0] &= ~2;
}


bool JoyceFastCRT::isDim()
{
	return ((m_vars.attrs[0]) & 1) != 0;
}

void JoyceFastCRT::setDim(bool b)
{
	if (b)	m_vars.attrs[0] |= 1;
	else	m_vars.attrs[0] &= ~1;
}


bool JoyceFastCRT::isBold()
{
	return ((m_vars.attrs[0]) & 8) != 0;
}

void JoyceFastCRT::setBold(bool b)
{
	if (b)	m_vars.attrs[0] |= 8;
	else	m_vars.attrs[0] &= ~8;
}

bool JoyceFastCRT::isItalic()
{
	return ((m_vars.attrs[0]) & 16) != 0;
}

void JoyceFastCRT::setItalic(bool b)
{
	if (b)	m_vars.attrs[0] |= 16;
	else	m_vars.attrs[0] &= ~16;
}

bool JoyceFastCRT::isDoubleWidth()
{
	return ((m_vars.attrs[0]) & 32) != 0;
}

void JoyceFastCRT::setDoubleWidth(bool b)
{
	if (b)	m_vars.attrs[0] |= 32;
	else	m_vars.attrs[0] &= ~32;
}

bool JoyceFastCRT::isDoubleHeight()
{
	return ((m_vars.attrs[0]) & 64) != 0;
}

void JoyceFastCRT::setDoubleHeight(bool b)
{
	if (b)	m_vars.attrs[0] |= 64;
	else	m_vars.attrs[0] &= ~64;
}



void JoyceFastCRT::cr()
{
	hideCursor();
	m_vars.curx[0] = 0;
	timedShowCursor();
}


void JoyceFastCRT::lf(bool scale)
{
	int steps = (scale && isDoubleHeight()) ? 2 : 1;

	hideCursor();
	for (int n = 0; n < steps; n++)
	{
		if (m_vars.pagemode[0])
		{
			++m_vars.cury[0];
			if (m_vars.cury[0] > m_vars.height[0]) m_vars.cury[0] = 0;
			clearCurLine();
		}
		else if (m_vars.cury[0] < m_vars.height[0]) 
		{
			++m_vars.cury[0];
		}	
		else
		{
			unsigned y, y0, x0;
			/* Scroll window up */
	
			y0 = 8 * m_vars.top[0];
			x0 = m_vars.left[0];

			for (y = 1; y <= m_vars.height[0]; y++)
			{
				((JoycePcwTerm *)gl_sys->m_defaultTerminal)->copy_screen(y0 + 8 * y, x0, y0 + 8 * (y - 1), x0, m_vars.width[0] + 1, 1);

			}
			clearRect(m_vars.left[0], m_vars.top[0] + m_vars.height[0], m_vars.width[0] + 1, 1);
		}
	}
	timedShowCursor();
}


void JoyceFastCRT::reverseLF()
{
	hideCursor();

	if (m_vars.pagemode[0])
	{
		if (m_vars.cury[0] == 0) m_vars.cury[0] = m_vars.height[0];
		else --m_vars.cury[0];
		clearCurLine();
		timedShowCursor();
		return;
	}
	if (m_vars.cury[0] > 0)
	{
		--m_vars.cury[0];
	}	
	else
	{
		unsigned y, y0, x0;
		/* Scroll window down */

		y0 = 8 * m_vars.top[0];
		x0 = m_vars.left[0];

		for (y = m_vars.height[0]; y >= 1; y--)
		{
			((JoycePcwTerm *)gl_sys->m_defaultTerminal)->copy_screen(y0 + 8 * (y-1), x0, y0 + 8 * y, x0, m_vars.width[0] + 1, 1);

		}
		clearCurLine();	
	}
	timedShowCursor();
}



void JoyceFastCRT::cursorLeft(bool wrap, bool scale)
{
	int steps = 1;

	if (!m_vars.wrap[0]) wrap = false;

	hideCursor();
	if (scale && isDoubleWidth()) steps = 2;
	for (int n = 0; n < steps; n++)
	{
		if (m_vars.curx[0] > 0)
		{
			--m_vars.curx[0];
		}
		else if (wrap && m_vars.cury[0] > 0)
		{
			m_vars.curx[0] = m_vars.width[0];
			--m_vars.cury[0];	
		}
	}
	timedShowCursor();
}

void JoyceFastCRT::cursorRight(bool wrap, bool scale)
{
	int steps = 1;

	if (!m_vars.wrap[0]) wrap = false;

	if (scale && isDoubleWidth()) steps = 2;
	hideCursor();
	for (int n = 0; n < steps; n++)
	{
		++m_vars.curx[0];
		if (m_vars.curx[0] > m_vars.width[0])
		{
			if (m_vars.wrap[0])
			{
				cr(); lf();
			}
			else	m_vars.curx[0] = m_vars.width[0];
		}
	}
	timedShowCursor();
}

void JoyceFastCRT::cursorUp(bool wrap, bool scale) 
{
	int steps = 1;

	if (scale && isDoubleHeight()) steps = 2;
	
	for (int n = 0; n < steps; n++)
	{
		if (wrap && isWrap())
		{
			reverseLF();
		}
		else if (m_vars.cury[0] > 0)
		{
			hideCursor();
			--m_vars.cury[0];
			timedShowCursor();
		}
	}
}

void JoyceFastCRT::cursorDown(bool wrap, bool scale) 
{ 
	int steps = 1;

	if (scale && isDoubleHeight()) steps = 2;

	for (int n = 0; n < steps; n++)
	{
		if (wrap && isWrap())
		{
			lf();
		}
		else if (m_vars.cury[0] < m_vars.height[0])
		{
			hideCursor();
			++m_vars.cury[0];
			timedShowCursor();
		}
	}
}

void JoyceFastCRT::writeSys(unsigned char ch)
{
	/* If status line is disabled put this through the base class write() */
	if (!m_vars.stl_on[0])
	{
		write(ch);
		return;
	}	
	if (ch == 7)
	{
		bel();
		return;
	}
	hideCursor();
	if (ch == 0x0D)
	{
		m_vars.ric_xpos[0] = 0;
		timedShowCursor();
		return;	
	}
	if (ch == 0x0A)
	{
		clearRect(0, 31, 90, 1);
		timedShowCursor();
		return;	
	}
	zbyte *charbits = PCWRAM + 0xB800 + 8 * ch;
	int y, y0, x0;
	zbyte buf[8];


	memcpy(buf, charbits, 8);

	y0 = 31 * 8;
	x0 = m_vars.ric_xpos[0];
	for (y = 0; y < 8; y++)
	{
		((JoycePcwTerm *)gl_sys->m_defaultTerminal)->store_screen(y0 + y, x0, buf[y]);
	}
	++m_vars.ric_xpos[0];
	if (m_vars.ric_xpos[0] > 89)
		m_vars.ric_xpos[0] = 89;
	timedShowCursor();
}

void JoyceFastCRT::hideCursor()
{
	if (m_vars.curshown[0])
	{
		drawCursor();
		m_vars.curshown[0] = 0;

		/* Disable cursor paint event */
		unsigned ev = m_vars.curevent[0] + 256 * m_vars.curevent[1];

		if (ev)
		{
			PCWRAM[ev + 2] = 0;
		}
	}
}


void JoyceFastCRT::timedShowCursor()
{
	unsigned ev = m_vars.curevent[0] + 256 * m_vars.curevent[1];

	if (ev)
	{
		PCWRAM[ev + 2] = 10;
	}
}


void JoyceFastCRT::drawCursor()
{
	unsigned x0, y0, y;

	y0 = (m_vars.cury[0] + m_vars.top[0]) * 8;	
	x0 = (m_vars.curx[0] + m_vars.left[0]);	

	for (y = 0; y < 8; y++)
	{
		zbyte tmp = ((JoycePcwTerm *)gl_sys->m_defaultTerminal)->fetch_screen(y0 + y, x0);
		((JoycePcwTerm *)gl_sys->m_defaultTerminal)->store_screen(y0 + y, x0, ~tmp);
	}
}


void JoyceFastCRT::enableCursor(bool b) 
{ 
	m_vars.showcur[0] = b ? 0xFF : 0;
}


bool JoyceFastCRT::isCursorEnabled() 
{
	return (m_vars.showcur[0] != 0); 
}


void JoyceFastCRT::drawChar(unsigned char ch, int message)
{
	zbyte *charbits = PCWRAM + 0xB800 + 8 * ch;
	zbyte *char2;
	int y, y0, x0;
	zword buf[16];
	bool dwidth = isDoubleWidth();
	bool italic = isItalic();

	hideCursor();

	for (y = 0; y < 8; y++) buf[y] = charbits[y];
	/* Apply special effects to buf */

	if (m_vars.and_mask >= 0)
	{
		char2 = PCWRAM + 0xB800 + 8 * (m_vars.and_mask & 0xFF);
		for (y = 0; y < 8; y++) buf[y] &= char2[y];	
	}
	if (m_vars.or_mask >= 0)
	{
		char2 = PCWRAM + 0xB800 + 8 * (m_vars.or_mask & 0xFF);
		for (y = 0; y < 8; y++) buf[y] |= char2[y];	
	}
	if (m_vars.xor_mask >= 0)
	{
		char2 = PCWRAM + 0xB800 + 8 * (m_vars.xor_mask & 0xFF);
		for (y = 0; y < 8; y++) buf[y] ^= char2[y];	
	}
	if (dwidth)
	{
		for (y = 0; y < 8; y++)
		{
			zword w = stretch[buf[y] & 0xFF];
			if (italic) switch(y)
			{
				case 0: w = w >> 3; break;
				case 1: w = w >> 2; break;
				case 2: w = w >> 1; break;
				case 5: w = w << 1; break;
				case 6: w = w << 2; break;
				case 7: w = w << 3; break;
			}
			buf[y] = w;
		}
	}
	else
	{
		if (isItalic())
		{
			buf[0] = buf[0] >> 2;
			buf[1] = buf[1] >> 1;
			buf[2] = buf[2] >> 1;
			buf[5] = buf[5] << 1;
			buf[6] = buf[6] << 1;
			buf[7] = buf[7] << 2;
		}
	}
	if (isBold())
	{
		for (y = 0; y < 8; y++) 
		{
			buf[y] |= (buf[y] >> 1);
		}
	}
	if (isDim())
	{
		for (y = 0; y < 8; y+=2) 
		{
			buf[y] &= 0x5555;
			buf[y+1] &= 0xAAAA;
		}
	}
	/* Cater for double height */
	if (isDoubleHeight())
	{
		for (y = 15; y >= 0; y--)
		{
			buf[y] = buf[y/2];
		}
		if (isUnderline()) 
		{
			buf[15] |= m_vars.ul_mask[0];
			buf[15] |= (m_vars.ul_mask[0] << 8);
		}
	}
	else
	{
		if (isUnderline()) 
		{
			buf[7] |= m_vars.ul_mask[0];
			buf[7] |= (m_vars.ul_mask[0] << 8);
		}
	}
	if (isReverse())
	{
		for (y = 0; y < 16; y++) buf[y] = ~buf[y];
	}

	y0 = (m_vars.cury[0] + m_vars.top[0]) * 8;	
	x0 = (m_vars.curx[0] + m_vars.left[0]);	

	if (isDoubleHeight() && dwidth)
	{
		drawGlyph(buf, true);
		lf(false);
		drawGlyph(buf+8, true);
		cursorUp(true, false);
		cursorRight(true, false);
		drawGlyph(buf, false);
		lf(false);
		drawGlyph(buf+8, false);
		cursorUp(true, false);
	}
	else if (isDoubleHeight())
	{
		drawGlyph(buf, false);
		lf(false);
		drawGlyph(buf+8, false);
		cursorUp(true, false);
	}
	else if (dwidth)
	{
		drawGlyph(buf, true);
		cursorRight(true, false);
		drawGlyph(buf, false);
	}
	else
	{
		drawGlyph(buf, false);
	}
	cursorRight(true, false);
	timedShowCursor();
}

void JoyceFastCRT::drawGlyph(zword *buf, bool shift)
{
	unsigned y, y0, x0;
	y0 = (m_vars.cury[0] + m_vars.top[0]) * 8;	
	x0 = (m_vars.curx[0] + m_vars.left[0]);	

	for (y = 0; y < 8; y++)
	{
		((JoycePcwTerm *)gl_sys->m_defaultTerminal)->store_screen(
			y0 + y, x0, shift ? (buf[y] >> 8) : (buf[y] & 0xFF));
	}
}


void JoyceFastCRT::moveCursor(zbyte x, zbyte y, bool scale)
{
	hideCursor();

	if (isDoubleWidth() && scale) x *= 2;
	if (isDoubleHeight() && scale) y *= 2;

	if (x > m_vars.width[0]) x = m_vars.width[0];
	if (y > m_vars.height[0]) y = m_vars.height[0];

	m_vars.curx[0] = x;
	m_vars.cury[0] = y;	
	timedShowCursor();
}

void JoyceFastCRT::saveCursorPos() 
{ 
	m_vars.curx[2] = m_vars.curx[0];
	m_vars.cury[2] = m_vars.cury[0];
}

void JoyceFastCRT::loadCursorPos() 
{ 
	moveCursor(m_vars.curx[2], m_vars.cury[2], false);
}


void JoyceFastCRT::setWindow(zbyte left, zbyte top, zbyte width, zbyte height)
{
	/* Limit to screen size, home cursor */	
	if (left >= 90) left = 89;
	if (top >= 32) top = 31;
	if (left + width > 90) width = 90 - left;
	if (top + height > 32) height = 32 - top;

	m_vars.left[0] = left;
	m_vars.top[0] = top;
	m_vars.width[0] = width - 1;
	m_vars.height[0] = height - 1;

	hideCursor();
	home();
	timedShowCursor();
}

void JoyceFastCRT::clearRect(zbyte left, zbyte top, zbyte width, zbyte height)
{
	unsigned y0 = top * 8;
	unsigned x0 = left;
	unsigned y;

	hideCursor();
	for (y = 0; y < height; y++)
	{
		((JoycePcwTerm *)gl_sys->m_defaultTerminal)->zap_screen(y0 + 8 * y, x0, width, 1);
	}
	timedShowCursor();
}


void JoyceFastCRT::clearWindow() 
{
	hideCursor();
	clearRect(m_vars.left[0], m_vars.top[0], m_vars.width[0] + 1, m_vars.height[0] + 1);
	timedShowCursor();
} 

void JoyceFastCRT::clearCurLine()
{
	hideCursor();
	clearRect(m_vars.left[0], m_vars.top[0] + m_vars.cury[0], m_vars.width[0] + 1, 1);
	timedShowCursor();
}

void JoyceFastCRT::clearEOL() 
{
	hideCursor();
	clearRect(m_vars.left[0] + m_vars.curx[0], 
		   m_vars.top[0] + m_vars.cury[0], 
		   m_vars.width[0] - m_vars.curx[0] + 1, 1);
	if (isDoubleHeight())
	{
		lf(false);
		clearRect(m_vars.left[0] + m_vars.curx[0], 
			   m_vars.top[0] + m_vars.cury[0], 
			   m_vars.width[0] - m_vars.curx[0] + 1, 1);

		cursorUp(true, false);
	}
	timedShowCursor();
} 

void JoyceFastCRT::clearBOS() 
{
	hideCursor();
	int xa = isDoubleWidth() ? 2 : 1;

	if (m_vars.top[0] < m_vars.cury[0])
	{
		clearRect(m_vars.left[0], m_vars.top[0],
			   m_vars.width[0] + 1, m_vars.cury[0] - m_vars.top[0]);
	}
	clearRect(m_vars.left[0], m_vars.top[0] + m_vars.cury[0], 
		   m_vars.curx[0] + xa, 1);
	if (isDoubleHeight())
	{
		lf(false);
		clearRect(m_vars.left[0], m_vars.top[0] + m_vars.cury[0], 
			   m_vars.curx[0] + xa, 1);

		cursorUp(true, false);
	}
	timedShowCursor();
} 

void JoyceFastCRT::clearEOS() 
{
	int ya = isDoubleHeight() ? 2 : 1;

	hideCursor();
	if (m_vars.cury[0] + 1 < m_vars.height[0])
	{
		clearRect(m_vars.left[0], m_vars.cury[0] + ya,
			   m_vars.width[0] + 1, m_vars.height[0] - m_vars.cury[0] - (ya-1));
	}
	clearRect(m_vars.left[0] + m_vars.curx[0], 
		   m_vars.top[0] + m_vars.cury[0], 
		   m_vars.width[0] - m_vars.curx[0] + 1, 1);
	if (isDoubleHeight())
	{
		lf(false);
		clearRect(m_vars.left[0] + m_vars.curx[0], 
			   m_vars.top[0] + m_vars.cury[0], 
			   m_vars.width[0] - m_vars.curx[0] + 1, 1);
		cursorUp(true, false);
	}
	timedShowCursor();
} 

void JoyceFastCRT::clearBOL() 
{
	int xa = isDoubleWidth() ? 2 : 1;
	hideCursor();
	clearRect(m_vars.left[0], m_vars.top[0] + m_vars.cury[0], 
		   m_vars.curx[0] + xa, 1);
	if (isDoubleHeight())
	{
		lf(false);
		clearRect(m_vars.left[0], m_vars.top[0] + m_vars.cury[0], 
		   m_vars.curx[0] + xa, 1);

		cursorUp(true, false);
	}
	timedShowCursor();
}

void JoyceFastCRT::insertLine()
{
	unsigned y, y0, x0;
	int lines = isDoubleHeight() ? 2 : 1;
	hideCursor();

	for (int n = 0; n < lines; n++)
	{
		y0 = 8 * m_vars.top[0];
		x0 = m_vars.left[0];

		for (y = m_vars.height[0]; y >= m_vars.cury[0] + 1; y--)
		{
			((JoycePcwTerm *)gl_sys->m_defaultTerminal)->copy_screen(y0 + 8 * (y-1), x0, y0 + 8 * y, x0, m_vars.width[0] + 1, 1);
		}
		clearCurLine();
	}
	timedShowCursor();
}

void JoyceFastCRT::deleteLine() 
{
	unsigned y, y0, x0;
	int lines = isDoubleHeight() ? 2 : 1;

	hideCursor();
	for (int n = 0; n < lines; n++)
	{
		y0 = 8 * m_vars.top[0];
		x0 = m_vars.left[0];

		for (y = m_vars.cury[0]; y < m_vars.height[0]; y++)
		{
			((JoycePcwTerm *)gl_sys->m_defaultTerminal)->copy_screen(y0 + 8 * (y+1), x0, y0 + 8 * y, x0, m_vars.width[0] + 1, 1);
		}
		clearRect(m_vars.left[0], m_vars.top[0] + m_vars.height[0], m_vars.width[0] + 1, 1);
	}
	timedShowCursor();
}

void JoyceFastCRT::deleteChar() 
{ 
	unsigned y0, x0;

	hideCursor();
	int lines = isDoubleHeight() ? 2 : 1;
	for (int n = 0; n < lines; n++)
	{
		if (n == 1) lf(false);
		y0 = 8 * (m_vars.top[0] + m_vars.cury[0]);
		x0 = m_vars.curx[0];
		if (isDoubleWidth())
		{
			((JoycePcwTerm *)gl_sys->m_defaultTerminal)->copy_screen(y0, x0 + 2, y0, x0, m_vars.width[0] - m_vars.curx[0] - 1, 1);
			clearRect(m_vars.left[0] + m_vars.width[0] - 1, m_vars.top[0] + m_vars.cury[0], 2, 1);
		}
		else
		{
			((JoycePcwTerm *)gl_sys->m_defaultTerminal)->copy_screen(y0, x0 + 1, y0, x0, m_vars.width[0] - m_vars.curx[0], 1);
			clearRect(m_vars.left[0] + m_vars.width[0], m_vars.top[0] + m_vars.cury[0], 1, 1);
		}
		if (n == 1) cursorUp(true, false);
	}
	timedShowCursor();

}

void JoyceFastCRT::setForeground(zbyte r, zbyte g, zbyte b)
{
	zbyte encoded = ((r & 0xC0) >> 2) | ((g & 0xC0) >> 4) | ((b & 0xC0) >> 6);

/* If colour changes are enabled, set the PCW palette 'registers' to the 
 * CPC-like default (yellow on blue) and control the actual palette in the 
 * emulator */
	if (m_colour)
	{
		gl_sys->m_activeTerminal->setForeground(r, g, b);
		m_vars.palette[4] = m_vars.palette[5] = 60;
		m_vars.palette[2] = m_vars.palette[3] = 1;
	}
	else
	{
/* Otherwise do like the actual PCW terminal emulator and write to the 
 * palette 'registers' */
		m_vars.palette[4] = m_vars.palette[5] = encoded;
	}
}

void JoyceFastCRT::setBackground(zbyte r, zbyte g, zbyte b)
{
	zbyte encoded = ((r & 0xC0) >> 2) | ((g & 0xC0) >> 4) | ((b & 0xC0) >> 6);

	if (m_colour)
	{
/* If colour changes are enabled, set the PCW palette 'registers' to the 
 * CPC-like default (yellow on blue) and control the actual palette in the 
 * emulator */
		gl_sys->m_activeTerminal->setBackground(r, g, b);
		m_vars.palette[4] = m_vars.palette[5] = 60;
		m_vars.palette[2] = m_vars.palette[3] = 1;
	}
	else
	{
/* Otherwise do like the actual PCW terminal emulator and write to the 
 * palette 'registers' */
		m_vars.palette[2] = m_vars.palette[3] = encoded;
	}
}



void JoyceFastCRT::set24x80(bool b24x80)
{
	m_vars.is_24x80[0] = b24x80 ? 0xFF : 0;
	setMode(2);
}

void JoyceFastCRT::setMode(zbyte mode) 
{
	if (mode != 2) m_vars.is_24x80[0] = 0; 
/* Mode set. Clear window, reset page mode */
	clearWindow();
	m_vars.pagemode[0] = 0;
	recalcBounds();
}

void JoyceFastCRT::recalcBounds()
{
	unsigned top, left, height, width; 
/* JOYCE does not emulate a 200-line PCW, but if it did the bounds would
 * have to be set here. */

	m_vars.reqwidth[0] = 89;
	m_vars.reqheight[0] = 31;	/* Or 24 on a 200-line PCW */
	
	if (m_vars.is_24x80[0])		/* 24x80 mode */
	{
		left = 5;
		top = 4;		/* Or zero on a 200-line PCW */
		width = 79;
		height = 23;			
	}
	else if (m_vars.stl_on[0])	/* Status line on, 31x90 */
	{
		left = 0;
		top = 0;
		height = 30;
		width = 89;
	}
	else				/* Status line off, 32x90 */
	{
		left = 0;
		top = 0;
		height = 31;
		width = 89;
	}
	m_vars.left[0] = left;
	m_vars.top[0] = top;
	m_vars.width[0] = width;	
	m_vars.height[0] = height;

	/* Ensure cursor is within the new bounds */
	if (m_vars.cury[0] > m_vars.height[0] || m_vars.curx[0] > m_vars.width[0])
	{
		hideCursor();
		m_vars.curx[0] = m_vars.width[0];
		m_vars.cury[0] = m_vars.height[0];
		timedShowCursor();
	}

}


bool JoyceFastCRT::isStatusEnabled() 
{ 
	return (m_vars.stl_on[0] != 0); 
}


void JoyceFastCRT::enableStatus(bool b) 
{ 
	m_vars.stl_on[0] = (b) ? 0xFF : 0;

	recalcBounds();
}



static const Uint8 lang1[]=
	{	0x40, 0xEA,	/* Language 1 : @ -> a grave */ 
		0x5B, 0xA2,	/*              [ -> degree */
		0x5C, 0xF5,	/* 		\ -> c cedilla */
		0x5D, 0xA6,	/*		] -> Section */
		0x7B, 0xE1,	/*		{ -> e acute */
		0x7C, 0xEE,	/*		| -> u grave */
		0x7D, 0xEB,	/*		} -> e grave */
		0x7E, 0xB2,	/*		~ -> umlaut */
		0x00, 0x00	};

static const Uint8 lang2[]=
	{	0x40, 0xA6,	/* Language 2 : @ -> Section */ 
		0x5B, 0xD0,	/*              [ -> A umlaut */
		0x5C, 0xD3,	/* 		\ -> O umlaut */
		0x5D, 0xD4,	/*		] -> U umlaut */
		0x7B, 0xF0,	/*		{ -> a umlaut */
		0x7C, 0xF3,	/*		| -> o umlaut*/
		0x7D, 0xF4,	/*		} -> u umlaut */
		0x7E, 0xBA,	/*		~ -> double s */
		0x00, 0x00	};

static const Uint8 lang3[]=
	{	0x23, 0xA3,	/* Language 3 : # ->  */
		0x00, 0x00	};

static const Uint8 lang4[]=
	{	0x5B, 0xD6,	/* Language 4 : [ -> AE */
		0x5C, 0xD8,	/* 		\ -> O slash */
		0x5D, 0xD7,	/*		] -> A ring */
		0x7B, 0xF6,	/*		{ -> ae */
		0x7C, 0xF8,	/*		| -> o slash*/
		0x7D, 0xF7,	/*		} -> a ring */
		0x00, 0x00	};

static const Uint8 lang5[]=
	{	0x40, 0xC1,	/* Language 5 : @ -> E acute */
		0x5B, 0xD0,	/*              [ -> A umlaut */
		0x5C, 0xD3,	/* 		\ -> O umlaut */
		0x5D, 0xD7,	/*		] -> A ring */
		0x5E, 0xD4,	/*		^ -> U umlaut */
		0x7B, 0xF0,	/*		{ -> a umlaut */
		0x7C, 0xF3,	/*		| -> o umlaut*/
		0x7D, 0xF7,	/*		} -> a ring */
		0x7E, 0xF4,	/*		~ -> u umlaut */
		0x00, 0x00	};

static const Uint8 lang6[]=
	{	0x5B, 0xA2,	/* Language 6 : [ -> degree */
		0x5D, 0xE1,	/*		] -> e acute */
		0x60, 0xEE,	/*		` -> u grave */
		0x7B, 0xEA,	/*		{ -> a grave */
		0x7C, 0xED,	/*		| -> o grave */
		0x7D, 0xEB,	/*		} -> e grave */
		0x7E, 0xEC,	/*		~ -> i grave */
		0x00, 0x00	};

static const Uint8 lang7[]=
	{	0x5B, 0xAF,	/* Language 7 : [ -> upside-down pling */
		0x5C, 0xD9,	/*		\ -> N tilde */
		0x5D, 0xAE,	/*		] -> upside-down query */
		0x7B, 0xB2,	/*		{ -> umlaut */
		0x7C, 0xF9,	/*		| -> n tilde */
		0x00, 0x00	};



static const Uint8 *swaptbl[8] = 	
				/* Character swaps in language <n> */
{
	NULL,			/* Language 0 : no swaps */
	lang1, lang2, lang3,
	lang4, lang5, lang6, lang7
};


void JoyceFastCRT::setLanguage(int language) 
{
	/* Language already selected, nothing to do */
	if (m_vars.language[0] == language) return;

	swapLanguage(m_vars.language[0]);
	m_vars.language[0] = language;
	swapLanguage(m_vars.language[0]);
}

void JoyceFastCRT::swapLanguage(zbyte language)
{
	const Uint8 *swapp = swaptbl[language & 7];
	Uint8 b1, b2;
	Uint8 swbuf[8];
	zbyte *font = PCWRAM + 0xB800;

	do {
		b1 = swapp[0];
		b2 = swapp[1];
		swapp += 2;
		memcpy(swbuf,         font + 8 * b1, 8);
		memcpy(font + 8 * b1, font + 8 * b2, 8);
		memcpy(font + 8 * b2, swbuf,         8);
	} while (b1 != b2);
}


void JoyceFastCRT::setAndMask(int mask)
{
	m_vars.and_mask = mask;
}

void JoyceFastCRT::setOrMask(int mask)
{
	m_vars.or_mask = mask;
}

void JoyceFastCRT::setXorMask(int mask)
{
	m_vars.xor_mask = mask;
}

bool JoyceFastCRT::loadWav()
{
	SDL_AudioCVT cvt;
	SDL_AudioSpec wave;
	Uint8 *data;
	Uint32 dlen;

	string file = findPcwFile(FT_NORMAL, "beep_fm.wav", "rb");

	if (!SDL_LoadWAV(file.c_str(), &wave, &data, &dlen))
	{
		alert("Failed to load " + file);
		return false;
	}	
/* Convert to the format we're using for the beeper */
	SDL_BuildAudioCVT(&cvt, wave.format, wave.channels, wave.freq, 
			AUDIO_U8, 1, 25000);
	cvt.buf = (Uint8*)malloc(dlen*cvt.len_mult);
	memcpy(cvt.buf, data, dlen);
	cvt.len = dlen;
	SDL_ConvertAudio(&cvt);
	SDL_FreeWAV(data);
	m_beepBuffer = cvt.buf;
	m_beepLen = cvt.len;
	return true;
}

void JoyceFastCRT::bel() 
{ 
	gl_sys->m_beeper.playWave(m_beepBuffer, m_beepLen);
}

