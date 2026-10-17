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
#include "z80.h"
#include "../config.h"

#if defined(HAVE_LIBHPDF) && defined (HAVE_HPDF_H)
#include <hpdf.h>
#endif

#include "cgsx.h"

#define MEMORY_SIZE 0x10000
static uint8_t* memory = NULL;
static bool test_finished = 0;

// For ease of testing, limit the PTSIN, PTSOUT, INTIN and INTOUT arrays to 
// 256 entries each. The GSX API has a theoretical maximum of 32768 ints and
// 16384 points but real-life programs are more likely to stop at 256.
// 
#define MAXPOINTS 256	// Maximum number of point params / results supported. 
#define MAXINTS   256	// Maximum number of integer params / results supported.

static PGDOS gdos;
static RASTERSURFACE *pbm;
static RASTERSURFACE *ppm;
static RASTERSURFACE *png;
static int device;

static uint8_t rb(void* userdata, uint16_t addr) 
{
	return memory[addr];
}

static uint16_t rw(uint16_t addr) 
{
	return (memory[addr + 1] << 8) | memory[addr];
}


static void wb(void* userdata, uint16_t addr, uint8_t val) 
{
	memory[addr] = val;
}

static void ww(uint16_t addr, uint16_t val)
{
	memory[addr] = val & 0xFF;
	memory[addr+1] = val >> 8;
}

static int load_file(const char* filename, uint16_t addr) 
{
	FILE* f = fopen(filename, "rb");
	if (f == NULL) 
	{
		fprintf(stderr, "error: can't open file '%s'.\n", filename);
		return 1;
	}
	// file size check:
	fseek(f, 0, SEEK_END);
	size_t file_size = ftell(f);
	rewind(f);

	if (file_size + addr >= MEMORY_SIZE) 
	{
		fprintf(stderr, "error: file %s can't fit in memory.\n", 
			filename);
		return 1;
	}
	// copying the bytes in memory:
	size_t result = fread(&memory[addr], sizeof(uint8_t), file_size, f);
	if (result != file_size) 
	{
		fprintf(stderr, "error: while reading file '%s'\n", filename);
		return 1;
	}
	fclose(f);
	return 0;
}

/* Implement a minimalistic CP/M BDOS, enough to support gsxtest.com */
static uint8_t in(z80* const z, uint8_t port) 
{
	uint8_t operation = z->c;
	uint16_t addr;
	char ch;
	int n;

	gword acontrl;	// Z80 address of contrl array
	gword aintin;	// Z80 address of intin array
	gword aptsin;	// Z80 address of ptsin array
	gword aintout;	// Z80 address of intout array
	gword aptsout;	// Z80 address of ptsout array
	gword contrl[10];
	gword intin[MAXINTS];
	gword ptsin[MAXPOINTS*2];
	gword intout[MAXINTS];
	gword ptsout[MAXPOINTS*2];

	switch (operation)
	{
		case 0:
		test_finished = 1;
		break;

		case 1:	// Keyboard input
		z->l = z->a = getchar();
		return z->a;

		case 2: // Print a character stored in E
		putchar(z->e);
		break;

		case 5: // Printer output
		break;

		case 6: // Direct console I/O
		switch (z->e)
		{
			case 0xFF: 
			case 0xFE: 
				fprintf(stderr, "Unsupported BDOS function 6/%02x\n", z->e);
				exit(1);
			case 0xFD:
				z->l = z->a = getchar();
				return z->a;
			default:
				putchar(z->e);
				break;
		}
		break;

		case 9:	// Print a string at the address in DE
		addr = (z->d << 8) | z->e;
		while (  (ch = rb(z, addr++)) != '$')
		{
			putchar(ch);
		}
		break;

		case 12:	// CP/M BDOS version
		z->h = 0;
		z->l = 0x31;	// Claim to be CP/M 3 as it means we don't have
		break;		// to busy-wait on input

		case 0x73:	// GSX call
// When running on a system like x86 that supports unaligned accesses and is
// little-endian, it might be possible to cast the GSXPB entries directly to
// pointers to the data. But it's safer to do it this way.

		addr = (z->d << 8) | z->e;	// GSXPB address
//
// Get the Z80 addresses of the GSX parameter / result arrays
//
		acontrl = rw(addr);
		aintin  = rw(addr+2);
		aptsin  = rw(addr+4);
		aintout = rw(addr+6);
		aptsout = rw(addr+8);
		
		// Load the control array
		for (n = 0; n < 10; n++) 
		{
			contrl[n] = rw(acontrl + 2* n);
			// printf("contrl[%d]=%04x\n", n, contrl[n]);
		}
		// Sanity-check parameter counts
		check_contrl(contrl);
		if (contrl[1] > MAXPOINTS)
		{
			contrl[1] = MAXPOINTS;
		}
		if (contrl[3] > MAXINTS)
		{
			contrl[3] = MAXINTS;
		}
		// Load the point and integer arrays 
		for (n = 0; n < contrl[1] * 2; n++)
		{
			ptsin[n] = rw(aptsin + 2 * n);
		//	printf("ptsin[%d]=%04x\n", n, ptsin[n]);
		}
		for (n = 0; n < contrl[3]; n++)
		{
			intin[n] = rw(aintin + 2 * n);
		//	printf("intin[%d]=%04x\n", n, intin[n]);
		}
		contrl[2] = 0;
		contrl[4] = 0;
		gdos_dispatch(gdos, contrl, intin, ptsin, intout, ptsout);
		// Write back the contrl array. It's best to do this first
		// in case it overlaps ptsout or intout.
		for (n = 0; n < 10; n++) 
		{
			ww(acontrl + 2 * n, contrl[n]);
		}
		// Return the output point and integer arrays, if any
		for (n = 0; n < contrl[2] * 2; n++)
		{
			ww(aptsout + 2 * n, ptsout[n]);
		}
		for (n = 0; n < contrl[4]; n++)
		{
			ww(aintout + 2 * n, intout[n]);
		}
		return 0xFF;

		default:
		fprintf(stderr, "Unsupported BDOS function %d\n", operation);
		exit(1);
  }

  return 0xFF;
}

static void out(z80* const z, uint8_t port, uint8_t val) 
{
	test_finished = 1;
}

// MARK: test runner
static int run_test (z80* const z, const char* filename)
{
	z80_init(z);
	z->read_byte = rb;
	z->write_byte = wb;
	z->port_in = in;
	z->port_out = out;
	memset(memory, 0, MEMORY_SIZE);

	if (load_file(filename, 0x100) != 0) 
	{
		return 1;
	}
	/* If a device number is passed in, set up the CP/M command tail */
	if (device)
	{
		sprintf((char *)memory + 0x81, " %d", device); 
		memory[0x80] = strlen((char *)memory + 0x81);	
	}

	z->pc = 0x100;

	// inject "out 1,a" at 0x0000 (signal to stop the test)
	memory[0x0000] = 0xD3;
	memory[0x0001] = 0x00;

	// inject "in a,0" at 0x0005 (signal to output some characters)
	// This will also indicate a 51k TPA (top of memory at 0xC900) 
	// which is plenty for gsxtest
	memory[0x0005] = 0xDB;
	memory[0x0006] = 0x00;
	memory[0x0007] = 0xC9;

	test_finished = 0;
	while (!test_finished) 
	{
		z80_step(z);
	}
	return 0;
}

int main(int argc, char **argv) 
{
	if (argc > 1)
	{
		device = atoi(argv[1]);
	}
	// Create the main GDOS
	if (gdos_create(&gdos))
	{
		fprintf(stderr, "Failed to create GDOS\n");
		return 1;
	}
	/* Create two raster backends */
#if defined(HAVE_LIBPNG) && defined (HAVE_PNG_H)
	if (rdev_png_create(&png, ".", 800, 600))
	{
		fprintf(stderr, "Failed to create PNG raster device\n");
		return 1;
	}
#endif
	if (rdev_ppm_create(&ppm, ".", 800, 600))
	{
		fprintf(stderr, "Failed to create PPM raster device\n");
		return 1;
	}
	if (rdev_pbm_create(&pbm, ".", 800, 600, 1))
	{
		fprintf(stderr, "Failed to create PBM raster device\n");
		return 1;
	}
	/* Register them with the GDOS as devices 1 and 2 */
#if defined(HAVE_LIBPNG) && defined (HAVE_PNG_H)
	if (gdos_register_raster(gdos, 1, png))
	{
		fprintf(stderr, "Failed to allocate GIOS device 1 (PNG)\n");
		return 1;
	}
	if (gdos_register_raster(gdos, 2, ppm))
	{
		fprintf(stderr, "Failed to allocate GIOS device 2 (PPM)\n");
		return 1;
	}

#else
	if (gdos_register_raster(gdos, 1, ppm))
	{
		fprintf(stderr, "Failed to allocate GIOS device 1 (PPM)\n");
		return 1;
	}
#endif
	if (gdos_register_raster(gdos, 3, pbm))
	{
		fprintf(stderr, "Failed to allocate GIOS device 3 (PBM)\n");
		return 1;
	}
#if defined(HAVE_LIBHPDF) && defined (HAVE_HPDF_H)
	/* Register PDF with the GDOS as device 21 */
	{
		/* Page size is 595pt x 842pt (A4) and output directory is . */
		/* Note that the parameter must be statically declared so */
		/* it's still around when the GIOS device is created */
		static PDFPARAM param = { 595.0, 842.0, "." };

		if (gdos_register(gdos, 21, gios_pdf_create, &param))
		{
			fprintf(stderr, "Failed to allocate GIOS device 21 (PDF)\n");
			return 1;

		}
	}	
#endif
	memory = malloc(MEMORY_SIZE);
	if (memory == NULL) 
	{
		fprintf(stderr, "Failed to malloc %d bytes\n", MEMORY_SIZE);
		return 1;
	}

	z80 cpu;

	int r = run_test(&cpu, "gsxtest.com");

	free(memory);
	gdos_destroy(&gdos);
	(*pbm->destroy)(&pbm);
	(*ppm->destroy)(&ppm);
	(*png->destroy)(&png);
	return r;
}
