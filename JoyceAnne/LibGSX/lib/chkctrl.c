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

/* Ill-behaved GSX programs tend not to set the INTIN count on functions
 * that take a fixed number of parameters. Programs that actually do want to 
 * marshal INTIN need to have the count set, so this function sets the 
 * INTIN count for all GSX functions that take a fixed number of parameters.
 *
 * The default is to set contrl[NINTIN] to 0, except for the handful of
 * functions that do take a variable number of integer parameters */
void check_contrl(gword *contrl)
{
	switch (contrl[0])
	{
		case 1:	contrl[NINTIN] = 10;	break;	/* v_opnwk */
		/* escapes */
		case 5:	switch(contrl[SPECIAL])
			{
				case 11: contrl[NINTIN] = 2; break;
				case 12: break; /* variable intin count */
				default: contrl[NINTIN] = 0; break;
			}
			break;
		case 8: break;	/* variable intin count */
		case 10: break;	/* variable intin count */
		/* General Drawing Primitives */
		case 11:
			switch(contrl[SPECIAL])
			{
				case 2:	/* Arc */
				case 3:	/* Pie slice */
					contrl[NINTIN] = 2; break;
				case 5:	/* variable intin count */
					break;
			}
			break;
		case 13: contrl[NINTIN] = 3; break; /* Set text direction */
		case 14: contrl[NINTIN] = 4; break; /* Set colour index */
		case 15: /* Set line style */
		case 17: /* Set line colour */
		case 18: /* Set marker type */
		case 20: /* Set marker colour */
		case 21: /* Set text font */
		case 22: /* Set text colour */
		case 23: /* Set fill style */
		case 24: /* Set fill index */
		case 25: /* Set fill colour */
		case 28: /* Read locator */
		case 30: /* Read choice */
		case 32: /* Set write mode */
			 contrl[NINTIN] = 1; break;
		case 26: contrl[NINTIN] = 2; break;	/* Inquire colour */
		case 27: break; /* Variable intin count */
		case 29: contrl[NINTIN] = 2; break;	/* Read valuator */
		case 31: contrl[NINTIN] = 3; break;	/* Read keyboard */
		case 33: contrl[NINTIN] = 2; break;	/* Set input mode */
		default:
			contrl[NINTIN] = 0; break;
		
	}
}
