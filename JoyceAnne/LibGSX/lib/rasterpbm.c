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

typedef struct 
{
	RASTERSURFACE vtable;

	gword width;
	gword pitch;
	gword height;
	char *outputdir;	
	unsigned char *buffer;
	int dirty;
	int white_on_black;
} RDEV_PBM;


static void rdev_pbm_destroy(RASTERSURFACE **ptr);
static int rdev_pbm_dispatch(struct RasterSurface *self, GIOSDEVICE *caller,
			gword *contrl, gword *intin, gword *ptsin, 
			gword *intout, gword *ptsout);
static void rdev_pbm_dimensions(RASTERSURFACE *self, gword *w, gword *h);
static void rdev_pbm_pixelsize(RASTERSURFACE *self, gword *w, gword *h);
static void rdev_pbm_clear(RASTERSURFACE *self, unsigned ink);
static void rdev_pbm_flush(RASTERSURFACE *self, unsigned ink);
static unsigned rdev_pbm_getpixel(RASTERSURFACE *s, gword x, gword y);
static void rdev_pbm_setpixel(RASTERSURFACE *s, gword x, gword y, unsigned ink);
static unsigned rdev_pbm_has_palette(RASTERSURFACE *self);
static unsigned rdev_pbm_max_colours(RASTERSURFACE *self);
static void rdev_pbm_get_ink_rgb(RASTERSURFACE *self, unsigned ink, 
					gword *r, gword *g, gword *b);
static unsigned rdev_pbm_closest_rgb(RASTERSURFACE *self, 
					gword r, gword g, gword b);



static RASTERSURFACE def_vtable = 
{
	rdev_pbm_destroy,
	rdev_pbm_dispatch,
	rdev_pbm_dimensions,
	rdev_pbm_pixelsize,
	NULL,	/* No enter_g */
	NULL,	/* No exit_g */
	rdev_pbm_clear,
	rdev_pbm_flush,
	rdev_pbm_getpixel,
	rdev_pbm_setpixel,
	rdev_pbm_has_palette,
	rdev_pbm_max_colours,
	rdev_pbm_get_ink_rgb,
	NULL,
	rdev_pbm_closest_rgb,
};

int rdev_pbm_create(RASTERSURFACE **ptr, const char *outputdir, gword w, gword h, int white_on_black)
{
	/* Allocate all the memory required in one go */
	unsigned pitch = (7 + w) / 8;
	size_t required = sizeof(RDEV_PBM) + 1 + strlen(outputdir) + (h * pitch);
	char *data = malloc(required);
	RDEV_PBM *self;

	if (!data) return -1;
	memset(data, 0, required);
	self = (RDEV_PBM *)data;
	/* Now initialise the structure */
	self->vtable = def_vtable;
	self->width = w;
	self->pitch = pitch;
	self->height = h;
	self->dirty = 0;
	self->white_on_black = white_on_black;

	self->outputdir = data + sizeof(RDEV_PBM);
	self->buffer    = (unsigned char *)(self->outputdir + 1 + strlen(outputdir));

	strcpy(self->outputdir, outputdir);
	rdev_pbm_clear(&self->vtable, 0);

	*ptr = &self->vtable;
	return 0;
}


static void rdev_pbm_destroy(RASTERSURFACE **ptr)
{
	RDEV_PBM *self = (RDEV_PBM *)(*ptr);

	if (self && self->dirty)
	{
		rdev_pbm_flush(*ptr, 0);
	}
	free(*ptr);
	*ptr = NULL;
}

/* Override specific GSX functions. In this case, we override the escape 
 * (function 5) to report that there is no interactive text mode support */
static int rdev_pbm_dispatch(struct RasterSurface *self, GIOSDEVICE *caller,
			gword *contrl, gword *intin, gword *ptsin, 
			gword *intout, gword *ptsout)
{
	if (contrl[0] == 5)
	{
		switch (contrl[SPECIAL])
		{
			case 1:		/* Get text screen dimensions */
				contrl[NINTOUT] = 2;
				intout[0] = 0xFFFF;
				intout[1] = 0xFFFF;
				break;
			case 16:	/* Is a pointing device available? */
				contrl[NINTOUT] = 1;
				intout[0] = 0;
				break;
		}
		return 1;
	}
//	if (contrl[0] == 14)
//	{
//		/* Do not allow colour reassignment */
//		return 1;
//	}
	return 0;
}



static void rdev_pbm_clear(RASTERSURFACE *s, unsigned ink)
{
	RDEV_PBM *self = (RDEV_PBM *)s;

	if (self->white_on_black)
	{
		memset(self->buffer, ink ? 0xFF : 0, self->height * self->pitch);
	}
	else
	{
		memset(self->buffer, ink ? 0 : 0xFF, self->height * self->pitch);
	}
	self->dirty = 0;
}

void rdev_preview(const char *filename);

static void rdev_pbm_flush(RASTERSURFACE *s, unsigned ink)
{
	RDEV_PBM *self = (RDEV_PBM *)s;

	int n = 0;
	FILE *fp;
	char *filename;

	if (!self->dirty) 	/* Nothing to output */
	{
		rdev_pbm_clear(s, ink);
		return;
	}

 	filename = malloc(20 + strlen(self->outputdir));
	if (!filename) return;

	/* Find a filename that doesn't exist already */
	do
	{
		sprintf(filename, "%s/cgsx%08d.pbm", self->outputdir, n);
		fp = fopen(filename, "rb");
		if (fp)	/* File exists, close it and skip to the next */
		{
			fclose(fp);
		}
		else	/* File doesn't exist, create it and use it */
		{
			fp = fopen(filename, "wb");
			/* Can't create file. Bail. */
			if (!fp) return;
			break;
		}
		++n;
	} while (1);
	/* Spit out the data */
	fprintf(fp, "P4\n%d\n%d\n", self->width, self->height);
	fwrite(self->buffer, 1, self->pitch * self->height, fp);
	fclose(fp);

	rdev_preview(filename);
	free(filename);
	/* The image has been written. Reset the image buffer */
	rdev_pbm_clear(s, ink);
}


static void rdev_pbm_dimensions(RASTERSURFACE *self, gword *width, gword *height)
{
	if (width) *width = ((RDEV_PBM *)self)->width;
	if (height) *height = ((RDEV_PBM *)self)->height;
}

/* Report a 'square' aspect ratio */
static void rdev_pbm_pixelsize(RASTERSURFACE *self, gword *width, gword *height)
{
	if (width) *width = 800;
	if (height) *height = 800;
}

static unsigned rdev_pbm_getpixel(RASTERSURFACE *s, gword x, gword y)
{
	RDEV_PBM *self = (RDEV_PBM *)s;

	if (x < self->width && y < self->height)
	{
		unsigned char mask = 0x80 >> (x & 7);

		if (self->white_on_black)
		{
			return (self->buffer[y * self->pitch + (x >> 3)] & mask) ? 0 : 1;
		}
		else
		{
			return (self->buffer[y * self->pitch + (x >> 3)] & mask) ? 1 : 0;
		}
	}
	return 0;
}


static void rdev_pbm_setpixel(RASTERSURFACE *s, gword x, gword y, unsigned ink)
{
	RDEV_PBM *self = (RDEV_PBM *)s;

	if (self->white_on_black) 
	{
		ink = !ink;
	}
	if (x < self->width && y < self->height)
	{
		unsigned char mask = 0x80 >> (x & 7);

		if (ink) self->buffer[y * self->pitch + (x >> 3)] |= mask;
		else     self->buffer[y * self->pitch + (x >> 3)] &= ~mask;

		self->dirty = 1;
	}
}

static void rdev_pbm_get_ink_rgb(RASTERSURFACE *s, unsigned ink, 
					gword *r, gword *g, gword *b)
{
	RDEV_PBM *self = (RDEV_PBM *)s;

	if (self->white_on_black) 
	{
		ink = !ink;
	}

	if (ink == 0)	{ *r = *g = *b = 1000; } 	/* White */
	else		{ *r = *g = *b = 0; }		/* Black */	
}


static unsigned rdev_pbm_has_palette(RASTERSURFACE *self) 
{ 
	return 0; 
}

static unsigned rdev_pbm_max_colours(RASTERSURFACE *self) 
{ 
	return 2; 
}

static unsigned rdev_pbm_closest_rgb(RASTERSURFACE *s, gword r, gword g, gword b)
{
	RDEV_PBM *self = (RDEV_PBM *)s;
	int result;

	if (299 * r + 587 * g + 114 * b < 500000)
	{
		result = 1;	/* black */
	}	
	else
	{
		result = 0;	/* white */
	}
	if (self->white_on_black) 
	{
		result = 1 - result;
	}
	return result;
}

