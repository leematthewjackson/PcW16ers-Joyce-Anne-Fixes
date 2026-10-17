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

#ifndef I86_EXTERNS_H
#define I86_EXTERNS_H

static char i86_externs_rcsid[]=
  "$Id: $";

/* $Log: $
 *
 */


extern struct modrm modrmtab[];

void EXFUN(pack_flags,(PC_ENV *m));
void EXFUN(i386_exec,(PC_ENV *sys));
void EXFUN(halt_sys,(PC_ENV *sys));
void EXFUN(unpack_flags,(PC_ENV *m));

void  EXFUN(fetch_decode_modrm,
	    (PC_ENV *m, u_int16 *mod, u_int16 *regh, u_int16 *regl));

u_int8 *
  EXFUN(decode_rm_byte_register,
	(PC_ENV *m, int reg));
u_int16 *
  EXFUN(decode_rm_word_register,
	(PC_ENV *m, int reg));

u_int16 *
  EXFUN(decode_rm_seg_register,
	(PC_ENV *m, int reg));

u_int8  
  EXFUN(fetch_byte_imm,
	(PC_ENV *m));

u_int16  
  EXFUN(fetch_word_imm,
	(PC_ENV *m));

u_int16 
  EXFUN(decode_rm00_address,
	(PC_ENV *m, int rm));

u_int16 
  EXFUN(decode_rm01_address,
	(PC_ENV *m, int rm));

u_int16 
  EXFUN(decode_rm10_address,
	(PC_ENV *m, int rm));

u_int8 
  EXFUN(fetch_data_byte,
	(PC_ENV *m, u_int16 offset));

u_int8 
  EXFUN(fetch_data_byte_abs,
	(PC_ENV *m, u_int16 segment, u_int16 offset));

u_int16 
  EXFUN(fetch_data_word,
	(PC_ENV *m, u_int16 offset));

u_int16 
  EXFUN(fetch_data_word_abs,
	(PC_ENV *m, u_int16 segment, u_int16 offset));

void 
  EXFUN(store_data_byte,
	(PC_ENV *m, u_int16 offset, u_int8 val));

void 
  EXFUN(store_data_byte_abs,
	(PC_ENV *m, u_int16 segment, u_int16 offset, u_int8 val));


void 
  EXFUN(store_data_word,
	(PC_ENV *m, u_int16 offset, u_int16 val));

void 
  EXFUN(store_data_word_abs,
	(PC_ENV *m, u_int16 segment, u_int16 offset, u_int16 val));



#ifdef __STDC__
typedef    void  (*OP)(PC_ENV *m);
#else
typedef    void  (*OP)();
#endif
extern OP i86_optab[256];


/* PRIMITIVE OPERATIONS */

u_int	EXFUN(dsf_sign,(PC_ENV *m));
u_int	EXFUN(dsf_zero,(PC_ENV *m));
u_int	EXFUN(dsf_parity,(PC_ENV *m));

u_int8	EXFUN(aad_word,(PC_ENV *m AND u_int16 d));
u_int16	EXFUN(aam_word,(PC_ENV *m AND u_int8 d));
u_int8	EXFUN(adc_byte,(PC_ENV *m AND u_int8 d AND u_int8 s));
u_int16	EXFUN(adc_word,(PC_ENV *m AND u_int16 d AND u_int16 s));
u_int8	EXFUN(add_byte,(PC_ENV *m AND u_int8 d AND u_int8 s));
u_int16	EXFUN(add_word,(PC_ENV *m AND u_int16 d AND u_int16 s));
u_int8	EXFUN(and_byte,(PC_ENV *m AND u_int8 d AND u_int8 s));
u_int16	EXFUN(and_word,(PC_ENV *m AND u_int16 d AND u_int16 s));
u_int8	EXFUN(cmp_byte,(PC_ENV *m AND u_int8 d AND u_int8 s));
u_int16	EXFUN(cmp_word,(PC_ENV *m AND u_int16 d AND u_int16 s));
u_int8	EXFUN(dec_byte,(PC_ENV *m AND u_int8 d));
u_int16	EXFUN(dec_word,(PC_ENV *m AND u_int16 d));
u_int8	EXFUN(inc_byte,(PC_ENV *m AND u_int8 d));
u_int16	EXFUN(inc_word,(PC_ENV *m AND u_int16 d));
u_int8	EXFUN(or_byte,(PC_ENV *m AND u_int8 d AND u_int8 s));
u_int16	EXFUN(or_word,(PC_ENV *m AND u_int16 d AND u_int16 s));
u_int8	EXFUN(neg_byte,(PC_ENV *m AND u_int8 s));
u_int16	EXFUN(neg_word,(PC_ENV *m AND u_int16 s));
u_int8	EXFUN(not_byte,(PC_ENV *m AND u_int8 s));
u_int16	EXFUN(not_word,(PC_ENV *m AND u_int16 s));
u_int16	EXFUN(mem_access_word,(PC_ENV *m AND int addr));
void	EXFUN(push_word,(PC_ENV *m AND u_int16 w));
u_int16	EXFUN(pop_word,(PC_ENV *m));
u_int8	EXFUN(rcl_byte,(PC_ENV *m AND u_int8 d AND u_int8 s));
u_int16	EXFUN(rcl_word,(PC_ENV *m AND u_int16 d AND u_int16 s));
u_int8	EXFUN(rcr_byte,(PC_ENV *m AND u_int8 d AND u_int8 s));
u_int16	EXFUN(rcr_word,(PC_ENV *m AND u_int16 d AND u_int16 s));
u_int8	EXFUN(rol_byte,(PC_ENV *m AND u_int8 d AND u_int8 s));
u_int16	EXFUN(rol_word,(PC_ENV *m AND u_int16 d AND u_int16 s));
u_int8	EXFUN(ror_byte,(PC_ENV *m AND u_int8 d AND u_int8 s));
u_int16	EXFUN(ror_word,(PC_ENV *m AND u_int16 d AND u_int16 s));
u_int8	EXFUN(shl_byte,(PC_ENV *m AND u_int8 d AND u_int8 s)) ;
u_int16	EXFUN(shl_word,(PC_ENV *m AND u_int16 d AND u_int16 s));
u_int8	EXFUN(shr_byte,(PC_ENV *m AND u_int8 d AND u_int8 s));
u_int16	EXFUN(shr_word,(PC_ENV *m AND u_int16 d AND u_int16 s));
u_int8	EXFUN(sar_byte,(PC_ENV *m AND u_int8 d AND u_int8 s));
u_int16	EXFUN(sar_word,(PC_ENV *m AND u_int16 d AND u_int16 s));
u_int8	EXFUN(sbb_byte,(PC_ENV *m AND u_int8 d AND u_int8 s));
u_int16	EXFUN(sbb_word,(PC_ENV *m AND u_int16 d AND u_int16 s));
u_int8	EXFUN(sub_byte,(PC_ENV *m AND u_int8 d AND u_int8 s));
u_int16	EXFUN(sub_word,(PC_ENV *m AND u_int16 d AND u_int16 s));
void	EXFUN(test_byte,(PC_ENV *m AND u_int8 d AND u_int8 s));
void    EXFUN(test_word,(PC_ENV *m AND u_int16 d AND u_int16 s));
u_int8	EXFUN(xor_byte,(PC_ENV *m AND u_int8 d AND u_int8 s));
u_int16	EXFUN(xor_word,(PC_ENV *m AND u_int16 d AND u_int16 s));
void	EXFUN(imul_byte,(PC_ENV *m AND u_int8 s));
void	EXFUN(imul_word,(PC_ENV *m AND u_int16 s));
void	EXFUN(mul_byte,(PC_ENV *m AND u_int8 s));
void	EXFUN(mul_word,(PC_ENV *m AND u_int16 s));
void	EXFUN(idiv_byte,(PC_ENV *m AND u_int8 s));
void	EXFUN(idiv_word,(PC_ENV *m AND u_int16 s));
void	EXFUN(div_byte,(PC_ENV *m AND u_int8 s));
void	EXFUN(div_word,(PC_ENV *m AND u_int16 s));

#endif /* I86_EXTERNS_H */

