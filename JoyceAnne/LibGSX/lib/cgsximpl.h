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
#include "../config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#ifdef HAVE_ASSERT_H
#include <assert.h>
#else
#define assert(x)
#endif
#ifdef HAVE_HPDF_H
#include <hpdf.h>
#endif

#include "cgsx.h"

#  ifdef __cplusplus
extern "C" {
#  endif // __cplusplus

#define NPTSIN	1	/* contrl[1] = ptsin count */
#define NPTSOUT	2	/* contrl[2] = ptsout count */
#define NINTIN	3	/* contrl[3] = intin count */
#define NINTOUT	4	/* contrl[4] = intout count */
#define SPECIAL 5	/* contrl[5] = subfunction */

#define MAXDEVICE 50	/* GSX device numbers are hardly ever > 31 */

/* Write modes: */
#define MODE_REPLACE 1	/* NEW = (FORE and MASK) or (BACK and not MASK) */
#define MODE_TRANS   2	/* NEW = (FORE and MASK) or (OLD  and not MASK) */
#define MODE_XOR     3	/* NEW = (FORE and MASK) xor OLD */
#define MODE_ERASE   4	/* NEW = (BACK and MASK) or (OLD  and not MASK) */

/* Fill styles: */
#define FILL_HOLLOW   0	/* Fill with solid colour 0 */
#define FILL_SOLID    1	/* Fill with solid fill colour */
#define FILL_HALFTONE 2	/* Fill with halftone pattern */
#define FILL_HATCH    3	/* Fill with hatch pattern */

typedef struct GDOS
{
	gword curdev;
	gword width;
	gword height;

	GIOSCREATEFUNC	create[MAXDEVICE];
	GIOSDEVICE *	device[MAXDEVICE];
	void *		param[MAXDEVICE];
		
} GDOSimpl;

extern GSXFONT font_gsans08, font_gsans14, font_gsans16;
extern GSXFONT font_tsans08, font_tsans14, font_tsans16;

#ifndef M_PI	/* My mingw doesn't have it */
#define M_PI 3.14159265358979323846
#endif

#  ifdef __cplusplus
}
#  endif // __cplusplus

