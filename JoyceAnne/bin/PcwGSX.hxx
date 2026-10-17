/************************************************************************

    JOYCE v2.1.7 - Amstrad PCW emulator

    Copyright (C) 1996, 2005  John Elliott <seasip.webmaster@gmail.com>

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

class SdSdl;
struct gpoint;

class PcwGSX : public PcwSdlTerm
{
public:
	PcwGSX(PcwSystem *s);
	virtual ~PcwGSX();

	void command(zword rde);
	zbyte gsxlib(zbyte rb, zword rde, zword rhl);

protected:
        virtual bool parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
        virtual bool storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	virtual PcwTerminal& operator<<(unsigned char c);
        virtual void setForeground(zbyte r, zbyte g, zbyte b);
        virtual void setBackground(zbyte r, zbyte g, zbyte b);
	virtual void getColour(Uint32 *col, zword array, zword off);
	virtual void setColour(Uint32 *col);

	// GSXlib functions
	void pcwscreen(gword *contrl, gword *intin, gword *ptsin,
			gword *intout, gword *ptsout, gword viewport);
	void pcwpdf(gword *contrl, gword *intin, gword *ptsin,
			gword *intout, gword *ptsout);
	void pcwpng(gword *contrl, gword *intin, gword *ptsin,
			gword *intout, gword *ptsout);
	void pcwpbm(gword *contrl, gword *intin, gword *ptsin,
			gword *intout, gword *ptsout);

	zword getWord(zword addr);
	zword getArray(zword addr, zword idx);
	void putArray(zword addr, zword idx, zword value);
	struct gpoint *getPtsin();

	// GSX functions
	void openWorkstation();
	void closeWorkstation();
	void clearPicture();
	void updateWorkstation();
	void drawPolyline();
	void drawPolymarker();
	void graphicText();
	void fillArea();
	void outputIndexArray();
	void gdp();
	void setColourIndex();
	void setLineStyle();
	void setMarkerStyle();
	void setFillStyle();
	void setFillIndex();
	void setLineColour();
	void setFillColour();
	void setTextColour();
	void setMarkerColour();
	void setTextHeight();
	void setTextDirection();
	void setMarkerHeight();
	void setTextStyle();	
	void setLineWidth();
	void setWriteMode();
	void setInputMode();
	void inquireColour();
	void inquireCellArray();
	void readLocator();
	void readValuator();
	void readChoice();
	void readString();
	void escape();
	void readLocatorLibGSX(gword *ctrl, gword *intin, gword *intout,
				gword *ptsin, gword *ptsout, 
				gword vpleft, gword vptop, gword vpwidth, 
				gword vpheight);
	gword setInputModeLibGSX(gword *intin);
private:
	zword gsxpb, ctrl, intin, intout, ptsin, ptsout;
	int oldtype;
	zword m_height;
	zword m_mintxtheight;
	zword m_txtheight;
	zword m_locatorMode;
	int m_lastX, m_lastY;

	// GSXLib wrappers
	GIOSDEVICE  *m_screen;
	GIOSDEVICE  *m_pdf;
	GIOSDEVICE  *m_png;
	GIOSDEVICE  *m_pbm;

	RASTERSURFACE *m_pngsurface;
	RASTERSURFACE *m_pbmsurface;
	PDFPARAM m_pdfParam;
	string m_pngDir;
	string m_pdfDir;
	zbyte m_txtScreen[23040];

	PcwTerminal *m_oldTerm;
	SDL_Surface *m_surface;
	SdSdl *m_sdsdl;
	PcwSystem *m_sys;
};
