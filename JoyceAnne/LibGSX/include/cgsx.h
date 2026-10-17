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

#ifdef __cplusplus
extern "C" {
#endif

#define CGSX_VERSION 0x011;	/* version 0.1.1 */

typedef unsigned short gword;

/* The GDOS is an opaque structure, its implementation hidden */
typedef struct GDOS *PGDOS;

/* GIOSDEVICE represents a specific GSX device. This can't be an opaque 
 * structure because the library user may want to implement one. It has 
 * two public methods: */
typedef struct GIOSDevice
{
	/* Shut down this device and free all associated memory */
	void (*destroy)(struct GIOSDevice **ptr);

	/* Handle all GSX operations */
	int (*dispatch)(struct GIOSDevice *self, gword *contrl, gword *intin, 
				gword *ptsin, gword *intout, gword *ptsout);

} GIOSDEVICE;


/* If a GIOS device is to be created by the GDOS, then its creation function
 * needs to have this signature. The parameter is the value passed to
 * gdos_register() and is interpreted by the driver. For example, the 
 * gios_raster device takes a RASTERSURFACE as the parameter */
typedef int  (*GIOSCREATEFUNC)(PGDOS gdos, GIOSDEVICE **ptr, void *param);

/* Create a raster device. The parameter should be the RASTERSURFACE on 
 * which to render output. */
int gios_raster_create(PGDOS gdos, GIOSDEVICE **ptr, void *param);

#if defined(HAVE_LIBHPDF) && defined (HAVE_HPDF_H)

typedef struct pdfparam
{
	float width;
	float height;
	const char *outputdir;
} PDFPARAM;

/* Create a PDF device. The parameter should be a PDFPARAM giving 
 * page size and output directory */
int gios_pdf_create(PGDOS self, GIOSDEVICE **ptr, void *param);
#endif


/* Create the GDOS. Returns 0 for success, -1 if out of memory */
int gdos_create(PGDOS *ptr);

/* Free the GDOS */
void gdos_destroy(PGDOS *ptr);

/* Register a GIOS device with the GDOS. 
 *
 * Parameters: self    a GDOS instance created by gdos_create()
 *             device  GSX device number. Conventionally in the range
 *                      1-9  for displays
 *                     11-19 for plotters
 *                     21-29 for printers
 *                     31-39 for metafiles.
 *             create  A function that creates that type of device
 *             param   A device-specific parameter that is passed to that 
 *                     function
 * Returns 0 for success, -1 if device number is out of range. 
 * 
 */
int gdos_register(PGDOS self, gword device, GIOSCREATEFUNC create, void *param);



/* Unregister a GIOS device */
void gdos_unregister(PGDOS self, gword device);

/* Handle a GSX call */
int gdos_dispatch(PGDOS self, gword *contrl, gword *intin, gword *ptsin,
				gword *intout, gword *ptsout);



/* Ill-behaved GSX programs tend not to set the INTIN count on functions
 * that take a fixed number of parameters. Programs that actually do want to 
 * marshal INTIN need to have the count set, so this function sets the 
 * INTIN count for all GSX functions that take a fixed number of parameters.
 *
 * The default is to set contrl[NINTIN] to 0, except for the handful of
 * functions that do take a variable number of integer parameters */
void check_contrl(gword *contrl);

/* If a RASTERSURFACE has fonts, this structure is used to return them */
typedef struct GSXFont
{
	gword width;		/* Font width, pixels */
	gword height;		/* Font height, pixels */
	gword cellsize;		/* Should be ((width + 7)/8) * height */
	gword length;		/* Number of characters present */
	unsigned char *bits;	/* Font data */
} GSXFONT;


/* C-GSX supports output to a generic raster device. The raster device 
 * needs to implement these functions: */
typedef struct RasterSurface
{
	/* Destroy this device, free any memory used, flush any buffers */
	void (*destroy)(struct RasterSurface **pself);

	/* Override any GSX function. Returns 1 if handled, 0 to use the
	 * driver default implementation, -1 if error
 	 * 
	 * To call the driver default implementation from this function,
	 * temporarily set this pointer to null, then call the caller's
	 * 'dispatch' entry point, and restore this afterwards. This is
	 * one way to implement input or text-mode escapes */
	int (*dispatch)(struct RasterSurface *self, GIOSDEVICE *caller,
			gword *contrl, gword *intin, gword *ptsin, 
			gword *intout, gword *ptsout);

	/* Get screen dimensions in pixels */
	void (*dimensions)(struct RasterSurface *self, gword *width, gword *height);

	/* Get the size of a pixel (for aspect ratio purposes) */
	void (*pixelsize)(struct RasterSurface *self, gword *width, gword *height);

	/* Switch to graphics mode. Can be NULL if graphics/text are not
	 * separate modes */
	void (*enter_g)(struct RasterSurface *self);

	/* Switch to text mode. */
	void (*exit_g)(struct RasterSurface *self);

	/* Clear the display area */
	void (*clear)(struct RasterSurface *self, unsigned bgink);

	/* Flush page. For a printer-type device, this will write any pending
	 * output and throw the page */
	void (*flush)(struct RasterSurface *self, unsigned bgink);

	/* Read a pixel */
	unsigned (*getpixel)(struct RasterSurface *self, gword x, gword y);
	
	/* Write a pixel */
	void (*setpixel)(struct RasterSurface *self, gword x, gword y, unsigned ink);

	/* Does this have a fixed mapping of ink to RGB colour, or can it be 
	 * changed (eg by palette registers)? 
	 * Returns 0 for fixed palette, otherwise number of inks in palette */
	unsigned (*has_palette)(struct RasterSurface *self);

	/* How many colours does this device support in total? */
	unsigned (*max_colours)(struct RasterSurface *self);

	/* Get the RGB colour for an ink. RGB values are 0-1000 */
	void (*get_ink_rgb)(struct RasterSurface *self, unsigned ink, gword *r, gword *g, gword *b);
	
	/* Set the RGB colour for an ink. RGB values are 0-1000. Null if 
	 * the device has a fixed palette */
	void (*set_ink_rgb)(struct RasterSurface *self, unsigned ink, gword r, gword g, gword b);

	/* Given an RGB colour, return the ink number that is the closest 
	 * match. */
	unsigned (*closest_rgb)(struct RasterSurface *self, gword r, gword g, gword b);

	/* Get count of built-in fonts. Can be left NULL if the device has
	 * no built-in fonts */
	gword (*count_fonts)(struct RasterSurface *self);

	/* Select a particular font */
	int (*select_font)(struct RasterSurface *self, gword nfont, GSXFONT *font);

} RASTERSURFACE;

/* Typesafe version of gdos_register for raster devices since they're so 
 * common. Returns 0 for success, -1 if device number is out of range. */
int gdos_register_raster(PGDOS self, gword device, RASTERSURFACE *dev);

/* C-GSX supplies some sample raster device implementations */
int rdev_pbm_create(RASTERSURFACE **ptr, const char *outputdir, gword w, gword h, int white_on_black);
int rdev_ppm_create(RASTERSURFACE **ptr, const char *outputdir, gword w, gword h);

#if defined(HAVE_LIBPNG) && defined (HAVE_PNG_H)
int rdev_png_create(RASTERSURFACE **ptr, const char *outputdir, gword w, gword h);
#endif

#ifdef __cplusplus
}
#endif

