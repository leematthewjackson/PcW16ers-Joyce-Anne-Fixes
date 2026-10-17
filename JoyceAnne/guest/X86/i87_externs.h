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

static char i87_externs_rcsid[]=
  "$Id:  $";

/* $Log: $
 *
 */

/* these have to be defined, whether 8087 support compiled in or not. */
void EXFUN(i86op_esc_coprocess_d8,(PC_ENV *m));
void EXFUN(i86op_esc_coprocess_d9,(PC_ENV *m));
void EXFUN(i86op_esc_coprocess_da,(PC_ENV *m));
void EXFUN(i86op_esc_coprocess_db,(PC_ENV *m));
void EXFUN(i86op_esc_coprocess_dc,(PC_ENV *m));
void EXFUN(i86op_esc_coprocess_dd,(PC_ENV *m));
void EXFUN(i86op_esc_coprocess_de,(PC_ENV *m));
void EXFUN(i86op_esc_coprocess_df,(PC_ENV *m));



