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

#ifndef I86_H
#define I86_H


static char i86_h_rcsid[]=
  "$Id: $";


/* $Log: $
 *
 */


/* definition of the registers */


/* general EAX,EBX,ECX, EDX type registers.  
   Note that for portability, and speed, the issue of byte
   swapping is not addressed in the registers.  All registers
   are stored in the default format available on the
   host machine.  The only critical issue is that the 
   registers should line up EXACTLY in the same manner as 
   they do in the 386.  That is:

       EAX & 0xff  === AL
       EAX & 0xffff == AX

   etc.  The result is that alot of the calculations can then be
   done using the native instruction set fully.
*/

#ifdef WORDS_BIGENDIAN
typedef struct  { u_int32 e_reg; }            I32_reg_t;
typedef struct  { u_int16 d0, x_reg; }        I16_reg_t;
typedef struct  { u_int8 d0,d1,h_reg,l_reg; } I8_reg_t;
#else
typedef struct  { u_int32 e_reg; }            I32_reg_t;
typedef struct  { u_int16 x_reg; }            I16_reg_t;
typedef struct  { u_int8 l_reg,h_reg; }       I8_reg_t; 
               /* note reversal! */
#endif

/* Devices which interrupt the CPU from outside should implement this
 * interface. The CPU will call it when it handles the interrupt; it should
 * return the interrupt number to use, 0-255. */
struct IntAck
{
	virtual u_int8 ackInterrupt(void) = 0;
};


typedef union 
 {
    I32_reg_t    I32_reg;
    I16_reg_t    I16_reg;
    I8_reg_t     I8_reg;
 } i386_general_register;
    


struct i386_general_regs   
 {
    i386_general_register  A,B,C,D;
 };


typedef struct i386_general_regs Gen_reg_t;



struct i386_special_regs
 {
    i386_general_register SP,BP,SI,DI,IP;
    u_int32		FLAGS;
#ifdef UNPACKED_FLAGS
    u_int8              FLAGARR[16];
#endif
#ifdef QUICK_DECODE_RM
    u_int8              *rm_byte[8];
    u_int16             *rm_word[8];
#endif    
 };



/*  
 *  segment registers here represent the 16 bit quantities
 *  CS, DS, ES, SS
 *
 *  segment pointers --- used to speed up  the expressions:
 *	q = m->R_CSP + m->R_IP;
 *	fetched = *q; 
 *	m->R_IP += 1;
 *  compared to:
 * 	fetched = m->mem_base[((u_int32)m->R_CS << 4) + (m->R_IP++)];
 *  Save at least one shift, more if doing two byte moves.
 */

struct i386_segment_regs
 {
    u_int16   CS,DS,SS,ES,FS,GS;

#ifdef QUICK_FETCH
    u_int8   *CSP,*DSP,*SSP,*ESP,*FSP,*GSP;
#endif
 };


/* 8 bit registers */
#define R_AH  Gn_regs.A.I8_reg.h_reg
#define R_AL  Gn_regs.A.I8_reg.l_reg
#define R_BH  Gn_regs.B.I8_reg.h_reg
#define R_BL  Gn_regs.B.I8_reg.l_reg
#define R_CH  Gn_regs.C.I8_reg.h_reg
#define R_CL  Gn_regs.C.I8_reg.l_reg
#define R_DH  Gn_regs.D.I8_reg.h_reg
#define R_DL  Gn_regs.D.I8_reg.l_reg

/* 16 bit registers */
#define R_AX  Gn_regs.A.I16_reg.x_reg
#define R_BX  Gn_regs.B.I16_reg.x_reg
#define R_CX  Gn_regs.C.I16_reg.x_reg
#define R_DX  Gn_regs.D.I16_reg.x_reg


/* 32 bit extended registers */
#define R_EAX  Gn_regs.A.I32_reg.e_reg
#define R_EBX  Gn_regs.B.I32_reg.e_reg
#define R_ECX  Gn_regs.C.I32_reg.e_reg
#define R_EDX  Gn_regs.D.I32_reg.e_reg


/* special registers */
#define R_SP  Sp_regs.SP.I16_reg.x_reg
#define R_BP  Sp_regs.BP.I16_reg.x_reg
#define R_SI  Sp_regs.SI.I16_reg.x_reg
#define R_DI  Sp_regs.DI.I16_reg.x_reg
#define R_IP  Sp_regs.IP.I16_reg.x_reg
#define R_FLG Sp_regs.FLAGS

/* DEFINES FOR SPECIAL PURPOSE SPEEDUPS */
#define AR_FLG Sp_regs.FLAGARR
#define RM_BYTE Sp_regs.rm_byte
#define RM_WORD Sp_regs.rm_word
#define R_DFW Sp_regs.delay_flag_word

/* segment registers */
#define R_CS  Sg_regs.CS
#define R_DS  Sg_regs.DS
#define R_SS  Sg_regs.SS
#define R_ES  Sg_regs.ES
#define R_FS  Sg_regs.FS
#define R_GS  Sg_regs.GS

/* SEGMENT SHADOW REGISTERS */
#define R_CSP  Sg_regs.CSP
#define R_DSP  Sg_regs.DSP
#define R_SSP  Sg_regs.SSP
#define R_ESP  Sg_regs.ESP
#define R_FSP  Sg_regs.FSP
#define R_GSP  Sg_regs.GSP

/* flag conditions   */
#define FB_CF   0x1  /* CARRY flag  */
#define FB_PF   0x4  /* PARITY flag */
#define FB_AF  0x10  /* AUX  flag   */
#define FB_ZF  0x40  /* ZERO flag   */
#define FB_SF  0x80  /* SIGN flag   */
#define FB_TF 0x100  /* TRAP flag   */
#define FB_IF 0x200  /* INTERRUPT ENABLE flag */
#define FB_DF 0x400  /* DIR flag    */
#define FB_OF 0x800  /* OVERFLOW flag */

/* 8088 has top 4 bits of the flags set to 1 */
/* Also, bit#1 is set.  This is (not well) documented  behavior. */
/* see note in userman.tex about the subtleties of dealing with  */
/* code which attempts to detect the host processor.             */
/* This is def'd as F_ALWAYS_ON */

#define F_ALWAYS_ON  (0xf002)        /* flag bits always on */

/*
 *   DEFINE A MASK FOR ONLY THOSE FLAG BITS WE WILL EVER PASS BACK 
 *   (via PUSHF) 
 */

#define F_MSK (FB_CF|FB_PF|FB_AF|FB_ZF|FB_SF|FB_TF|FB_IF|FB_DF|FB_OF)



#ifndef UNPACKED_FLAGS

/* following bits masked in to a 16bit quantity */

#define F_CF   0x1  /* CARRY flag  */
#define F_PF   0x4  /* PARITY flag */
#define F_AF  0x10  /* AUX  flag   */
#define F_ZF  0x40  /* ZERO flag   */
#define F_SF  0x80  /* SIGN flag   */
#define F_TF 0x100  /* TRAP flag   */
#define F_IF 0x200  /* INTERRUPT ENABLE flag */
#define F_DF 0x400  /* DIR flag    */
#define F_OF 0x800  /* OVERFLOW flag */


#define TOGGLE_FLAG(M,FLAG) (M)->R_FLG ^= FLAG
#define SET_FLAG(M,FLAG)  (M)->R_FLG |= FLAG
#define CLEAR_FLAG(M, FLAG)  (M)->R_FLG &= ~FLAG
#define ACCESS_FLAG(M,FLAG) ((M)->R_FLG & (FLAG))
#define CLEARALL_FLAG(M)    (M)->R_FLG = 0
#define PACK_FLAGS(M)
#define UNPACK_FLAGS(M)

#else   /* UNPACKED_FLAG */

/* value represents the bit position of the flag */
#define F_CF     0    /* CARRY flag */
#define F_PF     2    /* PARITY flag   */
#define F_AF     4    /* AUX  flag      */
#define F_ZF     6    /* ZERO flag  */
#define F_SF     7    /* SIGN flag */
#define F_TF     8    /* TRAP flag */
#define F_IF     9    /* INTERRUPT ENABLE flag */
#define F_DF    10    /* DIR flag  */
#define F_OF    11    /* OVERFLOW flag */

#define TOGGLE_FLAG(M,FLAG) (M)->AR_FLG[FLAG] ^= 1
#define SET_FLAG(M,FLAG)  (M)->AR_FLG[FLAG] = 1
#define CLEAR_FLAG(M, FLAG)  (M)->AR_FLG[FLAG] = 0
#define ACCESS_FLAG(M,FLAG) ((M)->AR_FLG[FLAG])
#define CLEARALL_FLAG(M) \
    (M)->AR_FLG[0] = (M)->AR_FLG[1] = (M)->AR_FLG[2] = (M)->AR_FLG[3] = \
    (M)->AR_FLG[4] = (M)->AR_FLG[5] = (M)->AR_FLG[6] = (M)->AR_FLG[7] = \
    (M)->AR_FLG[8] = (M)->AR_FLG[9] = (M)->AR_FLG[10] = (M)->AR_FLG[11] = \
    (M)->AR_FLG[12] = (M)->AR_FLG[13] = (M)->AR_FLG[14] =  (M)->AR_FLG[15] =  0
#define PACK_FLAGS(m)  pack_flags(m)
#define UNPACK_FLAGS(m)  unpack_flags(m)
#endif /* UNPACKED_FLAGS */


#define CONDITIONAL_SET_FLAG(COND,M,FLAG) \
  if (COND) SET_FLAG(M,FLAG); else CLEAR_FLAG(M,FLAG)


#define F_PF_CALC 0x010000		/* PARITY flag has been calced	*/
#define F_ZF_CALC 0x020000		/* ZERO flag has been calced	*/
#define F_SF_CALC 0x040000		/* SIGN flag has been calced	*/

#define F_ALL_CALC	0xff0000	/* All have been calced	*/



/* emulator machine state. */
/* segment usage control */
#define SYSMODE_SEG_DS_SS       0x01
#define SYSMODE_SEGOVR_CS       0x02
#define SYSMODE_SEGOVR_DS       0x04
#define SYSMODE_SEGOVR_ES       0x08
#define SYSMODE_SEGOVR_SS       0x10

#define SYSMODE_SEGMASK  (  SYSMODE_SEG_DS_SS |   \
			    SYSMODE_SEGOVR_CS |   \
			    SYSMODE_SEGOVR_DS |   \
			    SYSMODE_SEGOVR_ES |   \
			    SYSMODE_SEGOVR_SS )

#define SYSMODE_PREFIX_REPE     0x20
#define SYSMODE_PREFIX_REPNE    0x40

#define SYSMODE_DF_ZF          0x80     /* delayed zero flag set */
#define SYSMODE_DF_SF          0x100    /* delayed sign flag set */
#define SYSMODE_DF_PF          0x200    /* delayed parity flag set */

/* unused ... here for documentation purposes */
#define SYSMODE_RSVD_MASK     (0x400|0x800|0x1000|0x2000|0x4000)

/* if one of these masks is set, an memory access using this register
   lies past the 512K point, and should be checked for video 
   memory access.
   Set when the segment registers are modified
 */

#define SYSMODE_VSEGMASK      0x0f000000
#define SYSMODE_VSEG_CS       0x01000000
#define SYSMODE_VSEG_DS       0x02000000
#define SYSMODE_VSEG_ES       0x04000000
#define SYSMODE_VSEG_SS       0x07000000

#define SYSMODE_INTR_PENDING  0x10000000
#define SYSMODE_EXTRN_INTR    0x20000000
#define SYSMODE_HALTED        0x40000000


#define  INTR_SYNCH           0x1
#define  INTR_ASYNCH          0x2
#define  INTR_HALTED          0x4


/* INSTRUCTION DECODING STUFF */

/*
 *	Function Prototypes
 */

#if defined(INLINE_DECODE_MODRM) && defined(QUICK_FETCH)
# define FETCH_DECODE_MODRM(m,mod,rh,rl) \
  {\
     u_int8 fetched;\
     register struct modrm *p;\
     fetched = m->R_CSP[m->R_IP];\
     m->R_IP += 1;\
     p = modrmtab + fetched;\
     mod = p->mod;\
     rh= p->rh;\
     rl= p->rl;\
  }
    
#else

# define FETCH_DECODE_MODRM(m,mod,rh,rl) \
  fetch_decode_modrm(m,&mod,&rh,&rl)

#endif

#ifdef QUICK_DECODE_RM
#define DECODE_RM_BYTE_REGISTER(m,r)\
  ((m)->RM_BYTE[r])
#define DECODE_RM_WORD_REGISTER(m,r)\
  ((m)->RM_WORD[r])
#define QUICK_DECODE_RM_INIT(m)  quick_decode_rm_init(m)
#else /* ! QUICK_DECODE_RM */
#define DECODE_RM_BYTE_REGISTER(m,r) decode_rm_byte_register(m,r)
#define DECODE_RM_WORD_REGISTER(m,r) decode_rm_word_register(m,r)
#define QUICK_DECODE_RM_INIT(m)
#endif /* QUICK_DECODE_RM */

#define     DECODE_CLEAR_SEGOVR(m)    m->sysmode &= ~( SYSMODE_SEGMASK )

#ifdef QUICK_FETCH
#define RESYNCH_SEG_CS(m) \
  m->R_CSP = m->mem_base + (m->R_CS << 4)
#define RESYNCH_SEG_DS(m) \
  m->R_DSP = m->mem_base + (m->R_DS << 4)
#define RESYNCH_SEG_ES(m) \
  m->R_ESP = m->mem_base + (m->R_ES << 4)
#define RESYNCH_SEG_SS(m) \
  m->R_SSP = m->mem_base + (m->R_SS << 4)
#else
#define RESYNCH_SEG_CS(m) 
#define RESYNCH_SEG_DS(m) 
#define RESYNCH_SEG_ES(m) 
#define RESYNCH_SEG_SS(m) 
#endif


/* some notes here on the handling of delayed flag setting...
   Any operation which uses the values of these flags (such as the 
   "jmp near on condition" instructions) must resynch the values of 
   the actual flags and clear the bits in sysmode indicating that the
   flag setting has been deferred.

   Additionally, any instruction which modifies the settings of
   these flags needs to kill any existing "delayed setting" flags.

 */
   
#ifdef DELAY_FLAG_SET
#define RESYNCH_FLAGS(m,mask) /* ??? */
#define CANCEL_DFLAGS(m,mask) \
  m->sysmode &= ~(mask);

#else
#define RESYNCH_FLAGS(m,mask)
#define CANCEL_DFLAGS(m,mask)
#endif


struct modrm{ u_int16 mod,rh,rl;  };



#endif /* I86_H */

