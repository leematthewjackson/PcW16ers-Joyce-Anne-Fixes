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

/* The state of the terminal emulator. All these point to the actual 
 * data, usually held within PCW RAM */
struct CRTVars
{
	zbyte *top;		/* Top row of output window */
	zbyte *left;		/* Left column of output window */
	zbyte *reqwidth;	/* Requested screen width */
	zbyte *reqheight;	/* Requested screen height */
	zbyte *width;		/* Width of output window */
	zbyte *height;		/* Height of output window */
	zbyte *curx;		/* Cursor X position within window */
	zbyte *cury;		/* Cursor Y position within window */
	zbyte *language;	/* Current language variant, 0-7 */
	zbyte *attrs;		/* Character attributes */
	zbyte *wrap;		/* Wrap at EOL? */
	zbyte *pagemode;	/* Vertically wrap rather than scroll? */
	zbyte *showcur;		/* Cursor enabled? */
	zbyte *curshown;	/* Cursor currently visible? */
	zbyte *curevent;	/* Cursor event block */
	zbyte *palette;		/* Palette 'registers' */
	zbyte *ric_xpos;	/* X position of status line message */
	zbyte *adrive;		/* Drive for 'Drive is A:' message */
	zbyte *stl_on;		/* Is status line shown? */
	zbyte *ul_mask;		/* Underline mask */
	zbyte *is_24x80;	/* In 24x80 mode? */
	zbyte defaults[30];

	int and_mask, or_mask, xor_mask;
	int inited;

	inline CRTVars() { reset(); }

	void reset();
};




class JoyceFastCRT : public PcwFastCRT
{
public:
	JoyceFastCRT(JoyceSystem *s);
	virtual ~JoyceFastCRT();

	virtual void reset(void);
	void tick(void);

        virtual bool parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
        virtual bool storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);

	virtual string getTitle(void);
	virtual bool hasSettings(SDLKey *key, string &caption);
	UiEvent settings(UiDrawer *d);
	UiEvent serialSettings(UiDrawer *d);

	virtual bool isInitialised();
	void setVars(unsigned scr_blinkcount, unsigned te_attrs, 
			unsigned te_showcursor, unsigned msg_ric_xpos);
	void hideCursor();
	void timedShowCursor();
	void drawCursor();

/* Terminal ops */

	virtual void drawChar(unsigned char ch, int message);
	virtual void cr();
	virtual void lf(bool scale = true);
	void reverseLF();
	virtual void bel();
	virtual void cursorUp(bool wrap, bool scale = true);
	virtual void cursorDown(bool wrap, bool scale = true);
	virtual void cursorLeft(bool wrap, bool scale = true);
	virtual void cursorRight(bool wrap, bool scale = true);
	virtual void moveCursor(zbyte x, zbyte y, bool scale = true);
	virtual void saveCursorPos();
	virtual void loadCursorPos();

	void clearRect(zbyte left, zbyte top, zbyte width, zbyte height);
	virtual void clearWindow();
	virtual void clearBOS();
	virtual void clearBOL();
	virtual void clearEOS();
	virtual void clearEOL();
	virtual void clearCurLine();
	virtual void insertLine();
	virtual void deleteLine();
	virtual void deleteChar();

	virtual bool isDim();
	virtual void setDim(bool b = true);	
	virtual bool isBold();
	virtual void setBold(bool b = true);	
	virtual bool isDoubleWidth();
	virtual void setDoubleWidth(bool b = true);	
	virtual bool isDoubleHeight();
	virtual void setDoubleHeight(bool b = true);	
	virtual bool isItalic();
	virtual void setItalic(bool b = true);	
	virtual bool isReverse();
	virtual void setReverse(bool b = true);	
	virtual bool isUnderline();
	virtual void setUnderline(bool b = true, zbyte mask = 0xFF);	
	virtual void setAndMask(int mask);
	virtual void setOrMask(int mask);
	virtual void setXorMask(int mask);
	virtual bool isWrap();
	virtual void setWrap(bool b = true);	
	virtual bool isPageMode();
	virtual void setPageMode(bool b = true);
	virtual bool isStatusEnabled();
	virtual void enableStatus(bool b = true);
	virtual bool isCursorEnabled();
	virtual void enableCursor(bool b = true);
	virtual void setLanguage(int lang);	
	virtual void setMode(unsigned char mode);
	virtual void set24x80(bool b = true);	

	virtual void   setForeground(Uint8 r, Uint8 g, Uint8 b);
	virtual void   setBackground(Uint8 r, Uint8 g, Uint8 b);
	virtual void   setWindow(zbyte x, zbyte y, zbyte w, zbyte h);

	virtual void	writeSys(unsigned char c);
protected:
	void recalcBounds();
	void swapLanguage(zbyte language);
	void drawGlyph(zword *buf, bool shift);
public:
	CRTVars m_vars;
	bool	m_colour;
	bool	loadWav();
	SDL_AudioSpec m_beepSpec;
	Uint8 *m_beepBuffer;
	Uint32 m_beepLen;
};




