/************************************************************************

    JOYCE v2.5.5 - Amstrad PCW emulator

    Copyright (C) 1996, 2001, 2026  John Elliott <seasip.webmaster@gmail.com>

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

/* The printer positions pixels vertically using 360dpi coordinates. 
 * Horizontally, it uses 720dpi. Storing the generated page at 360dpi is 
 * bad enough (it occupies about 14Mb!). Storing the generated page at 720dpi
 * is not to be thought of.
 *
 * As a compromise, store at 360dpi, and make the coordinates floats. In
 * some modes, columns will be off by half a pixel.
 *
 */
#include "GdiPrint.hxx"

#define MAX_W 3240
#define MAX_H 4440

#define FAST_STEP	1.0	// Motor running at draft speed.
#define SLOW_STEP	0.5	// Motor running at condensed speed. 

#define MD_NORMAL    0	// Waiting for command
#define MD_LINEFEED  1	// Doing linefeed
#define MD_PRINTING  2  // Printing

// Input from port 0xFD
#define MATRIX_BAIL      0x80	/* Bail bar in */
#define MATRIX_READY     0x40	/* Printer controller ready */
#define MATRIX_NOPRINTER 0x20	/* Printer not connected (9512?) */
#define MATRIX_LCOL      0x10	/* Not at left column */
#define MATRIX_FEEDER	 0x08	/* Sheet feeder present */
#define MATRIX_PAPER	 0x04	/* Paper present */
#define MATRIX_BUSY	 0x02	/* Busy */
#define MATRIX_FAILED	 0x01	/* Printer or controller failed */

#define PIN_SPACING	 4.6875	// 8 pins cover 37/360 in. 

typedef enum {PS, PNG, PDF
#ifdef HAVE_WINDOWS_H 
			, GDI
#endif
} PrinterFormat;

class JoyceMatrix : public PcwPrinter
{
protected:
	bool	m_noRam;	// Not enough RAM to operate
	bool	m_drawing_page;	// Do we have a page open?
	int	m_sequence;	// Page number being output
	char	m_outFilename[PATH_MAX];
	FILE	*m_fp;		// File being output
	png_bytep m_row_pointers[MAX_H];
	png_structp m_png_pstruct;
	png_infop   m_png_pinfo;

	float m_x, m_y;		// Coordinates of print head wrt paper
	float m_xstep;		// 1.0 if motor running at normal speed;
				// 0.5 for slow.
	float m_xdir;		// Direction of print head: 1 for right,
				// -1 for left.
	int   m_w, m_h;		// Width & height of paper, 360ths of an inch
	zbyte m_command[2];	// Command buffer 
	zbyte m_cmdptr;		// Is current byte first or second in command? 
	int  m_mode;		// In the middle of a command? 
	string m_psdir;		// Where does Postscript get saved to? 
	string m_pngdir;	// Where do the PNGs get saved to? 
	string m_pdfdir;	// Where does PDF get saved to? 
	bool m_bailIn;		// Bail bar in?
	PrinterFormat m_format;

#ifdef HAVE_WINDOWS_H
	GdiPrint m_gdiPrint;
#endif

public:
	JoyceMatrix();
	~JoyceMatrix();
	inline float getX() { return m_x; }
	void reset(void);
	void out(zword port, zbyte value);
	zbyte in(zword port);
        virtual bool hasSettings(SDLKey *key, string &caption);
        virtual UiEvent settings(UiDrawer *d);
        virtual UiEvent control(UiDrawer *d);

protected:
	virtual bool parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	virtual bool storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	virtual string getTitle(void);
	UiEvent getCustomPaper(UiDrawer *d);

	void obeyCommand(zword port);
	void checkEndForm(void);
	void dots(void);	// Simulate firing the 9 pins
////////////////////////////////////////////////////////////////////////////
//
// Output page as a PNG file; we use libpng. Override these three 
// functions to send the page somewhere else, eg to a real printer.
//
	bool startPagePNG(void);
	bool endPagePNG(void);
	void dotPNG(float xf, float yf);

////////////////////////////////////////////////////////////////////////////
//
// Output page as a Postscript file.
//
	bool startPagePS(void);
	bool endPagePS(void);
	void dotPS(float xf, float yf);
	void closePS(void);

////////////////////////////////////////////////////////////////////////////
//
// Output page as a PDF file.
//
	HPDF_Doc m_pdfDoc;
	HPDF_Page m_pdfPage;

	bool startPagePDF(void);
	bool endPagePDF(void);
	void dotPDF(float xf, float yf);
	void closePDF(void);
//
// This isn't at all OO, but I did want to keep everything in one class.
//
	bool startPage(void);
	bool endPage(void);
	// Dot: Simulate firing a pin (6x6 circle)
	void dot(float xf, float yf);
private:	
	// Plot a single pixel
	void ditPNG(int x, int y);	

};


