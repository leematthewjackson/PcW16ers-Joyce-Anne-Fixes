
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

extern IS_VOLATILE int intr;

static char rcsid[]=
  "$Id:$";

/* $Log: $
 * Revision 0.05  1992/04/12  23:16:42  hudgens
 * made modrmtab external.
 * Many changes.   Added support for the QUICK_FETCH option,
 * so that memory accesses are faster.  Now compiles with gcc -Wall
 * and gcc -traditional and Sun cc.
 *
 * Revision 0.04  1991/07/30  01:59:56  hudgens
 * added copyright.
 *
 * Revision 0.03  1991/06/03  01:02:09  hudgens
 * fixed minor problems due to unsigned to signed short integer
 * promotions.
 *
 * Revision 0.02  1991/03/31  01:29:39  hudgens
 * Fixed segment handling (overrides, default segment accessed) in
 * routines  decode_rmXX_address and the {fetch,store}_data_{byte,word}.
 *
 * Revision 0.01  1991/03/30  21:59:49  hudgens
 * Initial checkin.
 *
 *
 */

/* this file includes subroutines which do:
   stuff involving decoding instruction formats.
   stuff involving accessess of immediate data via IP.
   etc.
*/

static int poevt = 0;

#ifdef QUICK_DECODE_RM
/* an optimization for quicker register decoding.  This table 
   is setup at teh start of the run, and never changed.  
   It stores the address of the particular byte/word registers
   in an array.  Compare this code with "decode_rm_byte_register"
   and "decode_rm_word_register"
   */

static void DEFUN(quick_decode_rm_init,(m),PC_ENV *m)
 {
    int i;
    
    for (i=0; i<8; i++)
	{
	   switch(i)
	       {
		case 0:	
		  m->RM_BYTE[i] = &m->R_AL;
		  m->RM_WORD[i] = &m->R_AX;
		  break;
		case 1:
		  m->RM_BYTE[i] = &m->R_CL;
		  m->RM_WORD[i] = &m->R_CX;
		  break;
		case 2:
		  m->RM_BYTE[i] = &m->R_DL;
		  m->RM_WORD[i] = &m->R_DX;
		  break;
		case 3:
		  m->RM_BYTE[i] = &m->R_BL;
		  m->RM_WORD[i] = &m->R_BX;
		  break;
		case 4:
		  m->RM_BYTE[i] = &m->R_AH;
		  m->RM_WORD[i] = &m->R_SP;
		  break;
		case 5:
		  m->RM_BYTE[i] = &m->R_CH;
		  m->RM_WORD[i] = &m->R_BP;
		  break;
		case 6:
		  m->RM_BYTE[i] = &m->R_DH;
		  m->RM_WORD[i] = &m->R_SI;
		  break;
		case 7:
		  m->RM_BYTE[i] = &m->R_BH;
		  m->RM_WORD[i] = &m->R_DI;
		  break;
	       }
	}
 }
#endif

#ifdef UNPACKED_FLAGS
void DEFUN(pack_flags,(m),PC_ENV *m)
 {
    m->R_FLG = 0;
    if (ACCESS_FLAG(m,F_CF))
      m->R_FLG |= FB_CF;
    if (ACCESS_FLAG(m,F_PF))
      m->R_FLG |= FB_PF;
    if (ACCESS_FLAG(m,F_AF))
      m->R_FLG |= FB_AF;
    if (ACCESS_FLAG(m,F_ZF))
      m->R_FLG |= FB_ZF;
    if (ACCESS_FLAG(m,F_SF))
      m->R_FLG |= FB_SF;
    if (ACCESS_FLAG(m,F_TF))
      m->R_FLG |= FB_TF;
    if (ACCESS_FLAG(m,F_IF))
      m->R_FLG |= FB_IF;
    if (ACCESS_FLAG(m,F_DF))
      m->R_FLG |= FB_DF;
    if (ACCESS_FLAG(m,F_OF)) 
      m->R_FLG |= FB_OF;
 }    


void DEFUN(unpack_flags,(m),PC_ENV *m)
 {
    int i;
    
    /* clear em all. */
    for (i=0; i<16; i++) CLEAR_FLAG(m,i);
    
    /* set the unpacked flags if the corresponding packed bit is set */
    if (m->R_FLG & FB_CF)
      SET_FLAG(m,F_CF);
    if (m->R_FLG & FB_PF)
      SET_FLAG(m,F_PF);
    if (m->R_FLG & FB_AF) 
      SET_FLAG(m,F_AF);
    if (m->R_FLG & FB_ZF)
      SET_FLAG(m,F_ZF);
    if (m->R_FLG & FB_SF)
      SET_FLAG(m,F_SF);
    if (m->R_FLG & FB_TF)
      SET_FLAG(m,F_TF);
    if (m->R_FLG & FB_IF)
      SET_FLAG(m,F_IF);
    if (m->R_FLG & FB_DF)
      SET_FLAG(m,F_DF);
    if (m->R_FLG & FB_OF)
      SET_FLAG(m,F_OF);
 }    
#endif

void PC_ENV::i86_intr_handle(void)
{
	u_int16 tmp;
	u_int8  intno;

	if (intr & INTR_SYNCH)   /* raised by something */
	{
/* If the interrupt came from something that supports the interrupt acknowledge
 * signal, acknowledge it */
		if (m_intNo == -1) intno = m_intDev->ackInterrupt();
		else		   intno = m_intNo;

		tmp = (u_int16) mem_access_word(this, intno * 4);  
/*	
 *	Call to emulated BIOS, which at the moment we don't have 
 *	       if (tmp == BIOS_SEG) 
	       {
		  (*intr_tab[intno])(intno,m);
		  RESYNCH_SEG_CS(m);
		  RESYNCH_SEG_DS(m);
		  RESYNCH_SEG_ES(m);
		  RESYNCH_SEG_SS(m);
	       }
	   else */
	       {
		  PACK_FLAGS(this);
		  tmp = R_FLG;
		  push_word(this, tmp);
		  CLEAR_FLAG(this, F_IF);
		  CLEAR_FLAG(this, F_TF);
/* [JCE] If we're interrupting between a segment override (or REP override) 
 * and the following instruction, decrease IP to get back to the prefix.
 *
 * Strictly speaking, I think we actually should postpone interrupts until
 * the next instruction instead...
 
		  if (sysmode &    (SYSMODE_SEGMASK     |
				    SYSMODE_PREFIX_REPE |
				    SYSMODE_PREFIX_REPNE))
		  {
//			 fprintf(stderr, "Backstepping for prefix\n");
			 --R_IP;
		  }
 */
/* [JCE] CS and IP were the wrong way round... */
		  push_word(this, R_CS);
		  push_word(this, R_IP);
		  tmp = mem_access_word(this, intno * 4); 
		  R_IP = tmp;
		  tmp = mem_access_word(this, intno * 4 + 2);
		  R_CS = tmp;
		  RESYNCH_SEG_CS(this);
	       }
    	intr &= ~INTR_SYNCH;	/* [JCE] Dealt with, reset flag */
	}
    	/* The interrupt code can't pick up the segment override status. */
 	DECODE_CLEAR_SEGOVR(this); 
 }    

void PC_ENV::i86_intr_raise(u_int8 intrnum)
{
	m_intNo = intrnum;
	m_intDev = NULL;
	intr |= INTR_SYNCH;
}
    
void PC_ENV::i86_intr_raise(IntAck *device)
{
	m_intNo = -1;
	m_intDev = device;
	intr |= INTR_SYNCH;
}

void PC_ENV::i86_intr_lower(IntAck *device)
{
	intr &= ~INTR_SYNCH;
}


/***********************************************************************/
/* MAIN EXECUTION LOOP.  Avoid unnecessary code here like the plague.  */
/***********************************************************************/

int PC_ENV::i86_exec(void)
{
    u_int8   op1 = 0;
 
    intr = 0;
    RESYNCH_SEG_CS(this);
    RESYNCH_SEG_DS(this);
    RESYNCH_SEG_ES(this);
    RESYNCH_SEG_SS(this);
    QUICK_DECODE_RM_INIT(this);
   
//    fprintf(stderr, "i86_exec() m_state=%d\n", m_state); 
    while(m_state != GS_Terminate)
    {
	if (m_state == GS_Stopped)
	{
		m_host->yield();
		continue;
	}
//    	fprintf(stderr, "i86_exec() m_state->%d\n", m_state); 

#ifdef DEBUG
	   if (CHECK_IP_FETCH(this))
	     check_ip_access(this);
#endif
	   clock_tick();

	   /* if debugging, save the IP and CS values. */
	   SAVE_IP_CS(this, R_CS, R_IP);
	   INC_DECODED_INST_LEN(this, 1);
	   
	   if (intr)
	       {
/* Don't make this depend on interrupts; in a system with a PIC, interrupts
 * may not reach the CPU at all
 * 		  poll_events(); */
		  if (intr & INTR_HALTED)
		      {
			 if (halted())
			 {
			 	debug_printf(this,"halted\n");
			 	trace_regs(this);
			 	return -1;
			 }
		      }

/* If we have just had a segment override or a REP prefix, don't allow 
 * interrupt. Otherwise the return from the interrupt will lose the 
 * prefix. */
 
		  if (!(sysmode &  (SYSMODE_SEGMASK     |
				    SYSMODE_PREFIX_REPE |
				    SYSMODE_PREFIX_REPNE)))
		  {
			  if (((intr&INTR_SYNCH)&&( m_intNo == 0 || m_intNo == 2)) ||
			      (ACCESS_FLAG(this,F_IF)))
/* [JCE] Reversed the sense of this ACCESS_FLAG; it's set for interrupts
 * enabled, not interrupts blocked */
			      /* i.e. either not blockable (intr 0 or 2) 
				 or the IF flag not set so interrupts not blocked */
			      {
				 i86_intr_handle();  
			      }
		  }
	       }

#ifdef XXX
	   if (this->R_CSP != mem_base + (this->R_CS << 4))
	     {
	       printf("CS out of synch: IP: 0x%04x CS: 0x%04x\n", R_IP, R_CS);
	     }
	   if (this->R_DSP != mem_base + (this->R_DS << 4))
	     printf("DS out of synch: IP: 0x%04x CS: 0x%04x\n", R_IP, R_CS);
	   if (this->R_ESP != mem_base + (this->R_ES << 4))
	     printf("CS out of synch: IP: 0x%04x CS: 0x%04x\n", R_IP, R_CS);
	   if (this->R_SSP != mem_base + (this->R_SS << 4))
	     printf("CS out of synch: IP: 0x%04x CS: 0x%04x\n", R_IP, R_CS);
#endif	   
	   
#ifdef QUICK_FETCH
	   op1  = R_CSP[R_IP++];
#else
	/* [JCE] Wrap at 0xFFFFF */
	   op1 = fetch(this->R_CS, R_IP++);
#endif
	   (* (i86_optab[op1]) ) (this); 
	}
//    fprintf(stderr, "i86_exec() terminated\n");
	return 0; 
}



void DEFUN(halt_sys,(m),PC_ENV *m)
 {
    intr |= INTR_HALTED;
 }   



/* once the instruction is fetched, an optional byte follows which 
   has 3 fields encoded in it.  This routine  fetches the byte
   and breaks into the three fields.  
   
   This has been changed, in an attempt to reduce the amount of 
   executed code for this frequently executed subroutine.  If this
   works, then it may pay to somehow inline it.
   */

#ifdef NOTDEF
/* this code generated the following table */
main()
 {
    int i;
    printf("\n\nstruct modrm{ u_int8 mod,rh,rl;} modrmtab[] = {\n");
    for (i=0; i<256; i++)
	{
	   printf("{%d,%d,%d}, ",((i&0xc0)>>6),((i&0x38)>>3),(i&0x07));
    	   if (i%4==3) 
	     printf("/* %d to %d */\n",i&0xfc,i);
	}
    printf("};\n\n");
 }
#endif    
    
    struct modrm modrmtab[] = {
	 {0,0,0}, {0,0,1}, {0,0,2}, {0,0,3}, /* 0 to 3 */
	  {0,0,4}, {0,0,5}, {0,0,6}, {0,0,7}, /* 4 to 7 */
	  {0,1,0}, {0,1,1}, {0,1,2}, {0,1,3}, /* 8 to 11 */
	  {0,1,4}, {0,1,5}, {0,1,6}, {0,1,7}, /* 12 to 15 */
	  {0,2,0}, {0,2,1}, {0,2,2}, {0,2,3}, /* 16 to 19 */
	  {0,2,4}, {0,2,5}, {0,2,6}, {0,2,7}, /* 20 to 23 */
	  {0,3,0}, {0,3,1}, {0,3,2}, {0,3,3}, /* 24 to 27 */
	  {0,3,4}, {0,3,5}, {0,3,6}, {0,3,7}, /* 28 to 31 */
	  {0,4,0}, {0,4,1}, {0,4,2}, {0,4,3}, /* 32 to 35 */
	  {0,4,4}, {0,4,5}, {0,4,6}, {0,4,7}, /* 36 to 39 */
	  {0,5,0}, {0,5,1}, {0,5,2}, {0,5,3}, /* 40 to 43 */
	  {0,5,4}, {0,5,5}, {0,5,6}, {0,5,7}, /* 44 to 47 */
	  {0,6,0}, {0,6,1}, {0,6,2}, {0,6,3}, /* 48 to 51 */
	  {0,6,4}, {0,6,5}, {0,6,6}, {0,6,7}, /* 52 to 55 */
	  {0,7,0}, {0,7,1}, {0,7,2}, {0,7,3}, /* 56 to 59 */
	  {0,7,4}, {0,7,5}, {0,7,6}, {0,7,7}, /* 60 to 63 */
	  {1,0,0}, {1,0,1}, {1,0,2}, {1,0,3}, /* 64 to 67 */
	  {1,0,4}, {1,0,5}, {1,0,6}, {1,0,7}, /* 68 to 71 */
	  {1,1,0}, {1,1,1}, {1,1,2}, {1,1,3}, /* 72 to 75 */
	  {1,1,4}, {1,1,5}, {1,1,6}, {1,1,7}, /* 76 to 79 */
	  {1,2,0}, {1,2,1}, {1,2,2}, {1,2,3}, /* 80 to 83 */
	  {1,2,4}, {1,2,5}, {1,2,6}, {1,2,7}, /* 84 to 87 */
	  {1,3,0}, {1,3,1}, {1,3,2}, {1,3,3}, /* 88 to 91 */
	  {1,3,4}, {1,3,5}, {1,3,6}, {1,3,7}, /* 92 to 95 */
	  {1,4,0}, {1,4,1}, {1,4,2}, {1,4,3}, /* 96 to 99 */
	  {1,4,4}, {1,4,5}, {1,4,6}, {1,4,7}, /* 100 to 103 */
	  {1,5,0}, {1,5,1}, {1,5,2}, {1,5,3}, /* 104 to 107 */
	  {1,5,4}, {1,5,5}, {1,5,6}, {1,5,7}, /* 108 to 111 */
	  {1,6,0}, {1,6,1}, {1,6,2}, {1,6,3}, /* 112 to 115 */
	  {1,6,4}, {1,6,5}, {1,6,6}, {1,6,7}, /* 116 to 119 */
	  {1,7,0}, {1,7,1}, {1,7,2}, {1,7,3}, /* 120 to 123 */
	  {1,7,4}, {1,7,5}, {1,7,6}, {1,7,7}, /* 124 to 127 */
	  {2,0,0}, {2,0,1}, {2,0,2}, {2,0,3}, /* 128 to 131 */
	  {2,0,4}, {2,0,5}, {2,0,6}, {2,0,7}, /* 132 to 135 */
	  {2,1,0}, {2,1,1}, {2,1,2}, {2,1,3}, /* 136 to 139 */
	  {2,1,4}, {2,1,5}, {2,1,6}, {2,1,7}, /* 140 to 143 */
	  {2,2,0}, {2,2,1}, {2,2,2}, {2,2,3}, /* 144 to 147 */
	  {2,2,4}, {2,2,5}, {2,2,6}, {2,2,7}, /* 148 to 151 */
	  {2,3,0}, {2,3,1}, {2,3,2}, {2,3,3}, /* 152 to 155 */
	  {2,3,4}, {2,3,5}, {2,3,6}, {2,3,7}, /* 156 to 159 */
	  {2,4,0}, {2,4,1}, {2,4,2}, {2,4,3}, /* 160 to 163 */
	  {2,4,4}, {2,4,5}, {2,4,6}, {2,4,7}, /* 164 to 167 */
	  {2,5,0}, {2,5,1}, {2,5,2}, {2,5,3}, /* 168 to 171 */
	  {2,5,4}, {2,5,5}, {2,5,6}, {2,5,7}, /* 172 to 175 */
	  {2,6,0}, {2,6,1}, {2,6,2}, {2,6,3}, /* 176 to 179 */
	  {2,6,4}, {2,6,5}, {2,6,6}, {2,6,7}, /* 180 to 183 */
	  {2,7,0}, {2,7,1}, {2,7,2}, {2,7,3}, /* 184 to 187 */
	  {2,7,4}, {2,7,5}, {2,7,6}, {2,7,7}, /* 188 to 191 */
	  {3,0,0}, {3,0,1}, {3,0,2}, {3,0,3}, /* 192 to 195 */
	  {3,0,4}, {3,0,5}, {3,0,6}, {3,0,7}, /* 196 to 199 */
	  {3,1,0}, {3,1,1}, {3,1,2}, {3,1,3}, /* 200 to 203 */
	  {3,1,4}, {3,1,5}, {3,1,6}, {3,1,7}, /* 204 to 207 */
	  {3,2,0}, {3,2,1}, {3,2,2}, {3,2,3}, /* 208 to 211 */
	  {3,2,4}, {3,2,5}, {3,2,6}, {3,2,7}, /* 212 to 215 */
	  {3,3,0}, {3,3,1}, {3,3,2}, {3,3,3}, /* 216 to 219 */
	  {3,3,4}, {3,3,5}, {3,3,6}, {3,3,7}, /* 220 to 223 */
	  {3,4,0}, {3,4,1}, {3,4,2}, {3,4,3}, /* 224 to 227 */
	  {3,4,4}, {3,4,5}, {3,4,6}, {3,4,7}, /* 228 to 231 */
	  {3,5,0}, {3,5,1}, {3,5,2}, {3,5,3}, /* 232 to 235 */
	  {3,5,4}, {3,5,5}, {3,5,6}, {3,5,7}, /* 236 to 239 */
	  {3,6,0}, {3,6,1}, {3,6,2}, {3,6,3}, /* 240 to 243 */
	  {3,6,4}, {3,6,5}, {3,6,6}, {3,6,7}, /* 244 to 247 */
	  {3,7,0}, {3,7,1}, {3,7,2}, {3,7,3}, /* 248 to 251 */
	  {3,7,4}, {3,7,5}, {3,7,6}, {3,7,7}, /* 252 to 255 */
      };
    
    
    void  
      DEFUN(fetch_decode_modrm,
	    (m,mod,regh,regl),
	    PC_ENV *m AND u_int16 *mod AND  u_int16 *regh AND u_int16 *regl)
 {
    u_int8 fetched;
    register struct modrm *p;
    
#ifdef DEBUG
    if (CHECK_IP_FETCH(m))
      check_ip_access(m);
#endif
    
    /* do the fetch in real mode.  Shift the CS segment register
       over by 4 bits, and add in the IP register.  Index into 
       the system memory.  
       */
    
#ifdef QUICK_FETCH
    fetched = R_CSP[m->R_IP];
    m->R_IP += 1;
#else
    /* [JCE] Switch to virtual functions for store/fetch */
    fetched = m->fetch(m->R_CS, m->R_IP++);
#endif
    
    INC_DECODED_INST_LEN(m, 1);
    
#ifdef NotDef
    *mod = ((fetched&0xc0)>>6);
    *regh= ((fetched&0x38)>>3);
    *regl= (fetched&0x7);
#else
    p = modrmtab + fetched;
    *mod = p->mod;
    *regh= p->rh;
    *regl= p->rl;
#endif
    
 }
    
    /*  
      return a pointer to the register given by the R/RM field of 
      the modrm byte, for byte operands.
      Also enables the decoding of instructions.
      */
    
    u_int8 *
      DEFUN(decode_rm_byte_register,
	    (m,reg),
	    PC_ENV *m AND int reg)
 {
    switch(reg)
	{
	 case 0:	
	   DECODE_PRINTF(m,"AL");
	   return &m->R_AL;
	   break;
	 case 1:
	   DECODE_PRINTF(m,"CL");
	   return &m->R_CL;
	   break;
	 case 2:
	   DECODE_PRINTF(m,"DL");
	   return &m->R_DL;
	   break;
	 case 3:
	   DECODE_PRINTF(m,"BL");
	   return &m->R_BL;
	   break;
	 case 4:
	   DECODE_PRINTF(m,"AH");
	   return &m->R_AH;
	   break;
	 case 5:
	   DECODE_PRINTF(m,"CH");
	   return &m->R_CH;
	   break;
	 case 6:
	   DECODE_PRINTF(m,"DH");
	   return &m->R_DH;
	   break;
	 case 7:
	   DECODE_PRINTF(m,"BH");
	   return &m->R_BH;
	   break;
	}
    halt_sys(m);
    return NULL; /* NOT REACHED OR REACHED ON ERROR */
 }
    
    
    
    
    /*  
      return a pointer to the register given by the R/RM field of 
      the modrm byte, for word operands.
      Also enables the decoding of instructions.
      */
    
    u_int16 *
      DEFUN(decode_rm_word_register,
	    (m,reg),
	    PC_ENV *m AND int reg)
    
 {
    switch(reg)
	{
	 case 0:	
	   DECODE_PRINTF(m,"AX");
	   return &m->R_AX;
	   break;
	 case 1:
	   DECODE_PRINTF(m,"CX");
	   return &m->R_CX;
	   break;
	 case 2:
	   DECODE_PRINTF(m,"DX");
	   return &m->R_DX;
	   break;
	 case 3:
	   DECODE_PRINTF(m,"BX");
	   return &m->R_BX;
	   break;
	 case 4:
	   DECODE_PRINTF(m,"SP");
	   return &m->R_SP;
	   break;
	 case 5:
	   DECODE_PRINTF(m,"BP");
	   return &m->R_BP;
	   break;
	 case 6:
	   DECODE_PRINTF(m,"SI");
	   return &m->R_SI;
	   break;
	 case 7:
	   DECODE_PRINTF(m,"DI");
	   return &m->R_DI;
	   break;
	}
    
    halt_sys(m);
    return NULL; /* NOTREACHED OR REACHED ON ERROR*/
    
 }
    
    
    
    /*  
      return a pointer to the register given by the R/RM field of 
      the modrm byte, for word operands, modified from above
      for the weirdo special case of segreg operands.
      Also enables the decoding of instructions.
      */
    
    u_int16 *
      DEFUN(decode_rm_seg_register,
	    (m,reg),
	    PC_ENV *m AND int reg)
 {
    switch(reg)
	{
	 case 0:		 
	   DECODE_PRINTF(m,"ES");
	   return &m->R_ES;
	   break;
	 case 1:
	   DECODE_PRINTF(m,"CS");
	   return &m->R_CS;
	   break;
	 case 2:
	   DECODE_PRINTF(m,"SS");
	   return &m->R_SS;
	   break;
	 case 3:
	   DECODE_PRINTF(m,"DS");
	   return &m->R_DS;
	   break;
	 case 4:
	 case 5:
	 case 6:
	 case 7:
	   DECODE_PRINTF(m,"ILLEGAL SEGREG");
	   break;
	}
    halt_sys(m);
    return NULL;  /* NOT REACHED OR REACHED ON ERROR */
    
 }
    
    
    
    /* once the instruction is fetched, an optional byte follows which 
       has 3 fields encoded in it.  This routine  fetches the byte
       and breaks into the three fields.  
       */
    
    
    u_int8 
      DEFUN(fetch_byte_imm,
	    (m),PC_ENV *m)
 {
    u_int8 fetched;
    
#ifdef DEBUG
    if (CHECK_IP_FETCH(m))
      check_ip_access(m);
#endif
    
    /* do the fetch in real mode.  Shift the CS segment register
       over by 4 bits, and add in the IP register.  Index into 
       the system memory.  
       */
    
#ifdef QUICK_FETCH
    fetched = m->R_CSP[m->R_IP];
    m->R_IP += 1;
#else
    /* [JCE] Switch to virtual functions for store/fetch */
    fetched = m->fetch(m->R_CS, m->R_IP++);
#endif
    
    INC_DECODED_INST_LEN(m, 1);
    
    return fetched;
    
 }
    
    
    u_int16  
      DEFUN(fetch_word_imm,(m),PC_ENV *m)
 {
    u_int16 fetched;
#ifdef QUICK_FETCH
    u_int8  *p;
#endif
    
#ifdef DEBUG
    if (CHECK_IP_FETCH(m))
      check_ip_access(m);
#endif
    
    /* do the fetch in real mode.  Shift the CS segment register
       over by 4 bits, and add in the IP register.  Index into 
       the system PC_ENVory.  
       */
    
#ifdef QUICK_FETCH
    p = (u_int8 *) &fetched;
#ifdef WORDS_BIGENDIAN
    *p = m->R_CSP[m->R_IP+1]; 
    ++p;
    *p = m->R_CSP[m->R_IP]; 
    m->R_IP += 2;
#else
    *p = m->R_CSP[m->R_IP]; 
    ++p;
    *p = m->R_CSP[m->R_IP+1]; 
    m->R_IP += 2;
#endif	
    
#else    /* QUICK FETCH  */
    
    /* [JCE] Switch to virtual functions for store/fetch */
    fetched = m->fetch16(m->R_CS, m->R_IP); m->R_IP += 2;
    
#endif
    
    INC_DECODED_INST_LEN(m, 2);
    
    return fetched;
 }
    
    
    
    
    /*  
      return the offset given by mod=00 addressing.
      Also enables the decoding of instructions.
      */
    
    u_int16 
      DEFUN(decode_rm00_address,(m,rm),
	    PC_ENV *m AND int rm)
 {
    u_int16 offset;
    
    /* note the code which specifies the corresponding segment (ds vs ss)
       below in the case of [BP+..].  The assumption here is that at the 
       point that this subroutine is called, the bit corresponding to
       SYSMODE_SEG_DS_SS will be zero.  After every instruction 
       except the segment override instructions, this bit (as well
       as any bits indicating segment overrides) will be clear.  So
       if a SS access is needed, set this bit.  Otherwise, DS access 
       occurs (unless any of the segment override bits are set).
       */
    switch(rm)
	{
	 case 0:	
	   DECODE_PRINTF(m,"[BX+SI]");
	   return (int16)m->R_BX + (int16)m->R_SI;
	   break;
	 case 1:
	   DECODE_PRINTF(m,"[BX+DI]");
	   return (int16)m->R_BX + (int16)m->R_DI;
	   break;
	 case 2:
	   DECODE_PRINTF(m,"[BP+SI]");
	   m->sysmode |= SYSMODE_SEG_DS_SS;
	   return (int16)m->R_BP + (int16)m->R_SI;
	   break;
	 case 3:
	   DECODE_PRINTF(m,"[BP+DI]");
	   m->sysmode |= SYSMODE_SEG_DS_SS;
	   return (int16)m->R_BP + (int16)m->R_DI;
	   break;
	 case 4:
	   DECODE_PRINTF(m,"[SI]");
	   return m->R_SI;
	   break;
	 case 5:
	   DECODE_PRINTF(m,"[DI]");
	   return m->R_DI;
	   break;
	 case 6:
	   offset = (int16)fetch_word_imm(m);
	   DECODE_PRINTF2(m,"[%04x]",offset);
	   return offset;
	   break;
	 case 7:
	   DECODE_PRINTF(m,"[BX]");
	   return m->R_BX;
	}
    halt_sys(m);
    return 0;
 }
    
    
    
    /*  
      return the offset given by mod=01 addressing.
      Also enables the decoding of instructions.
      */
    
    u_int16 
      DEFUN(decode_rm01_address,
	    (m,rm),PC_ENV *m AND int rm)
 {
    int8 displacement;
    
    /* note comment on decode_rm00_address above */
    
    displacement = (int8)fetch_byte_imm(m);	/* !!!! Check this */
    
    switch(rm)
	{
	 case 0:	
	   DECODE_PRINTF2(m,"%d[BX+SI]",(int)displacement);
	   return (int16)m->R_BX + (int16)m->R_SI + displacement;
	   break;
	 case 1:
	   DECODE_PRINTF2(m,"%d[BX+DI]",(int)displacement);
	   return (int16)m->R_BX + (int16)m->R_DI + displacement;
	   break;
	 case 2:
	   DECODE_PRINTF2(m,"%d[BP+SI]",(int)displacement);
	   m->sysmode |= SYSMODE_SEG_DS_SS;
	   return (int16)m->R_BP + (int16)m->R_SI + displacement;
	   break;
	 case 3:
	   DECODE_PRINTF2(m,"%d[BP+DI]",(int)displacement);
	   m->sysmode |= SYSMODE_SEG_DS_SS;
	   return (int16)m->R_BP + (int16)m->R_DI + displacement;
	   break;
	 case 4:
	   DECODE_PRINTF2(m,"%d[SI]",(int)displacement);
	   return (int16)m->R_SI + displacement;
	   break;
	 case 5:
	   DECODE_PRINTF2(m,"%d[DI]",(int)displacement);
	   return (int16)m->R_DI + displacement;
	   break;
	 case 6:
	   DECODE_PRINTF2(m,"%d[BP]",(int)displacement);
	   m->sysmode |= SYSMODE_SEG_DS_SS;
	   return (int16)m->R_BP + displacement;
	   break;
	 case 7:
	   DECODE_PRINTF2(m,"%d[BX]",(int)displacement);
	   return (int16)m->R_BX + displacement;
	   break;
	}
    halt_sys(m);
    return 0;     /* SHOULD NOT HAPPEN */
 }
    
    
    /*  
      return the offset given by mod=01 addressing.
      Also enables the decoding of instructions.
      */
    
    u_int16 
      DEFUN(decode_rm10_address,
	    (m,rm),PC_ENV *m AND int rm)
 {
    int16 displacement;
    
    /* note comment on decode_rm00_address above */
    
    displacement = (int16)fetch_word_imm(m);
    
    switch(rm)
	{
	 case 0:	
	   DECODE_PRINTF2(m,"%d[BX+SI]",(int)displacement);
	   return (int16)m->R_BX + (int16)m->R_SI + displacement;
	   break;
	 case 1:
	   DECODE_PRINTF2(m,"%d[BX+DI]",(int)displacement);
	   return (int16)m->R_BX + (int16)m->R_DI + displacement;
	   break;
	 case 2:
	   DECODE_PRINTF2(m,"%d[BP+SI]",(int)displacement);	
	   m->sysmode |= SYSMODE_SEG_DS_SS;
	   return (int16)m->R_BP + (int16)m->R_SI + displacement;
	   break;
	 case 3:
	   DECODE_PRINTF2(m,"%d[BP+DI]",(int)displacement);
	   m->sysmode |= SYSMODE_SEG_DS_SS;
	   return (int16)m->R_BP + (int16)m->R_DI + displacement;
	   break;
	 case 4:
	   DECODE_PRINTF2(m,"%d[SI]",(int)displacement);
	   return (int16)m->R_SI + displacement;
	   break;
	 case 5:
	   DECODE_PRINTF2(m,"%d[DI]",(int)displacement);
	   return (int16)m->R_DI + displacement;
	   break;
	 case 6:
	   DECODE_PRINTF2(m,"%d[BP]",(int)displacement);
	   m->sysmode |= SYSMODE_SEG_DS_SS;
	   return (int16)m->R_BP + displacement;
	   break;
	 case 7:
	   DECODE_PRINTF2(m,"%d[BX]",(int)displacement);
	   return (int16)m->R_BX + displacement;
	   break;
	}
    halt_sys(m);
    return 0;
    /*NOTREACHED */
 }
    
    
    
    /* fetch a byte of data, given an offset, the current register set,
       and a descriptor for memory.
       */
    
    u_int8 
      DEFUN(fetch_data_byte,
	    (m,offset),
	    PC_ENV *m AND u_int16 offset)
 {
    register u_int8 value;
    
    /* this code originally completely broken, and never showed 
       up since the DS segments === SS segment in all test cases.
       It had been originally assumed, that all access to data would
       involve the DS register unless there was a segment override.
       Not so.   Address modes such as -3[BP] or 10[BP+SI] all 
       refer to addresses relative to the SS.  So, at the minimum, 
       all decodings of addressing modes would have to set/clear 
       a bit describing whether the access is relative to DS or SS.
       That is the function of the cpu-state-varible  m->sysmode.
       There are several potential states:
       
       repe prefix seen  (handled elsewhere)
       repne prefix seen  (ditto)
       
       cs segment override
       ds segment override
       es segment override
       ss segment override
       
       ds/ss select (in absense of override)
       
       Each of the above 7 items are handled with a bit in the sysmode
       field.
       
       The latter 5 can be implemented as a simple state machine:
       
       */
    
    
    switch(m->sysmode & SYSMODE_SEGMASK)
	{
	 case 0:
	   
	   /* default case: use ds register */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_DS, offset);
#endif
	   
#ifdef QUICK_FETCH	   
	   value = m->R_DSP[offset];
#else
	   value = m->fetch(m->R_DS, offset);
#endif
	   break;
	   
	   
	   
	 case SYSMODE_SEG_DS_SS:  
	   /* non-overridden, use ss register */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_SS, offset);
#endif
	   
#ifdef QUICK_FETCH	   
	   value = m->R_SSP[offset];
#else
	   /* [JCE] Switch to virtual functions for store/fetch */
	   value  = m->fetch(m->R_SS, offset);
#endif
	   break;
	   
	   
	 case SYSMODE_SEGOVR_CS:
	   /* ds overridden */
	 case SYSMODE_SEGOVR_CS|SYSMODE_SEG_DS_SS:
	   /* ss overridden, use cs register */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_CS, offset);
#endif
#ifdef QUICK_FETCH	   
	   value = m->R_CSP[offset];
#else
	   /* [JCE] Switch to virtual functions for store/fetch */
	   value  = m->fetch(m->R_CS, offset);
#endif
	   break;
	   
	 case SYSMODE_SEGOVR_DS:
	   /* ds overridden --- shouldn't happen, but hey. */
	 case SYSMODE_SEGOVR_DS|SYSMODE_SEG_DS_SS:
	   /* ss overridden, use ds register */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_DS, offset);
#endif
#ifdef QUICK_FETCH	   
	   value = m->R_DSP[offset];
#else
	   /* [JCE] Switch to virtual functions for store/fetch */
	   value  = m->fetch(m->R_DS, offset);
#endif
	   break;
	   
	   
	   
	 case SYSMODE_SEGOVR_ES:
	   /* ds overridden */
	 case SYSMODE_SEGOVR_ES|SYSMODE_SEG_DS_SS:
	   /* ss overridden, use es register */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_ES, offset);
#endif

#ifdef QUICK_FETCH	   
	   value = m->R_ESP[offset];
#else
	   /* [JCE] Switch to virtual functions for store/fetch */
	   value  = m->fetch(m->R_ES, offset);
#endif
	   break;
	   
	   
	 case SYSMODE_SEGOVR_SS:
	   /* ds overridden */
	 case SYSMODE_SEGOVR_SS|SYSMODE_SEG_DS_SS:
	   /* ss overridden, use ss register === should not happen */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_SS, offset);
#endif
#ifdef QUICK_FETCH	   
	   value = m->R_SSP[offset];
#else
	   /* [JCE] Switch to virtual functions for store/fetch */
	   value  =  m->fetch(m->R_SS, offset);
#endif
	   break;
	   
	 default:
	   debug_printf(m, 
			"error: should not happen:  multiple overrides. \n");
	   value = 0;
	   halt_sys(m);
	}
    
    return value;
 }
    
    
    /* fetch a byte of data, given an offset, the current register set,
       and a descriptor for memory.
       */
    
    u_int8 
      DEFUN(fetch_data_byte_abs,
	    (m,segment,offset),
	    PC_ENV *m AND u_int16 segment AND u_int16 offset)
 {
    register u_int8 value;
    u_int32 addr; 
#ifdef DEBUG
    if (CHECK_DATA_ACCESS(m))
      check_data_access(m, segment, offset);
#endif
    
    /* note, cannot change this, since we do not know the ID of the segment. */
    
    value = m->fetch(segment, offset);
    return value;
 }
    
    
    /* fetch a byte of data, given an offset, the current register set,
       and a descriptor for memory.
       */
    
    u_int16 
      DEFUN(fetch_data_word,
	    (m,offset),
	    PC_ENV *m AND u_int16 offset)
 {
    
    u_int16 value;
#ifdef QUICK_FETCH
    u_int8  *p = (u_int8*) &value;
#endif
    /* See note above in fetch_data_byte. */
    
    switch(m->sysmode & SYSMODE_SEGMASK)
	{
	 case 0:
	   
	   /* default case: use ds register */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_DS, offset);
#endif
#ifdef QUICK_FETCH 
#ifdef WORDS_BIGENDIAN
	   *p = m->R_DSP[offset + 1];
	   p++;
	   *p = m->R_DSP[offset];
#else
	   *p = m->R_DSP[offset];
	   p++;
	   *p = m->R_DSP[offset+1];
#endif	   
#else
	   /* [JCE] Switch to virtual functions for store/fetch */
	   value = m->fetch16(m->R_DS, offset);
#endif
	   
	   break;
	   
	 case SYSMODE_SEG_DS_SS:  
	   /* non-overridden, use ss register */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_SS, offset);
#endif
#ifdef QUICK_FETCH 
#ifdef WORDS_BIGENDIAN
	   *p = m->R_SSP[offset + 1];
	   p++;
	   *p = m->R_SSP[offset];
#else
	   *p = m->R_SSP[offset];
	   p++;
	   *p = m->R_SSP[offset+1];
#endif	   
#else
	   /* [JCE] Switch to virtual functions for store/fetch */
	   value = m->fetch16(m->R_SS, offset);
#endif
	   break;
	   
	   
	 case SYSMODE_SEGOVR_CS:
	   /* ds overridden */
	 case SYSMODE_SEGOVR_CS|SYSMODE_SEG_DS_SS:
	   /* ss overridden, use cs register */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_CS, offset);
#endif
	   
#ifdef QUICK_FETCH 
#ifdef WORDS_BIGENDIAN
	   *p = m->R_CSP[offset + 1];
	   p++;
	   *p = m->R_CSP[offset];
#else
	   *p = m->R_CSP[offset];
	   p++;
	   *p = m->R_CSP[offset+1];
#endif	   
#else
	   /* [JCE] Switch to virtual functions for store/fetch */
	   value = m->fetch16(m->R_CS, offset);
#endif
	   break;
	   
	 case SYSMODE_SEGOVR_DS:
	   /* ds overridden --- shouldn't happen, but hey. */
	 case SYSMODE_SEGOVR_DS|SYSMODE_SEG_DS_SS:
	   /* ss overridden, use ds register */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_DS, offset);
#endif
	   
#ifdef QUICK_FETCH 
#ifdef WORDS_BIGENDIAN
	   *p = m->R_DSP[offset + 1];
	   p++;
	   *p = m->R_DSP[offset];
#else
	   *p = m->R_DSP[offset];
	   p++;
	   *p = m->R_DSP[offset+1];
#endif	   
#else
	   /* [JCE] Switch to virtual functions for store/fetch */
	   value = m->fetch16(m->R_DS, offset);
#endif
	   
	   break;
	   
	   
	   
	 case SYSMODE_SEGOVR_ES:
	   /* ds overridden */
	 case SYSMODE_SEGOVR_ES|SYSMODE_SEG_DS_SS:
	   /* ss overridden, use es register */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_ES, offset);
#endif
#ifdef QUICK_FETCH 
#ifdef WORDS_BIGENDIAN
	   *p = m->R_ESP[offset + 1];
	   p++;
	   *p = m->R_ESP[offset];
#else
	   *p = m->R_ESP[offset];
	   p++;
	   *p = m->R_ESP[offset+1];
#endif	   
#else
	   value = m->fetch16(m->R_ES, offset);
#endif
	   break;
	   
	   
	 case SYSMODE_SEGOVR_SS:
	   /* ds overridden */
	 case SYSMODE_SEGOVR_SS|SYSMODE_SEG_DS_SS:
	   /* ss overridden, use ss register === should not happen */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_SS, offset);
#endif
#ifdef QUICK_FETCH 
#ifdef WORDS_BIGENDIAN
	   *p = m->R_SSP[offset + 1];
	   p++;
	   *p = m->R_SSP[offset];
#else
	   *p = m->R_SSP[offset];
	   p++;
	   *p = m->R_SSP[offset+1];
#endif	   
#else
	   value = m->fetch16(m->R_SS, offset);
#endif
	   break;
	   
	 default:
	   debug_printf(m, "error: should not happen:  multiple overrides. \n");
	   value = 0;
	   halt_sys(m);
	}
    
    return value;
 }
    
    
    /* fetch a byte of data, given an offset, the current register set,
       and a descriptor for memory.
       */
    
u_int16 
      DEFUN(fetch_data_word_abs,
	    (m,segment,offset),
	    PC_ENV *m AND u_int16 segment AND u_int16 offset)
 {
    u_int16 value;
    u_int32 addr;    
#ifdef DEBUG
    if (CHECK_DATA_ACCESS(m))
      check_data_access(m, segment, offset);
#endif
    /* [JCE] Switch to virtual functions for store/fetch */
    value = m->fetch16(segment, offset);
    return value;
 }
    
    
    
    
    
    /* Store a byte of data, given an offset, the current register set,
       and a descriptor for memory.
       */
    
    void 
      DEFUN(store_data_byte,
	    (m,offset,val),
	    PC_ENV *m AND u_int16 offset AND u_int8 val)
 {
    /* See note above in fetch_data_byte. */
    
    u_int32             addr;
    register u_int16	segment;
#ifdef QUICK_FETCH
    register u_int8     *segaddr;
#endif
    
    switch(m->sysmode & SYSMODE_SEGMASK)
	{
	 case 0:
	   
	   /* default case: use ds register */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_DS, offset);
#endif
	   
#ifdef QUICK_FETCH
	   segaddr = m->R_DSP;
#else	   
	   segment = m->R_DS;
#endif
	   break;
	   
	   
	 case SYSMODE_SEG_DS_SS:  
	   /* non-overridden, use ss register */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_SS, offset);
#endif
#ifdef QUICK_FETCH
	   segaddr = m->R_SSP;
#else	   
	   segment = m->R_SS;
#endif
	   break;
	   
	   
	 case SYSMODE_SEGOVR_CS:
	   /* ds overridden */
	 case SYSMODE_SEGOVR_CS|SYSMODE_SEG_DS_SS:
	   /* ss overridden, use cs register */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_CS, offset);
#endif
	   
#ifdef QUICK_FETCH
	   segaddr = m->R_CSP;
#else	   
	   segment = m->R_CS;
#endif
	   
	   break;
	   
	 case SYSMODE_SEGOVR_DS:
	   /* ds overridden --- shouldn't happen, but hey. */
	 case SYSMODE_SEGOVR_DS|SYSMODE_SEG_DS_SS:
	   /* ss overridden, use ds register */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_DS, offset);
#endif
#ifdef QUICK_FETCH
	   segaddr = m->R_DSP;
#else	   
	   segment = m->R_DS;
#endif
	   break;
	   
	   
	   
	 case SYSMODE_SEGOVR_ES:
	   /* ds overridden */
	 case SYSMODE_SEGOVR_ES|SYSMODE_SEG_DS_SS:
	   /* ss overridden, use es register */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_ES, offset);
#endif
#ifdef QUICK_FETCH
	   segaddr = m->R_ESP;
#else	   
	   segment = m->R_ES;
#endif
	   break;
	   
	 case SYSMODE_SEGOVR_SS:
	   /* ds overridden */
	 case SYSMODE_SEGOVR_SS|SYSMODE_SEG_DS_SS:
	   /* ss overridden, use ss register === should not happen */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_SS, offset);
#endif
#ifdef QUICK_FETCH
	   segaddr = m->R_SSP;
#else	   
	   segment = m->R_SS;
#endif
	   break;
	   
	 default:
	   debug_printf(m, "error: should not happen:  multiple overrides. \n");
	   segment = 0;
	   halt_sys(m);
	}
    
#ifdef QUICK_FETCH
    segaddr[offset] = val;
#else	   
    /* [JCE] Switch to virtual functions for store/fetch */
    m->store(segment, offset, val);
#endif

 }
    
    
    void 
      DEFUN(store_data_byte_abs,
	    (m,segment,offset,val),
	    PC_ENV *m AND u_int16 segment AND u_int16 offset AND u_int8 val)
 {
    register u_int32	addr;
#ifdef DEBUG
    if (CHECK_DATA_ACCESS(m))
      check_data_access(m, segment, offset);
#endif
    /* [JCE] Switch to virtual functions for store/fetch */
    m->store(segment, offset, val);
 }
    
    
    /* Store  a word of data, given an offset, the current register set,
       and a descriptor for memory.
       */
    
    void 
      DEFUN(store_data_word,
	    (m,offset,val),
	    PC_ENV *m AND u_int16 offset AND u_int16 val)
 {
    register u_int32	addr;
    register u_int16	segment;
    /* See note above in fetch_data_byte. */
    
    switch(m->sysmode & SYSMODE_SEGMASK)
	{
	 case 0:
	   /* default case: use ds register */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_DS, offset);
#endif
	   segment = m->R_DS;
	   break;
	   
	 case SYSMODE_SEG_DS_SS:  
	   /* non-overridden, use ss register */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_SS, offset);
#endif
	   segment = m->R_SS;
	   break;
	   
	 case SYSMODE_SEGOVR_CS:
	   /* ds overridden */
	 case SYSMODE_SEGOVR_CS|SYSMODE_SEG_DS_SS:
	   /* ss overridden, use cs register */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_CS, offset);
#endif
	   segment = m->R_CS;
	   break;
	   
	 case SYSMODE_SEGOVR_DS:
	   /* ds overridden --- shouldn't happen, but hey. */
	 case SYSMODE_SEGOVR_DS|SYSMODE_SEG_DS_SS:
	   /* ss overridden, use ds register */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_DS, offset);
#endif
	   segment = m->R_DS;
	   break;
	   
	 case SYSMODE_SEGOVR_ES:
	   /* ds overridden */
	 case SYSMODE_SEGOVR_ES|SYSMODE_SEG_DS_SS:
	   /* ss overridden, use es register */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_ES, offset);
#endif
	   segment = m->R_ES;
	   break;
	   
	 case SYSMODE_SEGOVR_SS:
	   /* ds overridden */
	 case SYSMODE_SEGOVR_SS|SYSMODE_SEG_DS_SS:
	   /* ss overridden, use ss register === should not happen */
#ifdef DEBUG
	   if (CHECK_DATA_ACCESS(m))
	     check_data_access(m, m->R_SS, offset);
#endif
	   segment = m->R_SS;
	   break;
	   
	 default:
	   debug_printf(m, "error: should not happen:  multiple overrides. \n");
	   segment = 0;
	   halt_sys(m);
	}
    /* [JCE] Switch to virtual functions for store/fetch */
/*    addr = (segment << 4) + offset; */
    m->store16(segment, offset, val);
 }
    
    
    void 
      DEFUN(store_data_word_abs,
	    (m,segment,offset,val),
	    PC_ENV *m AND u_int16 segment AND u_int16 offset AND u_int16 val)
 {
    register u_int32	addr;
#ifdef DEBUG
    if (CHECK_DATA_ACCESS(m))
      check_data_access(m, segment, offset);
#endif
    /* [JCE] Wrap at top of memory */
    m->store16(segment, offset, val);
    
 }
    
    
    /*	End of File */
