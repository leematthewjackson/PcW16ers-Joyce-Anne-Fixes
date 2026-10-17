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

static unsigned char data_gsans08[] = 
{
#include "gsans08.inc"
};

static unsigned char data_tsans08[] = 
{
#include "tsans08.inc"
};

static unsigned char data_gsans14[] = 
{
#include "gsans14.inc"
};

static unsigned char data_tsans14[] = 
{
#include "tsans14.inc"
};

static unsigned char data_gsans16[] = 
{
#include "gsans16.inc"
};

static unsigned char data_tsans16[] = 
{
#include "tsans16.inc"
};


/* These fonts are all in PSF1 format, so the bitmap data can be found 4 
 * bytes after the header */
GSXFONT font_gsans08 = { 8,  8,  8, 256, &data_gsans08[4] };
GSXFONT font_tsans08 = { 8,  8,  8, 256, &data_tsans08[4] };
GSXFONT font_gsans14 = { 8, 14, 14, 256, &data_gsans14[4] };
GSXFONT font_tsans14 = { 8, 14, 14, 256, &data_tsans14[4] };
GSXFONT font_gsans16 = { 8, 16, 16, 256, &data_gsans16[4] };
GSXFONT font_tsans16 = { 8, 16, 16, 256, &data_tsans16[4] };

