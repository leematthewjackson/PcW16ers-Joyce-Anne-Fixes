/************************************************************************

    JOYCE v2.2.5a - Amstrad PCW emulator

    Copyright (C) 1996, 2001, 2012  John Elliott <seasip.webmaster@gmail.com>

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
#include "PcwFileList.hxx"
//#include "JoyceDirChooser.hxx"

/* Define png_jmpbuf() in case we are using an early version of libpng */
#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr) png_ptr->jmpbuf
#endif


/* Here is where I try to work out how the sequence of commands sent to the
 * matrix printer matches up with the output.
 *
 * For ease of calculation all measurements will be in 'slow' ticks, 1/1440 in.
 *
 * If I print, bidirectionally, a line of 36 characters followed by a 
 * line of 53 characters, then the printer sends:
 *        A9 4C, 36*12+9 column draws, 80 02, C0 00  - first line
 *        AC 3D                                      - linefeed
 *        A9 2D, 83 00, C0 00			     - move right 
 *	  A8 4C, 53*12+9 column draws, 80 02, C0 00  - second line
 *
 * First thing to note is that the nine extra blank columns are always drawn
 * at the end of the sequence. That is, if you plotted out the blocks one 
 * above the other, you'd get:
 * 
 *           This line is drawn left to right<9 cols>
 *           <9 cols>This line is drawn right to left
 *
 * They ought to appear aligned. So either the second line ends up being 
 * drawn 9 columns further left, or the last 9 columns of a printed block 
 * aren't drawn.
 * 
 * The next thing to note is that the total widths of the two columns, on a 
 * simplistic approach, aren't the same. 
 *
 * First line of text:			Second line of text:
 *       152 at left edge		 152 at right edge	
 *      5292 from column drawing	7740 from column drawing	
 *         4 from spacing		   4 from spacing
 *                                      7896 total
 *        90 at left edge
 *      2048 from spacing
 *      7586 total
 *
 * That is, the first line is 310 narrower than the second. 
 * Assuming the dimensions in the commands are the same in both directions,  
 * the main difference is that there are two 'start block' and 'end block' 
 * commands on the first line, and one on the second. If we assume 
 * that between them 'start block' and 'end block' add a constant 310 to the 
 * amount moved, both lines then become the same width, 8206.
 *
 * If I just add the constant to the width in 'start block', though, the 
 * extra 9 columns cause alternate lines to misalign. In order to get 
 * bidirectional printing aligned, I have to move some of the packing to 
 * 'end block'. The split was determined by trial and error.
 * 
 * Now let's consider NLQ.
 * 
 * If I draw a line of text in NLQ, it is drawn at the slow speed and then
 * the carriage is returned at fast speed:
 * 
 *	AB 86, 80*24+21 column draws, 80 04, C0 00
 *	A8 05, 96 00, C0 00
 *
 * So, if these are supposed to be the same width: 
 * Line of text:			Return carriage:
 *       134 at left edge		10 at right edge
 *     11646 from column drawing     11776 from spacing
 *         4 from spacing
 *     11784 total                   11786 total
 * 
 *       134 at left edge		10 at right edge
 *     11646 from column drawing     11776 from spacing
 *         4 from spacing
 *     11784 total                   11786 total
 *
 * Those are not quite the same, so it looks as if the start+end block offset
 * needs to be increased by 2 at NLQ speed.
 *        
 */
#define BEGIN_ADJUST 134
#define END_ADJUST 176		/* These should sum to 310 */

JoyceMatrix::JoyceMatrix() : PcwPrinter("matrix")
{
	XLOG("JoyceMatrix::JoyceMatrix()");
	m_noRam = false;
	m_drawing_page = false;
	m_sequence = 0;
	m_outFilename[0] = 0;
	m_format = PNG;
	m_fp = NULL;
	m_w = 2997;	// A4 width
	m_h = 4208;	// A4 height
	m_y = 360;	// Start 1 inch down
	m_pngdir = findPcwDir(FT_PNGS, false);
	m_psdir = findPcwDir(FT_PS, false);
	m_pdfdir = findPcwDir(FT_PDF, false);
	m_bailIn = true;
	for (int r = 0; r < MAX_H; r++)
	{
		m_row_pointers[r] = (png_bytep)malloc(MAX_W);
		if (!(m_row_pointers[r])) m_noRam = true;
		else memset(m_row_pointers[r], 0xFF, MAX_W);	
	}
	m_pdfDoc = NULL;
	m_pdfPage = NULL;
	reset();
}


JoyceMatrix::~JoyceMatrix()
{
	XLOG("JoyceMatrix::~JoyceMatrix()");
	endPage();
	if (m_fp) closePS();
	if (m_pdfDoc)
	{
		closePDF();
	}
	for (int r = 0; r < MAX_H; r++)
	{
		if (m_row_pointers[r]) free(m_row_pointers[r]);
	}
}


void JoyceMatrix::reset(void)
{
	m_cmdptr = 0;
	m_x = 0;
/* 	m_y = 0; no. This only happens at a page throw. */
	m_xstep = FAST_STEP;
	m_xdir  = 1;
	m_mode = MD_NORMAL;
}

void JoyceMatrix::out(zword port, zbyte value)
{
	/* Note: There seems to be no difference between accessing the
	 * ports 0xFC and 0xFD - the commands seem to stay the same. */
	m_command[m_cmdptr++] = value;
	if (m_cmdptr == 2) 
	{
		m_cmdptr = 0;
		obeyCommand(port);
	}
}


zbyte JoyceMatrix::in(zword port)
{	
	zbyte v;
	
	port &= 0xFF;
	if (!isEnabled()) return MATRIX_NOPRINTER;

	if (m_noRam)
	{
		if (port == 0xFC) return 0x01;
		return MATRIX_FAILED | MATRIX_BAIL;	/* Controller fails */
	}
	if (port == 0xFC) return 0xF8;	/* Standard response */

	// Should we deal with "out of paper" case?	
	v = MATRIX_READY | MATRIX_PAPER | MATRIX_FEEDER;

	// Bail bar in/out? 
	if (m_bailIn) v |= MATRIX_BAIL;
	// The "Left column" indicator is active low. If the head is 
	// at the left edge, it's 0. The initialisation sequence moves
	// it away from there.
	if (m_x > 0) v |= MATRIX_LCOL;
	return v;	
}


bool JoyceMatrix::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	char *t;
	parseEnabled(doc, ns, cur);
	// for back compatibility: "outputdir" node becomes "pngoutputdir"
	parseFilename(doc, ns, cur, "outputdir", m_pngdir);
	parseFilename(doc, ns, cur, "pngoutputdir", m_pngdir);
	parseFilename(doc, ns, cur, "psoutputdir", m_psdir);

        t = (char *)xmlGetProp(cur, (xmlChar *)"width");
        if (t) { m_w = atoi(t); xmlFree(t); }
        t = (char *)xmlGetProp(cur, (xmlChar *)"height");
        if (t) { m_h = atoi(t); xmlFree(t); }
        t = (char *)xmlGetProp(cur, (xmlChar *)"format");
        if (t) 
	{ 
		if (!strcmp(t, "PS"))  m_format = PS;
		if (!strcmp(t, "PNG")) m_format = PNG;
		if (!strcmp(t, "PDF")) m_format = PDF;
#ifdef HAVE_WINDOWS_H
		if (!strcmp(t, "GDI")) m_format = GDI;
#endif
		xmlFree(t);
	}

	return true;
}


bool JoyceMatrix::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
        xmlNodePtr outputdir;
        char buf[30];

        storeEnabled(doc, ns, cur);
        storeFilename(doc, ns, cur, "pngoutputdir", m_pngdir);
        storeFilename(doc, ns, cur, "psoutputdir", m_psdir);
        bool found = false;
	//
        // Remove any old "outputdir" nodes
        //
        for (outputdir = cur->xmlChildrenNode; outputdir; outputdir = outputdir->next)
        {
        	if (outputdir->ns != ns) continue;
                if (strcmp((char *)outputdir->name, "outputdir")) continue;
		xmlUnlinkNode(outputdir);
        } 
        sprintf(buf, "%d", m_w);
        xmlSetProp(cur, (xmlChar *)"width", (xmlChar *)buf);
        sprintf(buf, "%d", m_h);
        xmlSetProp(cur, (xmlChar *)"height", (xmlChar *)buf);
        switch(m_format)
	{
		case PNG:  xmlSetProp(cur, (xmlChar *)"format", (xmlChar *)"PNG"); break;
		case PDF:  xmlSetProp(cur, (xmlChar *)"format", (xmlChar *)"PDF"); break;
		case PS:   xmlSetProp(cur, (xmlChar *)"format", (xmlChar *)"PS");  break;
#ifdef HAVE_WINDOWS_H
		case GDI:  xmlSetProp(cur, (xmlChar *)"format", (xmlChar *)"GDI");  break;
#endif
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////
//
// Perform printing commands
//

void JoyceMatrix::obeyCommand(zword port)
{
	float headmove;
	
/*	printf("MATRIX %02x %02x  W6D3F=%04x W6D41=%04x m_x=%f m_xdir=%f m_xstep=%f\n", m_command[0], m_command[1], 
		PCWRAM[0x22D3F] + 256 * PCWRAM[0x22D40],
		PCWRAM[0x22D41] + 256 * PCWRAM[0x22D42],
		m_x, m_xdir, m_xstep); */
/////////////////////////////////////////
// In the middle of a linefeed command //
/////////////////////////////////////////
	if (m_mode == MD_LINEFEED) switch(m_command[0])
	{
		case 0xC0:	m_mode = MD_NORMAL;
				return;
		case 0x80:	m_y += 256;
				checkEndForm();
				return;
		default:
			joyce_dprintf("Unknown    lf-mode command: [%02x] %02x %02x\n", port, m_command[0], m_command[1]);
			return;
	}
//////////////////////////////////////
// In the middle of a print command //
//////////////////////////////////////
	if (m_mode == MD_PRINTING) 
	{
		float spacing;

		// Move further left or right. Distance is given in
		// units of 1/720 in, so we divide by 2 to get units
		// of 1/360 in. (in low-speed mode, units are 1/1440in).
		if (m_command[0] >= 0x80 && m_command[0] < 0xC0)
		{
			headmove = ((m_command[0] - 0x80) * 256);
			if (m_command[1]) headmove += m_command[1];
			else		  headmove += 256;

			// Headmove is in 720ths of an inch, ie half-pixels.
			m_x += m_xdir * (headmove * m_xstep / 2);
			return;
		}
		//
		// Drawing command.
		//
		if ((m_command[0] & 0xC0) == 0)
		{
			spacing = ((m_command[0] & 0x0E) / 2) + 5;
	
			dots();
			// Fast: Spacing is in 720ths of an inch, ie half-pixels.
			// Slow: Spacing is in 1440ths of an inch, ie half-pixels.
			m_x += (m_xdir * spacing * m_xstep / 2);
			return;
		}
		//
		// End of this line
		//
		else if (m_command[0] == 0xC0 && m_command[1] == 0)
		{
			m_mode = MD_NORMAL;
			m_x += (m_xdir * END_ADJUST / 4.0);
			if (m_xstep == SLOW_STEP) m_x += m_xdir;
			return;
		}
		joyce_dprintf("Unknown print-mode command: [%02x] %02x %02x\n", port, m_command[0], m_command[1]);
		return;
	}
//////////////////////////////////////
// Not in the middle of any command //
//////////////////////////////////////
	switch(m_command[0])
	{
		// Initialise
		case 0x00:	if ((port & 0xFF) == 0xFC) break;
				goto unknown;
		case 0xA1:	// Instant move right
			m_x += 6 * m_command[1];
			break;	
		case 0xA4:	// Instant feed
			m_y += m_command[1];
			break;
		case 0xA8:
		case 0xA9:
		case 0xAA:
		case 0xAB:
			m_mode = MD_PRINTING;
			float headmove;
			if (m_command[0] & 1) m_xdir = 1;
			else		      m_xdir = -1; 
			if (m_command[0] & 2) m_xstep = SLOW_STEP;
			else		      m_xstep = FAST_STEP;
			// Perform the initial head move (possibly this
			// is to let the head get up to speed).
			if (m_command[1]) headmove = m_command[1]; 
			else		  headmove = 256;
			m_x += m_xdir * (headmove * m_xstep / 2.0);
			m_x += m_xdir * BEGIN_ADJUST / 4.0;
			break;

		case 0xAC:
			m_y += m_command[1];
			m_mode = MD_LINEFEED;
			checkEndForm();
			break;	
		case 0xB8:
			if ((port & 0xFF) == 0xFC)
			{
				if (m_command[1] == 0) { reset(); break; }
			}
			// FALL THROUGH	
		unknown:
		default:
			joyce_dprintf("Unknown dot-matrix command: [%02x] %02x %02x\n", port, m_command[0], m_command[1]);
			break;
	}
}

void JoyceMatrix::checkEndForm(void)
{
	if (m_y >= MAX_H && m_drawing_page)
	{
		endPage();	
		m_y = 360;	/* Feed a new sheet; and we can't print 
				 * on the top 6 lines */
	}
}

void JoyceMatrix::dots(void)
{
	int mask = 0x01;
	float y = m_y;

	for (int n = 0; n < 8; n++)
	{
		if (m_command[1] & mask) dot(m_x, y); 
		mask = mask << 1;
		y += PIN_SPACING;	
	}
	if (m_command[0] & 1) dot(m_x, y);
}

static void matrix_png_error(png_structp png_ptr, png_const_charp error_msg)
{
	JoyceMatrix *m = (JoyceMatrix *)png_get_error_ptr(png_ptr);
	UiDrawer *d = gl_sys->m_termMenu;
        UiMenu uim(d);
	string sl1,sl2;
       
	sl1 = "  Error writing PNG file  ";
        uim.add(new UiLabel(sl1, d));
        uim.add(new UiLabel("", d));
        uim.add(new UiLabel(error_msg, d));
        uim.add(new UiLabel("", d));
        uim.add(new UiCommand(SDLK_ESCAPE, "  Cancel", d));
        d->centreContainer(uim);
	uim.eventLoop();

	longjmp(png_jmpbuf(png_ptr), 1);
}


static void matrix_png_warn(png_structp png_ptr, png_const_charp warning_msg)
{
	JoyceMatrix *m = (JoyceMatrix *)png_get_error_ptr(png_ptr);
	joyce_dprintf("PNG warning: %s\n", warning_msg);
}


////////////////////////////////////////////////////////////////////////////
//
// Output page as a PNG file; we use libpng
//
bool JoyceMatrix::startPagePNG(void)
{	
	if (m_drawing_page) return true;
	m_drawing_page = true;

	m_png_pstruct = png_create_write_struct(PNG_LIBPNG_VER_STRING, 
		(png_voidp)this, matrix_png_error, matrix_png_warn);

	if (!m_png_pstruct)
	{
		m_noRam = true;
		return false;
	}

	m_png_pinfo   = png_create_info_struct(m_png_pstruct);
	if (!m_png_pinfo)
	{
		png_destroy_write_struct(&m_png_pstruct, NULL);
		m_noRam = true;
		return false;	
	}
	/* PNG error handling */
	if (setjmp(png_jmpbuf(m_png_pstruct)))
	{
		/* If we get here, we had a problem */
      		png_destroy_write_struct(&m_png_pstruct, &m_png_pinfo);
		m_noRam = true;
		return false;
   	}


	/* Try to open a printer page. For each filename we try, first
	 * attempt to open it for reading. If that succeeds, then 
	 * go to the next file sequence. If it fails, then try to open
	 * it for writing. */
	do
	{
		if (m_pngdir[0] == '|')
		{
			m_fp = openTemp(m_outFilename, ".png", "wb");
		}
		else
		{	
			if (m_pngdir == "") m_pngdir = ".";	
			if (m_pngdir[m_pngdir.length() - 1] == '/')
				m_pngdir = m_pngdir.substr(0, m_pngdir.length() -1);
			checkExistsDir(m_pngdir);
			sprintf(m_outFilename, "%s/matrix%d.png", m_pngdir.c_str(), m_sequence);
			m_fp = fopen(m_outFilename, "rb");
			if (m_fp)
			{
				fclose(m_fp);
				m_fp = NULL;
				++m_sequence;
			}
			else
			{
				m_fp = fopen(m_outFilename, "wb");
				if (!m_fp) ++m_sequence;
			}
		}
	} while (!m_fp);
	png_init_io (m_png_pstruct, m_fp);
	png_set_IHDR(m_png_pstruct, m_png_pinfo,
			m_w, m_h, 1, PNG_COLOR_TYPE_GRAY,
			PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
			PNG_FILTER_TYPE_DEFAULT);
	// png_set_bKGD(m_png_struct, m_png_info, 
	return true;
}


bool JoyceMatrix::endPagePNG(void)
{
	if (!m_drawing_page) return true;
	m_drawing_page = false;

	/* PNG error handling */
	if (setjmp(png_jmpbuf(m_png_pstruct)))
	{
		/* If we get here, we had a problem */
      		png_destroy_write_struct(&m_png_pstruct, &m_png_pinfo);
		m_noRam = true;
		return false;
   	}
	png_set_rows(m_png_pstruct, m_png_pinfo, m_row_pointers);
	png_write_png(m_png_pstruct, m_png_pinfo, PNG_TRANSFORM_IDENTITY, NULL);
        png_destroy_write_struct(&m_png_pstruct, &m_png_pinfo);
	fclose(m_fp);
	if (m_pngdir[0] == '|')
	{
		string s = m_pngdir.substr(1);
		s += ' ';
		s += m_outFilename;
		// Launch in the background. When "lpr" has finished,
		// remove the temp file.
	        Executor ex(s.c_str(), true);

       		ex.exec();
	}
	m_fp = NULL;
	for (int n = 0; n < MAX_H; n++)
	{
		memset(m_row_pointers[n], 0xFF, MAX_W);	
	}
	return true;
}

void JoyceMatrix::ditPNG(int y, int x)
{
	if (y < 0 || y >= MAX_H || x < 0 || x >= MAX_W) return;

	png_byte mask = 0x80;
	png_bytep pb  = m_row_pointers[y] + (x/8);

	while (x % 8)
	{
		--x;
		mask = mask >> 1;
	}
	(*pb) &= ~mask;
}

void JoyceMatrix::dotPNG(float xf, float yf)
{
	int y = (int)(yf + 0.49999);
	int x = (int)(xf + 0.49999);

/* Draw a little circle:
 *
 * 	 xxx
 *	xxxxx
 *	xxxxx
 *	xxxxx
 *	 xxx 
 */

	for (int n = 0; n < 5; n++)
	{	
		if (n != 0 && n != 4)
		{
			ditPNG(y+n, x); ditPNG(y+n, x+4);
		}
		ditPNG(y+n, x+1); 
		ditPNG(y+n, x+2); 
		ditPNG(y+n, x+3);
	}
}

bool JoyceMatrix::hasSettings(SDLKey *key, string &caption)
{
	*key = SDLK_m;
	caption = "  Matrix printer  ";
	return true;
}


UiEvent JoyceMatrix::settings(UiDrawer *d)
{
	char cust[50];
	int x,y,sel;
        UiEvent uie;
	bool a4, a5, a5l, cont11, cont12, custom;
        x = y = 0;
        sel = -1;
	PrinterFormat format = m_format;
	string strdir;
	string oldfn, olddr;

        while (1)
        {
                UiMenu uim(d);
                bool bEnabled;

		a4  = (m_w == 2997 && m_h == 4208);
		a5l = (m_w == 2997 && m_h == 2104);
		a5  = (m_h == 2997 && m_w == 2104);
		cont11 = (m_w == 2880 && m_h == 3960);
		cont12 = (m_w == 2880 && m_h == 4320);
		custom = false;	
		if (!(a4 || a5l || a5 || cont11 || cont12)) custom = true;
		if (custom) sprintf(cust, "  Custom size (%f\" x %f\")  ",
				(m_w / 360.0), (m_h / 360.0) );
		else sprintf(cust, "  Custom size ... ");

                bEnabled = isEnabled();
                uim.add(new UiLabel(getTitle(), d));
                uim.add(new UiSeparator(d));
                uim.add(new UiSetting(SDLK_d, !bEnabled, "  Disconnected  ", d));
                uim.add(new UiSetting(SDLK_c,  bEnabled, "  Connected  ",    d));

                if (bEnabled)
                {
			switch(format)
			{
				case PNG: strdir = m_pngdir; break;
				case PDF: strdir = m_pdfdir; break;
				case PS:  strdir = m_psdir;  break;
			}

                	uim.add(new UiSeparator(d));
                	uim.add(new UiSetting(SDLK_4,  a4,     "  A4 portrait ",    d));
                	uim.add(new UiSetting(SDLK_5,  a5,     "  A5 portrait ",    d));
                	uim.add(new UiSetting(SDLK_l,  a5l,     "  A5 landscape ",   d));
                	uim.add(new UiSetting(SDLK_1,  cont11, "  11\" continuous ",   d));
                	uim.add(new UiSetting(SDLK_2,  cont12, "  12\" continuous ",   d));
                	uim.add(new UiSetting(SDLK_0,  custom, cust,                   d));
                	uim.add(new UiSeparator(d));
                	uim.add(new UiSetting(SDLK_n, (format == PNG), "  Output PNG files",   d));
                	uim.add(new UiSetting(SDLK_o, (format == PS),  "  Output PostScript",  d));
                	uim.add(new UiSetting(SDLK_f, (format == PDF), "  Output PDF files",   d));
#ifdef HAVE_WINDOWS_H
                	uim.add(new UiSetting(SDLK_g, (format == GDI), "  Use Windows' GDI",   d));
#endif
			if (format == PDF)
			{
             		        string ostat = "  Output is to ";
                        	ostat += "directory: " + displayName(strdir, 40) + "  "; 
                        	uim.add(new UiSeparator(d));
                        	uim.add(new UiLabel  (ostat, d));
                        	uim.add(new UiCommand(SDLK_r,  "  Output to diRectory  ", d));
			}
			else 
#ifdef HAVE_WINDOWS_H
			if (format != GDI)
#endif
			{
					
             		        string ostat = "  Output is to ";
                        	if (strdir[0] == '|') ostat += "pipe: " + displayName(strdir.substr(1), 40) + "  ";
                        	else                  ostat += "directory: " + displayName(strdir, 40) + "  "; 
                        	uim.add(new UiSeparator(d));
                        	uim.add(new UiLabel  (ostat, d));
                        	uim.add(new UiCommand(SDLK_r,  "  Output to diRectory  ", d));
				uim.add(new UiCommand(SDLK_u,  "  Output to UNIX command... ",d, UIG_SUBMENU)); 
			}
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
			case SDLK_f:    format = PDF; break;
			case SDLK_n:    format = PNG; break;
			case SDLK_o:    format = PS; break;
#ifdef HAVE_WINDOWS_H
			case SDLK_g:    format = GDI; break;
#endif
			case SDLK_4:	m_w = 2997; m_h = 4208; break;
			case SDLK_l:	m_w = 2997; m_h = 2104;	break;
			case SDLK_5:	m_w = 2104; m_h = 2997; break;
			case SDLK_1:	m_w = 2880; m_h = 3960; break;
			case SDLK_2:	m_w = 2880; m_h = 4320; break;
			case SDLK_0:	uie = getCustomPaper(d);
					if (uie == UIE_QUIT) return uie;
					break;
                        case SDLK_r:	m_y = MAX_H + 1;
					checkEndForm();
					switch (format)
					{
						case PNG: strdir = findPcwDir(FT_PNGS, false); break;
						case PDF: strdir = findPcwDir(FT_PDF, false); break;
						case PS:  strdir = findPcwDir(FT_PS,   false); break;
					}
					break;
                        case SDLK_d:	enable(false); break;
                        case SDLK_c:	enable();      break;
                        case SDLK_u:	m_y = MAX_H + 1;
					oldfn = m_llfilename;
					olddr = m_lldriver;
					checkEndForm();
                                        uie = getPipe(d);
                                        if (uie == UIE_QUIT) return uie;
					strdir = "|";
					strdir += m_llfilename;
					m_llfilename = oldfn;
					m_lldriver = olddr;
                                        break;

                        case SDLK_ESCAPE: return UIE_OK;
                        default: break;
                }	
		if (m_format == PNG) m_pngdir = strdir;
		if (m_format == PDF) m_pdfdir = strdir;
		if (m_format == PS)  m_psdir  = strdir;
		m_format = format;
        }
	return UIE_CONTINUE;
}


string JoyceMatrix::getTitle(void)
{
        return "Built-in dot matrix printer";
}



UiEvent JoyceMatrix::control(UiDrawer *d)
{
	int x,y,sel;
        UiEvent uie;
	char sLine[50];

        x = y = 0;
        sel = -1;

	if (!isEnabled())
	{
                UiMenu uim(d);
                uim.add(new UiLabel("  No printer  ", d));
                uim.add(new UiSeparator(d));
                uim.add(new UiCommand(SDLK_ESCAPE,  "  EXIT  ", d));
                d->centreContainer(uim);
                uie =  uim.eventLoop();
		if (uie != UIE_QUIT) return UIE_CONTINUE;
		return uie;	
	}
        while (1)
        {
                UiMenu uim(d);

		if (m_y <= 390) sprintf(sLine, "  Top of form  ");
		else sprintf(sLine, "  At line: %03d  ", (int)(m_y / 60));

                uim.add(new UiLabel("  Dot-matrix printer  ", d));
		uim.add(new UiLabel(sLine, d));
                uim.add(new UiSeparator(d));
                uim.add(new UiSetting(SDLK_o, !m_bailIn, "  Bail bar Out  ", d));
                uim.add(new UiSetting(SDLK_i,  m_bailIn, "  Bail bar In  ",  d));
                uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_f,  "  Form feed  ", d));
                uim.add(new UiCommand(SDLK_ESCAPE,  "  EXIT  ", d));
                uim.setSelected(sel);
                d->centreContainer(uim);
                uie =  uim.eventLoop();

                if (uie == UIE_QUIT) return uie;
                sel = uim.getSelected();
                switch(uim.getKey(sel))
                {
                        case SDLK_f:
				m_y = MAX_H + 1;
				checkEndForm();
				break;
			case SDLK_i: 
				m_bailIn = true;
				break;
			case SDLK_o:
				m_bailIn = false;
				break;
                        case SDLK_ESCAPE: 	
				return UIE_OK;
                        default: break;
                }

        }
	return UIE_CONTINUE;
}

UiEvent JoyceMatrix::getCustomPaper(UiDrawer *d)
{
        int x,y,sel;
        UiEvent uie;
        char sw[50], sh[50];
	float f;

        x = y = 0;
        sel = -1;
	while (1)
	{
		sprintf(sw, "%f", m_w / 360.0);
		sprintf(sh, "%f", m_h / 360.0);
		UiMenu uim(d);
                uim.add(new UiLabel("  Custom paper size  ", d));
                uim.add(new UiSeparator(d));
                uim.add(new UiTextInput(SDLK_w, "  Width in inches: __________  ",d));
                uim.add(new UiTextInput(SDLK_h, "  Height in inches: __________  ",d));
                uim.add(new UiSeparator(d));
                uim.add(new UiCommand(SDLK_ESCAPE,  "  EXIT  ", d));
                d->centreContainer(uim);
                uim.setSelected(sel);
                ((UiTextInput &)uim[2]).setText(sw);
                ((UiTextInput &)uim[3]).setText(sh);

                uie =  uim.eventLoop();
                if (uie == UIE_QUIT) return uie;
                sel = uim.getSelected();
                switch(uim.getKey(sel))
                {
			case SDLK_w:	f = atof(((UiTextInput &)uim[2]).getText().c_str());
					if (f) m_w = (int)(360.0 * f);
					break;
			case SDLK_h:	f = atof(((UiTextInput &)uim[3]).getText().c_str());
					if (f) m_h = (int)(360.0 * f);
					break;
			case SDLK_ESCAPE:
					return UIE_OK;
			default:	break;
		}
	}
	return UIE_CONTINUE;	
}


////////////////////////////////////////////////////////////////////////////
//
// Output page as a PDF document
//

static void pdf_error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no,
				void *user_data)
{
	JoyceMatrix *m = (JoyceMatrix *)user_data;

	UiDrawer *d = gl_sys->m_termMenu;
        UiMenu uim(d);
	string sl1,sl2;
	char msg1[220];
	char msg2[220];
      
	sprintf(msg1, "  HPDF error: %ld  ", error_no);
	sprintf(msg2, "  HPDF detail number: %ld  ", detail_no);
	sl1 = "  Error writing PDF file  ";
        uim.add(new UiLabel(sl1, d));
        uim.add(new UiLabel("", d));
        uim.add(new UiLabel(msg1, d));
        uim.add(new UiLabel(msg2, d));
        uim.add(new UiLabel("", d));
        uim.add(new UiCommand(SDLK_ESCAPE, "  Cancel", d));
        d->centreContainer(uim);
	uim.eventLoop();

}

bool JoyceMatrix::startPagePDF(void)
{	
	if (m_pdfDoc == NULL)
	{
		m_pdfDoc = HPDF_New(pdf_error_handler, this);
		if (!m_pdfDoc) return false;
		HPDF_SetCompressionMode(m_pdfDoc, HPDF_COMP_ALL);
	}
	if (m_pdfPage) return true;

	m_pdfPage = HPDF_AddPage(m_pdfDoc);
	HPDF_Page_SetWidth(m_pdfPage, m_w / 5.0);
	HPDF_Page_SetHeight(m_pdfPage, m_h / 5.0);

	return true;
}


void JoyceMatrix::closePDF(void)
{
	if (m_pdfDoc)
	{
		if (m_pdfdir == "") m_pdfdir = ".";	
		if (m_pdfdir[m_pdfdir.length() - 1] == '/')
		m_pdfdir = m_pdfdir.substr(0, m_pdfdir.length() -1);
		checkExistsDir(m_pdfdir);
		do
		{
			sprintf(m_outFilename, "%s/matrix%d.pdf", m_pdfdir.c_str(), m_sequence);
			m_fp = fopen(m_outFilename, "rb");
			if (!m_fp) break;
			if (m_fp)
			{
				fclose(m_fp);
				m_fp = NULL;
				++m_sequence;
			}
		} while (1);
		HPDF_SaveToFile(m_pdfDoc, m_outFilename);
		HPDF_Free(m_pdfDoc);
		m_pdfDoc = NULL;
		m_pdfPage = NULL;
	}
}


bool JoyceMatrix::endPagePDF(void)
{
	m_pdfPage = NULL;
	return true;
}

void JoyceMatrix::dotPDF(float xf, float yf)
{
	if (!m_pdfDoc || !m_pdfPage) return;
//
// Convert from the 360dpi coords used by JOYCE internally
// to the 72dpi coordinates used by Postscript
// Size of dot is 1/72in x 1/72in.
//
	double xp = (xf / 5);
	double yp = ((m_h + 360 - yf) / 5);

	HPDF_Page_Circle(m_pdfPage, xp - 0.5, yp - 0.5, 1);
	HPDF_Page_Fill(m_pdfPage);
}




////////////////////////////////////////////////////////////////////////////
//
// Output page as a Postscript file
//
bool JoyceMatrix::startPagePS(void)
{	
	if (m_drawing_page) return true;
	m_drawing_page = true;

	/* Try to open a printer page. For each filename we try, first
	 * attempt to open it for reading. If that succeeds, then 
	 * go to the next file sequence. If it fails, then try to open
	 * it for writing. 
	 *
	 * Note that this is a while() loop rather than a do() loop, 
	 * because PS (unlike PNG) can use the same file for multiple pages.
	 * */
	
	while (!m_fp)	
	{
		if (m_psdir[0] == '|')
		{
			m_fp = popen(m_psdir.c_str() + 1, "w");
		}
		else
		{	
			if (m_psdir == "") m_psdir = ".";	
			if (m_psdir[m_psdir.length() - 1] == '/')
				m_psdir = m_psdir.substr(0, m_psdir.length() -1);
			checkExistsDir(m_psdir);
			sprintf(m_outFilename, "%s/matrix%d.ps", m_psdir.c_str(), m_sequence);
			m_fp = fopen(m_outFilename, "rb");
			if (m_fp)
			{
				fclose(m_fp);
				m_fp = NULL;
				++m_sequence;
			}
			else
			{
				m_fp = fopen(m_outFilename, "w");
				if (!m_fp) ++m_sequence;
			}
		}
	}
	fprintf(m_fp,
		"%%! \n"
		"%% PCW matrix page \n"
		"%% Auto-generated by JOYCE v%d.%d.%d"
		"%% All coordinates in points\n\n", 
                BCDVERS >> 8,
               (BCDVERS >> 4) & 0x0F,
                BCDVERS & 0x0F);

	return true;
}


void JoyceMatrix::closePS(void)
{
	if (!m_fp) return;
	if (m_psdir[0] == '|') pclose(m_fp);	
	else fclose(m_fp);
	m_fp = NULL;
}


bool JoyceMatrix::endPagePS(void)
{
	if (!m_drawing_page) return true;
	m_drawing_page = false;

	fprintf(m_fp, "\n\nshowpage\n");

	//closePS();	// No point. All printer output can go in one
			// big PS file.

	for (int n = 0; n < MAX_H; n++)
	{
		memset(m_row_pointers[n], 0xFF, MAX_W);	
	}
	return true;
}

void JoyceMatrix::dotPS(float xf, float yf)
{
	if (!m_fp) return;

//
// Convert from the 360dpi coords used by JOYCE internally
// to the 72dpi coordinates used by Postscript
// Size of dot is 1/72in x 1/72in.
//
	double xp = (xf / 5);
	double yp = ((m_h + 360 - yf) / 5);

	fprintf(m_fp, " newpath \n");
	fprintf(m_fp, " %f %f moveto \n", xp - 0.5, yp - 0.5);
	fprintf(m_fp, "1 0 rlineto 0 1 rlineto -1 0 rlineto closepath fill\n"); 
}


////////////////////////////////////////////////////////////////////////////
//
// Output page as a PNG file; we use libpng
//
bool JoyceMatrix::startPage(void)
{	
	switch(m_format)
	{
		case PS:  return startPagePS();
		case PNG: return startPagePNG();
		case PDF: return startPagePDF();
#ifdef HAVE_WINDOWS_H
		case GDI: 
			  if (m_drawing_page) return true;
			  m_drawing_page = m_gdiPrint.startPage();
			  return m_drawing_page;
#endif
	}
	return false;
}


bool JoyceMatrix::endPage(void)
{
	switch(m_format)
	{
		case PS:  return endPagePS();
		case PNG: return endPagePNG();
		case PDF: return endPagePDF();
#ifdef HAVE_WINDOWS_H
		case GDI: m_drawing_page = false;
			  return m_gdiPrint.endPage();
#endif
	}
	return false;
}


void JoyceMatrix::dot(float xf, float yf)
{
	if (!m_drawing_page)
	{
		if (!startPage()) return;
	}

	switch(m_format)
	{
		case PS:  dotPS(xf, yf);   break;
		case PNG: dotPNG(xf, yf);  break;
		case PDF: dotPDF(xf, yf);  break;
#ifdef HAVE_WINDOWS_H
		case GDI: m_gdiPrint.putDot(xf, yf); break;
#endif
	}
}


