/* 
 * Dos/PC Emulator
 * Copyright (C) 1991 Jim Hudgens
 * 
 * 
 * The file is part of GDE.
 * 
 * GDE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 * 
 * GDE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GDE; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  
 *
 */

#ifndef I87_H
#define I87_H

#ifdef I87_SUPPORT


static char i87_h_rcsid[]=
  "$Id: $";

/* $Log: $
 *
 */


/* basic 8087 register can hold any of the following 
   values */

union i87_reg_u 
 {
    char    tenbytes[10];
    double  dval;
    float   fval;
    long    lval;
    short   sval;
 };

struct i87_reg
  {
    union i87_reg_u  reg;
    char             tag;    
  };


/*
 * since we are not going to worry about the problems of 
 * aliasing registers, every time a register is modified,
 * its result type is set in the tag fields for that register.
 * If some operation attempts to access the type in a
 * way inconsistent with its current storage format, then
 * we flag the operation.  If common, we'll attempt the 
 * conversion.
 *
 */


#define  I87_VALID   0x80

#define  I87_REGTYP(r)   ((r) & 0x7F)

#define  I87_WORD    0x0
#define  I87_SHORT   0x1
#define  I87_LONG    0x2
#define  I87_FLOAT   0x3 
#define  I87_DOUBLE  0x4
#define  I87_LDBL    0x5
#define  I87_BSD     0x6


#define  I87_STKTOP  0


struct i87_registers
 {
    struct  i87_reg   i87_stack[8];
    int               i87_flags; 
    int               i87_config;   /* rounding modes, etc. */
    short             i87_tos, i87_bos;
 };


/* 
  There are two versions of the following macro.

  One version is for opcode D9, for which there are 
  more than 32 instructions encoded in the second byte
  of the opcode. 
  
  The other version, deals with all the other 7 i87 
  opcodes, for which there are only 32 strings needed to 
  describe the instructions.
  */



#endif  /* I87_SUPPORT */

/* the decoding must occur whether i87_support is defined or not.
   So it must be outside the previous ifdef block. 
*/
    
#ifdef DEBUG
#define DECODE_PRINTINSTR32(m,t,mod,rh,rl)\
    DECODE_PRINTF(m,t[(mod<<3)+(rh)]);
#define DECODE_PRINTINSTR256(m,t,mod,rh,rl)\
    DECODE_PRINTF(m,t[(mod<<6)+(rh<<3)+(rl)]);
#else
#define DECODE_PRINTINSTR32(m,t,mod,rh,rl)  
#define DECODE_PRINTINSTR256(m,t,mod,rh,rl)
#endif

#endif /* I87_H */

    


