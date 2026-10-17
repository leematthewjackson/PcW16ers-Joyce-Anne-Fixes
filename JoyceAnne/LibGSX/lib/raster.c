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

/* Because GSX uses 16-bit parameters, the maximum supported number of 
 * colours is 65536. If the display has more colours than that, you'd
 * use function 14 to map the 65536 GSX inks to available colours. 
 *
 * If you're trying to compile this under MSDOS you may have to reduce 
 * MAXCOLOUR to, say, 256 to avoid memory overflows. */
#define MAXCOLOUR 65536

/* GIOS device that draws on a RASTERSURFACE */

static void raster_destroy(GIOSDEVICE **ptr);
static int raster_dispatch(GIOSDEVICE *self, gword *contrl, 
		gword *intin, gword *ptsin, gword *intout, gword *ptsout);

typedef struct palette_entry
{
	unsigned ink;		/* Colour number to use writing to surface */
	gword	rq, gq, bq;	/* Requested RGB values */
	gword	ra, ga, ba;	/* Actual colour values */		
} PALETTE_ENTRY;

typedef struct raster_gios
{
	GIOSDEVICE vtable;
	RASTERSURFACE *surface;
	gword width;
	gword height;
	gword xpixel;
	gword ypixel;
	gword fonts;
	unsigned haspalette;
	unsigned maxcolours;

	/* Current drawing parameters */
	gword linestyle;
	gword linewidth;
	gword linepen;

	gword markheight;
	gword marktype;
	gword markpen;

	double textangle;
	unsigned textscale;
	unsigned textcharw;
	unsigned textbaseline;
	gword textpen;

	gword fillstyle;
	gword fillindex;
	gword fillpen;
	gword writemode;

	gword txtrows;
	gword txtcols;
	gword txtx;
	gword txty;
	gword gcurx;
	gword gcury;
	gword txtcurshown;
	gword txtreverse;

	GSXFONT txtfont;
	GSXFONT curfont;
	GSXFONT tsans;
	GSXFONT gsans;
	PALETTE_ENTRY *palette;

} RASTER_GIOS;




/* Implementations of specific GSX functions */
static int raster_opnwk(RASTER_GIOS *self, gword *contrl, 
		gword *intin, gword *ptsin, gword *intout, gword *ptsout);
static void raster_clswk(RASTER_GIOS *self);
static void raster_clrwk(RASTER_GIOS *self);
static void raster_updwk(RASTER_GIOS *self);
static void raster_escape(RASTER_GIOS *self, gword *contrl, gword *intin,
		gword *ptsin, gword *intout, gword *ptsout);
static void raster_polyline(RASTER_GIOS *self, gword count, gword *ptsin);
static void raster_polymarker(RASTER_GIOS *self, gword count, gword *ptsin);
static void raster_gtext(RASTER_GIOS *self, gword x, gword y, gword count,
				gword *text);
static void raster_fillarea(RASTER_GIOS *self, gword count, gword *ptsin);
static void raster_cellarray(RASTER_GIOS *self, gword *contrl, gword *ptsin, 
		gword count, gword *intin);
static void raster_gdp(RASTER_GIOS *self, gword *contrl, gword *intin,
		gword *ptsin, gword *intout, gword *ptsout);
static void raster_bar(RASTER_GIOS *self, gword *ptsin);
static void raster_arc(RASTER_GIOS *self, gword *intin, gword *ptsin, int fill);
static void raster_fillcircle(RASTER_GIOS *self, gword *intin, gword *ptsin);
static void  raster_setcolour(RASTER_GIOS *self, gword colour, gword r, gword g, gword b);
static void  raster_inqcolour(RASTER_GIOS *self, gword colour, gword opt, gword *result);
static gword raster_slstyle(RASTER_GIOS *self, gword style);
static gword raster_slwidth(RASTER_GIOS *self, gword width);
static gword raster_setpen(RASTER_GIOS *self, gword colour, gword *pen);
static gword raster_smtype(RASTER_GIOS *self, gword type);
static gword raster_smheight(RASTER_GIOS *self, gword height);
static void  raster_stheight(RASTER_GIOS *self, gword height, gword *ptsout);
static gword raster_stdirection(RASTER_GIOS *self, gword angle);
static gword raster_stfont(RASTER_GIOS *self, gword font);
static gword raster_sfstyle(RASTER_GIOS *self, gword style);
static gword raster_sfindex(RASTER_GIOS *self, gword style);
static void raster_readcellarray(RASTER_GIOS *self, gword *contrl, gword *ptsin, gword *intout);

static gword raster_swrmode(RASTER_GIOS *self, gword mode);


static unsigned char fill2[] = 
{
	0x11, 0x00, 0x44, 0x00, 0x11, 0x00, 0x44, 0x00,
	0x11, 0x44, 0x11, 0x44, 0x11, 0x44, 0x11, 0x44,
	0x55, 0x88, 0x55, 0x88, 0x55, 0x88, 0x55, 0x88,
	0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
	0x55, 0xEE, 0x55, 0xBB, 0x55, 0xEE, 0x55, 0xBB,
	0xDD, 0xEE, 0xDD, 0xBB, 0xDD, 0xEE, 0xDD, 0xBB,
	0xDD, 0xFF, 0xBB, 0xFF, 0xDD, 0xFF, 0xBB, 0xFF
};

static unsigned char fill3[] = 
{
	0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,	/* vertical lines */
	0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF,	/* horizontal lines */
	0x11, 0x22, 0x44, 0x88, 0x11, 0x22, 0x44, 0x88,	/* diagonal lines / */
	0x88, 0x44, 0x22, 0x11, 0x88, 0x44, 0x22, 0x11,	/* diagonal lines \ */
	0x44, 0x44, 0x44, 0xFF, 0x44, 0x44, 0x44, 0xFF,	/* crossed lines */
	0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81,	/* X pattern */
};



int gios_raster_create(PGDOS gdos, GIOSDEVICE **ptr, void *param)
{
	RASTER_GIOS *g = malloc(sizeof(RASTER_GIOS));

	if (!g) return -1;
	memset(g, 0, sizeof(*g));

	g->vtable.destroy  = raster_destroy;
	g->vtable.dispatch = raster_dispatch;
	g->surface = param;
	g->writemode = MODE_REPLACE;
	g->fonts = 0;
	g->palette = NULL;
	*ptr = &g->vtable;
	return 0;
}


static void raster_destroy(GIOSDEVICE **ptr)
{
	if (*ptr)
	{
		RASTER_GIOS *self = (RASTER_GIOS *)(*ptr);
		if (self->palette)
		{
			free(self->palette);
		}
/* Note: This does not destroy the associated RASTERSURFACE, because the 
 * lifespan of a GIOSDEVICE is shorter than that of a RASTERDEVICE */
		free(*ptr);
		*ptr = NULL;
	}
}


static int raster_dispatch(GIOSDEVICE *s, gword *contrl, gword *intin,
		gword *ptsin, gword *intout, gword *ptsout)
{
	RASTER_GIOS *self = (RASTER_GIOS *)s;
	int r;

	/* Default the output counts to 0 */
	contrl[NPTSOUT] = 0;
	contrl[NINTOUT] = 0;

	if (self->surface->dispatch)
	{
		r = (*self->surface->dispatch)(self->surface, s,
			contrl, intin, ptsin, intout, ptsout);
		if (r < 0) return r;
		if (r > 0) return 0;
	}

	switch (contrl[0])
	{
		case 1: return raster_opnwk(self, contrl, intin, ptsin, intout, ptsout);
		case 2: raster_clswk(self); break;
		case 3: raster_clrwk(self); break;
		case 4: raster_updwk(self); break;
		case 5:	raster_escape(self, contrl, intin, ptsin, intout, ptsout);
			break;
		case 6: raster_polyline(self, contrl[NPTSIN], ptsin);
			break;
		case 7: raster_polymarker(self, contrl[NPTSIN], ptsin);
			break;
		case 8: raster_gtext(self, ptsin[0], ptsin[1], contrl[NINTIN], intin); 
			break;
		case 9: raster_fillarea(self, contrl[NPTSIN], ptsin);
			break;
		case 10: raster_cellarray(self, contrl, ptsin, contrl[NINTIN], intin);
			 break;
		case 11: raster_gdp(self, contrl, intin, ptsin, intout, ptsout);
			 break;
		case 12: raster_stheight(self, ptsin[1], ptsout);
			 contrl[NPTSOUT] = 2;
			 break;
		case 13: intout[0] = raster_stdirection(self, intin[0]);
			 contrl[NINTOUT] = 1;
			 break;
		case 14: raster_setcolour(self, intin[0], intin[1], intin[2], intin[3]);
			 break;
		case 15: intout[0] = raster_slstyle(self, intin[0]);
			 contrl[NINTOUT] = 1;
			 break;
		case 16: ptsout[0] = raster_slwidth(self, ptsin[0]);
			 ptsout[1] = 0;
			 contrl[NPTSOUT] = 1;
			 break;
		case 17: intout[0] = raster_setpen(self, intin[0], &self->linepen);
			 contrl[NINTOUT] = 1;
			 break;
		case 18: intout[0] = raster_smtype(self, intin[0]);
			 break;
		case 19: ptsout[0] = 0;
			 ptsout[1] = raster_smheight(self, ptsin[1]);
			 contrl[NPTSOUT] = 1;
			 break;
		case 20: intout[0] = raster_setpen(self, intin[0], &self->markpen);
			 contrl[NINTOUT] = 1;
			 break;
		case 21: intout[0] = raster_stfont(self, intin[0]);
			 contrl[NINTOUT] = 1;
			 break;
		case 22: intout[0] = raster_setpen(self, intin[0], &self->textpen);
			 contrl[NINTOUT] = 1;
			 break;
		case 23: intout[0] = raster_sfstyle(self, intin[0]);
			 contrl[NINTOUT] = 1;
			 break;
		case 24: intout[0] = raster_sfindex(self, intin[0]);
			 contrl[NINTOUT] = 1;
			 break;
		case 25: intout[0] = raster_setpen(self, intin[0], &self->fillpen);
			 contrl[NINTOUT] = 1;
			 break;
		case 26: raster_inqcolour(self, intin[0], intin[1], intout);
			 contrl[NINTOUT] = 4;
			 break;
		case 27: raster_readcellarray(self, contrl, ptsin, intout);
			 break;
		case 32: intout[0] = raster_swrmode(self, intin[0]);
			 contrl[NINTOUT] = 1;
			 break;
		default: 
			fprintf(stderr, "raster_dispatch: unimplemented function %d\n", contrl[0]);
	}
	return 0;	
}


static int raster_opnwk(RASTER_GIOS *self, gword *contrl, 
		gword *intin, gword *ptsin, gword *intout, gword *ptsout)
{
	int n;

	/* Switch to graphics mode */
	if (*self->surface->enter_g)
	{
		(*self->surface->enter_g)(self->surface);
	}
	else
	{
		gword r, g, b;
		unsigned bg;
/* We haven't loaded the palette yet, so we have to ask the device for 
 * colour 0's RGB values and then convert to an ink */
		(*self->surface->get_ink_rgb)(self->surface, 0, &r, &g, &b);
		bg = (*self->surface->closest_rgb)(self->surface, r, g, b);

		(*self->surface->clear)(self->surface, bg);
	}
	contrl[NINTOUT] = 45;
	contrl[NPTSOUT] = 6;

	for (n = 0; n < 45; n++) intout[n] = 0;
	for (n = 0; n < 12; n++) ptsout[n] = 0;

	(*self->surface->dimensions)(self->surface, &self->width, &self->height);
	(*self->surface->pixelsize) (self->surface, &self->xpixel, &self->ypixel);

	/* Does this device have palette registers? */
	self->haspalette = (*self->surface->has_palette)(self->surface);
	self->maxcolours = (*self->surface->max_colours)(self->surface);



	/* Select the appropriate built-in font for this display size */
	if (self->height < 350)
	{
		self->gsans = font_gsans08;
		self->tsans = font_tsans08;
	}
	else if (self->height < 400)
	{
		self->gsans = font_gsans14;
		self->tsans = font_tsans14;
	}
	else
	{
		self->gsans = font_gsans16;
		self->tsans = font_tsans16;
	}
	intout[0] = self->width - 1;
	intout[1] = self->height - 1;
	intout[2] = 1;	/* Raster device, cannot continuously scale */
	intout[3] = self->xpixel;
	intout[4] = self->ypixel;
	intout[5] = 8;	/* Let's say 8 character sizes */
	intout[6] = 5;	/* Just the standard 5 line styles */
	intout[7] = 8;	/* Allow multiple line widths */
	intout[8] = 6;	/* Number of marker styles */
	intout[9] = 8;	/* Number of marker sizes */	
	if (self->surface->count_fonts)
	{
		self->fonts = (*self->surface->count_fonts)(self->surface);
	}
	else
	{
		self->fonts = 0;
	}
	intout[10] = self->fonts + 2;
	intout[11] = 6;	/* Number of patterns */
	intout[12] = 6;	/* Number of hatches */
	/* intout[13] is handled below when palette is initialised */
	intout[14] = 4;	/* Support 4 GDPs */
	intout[15] = 1;	/* Bar */
	intout[16] = 2;	/* Arc */
	intout[17] = 3;	/* Pie slice */
	intout[18] = 4;	/* Circle */
	for (n = 19; n <= 24; n++) intout[n] = 0xFFFF;
	intout[25] = 3;	/* Filled area */
	intout[26] = 0;	/* Line */
	intout[27] = 3;	/* Filled area */
	intout[28] = 3;	/* Filled area */
	for (n = 29; n <= 34; n++) intout[n] = 0xFFFF;
	intout[35] = (self->maxcolours > 2);
	intout[36] = 1;	/* Text rotation support */
	intout[37] = 1;	/* Filled area support */
	intout[38] = 1;	/* Can read cell arrays */
	intout[39] = (self->maxcolours > 32767) ? 0 : self->maxcolours;
	/* Input devices */
	intout[40] = 0;	/* No locators available */
	intout[41] = 0;	/* No valuators available */
	intout[42] = 0;	/* No choice devices available */
	intout[43] = 0;	/* No string devices available */
	intout[45] = 0;	/* Output only */

	/* Set up an initial palette. */
	if (self->maxcolours > MAXCOLOUR)
		self->maxcolours = MAXCOLOUR;
	if (self->maxcolours < 2)
		self->maxcolours = 2;

	if (self->haspalette)
	{
		intout[13] = self->haspalette;
	}
	else if (self->maxcolours > 32767)
	{
		intout[13] = 32767;
	}
	else
	{
		intout[13] = self->maxcolours;
	}
	if (self->palette)
	{
		free(self->palette);
	}	
	self->palette = calloc(self->maxcolours, sizeof(PALETTE_ENTRY));
	if (!self->palette)
		return -1;

	/* Start by getting the existing mapping for colour 0. */
	(*self->surface->get_ink_rgb)(self->surface, 0, 
		&self->palette[0].ra, &self->palette[0].ga, &self->palette[0].ba);
	/* See if this display is black on white or white on black */
	if (299 * self->palette[0].ra + 587 * self->palette[0].ga + 114 * self->palette[0].ba < 500000)
	{
		/* Background is black */
		raster_setcolour(self, 0, 0, 0, 0);
		if (self->maxcolours == 2)
		{
			raster_setcolour(self, 1, 1000, 1000, 1000);
		}
		else
		{
			raster_setcolour(self, 1, 1000,    0,    0); /* Red */	
			raster_setcolour(self, 2,    0, 1000,    0); /* Green */
			raster_setcolour(self, 3,    0,    0, 1000); /* Blue */	
			raster_setcolour(self, 4,    0, 1000, 1000); /* Cyan */	
			raster_setcolour(self, 5, 1000, 1000,    0); /* Yellow*/
			raster_setcolour(self, 6, 1000,    0, 1000); /* Magnta*/	
			/* Anything else to white */
			for (n = 7; n < self->maxcolours; n++)
				raster_setcolour(self, n, 1000, 1000, 1000);
		}
	}
	else
	{
		/* Background is white */
		raster_setcolour(self, 0, 1000, 1000, 1000);
		if (self->maxcolours == 2)
		{
			raster_setcolour(self, 1, 0, 0, 0);
		}
		else
		{
			raster_setcolour(self, 1, 1000,    0,    0); /* Red */	
			raster_setcolour(self, 2,    0, 1000,    0); /* Green */
			raster_setcolour(self, 3,    0,    0, 1000); /* Blue */	
			raster_setcolour(self, 4,    0, 1000, 1000); /* Cyan */	
			raster_setcolour(self, 5, 1000, 1000,    0); /* Yellow*/
			raster_setcolour(self, 6, 1000,    0, 1000); /* Magnta*/	
			/* Anything else to black */
			for (n = 7; n < self->maxcolours; n++)
				raster_setcolour(self, n, 0, 0, 0);
		}
	}

	/* Set line parameters */
	raster_slstyle(self, intin[1]);
	raster_slwidth(self, 1);
	raster_setpen(self, intin[2], &self->linepen);

	/* Set marker parameters */
	raster_smtype(self, intin[3]);
	raster_smheight(self, 1);
	raster_setpen(self, intin[4], &self->markpen);

	/* Set text parameters */
	self->textscale = 1;
	self->textangle = 0;
	raster_stfont(self, intin[5]);
	raster_setpen(self, intin[6], &self->textpen);
	/* Use that font as the text mode font */
	self->txtfont = self->curfont;
	self->txtrows = self->height / self->txtfont.height;
	self->txtcols = self->width  / self->txtfont.width;
	self->txtx = 0;
	self->txty = 0;
	self->gcurx = 0xFFFF;
	self->gcury = 0xFFFF;
	self->txtcurshown = 0;
	self->txtreverse = 0; 

	/* Set fill parameters */
	raster_sfstyle(self, intin[7]);
	raster_sfindex(self, intin[8]);
	raster_setpen(self, intin[9], &self->fillpen);

	/* Select the appropriate font */
	ptsout[0] = self->textcharw;		/* Min/max font sizes */
	ptsout[1] = self->textbaseline;
	ptsout[2] = ptsout[0] * 8;
	ptsout[3] = ptsout[1] * 8;
	ptsout[4] = 1;				/* Min line width */
	ptsout[5] = 0;
	ptsout[6] = 8;				/* Max line width */
	ptsout[7] = 0;
	ptsout[8] = 0;				/* Min marker size */
	ptsout[9] = 1;		
	ptsout[10] = 0;				/* Max marker size */
	ptsout[11] = 40;		

	self->writemode = MODE_REPLACE;
	return 0;
}


static void raster_clswk(RASTER_GIOS *self)
{
	raster_updwk(self);
	if (*self->surface->exit_g)
	{
		(*self->surface->exit_g)(self->surface);
	}
	else
	{
		raster_clrwk(self);
	}
	if (self->palette)
	{
		free(self->palette);
		self->palette = NULL;
	}
}

static void raster_clrwk(RASTER_GIOS *self)
{
	(*self->surface->clear)(self->surface, self->palette[0].ink);
}


static void raster_updwk(RASTER_GIOS *self)
{
	if (self->surface->flush)
	{
		(*self->surface->flush)(self->surface, self->palette[0].ink);
	}
}

static void raster_plot(RASTER_GIOS *self, int x0, int y0, gword ink, 
				unsigned fg, unsigned bg, gword writemode)
{
	int x = x0;
	int y = self->height - 1 - y0;
	unsigned pix;

	if (x < 0 || y < 0 || x >= self->width || y >= self->height) return;
	switch (writemode)
	{
		case MODE_REPLACE:
			if (ink) (*self->surface->setpixel)(self->surface, x, y, fg);
			else	 (*self->surface->setpixel)(self->surface, x, y, bg);
			break;
		case MODE_TRANS:
			if (ink) (*self->surface->setpixel)(self->surface, x, y, fg);
			break;
		case MODE_XOR:
			if (ink) 
			{
				pix = (*self->surface->getpixel)(self->surface, x, y);
				(*self->surface->setpixel)(self->surface, x, y, fg ^ pix);
			}
			break;
		case MODE_ERASE:
			if (ink) (*self->surface->setpixel)(self->surface, x, y, bg);
			break;
	}
}


/* Draw a line (Bresenham's algorithm) from (x0,y0) to (x1,y1) */
static void raster_drawline(RASTER_GIOS *self, int x0, int y0, int x1, int y1, int lastpt, gword *pattern, unsigned linecolour, gword linewidth)
{
	int dx, dy, sx, sy, e1, e2, x, y, i, j, half_thickness;
	int startOffset, draw;

	dx = abs((int)x1 - (int)x0);
	dy = abs((int)y1 - (int)y0);
	sx = (x0 < x1) ? 1 : -1;
	sy = (y0 < y1) ? 1 : -1;
	e1 = dx - dy;
	half_thickness = (linewidth) / 2;
	startOffset = 0;

	if ((linewidth % 2) == 0)
	{
		startOffset = -(half_thickness - 1);
	}
	else
	{
		startOffset = -half_thickness;
	}
	x = x0;
	y = y0;
	while (1)
	{
		/* If we have reached the last point and are not drawing the
		 * last point because another line will follow, then bail */
		if (x == x1 && y == y1 && lastpt == 0) break;

		/* Get the top bit of the pattern mask to determine whether
		 * to draw in FG or BG colour */
		draw = ((*pattern) & 0x8000) ? 1 : 0;

		/* Rotate the pattern left by 1 */
		*pattern = ((*pattern) << 1) | draw;

		for (i = startOffset; i <= half_thickness; i++)
		{
			for (j = startOffset; j <= half_thickness; j++)
			{
				raster_plot(self, x + i, y + j, draw,
					linecolour, self->palette[0].ink, 
					self->writemode);
			}
		}
		if (x == x1 && y == y1) break;
		e2 = e1 * 2;
		if (e2 > -dy)
		{
			e1 -= dy;
			x += sx;	
		}
		if (e2 < dx)
		{
			e1 += dx;
			y += sy;
		}
	}
}

/* Based on <https://zingl.github.io/bresenham.c> */
static void raster_circle(RASTER_GIOS *self, gword xc, gword yc, gword radius, 
		gword *pattern, unsigned colour)
{
	int x = -radius, y = 0, err = 2 - 2*radius;
	int draw;
	int r;

	do
	{
		draw = ((*pattern) & 0x8000) ? 1 : 0;
		(*pattern) = ((*pattern) << 1) | draw;
		raster_plot(self, xc - x, yc + y, draw,
			colour, self->palette[0].ink, self->writemode);
		raster_plot(self, xc - y, yc - x, draw,
			colour, self->palette[0].ink, self->writemode);
		raster_plot(self, xc + x, yc - y, draw,
			colour, self->palette[0].ink, self->writemode);
		raster_plot(self, xc + y, yc + x, draw,
			colour, self->palette[0].ink, self->writemode);
		r = err;
		if (r <= y) err += ((++y)*2+1);
		if (r > x || err > y)
			err += (++x)*2+1;
	}
	while (x < 0);
}


static void raster_xorcur(RASTER_GIOS *self)
{
	gword x = self->txtx * self->txtfont.width;
	gword y = self->txty * self->txtfont.height;
	int r, c;
	unsigned pix;

	for (r = 0; r < self->txtfont.height; r++)
		for (c = 0; c < self->txtfont.width; c++)
	{
		pix = (*self->surface->getpixel)(self->surface, x + c, y + r);
		(*self->surface->setpixel)(self->surface, x + c, y + r, ~pix);
	}
}

static void raster_cursorto(RASTER_GIOS *self, int newx, int newy)
{
	if (self->txtcurshown) 
	{
		raster_xorcur(self);
	}
	if (newx < 0) newx = 0;
	if (newy < 0) newy = 0;
	if (newx >= self->txtcols - 1) newx = self->txtcols - 1;
	if (newy >= self->txtrows - 1) newy = self->txtrows - 1;

	self->txtx = newx;
	self->txty = newy;
	if (self->txtcurshown) 
	{
		raster_xorcur(self);
	}
}


static void raster_movecur(RASTER_GIOS *self, int dx, int dy)
{
	int newx = self->txtx + dx;
	int newy = self->txty + dy;

	raster_cursorto(self, newx, newy);
}

static void raster_cleareol(RASTER_GIOS *self)
{
	int y, x;

	if (self->txtcurshown) 
	{
		raster_xorcur(self);
	}

	for (y = (self->txty) * self->txtfont.height; 
	     y < (self->txty + 1) * self->txtfont.height; y++)
	{
		for (x = self->txtx * self->txtfont.width; x < self->width; x++)
			 (*self->surface->setpixel)(self->surface, x, y, self->palette[0].ink);
	}

	if (self->txtcurshown) 
	{
		raster_xorcur(self);
	}
}


static void raster_cleareos(RASTER_GIOS *self)
{
	int y, x;

	raster_cleareol(self);
	if (self->txtcurshown) 
	{
		raster_xorcur(self);
	}
	for (y = (self->txty + 1) * self->txtfont.height; 
	     y < self->height; y++)
	{
		for (x = 0; x < self->width; x++)
			 (*self->surface->setpixel)(self->surface, x, y, self->palette[0].ink);
	}
	if (self->txtcurshown) 
	{
		raster_xorcur(self);
	}
}


static void raster_drawtext(RASTER_GIOS *self, gword count, gword *txt)
{
	int n, p, q, pitch;
	unsigned char mask;
	unsigned char *row;
	int pix;
	unsigned fg, bg;

	if (self->txtreverse)
	{
		fg = self->palette[0].ink;
		bg = self->palette[self->textpen].ink;
	}
	else
	{
		fg = self->palette[self->textpen].ink;
		bg = self->palette[0].ink;
	}

	if (self->txtcurshown) 
	{
		raster_xorcur(self);
	}
	pitch = (self->txtfont.width + 7) / 8;
	for (n = 0; n < count; n++)
	{
		gword x = self->txtx * self->txtfont.width;
		gword y = (self->height - 1 - self->txty * self->txtfont.height);

		if (txt[n] < self->txtfont.length)
		{
			for (p = 0; p < self->txtfont.height; p++)
			{
				row = &self->txtfont.bits[txt[n] * self->txtfont.cellsize + p * pitch];
				for (mask = 0x80, q = 0; q < self->curfont.width; q++)
				{
					pix = (row[q >> 3] & mask) ? 1 : 0;
	
					raster_plot(self, x + q, y - p, pix,
						fg, bg, MODE_REPLACE);
					mask = mask >> 1;
					if (mask == 0) mask = 0x80;
				}
			}	
		}

		++self->txtx;
		if (self->txtx >= self->txtcols)
		{
			self->txtx = 0;
			++self->txty;
			if (self->txty >= self->txtrows)
			{
				self->txty = 0;
			}
		}			
	}

	if (self->txtcurshown) 
	{
		raster_xorcur(self);
	}
}


static void raster_xor_gcur(RASTER_GIOS *self, int x, int y)
{
	int n;
	unsigned pix;

	/* Switch to screen coordinates */	
	y = self->height - y - 1;
	
	for (n = x - 10; n < x + 10; n++)
	{
		if (n < 0 || n >= self->width || y < 0 || y >= self->height)
			continue;

		pix = (*self->surface->getpixel)(self->surface, n, y);
		(*self->surface->setpixel)(self->surface, n, y, ~pix);
	}
	for (n = y - 10; n < y + 10; n++)
	{
		if (x < 0 || x >= self->width || n < 0 || n >= self->height)
			continue;

		pix = (*self->surface->getpixel)(self->surface, x, n);
		(*self->surface->setpixel)(self->surface, x, n, ~pix);
	}
}



/* This is a default implementation of the text mode 'on top of' the 
 * graphics mode */
static void raster_escape(RASTER_GIOS *self, gword *contrl, gword *intin,
		gword *ptsin, gword *intout, gword *ptsout)
{
	switch(contrl[5])
	{
		case 1:	/* Text mode size */
		intout[0] = self->txtrows;
		intout[1] = self->txtcols;
		contrl[NINTOUT] = 2;
		break;

		case 2: /* Enter graphics mode */
		self->gcurx = 0xFFFF;
		self->gcury = 0xFFFF;
		raster_clrwk(self);
		break;	

		case 3: /* Enter text mode */
		raster_clrwk(self);
		self->txtcurshown = 1;
		self->gcurx = 0xFFFF;
		self->gcury = 0xFFFF;
		raster_xorcur(self);
		break;	

		case 4:	raster_movecur(self, 0, -1); break;
		case 5:	raster_movecur(self, 0,  1); break;
		case 6:	raster_movecur(self, 1,  0); break;
		case 7:	raster_movecur(self, -1, 0); break;
		case 8: raster_cursorto(self, 0, 0); break;
		case 9: raster_cleareos(self); break;
		case 10: raster_cleareol(self); break;
		case 11: raster_cursorto(self, intin[1]-1, intin[0]-1); break;
		case 12: raster_drawtext(self, contrl[NINTIN], intin); break;
		case 13: self->txtreverse = 1; break;
		case 14: self->txtreverse = 0; break;
		case 15: intout[0] = self->txty + 1; 
			 intout[1] = self->txtx + 1;
			 contrl[NINTOUT] = 2;
			 break;
		case 16: intout[0] = 0;	/* No tablet */
			 contrl[NINTOUT] = 1;
			 break;
/* Not implemented: 17 (screen dump) */
		case 18: /* Draw graphic cursor */
			raster_xor_gcur(self, ptsin[0], ptsin[1]);
			self->gcurx = ptsin[0];
			self->gcury = ptsin[1];
			break;
		case 19: /* Remove graphic cursor */
			if (self->gcurx != 0xFFFF && self->gcury != 0xFFFF)
			{
				raster_xor_gcur(self, self->gcurx, self->gcury);
				self->gcurx = 0xFFFF;
				self->gcury = 0xFFFF;
			}
			break;	
	}	

}





static void raster_polyline(RASTER_GIOS *self, gword count, gword *ptsin)
{
	gword n;
	gword ls = self->linestyle;

	if (count < 2) return;

	for (n = 0; n < (count-1); n++)
	{
		raster_drawline(self, (signed short)(ptsin[n*2]), 
				      (signed short)(ptsin[n*2+1]), 
				      (signed short)(ptsin[n*2+2]),
				      (signed short)(ptsin[n*2+3]), 
					(n == (count - 2)), &ls, 
			self->palette[self->linepen].ink, self->linewidth);
	}	
}

static void raster_drawmarker(RASTER_GIOS *self, int x, int y)
{
	gword ls = 0xFFFF;
	int markwidth = (self->markheight * self->ypixel) / self->xpixel;
	unsigned markcolour = self->palette[self->markpen].ink;

	switch(self->marktype)
	{
		case 1: /* dot */
			raster_plot(self, x, y, 1, markcolour,
					self->palette[0].ink, self->writemode);
			break;

		case 2: /* plus */
			raster_drawline(self, x - markwidth, y,
					      x + markwidth, y,
						1, &ls, markcolour, 1);
/* Draw the vertical line as two phases that don't touch the centre, so 
  that each pixel is only drawn once */
			raster_drawline(self, x, y - self->markheight,
					      x, y,
						0, &ls, markcolour, 1);
			raster_drawline(self, x, y + self->markheight,
					      x, y,
						0, &ls, markcolour, 1);
			break; 
		case 3: /* asterisk */
			raster_drawline(self, x - markwidth, y,
					      x + markwidth, y,
						1, &ls, markcolour, 1);
/* Draw the vertical line as two phases that don't touch the centre, so 
  that each pixel is only drawn once */
			raster_drawline(self, x - (markwidth / 2), y - self->markheight,
					      x, y,
						0, &ls, markcolour, 1);
			raster_drawline(self, x + (markwidth / 2), y - self->markheight,
					      x, y,
						0, &ls, markcolour, 1);
			raster_drawline(self, x - (markwidth / 2), y + self->markheight,
					      x, y,
						0, &ls, markcolour, 1);
			raster_drawline(self, x + (markwidth / 2), y + self->markheight,
					      x, y,
						0, &ls, markcolour, 1);
			break; 
		case 4: /* circle */
			raster_circle(self, x, y, self->markheight, &ls, markcolour);
			break;		
		case 5: /* diamond */
			raster_drawline(self, x - markwidth, y,
					      x, y + self->markheight,
						0, &ls, markcolour, 1);
			raster_drawline(self, x, y + self->markheight,
					      x + markwidth, y,
						0, &ls, markcolour, 1);
			raster_drawline(self, x + markwidth, y,
					      x, y - self->markheight,
						0, &ls, markcolour, 1);
			raster_drawline(self, x, y - self->markheight,
					      x - markwidth, y,
						0, &ls, markcolour, 1);
			break;
		case 6: /* square */
			raster_drawline(self, x - markwidth, y - self->markheight,
					      x + markwidth, y - self->markheight,
						0, &ls, markcolour, 1);
			raster_drawline(self, x + markwidth, y - self->markheight,
					      x + markwidth, y + self->markheight,
						0, &ls, markcolour, 1);
			raster_drawline(self, x + markwidth, y + self->markheight,
					      x - markwidth, y + self->markheight,
						0, &ls, markcolour, 1);
			raster_drawline(self, x - markwidth, y + self->markheight,
					      x - markwidth, y - self->markheight,
						0, &ls, markcolour, 1);

			break;

	}
}




static void raster_polymarker(RASTER_GIOS *self, gword count, gword *ptsin)
{
	gword n;

	for (n = 0; n < count; n++)
	{
		raster_drawmarker(self, ptsin[n*2], ptsin[n*2+1]);
	}	
}





static void raster_gtext(RASTER_GIOS *self, gword x0, gword y0, gword count,
				gword *text)
{
	unsigned char mask;
	int pitch = (self->curfont.width + 7) / 8;
	int baseline = self->textbaseline;
	int pix, p, q, r, s;
	double dcos, dsin, dx, dy, xnew, ynew;
	int x = 0, y = 0;
	int x1, y1;
	unsigned char *bitmap, *row;

	dsin = sin(self->textangle);
	dcos = cos(self->textangle);
	while (count)
	{
		gword ch = *text++; 
		--count;

		if (ch > self->curfont.length) 
		{
			x += self->curfont.width * self->textscale;
			continue;
		}
		bitmap = self->curfont.bits + ch * self->curfont.cellsize;
		for (p = 0; p < self->curfont.height; p++)
		{
			row = bitmap + p * pitch;
			y1 = y - (self->textscale * baseline) + (self->textscale * p);
			for (mask = 0x80, q = 0; q < self->curfont.width; q++)
			{
				x1 = x + (self->textscale * q);
				pix = (row[q >> 3] & mask) ? 1 : 0;

				for (r = 0; r < self->textscale; r++)
					for (s = 0; s < self->textscale; s++)
				{
					dx = x1 + s;
					dy = y1 + r;
					xnew = dx * dcos + dy * dsin;
					ynew = dx * (-dsin) + dy * dcos;

					raster_plot(self, x0 + xnew, y0 - ynew, pix, self->palette[self->textpen].ink, self->palette[0].ink, self->writemode);
				}
				mask = mask >> 1;
				if (mask == 0) mask = 0x80;
			}	
		}	
		x += self->curfont.width * self->textscale;
	}
}

static int compare_int(const void *a, const void *b)
{
	return (*(const int *)a) - (*(const int *)b);
}

static void raster_fill_line(RASTER_GIOS *self, int y, int x0, int x1)
{
	gword pattern;
	int shift;
	int x;
	int draw;
	
	if (x0 > x1)	/* which should never happen */
	{
		gword tmp = x0; x0 = x1; x1 = tmp;
	}
	if (x0 < 0) x0 = 0;
	if (x1 >= self->width) x1 = self->width - 1;
	if (y < 0 || y >= self->height) return;

	switch (self->fillstyle)
	{
		default:
		case FILL_HOLLOW: 
			pattern = 0; 
			break;
		case FILL_SOLID: 
			pattern = 0xFFFF; 
			break;
		case FILL_HALFTONE: 
			pattern = fill2[((self->fillindex - 1) << 3) + (y & 7)];
			pattern = (pattern << 8) | pattern;
			break;
		case FILL_HATCH:
			pattern = fill3[((self->fillindex - 1) << 3) + (y & 7)];
			pattern = (pattern << 8) | pattern;
			break;
	}
	shift = x0 & 7;
	if (shift)
	{
		pattern = (pattern << shift) | ((pattern << shift) >> 16);
	}


	for (x = x0; x <= x1; x++)
	{
		draw = (pattern & 0x8000) ? 1 : 0;
		pattern = (pattern << 1) | draw;
		raster_plot(self, x, y, draw, self->palette[self->fillpen].ink, self->palette[0].ink, self->writemode);
	}
}


static void raster_fillarea(RASTER_GIOS *self, gword count, gword *ptsin)
{
	int n, y;
	gword ls = 0xFFFF;
	int maxy = -1, miny = 0x7FFFFFFF;
	int *intersect;
	int icount;
	int x0, y0, x1, y1;
	unsigned fillcolour = self->palette[self->fillpen].ink;

	if (count < 2) return;	/* Polygon must have at least 2 points */

	/* We can have up to 2 intersections per line segment */
 	intersect = calloc(count * 2, sizeof(int));
	if (intersect)	/* If we've no memory for intersect, fall back to */
	{		/* drawing the outline */
		/* Work out how many scanlines there are */
		for (n = 0; n < count; n++)
		{
			y = (signed short)(ptsin[n*2+1]);

			if (y >= self->height) y = self->height;

			if (y < miny) miny = y;
			if (y > maxy) maxy = y;
		}
		for (y = miny; y <= maxy; y++)
		{
/* Calculate the intersections of each line segment with this scanline */
			icount = 0;
			for (n = 0; n < count; n++)
			{
				x0 = (signed short)(ptsin[n * 2]);
				y0 = (signed short)(ptsin[n * 2 + 1]);
				if (n < (count - 1))
				{
					x1 = (signed short)(ptsin[n * 2 + 2]);
					y1 = (signed short)(ptsin[n * 2 + 3]);
				}	
				else
				{
					x1 = (signed short)(ptsin[0]);
					y1 = (signed short)(ptsin[1]);
				}
				if (y0 == y && y1 == y)
				{
/* Horizontal line. Two intersections, one at each end */
					intersect[icount++] = x0;
					intersect[icount++] = x1;
				}
				else if (y0 < y && y1 >= y)
				{
					intersect[icount++] = ((x1-x0)*(y-y0))/(y1-y0) + x0;
				}
				else if (y0 >= y && y1 < y)
				{
					intersect[icount++] = ((x1-x0)*(y-y0))/(y1-y0) + x0;
				}
			}
			if (icount < 2) continue; 
			qsort(intersect, icount, sizeof(int), compare_int);
			for (n = 0; n < (icount / 2); n++)
			{
				raster_fill_line(self, y, intersect[n*2], intersect[n*2+1]);
			}
		}
		free(intersect);
		if (self->fillstyle == FILL_SOLID && self->writemode == MODE_XOR)
		{
/* If this is a solid fill in XOR mode don't draw the border as it will
 * clash with the shape just drawn. (It'll also clash with patterned fills,
 * but not so noticeably */
			return;
		}
	}
	/* Outline the polygon in the current fill colour */
	for (n = 0; n < (count-1); n++)
	{
		raster_drawline(self, (signed short)(ptsin[n*2]), 
				      (signed short)(ptsin[n*2+1]), 
				      (signed short)(ptsin[n*2+2]),
				      (signed short)(ptsin[n*2+3]), 0, &ls, 
					fillcolour, 1);
	}	
	n = count - 1;
	raster_drawline(self, (signed short)(ptsin[n*2]),
				(signed short)(ptsin[n*2+1]), 
				(signed short)(ptsin[0]),
				(signed short)(ptsin[1]), 0, &ls,
				fillcolour, 1);
}

static void raster_cellarray(RASTER_GIOS *self, gword *contrl, gword *ptsin, 
		gword count, gword *intin)
{
	gword x0, y0, x1, y1, h, w;
	gword pitch, cols, rows, writemode;
	int x, y, r, c;
	gword ink;

	x0 = ptsin[0];
	y0 = ptsin[1];
	x1 = ptsin[2];
	y1 = ptsin[3];
	pitch = contrl[5];
	cols  = contrl[6];
	rows  = contrl[7];
	writemode = contrl[8];

	if (x0 > x1) { gword tmp = x1; x1 = x0; x0 = tmp; }
	if (y0 > y1) { gword tmp = y1; y1 = y0; y0 = tmp; }

	h = y1 - y0;
	w = x1 - x0;

	for (y = y0; y <= y1; y++)
	{
		r = ( (h - (y-y0)) * rows / h);
		if (r >= rows) r = rows - 1;
		for (x = x0; x <= x1; x++)	
		{
			c = ((x - x0) * cols / w);
			if (c >= cols) c = cols - 1;
			ink = intin[r * pitch + c];
				
			raster_plot(self, x, y, ink != 0, 
				self->palette[ink].ink, self->palette[0].ink, 
				writemode);
		}
	}
}



static void raster_gdp(RASTER_GIOS *self, gword *contrl, gword *intin,
		gword *ptsin, gword *intout, gword *ptsout)
{
	switch (contrl[SPECIAL])
	{
		case 1: raster_bar(self, ptsin); break;
 		case 2:	raster_arc(self, intin, ptsin, 0); break;
 		case 3:	raster_arc(self, intin, ptsin, 1); break;
 		case 4:	raster_fillcircle(self, intin, ptsin); break;
	}
}


static void raster_bar(RASTER_GIOS *self, gword *ptsin)
{
	gword x0 = ptsin[0];
	gword y0 = ptsin[1];
	gword x1 = ptsin[2];
	gword y1 = ptsin[3];
	gword t;
	gword x, y;
	unsigned fillcolour = self->palette[self->fillpen].ink;

	if (x0 > x1)
	{
		t = x1; x1 = x0; x0 = t;
	}
	if (y0 > y1)
	{
		t = y1; y1 = y0; y0 = t;
	}
	for (y = y0; y <= y1; y++)
	{
		raster_fill_line(self, y, x0, x1);
	}	
	if (self->fillstyle != 1)	/* Border needed */
	{
		for (y = y0; y <= y1; y++)
		{
			raster_plot(self, x0, y, 1, fillcolour, 
					self->palette[0].ink, self->writemode);
			raster_plot(self, x1, y, 1, fillcolour, 
					self->palette[0].ink, self->writemode);
		}
		for (x = x0; x <= x1; x++)
		{
			raster_plot(self, x, y0, 1, fillcolour, 
					self->palette[0].ink, self->writemode);
			raster_plot(self, x, y1, 1, fillcolour, 
					self->palette[0].ink, self->writemode);
		}
	}
}

static void raster_arc(RASTER_GIOS *self, gword *intin, gword *ptsin, int fill)
{
/* Generate an approximation to the shape */
	gword maxpts = 20 * (self->width + self->height + 10);
	signed short *shape = calloc(maxpts, sizeof(gword));
	int nshp = 0;
	int angle;
	double dangle;
	int x, y, xlast, ylast;
	int xrad, yrad;
	int angle0, angle1;

	/* Radius in the X axis is as passed in */
	xrad = ptsin[6];
	/* Radius in the Y axis is according to aspect ratio */
	yrad = (ptsin[6] * self->xpixel) / self->ypixel;

	if (!shape) return;
	/* If filling, line from centre to start of arc */
	if (fill)
	{
		shape[nshp++] = ptsin[0];
		shape[nshp++] = ptsin[1];
	}
	/* Draw the start point */
	angle0 = intin[0] % 3600;
	angle1 = intin[1] % 3600;
	dangle = (angle0 * M_PI / 1800);
	shape[nshp++] = xlast = ptsin[0] + xrad * sin(dangle);
	shape[nshp++] = ylast = ptsin[1] + yrad * cos(dangle);

	for (angle = angle0; angle != angle1; angle = (angle + 1) % 3600)
	{
		dangle = (angle * M_PI / 1800);

		x = ptsin[0] + xrad * sin(dangle);
		y = ptsin[1] + yrad * cos(dangle);

/* Clipping should be done by raster_fill_line
		if (x < 0) x = 0; if (x >= self->width)  x = self->width - 1;
		if (y < 0) y = 0; if (y >= self->height) y = self->height - 1;
*/
		if (x != xlast || y != ylast)
		{
			shape[nshp++] = x;
			shape[nshp++] = y;
			ylast = y;	
			xlast = x;	
		}
		if (nshp + 2 > maxpts) break;
	}
	if (fill)	
	{
		raster_fillarea(self, nshp / 2, (gword *)shape);
	}
	else
	{
		raster_polyline(self, nshp / 2, (gword *)shape);
	}

	free(shape);
}


/* There's probably a better way to do this using the Bresenham circle
 * algorithm. But I'm just going to generate a polygon that looks 
 * something like a circle using sin() and cos(), and hand off to 
 * v_fillarea */
static void raster_fillcircle(RASTER_GIOS *self, gword *intin, gword *ptsin)
{
	gword maxpts = 20 * (self->width + self->height + 10);
	signed short *shape = calloc(maxpts, sizeof(gword));
	int nshp = 0;
	int angle;
	double dangle;
	int x, y, xlast, ylast;
	int xrad, yrad;

	/* Radius in the X axis is as passed in */
	xrad = ptsin[4];
	/* Radius in the Y axis is according to aspect ratio */
	yrad = (ptsin[4] * self->xpixel) / self->ypixel;

	if (!shape) return;
	dangle = 0;
	shape[nshp++] = xlast = ptsin[0] + xrad * sin(dangle);
	shape[nshp++] = ylast = ptsin[1] + yrad * cos(dangle);

	for (angle = 1; angle < 3600; angle++)
	{
		dangle = (angle * M_PI / 1800);

		x = ptsin[0] + xrad * sin(dangle);
		y = ptsin[1] + yrad * cos(dangle);

/* Clipping should be done by raster_fill_line
 		if (x < 0) x = 0; if (x >= self->width)  x = self->width - 1;
		if (y < 0) y = 0; if (y >= self->height) y = self->height - 1;
*/
		if (x != xlast || y != ylast)
		{
			shape[nshp++] = x;
			shape[nshp++] = y;
			ylast = y;	
			xlast = x;	
		}
		if (nshp + 2 > maxpts) break;
	}
	raster_fillarea(self, nshp / 2, (gword *)shape);
	free(shape);
}





static void raster_text_baseline(RASTER_GIOS *self)
{
	unsigned char *w = &self->curfont.bits[self->curfont.cellsize * 'W'];
	gword pitch = (self->curfont.width + 7) / 8;
	int n, m;
	int charh = -1;
	int charw = -1;
	unsigned char mask;

	/* Scan the 'W' character and get its maximum width and height */
	for (n = 0; n < self->curfont.height; n++)
	{
		mask = 0x80;
		for (m = 0; m < self->curfont.width; m++)
		{
			if ((w[n * pitch + (m >> 3)]) & mask)
			{
				if (n > charh) charh = n;
				if (m > charw) charw = m;
			}
			mask = mask >> 1;
			if (mask == 0) mask = 0x80;
		}	
	}
	self->textbaseline = charh;
	self->textcharw    = charw;

}

static void raster_stheight(RASTER_GIOS *self, gword height, gword *ptsout)
{
	unsigned scale;

	/* Locate character W */
	raster_text_baseline(self);

	scale = (height / self->curfont.height);
	if (scale < 1) scale = 1;
	self->textscale = scale;

	ptsout[0] = self->textcharw * scale;
	ptsout[1] = self->textbaseline * scale;
	ptsout[2] = self->curfont.width * scale;
	ptsout[3] = self->curfont.height * scale;
}

static gword raster_stdirection(RASTER_GIOS *self, gword angle)
{
	angle %= 3600;

	self->textangle = (angle * M_PI / 1800);
	return angle;
}


static void raster_setcolour(RASTER_GIOS *self, gword colour, gword r, gword g, gword b)
{
	if (colour >= self->maxcolours)	return;

	/* Save the requested colours */
	self->palette[colour].rq = r;
	self->palette[colour].gq = g;
	self->palette[colour].bq = b;

	if (self->haspalette)
	{
		self->palette[colour].ink = colour;
		(*self->surface->set_ink_rgb)(self->surface, colour, r, g, b);
	}
	else
	{
		self->palette[colour].ink = 
			(*self->surface->closest_rgb)(self->surface, r, g, b);
	}
	(*self->surface->get_ink_rgb)(self->surface, self->palette[colour].ink, 
		&self->palette[colour].ra, &self->palette[colour].ga, 
		&self->palette[colour].ba);
}

static void  raster_inqcolour(RASTER_GIOS *self, gword colour, gword opt, gword *result)
{
	if (colour >= self->maxcolours)	colour = self->maxcolours - 1;
	result[0] = colour;
	if (opt)
	{
		result[1] = self->palette[colour].ra;
		result[2] = self->palette[colour].ga;
		result[3] = self->palette[colour].ba;
	}
	else
	{
		result[1] = self->palette[colour].rq;
		result[2] = self->palette[colour].gq;
		result[3] = self->palette[colour].bq;
	}
}



static gword raster_slstyle(RASTER_GIOS *self, gword style)
{
	static gword l_styles[] = 
	{
		0xFFFF,
		0xFF00,
		0xE0E0,
		0xFE38,
		0xFFF0
	};

	if (style == 0 || style > 5)
	{
		style = 1;
	}
	self->linestyle = l_styles[style - 1];
	return style;
}



static gword raster_slwidth(RASTER_GIOS *self, gword width)
{
	if (width < 1) width = 1;
	self->linewidth = width;
	return width;
}

static gword raster_setpen(RASTER_GIOS *self, gword colour, gword *pen)
{
	if (colour >= self->maxcolours) 
		colour = self->maxcolours - 1;
	*pen = colour;
	return colour;
}


static gword raster_smtype(RASTER_GIOS *self, gword type)
{
	if (type == 0 || type > 6) type = 3;

	self->marktype = type;
	return type;
}

static gword raster_smheight(RASTER_GIOS *self, gword scale)
{
	if (scale < 3) scale = 3;
	self->markheight = scale;
	return scale;
}




static gword raster_stfont(RASTER_GIOS *self, gword font)
{
	if (font == 0 || font > 2 + self->fonts)
	{
		font = 1;
	}
	if (font == self->fonts + 1)
	{
		self->curfont = self->tsans;
	}
	else if (font == self->fonts + 2)
	{
		self->curfont = self->gsans;
	}
	else
	{
		(*self->surface->select_font)(self->surface, font, &self->curfont);
	}
	raster_text_baseline(self);
	return font;
}

static gword raster_sfstyle(RASTER_GIOS *self, gword style)
{
	if (style > 3) style = 0;

	self->fillstyle = style;

	return style;
}

static gword raster_sfindex(RASTER_GIOS *self, gword style)
{
	if (style < 1 || style > 6) style = 1;

	self->fillindex = style;

	return style;
}

static void raster_readcellarray(RASTER_GIOS *self, gword *contrl, gword *ptsin, gword *intout)
{
	gword x0, y0, x1, y1, x, y, w, h;
	gword row, col;
	float vscale, hscale;
	unsigned pix;
	int i;
	int errs = 0;

	x0 = ptsin[0];
	y0 = ptsin[1];
	x1 = ptsin[2];
	y1 = ptsin[3];

	if (x0 > x1) { gword tmp = x0; x0 = x1; x1 = tmp; }
	if (y0 > y1) { gword tmp = y0; y0 = y1; y1 = tmp; }

	h = y1 - y0;
	w = x1 - x0;

	vscale = (float)h / (float)contrl[6];
	hscale = (float)w / (float)contrl[5];

	for (row = 0; row < contrl[6]; row++)
	{
		y = self->height - 1 - y1 + ((row + 0.5) * vscale);

		for (col = 0; col < contrl[5]; col++)
		{
			x = x0 + ((col + 0.5) * hscale);
			pix = (*self->surface->getpixel)(self->surface, x, y);
	
			intout[row * contrl[5] + col] = 0xFFFF;	
			for (i = 0; i < self->maxcolours; i++)
			{
				if (self->palette[i].ink == pix)
				{
					intout[row * contrl[5] + col] = i;
					break;
				}
			}
			if (intout[row * contrl[5] + col] == 0xFFFF)
			{
				errs = 1;
			}
		}
	}
	contrl[NINTOUT] = contrl[5] * contrl[6];
	contrl[7] = contrl[5];	/* Number of columns populated */
	contrl[8] = contrl[6];	/* Number of rows populated */
	contrl[9] = errs;
}




static gword raster_swrmode(RASTER_GIOS *self, gword mode)
{
	if (mode >= 1 && mode <= 4)
	{
		self->writemode = mode;
	}
	else
	{
		self->writemode = 1;
	}
	return self->writemode;
}



