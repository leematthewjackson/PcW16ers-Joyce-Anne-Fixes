/************************************************************************

    JOYCE v2.1.7 - Amstrad PCW emulator

    Copyright (C) 1996, 2001, 2025  John Elliott <seasip.webmaster@gmail.com>

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
#include "sdsdl.hxx"
#include "JoycePcwTerm.hxx"

#define nPTSIN  1	/* Indices into the control array for sizes of */
#define nPTSOUT 2	/* the other arrays */
#define nINTIN  3
#define nINTOUT 4
#define nESC    5
extern int tr;

PcwGSX::PcwGSX(PcwSystem *s) : PcwSdlTerm("gsx", s)
{
	m_sys = s;
	oldtype = -1;
	m_term = NULL;
	m_sdsdl = NULL;
	m_height = 0;
	m_locatorMode = 1;
	m_pngDir = findPcwDir(FT_PNGS, false);
	m_pdfDir = findPcwDir(FT_PDF, false);
	checkExistsDir(m_pngDir);
	checkExistsDir(m_pdfDir);
	m_pdfParam.width = 595;
	m_pdfParam.height = 842;
	m_pdfParam.outputdir = m_pdfDir.c_str();
	m_pngsurface = NULL;
	m_screen = NULL;
	m_pdf = NULL;
	m_png = NULL;
	m_pbm = NULL;
	memset (m_txtScreen, 0, sizeof(m_txtScreen));
}

PcwGSX::~PcwGSX()
{
	if (m_screen != NULL)
	{
		(*m_screen->destroy)(&m_screen);
	}
	if (m_pdf != NULL)
	{
		(*m_pdf->destroy)(&m_pdf);
	}
	if (m_png != NULL)
	{
		(*m_png->destroy)(&m_png);
	}
	if (m_pbm != NULL)
	{
		(*m_pbm->destroy)(&m_pbm);
	}
	if (m_pngsurface != NULL)
	{
		(*m_pngsurface->destroy)(&m_pngsurface);
	}
	if (m_pbmsurface != NULL)
	{
		(*m_pbmsurface->destroy)(&m_pbmsurface);
	}
	if (m_sdsdl) delete m_sdsdl;
	if (m_term) delete m_term;
	m_term = NULL;
}





GPoint *PcwGSX::getPtsin()
{
	int n;
	int max = 1 + getArray(ctrl, nPTSIN); 
	GPoint *pt = new GPoint[max];

	for (n = 0; n < max; n++)
	{
		pt[n].x = getArray(ptsin, n * 2);
		pt[n].y = m_height - getArray(ptsin, n * 2 + 1);
	}
	return pt;
}



void PcwGSX::getColour(Uint32 *col, zword array, zword off)
{
	Uint8 r, g, b;
	zword nc = getArray(array, off);
	if (nc >= 128) nc = 127;

	m_sysVideo->indexToColour(nc, &r, &g, &b);

	*col = r;
	*col = (*col << 8) | g;
	*col = (*col << 8) | b;
	*col = (*col << 8) | 0xff;	/* Alpha */
}



void PcwGSX::setColour(Uint32 *ptr)
{
	int col = getWord(intin);
	getColour(ptr, intin, 0);
	m_sys->m_cpu->storeW(intout, (col > 127) ? 127 : col);
	putArray(ctrl, nINTOUT, 1);	
}





/********************************************************************/

// Default palette 
int gsxInitPal[24] = 
{
	   0,    0,    0,	/* 0: Black */
	1000,    0,    0,	/* 1: Red */
	   0, 1000,    0,	/* 2: Green */
	   0,    0, 1000,	/* 3: Blue */
	   0, 1000, 1000,	/* 4: Cyan */
	1000, 1000,    0,	/* 5: Yellow */
	1000,    0, 1000,	/* 6: Magenta */
	1000, 1000, 1000,	/* 7: White */
};



void PcwGSX::openWorkstation(void)
{
	int n;
	GWSParams param;
	GWSResult result;
	GPoint pt[4];
	long styles[8];

	for (n = 0; n < 9; n++)
	{
		param.defaults[n] = getArray(intin, n);
		param.xymode = 1;	/* We are scaling */
	}
        m_oldTerm = m_sys->m_activeTerminal;
	m_sys->selectTerminal(this);

	m_term = new SdlTerm(m_sysVideo->getSurface());
	SDL_Rect &rcTerm = m_term->getBounds();

	/* GSX likes a black background */
	m_term->setBackground(0,0,0);
	m_term->enableCursor(false);
	m_sdsdl = new SdSdl();
	if (!m_sdsdl->open(&param, &result, m_term->getSurface()))
	{
		/* Should never happen */
		return;
	}
	putArray(ctrl, nPTSOUT, 6);
	putArray(ctrl, nINTOUT, 45);

	/* GSX drivers use raster coordinates that have 0 at the bottom
	 * and work up. GEM drivers start at the top and work down. So store
	 * the screen height, and do the conversion here. */
	m_height = result.data[1] - 1;
	/* Similarly, text coordinates need adjusting by the height of the 
	 * font. */
	m_sdsdl->queryText(styles, pt);	// Get text height
	m_txtheight = pt[1].y;		// And save it.

	m_mintxtheight = result.points[0].y + 1;
	for (n = 0; n < 45; n++) 
	{
		putArray(intout, n, result.data[n]);
	}
	// Since SDSDL doesn't yet support multiple fonts, present GEM's 
	// text effects as fonts.
	putArray(intout, 10, 16);	
	for (n = 0; n < 6; n++)
	{
		putArray(ptsout, n*2,   result.points[n].x);
		putArray(ptsout, n*2+1, result.points[n].y);
	}
	// [2.5.4] Set up the palette specified in the DRI GSX manual
	for (n = 0; n < 256; n++)
	{
		if (n < 8) m_sdsdl->setColor(n, gsxInitPal[n*3], 
				gsxInitPal[n*3+1], gsxInitPal[n*3+2]);
		else	   m_sdsdl->setColor(n, 0xFF, 0xFF, 0xFF);
	}
	/* Do this after we've rearranged the palette */
	m_term->setBackground(0, 0, 0);
	m_term->setForeground(0xFF, 0xFF, 0xFF);
	clearPicture();
	m_lastX = -1; m_lastY = -1;
}


void PcwGSX::closeWorkstation()
{
	m_sdsdl->close();
	delete m_sdsdl;
	m_sdsdl = NULL;
	m_term->enableCursor(true);
	m_sys->selectTerminal(m_oldTerm);
	delete m_term;
	m_term = NULL;
}


void PcwGSX::clearPicture()
{
	m_sdsdl->clear();
}

void PcwGSX::updateWorkstation()
{
	m_sdsdl->flush();
}



void PcwGSX::escape(void)
{
	int x,y,txtc;
	GPoint pt;
//
// Rather than use SdSdl for (at the moment) nonexistent escape support, 
// implement them here using the underlying terminal emulator.
//
	switch(getArray(ctrl, nESC))
	{
		case 1:	/* Text screen size in cells */
		putArray(ctrl, nINTOUT, 2);
		putArray(intout, 0, 37);	
		putArray(intout, 1, 100);
		break;

		case 2: /* Enter gfx mode */
//		m_sdsdl->leaveCur();
		m_term->enableCursor(false);
		m_sdsdl->clear();
		break;

		case 3: /* Exit gfx mode */
//		m_sdsdl->enterCur();
		m_sdsdl->clear();
		m_term->enableCursor(true);
		break;

		case 4: /* Cursor up */ m_term->cursorUp(false); break;
		case 5: /* Cursor dn */ m_term->cursorDown(false); break;
		case 6: /* Cursor rt */ m_term->cursorRight(false); break;
		case 7: /* Cursor lt */ m_term->cursorLeft(false); break;
		case 8: /* Cursor hm */ m_term->home(); break;
		case 9: /* Erase EOS */ m_term->clearEOS(); break;
		case 10: /* Erase EOL */ m_term->clearEOL(); break;

		case 11: /* Position cursor */
		m_term->moveCursor(getArray(intin, 1),getArray(intin, 0));
		break;

		case 12: /* Text out */
		for (txtc = 0; txtc < getArray(ctrl, nINTIN); txtc++)
		{
			(*m_term) << ((unsigned char)(getArray(intin, txtc)));
		}
		break;

		case 13: /* Rev vid   */ m_term->setReverse(1); break;
		case 14: /* True vid  */ m_term->setReverse(0); break;
		
		case 15: /* Get cursor position */
		m_term->getCursorPos(&x, &y);
		putArray(ctrl, nINTOUT, 2);
		putArray(intout, 0, y);
		putArray(intout, 1, x);
		break;

		case 16:		/* Yes, we have a pointing device */
		putArray(ctrl, nINTOUT, 1);
		putArray(intout, 0, 1);
		break;
		
		case 17:		/* No hardcopy */
		break;

		case 18:		/* Show pointer */
		pt.x = getArray(ptsin, 0);
		pt.y = getArray(ptsin, 1);
		SDL_WarpMouse(pt.x, m_height - pt.y);
		SDL_ShowCursor(1);
//		m_sdsdl->escape(18, 1, &pt, 0, NULL, NULL, NULL, NULL, NULL, NULL);
		break;

		case 19:		/* Hide pointer */
		SDL_ShowCursor(0);
//		m_sdsdl->escape(19, 0, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
		break;
	}

}



void PcwGSX::drawPolyline(void)
{
	GPoint *pt = getPtsin();	
	m_sdsdl->polyLine(getArray(ctrl, nPTSIN), pt);
	delete pt;
}


void PcwGSX::drawPolymarker(void)
{
	GPoint *pt = getPtsin();	
	m_sdsdl->polyMarker(getArray(ctrl, nPTSIN), pt);
	delete pt;
}

void PcwGSX::fillArea(void)
{
	GPoint *pt = getPtsin();	
	m_sdsdl->fillArea(getArray(ctrl, nPTSIN), pt);
	delete pt;
}

void PcwGSX::graphicText(void)
{
	GPoint pt;
	int txtlen = getArray(ctrl, nINTIN);
	wchar_t *str = new wchar_t[1 + txtlen];
	int n;
	
	for (n = 0; n < txtlen; n++)
	{
		str[n] = getArray(intin, n);
	}
	str[n] = 0;
	pt.x = getArray(ptsin, 0);
	pt.y = (m_height - getArray(ptsin, 1));
	m_sdsdl->graphicText(&pt, str);
	delete str;
}



void PcwGSX::outputIndexArray(void)
{
	int x , y, rx, ry, c;
	int rowlen  = getArray(ctrl, 5);
	int rowused = getArray(ctrl, 6);
	int nrows   = getArray(ctrl, 7);
	int wmode   = getArray(ctrl, 8);
	int nint    = (rowlen * nrows) + 1;
	long *iv = new long[1 + nint];
	GPoint pt[2];
	
	pt[0].x = getArray(ptsin, 0);
	pt[0].y = (m_height - getArray(ptsin, 1));
	pt[1].x = getArray(ptsin, 2);
	pt[1].y = (m_height - getArray(ptsin, 3));

	for (y = 0; y < nint; y++)
	{
		iv[y] = getArray(intin, y);
	}
	m_sdsdl->cellArray(pt, nint, iv, rowlen, rowused, nrows, wmode);
	delete [] iv;
}



void PcwGSX::gdp()
{
	int npts = getArray(ctrl, nPTSIN);
	int nint = getArray(ctrl, nINTIN);
	GPoint *pt = new GPoint[1 + npts];
	long *iv = new long[1 + nint];
	wchar_t *str = new wchar_t[1 + nint];
	int n;
	for (n = 0; n < npts; n++)
	{
		pt[n].x = getArray(ptsin, n*2);
		pt[n].y = m_height - getArray(ptsin, n*2+1);
	}
	for (n = 0; n < nint; n++)
	{
		iv[n] = str[n] = getArray(intin, n);
	}
	str[n] = 0;
	m_sdsdl->gdp(getArray(ctrl, nESC), npts, pt, nint, iv, str);
	delete str;
	delete iv;
	delete pt;
}


void PcwGSX::setTextDirection(void)
{
	m_sdsdl->stRotation((short)getArray(intin, 0));
}


void PcwGSX::setColourIndex(void)
{
	int red   = getArray(intin, 1);
	int green = getArray(intin, 2);
	int blue  = getArray(intin, 3);
	int ncol  = getArray(intin, 0);

	m_sdsdl->setColor(ncol, red, green, blue);
}

void PcwGSX::setLineStyle(void)
{
	long style = m_sdsdl->slType(getWord(intin));
	m_sys->m_cpu->storeW(intout, style);
	putArray(ctrl, nINTOUT, 1);
}


void PcwGSX::setMarkerStyle(void)
{
	long style = m_sdsdl->smType(getWord(intin));
	m_sys->m_cpu->storeW(intout, style);
	putArray(ctrl, nINTOUT, 1);
}

void PcwGSX::setTextStyle(void)
{
	// Since SDSDL doesn't yet support multiple fonts, present GEM's 
	// text effects as fonts.
	long style = m_sdsdl->stStyle(getWord(intin) - 1);
	m_sys->m_cpu->storeW(intout, 1 + style);
	putArray(ctrl, nINTOUT, 1);
}


void PcwGSX::setTextColour(void)
{
	long style = m_sdsdl->stColor(getWord(intin));
	m_sys->m_cpu->storeW(intout, style);
	putArray(ctrl, nINTOUT, 1);
}

void PcwGSX::setMarkerColour(void)
{
	long style = m_sdsdl->smColor(getWord(intin));
	m_sys->m_cpu->storeW(intout, style);
	putArray(ctrl, nINTOUT, 1);
}

void PcwGSX::setLineColour(void)
{
	long style = m_sdsdl->slColor(getWord(intin));
	m_sys->m_cpu->storeW(intout, style);
	putArray(ctrl, nINTOUT, 1);
}

void PcwGSX::setLineWidth(void)
{
	GPoint pt[2];

	pt[0].x = getArray(ptsin, 0);
	pt[0].y = getArray(ptsin, 1);
	m_sdsdl->slWidth(&pt[0], &pt[1]);
	m_txtheight = pt[1].y;
	putArray(ptsout, 0, pt[1].x);
	putArray(ptsout, 1, pt[1].y);
	putArray(ctrl, nPTSOUT, 1);
}

void PcwGSX::setTextHeight(void)
{
	GPoint pt[3];

	pt[0].x = getArray(ptsin, 0);
	pt[0].y = getArray(ptsin, 1);
	if (pt[0].y < m_mintxtheight) 	// Don't let it get unreadably small
	{
		pt[0].y = m_mintxtheight;
	}
	m_sdsdl->stHeight(&pt[0], &pt[1]);
	putArray(ptsout, 0, pt[1].x);
	putArray(ptsout, 1, pt[1].y);
	putArray(ptsout, 2, pt[2].x);
	putArray(ptsout, 3, pt[2].y);
	putArray(ctrl, nPTSOUT, 2);
}

void PcwGSX::setMarkerHeight(void)
{
	GPoint pt[2];

	pt[0].x = getArray(ptsin, 0);
	pt[0].y = getArray(ptsin, 1);
	m_sdsdl->smHeight(&pt[0], &pt[1]);
	putArray(ptsout, 0, pt[1].x);
	putArray(ptsout, 1, pt[1].y);
	putArray(ctrl, nPTSOUT, 1);
}

void PcwGSX::setFillStyle(void)
{
	long style = m_sdsdl->sfStyle(getWord(intin));
	m_sys->m_cpu->storeW(intout, style);
	putArray(ctrl, nINTOUT, 1);
}

void PcwGSX::setFillColour(void)
{
	long style = m_sdsdl->sfColor(getWord(intin));
	m_sys->m_cpu->storeW(intout, style);
	putArray(ctrl, nINTOUT, 1);
}

void PcwGSX::setFillIndex(void)
{
	long style = m_sdsdl->sfIndex(getWord(intin));
	m_sys->m_cpu->storeW(intout, style);
	putArray(ctrl, nINTOUT, 1);
}


void PcwGSX::inquireColour(void)
{
	long colour = getArray(intin, 0);
	short mode  = getArray(intin, 1);
	short r, g, b, v;

	v = m_sdsdl->qColor(colour, mode, &r, &g, &b);

	putArray(intout, 0, v);
	putArray(intout, 1, r);
	putArray(intout, 2, g);
	putArray(intout, 3, b);
	putArray(ctrl, nINTOUT, 4);
}


void PcwGSX::inquireCellArray(void)
{
	long rowlen  = getArray(ctrl, 5);
	long nrows   = getArray(ctrl, 6);
	long nint    = getArray(ctrl, nINTIN);
	long rused   = 0;
	long cused   = 0;
	int err;
	long r, c;

	long *result = new long[1 + (rowlen * nrows)];

	GPoint pt[2];
	
	pt[0].x = getArray(ptsin, 0);
	pt[0].y = (m_height - getArray(ptsin, 1));
	pt[1].x = getArray(ptsin, 2);
	pt[1].y = (m_height - getArray(ptsin, 3));

	err = m_sdsdl->inquireCellArray(pt, rowlen, nrows, &cused, &rused,
						result);
	for (r = 0; r < nrows; r++)
	{
		for (c = 0; c < rowlen; c++)
		{
			if (c < cused && r < rused)
				putArray(intout, r * rowlen + c, result[r * rowlen + c]);
			else	putArray(intout, r * rowlen + c, -1); 
		}
	}
	putArray(ctrl, nINTOUT, rowlen * rused);
	putArray(ctrl, 7, cused);
	putArray(ctrl, 8, rused);
	putArray(ctrl, 9, err);
}

/*
static void InquireCellArray(void)
{
	int x , y, rx, ry, c;
	short c1;
	int rowused = 0;         
	int nrused  = 0;
	int error   = 0;
	
	getptsin();
	ry = 0;
	for (y = Y0; y < Y1; y++)
	{
		rx  = 0;
		for (x = X0; x < X1; x++)
		{
			c = GrPixel(x, y);
			for (c1 = 0; c1 < 128; c1++)
				if (m_colourMap[c1] == c) break;
			if (c1 == 128) 
			{	
				c1 = (-1);
				error = 1;
			}
			putArray(intin, ry * rowlen + rx, c1);
			++rx; if (rx >= nrused) nrused = rx;
		}
		++ry; if (ry >= nrused) nrused = ry;
		if (nrused > nrows) break;
	}
	putArray(ctrl, 7, rowused);
	putArray(ctrl, 8, nrused);
	putArray(ctrl, 9, error);
}

*/



void PcwGSX::setWriteMode(void)
{
	int mode = getArray(intin, 0);
	putArray(ctrl, nINTOUT, 1);
	putArray(intout, 0, m_sdsdl->swrMode(mode));
}


void PcwGSX::readLocator(void)
{
// Don't try to integrate SDSDL's event loop with our own; instead, 
// handle the locator (the mouse) here.
//
	long dev;
	GPoint pt[2]; 
	short termch;
	Uint8 b;
	int x, y, state;
	SDL_Event ev;

	pt[0].x = getArray(ptsin, 0);
	pt[0].y = getArray(ptsin, 1);
	dev = getArray(intin, 0);
	termch = 0;
//	m_sdsdl->qLocator(dev, &pt[0], &pt[1], &termch);
	switch(m_locatorMode)
	{
		case 1: // Modal
			state = SDL_ShowCursor(-1);
			SDL_ShowCursor(1);
			while (!termch)
			{
				if (!SDL_WaitEvent(&ev)) break;

				switch(ev.type)
				{
					case SDL_QUIT: goodbye(99);
					case SDL_MOUSEBUTTONDOWN:
						pt[1].x = ev.button.x;
						pt[1].y = ev.button.y;
						switch(ev.button.button)
						{
							case SDL_BUTTON_LEFT:
							termch = 0x20;
							break;
							case SDL_BUTTON_MIDDLE:
							termch = 0x22;
							break;
							case SDL_BUTTON_RIGHT:
							termch = 0x21;
							break;
						}
						break;
				}
			}	
			SDL_ShowCursor(state);
			// Modal always returns coords and buttons
			putArray(ctrl, nINTOUT, 1);
			putArray(intout, 0, termch);
			putArray(ctrl, nPTSOUT, 1);
			putArray(ptsout, 0, pt[1].x);
			putArray(ptsout, 1, m_height - pt[1].y);
			break;
		case 2: // Modeless. Return the current state of the mouse
			// as recorded by the main JOYCE event loop.
			b = m_sys->getMouseBtn();
			pt[1].x = m_sys->getMouseX();
			pt[1].y = m_sys->getMouseY();
			if (b & SDL_BUTTON_LMASK) termch = 0x20;
			else if (b & SDL_BUTTON_MMASK) termch = 0x22;
			else if (b & SDL_BUTTON_RMASK) termch = 0x21;
	/* If mouse has not moved, don't return a point */ 
			if (m_lastX == pt[1].x && m_lastY == pt[1].y)
			{
				putArray(intout, 0, termch);
				if (termch)
				{
					putArray(ctrl, nINTOUT, 1);
				}
				else
				{
					putArray(ctrl, nINTOUT, 0);
				}
				putArray(ctrl, nPTSOUT, 0);
			}
			else /* Return point but no button */
			{
				putArray(ctrl, nPTSOUT, 1);
				putArray(ptsout, 0, pt[1].x);
				putArray(ptsout, 1, m_height - pt[1].y);
			}
			m_lastX = pt[1].x;
			m_lastY = pt[1].y;
			break;
	}

}


void PcwGSX::readLocatorLibGSX(gword *contrl, gword *intin, gword *intout,
				gword *ptsin, gword *ptsout,
				gword vpleft, gword vptop, gword vpwidth,
				gword vpheight)
{

	long dev;
	GPoint pt[2]; 
	short termch;
	Uint8 b;
	int x, y, state;
	SDL_Event ev;

	pt[0].x = ptsin[0];
	pt[0].y = ptsin[1];
	dev = intin[0];
	termch = 0;
	switch(m_locatorMode)
	{
		case 1: // Modal
			state = SDL_ShowCursor(-1);
			SDL_ShowCursor(1);
			while (!termch)
			{
				if (!SDL_WaitEvent(&ev)) break;

				switch(ev.type)
				{
					case SDL_QUIT: goodbye(99);
					case SDL_MOUSEBUTTONDOWN:
						pt[1].x = ev.button.x;
						pt[1].y = ev.button.y;
						switch(ev.button.button)
						{
							case SDL_BUTTON_LEFT:
							termch = 0x20;
							break;
							case SDL_BUTTON_MIDDLE:
							termch = 0x22;
							break;
							case SDL_BUTTON_RIGHT:
							termch = 0x21;
							break;
						}
						break;
				}
			}	
			SDL_ShowCursor(state);
			// Clip to viewport
			if (pt[1].x < vpleft) pt[1].x = vpleft;
			if (pt[1].y < vptop)  pt[1].y = vptop;
			if (pt[1].x >= vpleft + vpwidth) pt[1].x = vpleft + vpwidth - 1;
			if (pt[1].y >= vptop + vpheight) pt[1].y = vptop + vpheight - 1;
			contrl[2] = 1;
			contrl[4] = 1;
			intout[0] = termch;
			ptsout[0] = pt[1].x - vpleft;
			ptsout[1] = (vpheight - 1 - (pt[1].y - vptop)/2);
			break;
		case 2: // Modeless. Return the current state of the mouse
			// as recorded by the main JOYCE event loop.
			b = m_sys->getMouseBtn();
			pt[1].x = m_sys->getMouseX();
			pt[1].y = m_sys->getMouseY();
			if (b & SDL_BUTTON_LMASK) termch = 0x20;
			else if (b & SDL_BUTTON_MMASK) termch = 0x22;
			else if (b & SDL_BUTTON_RMASK) termch = 0x21;
	/* If mouse has not moved, don't return a point */ 
			if (m_lastX == pt[1].x && m_lastY == pt[1].y)
			{
				intout[0] = termch;
				if (termch)
				{
					contrl[4] = 1;
				}
				else
				{
					contrl[4] = 0;
				}
				contrl[2] = 0;
			}
			else /* Return point but no button */
			{
				contrl[2] = 1;
				contrl[4] = 0;
				if (pt[1].x < vpleft) pt[1].x = vpleft;
				if (pt[1].y < vptop)  pt[1].y = vptop;
				if (pt[1].x >= vpleft + vpwidth) pt[1].x = vpleft + vpwidth - 1;
				if (pt[1].y >= vptop + vpheight) pt[1].y = vptop + vpheight - 1;
				ptsout[0] = pt[1].x - vpleft;
				ptsout[1] = (vpheight - 1 - (pt[1].y - vptop))/2;
			}
			m_lastX = pt[1].x;
			m_lastY = pt[1].y;
			break;
	}

}





void PcwGSX::setInputMode()
{
	short devtype = getArray(intin, 0);
	short mode = getArray(intin, 1);

	if (mode != 2) mode = 1;
	switch(devtype)
	{
		case 1: m_locatorMode = mode; break;
	}
// Don't try to integrate SDSDL's event loop with our own; instead, 
// handle the locator (the mouse) here.
//
//	short mode = m_sdsdl->sinMode(getArray(intin,0), getArray(intin,1));
	putArray(intout, 0, mode);
	putArray(ctrl, nINTOUT, 1);
}


gword PcwGSX::setInputModeLibGSX(gword *intin)
{
	gword devtype = intin[0];
	gword mode = intin[1];

	if (mode != 2) mode = 1;
	switch(devtype)
	{
		case 1: m_locatorMode = mode; break;
	}
	return mode;
}




void PcwGSX::command(zword pb)
{
	int j;
	zword function;

	m_surface = m_sys->m_activeTerminal->getSysVideo()->getSurface();
	gsxpb  = pb;
	ctrl   = getWord(pb);
	intin  = getWord(pb + 2);
	ptsin  = getWord(pb + 4);
	intout = getWord(pb + 6);
	ptsout = getWord(pb + 8);	

	putArray(ctrl, nINTOUT, 0);
	putArray(ctrl, nPTSOUT, 0);

	function = getWord(ctrl);
	if (function != 1 && m_sdsdl == NULL) return;
/* Uncomment this to get a log of GSX calls made */
/* 
	FILE *fp = fopen("pcwgsx.log", "a");
	if (function == 5 || function == 11)
	{
		fprintf(fp, "fn: %d/%d  intin: ", function,
				getArray(ctrl, 5));
	}
	else
	{
		fprintf(fp, "fn: %d  intin: ", function);
	}
	for (j = 0; j < getArray(ctrl, nINTIN) + 1; j++) 
		fprintf(fp, "%d;", getArray(intin, j));
	fprintf(fp, "\n  ptsin: ");
	for (j = 0; j < 2*getArray(ctrl, nPTSIN); j++) 
		fprintf(fp, "%d;", getArray(ptsin, j));
	fprintf(fp, "\n");
	fflush(fp);
*/
	switch(function)
	{
		case 1: openWorkstation();  break;
		case 2: closeWorkstation(); break;
		case 3: clearPicture();	    break;
		case 4:	updateWorkstation();break;
		case 5: escape();	    break;
		case 6: drawPolyline();	    break;
		case 7: drawPolymarker();   break;
		case 8: graphicText();	    break;
		case 9: fillArea();	    break;
		case 10: outputIndexArray();break;
		case 11: gdp();		    break;
		case 12: setTextHeight();   break;
		case 13: setTextDirection();break;
		case 14: setColourIndex();  break;
		case 15: setLineStyle();    break;
		case 16: setLineWidth();    break;
		case 17: setLineColour();   break;
		case 18: setMarkerStyle();  break;
		case 19: setMarkerHeight(); break;
		case 20: setMarkerColour(); break;
		case 21: setTextStyle();    break;
		case 22: setTextColour();   break;
		case 23: setFillStyle();    break;
		case 24: setFillIndex();    break;
		case 25: setFillColour();   break;
		case 26: inquireColour();   break;
		case 27: inquireCellArray();break;
		case 28: readLocator();	    break;
		case 32: setWriteMode();    break;
		case 33: setInputMode();    break;
		default:		    break;		
	}
/* 
	fprintf(fp, "  intout: ");
	for (j = 0; j < getArray(ctrl, nINTOUT); j++) 
		fprintf(fp, "%d;", getArray(intout, j));
	fprintf(fp, "\n  ptsout: ");
	for (j = 0; j < 2*getArray(ctrl, nPTSOUT); j++) 
		fprintf(fp, "%d;", getArray(ptsout, j));
	fprintf(fp, "\n");
	fclose(fp);
*/
}

zword PcwGSX::getWord(zword addr)
{
	return m_sys->m_cpu->fetchW(addr);
}

zword PcwGSX::getArray(zword addr, zword idx)
{
	return m_sys->m_cpu->fetchW(addr + 2 * idx);
}

void PcwGSX::putArray(zword addr, zword idx, zword value)
{
	m_sys->m_cpu->storeW(addr + 2 * idx, value);
}

bool PcwGSX::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	return true;
}

bool PcwGSX::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	return true;
}

PcwTerminal& PcwGSX::operator<<(unsigned char c)
{
	return *this;
}


void PcwGSX::setForeground(zbyte r, zbyte g, zbyte b)
{
	if (m_term) m_term->setForeground(r,g,b);
}

void PcwGSX::setBackground(zbyte r, zbyte g, zbyte b)
{
	if (m_term) m_term->setBackground(r,g,b);
}


zbyte PcwGSX::gsxlib(zbyte device, zword gsxpb, zword viewport)
{
	int n;
	zbyte result = 0xff;

	gword nintin, nptsin;
	gword contrl[10];  zword caddr;
	gword intout[256]; zword ioaddr;
	gword ptsout[512]; zword poaddr;
	gword *intin;	   zword iiaddr;
	gword *ptsin;      zword piaddr;

	caddr  = getWord(gsxpb);
	iiaddr = getWord(gsxpb+2);
	piaddr = getWord(gsxpb+4);
	ioaddr = getWord(gsxpb+6);
	poaddr = getWord(gsxpb+8);
	for (n = 0; n < 10; n++)
		contrl[n] = getArray(caddr, n);

	check_contrl(contrl);
	nintin = contrl[3]; 
	nptsin = contrl[1];
	ptsin = new gword[2 * nptsin + 1];
	intin = new gword[nintin + 1];
	for (n = 0; n < 2 * nptsin; n++) ptsin[n] = getArray(piaddr, n);
	for (n = 0; n < nintin; n++)   intin[n] = getArray(iiaddr, n);

	putArray(caddr, 2, 0);	/* Default to returning no ints */
	putArray(caddr, 4, 0);	/* And no points */
	switch (device)
	{
		case 1: 	/* PCW screen */
			pcwscreen(contrl, intin, ptsin, intout, ptsout, viewport);
			result = 0;
			break;
		case 21:	/* PDF */
			pcwpdf(contrl, intin, ptsin, intout, ptsout);
			result = 0;
			break;
		case 22:	/* PNG */	
			pcwpng(contrl, intin, ptsin, intout, ptsout);
			result = 0;
			break;
		case 29:	/* PBM */	
			pcwpbm(contrl, intin, ptsin, intout, ptsout);
			result = 0;
			break;
		default:
			result = 0xFF;	/* Unknown device */
			break;
	}
/* Write back contrl first, in case it overlaps ptsout or intout */
	for (n = 0; n < 10; n++) putArray(caddr, n, contrl[n]);
	for (n = 0; n < contrl[2]*2; n++) putArray(poaddr, n, ptsout[n]);
	for (n = 0; n < contrl[4]; n++) putArray(ioaddr, n, intout[n]);
	delete [] intin;
	delete [] ptsin;	
	return result;
}			        


void PcwGSX::pcwscreen(gword *contrl, gword *intin, gword *ptsin, gword *intout, gword *ptsout, gword viewport)
{
	unsigned x, y;
	JoycePcwTerm *term = (JoycePcwTerm *)(m_sys->m_defaultTerminal);

//fprintf(stderr, "pcwscreen: contrl[0]=%d PC=%04x SP=%04x %04x %04x %04x %04x %04x\n",
//		contrl[0],
//		m_sys->m_cpu->pc,
//		m_sys->m_cpu->sp,
//		getWord(m_sys->m_cpu->sp),
//		getWord(m_sys->m_cpu->sp+2),
//		getWord(m_sys->m_cpu->sp+4),
//		getWord(m_sys->m_cpu->sp+6),
//		getWord(m_sys->m_cpu->sp+8));
//	if (contrl[0] == 1) m_sys->m_cpu->trace = 1;

	//fprintf(stderr, "pcwscreen: contrl[0]=%d contrl[5]=%d\n", contrl[0],
	//	contrl[5]);
	// Override some of the methods to indicate that we have a pointing
	// device
	if (contrl[0] == 5) switch (contrl[5])
	{
		case 16:	/* Check for tablet */	
			intout[0] = 1;
			contrl[4] = 1;
			return;
		case 18:	/* Draw graphics cursor */ 
			x = 40 + (term->m_vpcol * 8) + ptsin[0];
			y = 44 + ((term->m_vprow + term->m_vph) * 16) - 1 - 2 * ptsin[1];
			SDL_WarpMouse(x, y);
			SDL_ShowCursor(1);
			return;

		case 19:	/* Hide graphics cursor */
			SDL_ShowCursor(0);
			return;
	}
	if (contrl[0] == 28) /* Read locator */
	{
		readLocatorLibGSX(contrl, intin, intout, ptsin, ptsout,
			term->m_vpcol * 8 + 40,
			term->m_vprow * 16 + 44,
			term->m_vpw * 8,
			term->m_vph * 16);
		return;
	}
	if (contrl[0] == 33) /* Set input mode */
	{
		intout[0] = setInputModeLibGSX(intin);
		contrl[4] = 1;
		return;
	}

	if (contrl[0] == 1) /* Open workstation */
	{
//		gl_sys->m_cpu->trace = 2;
		if (m_screen != NULL)
		{
			(*m_screen->destroy)(&m_screen);
		}
		gios_raster_create(NULL, &m_screen, 
			(void *)&(term->m_rasterSurface));
		term->m_vprow = getWord(viewport) >> 8;
		term->m_vpcol = getWord(viewport) & 0xFF;
		term->m_vph   = (getWord(viewport+2) >> 8) + 1;
		term->m_vpw   = (getWord(viewport+2) & 0xFF) + 1;

/* Back up what was on the screen before handing control to libgsx */

		for (y = 8 * term->m_vprow; y < 8 * (term->m_vprow + term->m_vph); y++)
		{
			for (x = term->m_vpcol; x < term->m_vpcol + term->m_vpw; x++)
				m_txtScreen[y*90+x] = term->fetch_screen(y, x);	
		}
	}
	if (m_screen != NULL)
	{
		(*m_screen->dispatch)(m_screen, contrl, intin, ptsin,
					intout, ptsout);
	}
	if (contrl[0] == 1) /* Open workstation */
	{
		intout[40] = 1;	/* Locator available */
		intout[44] = 2;	/* Workstation supports input & output */
	}	
	if (contrl[0] == 2)	/* Close workstation */
	{
/* Restore what was on the screen when control was handed to libgsx */
		for (y = 8 * term->m_vprow; y < 8 * (term->m_vprow + term->m_vph); y++)
		{
			for (x = term->m_vpcol; x < term->m_vpcol + term->m_vpw; x++)
				term->store_screen(y, x, m_txtScreen[y*90+x]);
		}
		if (m_screen != NULL)
		{
			(*m_screen->destroy)(&m_screen);
		}
	}
}



void PcwGSX::pcwpdf(gword *contrl, gword *intin, gword *ptsin, gword *intout, gword *ptsout)
{
	if (contrl[0] == 1) /* Open workstation */
	{
		if (m_pdf != NULL)
		{
			(*m_pdf->destroy)(&m_pdf);
		}
		gios_pdf_create(NULL, &m_pdf, (void *)&m_pdfParam);
	}
	if (m_pdf != NULL)
	{
		(*m_pdf->dispatch)(m_pdf, contrl, intin, ptsin,
					intout, ptsout);
	}
	if (contrl[0] == 2)	/* Close workstation */
	{
		(*m_pdf->destroy)(&m_pdf);
	}
}


void PcwGSX::pcwpng(gword *contrl, gword *intin, gword *ptsin, gword *intout, gword *ptsout)
{
	if (m_pngsurface == NULL)
	{
		rdev_png_create(&m_pngsurface, 
					m_pngDir.c_str(), 800, 600);
	}

	if (contrl[0] == 1) /* Open workstation */
	{
		if (m_png != NULL)
		{
			(*m_png->destroy)(&m_png);
		}
		gios_raster_create(NULL, &m_png, (void *)m_pngsurface);
	}
	if (m_png != NULL)
	{
		(*m_png->dispatch)(m_png, contrl, intin, ptsin,
					intout, ptsout);
	}
	if (contrl[0] == 2)	/* Close workstation */
	{
		(*m_png->destroy)(&m_png);
	}

}


void PcwGSX::pcwpbm(gword *contrl, gword *intin, gword *ptsin, gword *intout, gword *ptsout)
{
	if (m_pbmsurface == NULL)
	{
		rdev_pbm_create(&m_pbmsurface, 
					m_pngDir.c_str(), 800, 600, 0);
	}

	if (contrl[0] == 1) /* Open workstation */
	{
		if (m_pbm != NULL)
		{
			(*m_pbm->destroy)(&m_pbm);
		}
		gios_raster_create(NULL, &m_pbm, (void *)m_pbmsurface);
	}
	if (m_pbm != NULL)
	{
		(*m_pbm->dispatch)(m_pbm, contrl, intin, ptsin,
					intout, ptsout);
	}
	if (contrl[0] == 2)	/* Close workstation */
	{
		(*m_pbm->destroy)(&m_pbm);
	}

}


