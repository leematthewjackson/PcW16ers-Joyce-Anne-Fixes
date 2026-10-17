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

static char rcsid[]=
  "$Id: i87_ops.c,v 1.2 1991/07/30 02:06:32 hudgens Exp hudgens $";

/* $Log:
 *
 */




#include "i86int.hxx"



/* opcode=0xd8*/

void DEFUN(i86op_esc_coprocess_d8, (m), PC_ENV *m)
{
   START_OF_INSTR(m)
   DECODE_PRINTF(m,"ESC D8\n");
   DECODE_CLEAR_SEGOVR(m);
   END_OF_INSTR(m);
}


#ifdef DEBUG
const char *i87_op_d9_tab[] = 
  {
"FLD\tDWORD PTR ","ESC_D9\t","FST\tDWORD PTR ","FSTP\tDWORD PTR ",
"FLDENV\t","FLDCW\t","FSTENV\t","FSTCW\t",

"FLD\tDWORD PTR ","ESC_D9\t","FST\tDWORD PTR ","FSTP\tDWORD PTR ",
"FLDENV\t","FLDCW\t","FSTENV\t","FSTCW\t",

"FLD\tDWORD PTR ","ESC_D9\t","FST\tDWORD PTR ","FSTP\tDWORD PTR ",
"FLDENV\t","FLDCW\t","FSTENV\t","FSTCW\t",
};

const char *i87_op_d9_tab1[] =
  {
"FLD\t","FLD\t","FLD\t","FLD\t",
"FLD\t","FLD\t","FLD\t","FLD\t",

"FXCH\t","FXCH\t","FXCH\t","FXCH\t",
"FXCH\t","FXCH\t","FXCH\t","FXCH\t",

"FNOP","ESC_D9","ESC_D9","ESC_D9",
"ESC_D9","ESC_D9","ESC_D9","ESC_D9",

"FSTP\t","FSTP\t","FSTP\t","FSTP\t",
"FSTP\t","FSTP\t","FSTP\t","FSTP\t",

"FCHS","FABS","ESC_D9","ESC_D9",
"FTST","FXAM","ESC_D9","ESC_D9",

"FLD1","FLDL2T","FLDL2E","FLDPI",
"FLDLG2","FLDLN2","FLDZ","ESC_D9",

"F2XM1","FYL2X","FPTAN","FPATAN",
"FXTRACT","ESC_D9","FDECSTP","FINCSTP",

"FPREM","FYL2XP1","FSQRT","ESC_D9",
"FRNDINT","FSCALE","ESC_D9","ESC_D9",
};
    
#endif





/* opcode=0xd9*/
void DEFUN(i86op_esc_coprocess_d9, (m), PC_ENV *m)
{
    u_int16 mod,rl,rh;
    u_int16 destoffset;
    u_int8  stkelem;
  
    
    START_OF_INSTR(m)
    FETCH_DECODE_MODRM(m,mod,rh,rl);

#ifdef DEBUG
    if (mod != 3)
	  {
	    DECODE_PRINTINSTR32(m,i87_op_d9_tab,mod,rh,rl);
	  }
    else
	  {
	    DECODE_PRINTF(m,i87_op_d9_tab1[(rh<<3)+rl]); 
	  }
#endif
	   
    switch (mod) 
	  {
	  case 0:
	    destoffset=decode_rm00_address(m,rl);
	    DECODE_PRINTF(m,"\n");
	    break;
	  case 1:
	    destoffset=decode_rm01_address(m,rl);
	    DECODE_PRINTF(m,"\n");
	    break;
	  case 2:
	    destoffset=decode_rm10_address(m,rl);
	    DECODE_PRINTF(m,"\n");
	    break;
	  case 3:   /* register to register */
	    stkelem = rl;
	    if (rh < 4) 
	      DECODE_PRINTF2(m,"ST(%d)\n",stkelem);
	    else
	      DECODE_PRINTF(m,"\n");
	    break;
	  }

#ifdef I87_PRESENT
    /* execute */   
    switch (mod)
	  {
	  case 3:
	    switch(rh)
		  {
		    
		  case 0:
		    i87_R_fld(m,I87_STKTOP,stkelem);
		    break;
		  case 1:
		    i87_R_fxch(m,I87_STKTOP,stkelem);
		    break;
		  case 2:
		    switch (rl)
			  {
			  case 0:
			    i87_R_nop(m);
			    break;
			  default:
			    i87_illegal(m);
			    break;
			  }
		  case 3:
		    i87_R_fstp(m,I87_STKTOP,stkelem);
		    break;
		  case 4:
		    switch(rl)
			  {
			  case 0:
			    i87_R_fchs(m,I87_STKTOP);
			    break;
			  case 1:
			    i87_R_fabs(m,I87_STKTOP);
			    break;
			  case 4:
			    i87_R_ftst(m,I87_STKTOP);
			    break;
			  case 5:
			    i87_R_fxam(m,I87_STKTOP);
			    break;
			  default:
			    /* 2,3,6,7 */
			    i87_illegal(m);
			    break;
			  }
		    break;
		    
		  case 5:
		    switch(rl)
			  {
			  case 0:
			    i87_R_fld1(m,I87_STKTOP);
			    break;
			  case 1:
			    i87_R_fldl2t(m,I87_STKTOP);
			    break;
			  case 2:
			    i87_R_fldl2e(m,I87_STKTOP);
			    break;
			  case 3:
			    i87_R_fldpi(m,I87_STKTOP);
			    break;
			  case 4:
			    i87_R_fldlg2(m,I87_STKTOP);
			    break;
			  case 5:
			    i87_R_fldln2(m,I87_STKTOP);
			    break;
			  case 6:
			    i87_R_fldz(m,I87_STKTOP);
			    break;
			  default:
			    /* 7 */
			    i87_illegal(m);
			    break;
			  }
		    break;
		    

		  case 6:
		    switch(rl)
			  {
			  case 0:
			    i87_R_f2xm1(m,I87_STKTOP);
			    break;
			  case 1:
			    i87_R_fyl2x(m,I87_STKTOP);
			    break;
			  case 2:
			    i87_R_fptan(m,I87_STKTOP);
			    break;
			  case 3:
			    i87_R_fpatan(m,I87_STKTOP);
			    break;
			  case 4:
			    i87_R_fxtract(m,I87_STKTOP);
			    break;
			  case 5:
			    i87_illegal(m);
			    break;
			  case 6:
			    i87_R_decstp(m);
			    break;
			  case 7:
			    i87_R_incstp(m);
			    break;
			  }
		    break;
		    
		  case 7:
		    switch(rl)
			  {
			  case 0:
			    i87_R_fprem(m,I87_STKTOP);
			    break;
			  case 1:
			    i87_R_fyl2xp1(m,I87_STKTOP);
			    break;
			  case 2:
			    i87_R_fsqrt(m,I87_STKTOP);
			    break;
			  case 3:
			    i87_illegal(m);
			    break;
			  case 4:
			    i87_R_frndint(m,I87_STKTOP);
			    break;
			  case 5:
			    i87_R_fscale(m,I87_STKTOP);
			    break;
			  case 6:
			  case 7:
			  default:
			    i87_illegal(m);
			    break;
			  }
		    
				

	    
	    break;
	  default:
	    switch(rh)
		  {
		  case 0:
		    i87_M_fld(m,I87_FLOAT,destoffset);
		    break;
		  case 1:
		    i87_illegal(m);
		    break;
		  case 2:
		    i87_M_fst(m,I87_FLOAT,destoffset);
		    break;
		  case 3:
		    i87_M_fstp(m,I87_FLOAT,destoffset);
		    break;
		  case 4:
		    i87_M_fldenv(m,I87_WORD,destoffset);
		    break;
		  case 5:
		    i87_M_fldcw(m,I87_WORD,destoffset);
		    break;
		  case 6:
		    i87_M_fstenv(m,I87_WORD,destoffset);
		    break;
		  case 7:
		    i87_M_fstcw(m,I87_WORD,destoffset);
		    break;
		  }
	  }
#endif   
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);

}


#ifdef DEBUG

const char *i87_op_da_tab[] = 
 {
"FIADD\tDWORD PTR ","FIMUL\tDWORD PTR ","FICOM\tDWORD PTR ","FICOMP\tDWORD PTR ",
"FISUB\tDWORD PTR ","FISUBR\tDWORD PTR ","FIDIV\tDWORD PTR ","FIDIVR\tDWORD PTR ",

"FIADD\tDWORD PTR ","FIMUL\tDWORD PTR ","FICOM\tDWORD PTR ","FICOMP\tDWORD PTR ",
"FISUB\tDWORD PTR ","FISUBR\tDWORD PTR ","FIDIV\tDWORD PTR ","FIDIVR\tDWORD PTR ",

"FIADD\tDWORD PTR ","FIMUL\tDWORD PTR ","FICOM\tDWORD PTR ","FICOMP\tDWORD PTR ",
"FISUB\tDWORD PTR ","FISUBR\tDWORD PTR ","FIDIV\tDWORD PTR ","FIDIVR\tDWORD PTR ",

"ESC_DA ","ESC_DA ","ESC_DA ","ESC_DA ",
"ESC_DA	","ESC_DA ","ESC_DA	","ESC_DA ",
};
#endif



/* opcode=0xda*/
void DEFUN(i86op_esc_coprocess_da, (m), PC_ENV *m)
{
    u_int16 mod,rl,rh;
    u_int16 destoffset;
    u_int8  stkelem;
  
    
    START_OF_INSTR(m)
    FETCH_DECODE_MODRM(m,mod,rh,rl);
    
    DECODE_PRINTINSTR32(m,i87_op_da_tab,mod,rh,rl);
    
    switch (mod) 
	  {
	  case 0:
	    destoffset=decode_rm00_address(m,rl);
	    DECODE_PRINTF(m,"\n");
	    break;
	  case 1:
	    destoffset=decode_rm01_address(m,rl);
	    DECODE_PRINTF(m,"\n");
	    break;
	  case 2:
	    destoffset=decode_rm10_address(m,rl);
	    DECODE_PRINTF(m,"\n");
	    break;
	  case 3:   /* register to register */
	    stkelem = rl;
	    DECODE_PRINTF2(m,"\tST(%d),ST\n",stkelem);
	    break;
	  }

#ifdef I87_PRESENT
    /* execute */   
    switch (mod)
	  {
	  case 3:
	    i87_illegal(m);
	    break;
	  default:
	    switch(rh)
		  {
		  case 0:
		    i87_M_iadd(m,I87_SHORT,destoffset);
		    break;
		  case 1:
		    i87_M_imul(m,I87_SHORT,destoffset);
		    break;
		  case 2:
		    i87_M_icom(m,I87_SHORT,destoffset);
		    break;
		  case 3:
		    i87_M_icomp(m,I87_SHORT,destoffset);
		    break;
		  case 4:
		    i87_M_isub(m,I87_SHORT,destoffset);
		    break;
		  case 5:
		    i87_M_isubr(m,I87_SHORT,destoffset);
		    break;
		  case 6:
		    i87_M_idiv(m,I87_SHORT,destoffset);
		    break;
		  case 7:
		    i87_M_idivr(m,I87_SHORT,destoffset);
		    break;
		  }
	  }
#endif   
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}

#ifdef DEBUG
const char *i87_op_db_tab[] = 
 {
"FILD\tDWORD PTR ","ESC_DB\t19","FIST\tDWORD PTR ","FISTP\tDWORD PTR ",
"ESC_DB\t1C","FLD\tTBYTE PTR ","ESC_DB\t1E","FSTP\tTBYTE PTR ",

"FILD\tDWORD PTR ","ESC_DB\t19","FIST\tDWORD PTR ","FISTP\tDWORD PTR ",
"ESC_DB\t1C","FLD\tTBYTE PTR ","ESC_DB\t1E","FSTP\tTBYTE PTR ",

"FILD\tDWORD PTR ","ESC_DB\t19","FIST\tDWORD PTR ","FISTP\tDWORD PTR ",
"ESC_DB\t1C","FLD\tTBYTE PTR ","ESC_DB\t1E","FSTP\tTBYTE PTR ",

};
#endif
   

/* opcode=0xdb*/
void DEFUN(i86op_esc_coprocess_db, (m), PC_ENV *m)
{
    u_int16 mod,rl,rh;
    u_int16 destoffset;
    u_int8  stkelem;
  
    
    START_OF_INSTR(m)
    FETCH_DECODE_MODRM(m,mod,rh,rl);

#ifdef DEBUG
    if (mod != 3)
	{
	   DECODE_PRINTINSTR32(m,i87_op_db_tab,mod,rh,rl);
	}
    else if (rh == 4)  /* === 11 10 0 nnn */
        {
	   switch(rl)
	       {
		case 0:  DECODE_PRINTF(m,"FENI\n"); break;
		case 1:  DECODE_PRINTF(m,"FDISI\n"); break;
		case 2:  DECODE_PRINTF(m,"FCLEX\n"); break;  
		case 3:  DECODE_PRINTF(m,"FINIT\n"); break;  
	       }
	}
    else
	{
	   DECODE_PRINTF2(m,"ESC_DB %0x\n",(mod<<6)+(rh<<3)+(rl)); 
	}
#endif    
	   
    
    switch (mod) 
	  {
	  case 0:
	    destoffset=decode_rm00_address(m,rl);
	    break;
	  case 1:
	    destoffset=decode_rm01_address(m,rl);
	    break;
	  case 2:
	    destoffset=decode_rm10_address(m,rl);
	    break;
	  case 3:   /* register to register */
	    break;
	  }

#ifdef I87_PRESENT
    /* execute */   
    switch (mod)
	  {
	  case 3:
	    switch(rh)
		  {
		  case 4:
		     switch(rl)
			 {
			  case 0:
			    i87_R_feni(m);
			    break;
			  case 1:
			    i87_R_fdisi(m);
			    break;
			  case 2:
			    i87_R_fclex(m);
			    break;
			  case 3:
			    i87_R_finit(m);
			    break;
			  default:
			    i87_illegal(m);
			    break;
			 }
		     break;
		   default:
		     i87_illegal(m);
		     break;	
		  }
	    break;
	  default:
	    switch(rh)
		  {
		  case 0:
		    i87_M_fild(m,I87_SHORT,destoffset);
		    break;
		  case 1:
		    i87_illegal(m);
		    break;
		  case 2:
		    i87_M_fist(m,I87_SHORT,destoffset);
		    break;
		  case 3:
		    i87_M_fistp(m,I87_SHORT,destoffset);
		    break;
		  case 4:
		    i87_illegal(m);
		    break;
		  case 5:
		    i87_M_fld(m,I87_LDBL,destoffset);
		    break;
		  case 6:
		    i87_illegal(m);
		    break;
		  case 7:
		    i87_M_fstp(m,I87_LDBL,destoffset);
		    break;
		  }
	  }
#endif   
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


#ifdef DEBUG
const char *i87_op_dc_tab[] = {
"FADD\tQWORD PTR ","FMUL\tQWORD PTR ","FCOM\tQWORD PTR ","FCOMP\tQWORD PTR ",
"FSUB\tQWORD PTR ","FSUBR\tQWORD PTR ","FDIV\tQWORD PTR ","FDIVR\tQWORD PTR ",

"FADD\tQWORD PTR ","FMUL\tQWORD PTR ","FCOM\tQWORD PTR ","FCOMP\tQWORD PTR ",
"FSUB\tQWORD PTR ","FSUBR\tQWORD PTR ","FDIV\tQWORD PTR ","FDIVR\tQWORD PTR ",

"FADD\tQWORD PTR ","FMUL\tQWORD PTR ","FCOM\tQWORD PTR ","FCOMP\tQWORD PTR ",
"FSUB\tQWORD PTR ","FSUBR\tQWORD PTR ","FDIV\tQWORD PTR ","FDIVR\tQWORD PTR ",

"FADD\t","FMUL\t","FCOM\t","FCOMP\t",
"FSUBR\t","FSUB\t","FDIVR\t","FDIV\t",
};
#endif    

/* opcode=0xdc*/
void DEFUN(i86op_esc_coprocess_dc, (m), PC_ENV *m)	   
{
    u_int16 mod,rl,rh;
    u_int16 destoffset;
    u_int8  stkelem;
  
    
    START_OF_INSTR(m)
    FETCH_DECODE_MODRM(m,mod,rh,rl);
    
    DECODE_PRINTINSTR32(m,i87_op_dc_tab,mod,rh,rl);
    
    switch (mod) 
	  {
	  case 0:
	    destoffset=decode_rm00_address(m,rl);
	    DECODE_PRINTF(m,"\n");
	    break;
	  case 1:
	    destoffset=decode_rm01_address(m,rl);
	    DECODE_PRINTF(m,"\n");
	    break;
	  case 2:
	    destoffset=decode_rm10_address(m,rl);
	    DECODE_PRINTF(m,"\n");
	    break;
	  case 3:   /* register to register */
	    stkelem = rl;
	    DECODE_PRINTF2(m,"\tST(%d),ST\n",stkelem);
	    break;
	  }

#ifdef I87_PRESENT
    /* execute */   
    switch (mod)
	  {
	  case 3:
	    switch(rh)
		  {
		  case 0:
		    i87_R_fadd(m,stkelem,I87_STKTOP);
		    break;
		  case 1:
		    i87_R_fmul(m,stkelem,I87_STKTOP);
		    break;
		  case 2:
		    i87_R_fcom(m,stkelem,I87_STKTOP);
		    break;
		  case 3:
		    i87_R_fcomp(m,stkelem,I87_STKTOP); 
		    break;
		  case 4:
		    i87_R_fsubr(m,stkelem,I87_STKTOP);
		    break;
		  case 5:
		    i87_R_fsub(m,stkelem,I87_STKTOP);
		    break;
		  case 6:
		    i87_R_fdivr(m,stkelem,I87_STKTOP);
		    break;
		  case 7:
		    i87_R_fdiv(m,stkelem,I87_STKTOP);
		    break;
		  }		    
	    break;
	  default:
	    switch(rh)
		  {
		  case 0:
		    i87_M_fadd(m,I87_DOUBLE,destoffset);
		    break;
		  case 1:
		    i87_M_fmul(m,I87_DOUBLE,destoffset);
		    break;
		  case 2:
		    i87_M_fcom(m,I87_DOUBLE,destoffset);
		    break;
		  case 3:
		    i87_M_fcomp(m,I87_DOUBLE,destoffset);
		    break;
		  case 4:
		    i87_M_fsub(m,I87_DOUBLE,destoffset);
		    break;
		  case 5:
		    i87_M_fsubr(m,I87_DOUBLE,destoffset);
		    break;
		  case 6:
		    i87_M_fdiv(m,I87_DOUBLE,destoffset);
		    break;
		  case 7:
		    i87_M_fdivr(m,I87_DOUBLE,destoffset);
		    break;
		  }
	  }
#endif   
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


#ifdef DEBUG
static const char *i87_op_dd_tab[] = {
"FLD\tQWORD PTR ","ESC_DD\t29,","FST\tQWORD PTR ","FSTP\tQWORD PTR ",
"FRSTOR\t","ESC_DD\t2D,","FSAVE\t","FSTSW\t",

"FLD\tQWORD PTR ","ESC_DD\t29,","FST\tQWORD PTR ","FSTP\tQWORD PTR ",
"FRSTOR\t","ESC_DD\t2D,","FSAVE\t","FSTSW\t",

"FLD\tQWORD PTR ","ESC_DD\t29,","FST\tQWORD PTR ","FSTP\tQWORD PTR ",
"FRSTOR\t","ESC_DD\t2D,","FSAVE\t","FSTSW\t",

"FFREE\t","FXCH\t","FST\t","FSTP\t",
"ESC_DD\t2C,","ESC_DD\t2D,","ESC_DD\t2E,","ESC_DD\t2F,",
};
#endif

/* opcode=0xdd*/
void DEFUN(i86op_esc_coprocess_dd, (m), PC_ENV *m)
{
    u_int16 mod,rl,rh;
    u_int16 destoffset;
    u_int8  stkelem;
  
    
    START_OF_INSTR(m)
    FETCH_DECODE_MODRM(m,mod,rh,rl);
    
    DECODE_PRINTINSTR32(m,i87_op_dd_tab,mod,rh,rl);
    
    switch (mod) 
	  {
	  case 0:
	    destoffset=decode_rm00_address(m,rl);
	    DECODE_PRINTF(m,"\n");
	    break;
	  case 1:
	    destoffset=decode_rm01_address(m,rl);
	    DECODE_PRINTF(m,"\n");
	    break;
	  case 2:
	    destoffset=decode_rm10_address(m,rl);
	    DECODE_PRINTF(m,"\n");
	    break;
	  case 3:   /* register to register */
	    stkelem = rl;
	    DECODE_PRINTF2(m,"\tST(%d),ST\n",stkelem);
	    break;
	  }

#ifdef I87_PRESENT
    /* execute */   
    switch (mod)
	  {
	  case 3:
	    switch(rh)
		  {
		  case 0:
		    i87_R_ffree(m,stkelem);
		    break;
		  case 1:
		    i87_R_fxch(m,stkelem);
		    break;
		  case 2:
		    i87_R_fst(m,stkelem);  /* register version */
		    break;
		  case 3:
    		    i87_R_fstp(m,stkelem);  /* register version */
		    break;
		  default:
		    i87_illegal(m);
		    break;
		  }		    
	    break;
	  default:
	    switch(rh)
		  {
		  case 0:
		    i87_M_fld(m,I87_DOUBLE,destoffset);
		    break;
		  case 1:
		    i87_illegal(m);
		    break;
		  case 2:
		    i87_M_fst(m,I87_DOUBLE,destoffset);
		    break;
		  case 3:
		    i87_M_fstp(m,I87_DOUBLE,destoffset);
		    break;
		  case 4:
		    i87_M_frstor(m,I87_WORD,destoffset);
		    break;
		  case 5:

		    i87_illegal(m);
		    break;
		  case 6:
		    i87_M_fsave(m,I87_WORD,destoffset);
		    break;
		  case 7:
		    i87_M_fstsw(m,I87_WORD,destoffset);
		    break;
		  }
	  }
#endif   
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}



#ifdef DEBUG
static const char *i87_op_de_tab[] = {
"FIADD\tWORD PTR ","FIMUL\tWORD PTR ","FICOM\tWORD PTR ","FICOMP\tWORD PTR ",
"FISUB\tWORD PTR ","FISUBR\tWORD PTR ","FIDIV\tWORD PTR ","FIDIVR\tWORD PTR ",

"FIADD\tWORD PTR ","FIMUL\tWORD PTR ","FICOM\tWORD PTR ","FICOMP\tWORD PTR ",
"FISUB\tWORD PTR ","FISUBR\tWORD PTR ","FIDIV\tWORD PTR ","FIDIVR\tWORD PTR ",

"FIADD\tWORD PTR ","FIMUL\tWORD PTR ","FICOM\tWORD PTR ","FICOMP\tWORD PTR ",
"FISUB\tWORD PTR ","FISUBR\tWORD PTR ","FIDIV\tWORD PTR ","FIDIVR\tWORD PTR ",

"FADDP\t","FMULP\t","FCOMP\t","FCOMPP\t",
"FSUBRP\t","FSUBP\t","FDIVRP\t","FDIVP\t",
};
#endif
       

/* opcode=0xde*/
void DEFUN(i86op_esc_coprocess_de, (m), PC_ENV *m)
  {
    u_int16 mod,rl,rh;
    u_int16 destoffset;
    u_int8  stkelem;
    
    
    START_OF_INSTR(m)
    FETCH_DECODE_MODRM(m,mod,rh,rl);
    
    DECODE_PRINTINSTR32(m,i87_op_de_tab,mod,rh,rl);
    
    switch (mod) 
	  {
	  case 0:
	    destoffset=decode_rm00_address(m,rl);
	    DECODE_PRINTF(m,"\n");
	    break;
	  case 1:
	    destoffset=decode_rm01_address(m,rl);
	    DECODE_PRINTF(m,"\n");
	    break;
	  case 2:
	    destoffset=decode_rm10_address(m,rl);
	    DECODE_PRINTF(m,"\n");
	    break;
	  case 3:   /* register to register */
	    stkelem = rl;
	    DECODE_PRINTF2(m,"\tST(%d),ST\n",stkelem);
	    break;
	  }

#ifdef I87_PRESENT
    /* execute */   
    switch (mod)
	  {
	  case 3:
	    switch(rh)
		  {
		  case 0:
		    i87_R_faddp(m,stkelem,I87_STKTOP);
		    break;
		  case 1:
		    i87_R_fmulp(m,stkelem,I87_STKTOP);
		    break;
		  case 2:
		    i87_R_fcomp(m,stkelem,I87_STKTOP);
		    break;
		  case 3:
		    if (stkelem == 1)
		      i87_R_fcompp(m,stkelem,I87_STKTOP); 
		    else
		      i87_illegal(m);
		    break;
		  case 4:
		    i87_R_fsubrp(m,stkelem,I87_STKTOP);
		    break;
		  case 5:
		    i87_R_fsubp(m,stkelem,I87_STKTOP);
		    break;
		  case 6:
		    i87_R_fdivrp(m,stkelem,I87_STKTOP);
		    break;
		  case 7:
		    i87_R_fdivp(m,stkelem,I87_STKTOP);
		    break;
		  }		    
	    break;
	  default:
	    switch(rh)
		  {
		  case 0:
		    i87_M_fiadd(m,I87_WORD,destoffset);
		    break;
		  case 1:
		    i87_M_fimul(m,I87_WORD,destoffset);
		    break;
		  case 2:
		    i87_M_ficom(m,I87_WORD,destoffset);
		    break;
		  case 3:
		    i87_M_ficomp(m,I87_WORD,destoffset);
		    break;
		  case 4:
		    i87_M_fisub(m,I87_WORD,destoffset);
		    break;
		  case 5:
		    i87_M_fisubr(m,I87_WORD,destoffset);
		    break;
		  case 6:
		    i87_M_fidiv(m,I87_WORD,destoffset);
		    break;
		  case 7:
		    i87_M_fidivr(m,I87_WORD,destoffset);
		    break;
		  }
	  }
#endif   
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
}


#ifdef DEBUG
static const char *i87_op_df_tab[] = {
/* mod == 00 */
"FILD\tWORD PTR ","ESC_DF\t39\n","FIST\tWORD PTR ","FISTP\tWORD PTR ",
"FBLD\tTBYTE PTR ","FILD\tQWORD PTR ","FBSTP\tTBYTE PTR ","FISTP\tQWORD PTR ",
/* mod == 01 */
"FILD\tWORD PTR ","ESC_DF\t39 ","FIST\tWORD PTR ","FISTP\tWORD PTR ",
"FBLD\tTBYTE PTR ","FILD\tQWORD PTR ","FBSTP\tTBYTE PTR ","FISTP\tQWORD PTR ",
/* mod == 10 */
"FILD\tWORD PTR ","ESC_DF\t39 ","FIST\tWORD PTR ","FISTP\tWORD PTR ",
"FBLD\tTBYTE PTR ","FILD\tQWORD PTR ","FBSTP\tTBYTE PTR ","FISTP\tQWORD PTR ",
/* mod == 11 */
"FFREE\t","FXCH\t","FST\t","FSTP\t",
"ESC_DF\t3C,","ESC_DF\t3D,","ESC_DF\t3E,","ESC_DF\t3F,"
};
#endif

/* opcode=0xdf*/
void DEFUN(i86op_esc_coprocess_df, (m), PC_ENV *m)
  {
    u_int16 mod,rl,rh;
    u_int16 destoffset;
    u_int8  stkelem;
    
    START_OF_INSTR(m)
    FETCH_DECODE_MODRM(m,mod,rh,rl);
    
    DECODE_PRINTINSTR32(m,i87_op_df_tab,mod,rh,rl);
    
    switch (mod) 
	  {
	  case 0:
	    destoffset=decode_rm00_address(m,rl);
	    DECODE_PRINTF(m,"\n");
	    break;
	  case 1:
	    destoffset=decode_rm01_address(m,rl);
	    DECODE_PRINTF(m,"\n");
	    break;
	  case 2:
	    destoffset=decode_rm10_address(m,rl);
	    DECODE_PRINTF(m,"\n");
	    break;
	  case 3:   /* register to register */
	    stkelem = rl;
	    DECODE_PRINTF2(m,"\tST(%d)\n",stkelem);
	    break;
	  }
    
#ifdef I87_PRESENT
    /* begin  execution of instruction */   
    switch (mod)
	  {
	  case 3:
	    switch(rh)
		  {
		  case 0:
		    i87_R_ffree(m,stkelem);
		    break;
		  case 1:
		    i87_R_fxch(m,stkelem);
		    break;
		  case 2:
		    i87_R_fst(m,stkelem);  /* register version */
		    break;
		  case 3:
    		    i87_R_fstp(m,stkelem);  /* register version */
 		    break;
		  default:
		    i87_illegal(m);
		    break;
		  }		    
	    break;
	  default:
	    switch(rh)
		  {
		  case 0:
		    i87_M_fild(m,I87_WORD,destoffset);
		    break;
		  case 1:
		    i87_illegal(m);
		    break;
		  case 2:
		    i87_M_fist(m,I87_WORD,destoffset);
		    break;
		  case 3:
		    i87_M_fistp(m,I87_WORD,destoffset);
		    break;
		  case 4:
		    i87_M_fbld(m,I87_BSD,destoffset);
		    break;
		  case 5:
		    i87_M_fild(m,I87_LONG,destoffset);
		    break;
		  case 6:
		    i87_M_fbstp(m,I87_BSD,destoffset);
		    break;
		  case 7:
		    i87_M_fistp(m,I87_LONG,destoffset);
		    break;
		  }
	  }
    
#endif   
    DECODE_CLEAR_SEGOVR(m);
    END_OF_INSTR(m);
  }





