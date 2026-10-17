
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

#include "i86int.hxx"

static char rcsid[]=
  "$Id: i86_ops.c,v 0.11 1991/07/30 02:02:04 hudgens Exp $";

/* $Log: i86_ops.c,v $
 * Revision 0.11  1991/07/30  02:02:04  hudgens
 * added copyright.
 *
 * Revision 0.10  1991/07/21  18:22:08  hudgens
 * Fixed problem with scans, which was the result of the loop break
 * condition being incorrect when used in conjunction with the repe
 * or repne prefixes.  Eureka.  pkzip/pkunzip now compress/decompress
 * correctly.
 *
 * Revision 0.9  1991/07/21  03:33:18  hudgens
 * fixed popf so that it appears to be the same as an 8088 popf, and always
 * sets the high 4 bits of the flags.
 *
 * Revision 0.8  1991/07/21  01:44:11  hudgens
 * fixed aad and aam instructions.
 *
 * Revision 0.7  1991/07/21  00:31:24  hudgens
 * Fixed iret so that it works correctly.
 *
 * Revision 0.6  1991/07/20  16:54:50  hudgens
 * removed the 8087 coprocessor operations.  Moved to i87_ops.c
 *
 * Revision 0.5  1991/07/17  03:50:10  hudgens
 * Minor modifications.
 *
 * Revision 0.4  1991/06/18  02:48:41  hudgens
 * Fixed a problem with scasb and scasw.
 *
 * Revision 0.3  1991/06/03  01:01:10  hudgens
 * fixed minor problems due to unsigned to signed short integer
 * promotions.
 *
 * Revision 0.2  1991/03/31  01:32:10  hudgens
 * fixed segment handling.  Added calls to DECODE_CLEAR_SEGOVR in
 * many places in the code.  Should work much better now.
 *
 * Revision 0.1  1991/03/30  21:15:48  hudgens
 * Initial checkin to RCS.
 *
 *
 */


/* 2/23/91  fixed decode for operand x87. */


/* partially mechanically generated file....(based on the optable) */
/* 
  There are approximately 250 subroutines in here, which correspond
  to the 256 byte-"opcodes" found on the 8086.  The table which 
  dispatches this is found in the files optab.[ch].  
  
  Each opcode proc has a comment preceeding it which gives it's table 
  address.  Several opcodes are missing (undefined) in the table.
  
  Each proc includes information for decoding (DECODE_PRINTF and
  DECODE_PRINTF2), debugging (TRACE_REGS, SINGLE_STEP),
  and misc functions (START_OF_INSTR, END_OF_INSTR).

  Many of the procedures are *VERY* similar in coding.  This has
  allowed for a very large amount of code to be generated in a fairly
  short amount of time (i.e. cut, paste, and modify).  
  The result is that much of the code below could
  have been folded into subroutines for a large reduction in size of
  this file.  The downside would be that there would be a penalty in
  execution speed.  The file could also have been *MUCH* larger by
  inlining certain functions which were called.  This could have
  resulted even faster execution.  The prime directive I used to decide
  whether to inline the code or to modularize it, was basically: 1) no
  unnecessary subroutine calls, 2) no routines more than about 200 lines
  in size, and 3) modularize any code that I might not get right the first
  time.  The fetch_*  subroutines  fall into the latter category.  The 
  The decode_* fall into the second category.  The coding of the 
  "switch(mod){ .... }"  in many of the subroutines below falls into the 
  first category.  Especially, the coding of {add,and,or,sub,...}_{byte,word}
  subroutines are an especially glaring case of the third guideline.
  Since so much of the code is cloned from other modules (compare 
  opcode #00 to opcode #01), making the basic operations subroutine calls
  is especially important; otherwise mistakes in coding an "add" 
  would represent a nightmare in maintenance.  

  So, without further ado, ...
*/
 

extern char parity_tab[];  




void DEFUN(i86op_illegal_op, (m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"ILLEGAL 8086 OPCODE\n");
   TRACE_REGS(m);
   if (m->illegal_op())
   {
   	halt_sys(m);
   }
   END_OF_INSTR(m);
}

/*opcode=0x00*/
void DEFUN(i86op_add_byte_RM_R, (m), PC_ENV *m) 
{
   u_int16     mod,rl,rh;
   u_int8      *destreg,*srcreg;
   u_int16     destoffset;
   u_int8      destval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"ADD\t");

   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = add_byte(m, destval, *srcreg);
	  store_data_byte(m,destoffset,destval);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = add_byte(m, destval, *srcreg);
	  store_data_byte(m,destoffset,destval);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = add_byte(m, destval, *srcreg);
	  store_data_byte(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = add_byte(m, *destreg, *srcreg);
	  break;
       }
   
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}





/*opcode=0x01*/
void DEFUN(i86op_add_word_RM_R,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int16      *destreg,*srcreg;
   u_int16      destoffset;
   u_int16        destval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"ADD\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = add_word(m, destval, *srcreg);
	  store_data_word(m,destoffset,destval);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = add_word(m, destval, *srcreg);
	  store_data_word(m,destoffset,destval);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = add_word(m, destval, *srcreg);
	  store_data_word(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = add_word(m, *destreg, *srcreg);
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x02*/
void DEFUN(i86op_add_byte_R_RM,(m),  PC_ENV *m) 
{

   u_int16     mod,rl,rh;
   u_int8      *destreg,*srcreg;
   u_int16     srcoffset;
   u_int8      srcval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"ADD\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:
	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm00_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = add_byte(m, *destreg, srcval);
	  break;
       
	case 1:

	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm01_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = add_byte(m, *destreg, srcval);
	  break;

	case 2:
	  
	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm10_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = add_byte(m, * destreg, srcval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = add_byte(m, *destreg, *srcreg);
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x03*/
void DEFUN(i86op_add_word_R_RM,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int16      *destreg,*srcreg;
   u_int16      srcoffset;
   u_int16      srcval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"ADD\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:
	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm00_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = add_word(m, *destreg, srcval);
	  break;
       
	case 1:

	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm01_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = add_word(m, *destreg, srcval);
	  break;

	case 2:
	  
	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm10_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = add_word(m, *destreg, srcval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = add_word(m, *destreg, *srcreg);
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x04*/
void DEFUN(i86op_add_byte_AL_IMM,(m),  PC_ENV *m) 
{

   u_int8 srcval;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"ADD\tAL,");
   srcval  =  fetch_byte_imm(m);
   DECODE_PRINTF2(m,"%x\n",srcval);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AL  = add_byte(m, m->R_AL, srcval);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x05*/
void DEFUN(i86op_add_word_AX_IMM,(m),  PC_ENV *m) 
{
   u_int16 srcval;

   START_OF_INSTR(m);   
   DECODE_PRINTF(m,"ADD\tAX,");
   srcval  =  fetch_word_imm(m);
   DECODE_PRINTF2(m,"%d\n",srcval);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AX = add_word(m, m->R_AX, srcval);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x06*/
void DEFUN(i86op_push_ES,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"PUSH\tES\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   push_word(m,m->R_ES);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x07*/
void DEFUN(i86op_pop_ES,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"POP\tES\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_ES = pop_word(m);
   RESYNCH_SEG_ES(m);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0x08*/
void DEFUN(i86op_or_byte_RM_R,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int8       *destreg,*srcreg;
   u_int16      destoffset;
   u_int8       destval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"OR\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = or_byte(m, destval, *srcreg);
	  store_data_byte(m,destoffset,destval);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = or_byte(m, destval, *srcreg);
	  store_data_byte(m,destoffset,destval);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = or_byte(m, destval, *srcreg);
	  store_data_byte(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = or_byte(m, *destreg, *srcreg);
	  break;
       }
   
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x09*/
void DEFUN(i86op_or_word_RM_R,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int16      *destreg,*srcreg;
   u_int16      destoffset;
   u_int16      destval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"OR\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = or_word(m, destval, *srcreg);
	  store_data_word(m,destoffset,destval);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = or_word(m, destval, *srcreg);
	  store_data_word(m,destoffset,destval);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = or_word(m, destval, *srcreg);
	  store_data_word(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = or_word(m, *destreg, *srcreg);
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x0a*/
void DEFUN(i86op_or_byte_R_RM,(m),  PC_ENV *m) 
{

   u_int16      mod,rl,rh;
   u_int8       *destreg,*srcreg;
   u_int16      srcoffset;
   u_int8       srcval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"OR\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:
	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm00_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = or_byte(m, *destreg, srcval);
	  break;
       
	case 1:

	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm01_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = or_byte(m, *destreg, srcval);
	  break;

	case 2:
	  
	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm10_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = or_byte(m, * destreg, srcval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = or_byte(m, *destreg, *srcreg);
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x0b*/
void DEFUN(i86op_or_word_R_RM,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int16      *destreg,*srcreg;
   u_int16      srcoffset;
   u_int16      srcval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"OR\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:
	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm00_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = or_word(m, *destreg, srcval);
	  break;
       
	case 1:

	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm01_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = or_word(m, *destreg, srcval);
	  break;

	case 2:
	  
	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm10_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = or_word(m, *destreg, srcval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = or_word(m, *destreg, *srcreg);
	  break;
       }
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x0c*/
void DEFUN(i86op_or_byte_AL_IMM,(m),  PC_ENV *m) 
{
   u_int8 srcval;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"OR\tAL,");

   srcval  =  fetch_byte_imm(m);
   DECODE_PRINTF2(m,"%x\n",srcval);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AL  = or_byte(m, m->R_AL, srcval);

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x0d*/
void DEFUN(i86op_or_word_AX_IMM,(m),  PC_ENV *m) 
{
   u_int16 srcval;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"OR\tAX,");

   srcval  =  fetch_word_imm(m);
   DECODE_PRINTF2(m,"%x\n",srcval);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AX = or_word(m, m->R_AX, srcval);

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x0e*/
void DEFUN(i86op_push_CS,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"PUSH\tCS\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   push_word(m,m->R_CS);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x0f === ILLEGAL OP*/

/*opcode=0x10*/
void DEFUN(i86op_adc_byte_RM_R,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int8       *destreg,*srcreg;
   u_int16      destoffset;
   u_int8       destval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"ADC\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = adc_byte(m, destval, *srcreg);
	  store_data_byte(m,destoffset,destval);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = adc_byte(m, destval, *srcreg);
	  store_data_byte(m,destoffset,destval);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = adc_byte(m, destval, *srcreg);
	  store_data_byte(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = adc_byte(m, *destreg, *srcreg);
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x11*/
void DEFUN(i86op_adc_word_RM_R,(m),  PC_ENV *m) 
{

   u_int16      mod,rl,rh;
   u_int16      *destreg,*srcreg;
   u_int16      destoffset;
   u_int16      destval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"ADC\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = adc_word(m, destval, *srcreg);
	  store_data_word(m,destoffset,destval);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = adc_word(m, destval, *srcreg);
	  store_data_word(m,destoffset,destval);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = adc_word(m, destval, *srcreg);
	  store_data_word(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = adc_word(m, *destreg, *srcreg);
	  break;
       }


   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x12*/
void DEFUN(i86op_adc_byte_R_RM,(m),  PC_ENV *m) 
{

   u_int16      mod,rl,rh;
   u_int8       *destreg,*srcreg;
   u_int16      srcoffset;
   u_int8       srcval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"ADC\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:
	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm00_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = adc_byte(m, *destreg, srcval);
	  break;
       
	case 1:

	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm01_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = adc_byte(m, *destreg, srcval);
	  break;

	case 2:
	  
	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm10_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = adc_byte(m, * destreg, srcval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = adc_byte(m, *destreg, *srcreg);
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x13*/
void DEFUN(i86op_adc_word_R_RM,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int16      *destreg,*srcreg;
   u_int16      srcoffset;
   u_int16      srcval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"ADC\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:
	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm00_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = adc_word(m, *destreg, srcval);
	  break;
       
	case 1:

	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm01_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = adc_word(m, *destreg, srcval);
	  break;

	case 2:
	  
	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm10_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = adc_word(m, *destreg, srcval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = adc_word(m, *destreg, *srcreg);
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x14*/
void DEFUN(i86op_adc_byte_AL_IMM,(m),  PC_ENV *m) 
{
   u_int8 srcval;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"ADC\tAL,");
   srcval  =  fetch_byte_imm(m);
   DECODE_PRINTF2(m,"%x\n",srcval);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AL  = adc_byte(m, m->R_AL, srcval);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x15*/
void DEFUN(i86op_adc_word_AX_IMM,(m),  PC_ENV *m) 
{
   u_int16 srcval;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"ADC\tAX,");
   srcval  =  fetch_word_imm(m);
   DECODE_PRINTF2(m,"%x\n",srcval);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AX = adc_word(m, m->R_AX, srcval);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x16*/
void DEFUN(i86op_push_SS,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"PUSH\tSS\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   push_word(m,m->R_SS);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x17*/
void DEFUN(i86op_pop_SS,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"POP\tSS\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_SS = pop_word(m);
   RESYNCH_SEG_SS(m);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}



/*opcode=0x18*/
void DEFUN(i86op_sbb_byte_RM_R,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int8       *destreg,*srcreg;
   u_int16      destoffset;
   u_int8       destval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"SBB\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = sbb_byte(m, destval, *srcreg);
	  store_data_byte(m,destoffset,destval);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = sbb_byte(m, destval, *srcreg);
	  store_data_byte(m,destoffset,destval);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = sbb_byte(m, destval, *srcreg);
	  store_data_byte(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = sbb_byte(m, *destreg, *srcreg);
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x19*/
void DEFUN(i86op_sbb_word_RM_R,(m),  PC_ENV *m) 
{

   u_int16      mod,rl,rh;
   u_int16      *destreg,*srcreg;
   u_int16      destoffset;
   u_int16      destval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"SBB\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = sbb_word(m, destval, *srcreg);
	  store_data_word(m,destoffset,destval);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = sbb_word(m, destval, *srcreg);
	  store_data_word(m,destoffset,destval);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = sbb_word(m, destval, *srcreg);
	  store_data_word(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = sbb_word(m, *destreg, *srcreg);
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x1a*/
void DEFUN(i86op_sbb_byte_R_RM,(m),  PC_ENV *m) 
{

   u_int16      mod,rl,rh;
   u_int8       *destreg,*srcreg;
   u_int16      srcoffset;
   u_int8       srcval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"SBB\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:
	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm00_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = sbb_byte(m, *destreg, srcval);
	  break;
       
	case 1:

	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm01_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = sbb_byte(m, *destreg, srcval);
	  break;

	case 2:
	  
	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm10_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = sbb_byte(m, * destreg, srcval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = sbb_byte(m, *destreg, *srcreg);
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x1b*/
void DEFUN(i86op_sbb_word_R_RM,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int16      *destreg,*srcreg;
   u_int16      srcoffset;
   u_int16      srcval;

   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"SBB\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:
	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm00_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = sbb_word(m, *destreg, srcval);
	  break;
       
	case 1:

	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm01_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = sbb_word(m, *destreg, srcval);
	  break;

	case 2:
	  
	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm10_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = sbb_word(m, *destreg, srcval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = sbb_word(m, *destreg, *srcreg);
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x1c*/
void DEFUN(i86op_sbb_byte_AL_IMM,(m),  PC_ENV *m) 
{
   u_int8 srcval;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"SBB\tAL,");
   srcval  =  fetch_byte_imm(m);
   DECODE_PRINTF2(m,"%x\n",srcval);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AL  = sbb_byte(m, m->R_AL, srcval);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x1d*/
void DEFUN(i86op_sbb_word_AX_IMM,(m),  PC_ENV *m) 
{
   u_int16 srcval;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"SBB\tAX,");
   srcval  =  fetch_word_imm(m);
   DECODE_PRINTF2(m,"%x\n",srcval);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AX = sbb_word(m, m->R_AX, srcval);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x1e*/
void DEFUN(i86op_push_DS,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m); 
   DECODE_PRINTF(m,"PUSH\tDS\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   push_word(m,m->R_DS);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x1f*/
void DEFUN(i86op_pop_DS,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"POP\tDS\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_DS = pop_word(m);
   RESYNCH_SEG_DS(m);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0x20*/
void DEFUN(i86op_and_byte_RM_R,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int8       *destreg,*srcreg;
   u_int16      destoffset;
   u_int8       destval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"AND\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = and_byte(m, destval, *srcreg);
	  store_data_byte(m,destoffset,destval);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = and_byte(m, destval, *srcreg);
	  store_data_byte(m,destoffset,destval);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = and_byte(m, destval, *srcreg);
	  store_data_byte(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = and_byte(m, *destreg, *srcreg);
	  break;
       }


   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x21*/
void DEFUN(i86op_and_word_RM_R,(m),  PC_ENV *m) 
{

   u_int16      mod,rl,rh;
   u_int16      *destreg,*srcreg;
   u_int16      destoffset;
   u_int16      destval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"AND\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = and_word(m, destval, *srcreg);
	  store_data_word(m,destoffset,destval);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = and_word(m, destval, *srcreg);
	  store_data_word(m,destoffset,destval);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = and_word(m, destval, *srcreg);
	  store_data_word(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = and_word(m, *destreg, *srcreg);
	  break;
       }


   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x22*/
void DEFUN(i86op_and_byte_R_RM,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int8       *destreg,*srcreg;
   u_int16      srcoffset;
   u_int8       srcval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"AND\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:
	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm00_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = and_byte(m, *destreg, srcval);
	  break;
       
	case 1:

	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm01_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = and_byte(m, *destreg, srcval);
	  break;

	case 2:
	  
	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm10_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = and_byte(m, * destreg, srcval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = and_byte(m, *destreg, *srcreg);
	  break;

       }


   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x23*/
void DEFUN(i86op_and_word_R_RM,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int16      *destreg,*srcreg;
   u_int16      srcoffset;
   u_int16      srcval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"AND\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:
	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm00_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = and_word(m, *destreg, srcval);
	  break;
       
	case 1:

	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm01_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = and_word(m, *destreg, srcval);
	  break;

	case 2:
	  
	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm10_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = and_word(m, *destreg, srcval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = and_word(m, *destreg, *srcreg);
	  break;
       }
   
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x24*/
void DEFUN(i86op_and_byte_AL_IMM,(m),  PC_ENV *m) 
{
   u_int8 srcval;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"AND\tAL,");
   srcval  =  fetch_byte_imm(m);
   DECODE_PRINTF2(m,"%x\n",srcval);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AL  = and_byte(m, m->R_AL, srcval);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x25*/
void DEFUN(i86op_and_word_AX_IMM,(m),  PC_ENV *m) 
{
   u_int16 srcval;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"AND\tAX,");
   srcval  =  fetch_word_imm(m);
   DECODE_PRINTF2(m,"%x\n",srcval);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AX = and_word(m, m->R_AX, srcval);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x26*/
void DEFUN(i86op_segovr_ES,(m),  PC_ENV *m) 
{

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"ES:\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->sysmode |= SYSMODE_SEGOVR_ES;
   /*   note the lack of DECODE_CLEAR_SEGOVR(r)
	since, here is one of 4 opcode subroutines we do not 
	want to do this. 
    */
   END_OF_INSTR(m);
}

/*opcode=0x27*/
void DEFUN(i86op_daa,(m),  PC_ENV *m) 
{
   u_int16 dbyte;
  
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"DAA\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   CANCEL_DFLAGS(m,SYSMODE_DF_ZF|SYSMODE_DF_SF|SYSMODE_DF_PF);

   dbyte = m->R_AL;
   
   if (ACCESS_FLAG(m,F_AF)|| (dbyte&0xf) > 9)
     {
	dbyte += 6;
	if (dbyte&0x100)
	  SET_FLAG(m, F_CF);
	SET_FLAG(m, F_AF);
     }
   else
     CLEAR_FLAG(m, F_AF);

   if (ACCESS_FLAG(m,F_CF) || (dbyte&0xf0) > 0x90)
     {
	dbyte += 0x60;
	SET_FLAG(m, F_CF);
     }
   else 
     CLEAR_FLAG(m, F_CF);
   m->R_AL = dbyte;

   CONDITIONAL_SET_FLAG((m->R_AL & 0x80),m,F_SF);
   CONDITIONAL_SET_FLAG((m->R_AL == 0), m,F_ZF);
   CONDITIONAL_SET_FLAG((parity_tab[m->R_AL]),m,F_PF);
   
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x28*/
void DEFUN(i86op_sub_byte_RM_R,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int8       *destreg,*srcreg;
   u_int16      destoffset;
   u_int8       destval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"SUB\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = sub_byte(m, destval, *srcreg);
	  store_data_byte(m,destoffset,destval);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = sub_byte(m, destval, *srcreg);
	  store_data_byte(m,destoffset,destval);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = sub_byte(m, destval, *srcreg);
	  store_data_byte(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = sub_byte(m, *destreg, *srcreg);
	  break;
       }


   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x29*/
void DEFUN(i86op_sub_word_RM_R,(m),  PC_ENV *m) 
{

   u_int16      mod,rl,rh;
   u_int16      *destreg,*srcreg;
   u_int16      destoffset;
   u_int16      destval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"SUB\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = sub_word(m, destval, *srcreg);
	  store_data_word(m,destoffset,destval);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = sub_word(m, destval, *srcreg);
	  store_data_word(m,destoffset,destval);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = sub_word(m, destval, *srcreg);
	  store_data_word(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = sub_word(m, *destreg, *srcreg);
	  break;
       }


   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x2a*/
void DEFUN(i86op_sub_byte_R_RM,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int8       *destreg,*srcreg;
   u_int16      srcoffset;
   u_int8       srcval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"SUB\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:
	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm00_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = sub_byte(m, *destreg, srcval);
	  break;
       
	case 1:

	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm01_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = sub_byte(m, *destreg, srcval);
	  break;

	case 2:
	  
	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm10_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = sub_byte(m, * destreg, srcval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = sub_byte(m, *destreg, *srcreg);
	  break;

       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x2b*/
void DEFUN(i86op_sub_word_R_RM,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int16      *destreg,*srcreg;
   u_int16      srcoffset;
   u_int16      srcval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"SUB\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:
	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm00_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = sub_word(m, *destreg, srcval);
	  break;
       
	case 1:

	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm01_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = sub_word(m, *destreg, srcval);
	  break;

	case 2:
	  
	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm10_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = sub_word(m, *destreg, srcval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = sub_word(m, *destreg, *srcreg);
	  break;
       }


   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x2c*/
void DEFUN(i86op_sub_byte_AL_IMM,(m),  PC_ENV *m) 
{
   u_int8 srcval;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"SUB\tAL,");
   srcval  =  fetch_byte_imm(m);
   DECODE_PRINTF2(m,"%x\n",srcval);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AL  = sub_byte(m, m->R_AL, srcval);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x2d*/
void DEFUN(i86op_sub_word_AX_IMM,(m),  PC_ENV *m) 
{
   u_int16 srcval;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"SUB\tAX,");

   srcval  =  fetch_word_imm(m);
   DECODE_PRINTF2(m,"%x\n",srcval);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AX = sub_word(m, m->R_AX, srcval);

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x2e*/
void DEFUN(i86op_segovr_CS,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"CS:\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->sysmode |= SYSMODE_SEGOVR_CS;
   /* note no DECODE_CLEAR_SEGOVER here. */
   END_OF_INSTR(m);
}

/*opcode=0x2f*/
void DEFUN(i86op_das,(m),  PC_ENV *m) 
{
   u_int16 dbyte;
   
   START_OF_INSTR(m);   
   DECODE_PRINTF(m,"DAS\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   CANCEL_DFLAGS(m,SYSMODE_DF_SF|SYSMODE_DF_ZF|SYSMODE_DF_PF);
   
   dbyte = m->R_AL;
   if ( ACCESS_FLAG(m,F_AF) || (dbyte&0xf) > 9)
     {
	dbyte -= 6;

	if (dbyte&0x100)            /* XXXXX --- this is WRONG */
	  SET_FLAG(m, F_CF);

	SET_FLAG(m, F_AF);
     }
   else
     CLEAR_FLAG(m, F_AF);

   if (ACCESS_FLAG(m,F_CF) || (dbyte&0xf0) > 0x90)
     {
	dbyte -= 0x60;
	SET_FLAG(m, F_CF);
     }
   else
     CLEAR_FLAG(m, F_CF);

   m->R_AL = dbyte;

   CONDITIONAL_SET_FLAG(m->R_AL & 0x80,m,F_SF);
   CONDITIONAL_SET_FLAG(m->R_AL == 0,m,F_ZF);
   CONDITIONAL_SET_FLAG(parity_tab[m->R_AL],m,F_PF);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0x30*/
void DEFUN(i86op_xor_byte_RM_R,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int8       *destreg,*srcreg;
   u_int16      destoffset;
   u_int8       destval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"XOR\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = xor_byte(m, destval, *srcreg);
	  store_data_byte(m,destoffset,destval);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = xor_byte(m, destval, *srcreg);
	  store_data_byte(m,destoffset,destval);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = xor_byte(m, destval, *srcreg);
	  store_data_byte(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = xor_byte(m, *destreg, *srcreg);
	  break;
       }


   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x31*/
void DEFUN(i86op_xor_word_RM_R,(m),  PC_ENV *m) 
{

   u_int16      mod,rl,rh;
   u_int16      *destreg,*srcreg;
   u_int16      destoffset;
   u_int16      destval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"XOR\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = xor_word(m, destval, *srcreg);
	  store_data_word(m,destoffset,destval);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = xor_word(m, destval, *srcreg);
	  store_data_word(m,destoffset,destval);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = xor_word(m, destval, *srcreg);
	  store_data_word(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = xor_word(m, *destreg, *srcreg);
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x32*/
void DEFUN(i86op_xor_byte_R_RM,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int8       *destreg,*srcreg;
   u_int16      srcoffset;
   u_int8       srcval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"XOR\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:
	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm00_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = xor_byte(m, *destreg, srcval);
	  break;
       
	case 1:

	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm01_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = xor_byte(m, *destreg, srcval);
	  break;

	case 2:
	  
	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm10_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = xor_byte(m, *destreg, srcval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = xor_byte(m, *destreg, *srcreg);
	  break;

       }


   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x33*/
void DEFUN(i86op_xor_word_R_RM,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int16      *destreg,*srcreg;
   u_int16      srcoffset;
   u_int16      srcval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"XOR\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:
	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm00_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = xor_word(m, *destreg, srcval);
	  break;
       
	case 1:

	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm01_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = xor_word(m, *destreg, srcval);
	  break;

	case 2:
	  
	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm10_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = xor_word(m, *destreg, srcval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = xor_word(m, *destreg, *srcreg);
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x34*/
void DEFUN(i86op_xor_byte_AL_IMM,(m),  PC_ENV *m) 
{
   u_int8 srcval;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"XOR\tAL,");
   srcval  =  fetch_byte_imm(m);
   DECODE_PRINTF2(m,"%x\n",srcval);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AL  = xor_byte(m, m->R_AL, srcval);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x35*/
void DEFUN(i86op_xor_word_AX_IMM,(m),  PC_ENV *m) 
{
   u_int16 srcval;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"XOR\tAX,");
   srcval  =  fetch_word_imm(m);
   DECODE_PRINTF2(m,"%x\n",srcval);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AX = xor_word(m, m->R_AX, srcval);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x36*/
void DEFUN(i86op_segovr_SS,(m),  PC_ENV *m) 
{

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"SS:\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->sysmode |= SYSMODE_SEGOVR_SS;
   /* no DECODE_CLEAR_SEGOVER ! */
   END_OF_INSTR(m);
}

/*opcode=0x37*/
void DEFUN(i86op_aaa,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"AAA\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   if ( (m->R_AL & 0xf) > 0x9 || ACCESS_FLAG(m,F_AF))
       {
	 m->R_AL += 0x6;
	 m->R_AH += 1;
	 SET_FLAG(m, F_AF);
	 SET_FLAG(m, F_CF);
       }
   else
       {
	  CLEAR_FLAG(m, F_CF);
	  CLEAR_FLAG(m, F_AF);
       }
   
   m->R_AL &= 0xf;
   
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0x38*/
void DEFUN(i86op_cmp_byte_RM_R,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int8       *destreg,*srcreg;
   u_int16      destoffset;
   u_int8         destval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"CMP\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  cmp_byte(m, destval, *srcreg);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  cmp_byte(m, destval, *srcreg);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  cmp_byte(m, destval, *srcreg);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  cmp_byte(m, *destreg, *srcreg);
	  break;
       }
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x39*/
void DEFUN(i86op_cmp_word_RM_R,(m),  PC_ENV *m) 
{

   u_int16      mod,rl,rh;
   u_int16      *destreg,*srcreg;
   u_int16      destoffset;
   u_int16      destval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"CMP\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  cmp_word(m, destval, *srcreg);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  cmp_word(m, destval, *srcreg);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  cmp_word(m, destval, *srcreg);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  cmp_word(m, *destreg, *srcreg);
	  break;
       }
   
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x3a*/
void DEFUN(i86op_cmp_byte_R_RM,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int8       *destreg,*srcreg;
   u_int16      srcoffset;
   u_int8       srcval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"CMP\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:
	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm00_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  cmp_byte(m, *destreg, srcval);
	  break;
       
	case 1:

	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm01_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  cmp_byte(m, *destreg, srcval);
	  break;

	case 2:
	  
	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm10_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  cmp_byte(m, * destreg, srcval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  cmp_byte(m, *destreg, *srcreg);
	  break;

       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x3b*/
void DEFUN(i86op_cmp_word_R_RM,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int16      *destreg,*srcreg;
   u_int16      srcoffset;
   u_int16      srcval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"CMP\t");   
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:
	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm00_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  cmp_word(m, *destreg, srcval);
	  break;
       
	case 1:

	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm01_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  cmp_word(m, *destreg, srcval);
	  break;

	case 2:
	  
	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm10_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  cmp_word(m, *destreg, srcval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  cmp_word(m, *destreg, *srcreg);
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x3c*/
void DEFUN(i86op_cmp_byte_AL_IMM,(m),  PC_ENV *m) 
{
   u_int8 srcval;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"CMP\tAL,");
   srcval  =  fetch_byte_imm(m);
   DECODE_PRINTF2(m,"%x\n",srcval);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   cmp_byte(m, m->R_AL, srcval);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x3d*/
void DEFUN(i86op_cmp_word_AX_IMM,(m),  PC_ENV *m) 
{
   u_int16 srcval;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"CMP\tAX,");
   srcval  =  fetch_word_imm(m);
   DECODE_PRINTF2(m,"%x\n",srcval);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   cmp_word(m, m->R_AX, srcval);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x3e*/
void DEFUN(i86op_segovr_DS,(m),  PC_ENV *m) 
{

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"DS:\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->sysmode |= SYSMODE_SEGOVR_DS;
   /* NO DECODE_CLEAR_SEGOVR! */
   END_OF_INSTR(m);
}

/*opcode=0x3f*/
void DEFUN(i86op_aas,(m),  PC_ENV *m) 
{
  /* ????  Check out the subtraction here.  Will this ?ever? cause
     the contents of R_AL or R_AH to be affected incorrectly since
     they are being subtracted from *and* are unsigned. 
     Should put an assertion in here.
   */

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"AAS\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);

   if ( (m->R_AL & 0xf) > 0x9 || ACCESS_FLAG(m,F_AF))
       {
	  m->R_AL -= 0x6;
	  m->R_AH -= 1;
	  SET_FLAG(m, F_AF);
	  SET_FLAG(m, F_CF);
       }
   else 
       {
	  CLEAR_FLAG(m, F_CF);
	  CLEAR_FLAG(m, F_AF);
       }
   
   m->R_AL &= 0xf;
   
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0x40*/
void DEFUN(i86op_inc_AX,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"INC\tAX\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AX = inc_word(m,m->R_AX);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x41*/
void DEFUN(i86op_inc_CX,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"INC\tCX\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_CX = inc_word(m,m->R_CX);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x42*/
void DEFUN(i86op_inc_DX,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"INC\tDX\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_DX = inc_word(m,m->R_DX);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x43*/
void DEFUN(i86op_inc_BX,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"INC\tBX\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_BX = inc_word(m,m->R_BX);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x44*/
void DEFUN(i86op_inc_SP,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"INC\tSP\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_SP = inc_word(m,m->R_SP);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x45*/
void DEFUN(i86op_inc_BP,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"INC\tBP\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_BP = inc_word(m,m->R_BP);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x46*/
void DEFUN(i86op_inc_SI,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"INC\tSI\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_SI = inc_word(m,m->R_SI);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x47*/
void DEFUN(i86op_inc_DI,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"INC\tDI\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_DI = inc_word(m,m->R_DI);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x48*/
void DEFUN(i86op_dec_AX,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"DEC\tAX\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AX = dec_word(m,m->R_AX);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x49*/
void DEFUN(i86op_dec_CX,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"DEC\tCX\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_CX = dec_word(m,m->R_CX);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x4a*/
void DEFUN(i86op_dec_DX,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"DEC\tDX\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_DX = dec_word(m,m->R_DX);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x4b*/
void DEFUN(i86op_dec_BX,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"DEC\tBX\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_BX = dec_word(m,m->R_BX);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x4c*/
void DEFUN(i86op_dec_SP,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"DEC\tSP\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_SP = dec_word(m,m->R_SP);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x4d*/
void DEFUN(i86op_dec_BP,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"DEC\tBP\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_BP = dec_word(m,m->R_BP);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x4e*/
void DEFUN(i86op_dec_SI,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"DEC\tSI\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_SI = dec_word(m,m->R_SI);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x4f*/
void DEFUN(i86op_dec_DI,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"DEC\tDI\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_DI = dec_word(m,m->R_DI);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0x50*/
void DEFUN(i86op_push_AX,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"PUSH\tAX\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   push_word(m,m->R_AX);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x51*/
void DEFUN(i86op_push_CX,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"PUSH\tCX\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   push_word(m,m->R_CX);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x52*/
void DEFUN(i86op_push_DX,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"PUSH\tDX\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   push_word(m,m->R_DX);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x53*/
void DEFUN(i86op_push_BX,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"PUSH\tBX\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   push_word(m,m->R_BX);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x54*/
void DEFUN(i86op_push_SP,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"PUSH\tSP\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);

   /* ....  Note this weirdness: One book I have access to 
    claims that the value pushed here is actually sp-2.  I.e.
    it decrements the stackpointer, and then pushes it.  The 286
    I have does it this way.  Changing this causes many problems.*/
   /* changed to push SP-2, since this *IS* how a 8088 does this */

   push_word(m,m->R_SP-2);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x55*/
void DEFUN(i86op_push_BP,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"PUSH\tBP\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   push_word(m,m->R_BP);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x56*/
void DEFUN(i86op_push_SI,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"PUSH\tSI\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   push_word(m,m->R_SI);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x57*/
void DEFUN(i86op_push_DI,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"PUSH\tDI\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   push_word(m,m->R_DI);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x58*/
void DEFUN(i86op_pop_AX,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"POP\tAX\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AX = pop_word(m);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x59*/
void DEFUN(i86op_pop_CX,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"POP\tCX\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_CX = pop_word(m);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x5a*/
void DEFUN(i86op_pop_DX,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"POP\tDX\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_DX = pop_word(m);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x5b*/
void DEFUN(i86op_pop_BX,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"POP\tBX\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_BX = pop_word(m);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x5c*/
void DEFUN(i86op_pop_SP,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"POP\tSP\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_SP = pop_word(m);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x5d*/
void DEFUN(i86op_pop_BP,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"POP\tBP\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_BP = pop_word(m);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x5e*/
void DEFUN(i86op_pop_SI,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"POP\tSI\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_SI = pop_word(m);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x5f*/
void DEFUN(i86op_pop_DI,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"POP\tDI\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_DI = pop_word(m);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x60   ILLEGAL OP, calls i86op_illegal_op() */
/*opcode=0x61   ILLEGAL OP, calls i86op_illegal_op() */
/*opcode=0x62   ILLEGAL OP, calls i86op_illegal_op() */
/*opcode=0x63   ILLEGAL OP, calls i86op_illegal_op() */
/*opcode=0x64   ILLEGAL OP, calls i86op_illegal_op() */
/*opcode=0x65   ILLEGAL OP, calls i86op_illegal_op() */
/*opcode=0x66   ILLEGAL OP, calls i86op_illegal_op() */
/*opcode=0x67   ILLEGAL OP, calls i86op_illegal_op() */
/*opcode=0x68   ILLEGAL OP, calls i86op_illegal_op() */
/*opcode=0x69   ILLEGAL OP, calls i86op_illegal_op() */
/*opcode=0x6a   ILLEGAL OP, calls i86op_illegal_op() */
/*opcode=0x6b   ILLEGAL OP, calls i86op_illegal_op() */
/*opcode=0x6c   ILLEGAL OP, calls i86op_illegal_op() */
/*opcode=0x6d   ILLEGAL OP, calls i86op_illegal_op() */
/*opcode=0x6e   ILLEGAL OP, calls i86op_illegal_op() */
/*opcode=0x6f   ILLEGAL OP, calls i86op_illegal_op() */

/*opcode=0x70*/
void DEFUN(i86op_jump_near_O,(m),  PC_ENV *m) 
{
   int8  offset;
   u_int16 target;
   
   /* jump to byte offset if overflow flag is set */
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"JO\t");
   offset = (int8) fetch_byte_imm(m);
   target = (int16)(m->R_IP) + offset;
   DECODE_PRINTF2(m,"%x\n",target);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   if (ACCESS_FLAG(m,F_OF))
     m->R_IP = target;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x71*/
void DEFUN(i86op_jump_near_NO,(m),  PC_ENV *m) 
{
   int8  offset;
   u_int16 target;
   
   /* jump to byte offset if overflow is not set */
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"JNO\t");
   offset = (int8) fetch_byte_imm(m);
   target = (int16)(m->R_IP) + offset;
   DECODE_PRINTF2(m,"%x\n",target);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   if (!ACCESS_FLAG(m,F_OF))
     m->R_IP = target;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x72*/
void DEFUN(i86op_jump_near_B,(m),  PC_ENV *m) 
{

   int8  offset;
   u_int16 target;

   /* jump to byte offset if carry flag is set. */
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"JB\t");
   offset = (int8)fetch_byte_imm(m);   /* sign extended ??? */
   target = (int16)(m->R_IP) + offset;
   DECODE_PRINTF2(m,"%x\n",target);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   if (ACCESS_FLAG(m, F_CF))
     m->R_IP = target;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x73*/
void DEFUN(i86op_jump_near_NB,(m),  PC_ENV *m) 
{

   int8  offset;
   u_int16 target;

   /* jump to byte offset if carry flag is clear. */
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"JNB\t");
   offset = (int8)fetch_byte_imm(m);   /* sign extended ??? */
   target = (int16)(m->R_IP) + offset;
   DECODE_PRINTF2(m,"%x\n",target);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   if (!ACCESS_FLAG(m,F_CF))
     m->R_IP = target;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x74*/
void DEFUN(i86op_jump_near_Z,(m),  PC_ENV *m) 
{

   int8  offset;
   u_int16 target;

   /* jump to byte offset if zero flag is set. */
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"JZ\t");
   offset = (int8)fetch_byte_imm(m);
   target = (int16)(m->R_IP) + offset;
   DECODE_PRINTF2(m,"%x\n",target);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   RESYNCH_FLAGS(m,SYSMODE_DF_ZF);
   if (ACCESS_FLAG(m, F_ZF))
     m->R_IP = target;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x75*/
void DEFUN(i86op_jump_near_NZ,(m),  PC_ENV *m) 
{

   int8  offset;
   u_int16 target;

   /* jump to byte offset if zero flag is clear. */
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"JNZ\t");
   offset = (int8)fetch_byte_imm(m);
   target = (int16)(m->R_IP) + offset;
   DECODE_PRINTF2(m,"%x\n",target);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   RESYNCH_FLAGS(m,SYSMODE_DF_ZF);
   if (!ACCESS_FLAG(m, F_ZF))
     m->R_IP = target;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x76*/
void DEFUN(i86op_jump_near_BE,(m),  PC_ENV *m) 
{

   int8  offset;
   u_int16 target;

   /* jump to byte offset if carry flag is set or if the zero 
      flag is set. */
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"JBE\t");
   offset = (int8)fetch_byte_imm(m);
   target = (int16)(m->R_IP) + offset;
   DECODE_PRINTF2(m,"%x\n",target);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   RESYNCH_FLAGS(m,SYSMODE_DF_ZF);
   if (ACCESS_FLAG(m,F_CF) || ACCESS_FLAG(m,F_ZF))
     m->R_IP = target;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x77*/
void DEFUN(i86op_jump_near_NBE,(m),  PC_ENV *m) 
{

   int8  offset;
   u_int16 target;

   /* jump to byte offset if carry flag is clear and if the zero 
      flag is clear */
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"JNBE\t");
   offset = (int8)fetch_byte_imm(m);
   target = (int16)(m->R_IP) + offset;
   DECODE_PRINTF2(m,"%x\n",target);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   RESYNCH_FLAGS(m,SYSMODE_DF_ZF);
   if (!(ACCESS_FLAG(m,F_CF)||ACCESS_FLAG(m,F_ZF)))
     m->R_IP = target;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x78*/
void DEFUN(i86op_jump_near_S,(m),  PC_ENV *m) 
{
   int8  offset;
   u_int16 target;

   /* jump to byte offset if sign flag is set */
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"JS\t");
   offset = (int8)fetch_byte_imm(m);
   target = (int16)(m->R_IP) + offset;
   DECODE_PRINTF2(m,"%x\n",target);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   RESYNCH_FLAGS(m,SYSMODE_DF_SF);
   if (ACCESS_FLAG(m,F_SF))
     m->R_IP = target;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x79*/
void DEFUN(i86op_jump_near_NS,(m),  PC_ENV *m) 
{
   int8  offset;
   u_int16 target;

   /* jump to byte offset if sign flag is clear */
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"JNS\t");
   offset = (int8)fetch_byte_imm(m);
   target = (int16)(m->R_IP) + offset;
   DECODE_PRINTF2(m,"%x\n",target);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   RESYNCH_FLAGS(m,SYSMODE_DF_SF);
   if (!ACCESS_FLAG(m,F_SF))
     m->R_IP = target;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x7a*/
void DEFUN(i86op_jump_near_P,(m),  PC_ENV *m) 
{
   int8  offset;
   u_int16 target;

   /* jump to byte offset if parity flag is set (even parity) */
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"JP\t");
   offset = (int8)fetch_byte_imm(m);
   target = (int16)(m->R_IP) + offset;
   DECODE_PRINTF2(m,"%x\n",target);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   RESYNCH_FLAGS(m,SYSMODE_DF_PF);
   if (ACCESS_FLAG(m, F_PF))
     m->R_IP = target;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x7b*/
void DEFUN(i86op_jump_near_NP,(m),  PC_ENV *m) 
{
   int8  offset;
   u_int16 target;

   /* jump to byte offset if parity flag is clear (odd parity) */
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"JNP\t");
   offset = (int8)fetch_byte_imm(m);
   target = (int16)(m->R_IP) + offset;
   DECODE_PRINTF2(m,"%x\n",target);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   RESYNCH_FLAGS(m,SYSMODE_DF_PF);
   if (!ACCESS_FLAG(m, F_PF))
     m->R_IP = target;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/* JHH fixed till here... */

/*opcode=0x7c*/
void DEFUN(i86op_jump_near_L,(m),  PC_ENV *m) 
{
   int8  offset;
   u_int16 target;
   int   sf,of;
   
   /* jump to byte offset if sign flag not equal to overflow flag. */
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"JL\t");
   offset = (int8)fetch_byte_imm(m); 
   target = (int16)(m->R_IP) + offset;
   DECODE_PRINTF2(m,"%x\n",target);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   RESYNCH_FLAGS(m,SYSMODE_DF_SF);

   /* note:
    *  this is the simplest expression i could think of which 
    *  expresses SF != OF.  m->R_FLG&F_SF either equals x80 or x00.
    *  Similarly m->R_FLG&F_OF either equals x800 or x000.
    *  The former shifted right by 7 puts a 1 or 0 in bit 0.
    *  The latter shifter right by 11 puts a 1 or 0 in bit 0.
    *  if the two expressions are the same, i.e. equal, then
    *  a zero results from the xor.  If they are not equal,
    *  then a 1 results, and the jump is taken. 
    */

   sf = ACCESS_FLAG(m,F_SF) != 0;
   of = ACCESS_FLAG(m,F_OF) != 0;
   
   /* was: if ( ((m->R_FLG & F_SF)>>7) ^ ((m->R_FLG & F_OF) >> 11))*/
   if (sf ^ of)
     m->R_IP = target;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x7d*/
void DEFUN(i86op_jump_near_NL,(m),  PC_ENV *m) 
{
   int8  offset;
   u_int16 target;
   int sf,of;
   

   /* jump to byte offset if sign flag not equal to overflow flag. */
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"JNL\t");
   offset = (int8)fetch_byte_imm(m); 
   target = (int16)(m->R_IP) + offset;
   DECODE_PRINTF2(m,"%x\n",target);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   RESYNCH_FLAGS(m,SYSMODE_DF_SF);

   sf = ACCESS_FLAG(m,F_SF) != 0;
   of = ACCESS_FLAG(m,F_OF) != 0;
   
   /* note: inverse of above, but using == instead of xor. */
   /* was: if (((m->R_FLG & F_SF)>>7) == ((m->R_FLG & F_OF) >> 11))*/

   if (sf == of)
     m->R_IP = target;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x7e*/
void DEFUN(i86op_jump_near_LE,(m),  PC_ENV *m) 
{
   int8  offset;
   u_int16 target;
   int sf,of;
   
   /* jump to byte offset if sign flag not equal to overflow flag
    or the zero flag is set */
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"JLE\t");
   offset = (int8)fetch_byte_imm(m);
   target = (int16)(m->R_IP) + offset;
   DECODE_PRINTF2(m,"%x\n",target);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   RESYNCH_FLAGS(m,SYSMODE_DF_SF|SYSMODE_DF_ZF);

   sf = ACCESS_FLAG(m,F_SF) != 0;
   of = ACCESS_FLAG(m,F_OF) != 0;
   /* note: modification of JL */
   /* sf != of */
   /* was:  if ((((m->R_FLG & F_SF)>>7) ^ ((m->R_FLG & F_OF) >> 11)) 
     || (m->R_FLG & F_ZF) ) */
   if ( (sf ^ of) || ACCESS_FLAG(m,F_ZF))
     m->R_IP = target;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x7f*/
void DEFUN(i86op_jump_near_NLE,(m),  PC_ENV *m) 
{

   int8  offset;
   u_int16 target;
   int sf,of;
   
   /* jump to byte offset if sign flag equal to overflow flag.
    and the zero flag is clear*/
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"JNLE\t");
   offset = (int8)fetch_byte_imm(m);
   target = (int16)(m->R_IP) + offset;
   DECODE_PRINTF2(m,"%x\n",target);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   RESYNCH_FLAGS(m,SYSMODE_DF_SF|SYSMODE_DF_ZF);

   sf = ACCESS_FLAG(m,F_SF) != 0;
   of = ACCESS_FLAG(m,F_OF) != 0;
   
/*   if (((m->R_FLG & F_SF)>>7) == ((m->R_FLG & F_OF) >> 11)
       && (!(m->R_FLG & F_ZF))) */

   if ( ( sf == of ) && !ACCESS_FLAG(m,F_ZF))
     m->R_IP = target;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


#ifdef __STDC__
static u_int8    (*opc80_byte_operation[])(PC_ENV *m,u_int8 d,u_int8 s) = 
#else
static u_int8    (*opc80_byte_operation[])() = 
#endif
 {
    add_byte,/*00*/
    or_byte, /*01*/
    adc_byte,/*02*/
    sbb_byte,/*03*/
    and_byte,/*04*/
    sub_byte,/*05*/
    xor_byte,/*06*/
    cmp_byte,/*07*/
 };
    
/*opcode=0x80*/
void DEFUN(i86op_opc80_byte_RM_IMM,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int8       *destreg;
   u_int16      destoffset;
   u_int8       imm;
   u_int8       destval;

   
   /* weirdo special case instruction format.  Part of the 
      opcode held below in "RH".  Doubly nested case would 
      result, except that the decoded instruction 
    */
   START_OF_INSTR(m);
   FETCH_DECODE_MODRM(m,mod,rh,rl);

#ifdef DEBUG
   if (DEBUG_DECODE(m))
     {
	/* XXX DECODE_PRINTF may be changed to something more
	   general, so that it is important to leave the strings
	   in the same format, even though the result is that the 
	   above test is done twice. */

	switch (rh)
	    {
	     case 0:
	       DECODE_PRINTF(m,"ADD\t");
	       break;
	     case 1:
	       DECODE_PRINTF(m,"OR\t");
	       break;
	     case 2:
	       DECODE_PRINTF(m,"ADC\t");
	       break;
	     case 3:
	       DECODE_PRINTF(m,"SBB\t");
	       break;
	     case 4:
	       DECODE_PRINTF(m,"AND\t");
	       break;
	     case 5:
	       DECODE_PRINTF(m,"SUB\t");
	       break;
	     case 6:
	       DECODE_PRINTF(m,"XOR\t");
	       break;
	     case 7:
	       DECODE_PRINTF(m,"CMP\t");
	       break;
	    }	  
     }
   
#endif	  
   
   /* know operation, decode the mod byte to find the addressing 
      mode. */
   switch (mod) 
       {
	case 0:
	  DECODE_PRINTF(m,"BYTE PTR ");
	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  imm  =  fetch_byte_imm(m);
	  DECODE_PRINTF2(m,"%x\n",imm);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opc80_byte_operation[rh])(m, destval, imm);
	  if (rh != 7) store_data_byte(m,destoffset,destval);
	  break;
       
	case 1:

	  DECODE_PRINTF(m,"BYTE PTR ");
	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  imm  =  fetch_byte_imm(m);
	  DECODE_PRINTF2(m,"%x\n",imm);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opc80_byte_operation[rh])(m, destval, imm);
	  if (rh != 7) store_data_byte(m,destoffset,destval);
	  break;

	case 2:
	  
	  DECODE_PRINTF(m,"BYTE PTR ");
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  imm  =  fetch_byte_imm(m);
	  DECODE_PRINTF2(m,"%x\n",imm);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opc80_byte_operation[rh])(m, destval, imm);
	  if (rh != 7) store_data_byte(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  imm  =  fetch_byte_imm(m);
	  DECODE_PRINTF2(m,"%x\n",imm);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opc80_byte_operation[rh])(m, *destreg, imm);
	  if (rh != 7) *destreg = destval;
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}



#ifdef __STDC__
static u_int16    (*opc81_word_operation[])(PC_ENV *m,u_int16 d,u_int16 s) = 
#else
static u_int16    (*opc81_word_operation[])() = 
#endif
 {
    add_word,/*00*/
    or_word, /*01*/
    adc_word,/*02*/
    sbb_word,/*03*/
    and_word,/*04*/
    sub_word,/*05*/
    xor_word,/*06*/
    cmp_word,/*07*/
 };

/*opcode=0x81*/
void DEFUN(i86op_opc81_word_RM_IMM,(m),  PC_ENV *m) 
{

   u_int16      mod,rl,rh;
   u_int16      *destreg;
   u_int16      destoffset;
   u_int16      imm;
   u_int16      destval;

   
   /* weirdo special case instruction format.  Part of the 
      opcode held below in "RH".  Doubly nested case would 
      result, except that the decoded instruction 
    */
   START_OF_INSTR(m);
   FETCH_DECODE_MODRM(m,mod,rh,rl);

#ifdef DEBUG
   if (DEBUG_DECODE(m))
     {
	/* XXX DECODE_PRINTF may be changed to something more
	   general, so that it is important to leave the strings
	   in the same format, even though the result is that the 
	   above test is done twice. */

	switch (rh)
	    {
	     case 0:
	       DECODE_PRINTF(m,"ADD\t");
	       break;
	     case 1:
	       DECODE_PRINTF(m,"OR\t");
	       break;
	     case 2:
	       DECODE_PRINTF(m,"ADC\t");
	       break;
	     case 3:
	       DECODE_PRINTF(m,"SBB\t");
	       break;
	     case 4:
	       DECODE_PRINTF(m,"AND\t");
	       break;
	     case 5:
	       DECODE_PRINTF(m,"SUB\t");
	       break;
	     case 6:
	       DECODE_PRINTF(m,"XOR\t");
	       break;
	     case 7:
	       DECODE_PRINTF(m,"CMP\t");
	       break;
	    }	  
     }
   
#endif	  
   
   /* know operation, decode the mod byte to find the addressing 
      mode. */
   switch (mod) 
       {
	case 0:
	  DECODE_PRINTF(m,"WORD PTR ");
	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  imm  =  fetch_word_imm(m);
	  DECODE_PRINTF2(m,"%x\n",imm);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opc81_word_operation[rh])(m, destval, imm);
	  if (rh != 7) store_data_word(m,destoffset,destval);
	  break;
       
	case 1:

	  DECODE_PRINTF(m,"WORD PTR ");
	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  imm  =  fetch_word_imm(m);
	  DECODE_PRINTF2(m,"%x\n",imm);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opc81_word_operation[rh])(m, destval, imm);
	  if (rh != 7) store_data_word(m,destoffset,destval);
	  break;

	case 2:
	  
	  DECODE_PRINTF(m,"WORD PTR ");
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  imm  =  fetch_word_imm(m);
	  DECODE_PRINTF2(m,"%x\n",imm);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opc81_word_operation[rh])(m, destval, imm);
	  if (rh != 7) store_data_word(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  imm  =  fetch_word_imm(m);
	  DECODE_PRINTF2(m,"%x\n",imm);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opc81_word_operation[rh])(m, *destreg, imm);
	  if (rh != 7) *destreg = destval;
	  break;
       }
 
   DECODE_CLEAR_SEGOVR(m);
	END_OF_INSTR(m);
}


#ifdef __STDC__
static u_int8    (*opc82_byte_operation[])(PC_ENV *m,u_int8 s,u_int8 d) = 
#else
static u_int8    (*opc82_byte_operation[])() = 
#endif
 {
    add_byte,/*00*/
    or_byte, /*01*/  /*YYY UNUSED ????*/
    adc_byte,/*02*/
    sbb_byte,/*03*/
    and_byte,/*04*/  /*YYY UNUSED ????*/
    sub_byte,/*05*/
    xor_byte,/*06*/  /*YYY UNUSED ????*/
    cmp_byte,/*07*/
 };

/*opcode=0x82*/
void DEFUN(i86op_opc82_byte_RM_IMM,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int8       *destreg;
   u_int16      destoffset;
   u_int8       imm;
   u_int8       destval;

   
   /* weirdo special case instruction format.  Part of the 
      opcode held below in "RH".  Doubly nested case would 
      result, except that the decoded instruction 
      Similar to opcode 81, except that the immediate byte
      is sign extended to a word length.
    */
   START_OF_INSTR(m);
   FETCH_DECODE_MODRM(m,mod,rh,rl);

#ifdef DEBUG
   if (DEBUG_DECODE(m))
     {
	/* XXX DECODE_PRINTF may be changed to something more
	   general, so that it is important to leave the strings
	   in the same format, even though the result is that the 
	   above test is done twice. */

	switch (rh)
	    {
	     case 0:
	       DECODE_PRINTF(m,"ADD\t");
	       break;
	     case 1:
	       DECODE_PRINTF(m,"OR\t");	
	       break;
	     case 2:
	       DECODE_PRINTF(m,"ADC\t");
	       break;
	     case 3:
	       DECODE_PRINTF(m,"SBB\t");
	       break;
	     case 4:
	       DECODE_PRINTF(m,"AND\t");
	       break;
	     case 5:
	       DECODE_PRINTF(m,"SUB\t");
	       break;
	     case 6:
	       DECODE_PRINTF(m,"XOR\t");
	       break;
	     case 7:
	       DECODE_PRINTF(m,"CMP\t");
	       break;
	    }	  
     }
   
#endif	  
   
   /* know operation, decode the mod byte to find the addressing 
      mode. */
   switch (mod) 
       {
	case 0:
	  DECODE_PRINTF(m,"BYTE PTR ");
	  destoffset=decode_rm00_address(m,rl);
	  destval = fetch_data_byte(m,destoffset);
	  imm  = fetch_byte_imm(m);
	  DECODE_PRINTF2(m,",%x\n",imm );
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opc82_byte_operation[rh])(m, destval, imm);
	  if (rh != 7) store_data_byte(m,destoffset,destval);
	  break;
       
	case 1:

	  DECODE_PRINTF(m,"BYTE PTR ");
	  destoffset=decode_rm01_address(m,rl);
	  destval = fetch_data_byte(m,destoffset);
	  imm  =  fetch_byte_imm(m);
	  DECODE_PRINTF2(m,",%x\n",imm );
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opc82_byte_operation[rh])(m, destval, imm);
	  if (rh != 7) store_data_byte(m,destoffset,destval);
	  break;

	case 2:
	  
	  DECODE_PRINTF(m,"BYTE PTR ");
	  destoffset=decode_rm10_address(m,rl);
	  destval = fetch_data_byte(m,destoffset);
	  imm  =  fetch_byte_imm(m);
	  DECODE_PRINTF2(m,",%x\n",imm);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opc82_byte_operation[rh])(m, destval, imm);
	  if (rh != 7) store_data_byte(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rl);
	  imm  =  fetch_byte_imm(m);
	  DECODE_PRINTF2(m,",%x\n",imm);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opc82_byte_operation[rh])(m, *destreg, imm);
	  if (rh != 7) *destreg = destval;
	  break;
       }
 
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}



#ifdef __STDC__
static u_int16    (*opc83_word_operation[])(PC_ENV *m,u_int16 s,u_int16 d) = 
#else
static u_int16    (*opc83_word_operation[])() = 
#endif
 {
    add_word,/*00*/
    or_word, /*01*/  /*YYY UNUSED ????*/
    adc_word,/*02*/
    sbb_word,/*03*/
    and_word,/*04*/  /*YYY UNUSED ????*/
    sub_word,/*05*/
    xor_word,/*06*/  /*YYY UNUSED ????*/
    cmp_word,/*07*/
 };

/*opcode=0x83*/
void DEFUN(i86op_opc83_word_RM_IMM,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int16      *destreg;
   u_int16      destoffset;
   u_int16      imm;
   u_int16      destval;

   
   /* weirdo special case instruction format.  Part of the 
      opcode held below in "RH".  Doubly nested case would 
      result, except that the decoded instruction 
      Similar to opcode 81, except that the immediate byte
      is sign extended to a word length.
    */
   START_OF_INSTR(m);
   FETCH_DECODE_MODRM(m,mod,rh,rl);

#ifdef DEBUG
   if (DEBUG_DECODE(m))
     {
	/* XXX DECODE_PRINTF may be changed to something more
	   general, so that it is important to leave the strings
	   in the same format, even though the result is that the 
	   above test is done twice. */

	switch (rh)
	    {
	     case 0:
	       DECODE_PRINTF(m,"ADD\t");
	       break;
	     case 1:
	       DECODE_PRINTF(m,"OR\t");	
	       break;
	     case 2:
	       DECODE_PRINTF(m,"ADC\t");
	       break;
	     case 3:
	       DECODE_PRINTF(m,"SBB\t");
	       break;
	     case 4:
	       DECODE_PRINTF(m,"AND\t");
	       break;
	     case 5:
	       DECODE_PRINTF(m,"SUB\t");
	       break;
	     case 6:
	       DECODE_PRINTF(m,"XOR\t");
	       break;
	     case 7:
	       DECODE_PRINTF(m,"CMP\t");
	       break;
	    }	  
     }
   
#endif	  
   
   /* know operation, decode the mod byte to find the addressing 
      mode. */
   switch (mod) 
       {
	case 0:
	  DECODE_PRINTF(m,"WORD PTR ");
	  destoffset=decode_rm00_address(m,rl);
	  destval = fetch_data_word(m,destoffset);
	  imm  = (int8)fetch_byte_imm(m);
	  DECODE_PRINTF2(m,",%x\n",imm);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opc83_word_operation[rh])(m, destval, imm);
	  if (rh != 7) store_data_word(m,destoffset,destval);
	  break;
       
	case 1:

	  DECODE_PRINTF(m,"WORD PTR ");
	  destoffset=decode_rm01_address(m,rl);
	  destval = fetch_data_word(m,destoffset);
	  imm  =  (int8)fetch_byte_imm(m);
	  DECODE_PRINTF2(m,",%x\n",imm);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opc83_word_operation[rh])(m, destval, imm);
	  if (rh != 7) store_data_word(m,destoffset,destval);
	  break;

	case 2:
	  
	  DECODE_PRINTF(m,"WORD PTR ");
	  destoffset=decode_rm10_address(m,rl);
	  destval = fetch_data_word(m,destoffset);
	  imm  =  (int8) fetch_byte_imm(m);
	  DECODE_PRINTF2(m,",%x\n",imm);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opc83_word_operation[rh])(m, destval, imm);
	  if (rh != 7) store_data_word(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rl);
	  imm  = (int8) fetch_byte_imm(m);
	  DECODE_PRINTF2(m,",%x\n",imm);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opc83_word_operation[rh])(m, *destreg, imm);
	  if (rh != 7) *destreg = destval;
	  break;
       }
 
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
 
}

/*opcode=0x84*/
void DEFUN(i86op_test_byte_RM_R,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int8       *destreg,*srcreg;
   u_int16      destoffset;
   u_int8       destval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"TEST\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  test_byte(m, destval, *srcreg);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  test_byte(m, destval, *srcreg);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  test_byte(m, destval, *srcreg);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  test_byte(m, *destreg, *srcreg);
	  break;
       }
   DECODE_CLEAR_SEGOVR(m);
	END_OF_INSTR(m);
}

/*opcode=0x85*/
void DEFUN(i86op_test_word_RM_R,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int16      *destreg,*srcreg;
   u_int16      destoffset;
   u_int16      destval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"TEST\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  test_word(m, destval, *srcreg);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  test_word(m, destval, *srcreg);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  test_word(m, destval, *srcreg);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  test_word(m, *destreg, *srcreg);
	  break;
       }
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x86*/
void DEFUN(i86op_xchg_byte_RM_R,(m),  PC_ENV *m) 
{
   
   u_int16      mod,rl,rh;
   u_int8       *destreg,*srcreg;
   u_int16      destoffset;
   u_int8       destval;
   u_int8       tmp;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"XCHG\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  tmp = *srcreg;
	  *srcreg = destval;
	  destval = tmp;
	  store_data_byte(m,destoffset,destval);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  tmp = *srcreg;
	  *srcreg = destval;
	  destval = tmp;
	  store_data_byte(m,destoffset,destval);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_byte(m,destoffset);
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  tmp = *srcreg;
	  *srcreg = destval;
	  destval = tmp;
	  store_data_byte(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  tmp = *srcreg;
	  *srcreg = *destreg;
	  *destreg = tmp;
	  break;
       }
   
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
 

}

/*opcode=0x87*/
void DEFUN(i86op_xchg_word_RM_R,(m),  PC_ENV *m) 
{

   u_int16      mod,rl,rh;
   u_int16      *destreg,*srcreg;
   u_int16      destoffset;
   u_int16      destval;
   u_int16      tmp;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"XCHG\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  tmp = *srcreg;
	  *srcreg = destval;
	  destval = tmp;
	  store_data_word(m,destoffset,destval);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  tmp = *srcreg;
	  *srcreg = destval;
	  destval = tmp;
	  store_data_word(m,destoffset,destval);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  destval = fetch_data_word(m,destoffset);
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  tmp = *srcreg;
	  *srcreg = destval;
	  destval = tmp;
	  store_data_word(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  tmp = *srcreg;
	  *srcreg = *destreg;
	  *destreg = tmp;
	  break;
       }
   DECODE_CLEAR_SEGOVR(m);
	END_OF_INSTR(m);
}

/*opcode=0x88*/
void DEFUN(i86op_mov_byte_RM_R,(m),  PC_ENV *m) 
{
   u_int16       mod,rl,rh;
   u_int8       *destreg,*srcreg;
   u_int16       destoffset;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  store_data_byte(m,destoffset,*srcreg);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  store_data_byte(m,destoffset,*srcreg);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  store_data_byte(m,destoffset,*srcreg);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = *srcreg;
	  break;
       }
   
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x89*/
void DEFUN(i86op_mov_word_RM_R,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int16      *destreg,*srcreg;
   u_int16      destoffset;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  store_data_word(m,destoffset,*srcreg);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  store_data_word(m,destoffset,*srcreg);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  store_data_word(m,destoffset,*srcreg);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = *srcreg;
	  break;
       }
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x8a*/
void DEFUN(i86op_mov_byte_R_RM,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int8       *destreg,*srcreg;
   u_int16      srcoffset;
   u_int8       srcval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:
	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm00_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = srcval;
	  break;
       
	case 1:

	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm01_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = srcval;
	  break;

	case 2:
	  
	  destreg  =  DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm10_address(m,rl);
	  srcval = fetch_data_byte(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = srcval;
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = *srcreg;
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

 /*opcode=0x8b*/
void DEFUN(i86op_mov_word_R_RM,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int16      *destreg,*srcreg;
   u_int16      srcoffset;
   u_int16      srcval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);
   switch (mod) 
       {
	case 0:
	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm00_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = srcval;
	  break;
       
	case 1:

	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm01_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = srcval;
	  break;

	case 2:
	  
	  destreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm10_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = srcval;
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = *srcreg;
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x8c*/
void DEFUN(i86op_mov_word_RM_SR,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int16      *destreg,*srcreg;
   u_int16      destoffset;
   u_int16      destval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  decode_rm_seg_register(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = *srcreg;
	  store_data_word(m,destoffset,destval);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  decode_rm_seg_register(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = *srcreg;
	  store_data_word(m,destoffset,destval);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  decode_rm_seg_register(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = *srcreg;
	  store_data_word(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,",");
	  srcreg  =  decode_rm_seg_register(m,rh);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = *srcreg;
	  break;
       }
   
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0x8d*/
void DEFUN(i86op_lea_word_R_M,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int16      *srcreg;
   u_int16      destoffset;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"LEA\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);
   
   switch (mod) 
       {
	case 0:
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *srcreg = destoffset;
	  break;
       
	case 1:

	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *srcreg = destoffset;
	  break;
       
	case 2:
	  
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *srcreg = destoffset;
	  break;

	case 3:   /* register to register */
	  /* undefined.  Do nothing. */
	  break;
	  
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x8e*/
void DEFUN(i86op_mov_word_SR_RM,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int16      *destreg,*srcreg;
   u_int16      srcoffset;
   u_int16      srcval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   switch (mod) 
       {
	case 0:
	  destreg  =  decode_rm_seg_register(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm00_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = srcval;
	  break;
       
	case 1:

	  destreg  =  decode_rm_seg_register(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm01_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = srcval;
	  break;

	case 2:
	  
	  destreg  =  decode_rm_seg_register(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset = decode_rm10_address(m,rl);
	  srcval = fetch_data_word(m,srcoffset);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = srcval;
	  break;
	  
	case 3:   /* register to register */
	  destreg  = decode_rm_seg_register(m,rh);
	  DECODE_PRINTF(m,",");
	  srcreg  =  DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = *srcreg;
	  break;
       }

   /*\
    *  clean up, and reset all the R_xSP pointers to the correct 
    *  locations.  This is about 3x too much overhead (doing all the 
    *  segreg ptrs when only one is needed, but this instruction 
    *  *cannot* be that common, and this isn't too much work anyway. 
   \*/

   RESYNCH_SEG_CS(m);  
   RESYNCH_SEG_DS(m);  
   RESYNCH_SEG_ES(m);  
   RESYNCH_SEG_SS(m);  
	  
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x8f*/
void DEFUN(i86op_pop_RM,(m),  PC_ENV *m) 
{

   u_int16      mod,rl,rh;
   u_int16      *destreg;
   u_int16      destoffset;
   u_int16      destval;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"POP\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   if (rh != 0) 
       {
	  DECODE_PRINTF(m,"ILLEGAL DECODE OF OPCODE 8F\n");
	  if (m->illegal_op())
	  {
		  halt_sys(m);
	  }
       }
   
   switch (mod) 
       {
	case 0:

	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = pop_word( m);
	  store_data_word(m,destoffset,destval);
	  break;
       
	case 1:

	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = pop_word(m);
	  store_data_word(m,destoffset,destval);
	  break;

	case 2:
	  
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = pop_word(m);
	  store_data_word(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = pop_word(m);
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0x90*/
void DEFUN(i86op_nop,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"NOP\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x91*/
void DEFUN(i86op_xchg_word_AX_CX,(m),  PC_ENV *m) 
{
   u_int16 tmp;
    
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"XCHG\tAX,CX\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   tmp = m->R_AX;
   m->R_AX = m->R_CX;
   m->R_CX = tmp;
 
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x92*/
void DEFUN(i86op_xchg_word_AX_DX,(m),  PC_ENV *m) 
{
   u_int16 tmp;
  
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"XCHG\tAX,DX\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   tmp = m->R_AX;
   m->R_AX = m->R_DX;
   m->R_DX = tmp;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x93*/
void DEFUN(i86op_xchg_word_AX_BX,(m),  PC_ENV *m) 
{
   u_int16 tmp;
  
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"XCHG\tAX,BX\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   tmp = m->R_AX;
   m->R_AX = m->R_BX;
   m->R_BX = tmp;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x94*/
void DEFUN(i86op_xchg_word_AX_SP,(m),  PC_ENV *m) 
{
   u_int16 tmp;
   
   START_OF_INSTR(m); 
   DECODE_PRINTF(m,"XCHG\tAX,SP\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   tmp = m->R_AX;
   m->R_AX = m->R_SP;
   m->R_SP = tmp;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x95*/
void DEFUN(i86op_xchg_word_AX_BP,(m),  PC_ENV *m) 
{
   u_int16 tmp;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"XCHG\tAX,BP\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   tmp = m->R_AX;
   m->R_AX = m->R_BP;
   m->R_BP = tmp;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x96*/
void DEFUN(i86op_xchg_word_AX_SI,(m),  PC_ENV *m) 
{
   u_int16 tmp;
  
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"XCHG\tAX,SI\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   tmp = m->R_AX;
   m->R_AX = m->R_SI;
   m->R_SI = tmp;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x97*/
void DEFUN(i86op_xchg_word_AX_DI,(m),  PC_ENV *m) 
{
   u_int16 tmp;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"XCHG\tAX,DI\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   tmp = m->R_AX;
   m->R_AX = m->R_DI;
   m->R_DI = tmp;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0x98*/
void DEFUN(i86op_cbw,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"CBW\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   if (m->R_AL & 0x80) 
       {
	  m->R_AH = 0xff;
       }
   else
       {
	  m->R_AH = 0x0;
       }
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x99*/
void DEFUN(i86op_cwd,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"CWD\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   if (m->R_AX & 0x8000) 
       {
	  m->R_DX = 0xffff;
       }
   else
       {
	  m->R_DX = 0x0;
       }
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x9a*/
void DEFUN(i86op_call_far_IMM,(m),  PC_ENV *m) 
{
   u_int16 farseg,faroff;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"CALL\t");
   faroff = fetch_word_imm(m);
   farseg = fetch_word_imm(m);
   DECODE_PRINTF2(m,"%04x:",farseg);
   DECODE_PRINTF2(m,"%04x\n",faroff);

   /* XXX 
      HOOKED INTERRUPT VECTORS CALLING INTO OUR "BIOS"
      WILL CAUSE PROBLEMS UNLESS ALL INTERSEGMENT STUFF IS 
      CHECKED FOR BIOS ACCESS.  CHECK NEEDED HERE.
      FOR MOMENT, LET IT ALONE.
    */

   TRACE_REGS(m);
   SINGLE_STEP(m);
   push_word(m,m->R_CS);
   m->R_CS = farseg;
   RESYNCH_SEG_CS(m);
   push_word(m,m->R_IP);
   m->R_IP = faroff;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x9b*/
void DEFUN(i86op_wait,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"WAIT\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   /* NADA.  */
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x9c*/
void DEFUN(i86op_pushf_word,(m),  PC_ENV *m) 
{
   u_int16 flags;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"PUSHF\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);

   PACK_FLAGS(m);
   flags = m->R_FLG;
   /* clear out *all* bits not representing flags */
   flags &= F_MSK;
   /* TURN ON CHARACTERISTIC BITS OF FLAG FOR 8088 */
   flags |= F_ALWAYS_ON;       
   push_word(m,flags);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x9d*/
void DEFUN(i86op_popf_word,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"POPF\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_FLG = pop_word(m);
   UNPACK_FLAGS(m);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x9e*/
void DEFUN(i86op_sahf,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"SAHF\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   PACK_FLAGS(m);
   /* clear the lower bits of the flag register */
   m->R_FLG &= 0xffffff00;
   /* or in the AH register into the flags register */
   m->R_FLG |= m->R_AH;
   UNPACK_FLAGS(m);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0x9f*/
void DEFUN(i86op_lahf,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"LAHF\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   PACK_FLAGS(m);
   m->R_AH  = m->R_FLG & 0xff;
   /*undocumented TC++ behavior??? Nope.  It's documented, but 
    you have too look real hard to notice it. */   
   m->R_AH  |= 0x2;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0xa0*/
void DEFUN(i86op_mov_AL_M_IMM,(m),  PC_ENV *m) 
{
   u_int16      offset;
   u_int8       destval;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\tAL,");
   offset = fetch_word_imm(m);
   DECODE_PRINTF2(m,"[%04x]\n",offset);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   destval = fetch_data_byte(m,offset);
   m->R_AL  = destval;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}



/*opcode=0xa1*/
void DEFUN(i86op_mov_AX_M_IMM,(m),  PC_ENV *m) 
{
   u_int16  offset;
   u_int16  destval;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\tAX,");
   offset = fetch_word_imm(m);
   DECODE_PRINTF2(m,"[%04x]\n",offset);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   destval = fetch_data_word(m,offset);
   m->R_AX  = destval;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xa2*/
void DEFUN(i86op_mov_M_AL_IMM,(m),  PC_ENV *m) 
{
   u_int16  offset;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\t");
   offset = fetch_word_imm(m);
   DECODE_PRINTF2(m,"[%04x],AL\n",offset);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   store_data_byte(m,offset,m->R_AL);   
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}



/*opcode=0xa3*/
void DEFUN(i86op_mov_M_AX_IMM,(m),  PC_ENV *m) 
{
   u_int16  offset;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\t");
   offset = fetch_word_imm(m);
   DECODE_PRINTF2(m,"[%04x],AX\n",offset);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   store_data_word(m,offset,m->R_AX);   
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/* JHH CLEANED */

/*opcode=0xa4*/
void DEFUN(i86op_movs_byte,(m),  PC_ENV *m) 
{
   u_int8 val;
   int  inc;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOVS\tBYTE\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   if (ACCESS_FLAG(m,F_DF)) /* down */
     inc = -1;
   else 
     inc = 1;
   
   if (m->sysmode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE))
       {
	  /* dont care whether REPE or REPNE */
	  /* move them until CX is ZERO. */
	  while (m->R_CX != 0)
	    {
	       val = fetch_data_byte(m,m->R_SI);
	       store_data_byte_abs(m,m->R_ES,m->R_DI,val);
	       m->R_CX -= 1;
	       m->R_SI += inc;
	       m->R_DI += inc;
	    }
	  m->sysmode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
       }
   else
       {
	  val = fetch_data_byte(m,m->R_SI);
	  store_data_byte_abs(m,m->R_ES,m->R_DI,val);
	  m->R_SI += inc;
	  m->R_DI += inc;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0xa5*/
void DEFUN(i86op_movs_word,(m),  PC_ENV *m) 
{
   int16 val;
   int  inc;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOVS\tWORD\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   if (ACCESS_FLAG(m, F_DF)) /* down */
     inc = -2;
   else 
     inc = 2;
   
   if (m->sysmode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE))
       {
	  /* dont care whether REPE or REPNE */
	  /* move them until CX is ZERO. */
	  while (m->R_CX != 0)
	    {
	       val = fetch_data_word(m,m->R_SI);
	       store_data_word_abs(m,m->R_ES,m->R_DI,val);
	       m->R_CX -= 1;
	       m->R_SI += inc;
	       m->R_DI += inc;
	    }
	  m->sysmode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
	  
       }
   else
       {
	  val = fetch_data_word(m,m->R_SI);
	  store_data_word_abs(m,m->R_ES,m->R_DI,val);
	  m->R_SI += inc;
	  m->R_DI += inc;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}




/*opcode=0xa6*/
void DEFUN(i86op_cmps_byte,(m),  PC_ENV *m) 
{
   int8 val1,val2;
   int  inc;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"CMPS\tBYTE\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   if (ACCESS_FLAG(m,F_DF)) /* down */
     inc = -1;
   else 
     inc = 1;
   
   if (m->sysmode & SYSMODE_PREFIX_REPE)
       {
	  /* REPE  */
	  /* move them until CX is ZERO. */
	  while (m->R_CX != 0)
	    {
	       val1 = fetch_data_byte(m,m->R_SI);
	       val2 = fetch_data_byte_abs(m,m->R_ES,m->R_DI);
	       cmp_byte(m, val1,val2);
	       m->R_CX -= 1;
	       m->R_SI += inc;
	       m->R_DI += inc;
	       if (ACCESS_FLAG(m,F_ZF)==0) break;   
	    }
	  m->sysmode &= ~SYSMODE_PREFIX_REPE;
       }
   else if (m->sysmode & SYSMODE_PREFIX_REPNE)
       {
	  /* REPNE  */
	  /* move them until CX is ZERO. */
	  while (m->R_CX != 0)
	    {
	       val1 = fetch_data_byte(m,m->R_SI);
	       val2 = fetch_data_byte_abs(m,m->R_ES,m->R_DI);
	       cmp_byte(m, val1,val2);
	       m->R_CX -= 1;
	       m->R_SI += inc;
	       m->R_DI += inc;
	       if (ACCESS_FLAG(m,F_ZF)) break;  /* zero flag set means equal */
	    }
	  m->sysmode &= ~SYSMODE_PREFIX_REPNE;
       }
   else
       {
	  val1 = fetch_data_byte(m,m->R_SI);
	  val2 = fetch_data_byte_abs(m,m->R_ES,m->R_DI);
	  cmp_byte(m, val1,val2);
	  m->R_SI += inc;
	  m->R_DI += inc;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0xa7*/
void DEFUN(i86op_cmps_word,(m),  PC_ENV *m) 
{
   int16 val1,val2;
   int  inc;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"CMPS\tWORD\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   if (ACCESS_FLAG(m,F_DF)) /* down */
     inc = -2;
   else 
     inc = 2;
   
   if (m->sysmode & SYSMODE_PREFIX_REPE)
       {

	  /* REPE  */
	  /* move them until CX is ZERO. */
	  while (m->R_CX != 0)
	    {
	       val1 = fetch_data_word(m,m->R_SI);
	       val2 = fetch_data_word_abs(m,m->R_ES,m->R_DI);
	       cmp_word(m, val1,val2);
	       m->R_CX -= 1;
	       m->R_SI += inc;
	       m->R_DI += inc;
	       if (ACCESS_FLAG(m,F_ZF)==0) break;   
	    }
	  m->sysmode &= ~SYSMODE_PREFIX_REPE;
       }
   else if (m->sysmode & SYSMODE_PREFIX_REPNE)
       {
	  /* REPNE  */
	  /* move them until CX is ZERO. */
	  while (m->R_CX != 0)
	    {
	       val1 = fetch_data_word(m,m->R_SI);
	       val2 = fetch_data_word_abs(m,m->R_ES,m->R_DI);
	       cmp_word(m, val1,val2);
	       m->R_CX -= 1;
	       m->R_SI += inc;
	       m->R_DI += inc;	
	       if (ACCESS_FLAG(m,F_ZF)) break;  /* zero flag set means equal */
	    }
	  m->sysmode &= ~SYSMODE_PREFIX_REPNE;
       }
   else
       {
	  val1 = fetch_data_word(m,m->R_SI);
	  val2 = fetch_data_word_abs(m,m->R_ES,m->R_DI);
	  cmp_word(m, val1,val2);
	  m->R_SI += inc;
	  m->R_DI += inc;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}




/*opcode=0xa8*/
void DEFUN(i86op_test_AL_IMM,(m),  PC_ENV *m) 
{
   int  imm;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"TEST\tAL,");
   imm = fetch_byte_imm(m);
   DECODE_PRINTF2(m,"%04x\n",imm);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   test_byte(m, m->R_AL, imm);
   DECODE_CLEAR_SEGOVR(m);
	END_OF_INSTR(m);
}

/*opcode=0xa9*/
void DEFUN(i86op_test_AX_IMM,(m),  PC_ENV *m) 
{
   int  imm;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"TEST\tAX,");
   imm = fetch_word_imm(m);
   DECODE_PRINTF2(m,"%04x\n",imm);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   test_word(m, m->R_AX, imm);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}




/*opcode=0xaa*/
void DEFUN(i86op_stos_byte,(m),  PC_ENV *m) 
{
   int  inc;
 
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"STOS\tBYTE\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   if (ACCESS_FLAG(m, F_DF)) /* down */
     inc = -1;
   else 
     inc = 1;
   if (m->sysmode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE))
       {
	  /* dont care whether REPE or REPNE */
	  /* move them until CX is ZERO. */
	  while (m->R_CX != 0)
	    {
	       store_data_byte_abs(m,m->R_ES,m->R_DI,m->R_AL);
	       m->R_CX -= 1;
	       m->R_DI += inc;
	    }
	  m->sysmode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
       }
   else
       {
	  store_data_byte_abs(m,m->R_ES,m->R_DI,m->R_AL);
	  m->R_DI += inc;
       }
   
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0xab*/
void DEFUN(i86op_stos_word,(m),  PC_ENV *m) 
{
   int   inc;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"STOS\tWORD\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   if (ACCESS_FLAG(m, F_DF)) /* down */
     inc = -2;
   else 
     inc = 2;
   if (m->sysmode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE))
       {
	  /* dont care whether REPE or REPNE */
	  /* move them until CX is ZERO. */
	  while (m->R_CX != 0)
	    {
	       store_data_word_abs(m,m->R_ES,m->R_DI,m->R_AX);
	       m->R_CX -= 1;
	       m->R_DI += inc;
	    }
	  m->sysmode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
	  
       }
   else
       {
	  store_data_word_abs(m,m->R_ES,m->R_DI,m->R_AX);
	  m->R_DI += inc;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}







/*opcode=0xac*/
void DEFUN(i86op_lods_byte,(m),  PC_ENV *m) 
{
   int  inc;
 
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"LODS\tBYTE\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   if (ACCESS_FLAG(m,F_DF)) /* down */
     inc = -1;
   else 
     inc = 1;
   if (m->sysmode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE))
       {
	  /* dont care whether REPE or REPNE */
	  /* move them until CX is ZERO. */
	  while (m->R_CX != 0)
	    {
	       m->R_AL = fetch_data_byte(m,m->R_SI);
	       m->R_CX -= 1;
	       m->R_SI += inc;
	    }
	  m->sysmode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
       }
   else
       {
	  m->R_AL = fetch_data_byte(m,m->R_SI);
	  m->R_SI += inc;
       }
   
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0xad*/
void DEFUN(i86op_lods_word,(m),  PC_ENV *m) 
{
   int   inc;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"LODS\tWORD\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   if (ACCESS_FLAG(m,F_DF)) /* down */
     inc = -2;
   else 
     inc = 2;
   if (m->sysmode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE))
       {
	  /* dont care whether REPE or REPNE */
	  /* move them until CX is ZERO. */
	  while (m->R_CX != 0)
	    {
	       m->R_AX = fetch_data_word(m,m->R_SI);
	       m->R_CX -= 1;
	       m->R_SI += inc;
	    }
	  m->sysmode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
	  
       }
   else
       {
	  m->R_AX = fetch_data_word(m,m->R_SI);
	  m->R_SI += inc;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0xae*/
void DEFUN(i86op_scas_byte,(m),  PC_ENV *m) 
{
   int8 val2;
   int  inc;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"SCAS\tBYTE\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   if (ACCESS_FLAG(m,F_DF)) /* down */
     inc = -1;
   else 
     inc = 1;
   
   if (m->sysmode & SYSMODE_PREFIX_REPE)
       {

	  /* REPE  */
	  /* move them until CX is ZERO. */
	  while (m->R_CX != 0)
	    {
	       val2 = fetch_data_byte_abs(m,m->R_ES,m->R_DI);
	       cmp_byte(m, m->R_AL,val2);
	       m->R_CX -= 1;
	       m->R_DI += inc;
	       if (ACCESS_FLAG(m,F_ZF)==0) break;   
	    }
	  m->sysmode &= ~SYSMODE_PREFIX_REPE;
       }
   else if (m->sysmode & SYSMODE_PREFIX_REPNE)
       {
	  /* REPNE  */
	  /* move them until CX is ZERO. */
	  while (m->R_CX != 0)
	    {
	       val2 = fetch_data_byte_abs(m,m->R_ES,m->R_DI);
	       cmp_byte(m, m->R_AL,val2);
	       m->R_CX -= 1;
	       m->R_DI += inc;
	       if (ACCESS_FLAG(m,F_ZF)) break;  /* zero flag set means equal */
	    }
	  m->sysmode &= ~SYSMODE_PREFIX_REPNE;
       }
   else
       {
	  val2 = fetch_data_byte_abs(m,m->R_ES,m->R_DI);
	  cmp_byte(m, m->R_AL,val2);
	  m->R_DI += inc;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0xaf*/
void DEFUN(i86op_scas_word,(m),  PC_ENV *m) 
{
   int16 val2;
   int  inc;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"SCAS\tWORD\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   if (ACCESS_FLAG(m, F_DF)) /* down */
     inc = -2;
   else 
     inc = 2;
   
   if (m->sysmode & SYSMODE_PREFIX_REPE)
       {

	  /* REPE  */
	  /* move them until CX is ZERO. */
	  while (m->R_CX != 0)
	    {
	       val2 = fetch_data_word_abs(m,m->R_ES,m->R_DI);
	       cmp_word(m,m->R_AX,val2);
	       m->R_CX -= 1;
	       m->R_DI += inc;
	       if (ACCESS_FLAG(m,F_ZF)==0) break;   
	    }
	  m->sysmode &= ~SYSMODE_PREFIX_REPE;
       }
   else if (m->sysmode & SYSMODE_PREFIX_REPNE)
       {
	  /* REPNE  */
	  /* move them until CX is ZERO. */
	  while (m->R_CX != 0)
	    {
	       val2 = fetch_data_word_abs(m,m->R_ES,m->R_DI);
	       cmp_word(m, m->R_AX,val2);
	       m->R_CX -= 1;
	       m->R_DI += inc;
	       if (ACCESS_FLAG(m,F_ZF)) break;  /* zero flag set means equal */
	    }
	  m->sysmode &= ~SYSMODE_PREFIX_REPNE;
       }
   else
       {
	  val2 = fetch_data_word_abs(m,m->R_ES,m->R_DI);
	  cmp_word(m, m->R_AX,val2);
	  m->R_DI += inc;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0xb0*/
void DEFUN(i86op_mov_byte_AL_IMM,(m),  PC_ENV *m) 
{
   u_int8 imm;
  
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\tAL,");
   imm  =  fetch_byte_imm(m);
   DECODE_PRINTF2(m,"%x\n",imm);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AL = imm;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xb1*/
void DEFUN(i86op_mov_byte_CL_IMM,(m),  PC_ENV *m) 
{
   u_int8 imm;
  
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\tCL,");
   imm  =  fetch_byte_imm(m);
   DECODE_PRINTF2(m,"%x\n",imm);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_CL = imm;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xb2*/
void DEFUN(i86op_mov_byte_DL_IMM,(m),  PC_ENV *m) 
{
   u_int8 imm;
  
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\tDL,");
   imm  =  fetch_byte_imm(m);
   DECODE_PRINTF2(m,"%x\n",imm);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_DL = imm;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xb3*/
void DEFUN(i86op_mov_byte_BL_IMM,(m),  PC_ENV *m) 
{
   u_int8 imm;
  
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\tBL,");
   imm  =  fetch_byte_imm(m);
   DECODE_PRINTF2(m,"%x\n",imm);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_BL = imm;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xb4*/
void DEFUN(i86op_mov_byte_AH_IMM,(m),  PC_ENV *m) 
{
   u_int8 imm;
  
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\tAH,");
   imm  =  fetch_byte_imm(m);
   DECODE_PRINTF2(m,"%x\n",imm);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AH = imm;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xb5*/
void DEFUN(i86op_mov_byte_CH_IMM,(m),  PC_ENV *m) 
{
   u_int8 imm;
  
   START_OF_INSTR(m); 
   DECODE_PRINTF(m,"MOV\tCH,");
   imm  =  fetch_byte_imm(m);
   DECODE_PRINTF2(m,"%x\n",imm);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_CH = imm;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xb6*/
void DEFUN(i86op_mov_byte_DH_IMM,(m),  PC_ENV *m) 
{
   u_int8 imm;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\tDH,");
   imm  =  fetch_byte_imm(m);
   DECODE_PRINTF2(m,"%x\n",imm);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_DH = imm;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xb7*/
void DEFUN(i86op_mov_byte_BH_IMM,(m),  PC_ENV *m) 
{
   u_int8 imm;
  
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\tBH,");
   imm  =  fetch_byte_imm(m);
   DECODE_PRINTF2(m,"%x\n",imm);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_BH = imm;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}



/*opcode=0xb8*/
void DEFUN(i86op_mov_word_AX_IMM,(m),  PC_ENV *m) 
{
   u_int16 imm;
  
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\tAX,");
   imm  =  fetch_word_imm(m);
   DECODE_PRINTF2(m,"%x\n",imm);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AX = imm;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xb9*/
void DEFUN(i86op_mov_word_CX_IMM,(m),  PC_ENV *m) 
{
   u_int16 imm;
 
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\tCX,");
   imm  =  fetch_word_imm(m);
   DECODE_PRINTF2(m,"%x\n",imm);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_CX = imm;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xba*/
void DEFUN(i86op_mov_word_DX_IMM,(m),  PC_ENV *m) 
{
   u_int16 imm;
  
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\tDX,");
   imm  =  fetch_word_imm(m);
   DECODE_PRINTF2(m,"%x\n",imm);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_DX = imm;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xbb*/
void DEFUN(i86op_mov_word_BX_IMM,(m),  PC_ENV *m) 
{
   u_int16 imm;
  
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\tBX,");
   imm  =  fetch_word_imm(m);
   DECODE_PRINTF2(m,"%x\n",imm);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_BX = imm;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xbc*/
void DEFUN(i86op_mov_word_SP_IMM,(m),  PC_ENV *m) 
{
   u_int16 imm;
  
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\tSP,");
   imm  =  fetch_word_imm(m);
   DECODE_PRINTF2(m,"%x\n",imm);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_SP = imm;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xbd*/
void DEFUN(i86op_mov_word_BP_IMM,(m),  PC_ENV *m) 
{
   u_int16 imm;
  
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\tBP,");
   imm  =  fetch_word_imm(m);
   DECODE_PRINTF2(m,"%x\n",imm);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_BP = imm;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xbe*/
void DEFUN(i86op_mov_word_SI_IMM,(m),  PC_ENV *m) 
{
   u_int16 imm;
  
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\tSI,");
   imm  =  fetch_word_imm(m);
   DECODE_PRINTF2(m,"%x\n",imm);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_SI = imm;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xbf*/
void DEFUN(i86op_mov_word_DI_IMM,(m),  PC_ENV *m) 
{
   u_int16 imm;
  
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\tDI,");
   imm  =  fetch_word_imm(m);
   DECODE_PRINTF2(m,"%x\n",imm);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_DI = imm;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}



/* c0 === ILLEGAL OPERAND */
/* c1 === ILLEGAL OPERAND */



/*opcode=0xc2*/
void DEFUN(i86op_ret_near_IMM,(m),  PC_ENV *m) 
{
   u_int16 imm;
   
  
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"RET\t");
   imm  =  fetch_word_imm(m);
   DECODE_PRINTF2(m,"%x\n",imm);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_IP = pop_word(m);
   m->R_SP += imm;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0xc3*/
void DEFUN(i86op_ret_near,(m),  PC_ENV *m) 
{

  
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"RET\n");
   RETURN_TRACE(m,m->saved_ip);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_IP = pop_word(m);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0xc4*/
void DEFUN(i86op_les_R_IMM,(m),  PC_ENV *m) 
{
   u_int16      mod,rh,rl;
   u_int16      *dstreg;
   u_int16      srcoffset;
   
  
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"LES\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);
   switch (mod) 
       {
	case 0:

	  dstreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *dstreg = fetch_data_word(m,srcoffset);
	  m->R_ES = fetch_data_word(m,srcoffset+2);
	  RESYNCH_SEG_ES(m);
	  break;
       
	case 1:

	  dstreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *dstreg = fetch_data_word(m,srcoffset);
	  m->R_ES = fetch_data_word(m,srcoffset+2);
	  RESYNCH_SEG_ES(m);
	  break;

	case 2:
	  

	  dstreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *dstreg = fetch_data_word(m,srcoffset);
	  m->R_ES = fetch_data_word(m,srcoffset+2);
	  RESYNCH_SEG_ES(m);
	  break;
	  
	case 3:   /* register to register */
	  /* UNDEFINED! */
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0xc5*/
void DEFUN(i86op_lds_R_IMM,(m),  PC_ENV *m) 
{
   u_int16      mod,rh,rl;
   u_int16      *dstreg;
   u_int16      srcoffset;
   
  
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"LDS\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);
   switch (mod) 
       {
	case 0:

	  dstreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *dstreg = fetch_data_word(m,srcoffset);
	  m->R_DS = fetch_data_word(m,srcoffset+2);
	  RESYNCH_SEG_DS(m);
	  break;
       
	case 1:

	  dstreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *dstreg = fetch_data_word(m,srcoffset);
	  m->R_DS = fetch_data_word(m,srcoffset+2);
	  RESYNCH_SEG_DS(m);
	  break;

	case 2:
	  

	  dstreg  =  DECODE_RM_WORD_REGISTER(m,rh);
	  DECODE_PRINTF(m,",");
	  srcoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,"\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *dstreg = fetch_data_word(m,srcoffset);
	  m->R_DS = fetch_data_word(m,srcoffset+2);
	  RESYNCH_SEG_DS(m);
	  break;
	  
	case 3:   /* register to register */
	  /* UNDEFINED! */
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0xc6*/
void DEFUN(i86op_mov_byte_RM_IMM,(m),  PC_ENV *m) 
{

   u_int16       mod,rl,rh;
   u_int8         *destreg;
   u_int16           destoffset;
   u_int8           imm;

   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   if (rh != 0) 
       {
	  DECODE_PRINTF(m,"ILLEGAL DECODE OF OPCODE c6\n");
	  if (m->illegal_op())
	  {
		  halt_sys(m);
	  }
       }
   
   switch (mod) 
       {
	case 0:
	  DECODE_PRINTF(m,"BYTE PTR ");
	  destoffset=decode_rm00_address(m,rl);
	  imm = fetch_byte_imm(m);
	  DECODE_PRINTF2(m,",%2x\n",imm);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  store_data_byte(m,destoffset,imm);
	  break;
       
	case 1:
	  DECODE_PRINTF(m,"BYTE PTR ");
	  destoffset=decode_rm01_address(m,rl);
	  imm = fetch_byte_imm(m);
	  DECODE_PRINTF2(m,",%2x\n",imm);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  store_data_byte(m,destoffset,imm);
	  break;

	case 2:
	  DECODE_PRINTF(m,"BYTE PTR ");
	  destoffset=decode_rm10_address(m,rl);
	  imm = fetch_byte_imm(m);
	  DECODE_PRINTF2(m,",%2x\n",imm);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  store_data_byte(m,destoffset,imm);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rl);
	  imm = fetch_byte_imm(m);
	  DECODE_PRINTF2(m,",%2x\n",imm);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg =  imm;
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0xc7*/
void DEFUN(i86op_mov_word_RM_IMM,(m),  PC_ENV *m) 
{

   u_int16      mod,rl,rh;
   u_int16      *destreg;
   u_int16      destoffset;
   u_int16      imm;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"MOV\t");
   FETCH_DECODE_MODRM(m,mod,rh,rl);

   if (rh != 0) 
       {
	  DECODE_PRINTF(m,"ILLEGAL DECODE OF OPCODE 8F\n");
	  if (m->illegal_op())
	  {
		  halt_sys(m);
	  }
       }
   
   switch (mod) 
       {
	case 0:

	  DECODE_PRINTF(m,"WORD PTR ");
	  destoffset=decode_rm00_address(m,rl);
	  imm = fetch_word_imm(m);
	  DECODE_PRINTF2(m,",%x\n",imm);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  store_data_word(m,destoffset,imm);
	  break;
       
	case 1:
	  DECODE_PRINTF(m,"WORD PTR ");
	  destoffset=decode_rm01_address(m,rl);
	  imm = fetch_word_imm(m);
	  DECODE_PRINTF2(m,",%x\n",imm);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  store_data_word(m,destoffset,imm);
	  break;

	case 2:
	  DECODE_PRINTF(m,"WORD PTR ");
	  destoffset=decode_rm10_address(m,rl);
	  imm = fetch_word_imm(m);
	  DECODE_PRINTF2(m,",%x\n",imm);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  store_data_word(m,destoffset,imm);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rl);
	  imm = fetch_word_imm(m);
	  DECODE_PRINTF2(m,",%2x\n",imm);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = imm;
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
	END_OF_INSTR(m);
}





/*opcode=0xc8  ILLEGAL OP*/
/*opcode=0xc9  ILLEGAL OP*/

/*opcode=0xca*/
void DEFUN(i86op_ret_far_IMM,(m),  PC_ENV *m) 
{
   u_int16 imm;
 
  
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"RETF\t");
   imm  =  fetch_word_imm(m);
   DECODE_PRINTF2(m,"%x\n",imm);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_IP = pop_word(m);
   m->R_CS = pop_word(m);
   RESYNCH_SEG_CS(m);
   m->R_SP += imm;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}



/*opcode=0xcb*/
void DEFUN(i86op_ret_far,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"RETF\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_IP = pop_word(m);
   m->R_CS = pop_word(m);
   RESYNCH_SEG_CS(m);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/* opcode=0xcc*/
void DEFUN(i86op_int3,(m),  PC_ENV *m) 
 {

    u_int16 tmp;
    
    START_OF_INSTR(m); 
    DECODE_PRINTF(m,"INT 3\n");
    tmp = (u_int16) mem_access_word(m, 3 * 4);  
    /* access the segment register */
    TRACE_REGS(m);
    SINGLE_STEP(m);
/*
    if (tmp == BIOS_SEG) 
	{
	   // fake the call.  Were going to simulate the 
	   // bios and not code it in 80x86 and interpret it.

	   
	   (*intr_tab[3])(3,m);
	   RESYNCH_SEG_CS(m);
	   RESYNCH_SEG_DS(m);
	   RESYNCH_SEG_ES(m);
	   RESYNCH_SEG_SS(m);
	}
    else */
	{
	   PACK_FLAGS(m);
	   tmp = m->R_FLG;
	   push_word(m, tmp);
	   CLEAR_FLAG(m, F_IF);
	   CLEAR_FLAG(m, F_TF);
/* [JCE] If we're interrupting between a segment override (or REP override) 
 * and the following instruction, decrease IP to get back to the prefix */
	   if (m->sysmode &    (SYSMODE_SEGMASK     | 
				SYSMODE_PREFIX_REPE | 
				SYSMODE_PREFIX_REPNE))
	   {
//		  fprintf(stderr, "Backstepping for prefix\n");
		  --m->R_IP;
	   }
	   push_word(m, m->R_CS);
	   push_word(m, m->R_IP);
/* [JCE] CS and IP were the wrong way round... */
	   tmp = mem_access_word(m, 3 * 4);
	   m->R_IP = tmp;
	   tmp = mem_access_word(m, 3 * 4 + 2);
	   m->R_CS = tmp;
	   RESYNCH_SEG_CS(m);
	}
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);    
 }

/* opcode=0xcd*/
void DEFUN(i86op_int_IMM,(m),  PC_ENV *m) 
 {

    u_int16 tmp;
    u_int8 intnum;
    
    START_OF_INSTR(m);
    DECODE_PRINTF(m,"INT\t");
    intnum = fetch_byte_imm(m);
    DECODE_PRINTF2(m,"%x\n", intnum);
    tmp = mem_access_word(m, intnum * 4);
    TRACE_REGS(m);
    SINGLE_STEP(m);
//    if (intnum == 0xFC) 
//    {
//	if (m->R_BX == 0x34 && m->R_CX == 0x20) trace_on(m);
//	fprintf(stderr, "%04x:%04x INT FC / BX = %04x / CX = %04x / DX = %04x\n",
//			m->R_CS, m->R_IP, m->R_BX, m->R_CX, m->R_DX);
//    }
//    if (intnum == 0x21) log_int21(stderr, m);
//    if (intnum == 0x88 && m->R_AL == 6)
//    {
//	fprintf(stderr, "INT 88 / AL = 6 / BL='%c'\n", m->R_BL);
//    }
//    else if (intnum >= 0x88) fprintf(stderr, "INT %02x / AX=%04x CS:IP=%04x:%04x\n", 
//			intnum, m->R_AX, m->R_CS, m->R_IP);
/*
    if (tmp == BIOS_SEG) 
	{
	   // fake the call.  Were going to simulate the 
	   // bios and not code it in 80x86 and interpret it.
	     
	   (* intr_tab[intnum] ) (intnum,m);
	   RESYNCH_SEG_CS(m);
	   RESYNCH_SEG_DS(m);
	   RESYNCH_SEG_ES(m);
	   RESYNCH_SEG_SS(m);
	}
    
    else */
	{
	   PACK_FLAGS(m);
	   tmp = m->R_FLG;
	   push_word(m, tmp);
	   CLEAR_FLAG(m, F_IF);
	   CLEAR_FLAG(m, F_TF);
/* [JCE] If we're interrupting between a segment override (or REP override) 
 * and the following instruction, decrease IP to get back to the prefix */
	   if (m->sysmode &    (SYSMODE_SEGMASK     | 
				SYSMODE_PREFIX_REPE | 
				SYSMODE_PREFIX_REPNE))
	   {
//		  fprintf(stderr, "Backstepping for prefix\n");
		  --m->R_IP;
	   }
	   push_word(m, m->R_CS);
	   push_word(m, m->R_IP);
/* [JCE] CS and IP were the wrong way round... */

	   tmp = mem_access_word(m, intnum * 4);
	   m->R_IP = tmp;
	   tmp = mem_access_word(m, intnum * 4 + 2);
	   m->R_CS = tmp;
	   RESYNCH_SEG_CS(m);
	}
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
 }
    
    
/* opcode=0xce*/
void DEFUN(i86op_into,(m),  PC_ENV *m) 
 {
    u_int16 tmp;
    
    START_OF_INSTR(m);
    DECODE_PRINTF(m,"INTO\n");

    TRACE_REGS(m);
    SINGLE_STEP(m);

    if (ACCESS_FLAG(m,F_OF))
	{
	   tmp = mem_access_word(m, 4 * 4);  
/*	   if (tmp == BIOS_SEG) 
	       {
		  // fake the call.  Were going to simulate the 
		  // bios and not code it in 80x86 and interpret it.
		  // 
		  
		  (*intr_tab[4])(4,m);
		  RESYNCH_SEG_CS(m);
		  RESYNCH_SEG_DS(m);
		  RESYNCH_SEG_ES(m);
		  RESYNCH_SEG_SS(m);
	       }
	   else */
	       {
		  PACK_FLAGS(m);
		  tmp = m->R_FLG;
		  push_word(m, tmp);
		  CLEAR_FLAG(m, F_IF);
		  CLEAR_FLAG(m, F_TF);
/* [JCE] If we're interrupting between a segment override (or REP override) 
 * and the following instruction, decrease IP to get back to the prefix */
		  if (m->sysmode &    (SYSMODE_SEGMASK     | 
					SYSMODE_PREFIX_REPE | 
					SYSMODE_PREFIX_REPNE))
		  {
//			  fprintf(stderr, "Backstepping for prefix\n");
			  --m->R_IP;
		  }


		  push_word(m, m->R_CS);
		  push_word(m, m->R_IP);
/* [JCE] CS and IP were the wrong way round... */
		  tmp = mem_access_word(m, 4 * 4);
		  m->R_IP = tmp;
		  tmp = mem_access_word(m, 4 * 4 + 2);
		  m->R_CS = tmp;
		  RESYNCH_SEG_CS(m);
	       }
	}
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);    
    
 }

/* opcode=0xcf*/
void DEFUN(i86op_iret,(m),  PC_ENV *m) 
 {
    START_OF_INSTR(m);   
    DECODE_PRINTF(m,"IRET\n");

    TRACE_REGS(m);
    SINGLE_STEP(m);
    m->R_IP = pop_word(m);
    m->R_CS = pop_word(m);
    RESYNCH_SEG_CS(m);
    m->R_FLG = pop_word(m);
    UNPACK_FLAGS(m);
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
 }



#ifdef __STDC__
static u_int8 (*opcD0_byte_operation[])(PC_ENV *m,u_int8 d, u_int8 s) = 
#else
static u_int8 (*opcD0_byte_operation[])() = 
#endif
      /* used by opcodes d0 and d2. */
 {
    rol_byte,
    ror_byte,
    rcl_byte,
    rcr_byte,
    shl_byte,
    shr_byte,
    shl_byte,   /* sal_byte === shl_byte  by definition */
    sar_byte,
 };
    
    

/* opcode=0xd0*/
void DEFUN(i86op_opcD0_byte_RM_1,(m),  PC_ENV *m) 
 {
   u_int16      mod,rl,rh;
   u_int8       *destreg;
   u_int16      destoffset;
   u_int8       destval;

   
   /* Yet another weirdo special case instruction format.  Part of the 
      opcode held below in "RH".  Doubly nested case would 
      result, except that the decoded instruction 
    */
   START_OF_INSTR(m);
   FETCH_DECODE_MODRM(m,mod,rh,rl);

#ifdef DEBUG
   if (DEBUG_DECODE(m))
     {
	/* XXX DECODE_PRINTF may be changed to something more
	   general, so that it is important to leave the strings
	   in the same format, even though the result is that the 
	   above test is done twice. */

	switch (rh)
	    {
	     case 0:
	       DECODE_PRINTF(m,"ROL\t");
	       break;
	     case 1:
	       DECODE_PRINTF(m,"ROR\t");
	       break;
	     case 2:
	       DECODE_PRINTF(m,"RCL\t");
	       break;
	     case 3:
	       DECODE_PRINTF(m,"RCR\t");
	       break;
	     case 4:
	       DECODE_PRINTF(m,"SHL\t");
	       break;
	     case 5:
	       DECODE_PRINTF(m,"SHR\t");
	       break;
	     case 6:
	       DECODE_PRINTF(m,"SAL\t");
	       break;
	     case 7:
	       DECODE_PRINTF(m,"SAR\t");
	       break;
	    }	  
     }
   
#endif	  
   
   /* know operation, decode the mod byte to find the addressing 
      mode. */
   switch (mod) 
       {
	case 0:
	  DECODE_PRINTF(m,"BYTE PTR ");
	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",1\n");
	  destval = fetch_data_byte(m,destoffset);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opcD0_byte_operation[rh])(m, destval,1);
	  store_data_byte(m,destoffset,destval);
	  break;
       
	case 1:

	  DECODE_PRINTF(m,"BYTE PTR ");
	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",1\n");
	  destval = fetch_data_byte(m,destoffset);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opcD0_byte_operation[rh])(m, destval, 1);
	  store_data_byte(m,destoffset,destval);
	  break;

	case 2:
	  
	  DECODE_PRINTF(m,"BYTE PTR ");
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",1\n");
	  destval = fetch_data_byte(m,destoffset);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opcD0_byte_operation[rh])(m, destval, 1);
	  store_data_byte(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,",1\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opcD0_byte_operation[rh])(m, *destreg, 1);
	  *destreg = destval;
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
	END_OF_INSTR(m);
 }


#ifdef __STDC__
static u_int16 (*opcD1_word_operation[])(PC_ENV *m,u_int16 s,u_int16 d) = 
#else
static u_int16 (*opcD1_word_operation[])() = 
#endif
      /* used by opcodes d1 and d3. */
 {
    rol_word,
    ror_word,
    rcl_word,
    rcr_word,
    shl_word,
    shr_word,
    shl_word,   /* sal_byte === shl_byte  by definition */
    sar_word,
 };
    

/* opcode=0xd1*/
void DEFUN(i86op_opcD1_word_RM_1,(m),  PC_ENV *m) 
 {
   u_int16      mod,rl,rh;
   u_int16      *destreg;
   u_int16      destoffset;
   u_int16      destval;

   
   /* Yet another weirdo special case instruction format.  Part of the 
      opcode held below in "RH".  Doubly nested case would 
      result, except that the decoded instruction 
    */
   START_OF_INSTR(m);
   FETCH_DECODE_MODRM(m,mod,rh,rl);

#ifdef DEBUG
   if (DEBUG_DECODE(m))
     {
	/* XXX DECODE_PRINTF may be changed to something more
	   general, so that it is important to leave the strings
	   in the same format, even though the result is that the 
	   above test is done twice. */

	switch (rh)
	    {
	     case 0:
	       DECODE_PRINTF(m,"ROL\t");
	       break;
	     case 1:
	       DECODE_PRINTF(m,"ROR\t");
	       break;
	     case 2:
	       DECODE_PRINTF(m,"RCL\t");
	       break;
	     case 3:
	       DECODE_PRINTF(m,"RCR\t");
	       break;
	     case 4:
	       DECODE_PRINTF(m,"SHL\t");
	       break;
	     case 5:
	       DECODE_PRINTF(m,"SHR\t");
	       break;
	     case 6:
	       DECODE_PRINTF(m,"SAL\t");
	       break;
	     case 7:
	       DECODE_PRINTF(m,"SAR\t");
	       break;
	    }	  
     }
   
#endif	  
   
   /* know operation, decode the mod byte to find the addressing 
      mode. */
   switch (mod) 
       {
	case 0:
	  DECODE_PRINTF(m,"WORD PTR ");
	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",1\n");
	  destval = fetch_data_word(m,destoffset);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opcD1_word_operation[rh])(m, destval,1);
	  store_data_word(m,destoffset,destval);
	  break;
       
	case 1:

	  DECODE_PRINTF(m,"WORD PTR ");
	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",1\n");
	  destval = fetch_data_word(m,destoffset);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opcD1_word_operation[rh])(m, destval, 1);
	  store_data_word(m,destoffset,destval);
	  break;

	case 2:
	  
	  DECODE_PRINTF(m,"BYTE PTR ");
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",1\n");
	  destval = fetch_data_word(m,destoffset);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opcD1_word_operation[rh])(m, destval, 1);
	  store_data_word(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,",1\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opcD1_word_operation[rh])(m, *destreg, 1);
	  *destreg = destval;
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
 }


/* opcode=0xd2*/
void DEFUN(i86op_opcD2_byte_RM_CL,(m),  PC_ENV *m) 

 {
   u_int16      mod,rl,rh;
   u_int8       *destreg;
   u_int16      destoffset;
   u_int8       destval;
   u_int8       amt;
   
   
   /* Yet another weirdo special case instruction format.  Part of the 
      opcode held below in "RH".  Doubly nested case would 
      result, except that the decoded instruction 
    */
   START_OF_INSTR(m);
   FETCH_DECODE_MODRM(m,mod,rh,rl);

#ifdef DEBUG
   if (DEBUG_DECODE(m))
     {
	/* XXX DECODE_PRINTF may be changed to something more
	   general, so that it is important to leave the strings
	   in the same format, even though the result is that the 
	   above test is done twice. */

	switch (rh)
	    {
	     case 0:
	       DECODE_PRINTF(m,"ROL\t");
	       break;
	     case 1:
	       DECODE_PRINTF(m,"ROR\t");
	       break;
	     case 2:
	       DECODE_PRINTF(m,"RCL\t");
	       break;
	     case 3:
	       DECODE_PRINTF(m,"RCR\t");
	       break;
	     case 4:
	       DECODE_PRINTF(m,"SHL\t");
	       break;
	     case 5:
	       DECODE_PRINTF(m,"SHR\t");
	       break;
	     case 6:
	       DECODE_PRINTF(m,"SAL\t");
	       break;
	     case 7:
	       DECODE_PRINTF(m,"SAR\t");
	       break;
	    }	  
     }
   
#endif	  

   amt = m->R_CL;
   
   /* know operation, decode the mod byte to find the addressing 
      mode. */
   switch (mod) 
       {
	case 0:
	  DECODE_PRINTF(m,"BYTE PTR ");
	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",CL\n");
	  destval = fetch_data_byte(m,destoffset);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opcD0_byte_operation[rh])(m, destval,amt);
	  store_data_byte(m,destoffset,destval);
	  break;
       
	case 1:

	  DECODE_PRINTF(m,"BYTE PTR ");
	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",CL\n");
	  destval = fetch_data_byte(m,destoffset);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opcD0_byte_operation[rh])(m, destval, amt);
	  store_data_byte(m,destoffset,destval);
	  break;

	case 2:
	  
	  DECODE_PRINTF(m,"BYTE PTR ");
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",CL\n");
	  destval = fetch_data_byte(m,destoffset);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opcD0_byte_operation[rh])(m, destval, amt);
	  store_data_byte(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,",CL\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opcD0_byte_operation[rh])(m, *destreg, amt);
	  *destreg = destval;
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
    

 }


/* opcode=0xd3*/
void DEFUN(i86op_opcD3_word_RM_CL,(m),  PC_ENV *m) 
 {
   u_int16      mod,rl,rh;
   u_int16      *destreg;
   u_int16      destoffset;
   u_int16      destval;
   u_int8       amt;
   
   
   /* Yet another weirdo special case instruction format.  Part of the 
      opcode held below in "RH".  Doubly nested case would 
      result, except that the decoded instruction 
    */
   START_OF_INSTR(m);
   FETCH_DECODE_MODRM(m,mod,rh,rl);

#ifdef DEBUG
   if (DEBUG_DECODE(m))
     {
	/* XXX DECODE_PRINTF may be changed to something more
	   general, so that it is important to leave the strings
	   in the same format, even though the result is that the 
	   above test is done twice. */

	switch (rh)
	    {
	     case 0:
	       DECODE_PRINTF(m,"ROL\t");
	       break;
	     case 1:
	       DECODE_PRINTF(m,"ROR\t");
	       break;
	     case 2:
	       DECODE_PRINTF(m,"RCL\t");
	       break;
	     case 3:
	       DECODE_PRINTF(m,"RCR\t");
	       break;
	     case 4:
	       DECODE_PRINTF(m,"SHL\t");
	       break;
	     case 5:
	       DECODE_PRINTF(m,"SHR\t");
	       break;
	     case 6:
	       DECODE_PRINTF(m,"SAL\t");
	       break;
	     case 7:
	       DECODE_PRINTF(m,"SAR\t");
	       break;
	    }	  
     }
   
#endif	  

   amt = m->R_CL;   
   /* know operation, decode the mod byte to find the addressing 
      mode. */
   switch (mod) 
       {
	case 0:
	  DECODE_PRINTF(m,"WORD PTR ");
	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,",CL\n");
	  destval = fetch_data_word(m,destoffset);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opcD1_word_operation[rh])(m, destval, amt);
	  store_data_word(m,destoffset,destval);
	  break;
       
	case 1:

	  DECODE_PRINTF(m,"WORD PTR ");
	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,",CL\n");
	  destval = fetch_data_word(m,destoffset);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opcD1_word_operation[rh])(m, destval, amt);
	  store_data_word(m,destoffset,destval);
	  break;

	case 2:
	  
	  DECODE_PRINTF(m,"BYTE PTR ");
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,",CL\n");
	  destval = fetch_data_word(m,destoffset);
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  destval = (*opcD1_word_operation[rh])(m, destval, amt);
	  store_data_word(m,destoffset,destval);
	  break;
	  
	case 3:   /* register to register */
	  destreg  = DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,",CL\n");
	  TRACE_REGS(m);
	  SINGLE_STEP(m);
	  *destreg = (*opcD1_word_operation[rh])(m, *destreg, amt);
	  break;
       }

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);

 }



/* opcode=0xd4*/
void DEFUN(i86op_aam,(m),  PC_ENV *m) 
 {
    u_int8 a;

    START_OF_INSTR(m);   
    DECODE_PRINTF(m,"AAM\n");
    a = fetch_byte_imm(m);  /* this is a stupid encoding. */
    if (a != 10)
    { 
	fprintf(stderr,"error decoding aam\n");
	m->i86_intr_raise(0xD2);
    }
    TRACE_REGS(m);
    SINGLE_STEP(m);

    /* note the type change here --- returning AL and AH in AX. */
    m->R_AX = aam_word(m,m->R_AL);
    
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
 }

/* opcode=0xd5*/
void DEFUN(i86op_aad,(m),  PC_ENV *m) 

 {
    u_int8 a;
    
    START_OF_INSTR(m);   
    DECODE_PRINTF(m,"AAD\n");
    a = fetch_byte_imm(m);   
    TRACE_REGS(m);
    SINGLE_STEP(m);

    m->R_AX = aad_word(m,m->R_AX);
    
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
 }



/* opcode=0xd6 ILLEGAL OPCODE */

/* opcode=0xd7 */
void DEFUN(i86op_xlat,(m),  PC_ENV *m)
{
   u_int16 addr;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"XLAT\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   addr = m->R_BX + (u_int8)m->R_AL;
   m->R_AL = fetch_data_byte(m,addr);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}   


/* instuctions  D8 .. DF are in i87_ops.c */


/*opcode=0xe0*/
void DEFUN(i86op_loopne,(m),  PC_ENV *m) 
{
   int16 ip;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"LOOPNE\t");
   ip = (int8)fetch_byte_imm(m);
   ip += (int16)m->R_IP;
   DECODE_PRINTF2(m,"%04x\n",ip);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_CX -= 1;
   if (m->R_CX != 0 && !ACCESS_FLAG(m,F_ZF))  /* CX != 0 and !ZF */
     m->R_IP = ip;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xe1*/
void DEFUN(i86op_loope,(m),  PC_ENV *m) 
{
   int16 ip;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"LOOPE\t");
   ip = (int8)fetch_byte_imm(m);
   ip += (int16)m->R_IP;
   DECODE_PRINTF2(m,"%04x\n",ip);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_CX -= 1;
   if (m->R_CX != 0 && ACCESS_FLAG(m,F_ZF))  /* CX != 0 and ZF */
     m->R_IP = ip;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xe2*/
void DEFUN(i86op_loop,(m),  PC_ENV *m) 
{
   int16 ip;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"LOOP\t");
   ip = (int8)fetch_byte_imm(m);
   ip += (int16)m->R_IP;
   DECODE_PRINTF2(m,"%04x\n",ip);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_CX -= 1;
   if (m->R_CX != 0) 
     m->R_IP = ip;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xe3*/
void DEFUN(i86op_jcxz,(m),  PC_ENV *m) 
{
   int16 offset,target;
   
   /* jump to byte offset if overflow flag is set */
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"JCXZ\t");
   offset = (int8)fetch_byte_imm(m);   /* sign extended ??? */
   target = (int16)m->R_IP + offset;
   DECODE_PRINTF2(m,"%x\n",(u_int16)target);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   if (m->R_CX == 0)
     m->R_IP = target;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xe4*/
void DEFUN(i86op_in_byte_AL_IMM,(m),  PC_ENV *m) 
{
   u_int8 port;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"IN\t");
   port = (u_int8)fetch_byte_imm(m);
   DECODE_PRINTF2(m,"%x,AL\n",port);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AL = m->sys_in8(port);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xe5*/
void DEFUN(i86op_in_word_AX_IMM,(m),  PC_ENV *m) 
{
   u_int8 port;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"IN\t");
   port = (u_int8)fetch_byte_imm(m);
   DECODE_PRINTF2(m,"AX,%x",port);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AX = m->sys_in16(port);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xe6*/
void DEFUN(i86op_out_byte_IMM_AL,(m),  PC_ENV *m) 
{
   u_int8 port;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"OUT\t");
   port = (u_int8)fetch_byte_imm(m);
   DECODE_PRINTF2(m,"%x,AL\n",port);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->sys_out8(port, m->R_AL);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xe7*/
void DEFUN(i86op_out_word_IMM_AX,(m),  PC_ENV *m) 
{
   u_int8 port;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"OUT\t");
   port = (u_int8)fetch_byte_imm(m);
   DECODE_PRINTF2(m,"%x,AX",port);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->sys_out16(port, m->R_AX);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0xe8*/
void DEFUN(i86op_call_near_IMM,(m),  PC_ENV *m) 
{
   int16 ip;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"CALL\t"); 
   /* weird.  Thought this was a signed disp! */
   ip = (int16)fetch_word_imm(m);
   ip += (int16)m->R_IP;                  /* CHECK SIGN */
   DECODE_PRINTF2(m,"%04x\n",ip);
   CALL_TRACE(m,m->saved_ip, m->R_CS, ip);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   push_word(m,m->R_IP);
   m->R_IP = ip;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xe9*/
void DEFUN(i86op_jump_near_IMM,(m),  PC_ENV *m) 
{
   int ip;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"JMP\t");   
   /* weird.  Thought this was a signed disp too! */
   ip = (int16)fetch_word_imm(m);
   ip += (int16)m->R_IP;              /* CHECK SIGN */
   DECODE_PRINTF2(m,"%04x\n",ip);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_IP = ip;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xea*/
void DEFUN(i86op_jump_far_IMM,(m),  PC_ENV *m) 
{
   u_int16 cs,ip;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"JMP\tFAR ");
   ip = fetch_word_imm(m);
   cs = fetch_word_imm(m);
   DECODE_PRINTF2(m,"%04x:",cs);
   DECODE_PRINTF2(m,"%04x\n",ip);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_IP = ip;
   m->R_CS = cs;
   RESYNCH_SEG_CS(m);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xeb*/
void DEFUN(i86op_jump_byte_IMM,(m),  PC_ENV *m) 
{
   int8 offset;
   u_int16 target;
   
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"JMP\t");
   offset = (int8) fetch_byte_imm(m);             /* CHECK */
/*   printf("jump byte imm offset=%d\n",offset);*/
   target = (int16) m->R_IP + offset;
   DECODE_PRINTF2(m,"%04x\n",target);
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_IP = target;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xec*/
void DEFUN(i86op_in_byte_AL_DX,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"IN\tAL,DX\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AL = m->sys_in8(m->R_DX);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xed*/
void DEFUN(i86op_in_word_AX_DX,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"IN\tAX,DX\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->R_AX = m->sys_in16(m->R_DX);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xee*/
void DEFUN(i86op_out_byte_DX_AL,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"OUT\tDX,AL\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->sys_out8(m->R_DX, m->R_AL);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xef*/
void DEFUN(i86op_out_word_DX_AX,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"OUT\tDX,AX\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->sys_out16(m->R_DX, m->R_AX);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


/*opcode=0xf0*/
void DEFUN(i86op_lock,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"LOCK:\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->sys_lock();
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xf1 ILLEGAL OPERATION*/

/*opcode=0xf2*/
void DEFUN(i86op_repne,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"REPNE\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->sysmode |= SYSMODE_PREFIX_REPNE;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xf3*/
void DEFUN(i86op_repe,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"REPE\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   m->sysmode |= SYSMODE_PREFIX_REPE;
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xf4*/
void DEFUN(i86op_halt,(m),  PC_ENV *m) 
{
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"HALT\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);

   // If interrupts are enabled, wait for one.
   if ( (ACCESS_FLAG(m,F_IF)))
   {
	while (!intr)
	{
		m->clock_tick();
	}
   }
   else
   {
/* Watchdog interrupt */
	fprintf(stderr, "8086 crash by DI / HALT\n");
	m->i86_intr_raise(0xD3);
	m->i86_intr_handle();
   }	
}

/*opcode=0xf5*/
void DEFUN(i86op_cmc,(m),  PC_ENV *m) 
{
   /* complement the carry flag. */
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"CMC\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   TOGGLE_FLAG(m,F_CF);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}




/*opcode=0xf6*/
void DEFUN(i86op_opcF6_byte_RM,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int8       *destreg;
   u_int16      destoffset;
   u_int8       destval,srcval;

   /* long, drawn out code follows.  Double switch for a total
      of 32 cases.  */

   START_OF_INSTR(m);
   FETCH_DECODE_MODRM(m,mod,rh,rl);
   switch (mod) 
       {
	case 0:   /* mod=00 */
	  switch(rh)
	      {
	       case 0:  /* test byte imm */
		 DECODE_PRINTF(m,"TEST\tBYTE PTR ");
		 destoffset=decode_rm00_address(m,rl);
		 DECODE_PRINTF(m,",");
		 srcval  =  fetch_byte_imm(m);
		 DECODE_PRINTF2(m,"%02x\n",srcval);
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 test_byte(m, destval, srcval);
		 break;
	       case 1:
		 DECODE_PRINTF(m,"ILLEGAL OP MOD=00 RH=01 OP=F6\n");
	  	 if (m->illegal_op())
	  	 {
		    halt_sys(m);
	  	 }
		 break;
	       case 2:
		 DECODE_PRINTF(m,"NOT\tBYTE PTR ");
		 destoffset=decode_rm00_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 destval = not_byte(m, destval);
		 store_data_byte(m,destoffset,destval);
		 break;
	       case 3:
		 DECODE_PRINTF(m,"NEG\tBYTE PTR ");
		 destoffset=decode_rm00_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 destval = neg_byte(m, destval);
		 store_data_byte(m,destoffset,destval);
		 break;
	       case 4:
		 DECODE_PRINTF(m,"MUL\tBYTE PTR ");
		 destoffset=decode_rm00_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 mul_byte(m, destval);
		 break;
	       case 5:
		 DECODE_PRINTF(m,"IMUL\tBYTE PTR ");
		 destoffset=decode_rm00_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 imul_byte(m, destval);
		 break;
	       case 6:
		 DECODE_PRINTF(m,"DIV\tBYTE PTR ");
		 destoffset=decode_rm00_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 div_byte(m, destval);
		 break;
	       case 7:
		 DECODE_PRINTF(m,"IDIV\tBYTE PTR ");
		 destoffset=decode_rm00_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 idiv_byte(m, destval);
		 break;
	      }
	  break;  /* end mod==00 */
	case 1:   /* mod=01 */
	  switch(rh)
	      {
	       case 0:  /* test byte imm */
		 DECODE_PRINTF(m,"TEST\tBYTE PTR ");
		 destoffset=decode_rm01_address(m,rl);
		 DECODE_PRINTF(m,",");
		 srcval  =  fetch_byte_imm(m);
		 DECODE_PRINTF2(m,"%02x\n",srcval);
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 test_byte(m, destval, srcval);
		 break;
	       case 1:
		 DECODE_PRINTF(m,"ILLEGAL OP MOD=01 RH=01 OP=F6\n");
	  	 if (m->illegal_op())
	  	 {
		    halt_sys(m);
	  	 }
		 break;
	       case 2:
		 DECODE_PRINTF(m,"NOT\tBYTE PTR ");
		 destoffset=decode_rm01_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 destval = not_byte(m, destval);
		 store_data_byte(m,destoffset,destval);
		 break;
	       case 3:
		 DECODE_PRINTF(m,"NEG\tBYTE PTR ");
		 destoffset=decode_rm01_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 destval = neg_byte(m, destval);
		 store_data_byte(m,destoffset,destval);
		 break;
	       case 4:
		 DECODE_PRINTF(m,"MUL\tBYTE PTR ");
		 destoffset=decode_rm01_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 mul_byte(m, destval);
		 break;
	       case 5:
		 DECODE_PRINTF(m,"IMUL\tBYTE PTR ");
		 destoffset=decode_rm01_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 imul_byte(m, destval);
		 break;
	       case 6:
		 DECODE_PRINTF(m,"DIV\tBYTE PTR ");
		 destoffset=decode_rm01_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 div_byte(m, destval);
		 break;
	       case 7:
		 DECODE_PRINTF(m,"IDIV\tBYTE PTR ");
		 destoffset=decode_rm01_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 idiv_byte(m, destval);
		 break;
	      }
	  break;  /* end mod==01 */
	case 2:   /* mod=10 */
	  switch(rh)
	      {
	       case 0:  /* test byte imm */
		 DECODE_PRINTF(m,"TEST\tBYTE PTR ");
		 destoffset=decode_rm10_address(m,rl);
		 DECODE_PRINTF(m,",");
		 srcval  =  fetch_byte_imm(m);
		 DECODE_PRINTF2(m,"%02x\n",srcval);
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 test_byte(m, destval, srcval);
		 break;
	       case 1:
		 DECODE_PRINTF(m,"ILLEGAL OP MOD=10 RH=01 OP=F6\n");
	  	 if (m->illegal_op())
	  	 {
		    halt_sys(m);
	  	 }
		 break;
	       case 2:
		 DECODE_PRINTF(m,"NOT\tBYTE PTR ");
		 destoffset=decode_rm10_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 destval = not_byte(m, destval);
		 store_data_byte(m,destoffset,destval);
		 break;
	       case 3:
		 DECODE_PRINTF(m,"NEG\tBYTE PTR ");
		 destoffset=decode_rm10_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 destval = neg_byte(m, destval);
		 store_data_byte(m,destoffset,destval);
		 break;
	       case 4:
		 DECODE_PRINTF(m,"MUL\tBYTE PTR ");
		 destoffset=decode_rm10_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 mul_byte(m, destval);
		 break;
	       case 5:
		 DECODE_PRINTF(m,"IMUL\tBYTE PTR ");
		 destoffset=decode_rm10_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 imul_byte(m, destval);
		 break;
	       case 6:
		 DECODE_PRINTF(m,"DIV\tBYTE PTR ");
		 destoffset=decode_rm10_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 div_byte(m, destval);
		 break;
	       case 7:
		 DECODE_PRINTF(m,"IDIV\tBYTE PTR ");
		 destoffset=decode_rm10_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 idiv_byte(m, destval);
		 break;
	      }
	  break;  /* end mod==10 */
	case 3:   /* mod=11 */
	  switch(rh)
	      {
	       case 0:  /* test byte imm */
		 DECODE_PRINTF(m,"TEST\t");
		 destreg=DECODE_RM_BYTE_REGISTER(m,rl);
		 DECODE_PRINTF(m,",");
		 srcval  =  fetch_byte_imm(m);
		 DECODE_PRINTF2(m,"%02x\n",srcval);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 test_byte(m, *destreg, srcval);
		 break;
	       case 1:
		 DECODE_PRINTF(m,"ILLEGAL OP MOD=00 RH=01 OP=F6\n");
	  	 if (m->illegal_op())
	  	 {
		    halt_sys(m);
	  	 }
		 break;
	       case 2:
		 DECODE_PRINTF(m,"NOT\t");
		 destreg=DECODE_RM_BYTE_REGISTER(m,rl);
		 DECODE_PRINTF(m,"\n");
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 *destreg = not_byte(m, *destreg);
		 break;
	       case 3:
		 DECODE_PRINTF(m,"NEG\t");
		 destreg=DECODE_RM_BYTE_REGISTER(m,rl);
		 DECODE_PRINTF(m,"\n");
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 *destreg = neg_byte(m, *destreg);
		 break;
	       case 4:
		 DECODE_PRINTF(m,"MUL\t");
		 destreg=DECODE_RM_BYTE_REGISTER(m,rl);
		 DECODE_PRINTF(m,"\n");
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 mul_byte(m, *destreg);  /*!!!  */
		 break;
	       case 5:
		 DECODE_PRINTF(m,"IMUL\t");
		 destreg=DECODE_RM_BYTE_REGISTER(m,rl);
		 DECODE_PRINTF(m,"\n");
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 imul_byte(m, *destreg);  
		 break;
	       case 6:
		 DECODE_PRINTF(m,"DIV\t");
		 destreg=DECODE_RM_BYTE_REGISTER(m,rl);
		 DECODE_PRINTF(m,"\n");
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 div_byte(m, *destreg);  
		 break;
	       case 7:
		 DECODE_PRINTF(m,"IDIV\t");
		 destreg=DECODE_RM_BYTE_REGISTER(m,rl);
		 DECODE_PRINTF(m,"\n");
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 idiv_byte(m, *destreg);  
		 break;
	      }
	  break;  /* end mod==11 */
       }
   
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xf7*/
void DEFUN(i86op_opcF7_word_RM,(m),  PC_ENV *m) 
{
   u_int16      mod,rl,rh;
   u_int16      *destreg;
   u_int16      destoffset;
   u_int16         destval,srcval;

   /* long, drawn out code follows.  Double switch for a total
      of 32 cases.  */

   START_OF_INSTR(m);
   FETCH_DECODE_MODRM(m,mod,rh,rl);
   switch (mod) 
       {
	case 0:   /* mod=00 */
	  switch(rh)
	      {
	       case 0:  /* test word imm */
		 DECODE_PRINTF(m,"TEST\tWORD PTR ");
		 destoffset=decode_rm00_address(m,rl);
		 DECODE_PRINTF(m,",");
		 srcval  =  fetch_word_imm(m);
		 DECODE_PRINTF2(m,"%04x\n",srcval);
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 test_word(m, destval, srcval);
		 break;
	       case 1:
		 DECODE_PRINTF(m,"ILLEGAL OP MOD=00 RH=01 OP=F7\n");
	  	 if (m->illegal_op())
	  	 {
		    halt_sys(m);
	  	 }
		 break;
	       case 2:
		 DECODE_PRINTF(m,"NOT\tWORD PTR ");
		 destoffset=decode_rm00_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 destval = not_word(m, destval);
		 store_data_word(m,destoffset,destval);
		 break;
	       case 3:
		 DECODE_PRINTF(m,"NEG\tWORD PTR ");
		 destoffset=decode_rm00_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 destval = neg_word(m, destval);
		 store_data_word(m,destoffset,destval);
		 break;
	       case 4:
		 DECODE_PRINTF(m,"MUL\tWORD PTR ");
		 destoffset=decode_rm00_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 mul_word(m, destval);
		 break;
	       case 5:
		 DECODE_PRINTF(m,"IMUL\tWORD PTR ");
		 destoffset=decode_rm00_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 imul_word(m, destval);
		 break;
	       case 6:
		 DECODE_PRINTF(m,"DIV\tWORD PTR ");
		 destoffset=decode_rm00_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 div_word(m, destval);
		 break;
	       case 7:
		 DECODE_PRINTF(m,"IDIV\tWORD PTR ");
		 destoffset=decode_rm00_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 idiv_word(m, destval);
		 break;
	      }
	  break;  /* end mod==00 */
	case 1:   /* mod=01 */
	  switch(rh)
	      {
	       case 0:  /* test word imm */
		 DECODE_PRINTF(m,"TEST\tWORD PTR ");
		 destoffset=decode_rm01_address(m,rl);
		 DECODE_PRINTF(m,",");
		 srcval  =  fetch_word_imm(m);
		 DECODE_PRINTF2(m,"%02x\n",srcval);
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 test_word(m, destval, srcval);
		 break;
	       case 1:
		 DECODE_PRINTF(m,"ILLEGAL OP MOD=01 RH=01 OP=F6\n");
	  	 if (m->illegal_op())
	  	 {
		    halt_sys(m);
	  	 }
		 break;
	       case 2:
		 DECODE_PRINTF(m,"NOT\tWORD PTR ");
		 destoffset=decode_rm01_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 destval = not_word(m, destval);
		 store_data_word(m,destoffset,destval);
		 break;
	       case 3:
		 DECODE_PRINTF(m,"NEG\tWORD PTR ");
		 destoffset=decode_rm01_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 destval = neg_word(m, destval);
		 store_data_word(m,destoffset,destval);
		 break;
	       case 4:
		 DECODE_PRINTF(m,"MUL\tWORD PTR ");
		 destoffset=decode_rm01_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 mul_word(m, destval);
		 break;
	       case 5:
		 DECODE_PRINTF(m,"IMUL\tWORD PTR ");
		 destoffset=decode_rm01_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 imul_word(m, destval);
		 break;
	       case 6:
		 DECODE_PRINTF(m,"DIV\tWORD PTR ");
		 destoffset=decode_rm01_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 div_word(m, destval);
		 break;
	       case 7:
		 DECODE_PRINTF(m,"IDIV\tWORD PTR ");
		 destoffset=decode_rm01_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 idiv_word(m, destval);
		 break;
	      }
	  break;  /* end mod==01 */
	case 2:   /* mod=10 */
	  switch(rh)
	      {
	       case 0:  /* test word imm */
		 DECODE_PRINTF(m,"TEST\tWORD PTR ");
		 destoffset=decode_rm10_address(m,rl);
		 DECODE_PRINTF(m,",");
		 srcval  =  fetch_word_imm(m);
		 DECODE_PRINTF2(m,"%02x\n",srcval);
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 test_word(m, destval, srcval);
		 break;
	       case 1:
		 DECODE_PRINTF(m,"ILLEGAL OP MOD=10 RH=01 OP=F6\n");
	  	 if (m->illegal_op())
	  	 {
		    halt_sys(m);
	  	 }
		 break;
	       case 2:
		 DECODE_PRINTF(m,"NOT\tWORD PTR ");
		 destoffset=decode_rm10_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 destval = not_word(m, destval);
		 store_data_word(m,destoffset,destval);
		 break;
	       case 3:
		 DECODE_PRINTF(m,"NEG\tWORD PTR ");
		 destoffset=decode_rm10_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 destval = neg_word(m, destval);
		 store_data_word(m,destoffset,destval);
		 break;
	       case 4:
		 DECODE_PRINTF(m,"MUL\tWORD PTR ");
		 destoffset=decode_rm10_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 mul_word(m, destval);
		 break;
	       case 5:
		 DECODE_PRINTF(m,"IMUL\tWORD PTR ");
		 destoffset=decode_rm10_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 imul_word(m, destval);
		 break;
	       case 6:
		 DECODE_PRINTF(m,"DIV\tWORD PTR ");
		 destoffset=decode_rm10_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 div_word(m, destval);
		 break;
	       case 7:
		 DECODE_PRINTF(m,"IDIV\tWORD PTR ");
		 destoffset=decode_rm10_address(m,rl);
		 DECODE_PRINTF(m,"\n");
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 idiv_word(m, destval);
		 break;
	      }
	  break;  /* end mod==10 */
	case 3:   /* mod=11 */
	  switch(rh)
	      {
	       case 0:  /* test word imm */
		 DECODE_PRINTF(m,"TEST\t");
		 destreg=DECODE_RM_WORD_REGISTER(m,rl);
		 DECODE_PRINTF(m,",");
		 srcval  =  fetch_word_imm(m);
		 DECODE_PRINTF2(m,"%02x\n",srcval);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 test_word(m, *destreg, srcval);
		 break;
	       case 1:
		 DECODE_PRINTF(m,"ILLEGAL OP MOD=00 RH=01 OP=F6\n");
	  	 if (m->illegal_op())
	  	 {
		    halt_sys(m);
	  	 }
		 break;
	       case 2:
		 DECODE_PRINTF(m,"NOT\t");
		 destreg=DECODE_RM_WORD_REGISTER(m,rl);
		 DECODE_PRINTF(m,"\n");
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 *destreg = not_word(m, *destreg);
		 break;
	       case 3:
		 DECODE_PRINTF(m,"NEG\t");
		 destreg=DECODE_RM_WORD_REGISTER(m,rl);
		 DECODE_PRINTF(m,"\n");
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 *destreg = neg_word(m, *destreg);
		 break;
	       case 4:
		 DECODE_PRINTF(m,"MUL\t");
		 destreg=DECODE_RM_WORD_REGISTER(m,rl);
		 DECODE_PRINTF(m,"\n");
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 mul_word(m, *destreg);  /*!!!  */
		 break;
	       case 5:
		 DECODE_PRINTF(m,"IMUL\t");
		 destreg=DECODE_RM_WORD_REGISTER(m,rl);
		 DECODE_PRINTF(m,"\n");
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 imul_word(m, *destreg);  
		 break;
	       case 6:
		 DECODE_PRINTF(m,"DIV\t");
		 destreg=DECODE_RM_WORD_REGISTER(m,rl);
		 DECODE_PRINTF(m,"\n");
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 div_word(m, *destreg);  
		 break;
	       case 7:
		 DECODE_PRINTF(m,"IDIV\t");
		 destreg=DECODE_RM_WORD_REGISTER(m,rl);
		 DECODE_PRINTF(m,"\n");
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 idiv_word(m, *destreg);  
		 break;
	      }
	  break;  /* end mod==11 */
       }
   
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xf8*/
void DEFUN(i86op_clc,(m),  PC_ENV *m) 
{
   /* clear the carry flag. */
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"CLC\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   CLEAR_FLAG(m, F_CF);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xf9*/
void DEFUN(i86op_stc,(m),  PC_ENV *m) 
{
   /* set the carry flag. */
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"STC\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   SET_FLAG(m, F_CF);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xfa*/
void DEFUN(i86op_cli,(m),  PC_ENV *m) 
{
   /* clear interrupts. */
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"CLI\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   CLEAR_FLAG(m, F_IF);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xfb*/
void DEFUN(i86op_sti,(m),  PC_ENV *m) 
{
   /* enable  interrupts. */
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"STI\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   SET_FLAG(m, F_IF);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xfc*/
void DEFUN(i86op_cld,(m),  PC_ENV *m) 
{
   /* clear interrupts. */
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"CLD\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   CLEAR_FLAG(m, F_DF);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xfd*/
void DEFUN(i86op_std,(m),  PC_ENV *m) 
{
   /* clear interrupts. */
   START_OF_INSTR(m);
   DECODE_PRINTF(m,"STD\n");
   TRACE_REGS(m);
   SINGLE_STEP(m);
   SET_FLAG(m, F_DF);
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}

/*opcode=0xfe*/
void DEFUN(i86op_opcFE_byte_RM,(m),  PC_ENV *m) 
{
   /* Yet another damned special case instruction. */

   u_int16      mod,rh,rl;
   u_int8       destval;
   u_int16      destoffset;
   u_int8       *destreg;
   
   
   
   /* ARRGH, ANOTHER GODDAMN SPECIAL CASE INSTRUCTION!!! */
   START_OF_INSTR(m);
   FETCH_DECODE_MODRM(m,mod,rh,rl);
#ifdef DEBUG
   if (DEBUG_DECODE(m))
     {
	/* XXX DECODE_PRINTF may be changed to something more
	   general, so that it is important to leave the strings
	   in the same format, even though the result is that the 
	   above test is done twice. */

	switch (rh)
	    {
	     case 0:
	       DECODE_PRINTF(m,"INC\t");
	       break;
	     case 1:
	       DECODE_PRINTF(m,"DEC\t");
	       break;
	     case 2:  case 3:
	     case 4:  case 5:
	     case 6:  case 7:
	       DECODE_PRINTF2(m,"ILLEGAL OP MAJOR OP 0xFE MINOR OP %x \n",mod);
	       if (m->illegal_op())
	       {	
		    halt_sys(m);
	       }
	       break;
	    }	  
     }
#endif	  

   switch (mod) 
       {
	case 0:
	  DECODE_PRINTF(m,"BYTE PTR ");
	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,"\n");
	  switch (rh)
	    {
	     case 0:  /* inc word ptr ... */
	       destval = fetch_data_byte(m,destoffset);
	       TRACE_REGS(m);
	       SINGLE_STEP(m);
	       destval = inc_byte(m,destval);
	       store_data_byte(m,destoffset,destval);
	       break;
	     case 1:  /* dec word ptr ... */
	       destval = fetch_data_byte(m,destoffset);
	       TRACE_REGS(m);
	       SINGLE_STEP(m);
	       destval = dec_byte(m,destval);
	       store_data_byte(m,destoffset,destval);
	       break;
	    }
	  break;
	  
       
	case 1:
	  DECODE_PRINTF(m,"BYTE PTR ");
	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,"\n");
	  switch (rh)
	      {
	       case 0:
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 destval = inc_byte(m,destval);
		 store_data_byte(m,destoffset,destval);
		 break;
	       case 1:
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 destval = dec_byte(m,destval);
		 store_data_byte(m,destoffset,destval);
		 break;
	      }
	  break;
	  
	case 2:
	  DECODE_PRINTF(m,"BYTE PTR ");
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,"\n");
	  switch (rh)
	      {
	       case 0:
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 destval = inc_byte(m,destval);
		 store_data_byte(m,destoffset,destval);
		 break;
	       case 1:
		 destval = fetch_data_byte(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 destval = dec_byte(m,destval);
		 store_data_byte(m,destoffset,destval);
		 break;
	      }
	  break;
	  
	case 3:   
	  destreg  = DECODE_RM_BYTE_REGISTER(m,rl);
	  DECODE_PRINTF(m,"\n");
	  switch (rh)
	      {
	       case 0:
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 *destreg = inc_byte(m,*destreg);
		 break;
	       case 1:
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 *destreg = dec_byte(m,*destreg);
		 break;
	      }
	  break;
       }
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}




/*opcode=0xff*/
void DEFUN(i86op_opcFF_word_RM,(m),  PC_ENV *m) 
{

   u_int16      mod,rh,rl;
   u_int16      destval,destval2;
   u_int16      destoffset;
   u_int16     *destreg;
   
   
   
   /* ANOTHER DAMN SPECIAL CASE INSTRUCTION!!! */
   START_OF_INSTR(m);
   FETCH_DECODE_MODRM(m,mod,rh,rl);

#ifdef DEBUG
   if (DEBUG_DECODE(m))
     {
	/* XXX DECODE_PRINTF may be changed to something more
	   general, so that it is important to leave the strings
	   in the same format, even though the result is that the 
	   above test is done twice. */

	switch (rh)
	    {
	     case 0:
	       DECODE_PRINTF(m,"INC\tWORD PTR ");
	       break;
	     case 1:
	       DECODE_PRINTF(m,"DEC\tWORD PTR ");
	       break;
	     case 2:
	       DECODE_PRINTF(m,"CALL\t ");
	       break;
	     case 3:
	       DECODE_PRINTF(m,"CALL\tFAR ");
	       break;
	     case 4:
	       DECODE_PRINTF(m,"JMP\t");
	       break;
	     case 5:
	       DECODE_PRINTF(m,"JMP\tFAR ");
	       break;
	     case 6:
	       DECODE_PRINTF(m,"PUSH\t");
	       break;
	     case 7:
	       DECODE_PRINTF(m,"ILLEGAL DECODING OF OPCODE FF\t");
	       if (m->illegal_op())
	       {	
		    halt_sys(m);
	       }
	       break;
	    }	  
     }
#endif	  

   switch (mod) 
       {
	case 0:
	  destoffset=decode_rm00_address(m,rl);
	  DECODE_PRINTF(m,"\n");
	  
	  switch (rh)
	      {
	       case 0:  /* inc word ptr ... */
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 destval = inc_word(m,destval);
		 store_data_word(m,destoffset,destval);
		 break;
	       case 1:  /* dec word ptr ... */
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 destval = dec_word(m,destval);
		 store_data_word(m,destoffset,destval);
		 break;		 
	       case 2:  /* call word ptr ... */
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 push_word(m,m->R_IP);
		 m->R_IP = destval;
		 break;
	       case 3:  /* call far ptr ... */
		 destval = fetch_data_word(m,destoffset);
		 destval2 = fetch_data_word(m,destoffset+2);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 push_word(m,m->R_CS);
		 m->R_CS = destval2;
		 RESYNCH_SEG_CS(m);
		 push_word(m,m->R_IP);
		 m->R_IP = destval;
		 break;
	       case 4:  /* jmp word ptr ... */
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 m->R_IP = destval;
		 break;
	       case 5:  /* jmp far ptr ... */
		 destval = fetch_data_word(m,destoffset);
		 destval2 = fetch_data_word(m,destoffset+2);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 m->R_IP = destval;
		 m->R_CS = destval2;
		 RESYNCH_SEG_CS(m);
		 break;
		 
	       case 6:  /*  push word ptr ... */
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 push_word(m,destval);
		 break;
	      }
	  break;
	  
	  
	case 1:
	  destoffset=decode_rm01_address(m,rl);
	  DECODE_PRINTF(m,"\n");
	  switch (rh)
	      {
	       case 0:
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 destval = inc_word(m,destval);
		 store_data_word(m,destoffset,destval);
		 break;
	       case 1:
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 destval = dec_word(m,destval);
		 store_data_word(m,destoffset,destval);
		 break;
	       case 2:  /* call word ptr ... */
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 push_word(m,m->R_IP);
		 m->R_IP = destval;
		 break;
	       case 3:  /* call far ptr ... */
		 destval = fetch_data_word(m,destoffset);
		 destval2 = fetch_data_word(m,destoffset+2);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 push_word(m,m->R_CS);
		 m->R_CS = destval2;
		 RESYNCH_SEG_CS(m);
		 push_word(m,m->R_IP);
		 m->R_IP = destval;
		 break;
	       case 4:  /* jmp word ptr ... */
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 m->R_IP = destval;
		 break;	
	       case 5:  /* jmp far ptr ... */
		 destval = fetch_data_word(m,destoffset);
		 destval2 = fetch_data_word(m,destoffset+2);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 m->R_IP = destval;
		 m->R_CS = destval2;
		 RESYNCH_SEG_CS(m);
		 break;
	       case 6:  /*  push word ptr ... */
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 push_word(m,destval);
		 break;
	      }
	  break;
	  
	case 2:
	  destoffset=decode_rm10_address(m,rl);
	  DECODE_PRINTF(m,"\n");
	  switch (rh)
	      {
	       case 0:
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 destval = inc_word(m,destval);
		 store_data_word(m,destoffset,destval);
		 break;
	       case 1:
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 destval = dec_word(m,destval);
		 store_data_word(m,destoffset,destval);
		 break;
	       case 2:  /* call word ptr ... */
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 push_word(m,m->R_IP);
		 m->R_IP = destval;
		 break;

	       case 3:  /* call far ptr ... */
		 destval = fetch_data_word(m,destoffset);
		 destval2 = fetch_data_word(m,destoffset+2);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 push_word(m,m->R_CS);
		 m->R_CS = destval2;
		 RESYNCH_SEG_CS(m);
		 push_word(m,m->R_IP);
		 m->R_IP = destval;
		 break;

	       case 4:  /* jmp word ptr ... */
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 m->R_IP = destval;
		 break;
	       case 5:  /* jmp far ptr ... */
		 destval = fetch_data_word(m,destoffset);
		 destval2 = fetch_data_word(m,destoffset+2);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 m->R_IP = destval;
		 m->R_CS = destval2;
		 RESYNCH_SEG_CS(m);
		 break;
	       case 6:  /*  push word ptr ... */
		 destval = fetch_data_word(m,destoffset);
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 push_word(m,destval);
		 break;
	      }
	  break;
	  
	case 3:   
	  destreg  = DECODE_RM_WORD_REGISTER(m,rl);
	  DECODE_PRINTF(m,"\n");
	  switch (rh)
	      {
	       case 0:
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 *destreg = inc_word(m,*destreg);
		 break;
	       case 1:
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 *destreg = dec_word(m,*destreg);
		 break;
	       case 2:  /* call word ptr ... */
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 push_word(m,m->R_IP);
		 m->R_IP = *destreg;
		 break;
	       case 3:  /* jmp far ptr ... */
		 DECODE_PRINTF(m,"OPERATION UNDEFINED 0XFF \n");
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 if (m->illegal_op())
		 {
			 halt_sys(m);
		 }
		 break;

	       case 4:  /* jmp  ... */
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 m->R_IP = (u_int16)(*destreg);
		 break;
	       case 5:  /* jmp far ptr ... */
		 DECODE_PRINTF(m,"OPERATION UNDEFINED 0XFF \n");
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 if (m->illegal_op())
		 {
		 	halt_sys(m);
		 }
		 break;
	       case 6:
		 TRACE_REGS(m);
		 SINGLE_STEP(m);
		 push_word(m,*destreg);
		 break;
	      }
	  break;
       }
   
   
   

   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}
 




/* --------------------------------------------------- */ 
/* --------------------------------------------------- */ 
/* --------------------------------------------------- */ 
/* OPERAND TABLE                                       */
/* --------------------------------------------------- */ 
/* --------------------------------------------------- */ 
/* --------------------------------------------------- */ 

OP  i86_optab[256]  =  {

/*  0x00 */  i86op_add_byte_RM_R ,
/*  0x01 */  i86op_add_word_RM_R ,
/*  0x02 */  i86op_add_byte_R_RM ,
/*  0x03 */  i86op_add_word_R_RM ,
/*  0x04 */  i86op_add_byte_AL_IMM ,
/*  0x05 */  i86op_add_word_AX_IMM ,
/*  0x06 */  i86op_push_ES ,
/*  0x07 */  i86op_pop_ES ,

/*  0x08 */  i86op_or_byte_RM_R ,
/*  0x09 */  i86op_or_word_RM_R ,
/*  0x0a */  i86op_or_byte_R_RM ,
/*  0x0b */  i86op_or_word_R_RM ,
/*  0x0c */  i86op_or_byte_AL_IMM ,
/*  0x0d */  i86op_or_word_AX_IMM ,
/*  0x0e */  i86op_push_CS ,
/*  0x0f */  i86op_illegal_op ,

/*  0x10 */  i86op_adc_byte_RM_R ,
/*  0x11 */  i86op_adc_word_RM_R ,
/*  0x12 */  i86op_adc_byte_R_RM ,
/*  0x13 */  i86op_adc_word_R_RM ,
/*  0x14 */  i86op_adc_byte_AL_IMM ,
/*  0x15 */  i86op_adc_word_AX_IMM ,
/*  0x16 */  i86op_push_SS ,
/*  0x17 */  i86op_pop_SS ,

/*  0x18 */  i86op_sbb_byte_RM_R ,
/*  0x19 */  i86op_sbb_word_RM_R ,
/*  0x1a */  i86op_sbb_byte_R_RM ,
/*  0x1b */  i86op_sbb_word_R_RM ,
/*  0x1c */  i86op_sbb_byte_AL_IMM ,
/*  0x1d */  i86op_sbb_word_AX_IMM ,
/*  0x1e */  i86op_push_DS ,
/*  0x1f */  i86op_pop_DS ,

/*  0x20 */  i86op_and_byte_RM_R ,
/*  0x21 */  i86op_and_word_RM_R ,
/*  0x22 */  i86op_and_byte_R_RM ,
/*  0x23 */  i86op_and_word_R_RM ,
/*  0x24 */  i86op_and_byte_AL_IMM ,
/*  0x25 */  i86op_and_word_AX_IMM ,
/*  0x26 */  i86op_segovr_ES ,
/*  0x27 */  i86op_daa ,

/*  0x28 */  i86op_sub_byte_RM_R ,
/*  0x29 */  i86op_sub_word_RM_R ,
/*  0x2a */  i86op_sub_byte_R_RM ,
/*  0x2b */  i86op_sub_word_R_RM ,
/*  0x2c */  i86op_sub_byte_AL_IMM ,
/*  0x2d */  i86op_sub_word_AX_IMM ,
/*  0x2e */  i86op_segovr_CS ,
/*  0x2f */  i86op_das ,

/*  0x30 */  i86op_xor_byte_RM_R ,
/*  0x31 */  i86op_xor_word_RM_R ,
/*  0x32 */  i86op_xor_byte_R_RM ,
/*  0x33 */  i86op_xor_word_R_RM ,
/*  0x34 */  i86op_xor_byte_AL_IMM ,
/*  0x35 */  i86op_xor_word_AX_IMM ,
/*  0x36 */  i86op_segovr_SS ,
/*  0x37 */  i86op_aaa ,

/*  0x38 */  i86op_cmp_byte_RM_R ,
/*  0x39 */  i86op_cmp_word_RM_R ,
/*  0x3a */  i86op_cmp_byte_R_RM ,
/*  0x3b */  i86op_cmp_word_R_RM ,
/*  0x3c */  i86op_cmp_byte_AL_IMM ,
/*  0x3d */  i86op_cmp_word_AX_IMM ,
/*  0x3e */  i86op_segovr_DS ,
/*  0x3f */  i86op_aas ,

/*  0x40 */  i86op_inc_AX ,
/*  0x41 */  i86op_inc_CX ,
/*  0x42 */  i86op_inc_DX ,
/*  0x43 */  i86op_inc_BX ,
/*  0x44 */  i86op_inc_SP ,
/*  0x45 */  i86op_inc_BP ,
/*  0x46 */  i86op_inc_SI ,
/*  0x47 */  i86op_inc_DI ,

/*  0x48 */  i86op_dec_AX ,
/*  0x49 */  i86op_dec_CX ,
/*  0x4a */  i86op_dec_DX ,
/*  0x4b */  i86op_dec_BX ,
/*  0x4c */  i86op_dec_SP ,
/*  0x4d */  i86op_dec_BP ,
/*  0x4e */  i86op_dec_SI ,
/*  0x4f */  i86op_dec_DI ,

/*  0x50 */  i86op_push_AX ,
/*  0x51 */  i86op_push_CX ,
/*  0x52 */  i86op_push_DX ,
/*  0x53 */  i86op_push_BX ,
/*  0x54 */  i86op_push_SP ,
/*  0x55 */  i86op_push_BP ,
/*  0x56 */  i86op_push_SI ,
/*  0x57 */  i86op_push_DI ,

/*  0x58 */  i86op_pop_AX ,
/*  0x59 */  i86op_pop_CX ,
/*  0x5a */  i86op_pop_DX ,
/*  0x5b */  i86op_pop_BX ,
/*  0x5c */  i86op_pop_SP ,
/*  0x5d */  i86op_pop_BP ,
/*  0x5e */  i86op_pop_SI ,
/*  0x5f */  i86op_pop_DI ,

/*  0x60 */  i86op_illegal_op ,
/*  0x61 */  i86op_illegal_op ,
/*  0x62 */  i86op_illegal_op ,
/*  0x63 */  i86op_illegal_op ,
/*  0x64 */  i86op_illegal_op ,
/*  0x65 */  i86op_illegal_op ,
/*  0x66 */  i86op_illegal_op ,
/*  0x67 */  i86op_illegal_op ,

/*  0x68 */  i86op_illegal_op ,
/*  0x69 */  i86op_illegal_op ,
/*  0x6a */  i86op_illegal_op ,
/*  0x6b */  i86op_illegal_op ,
/*  0x6c */  i86op_illegal_op ,
/*  0x6d */  i86op_illegal_op ,
/*  0x6e */  i86op_illegal_op ,
/*  0x6f */  i86op_illegal_op ,

/*  0x70 */  i86op_jump_near_O ,
/*  0x71 */  i86op_jump_near_NO ,
/*  0x72 */  i86op_jump_near_B ,
/*  0x73 */  i86op_jump_near_NB ,
/*  0x74 */  i86op_jump_near_Z ,
/*  0x75 */  i86op_jump_near_NZ ,
/*  0x76 */  i86op_jump_near_BE ,
/*  0x77 */  i86op_jump_near_NBE ,

/*  0x78 */  i86op_jump_near_S ,
/*  0x79 */  i86op_jump_near_NS ,
/*  0x7a */  i86op_jump_near_P ,
/*  0x7b */  i86op_jump_near_NP ,
/*  0x7c */  i86op_jump_near_L ,
/*  0x7d */  i86op_jump_near_NL ,
/*  0x7e */  i86op_jump_near_LE ,
/*  0x7f */  i86op_jump_near_NLE ,

/*  0x80 */  i86op_opc80_byte_RM_IMM ,
/*  0x81 */  i86op_opc81_word_RM_IMM ,
/*  0x82 */  i86op_opc82_byte_RM_IMM ,
/*  0x83 */  i86op_opc83_word_RM_IMM ,
/*  0x84 */  i86op_test_byte_RM_R ,
/*  0x85 */  i86op_test_word_RM_R ,
/*  0x86 */  i86op_xchg_byte_RM_R ,
/*  0x87 */  i86op_xchg_word_RM_R ,

/*  0x88 */  i86op_mov_byte_RM_R ,
/*  0x89 */  i86op_mov_word_RM_R ,
/*  0x8a */  i86op_mov_byte_R_RM ,
/*  0x8b */  i86op_mov_word_R_RM ,
/*  0x8c */  i86op_mov_word_RM_SR ,
/*  0x8d */  i86op_lea_word_R_M ,
/*  0x8e */  i86op_mov_word_SR_RM ,
/*  0x8f */  i86op_pop_RM ,

/*  0x90 */  i86op_nop ,
/*  0x91 */  i86op_xchg_word_AX_CX ,
/*  0x92 */  i86op_xchg_word_AX_DX ,
/*  0x93 */  i86op_xchg_word_AX_BX ,
/*  0x94 */  i86op_xchg_word_AX_SP ,
/*  0x95 */  i86op_xchg_word_AX_BP  ,
/*  0x96 */  i86op_xchg_word_AX_SI   ,
/*  0x97 */  i86op_xchg_word_AX_DI    ,

/*  0x98 */  i86op_cbw ,
/*  0x99 */  i86op_cwd ,
/*  0x9a */  i86op_call_far_IMM ,
/*  0x9b */  i86op_wait ,
/*  0x9c */  i86op_pushf_word ,
/*  0x9d */  i86op_popf_word ,
/*  0x9e */  i86op_sahf ,
/*  0x9f */  i86op_lahf ,

/*  0xa0 */  i86op_mov_AL_M_IMM,
/*  0xa1 */  i86op_mov_AX_M_IMM,
/*  0xa2 */  i86op_mov_M_AL_IMM,
/*  0xa3 */  i86op_mov_M_AX_IMM,
/*  0xa4 */  i86op_movs_byte,
/*  0xa5 */  i86op_movs_word,
/*  0xa6 */  i86op_cmps_byte,
/*  0xa7 */  i86op_cmps_word,
/*  0xa8 */  i86op_test_AL_IMM,
/*  0xa9 */  i86op_test_AX_IMM,
/*  0xaa */  i86op_stos_byte,
/*  0xab */  i86op_stos_word,
/*  0xac */  i86op_lods_byte,
/*  0xad */  i86op_lods_word,
/*  0xac */  i86op_scas_byte,
/*  0xad */  i86op_scas_word,


/*  0xb0 */  i86op_mov_byte_AL_IMM ,
/*  0xb1 */  i86op_mov_byte_CL_IMM ,
/*  0xb2 */  i86op_mov_byte_DL_IMM ,
/*  0xb3 */  i86op_mov_byte_BL_IMM ,
/*  0xb4 */  i86op_mov_byte_AH_IMM ,
/*  0xb5 */  i86op_mov_byte_CH_IMM ,
/*  0xb6 */  i86op_mov_byte_DH_IMM ,
/*  0xb7 */  i86op_mov_byte_BH_IMM ,

/*  0xb8 */  i86op_mov_word_AX_IMM ,
/*  0xb9 */  i86op_mov_word_CX_IMM ,
/*  0xba */  i86op_mov_word_DX_IMM ,
/*  0xbb */  i86op_mov_word_BX_IMM ,
/*  0xbc */  i86op_mov_word_SP_IMM ,
/*  0xbd */  i86op_mov_word_BP_IMM ,
/*  0xbe */  i86op_mov_word_SI_IMM ,
/*  0xbf */  i86op_mov_word_DI_IMM ,


/*  0xc0 */  i86op_illegal_op ,
/*  0xc1 */  i86op_illegal_op ,
/*  0xc2 */  i86op_ret_near_IMM,
/*  0xc3 */  i86op_ret_near,
/*  0xc4 */  i86op_les_R_IMM,
/*  0xc5 */  i86op_lds_R_IMM,
/*  0xc6 */  i86op_mov_byte_RM_IMM ,
/*  0xc7 */  i86op_mov_word_RM_IMM ,
/*  0xc8 */  i86op_illegal_op ,
/*  0xc9 */  i86op_illegal_op ,
/*  0xca */  i86op_ret_far_IMM,
/*  0xcb */  i86op_ret_far,
/*  0xcc */  i86op_int3,
/*  0xcd */  i86op_int_IMM,
/*  0xce */  i86op_into,
/*  0xcf */  i86op_iret,
	     
/*  0xd0 */  i86op_opcD0_byte_RM_1,
/*  0xd1 */  i86op_opcD1_word_RM_1,
/*  0xd2 */  i86op_opcD2_byte_RM_CL,
/*  0xd3 */  i86op_opcD3_word_RM_CL,
/*  0xd4 */  i86op_aam,
/*  0xd5 */  i86op_aad,
/*  0xd6 */  i86op_illegal_op ,
/*  0xd7 */  i86op_xlat,
/*  0xd8 */  i86op_esc_coprocess_d8 ,
/*  0xd9 */  i86op_esc_coprocess_d9 ,
/*  0xda */  i86op_esc_coprocess_da ,
/*  0xdb */  i86op_esc_coprocess_db ,
/*  0xdc */  i86op_esc_coprocess_dc ,
/*  0xdd */  i86op_esc_coprocess_dd ,
/*  0xde */  i86op_esc_coprocess_de ,
/*  0xdf */  i86op_esc_coprocess_df ,

/*  0xe0 */  i86op_loopne ,
/*  0xe1 */  i86op_loope ,
/*  0xe2 */  i86op_loop ,
/*  0xe3 */  i86op_jcxz ,
/*  0xe4 */  i86op_in_byte_AL_IMM ,
/*  0xe5 */  i86op_in_word_AX_IMM ,
/*  0xe6 */  i86op_out_byte_IMM_AL ,
/*  0xe7 */  i86op_out_word_IMM_AX ,

/*  0xe8 */  i86op_call_near_IMM ,
/*  0xe9 */  i86op_jump_near_IMM ,
/*  0xea */  i86op_jump_far_IMM ,
/*  0xeb */  i86op_jump_byte_IMM ,
/*  0xec */  i86op_in_byte_AL_DX ,
/*  0xed */  i86op_in_word_AX_DX ,
/*  0xee */  i86op_out_byte_DX_AL ,
/*  0xef */  i86op_out_word_DX_AX ,

/*  0xf0 */  i86op_lock ,
/*  0xf1 */  i86op_illegal_op ,
/*  0xf2 */  i86op_repne ,
/*  0xf3 */  i86op_repe ,
/*  0xf4 */  i86op_halt ,
/*  0xf5 */  i86op_cmc ,
/*  0xf6 */  i86op_opcF6_byte_RM ,
/*  0xf7 */  i86op_opcF7_word_RM ,

/*  0xf8 */  i86op_clc ,
/*  0xf9 */  i86op_stc ,
/*  0xfa */  i86op_cli ,
/*  0xfb */  i86op_sti ,
/*  0xfc */  i86op_cld ,
/*  0xfd */  i86op_std ,
/*  0xfe */  i86op_opcFE_byte_RM ,
/*  0xff */  i86op_opcFF_word_RM ,

};


int N_i86_optab = sizeof (i86_optab) / sizeof(OP);

    

