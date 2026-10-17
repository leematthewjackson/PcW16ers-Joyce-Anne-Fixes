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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cgsx.h"

/* Basic functionality test of a raster device */
int main(int argc, char **argv)
{
	RASTERSURFACE *device;
	int x;

	if (rdev_pbm_create(&device, ".", 800, 600, 1))
	{
		fprintf(stderr, "Failed to create raster device\n");
	}
	for (x = 0; x < 200; x++)
	{
		(*device->setpixel)(device, x + 100, 100, 1);
		(*device->setpixel)(device, x + 100, 300, 1);
		(*device->setpixel)(device, 100, x + 100, 1);
		(*device->setpixel)(device, 300, x + 100, 1);
	}
	(*device->flush)(device, 0);
	(*device->destroy)(&device);	

	return 0;
}
