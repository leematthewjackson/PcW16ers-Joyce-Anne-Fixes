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

class PcwFastCRT : public PcwDevice
{
public:
	PcwFastCRT(PcwSystem *s);
	virtual ~PcwFastCRT();

	virtual bool isInitialised() = 0;
	virtual void setVars(unsigned scr_blinkcount, unsigned te_attrs,
				unsigned te_showcursor, unsigned msg_ric_xpos) = 0;

/* Terminal ops, similar to those in SdlTerm */
	virtual void drawChar(unsigned char c, int message) = 0;
	virtual void cr() = 0;
	virtual void lf(bool scale = true) = 0;
	virtual void bel() = 0;
	virtual void cursorUp(bool wrap, bool scale = true) = 0;
	virtual void cursorDown(bool wrap, bool scale = true) = 0;
	virtual void cursorLeft(bool wrap, bool scale = true) = 0;
	virtual void cursorRight(bool wrap, bool scale = true) = 0;
	virtual void moveCursor(zbyte x, zbyte y, bool scale = true) = 0;
	virtual void saveCursorPos() = 0;
	virtual void loadCursorPos() = 0;
	inline void home() { moveCursor(0, 0); }

	virtual void clearWindow() = 0;
	virtual void clearBOS() = 0;
	virtual void clearBOL() = 0;
	virtual void clearEOS() = 0;
	virtual void clearEOL() = 0;
	virtual void clearCurLine() = 0;
	virtual void insertLine() = 0;
	virtual void deleteLine() = 0;
	virtual void deleteChar() = 0;

	virtual bool isBold() = 0;
	virtual void setBold(bool b = true) = 0;	
	virtual bool isItalic() = 0;
	virtual void setItalic(bool b = true) = 0;	
	virtual bool isDim() = 0;
	virtual void setDim(bool b = true) = 0;	
	virtual bool isDoubleWidth() = 0;
	virtual void setDoubleWidth(bool b = true) = 0;	
	virtual bool isDoubleHeight() = 0;
	virtual void setDoubleHeight(bool b = true) = 0;	
	virtual bool isReverse() = 0;
	virtual void setReverse(bool b = true) = 0;	
	virtual bool isUnderline() = 0;
	virtual void setUnderline(bool b = true, zbyte mask = 0xFF) = 0;	
	virtual void setAndMask(int mask) = 0;
	virtual void setOrMask(int mask) = 0;
	virtual void setXorMask(int mask) = 0;

	virtual bool isPageMode() = 0;
	virtual void setPageMode(bool b = true) = 0;
	virtual bool isStatusEnabled() = 0;
	virtual void enableStatus(bool b = true) = 0;
	virtual bool isCursorEnabled() = 0;
	virtual void enableCursor(bool b = true) = 0;
	virtual bool isWrap() = 0;
	virtual void setWrap(bool b = true) = 0;
	virtual void setLanguage(int lang) = 0;

	virtual void   setForeground(Uint8 r, Uint8 g, Uint8 b) = 0;
	virtual void   setBackground(Uint8 r, Uint8 g, Uint8 b) = 0;
	virtual void   setWindow(zbyte x, zbyte y, zbyte w, zbyte h) = 0;
	virtual void   setMode(unsigned char mode) = 0;
	virtual void   set24x80(bool b = true) = 0;

/* VT52 escape code parser */
	void write(unsigned char c);
	virtual void writeSys(unsigned char c) = 0;

	char m_esc;
	int  m_escCount;
	unsigned char m_escBuf[10];
	void escMulti(unsigned char first, int count);
};




