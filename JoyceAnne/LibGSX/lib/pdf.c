/*
 * MIT License
 * C-GSX Copyright 2026 John Elliott
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "cgsximpl.h"
#if defined(HAVE_LIBHPDF) && defined(HAVE_HPDF_H)

#include <setjmp.h>

typedef struct palette_entry
{
	float r, g, b;
} PALETTE_ENTRY;

static PALETTE_ENTRY defpalette[8] = 
{
	{ 1.0, 1.0, 1.0 },	/* White (background) */
	{ 1.0, 0.0, 0.0 },	/* Red */
	{ 0.0, 1.0, 0.0 },	/* Green */
	{ 0.0, 0.0, 1.0 },	/* Blue */
	{ 0.0, 1.0, 1.0 },	/* Cyan */
	{ 1.0, 1.0, 0.0 },	/* Yellow */
	{ 1.0, 0.0, 1.0 },	/* Magenta */
	{ 0.0, 0.0, 0.0 },	/* Black */
};

#define MAXHATCH 6
#define HATCHPITCH 5
#define MAXINK 32768

typedef struct pdf_gios
{
	GIOSDEVICE vtable;
	gword width;
	gword height;
	int dirty;
	unsigned pages;
	HPDF_Doc pdf;
	HPDF_Page page;
	HPDF_Font font[4];
	char *outputdir;	
	char *outputfile;	
	float fontsize;
	jmp_buf env;

	gword linestyle;
	gword linecolour;
	gword linewidth;

	gword markstyle;
	gword markcolour;
	gword markheight;

	gword textfont;
	gword textangle;
	gword textcolour;
	gword fillstyle;
	gword fillindex;
	gword fillcolour;
	gword writemode;
	PALETTE_ENTRY palette[MAXINK];
} PDF_GIOS;

static void pdf_destroy(GIOSDEVICE **ptr);
static int pdf_dispatch(GIOSDEVICE *self, gword *contrl, 
		gword *intin, gword *ptsin, gword *intout, gword *ptsout);

static int pdf_opnwk(PDF_GIOS *self, gword *contrl, 
		gword *intin, gword *ptsin, gword *intout, gword *ptsout);
static void pdf_clswk(PDF_GIOS *self);
static void pdf_clrwk(PDF_GIOS *self);
static void pdf_updwk(PDF_GIOS *self);
static void pdf_escape(PDF_GIOS *self, gword *contrl, 
		gword *intin, gword *ptsin, gword *intout, gword *ptsout);
static void pdf_pline(PDF_GIOS *self, gword count, gword *ptsin);
static void pdf_polymarker(PDF_GIOS *self, gword count, gword *ptsin);
static void pdf_gtext(PDF_GIOS *self, gword count, gword *intin, gword x0, 
		gword y0);
static void pdf_fillarea(PDF_GIOS *self, gword count, gword *ptsin);

static void pdf_setcolour(PDF_GIOS *self, gword *intin);
static void pdf_getcolour(PDF_GIOS *self, gword colour, gword *intout);
static void pdf_cellarray(PDF_GIOS *self, gword *contrl, gword *ptsin, 
		gword count, gword *intin);
static void pdf_gdp(PDF_GIOS *self, gword *contrl, 
		gword *intin, gword *ptsin, gword *intout, gword *ptsout);

static gword pdf_slstyle(PDF_GIOS *self, gword style);
static gword pdf_slwidth(PDF_GIOS *self, gword width);
static gword pdf_slcolour(PDF_GIOS *self, gword colour);
static gword pdf_smtype(PDF_GIOS *self, gword style);
static gword pdf_smheight(PDF_GIOS *self, gword height);
static gword pdf_smcolour(PDF_GIOS *self, gword colour);
static void  pdf_stheight(PDF_GIOS *self, gword height, gword *ptsout);
static gword pdf_stangle(PDF_GIOS *self, gword angle);
static gword pdf_stfont(PDF_GIOS *self, gword font);
static gword pdf_stcolour(PDF_GIOS *self, gword colour);
static gword pdf_sfstyle(PDF_GIOS *self, gword style);
static gword pdf_sfindex(PDF_GIOS *self, gword index);
static gword pdf_sfcolour(PDF_GIOS *self, gword colour);
static gword pdf_swrmode(PDF_GIOS *self, gword mode);
static void pdf_hatched_rect(PDF_GIOS *self, float x0, float y0, float x1, float y1);
static void pdf_newpage(PDF_GIOS *self);

int gios_pdf_create(PGDOS gdos, GIOSDEVICE **ptr, void *p)
{
	PDF_GIOS *g = malloc(sizeof(PDF_GIOS));
	PDFPARAM *param = (PDFPARAM *)p;

	if (!g) return -1;
	memset(g, 0, sizeof(*g));
	g->vtable.destroy = pdf_destroy;
	g->vtable.dispatch = pdf_dispatch;
	g->width = 10 * param->width;
	g->height = 10 * param->height;
	g->pdf = NULL;

	g->outputfile = malloc(20 + strlen(param->outputdir));
	if (g->outputfile)
		g->outputfile[0] = 0;
	if (param->outputdir == NULL)
	{
		g->outputdir = malloc(2);
		if (g->outputdir)
			strcpy(g->outputdir, ".");
	}
	else
	{
		g->outputdir = malloc(1 + strlen(param->outputdir));
		if (g->outputdir)
			strcpy(g->outputdir, param->outputdir);
	}		
	*ptr = &g->vtable;
	return 0;	
}

static int pdf_genfilename(PDF_GIOS *self)
{
	FILE *fp;
	int n = 0;

	do
	{
		sprintf(self->outputfile, "%s/cgsx%08d.pdf", self->outputdir, n);
		fp = fopen(self->outputfile, "rb");
		if (fp)
		{
			fclose(fp);	/* File exists. Go to next number */
		}
		else
		{
			fp = fopen(self->outputfile, "wb");
			/* Can't create file. Bail. */
			if (!fp) return -1;
			/* Can create file. Use it. */
			fclose(fp);
			return 0;
		}
		++n;
	} while (1);

}

static void pdf_newpage(PDF_GIOS *self)
{
	self->page = HPDF_AddPage(self->pdf);
	HPDF_Page_SetWidth(self->page, self->width / 10.0);
	HPDF_Page_SetHeight(self->page, self->height / 10.0);
	++self->pages;
/* If the background is not white, we need to draw it in the colour it actually
 * is */
	if (self->palette[0].r != 1.0 ||
	    self->palette[0].g != 1.0 ||
	    self->palette[0].b != 1.0)
	{
		HPDF_Page_SetRGBFill(self->page, 
			self->palette[0].r,
			self->palette[0].g,
			self->palette[0].b);

		HPDF_Page_Rectangle(self->page, 0, 0, self->width, self->height);
		HPDF_Page_Fill(self->page);
	}
}

static void pdf_preview(PDF_GIOS *self)
{
/*
	char *buf = malloc(20 + strlen(self->outputfile));
	if (buf)
	{	
		sprintf(buf, "evince %s &", self->outputfile);
		system(buf);
		free(buf);
	}
*/
}


static void pdf_destroy(GIOSDEVICE **ptr)
{
	if (*ptr)
	{
		PDF_GIOS *self = (PDF_GIOS *)(*ptr);
		if (self->pdf)
		{
			if (self->pages && !pdf_genfilename(self))
			{
				HPDF_SaveToFile(self->pdf, self->outputfile);
				pdf_preview(self);
			}
			HPDF_Free(self->pdf);
		}
		if (self->outputdir) free(self->outputdir);
		if (self->outputfile) free(self->outputfile);
	
		free(*ptr);
		*ptr = NULL;
	}
}

/* static FILE *fp; */

static int pdf_dispatch(GIOSDEVICE *s, gword *contrl, 
		gword *intin, gword *ptsin, gword *intout, gword *ptsout)
{
	PDF_GIOS *self = (PDF_GIOS *)s;
/*	fp = fopen("pdflog.log", "at");

fprintf(fp, "PDF GIOS function %d entry\n", contrl[0]);  */
	if (setjmp(self->env))
	{
fprintf(stderr, "PDF GIOS function %d failed\n", contrl[0]); 
/* fprintf(fp, "PDF GIOS function %d failed\n", contrl[0]); 
fclose(fp); */
		return -1;
	}
	switch (contrl[0])
	{
		case 1: return pdf_opnwk(self, contrl, intin, ptsin, 
					intout, ptsout);
		case 2: pdf_clswk(self); break;
		case 3: pdf_clrwk(self); break;
		case 4: pdf_updwk(self); break;
		case 5: pdf_escape(self, contrl, intin, ptsin, intout, ptsout);
			break;
		case 6: pdf_pline(self, contrl[NPTSIN], ptsin);
			break;
		case 7: pdf_polymarker(self, contrl[NPTSIN], ptsin);
			break;
		case 8: pdf_gtext(self, contrl[NINTIN], intin, ptsin[0], ptsin[1]);
			break;
		case 9: pdf_fillarea(self, contrl[NPTSIN], ptsin);
			break;
		case 10: pdf_cellarray(self, contrl, ptsin, contrl[NINTIN],
				intin);
		case 11: pdf_gdp(self, contrl, intin, ptsin, intout, ptsout);
			break;
		case 12: pdf_stheight(self, ptsin[1], ptsout);
			 contrl[NPTSOUT] = 2;
			 break;
		case 13: intout[0] = pdf_stangle(self, intin[0]);
			 contrl[NINTOUT] = 1;
			 break;
		case 14: pdf_setcolour(self, intin);
			 break;
		case 15: intout[0] = pdf_slstyle(self, intin[0]);
			 contrl[NINTOUT] = 1;
			 break;
		case 16: ptsout[0] = pdf_slwidth(self, ptsin[0]);
			 ptsout[1] = 0;
			 contrl[NPTSOUT] = 1;
			 break;
		case 17: intout[0] = pdf_slcolour(self, intin[0]);
			 contrl[NINTOUT] = 1;
			 break;
		case 18: intout[0] = pdf_smtype(self, intin[0]);
			 contrl[NINTOUT] = 1;
			 break;
		case 19: ptsout[1] = pdf_smheight(self, ptsin[1]);
			 ptsout[0] = 0;
			 contrl[NPTSOUT] = 1;
			 break;
		case 20: intout[0] = pdf_smcolour(self, intin[0]);
			 contrl[NINTOUT] = 1;
			 break;
		case 21: intout[0] = pdf_stfont(self, intin[0]);
			 contrl[NINTOUT] = 1;
			 break;
		case 22: intout[0] = pdf_stcolour(self, intin[0]);
			 contrl[NINTOUT] = 1;
			 break;
		case 23: intout[0] = pdf_sfstyle(self, intin[0]);
			 contrl[NINTOUT] = 1;
			 break;
		case 24: intout[0] = pdf_sfindex(self, intin[0]);
			 contrl[NINTOUT] = 1;
			 break;
		case 25: intout[0] = pdf_sfcolour(self, intin[0]);
			 contrl[NINTOUT] = 1;
			 break;
		case 26: pdf_getcolour(self, intin[0], intout);
			 contrl[NINTOUT] = 3;
			 break;
/* Reading a cell array is not possible, but set the results to reasonable
 * values just in case anyone tries to */
		case 27:
			contrl[7] = 0;
			contrl[8] = 0;
			contrl[9] = 1;
			break;
		case 32: intout[0] = pdf_swrmode(self, intin[0]);
			 contrl[NINTOUT] = 1;
			 break;

		default:
		fprintf(stderr, "pdf_dispatch function=%d unimplemented\n",
			contrl[0]);
	}	
/*
fprintf(fp, "PDF GIOS function %d complete\n", contrl[0]); 
fclose(fp); */
	return 0;
}

static void error_handler (HPDF_STATUS   error_no,
               HPDF_STATUS   detail_no,
               void         *user_data)
{
	PDF_GIOS *self = (PDF_GIOS *)user_data;

	fprintf (stderr, "HPDF ERROR: error_no=%04X, detail_no=%u\n", 
		(HPDF_UINT)error_no, (HPDF_UINT)detail_no);
/*	fprintf (fp, "HPDF ERROR: error_no=%04X, detail_no=%u\n", 
		(HPDF_UINT)error_no, (HPDF_UINT)detail_no); */
	longjmp(self->env, 1);
}


static int pdf_opnwk(PDF_GIOS *self, gword *contrl, 
		gword *intin, gword *ptsin, gword *intout, gword *ptsout)
{
	int n = 0;

	if (self->pdf)
	{
		pdf_clswk(self);
	}

	if (self->outputdir == NULL || self->outputfile == NULL)
		return -1;	/* Not enough memory to open workstation */

	self->pdf = HPDF_New(error_handler, self);
	if (!self->pdf)
	{
		fprintf(stderr, "Could not create HPDF object\n");
		return -1;
	}
	self->pages = 0;
	HPDF_SetCompressionMode(self->pdf, HPDF_COMP_ALL);

	contrl[NINTOUT] = 45;
	contrl[NPTSOUT] = 6;
	for (n = 0; n < 45; n++) intout[n] = 0;
	for (n = 0; n < 12; n++) ptsout[n] = 0;

	intout[0] = self->width - 1;
	intout[1] = self->height - 1;
	intout[2] = 0;	/* Vector device, can scale */
	intout[3] = 353;
	intout[4] = 353;	/* 1pt is roughly 353 micrometers */
	intout[5] = 8;		/* 8 character sizes */
	intout[6] = 5;		/* 5 line styles */
	intout[7] = 8;		/* 8 line widths */
	intout[8] = 6;		/* 6 marker styles */
	intout[9] = 8;		/* 8 marker sizes */
	intout[10] = 4;		/* 4 fonts */
	intout[11] = MAXHATCH;	/* We support 6 halftones */
	intout[12] = MAXHATCH;	/* And six hatches */
	intout[13] = MAXINK-1;	/* Palette size */
	intout[14] = 4;		/* 4 GDPs supported */
	intout[15] = 1;		/* Bar */
	intout[16] = 2;		/* Arc */
	intout[17] = 3;		/* Pie slice */
	intout[18] = 4;		/* Circle */
	for (n = 19; n <= 24; n++) intout[n] = 0xFFFF;
	intout[25] = 3;		/* Bar is filled area */
	intout[26] = 1;		/* Arc is a line */
	intout[27] = 3;		/* Pie slice is filled area */
	intout[28] = 3;		/* Circle is filled area */
	for (n = 29; n <= 34; n++) intout[n] = 0xFFFF;
	intout[35] = 1;		/* Colour device */
	intout[36] = 1;		/* Text rotation supported */
	intout[37] = 1;		/* Filled area support */
	intout[38] = 0;		/* Can't read cell arrays */
	intout[39] = 0;		/* More than 32768 colours */
	/* Input devices */
	intout[40] = 0;	/* No locators available */
	intout[41] = 0;	/* No valuators available */
	intout[42] = 0;	/* No choice devices available */
	intout[43] = 0;	/* No string devices available */
	intout[45] = 0;	/* Output only */

	self->dirty = 0;
	self->page = NULL;
	self->font[0] = HPDF_GetFont(self->pdf, "Courier", NULL);
	self->font[1] = HPDF_GetFont(self->pdf, "Courier-Bold", NULL);
	self->font[2] = HPDF_GetFont(self->pdf, "Courier-Oblique", NULL);
	self->font[3] = HPDF_GetFont(self->pdf, "Courier-BoldOblique", NULL);
	self->textfont = 1;
	self->textangle = 0;
	self->fontsize = 10.0;
/* Initialise the palette */
	for (n = 0; n < 8; n++)
	{
		self->palette[n] = defpalette[n];
	}
	for (n = 8; n < MAXINK; n++)
	{
		self->palette[n].r = 0.0;
		self->palette[n].g = 0.0;
		self->palette[n].b = 0.0;
	}
	pdf_slstyle(self, intin[1]);
	pdf_slwidth(self, 10);
	pdf_slcolour(self, intin[2]);
	pdf_smheight(self, 10);
	pdf_smcolour(self, intin[4]);
	pdf_stfont(self, intin[5]);
	pdf_stcolour(self, intin[6]);
	pdf_sfstyle(self, intin[7]);
	pdf_sfindex(self, intin[8]);
	pdf_sfcolour(self, intin[9]);
	pdf_stheight(self, 100, ptsout);
	ptsout[0] = ptsout[2];	/* Minimum character width */
	ptsout[1] = ptsout[3];	/* Minimum character height */
	ptsout[2] = ptsout[0] * 8;
	ptsout[3] = ptsout[1] * 8;	/* Maximum character size */
	ptsout[4] = 1;			/* Minimum line width */
	ptsout[5] = 0;
	ptsout[6] = 150;		/* Maximum line width */
	ptsout[7] = 0;
	ptsout[8] = 0;
	ptsout[9] = 10;			/* Minimum marker size */
	ptsout[10] = 0;
	ptsout[11] = 400;		/* Maximum marker size */
	return 0;
}

static void pdf_clswk(PDF_GIOS *self)
{
	if (self->pdf)
	{
		if (self->pages && !pdf_genfilename(self))
		{
			HPDF_SaveToFile(self->pdf, self->outputfile);
			pdf_preview(self);
		}
		HPDF_Free(self->pdf);
		self->pdf = NULL;	
	}
}


static void pdf_clrwk(PDF_GIOS *self)
{
	if (self->pdf)
	{
		if (self->dirty)
		{
			pdf_newpage(self);
			self->dirty = 0;
		}
	}
}

static void pdf_updwk(PDF_GIOS *self)
{
/* If there was a function to flush the current page, I'd call it here; but
 * it doesn't seem that there is */
	self->page = NULL;
	self->dirty = 0;	/* Will trigger an AddPage on the next draw */
}


static void pdf_escape(PDF_GIOS *self, gword *contrl, 
		gword *intin, gword *ptsin, gword *intout, gword *ptsout)
{
	switch(contrl[0])
	{
		case 1: intout[0] = 0xFFFF;	/* No addressable cursor */
			intout[1] = 0xFFFF;
			contrl[NINTOUT] = 2;
			break;
		case 16:intout[0] = 0;		/* No pointing device */
			contrl[NINTOUT] = 1;
			break;
	}
}


static void pdf_setupline(PDF_GIOS *self, gword writemode)
{
/* The type used by HPDF to specify dashes changed after 2.3.x */
#if ((HPDF_MAJOR_VERSION <= 2) && (HPDF_MINOR_VERSION <= 3))
	static HPDF_UINT16 dashes[] = { 8 }; 
	static HPDF_UINT16 dotted[] = { 3, 5 }; 
	static HPDF_UINT16 dashdot[] = { 7, 3, 3, 3 }; 
	static HPDF_UINT16 longdash[] = { 12, 4 }; 
#else
	static HPDF_REAL dashes[] = { 8 }; 
	static HPDF_REAL dotted[] = { 3, 5 }; 
	static HPDF_REAL dashdot[] = { 7, 3, 3, 3 }; 
	static HPDF_REAL longdash[] = { 12, 4 }; 
#endif
	gword ink;

	if (writemode == MODE_ERASE)
	{
		ink = 0;
	}
	else
	{
		ink = self->linecolour;
	}		

	HPDF_Page_SetLineWidth(self->page, self->linewidth / 10.0);
	
	HPDF_Page_SetRGBStroke(self->page,
			self->palette[ink].r,
			self->palette[ink].g,
			self->palette[ink].b);

	switch(self->linestyle)
	{
		default:
		case 1: HPDF_Page_SetDash(self->page, NULL, 0, 0); break;
		case 2: HPDF_Page_SetDash(self->page, dashes, 1, 1); break;
		case 3: HPDF_Page_SetDash(self->page, dotted, 2, 1); break;
		case 4: HPDF_Page_SetDash(self->page, dashdot, 4, 1); break;
		case 5: HPDF_Page_SetDash(self->page, longdash, 2, 1); break;
	}	
}

#include <stdio.h>

static void pdf_pline(PDF_GIOS *self, gword count, gword *ptsin)
{
	int n;

	if (!count) return;
	if (self->page == NULL)
	{
		pdf_newpage(self);
	}
	if (self->writemode == MODE_REPLACE)
	{
		pdf_setupline(self, MODE_ERASE);
		HPDF_Page_SetDash(self->page, NULL, 0, 0);
		HPDF_Page_MoveTo(self->page, ptsin[0] / 10.0, ptsin[1] / 10.0);
		for (n = 1; n < count; n++)
		{
			HPDF_Page_LineTo(self->page, ptsin[n*2] / 10.0, ptsin[n*2+1] / 10.0);
		}
		HPDF_Page_Stroke(self->page);
	}
	pdf_setupline(self, self->writemode);

	HPDF_Page_MoveTo(self->page, ptsin[0] / 10.0, ptsin[1] / 10.0);
	for (n = 1; n < count; n++)
	{
		HPDF_Page_LineTo(self->page, ptsin[n*2] / 10.0, ptsin[n*2+1] / 10.0);
	}
	HPDF_Page_Stroke(self->page);
	self->dirty = 1;
}

static void pdf_setupmarker(PDF_GIOS *self, gword writemode)
{
	gword ink;

	if (writemode == MODE_ERASE)
	{
		ink = 0;
	}
	else
	{
		ink = self->markcolour;
	}		
	HPDF_Page_SetLineWidth(self->page, 0.5);
	HPDF_Page_SetRGBStroke(self->page,
			self->palette[ink].r,
			self->palette[ink].g,
			self->palette[ink].b);
	HPDF_Page_SetRGBFill(self->page,
			self->palette[ink].r,
			self->palette[ink].g,
			self->palette[ink].b);

	HPDF_Page_SetDash(self->page, NULL, 0, 0); 
}

static void pdf_drawmarker(PDF_GIOS *self, int x0, int y0)
{
	float x = x0 / 10.0;
	float y = y0 / 10.0;
	float marksize = self->markheight / 10.0;
	
	switch(self->markstyle)
	{
		case 1: /* dot */
			HPDF_Page_Rectangle(self->page, x, y, 1, 1);
			break;

		case 2: /* plus */
			HPDF_Page_MoveTo(self->page, x - marksize, y);
			HPDF_Page_LineTo(self->page, x + marksize, y);
			HPDF_Page_MoveTo(self->page, x, y - marksize);
			HPDF_Page_LineTo(self->page, x, y + marksize);
			break; 
		case 3: /* asterisk */
			HPDF_Page_MoveTo(self->page, x - marksize, y);
			HPDF_Page_LineTo(self->page, x + marksize, y);
			HPDF_Page_MoveTo(self->page, x - (marksize / 2), y - marksize);
			HPDF_Page_LineTo(self->page, x + (marksize / 2), y + marksize);
			HPDF_Page_MoveTo(self->page, x + (marksize / 2), y - marksize);
			HPDF_Page_LineTo(self->page, x - (marksize / 2), y + marksize);
			break; 
		case 4: /* circle */
			HPDF_Page_Circle(self->page, x, y, marksize);
			break;		
		case 5: /* diamond */
			HPDF_Page_MoveTo(self->page, x - marksize, y);
			HPDF_Page_LineTo(self->page, x, y - marksize);
			HPDF_Page_LineTo(self->page, x + marksize, y);
			HPDF_Page_LineTo(self->page, x, y + marksize);
			HPDF_Page_LineTo(self->page, x - marksize, y);
			break;
		case 6: /* square */
			HPDF_Page_MoveTo(self->page, x - marksize, y - marksize);
			HPDF_Page_LineTo(self->page, x + marksize, y - marksize);
			HPDF_Page_LineTo(self->page, x + marksize, y + marksize);
			HPDF_Page_LineTo(self->page, x - marksize, y + marksize);
			HPDF_Page_LineTo(self->page, x - marksize, y - marksize);
			break;

	}
	if (self->markstyle == 1)
	{
		HPDF_Page_FillStroke(self->page);
	}
	else
	{
		HPDF_Page_Stroke(self->page);
	}
}



static void pdf_polymarker(PDF_GIOS *self, gword count, gword *ptsin)
{
	gword n;

	if (self->page == NULL)
	{
		pdf_newpage(self);
	}
	if (self->writemode == MODE_REPLACE)
	{
		pdf_setupmarker(self, MODE_ERASE);
		for (n = 0; n < count; n++)
		{
			pdf_drawmarker(self, ptsin[n*2], ptsin[n*2+1]);
		}
	}
	pdf_setupmarker(self, self->writemode);
	for (n = 0; n < count; n++)
	{
		pdf_drawmarker(self, ptsin[n*2], ptsin[n*2+1]);
	}	
	self->dirty = 1;
}



static void pdf_gtext(PDF_GIOS *self, gword count, gword *intin, gword x0, gword y0)
{
	float x = (x0 / 10.0);
	float y = (y0 / 10.0);
	char *buf = malloc(1 + count);
	gword n, ink;
	float angle;

	if (!buf) return;


	angle = self->textangle / 1800.0 * M_PI;

	for (n = 0; n < count; n++)
	{
		buf[n] = intin[n];
	}
	buf[count] = 0;
	if (self->page == NULL)
	{
		pdf_newpage(self);
	}
	/* To match the behaviour of GSX when drawing text in REPLACE mode,
	 * a white rectangle behind it */
	if (self->writemode == MODE_REPLACE)
	{
		HPDF_Rect textbox;

		/* Get text rectangle */
		HPDF_Rect bbox = HPDF_Font_GetBBox(self->font[self->textfont - 1]);
		HPDF_TextWidth capWidth = HPDF_Font_TextWidth(self->font[self->textfont - 1], (HPDF_BYTE *)buf, count);

		textbox.top    = (bbox.top * self->fontsize / 1000.0);
		textbox.bottom = (bbox.bottom * self->fontsize / 1000.0);
		textbox.left   = (bbox.left * self->fontsize / 1000.0); 
		textbox.right  = (capWidth.width * self->fontsize / 1000.0); 
	
		HPDF_Page_SetRGBFill(self->page, 
				self->palette[0].r,
				self->palette[0].g,
				self->palette[0].b);
		/* Optimise for what I hope is the default case, where the 
		 * text angle is standard horizontal */
		if (0 == self->textangle)
		{
			HPDF_Page_Rectangle(self->page, 
				x + textbox.left, y + textbox.top, 
				textbox.right - textbox.left,
				textbox.bottom - textbox.top);
		}
		else	/* Transform the rectangle so it (hopefully) matches  */
		{	/* the text rotation */
			float dcos = -cos(angle);
			float dsin =  sin(angle);
			float xorg = x + textbox.left;
			float yorg = y;

			HPDF_Page_MoveTo(self->page, xorg, yorg);
			HPDF_Page_LineTo(self->page, xorg - dsin * textbox.top,
						     yorg - dcos * textbox.top);
			HPDF_Page_LineTo(self->page, xorg - dsin * textbox.top - dcos * (textbox.right - textbox.left),
						     yorg - dcos * textbox.top + dsin * (textbox.right - textbox.left));
			HPDF_Page_LineTo(self->page, xorg - dsin * textbox.bottom - dcos * (textbox.right - textbox.left),
						     yorg - dcos * textbox.bottom + dsin * (textbox.right - textbox.left));
			HPDF_Page_LineTo(self->page, xorg - dsin * textbox.bottom,
						     yorg - dcos * textbox.bottom);
			HPDF_Page_LineTo(self->page, xorg, yorg);
		}
		HPDF_Page_Fill(self->page);
	}
	HPDF_Page_BeginText(self->page);
	if (self->writemode == MODE_ERASE)
	{
		ink = 0;
	}
	else
	{
		ink = self->textcolour;
	}


	HPDF_Page_MoveTextPos(self->page, x, y);
	HPDF_Page_SetFontAndSize (self->page, self->font[self->textfont - 1],
					self->fontsize);
	HPDF_Page_SetTextMatrix(self->page, cos(angle), sin(angle), 
					-sin(angle), cos(angle), x, y);
	HPDF_Page_SetRGBFill(self->page, 
			self->palette[ink].r,
			self->palette[ink].g,
			self->palette[ink].b);
	HPDF_Page_ShowText(self->page, buf);
	HPDF_Page_EndText(self->page);

	free(buf); 
	self->dirty = 1;
}


static void pdf_setupfill(PDF_GIOS *self, int writemode)
{
	float shade;
	gword ink;

	if (writemode == MODE_ERASE)
	{
		ink = 0;
	}
	else
	{
		ink = self->fillcolour;
	}		
	HPDF_Page_SetDash(self->page, NULL, 0, 0); 
	HPDF_Page_SetRGBStroke(self->page, 
			self->palette[ink].r,
			self->palette[ink].g,
			self->palette[ink].b);

	HPDF_Page_SetLineWidth(self->page, 0.5);

	if (self->fillstyle == 2 && writemode != MODE_ERASE)
	{
		shade = 1.0 - (self->fillindex / (float)MAXHATCH);
	}
	else
	{
		shade = 1.0;
	}
	if (self->palette[ink].r == 1.0 &&
	    self->palette[ink].g == 1.0 &&
	    self->palette[ink].b == 1.0)
	{
		/* We can't desaturate white, so just use the shade */
		HPDF_Page_SetRGBFill(self->page, 
			self->palette[ink].r * (1 - shade),
			self->palette[ink].g * (1 - shade),
			self->palette[ink].b * (1 - shade));
	}
	else
	{
		HPDF_Page_SetRGBFill(self->page, 
			self->palette[ink].r * shade + 1.0 * (1 - shade),
			self->palette[ink].g * shade + 1.0 * (1 - shade),
			self->palette[ink].b * shade + 1.0 * (1 - shade));
	}
}

static void pdf_fillarea(PDF_GIOS *self, gword count, gword *ptsin)
{
	int n;
	float x, y, xmin, ymin, xmax, ymax;

	if (count < 2) return;
	if (self->page == NULL)
	{
		pdf_newpage(self);
	}
	HPDF_Page_GSave(self->page);	/* Save graphics state */
	xmax = xmin = x = ptsin[0] / 10.0;
	ymax = ymin = y = ptsin[1] / 10.0;
	pdf_setupfill(self, self->writemode);

	HPDF_Page_MoveTo(self->page, x, y);
	for (n = 1; n < count; n++)
	{
		x = ptsin[n * 2] / 10.0;
		y = ptsin[n * 2 + 1] / 10.0;
		if (x < xmin) xmin = x;
		if (x > xmax) xmax = x;
		if (y < ymin) ymin = y;
		if (y > ymax) ymax = y;
		HPDF_Page_LineTo(self->page, x, y);
	}
	HPDF_Page_LineTo(self->page, ptsin[0] / 10.0, ptsin[1] / 10.0);
	switch (self->fillstyle)
	{
		case FILL_HOLLOW: 
			HPDF_Page_Stroke(self->page); 
			break;
		case FILL_SOLID:
		case FILL_HALFTONE: 
			HPDF_Page_EofillStroke(self->page);
			break;
		case FILL_HATCH:	
			HPDF_Page_Eoclip(self->page);	/* Clip to current shape */
			HPDF_Page_Stroke(self->page); /* Draw boundary */
			pdf_hatched_rect(self, xmin, ymin, xmax, ymax);
			break;
	}
	HPDF_Page_GRestore(self->page);
	self->dirty = 1;
}


static void pdf_cellarray(PDF_GIOS *self, gword *contrl, gword *ptsin, 
		gword count, gword *intin)
{
	float x0, y0, x1, y1, h, w;
	gword pitch, cols, rows;
	int x, y;
	gword ink;
	gword writemode;

	x0 = ptsin[0] / 10.0;
	y0 = ptsin[1] / 10.0;
	x1 = ptsin[2] / 10.0;
	y1 = ptsin[3] / 10.0;
	pitch = contrl[5];
	cols  = contrl[6];
	rows  = contrl[7];
	writemode = contrl[8];

	if (!cols || !rows) return;

	if (self->page == NULL)
	{
		pdf_newpage(self);
	}

	HPDF_Page_SetDash(self->page, NULL, 0, 0); 

	if (x0 > x1) { gword tmp = x1; x1 = x0; x0 = tmp; }
	if (y0 > y1) { gword tmp = y1; y1 = y0; y0 = tmp; }

	w = x1 - x0;
	h = y1 - y0;

	for (y = 0; y < rows; y++)
		for (x = 0; x < cols; x++)
	{
		ink = intin[(rows - 1 - y) * pitch + x];
		if (ink > MAXINK-1) ink = MAXINK-1;
		if (writemode == 4) ink = 0;

		HPDF_Page_SetRGBFill(self->page, self->palette[ink].r,
					self->palette[ink].g,
					self->palette[ink].b);
		HPDF_Page_Rectangle(self->page, x0 + (w * x / cols),
						y0 + (h * y / rows),
						w / cols,
						h / rows);
		HPDF_Page_Fill(self->page);
	}
	self->dirty = 1;
}


static void pdf_hatched_rect(PDF_GIOS *self, float x0, float y0, float x1, float y1)
{
	float x, y;
	float dy = fabs(y1-y0);

	switch (self->fillindex)
	{
		case 1:	/* Vertical lines */
			for (x = x0; x < x1; x += HATCHPITCH)
			{	
				HPDF_Page_MoveTo(self->page, x, y0);
				HPDF_Page_LineTo(self->page, x, y1);
				HPDF_Page_Stroke(self->page);
			}
			break;
		case 2:	/* Horizontal lines */
			for (y = y0; y < y1; y += HATCHPITCH)
			{	
				HPDF_Page_MoveTo(self->page, x0, y);
				HPDF_Page_LineTo(self->page, x1, y);
				HPDF_Page_Stroke(self->page);
			}
			break;
		case 3:	/* Diagonal lines */
			for (x = x0 - dy; x < x1; x += HATCHPITCH)
			{
				HPDF_Page_MoveTo(self->page, x, y0);
				HPDF_Page_LineTo(self->page, x + dy, y0 + dy);
				HPDF_Page_Stroke(self->page);
			}
			break;
		case 4:	/* Diagonal lines */
			for (x = x0; x < x1 + dy; x += HATCHPITCH)
			{
				HPDF_Page_MoveTo(self->page, x, y0);
				HPDF_Page_LineTo(self->page, x - dy, y0 + dy);
				HPDF_Page_Stroke(self->page);
			}
			break;
		case 5:	/* Horizontal and vertical lines */
			for (x = x0; x < x1; x += HATCHPITCH)
			{	
				HPDF_Page_MoveTo(self->page, x, y0);
				HPDF_Page_LineTo(self->page, x, y1);
				HPDF_Page_Stroke(self->page);
			}
			for (y = y0; y < y1; y += HATCHPITCH)
			{	
				HPDF_Page_MoveTo(self->page, x0, y);
				HPDF_Page_LineTo(self->page, x1, y);
				HPDF_Page_Stroke(self->page);
			}
			break;
		case 6:	/* Crisscross diagonal */
			for (x = x0 - dy; x < x1; x += HATCHPITCH)
			{
				HPDF_Page_MoveTo(self->page, x, y0);
				HPDF_Page_LineTo(self->page, x + dy, y0 + dy);
				HPDF_Page_Stroke(self->page);
			}
			for (x = x0; x < x1 + dy; x += HATCHPITCH)
			{
				HPDF_Page_MoveTo(self->page, x, y0);
				HPDF_Page_LineTo(self->page, x - dy, y0 + dy);
				HPDF_Page_Stroke(self->page);
			}
			break;
	}
}


static void pdf_bar(PDF_GIOS *self, gword *ptsin)
{
	float x0 = ptsin[0] / 10.0;
	float y0 = ptsin[1] / 10.0;
	float x1 = ptsin[2] / 10.0;
	float y1 = ptsin[3] / 10.0;

	if (self->page == NULL)
	{
		pdf_newpage(self);
	}

	HPDF_Page_GSave(self->page);	/* Save graphics state */
	if (self->writemode == MODE_REPLACE)
	{
		pdf_setupfill(self, MODE_ERASE);
		HPDF_Page_Rectangle(self->page, x0, y0, fabs(x1-x0), fabs(y1-y0));
		HPDF_Page_Eofill(self->page);
	}
	pdf_setupfill(self, self->writemode);
	HPDF_Page_Rectangle(self->page, x0, y0, fabs(x1-x0), fabs(y1-y0));
	switch (self->fillstyle)
	{
		case FILL_HOLLOW: 
			HPDF_Page_Stroke(self->page); 
			break;
		case FILL_SOLID:
		case FILL_HALFTONE: 
			HPDF_Page_EofillStroke(self->page);
			break;
		case FILL_HATCH:	
			HPDF_Page_Eoclip(self->page);	/* Clip to current shape */
			HPDF_Page_Stroke(self->page); /* Draw boundary */
			pdf_hatched_rect(self, x0, y0, x1, y1);
			break;
	}
	HPDF_Page_GRestore(self->page);
	self->dirty = 1;
}


static void pdf_arc(PDF_GIOS *self, gword *intin, gword *ptsin, int fill)
{
	float angle0 = intin[0] / 10.0;
	float angle1 = intin[1] / 10.0;
	float x0 = ptsin[0] / 10.0;
	float y0 = ptsin[1] / 10.0;
	float x1 = ptsin[2] / 10.0;
	float y1 = ptsin[3] / 10.0;
/*	float x2 = ptsin[4] / 10.0;
	float y2 = ptsin[5] / 10.0; */
	float radius = ptsin[6] / 10.0;

	if (angle1 < angle0) angle1 += 360;

	if (self->page == NULL)
	{
		pdf_newpage(self);
	}
	if (fill)
	{
		HPDF_Page_GSave(self->page);	/* Save graphics state */
		if (self->writemode == MODE_REPLACE)
		{
			pdf_setupfill(self, MODE_ERASE);
			HPDF_Page_MoveTo(self->page, x0, y0);
			HPDF_Page_Arc(self->page, x0, y0, radius, angle0, angle1);
			HPDF_Page_LineTo(self->page, x0, y0);
			HPDF_Page_Eofill(self->page);
		}
		pdf_setupfill(self, self->writemode);
		/* Start at the centre */
		HPDF_Page_MoveTo(self->page, x0, y0);
		/* Move to the start of the arc and draw the arc */
		HPDF_Page_Arc(self->page, x0, y0, radius, angle0, angle1);
		/* And close the pie slice */
		HPDF_Page_LineTo(self->page, x0, y0);
		switch (self->fillstyle)
		{
			case FILL_HOLLOW: 
				HPDF_Page_Stroke(self->page); 
				break;
			case FILL_SOLID:
			case FILL_HALFTONE: 
				HPDF_Page_EofillStroke(self->page);
				break;
			case FILL_HATCH:	
				HPDF_Page_Eoclip(self->page);	/* Clip to current shape */
				HPDF_Page_Stroke(self->page); /* Draw boundary */
				pdf_hatched_rect(self, x0 - radius, y0 - radius, x1 + radius, y1 + radius);
				break;
		}
		HPDF_Page_GRestore(self->page);	/* Restore graphics state */
	}
	else
	{
		if (self->writemode == MODE_REPLACE)
		{
			pdf_setupline(self, MODE_ERASE);
			HPDF_Page_SetDash(self->page, NULL, 0, 0);
			HPDF_Page_Arc(self->page, x0, y0, radius, angle0, angle1);
			HPDF_Page_Stroke(self->page);
		}
		pdf_setupline(self, self->writemode);
		HPDF_Page_Arc(self->page, x0, y0, radius, angle0, angle1);
		HPDF_Page_Stroke(self->page);
	}
	self->dirty = 1;
}


static void pdf_circle(PDF_GIOS *self, gword *ptsin)
{
	float x0 = ptsin[0] / 10.0;
	float y0 = ptsin[1] / 10.0;
	float radius = ptsin[4] / 10.0;

	if (self->page == NULL)
	{
		pdf_newpage(self);
	}

	HPDF_Page_GSave(self->page);	/* Save graphics state */
	if (self->writemode == MODE_REPLACE)
	{
		pdf_setupfill(self, MODE_ERASE);
		HPDF_Page_Circle(self->page, x0, y0, radius);
		HPDF_Page_Eofill(self->page);
	}
	pdf_setupfill(self, self->writemode);
	HPDF_Page_Circle(self->page, x0, y0, radius);
	switch (self->fillstyle)
	{
		case FILL_HOLLOW: 
			HPDF_Page_Stroke(self->page); 
			break;
		case FILL_SOLID:
		case FILL_HALFTONE: 
			HPDF_Page_EofillStroke(self->page);
			break;
		case FILL_HATCH:	
			HPDF_Page_Eoclip(self->page);	/* Clip to current shape */
			HPDF_Page_Stroke(self->page); /* Draw boundary */
			pdf_hatched_rect(self, x0 - radius, y0 - radius, x0 + radius, y0 + radius);
			break;
	}
	HPDF_Page_GRestore(self->page);	/* Save graphics state */
	self->dirty = 1;
}



static void pdf_gdp(PDF_GIOS *self, gword *contrl, 
		gword *intin, gword *ptsin, gword *intout, gword *ptsout)
{
	switch (contrl[SPECIAL])
	{
		case 1: pdf_bar(self, ptsin); break;
		case 2: pdf_arc(self, intin, ptsin, 0); break;
		case 3: pdf_arc(self, intin, ptsin, 1); break;
		case 4: pdf_circle(self, ptsin); break;
	}
}

static void  pdf_stheight(PDF_GIOS *self, gword height, gword *ptsout)
{	
	HPDF_Rect rect;
	HPDF_UINT capheight;
	HPDF_TextWidth capwidth;
	float size;

	if (height == 0) size = 10.0;
	else		 size = (height / 10.0);

	rect = HPDF_Font_GetBBox(self->font[self->textfont - 1]);
	capheight = HPDF_Font_GetCapHeight(self->font[self->textfont - 1]);
	capwidth = HPDF_Font_TextWidth(self->font[self->textfont - 1], (HPDF_BYTE *)"W", 1);

	ptsout[0] = (capwidth.width * size) / 100.0;
	ptsout[1] = (capheight * size) / 100.0;
	ptsout[2] = ((rect.right - rect.left) * size) / 100.0;
	ptsout[3] = ((rect.top - rect.bottom) * size) / 100.0;

	self->fontsize = size;
}

static gword pdf_stangle(PDF_GIOS *self, gword angle)
{
	angle = angle % 3600;

	self->textangle = angle;
	return angle;
}

static void pdf_setcolour(PDF_GIOS *self, gword *intin)
{
	if (intin[0] > MAXINK-1) return;

	self->palette[intin[0]].r = (intin[1] / 1000.0);
	self->palette[intin[0]].g = (intin[2] / 1000.0);
	self->palette[intin[0]].b = (intin[3] / 1000.0);
}

static void pdf_getcolour(PDF_GIOS *self, gword colour, gword *intout)
{
	if (colour > MAXINK-1) return;

	intout[0] = self->palette[colour].r * 1000.0;
	intout[1] = self->palette[colour].g * 1000.0;
	intout[2] = self->palette[colour].b * 1000.0;
}


static gword pdf_slstyle(PDF_GIOS *self, gword style)
{
	if (style < 1 || style > 5) style = 1;
	self->linestyle = style;
	return style;
}

static gword pdf_slwidth(PDF_GIOS *self, gword width)
{
	self->linewidth = width;
	return width;
}

static gword pdf_slcolour(PDF_GIOS *self, gword colour)
{
	if (colour > MAXINK-1) colour = MAXINK-1;
	self->linecolour = colour;
	return self->linecolour;
}


static gword pdf_smtype(PDF_GIOS *self, gword style)
{
	if (style < 1 || style > 6) style = 1;
	self->markstyle = style;
	return style;
}

static gword pdf_smheight(PDF_GIOS *self, gword height)
{
	self->markheight = height;
	return height;
}

static gword pdf_smcolour(PDF_GIOS *self, gword colour)
{
	if (colour > MAXINK-1) colour = MAXINK-1;
	self->markcolour = colour;
	return self->markcolour;
}


static gword pdf_stcolour(PDF_GIOS *self, gword colour)
{
	if (colour > MAXINK-1) colour = MAXINK-1;
	self->textcolour = colour;
	return self->textcolour;
}

static gword pdf_stfont(PDF_GIOS *self, gword font)
{
	if (font < 1 || font >= 5)
	{
		self->textfont = 1;
	}
	else
	{
		self->textfont = font;
	}
	return self->textfont;
}

static gword pdf_sfstyle(PDF_GIOS *self, gword style)
{
	if (style > 3) style = 1;
	self->fillstyle = style;
	return style;
}

static gword pdf_sfindex(PDF_GIOS *self, gword index)
{
	if (index < 1 || index > MAXHATCH) index = 1;
	self->fillindex = index;
	return index;
}

static gword pdf_sfcolour(PDF_GIOS *self, gword colour)
{
	if (colour > MAXINK-1) colour = MAXINK-1;
	self->fillcolour = colour;
	return self->fillcolour;
}

static gword pdf_swrmode(PDF_GIOS *self, gword mode)
{
	/* Can't do XOR, fall back to transparent */
	if (mode == 0 || mode > MODE_ERASE) mode = MODE_REPLACE;
	if (mode == MODE_XOR) mode = MODE_TRANS;
	self->writemode = mode;
	return mode;
}



#endif /* defined(HAVE_LIBHPDF) && defined(HAVE_HPDF_H) */

