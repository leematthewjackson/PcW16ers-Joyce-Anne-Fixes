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

#if defined(HAVE_LIBPNG) && defined (HAVE_PNG_H)
# include <png.h>
/* Define png_jmpbuf() in case we are using an early version of libpng */
# ifndef png_jmpbuf
#   define png_jmpbuf(png_ptr) png_ptr->jmpbuf
# endif
#endif


typedef struct 
{
	RASTERSURFACE vtable;

	gword width;
	gword height;
	char *outputdir;	
	unsigned char *buffer;
	int dirty;
} RDEV_PPM;

static int rdev_ppm_dispatch(struct RasterSurface *self, GIOSDEVICE *caller,
			gword *contrl, gword *intin, gword *ptsin, 
			gword *intout, gword *ptsout);
static void rdev_ppm_destroy(RASTERSURFACE **ptr);
static void rdev_ppm_dimensions(RASTERSURFACE *self, gword *w, gword *h);
static void rdev_ppm_pixelsize(RASTERSURFACE *self, gword *w, gword *h);
static void rdev_ppm_clear(RASTERSURFACE *self, unsigned ink);
static void rdev_ppm_flush(RASTERSURFACE *self, unsigned ink);
static unsigned rdev_ppm_getpixel(RASTERSURFACE *s, gword x, gword y);
static void rdev_ppm_setpixel(RASTERSURFACE *s, gword x, gword y, unsigned ink);
static unsigned rdev_ppm_has_palette(RASTERSURFACE *self);
static unsigned rdev_ppm_max_colours(RASTERSURFACE *self);
static void rdev_ppm_get_ink_rgb(RASTERSURFACE *self, unsigned ink, 
					gword *r, gword *g, gword *b);
static unsigned rdev_ppm_closest_rgb(RASTERSURFACE *self, 
					gword r, gword g, gword b);

#if defined(HAVE_LIBPNG) && defined (HAVE_PNG_H)
static void rdev_png_flush(RASTERSURFACE *self, unsigned ink);
#endif


static RASTERSURFACE def_vtable = 
{
	rdev_ppm_destroy,
	rdev_ppm_dispatch,
	rdev_ppm_dimensions,
	rdev_ppm_pixelsize,
	NULL,	/* No enter_g */
	NULL,	/* No exit_g */
	rdev_ppm_clear,
	rdev_ppm_flush,
	rdev_ppm_getpixel,
	rdev_ppm_setpixel,
	rdev_ppm_has_palette,
	rdev_ppm_max_colours,
	rdev_ppm_get_ink_rgb,
	NULL,
	rdev_ppm_closest_rgb,
};

int rdev_ppm_create(RASTERSURFACE **ptr, const char *outputdir, gword w, gword h)
{
	size_t required = sizeof(RDEV_PPM) + 1 + strlen(outputdir) + (w * h * 3);
	char *data = malloc(required);
	RDEV_PPM *self;

	if (!data) return -1;
	memset(data, 0, required);
	self = (RDEV_PPM *)data;
	/* Now initialise the structure */
	self->vtable = def_vtable;
	self->width = w;
	self->height = h;
	self->dirty = 0;

	self->outputdir = data + sizeof(RDEV_PPM);
	self->buffer    = (unsigned char *)(self->outputdir + 1 + strlen(outputdir));

	strcpy(self->outputdir, outputdir);
	rdev_ppm_clear(&self->vtable, 0);

	*ptr = &self->vtable;
	return 0;
}

#if defined(HAVE_LIBPNG) && defined (HAVE_PNG_H)
/* rdev_png is basically the same as rdev_ppm, except for the format of
 * the output file created by the flush function */
int rdev_png_create(RASTERSURFACE **ptr, const char *outputdir, gword w, gword h)
{
	int r = rdev_ppm_create(ptr, outputdir, w, h);

	if (*ptr)
	{
		(*ptr)->flush = &rdev_png_flush;
	}
	return r;
}


#endif


static void rdev_ppm_destroy(RASTERSURFACE **ptr)
{
	RDEV_PPM *self = (RDEV_PPM *)(*ptr);

	if (self && self->dirty)
	{
		(*self->vtable.flush)(*ptr, 0);
	}
	free(*ptr);
	*ptr = NULL;
}


/* Override specific GSX functions. In this case, we override the escape 
 * (function 5) to report that there is no interactive text mode support */
static int rdev_ppm_dispatch(struct RasterSurface *self, GIOSDEVICE *caller,
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
	return 0;
}

static void rdev_ppm_clear(RASTERSURFACE *s, unsigned ink)
{
	RDEV_PPM *self = (RDEV_PPM *)s;
	unsigned char r = (ink >> 16) & 0xFF;
	unsigned char g = (ink >> 8) & 0xFF;
	unsigned char b = ink & 0xFF;
	unsigned bufsize = self->height * self->width * 3;
	int n;

	for (n = 0; n < bufsize;)
	{
		self->buffer[n++] = r;
		self->buffer[n++] = g;
		self->buffer[n++] = b;
	}
	self->dirty = 0;
}

void rdev_preview(const char *filename)
{
/*
	char *cmd = malloc(10 + strlen(filename));
	if (!cmd) return;
	char *type;

	type = strrchr(filename, '.');
	if (type && !strcmp(type, ".png"))
	{	
		sprintf(cmd, "eog %s &", filename);
	}
	else
	{
		sprintf(cmd, "xv %s &", filename);
	}
	(void)system(cmd);
	free(cmd);
*/
}


static void rdev_ppm_flush(RASTERSURFACE *s, unsigned ink)
{
	RDEV_PPM *self = (RDEV_PPM *)s;

	int n = 0;
	FILE *fp;
	char *filename;

	if (!self->dirty) 	/* Nothing to output */
	{
		rdev_ppm_clear(s, ink);
		return;
	}

 	filename = malloc(20 + strlen(self->outputdir));
	if (!filename) return;

	/* Find a filename that doesn't exist already */
	do
	{
		sprintf(filename, "%s/cgsx%08d.ppm", self->outputdir, n);
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
	fprintf(fp, "P6\n%d\n%d\n255\n", self->width, self->height);
	fwrite(self->buffer, 1, self->width * self->height * 3, fp);
	fclose(fp);

	rdev_preview(filename);
	free(filename);
	/* The image has been written. Reset the image buffer */
	rdev_ppm_clear(s, ink);
}

#if defined(HAVE_LIBPNG) && defined (HAVE_PNG_H)

static void rdev_png_error(png_structp png_ptr, png_const_charp error_msg)
{
	fprintf(stderr, "Error writing PNG file: %s\n", error_msg);

	longjmp(png_jmpbuf(png_ptr), 1);
}

static void rdev_png_warn(png_structp png_ptr, png_const_charp warning_msg)
{
	fprintf(stderr, "libpng warning: %s\n", warning_msg);
}


static void rdev_png_flush(RASTERSURFACE *s, unsigned ink)
{
	RDEV_PPM *self = (RDEV_PPM *)s;

	int n = 0;
	FILE *fp;
	char *filename;
	png_bytepp rows;
	png_structp png;
	png_infop pnginfo;
	png_color_16 col16;

	if (!self->dirty) 	/* Nothing to output */
	{
		rdev_ppm_clear(s, ink);
		return;
	}

 	filename = malloc(20 + strlen(self->outputdir));
	if (!filename) return;

	/* Find a filename that doesn't exist already */
	do
	{
		sprintf(filename, "%s/cgsx%08d.png", self->outputdir, n);
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

	rows = calloc(self->height, sizeof(png_bytepp));
	if (!rows)
	{
		remove(filename);
		fclose(fp);
		return;	/* No memory to create png */
	}
	for (n = 0; n < self->height; n++)
	{
		rows[n] = self->buffer + (n * self->width * 3);
	}
	png = png_create_write_struct(PNG_LIBPNG_VER_STRING, self, 
			rdev_png_error, rdev_png_warn);
	if (!png) 
	{
		remove(filename);
		fclose(fp);
		free(rows);
		return;	/* No memory to create png */
	}
	pnginfo = png_create_info_struct(png);
	if (!pnginfo)
	{
		png_destroy_write_struct(&png, NULL);
		remove(filename);
		fclose(fp);
		free(rows);
		return;
	}
	if (setjmp(png_jmpbuf(png)))
	{
		/* Caught a fatal error */
		png_destroy_write_struct(&png, &pnginfo);
		remove(filename);
		fclose(fp);
		free(rows);
		return;
	}
	/* Spit out the data to libpng */
	png_init_io (png, fp);
	png_set_IHDR(png, pnginfo, self->width, self->height,
			8,  PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	col16.index = 0;
	col16.red = col16.green = col16.blue = col16.gray = 0;
	png_set_bKGD(png, pnginfo, &col16);		
	png_set_rows(png, pnginfo, rows);
	png_write_png(png, pnginfo, PNG_TRANSFORM_IDENTITY, NULL);
	png_destroy_write_struct(&png, &pnginfo);
	fclose(fp);
	free(rows);

	rdev_preview(filename);
	free(filename);
	/* The image has been written. Reset the image buffer */
	rdev_ppm_clear(s, ink);

}
#endif




static void rdev_ppm_dimensions(RASTERSURFACE *self, gword *width, gword *height)
{
	if (width) *width = ((RDEV_PPM *)self)->width;
	if (height) *height = ((RDEV_PPM *)self)->height;
}

/* Report a 'square' aspect ratio */
static void rdev_ppm_pixelsize(RASTERSURFACE *self, gword *width, gword *height)
{
	if (width) *width = 800;
	if (height) *height = 800;
}

static unsigned rdev_ppm_getpixel(RASTERSURFACE *s, gword x, gword y)
{
	RDEV_PPM *self = (RDEV_PPM *)s;

	if (x < self->width && y < self->height)
	{
		unsigned char *data = self->buffer + ((y * self->width) + x) * 3;
		return (((data[0] & 0xFF) << 16) | 
			((data[1] & 0xFF) << 8)  | 
		   	 (data[2] & 0xFF));
	}
	return 0;
}


static void rdev_ppm_setpixel(RASTERSURFACE *s, gword x, gword y, unsigned ink)
{
	RDEV_PPM *self = (RDEV_PPM *)s;



	if (x < self->width && y < self->height)
	{
		unsigned char *data = self->buffer + ((y * self->width) + x) * 3;
		data[0] = (ink >> 16) & 0xFF;
		data[1] = (ink >> 8) & 0xFF;
		data[2] = ink & 0xFF;
		self->dirty = 1;
	}
}

static void rdev_ppm_get_ink_rgb(RASTERSURFACE *s, unsigned ink, 
					gword *r, gword *g, gword *b)
{
	*r = ((ink >> 16) & 0xFF) * 1000 / 255;
	*g = ((ink >>  8) & 0xFF) * 1000 / 255;
	*b = ( ink        & 0xFF) * 1000 / 255;
}


static unsigned rdev_ppm_has_palette(RASTERSURFACE *self) 
{ 
	return 0; 
}

static unsigned rdev_ppm_max_colours(RASTERSURFACE *self) 
{ 
	return 0x100000; 
}

static unsigned rdev_ppm_closest_rgb(RASTERSURFACE *s, gword r, gword g, gword b)
{
	return (( r * 255 / 1000) << 16) | ((g * 255 / 1000) << 8) | ((b * 255) / 1000);
}

