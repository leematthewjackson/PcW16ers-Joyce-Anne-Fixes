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

#include "Pcw.hxx"


PcwFastCRT::PcwFastCRT(PcwSystem *s) : PcwDevice("fastcrt")
{
	m_esc = 0;
	m_escCount = 0;
}

PcwFastCRT::~PcwFastCRT()
{

}




void PcwFastCRT::write(unsigned char c)
{
	if (!m_esc) switch(c)
	{
		case 27: m_esc = 27; return;
		case 13: cr();     return;
		case 10: lf();     return;
		case  8: cursorLeft(true); return;
		case  7: bel();	   return;
		
		default: if (c < 32) return;
			 drawChar(c, 0); 
			 return;
	}
	// m_esc = 27: First byte of an ESC code
	if (m_esc == 27) switch(c)
	{
		default: 	drawChar(c,0); m_esc = 0; return;

		case '0': enableStatus(false); m_esc = 0; return;
		case '1': enableStatus();      m_esc = 0; return;
		case '2': escMulti(c, 2);                 return;
                case '3': escMulti(c, 2);                 return;
                case '4': escMulti(c, 2);                 return;
                case '5': escMulti(c, 2);                 return;
		case '+': escMulti(c, 2);		  return;
		case '-': escMulti(c, 2);		  return;

		case 'A': cursorUp(false);    m_esc = 0; return;
		case 'B': cursorDown(false);  m_esc = 0; return;
		case 'C': cursorRight(false); m_esc = 0; return;
		case 'D': cursorLeft(false);  m_esc = 0; return;
		case 'E': clearWindow();      m_esc = 0; return;
		case 'H': home();             m_esc = 0; return;
		case 'I': cursorUp(true);     m_esc = 0; return;
		case 'J': clearEOS();         m_esc = 0; return;
		case 'K': clearEOL();         m_esc = 0; return;
		case 'L': insertLine();       m_esc = 0; return;
		case 'M': deleteLine();       m_esc = 0; return;
		case 'N': deleteChar();       m_esc = 0; return;
		case 'X': escMulti(c, 5);                return;
		case 'Y': escMulti(c, 3);                return;
		case 'b': escMulti(c, 2);                return;
		case 'c': escMulti(c, 2);                return;
		case 'd': clearBOS();        m_esc = 0;  return;
		case 'e': enableCursor();    m_esc = 0;  return;
		case 'f': enableCursor(false);m_esc = 0; return;
		case 'j': saveCursorPos();   m_esc = 0;  return;
		case 'k': loadCursorPos();   m_esc = 0;  return;
		case 'l': clearCurLine();    m_esc = 0;  return;
		case 'o': clearBOL();        m_esc = 0;  return;
		// Special effects
		case 'p': setReverse();      m_esc = 0; return;
		case 'q': setReverse(false); m_esc = 0; return;
                case 'r': setUnderline();    m_esc = 0; return;
                case 's': setBold();	     m_esc = 0; return;
                case 't': setBold(false);    m_esc = 0; return;
                case 'u': setUnderline(0);   m_esc = 0; return;
		case 'v': setWrap();	     m_esc = 0; return;
		case 'w': setWrap(false);    m_esc = 0; return;
		case 'x': set24x80(true);    m_esc = 0; return;
		case 'y': set24x80(false);   m_esc = 0; return;
		case 'z': set24x80(false);   m_esc = 0; return;
	}
	else
	{
		m_escBuf[m_esc++] = c;
		--m_escCount;
/* These CRT+ escape codes take an extra parameter */
		if (m_esc == 2 && m_escBuf[0] == '+' && 
			(m_escBuf[1] == 'u' || m_escBuf[1] == 'a' || 
			 m_escBuf[1] == 'o' || m_escBuf[1] == 'x'))
		{
			++m_escCount;
		}
/* ESC + Y takes 2 extra parameters */
		if (m_esc == 2 && m_escBuf[0] == '+' && m_escBuf[1] == 'Y')
		{
			m_escCount += 2;
		}
/* ESC + X takes 4 extra parameters */
		if (m_esc == 2 && m_escBuf[0] == '+' && m_escBuf[1] == 'X')
		{
			m_escCount += 4;
		}

		if (!m_escCount) 
		{
			int r,g,b;

			m_esc = 0;
			switch(m_escBuf[0])
			{
				case '+': switch (m_escBuf[1])
				{
					case 'a': setAndMask(m_escBuf[2]); break;
					case 'b': setBold(true); break;
					case 'd': setDoubleWidth(true); break;
					case 'g': setDim(true); break;
					case 'h': setDoubleHeight(true); break;
					case 'i': setItalic(true); break;
					case 'o': setOrMask(m_escBuf[2]); break;
					case 'u': setUnderline(true, m_escBuf[2]); break;
					case 'x': setXorMask(m_escBuf[2]); break;
					case 'Y': moveCursor(m_escBuf[3] - 0x20, m_escBuf[2] - 0x20, false); break;
				}
				break;
				case '-': switch (m_escBuf[1])
				{
					case 'a': setAndMask(-1); break;
					case 'b': setBold(false); break;
					case 'd': setDoubleWidth(false); break;
					case 'g': setDim(false); break;
					case 'h': setDoubleHeight(false); break;
					case 'i': setItalic(false); break;
					case 'o': setOrMask(-1); break;
					case 'u': setUnderline(false); break;
					case 'x': setXorMask(-1); break;

				}
				break;
					
				case '2': setLanguage(m_escBuf[1] & 7); break;
				case '3': setMode    (m_escBuf[1] & 3); break;
				case '4': setPageMode(m_escBuf[1] & 1); break;
				case 'X': setWindow (m_escBuf[2] - 0x20,
						     m_escBuf[1] - 0x20,
                                                     m_escBuf[4] - 0x1F,
						     m_escBuf[3] - 0x1F); break;
				case 'Y': moveCursor(m_escBuf[2] - 0x20,
						     m_escBuf[1] - 0x20); break;
				case 'b':
				case 'c':
					b = ( (m_escBuf[1] - 0x20)       & 3) * 85;
					g = (((m_escBuf[1] - 0x20) >> 2) & 3) * 85;
					r = (((m_escBuf[1] - 0x20) >> 4) & 3) * 85;
					if (m_escBuf[0] == 'b') setForeground(r,g,b);
					else			setBackground(r,g,b);
					break;				
	
			}
		}
	}	
}

//
// Prepare m_escBuf to store a multi-byte escape code
// m_escCount holds no. of bytes remaining
// m_esc      holds no. of bytes in buffer so far
// m_escBuf   holds data, eg "Y<foo><bar>"
//
void PcwFastCRT::escMulti(unsigned char first, int count)
{
	m_escCount = count - 1;
	m_esc = 1;
	m_escBuf[0] = first;	
}




