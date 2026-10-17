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
  "$Id: $";

/* $Log: $
 *
 * Revision 0.03  1991/07/30  01:47:59  hudgens
 * added copyright.
 *
 * Revision 0.02  1991/07/20  18:35:25  hudgens
 * added code for dumping interrupt table.
 *
 * Revision 0.01  1991/07/20  04:04:58  hudgens
 * Initial revision
 *
 *
 */



#include "i86int.hxx"

static void EXFUN(print_encoded_bytes,(PC_ENV *m, u_int16 s, u_int16 o));
static void EXFUN(print_decoded_instruction,(PC_ENV *m));
static void EXFUN(print_ivectab,(PC_ENV *m));
static void EXFUN(disassemble_forward,
		  (PC_ENV *m, u_int16 seg, u_int16 off, int n));  

static int EXFUN(parse_line,(char *s, int *ps, int *n));
  
#ifdef DEBUG
/* should look something like debug's output. */
void DEFUN(trace_regs,(m),
	   PC_ENV  *m )
 {
    dump_regs(m);
    if (DEBUG_DECODE(m))
      {
	 debug_printf(m,"%04x:%04x ",m->saved_cs, m->saved_ip);
	 print_encoded_bytes(m, m->saved_cs, m->saved_ip);
	 print_decoded_instruction(m);
      }
}



void DEFUN(just_disassemble,(m),
	   PC_ENV *m)
 {
    /* this routine called if the flag DEBUG_DISASSEMBLE is set */
    /* kind of a hack! */

    debug_printf(m,"%04x:%04x ",m->saved_cs, m->saved_ip);
    print_encoded_bytes(m, m->saved_cs, m->saved_ip);
    print_decoded_instruction(m);
 }


static void DEFUN(disassemble_forward,(m,seg,off,n),
		  PC_ENV  *m AND
		  u_int16 seg AND
		  u_int16 off AND
		  int n)
 {
    PC_ENV *tregs;
    int i, op1;
    
    /* hack, hack, hack.  What we do is use the exact machinery 
       set up for execution, except that now there is an additional
       state flag associated with the "execution", and we are using
       a copy of the register struct.  All the major opcodes, once
       fully decoded, have the following two steps:
           TRACE_REGS(r,m);
	   SINGLE_STEP(r,m);
       which disappear if DEBUG is not defined to the preprocessor.
       The TRACE_REGS macro expands to:

       if (debug&DEBUG_DISASSEMBLE) 
           {just_disassemble(); goto EndOfInstruction;}
       if (debug&DEBUG_TRACE) trace_regs(r,m);

       ......  and at the last line of the routine. 

       EndOfInstruction: end_instr();
       
       Up to the point where TRACE_REG is expanded, NO modifications
       are done to any register EXCEPT the IP register, for 
       fetch and decoding purposes.  

       This was done for an entirely different reason, but makes 
       a nice way to get the system to help debug codes.
       */

    tregs = m->clone();
    tregs->R_IP = off;
    tregs->R_CS = seg;
    
    /* reset the decoding buffers */
    tregs->enc_str_pos = 0;
    tregs->enc_pos = 0;
    
    /* turn on the "disassemble only, no execute" flag */
    tregs->m_settings.debug |= DEBUG_DISASSEMBLE_F;
 
    /* DUMP NEXT n instructions to screen in straight_line fashion */
    /* this looks like the regular instruction fetch stream,
       except that when this occurs, each fetched opcode, 
       upon seeing the DEBUG_DISASSEMBLE flag set, 
       exits immediately after decoding the instruction.  
       XXX --- CHECK THAT MEM IS NOT AFFECTED!!! 
       Note the use of a copy of the register structure...
     */

    for (i=0; i<n; i++)
	{
#ifdef QUICK_FETCH
	  op1  = tregs->R_CSP[m->R_IP++];
#else
	  op1 = tregs->fetch(m->R_CS, m->R_IP++);
#endif
	  (* (i86_optab[op1]) )(tregs);

	}
    delete tregs;
    /* end major hack mode. */
 }

void DEFUN(check_ip_access,(m),
	   PC_ENV *m)
 {
    /* NULL as of now */
 }
    

void DEFUN(check_sp_access,(m),
    PC_ENV *m)
 {

 }

void DEFUN(check_mem_access,(m, p),
	   PC_ENV *m AND
	   u_int32 p)
 {
    /*  check bounds, etc */

 }

void DEFUN(check_data_access,(m, s,o),
	   PC_ENV *m AND
	   u_int16 s AND
	   u_int16 o)
 {
    /*  check bounds, etc */

 }


void DEFUN(inc_decoded_inst_len,(m,x),
	   PC_ENV *m AND
	   int x)
 {
    m->enc_pos += x;
 }    

   
void DEFUN(decode_printf,(m,x),
	   PC_ENV *m AND
	   const char *x)
 {
	fprintf(stderr, "%s", x);
//    sprintf(m->decoded_buf+m->enc_str_pos,"%s",x);
//    m->enc_str_pos += strlen(x);
 }

void DEFUN(decode_printf2,(m,x,y),
	   PC_ENV *m AND
	   const char *x AND
	   u_int16  y)
 {
    char temp[100];
    sprintf(temp,x,y);
	fprintf(stderr, "%s", temp);
//    sprintf(m->decoded_buf+m->enc_str_pos,"%s",temp);    
//    m->enc_str_pos += strlen(temp);
 }

void DEFUN(end_instr,(m),
	   PC_ENV *m)
 {

    m->enc_str_pos = 0;
    m->enc_pos = 0;
 }



static void DEFUN(print_encoded_bytes,(m,s,o),
		  PC_ENV *m AND
		  u_int16 s AND
		  u_int16 o)
 {
    int i;
    char buf1[64];
    for (i=0; i< m->enc_pos; i++)
	{
	   sprintf(buf1+2*i,"%02x", 
		   fetch_data_byte_abs(m,s,o+i));
	}
    debug_printf(m,"%-20s",buf1);
 }
  
static void DEFUN(print_decoded_instruction,(m),
		  PC_ENV *m)
{
   debug_printf(m,"%s", m->decoded_buf);
}




static void DEFUN(print_ivectab,(m),
    PC_ENV *m)
 {
    int iv;
    u_int32 i,j;
    
    iv = 0;
    for (i=0; i<256/4; i++)
	{
	   for (j=0; j<4; j++,iv++)
	     print_int_vect(m,iv);
	   debug_printf(m,"\n");
	}
 }
    

void DEFUN(print_int_vect,(m,iv),
		  PC_ENV *m AND
		  u_int16 iv)
 {
    
    u_int16 seg,off;

    if (iv > 256) return;
    seg   = fetch_data_word_abs(m,0,iv*4);
    off   = fetch_data_word_abs(m,0,iv*4+2);
    debug_printf(m,"%04x:%04x ", seg, off);
 }

void DEFUN(dump_memory,(m,seg,off,amt),
	   PC_ENV *m AND
	   u_int16 seg AND
	   u_int16 off AND 
	   u_int32 amt)
 {
    u_int32 start = off & 0xfffffff0;
    u_int32 end  = (off+16) & 0xfffffff0;
    u_int32 i;
    u_int32 current;
    
    current = start;
    while (end <= off + amt)
      {
	 debug_printf(m,"%04x:%04x ", seg, start);
	 for (i=start; i< off; i++)
	   debug_printf(m,"   ");
	 for (       ; i< end; i++)
	   debug_printf(m,"%02x ", 
			fetch_data_byte_abs(m,seg,i));
	 debug_printf(m,"\n");
	 start = end;
	 end = start + 16;
      }
    
 }
 
void DEFUN(single_step,(m),
	   PC_ENV  *m)
 {
   
    int c;
    char s[1024];
    int  ps[10];
    int ntok;
    int cmd;
    int done;
    int offset;
    static int breakpoint;
    char *p;
    

    if (DEBUG_BREAK(m))
      if (m->saved_ip != breakpoint)
	return;
      else
	  {
	     m->m_settings.debug |= DEBUG_TRACE_F;
	     m->m_settings.debug &= ~DEBUG_BREAK_F;
	     trace_regs(m);
	  }
    
    done=0;
    
    offset = m->saved_ip;
    
    while (! done)
	{
	   printf("-");
	   p = fgets(s, 1023, stdin);
	   cmd = parse_line(s, ps, &ntok);
	   switch(cmd)
	       {
		case 'u':
		  disassemble_forward(m,m->saved_cs,offset,10);
		  break;
		case 'd':  
		  if (ntok == 2)
		      {
			 offset = ps[1];
			 dump_memory(m,m->saved_cs,offset,16);
			 offset += 16;
		      }
		  else
		      {
			 dump_memory(m,m->saved_cs,offset,16);
			 offset += 16;
		      }
		  break;
		case 'c':
		  m->m_settings.debug ^= DEBUG_TRACECALL_F;
		  break;

		case 's':
		  m->m_settings.debug ^= DEBUG_SVC_F | DEBUG_SYS_F | DEBUG_SYSINT_F;
		  break;
		  
		case 'r':
		  trace_regs(m);
		  break;
		case 'g':
		  if (ntok == 2)
		      {
			 breakpoint = ps[1];
			 m->m_settings.debug &= ~DEBUG_TRACE_F;
			 m->m_settings.debug |= DEBUG_BREAK_F;
			 done = 1;
		      }
		  if (ntok < 2)
		  {
			 step_off(m);
			 done = 1;
		  }
		  break;
 
		case 'q':
		  exit(1);
		case 't':
		case 0:
		  done = 1;
		  break;
	       }   
	}
    
 }
    
int trace_on(PC_ENV *m)
{
	m->m_settings.debug |= DEBUG_DECODE_F | DEBUG_TRACE_F;
	return m->m_settings.debug;
}

int trace_off(PC_ENV *m)
{
	m->m_settings.debug &= ~(DEBUG_DECODE_F | DEBUG_TRACE_F);
	return m->m_settings.debug;
}

int step_on(PC_ENV *m)
{
	m->m_settings.debug |= DEBUG_STEP_F | DEBUG_DECODE_F | DEBUG_TRACE_F;
	return m->m_settings.debug;
}

int step_off(PC_ENV *m)
{
	m->m_settings.debug &= ~(DEBUG_STEP_F | DEBUG_DECODE_F | DEBUG_TRACE_F);
	return m->m_settings.debug;
}


    

    

static int
  DEFUN(parse_line,(s,ps,n),
	char *s AND
	int *ps AND
	int *n)
 {
    int cmd;
    
    *n = 0;
   
    while(*s == ' ' || *s == '\t') s++;

    ps[*n] = *s;

    switch (*s)
	{
	 case '\n':
	   *n += 1;
	   return 0;
	 default:
	   cmd = *s;
	   *n += 1;
	}

    while (*s != ' ' && *s != '\t' && *s != '\n')  s++;

    if (*s == '\n')
      return cmd;

    while(*s == ' ' || *s == '\t') s++;
  
    sscanf(s,"%x",&ps[*n]);
    *n += 1;
    return cmd;
    
 }



    
#endif

void    debug_printf(PC_ENV *m, const char *fmt, ...)
{
   va_list	p;
	
   va_start(p, fmt);
   vfprintf(m->dbg_stream, fmt, p);
   fflush(m->dbg_stream);
   va_end(p);
}

void DEFUN(dump_regs,(m),
	   PC_ENV  *m)
 {

    debug_printf(m,"AX=%04x  ", m->R_AX ); 
    debug_printf(m,"BX=%04x  ", m->R_BX );
    debug_printf(m,"CX=%04x  ", m->R_CX );
    debug_printf(m,"DX=%04x  ", m->R_DX );
    debug_printf(m,"SP=%04x  ", m->R_SP );
    debug_printf(m,"BP=%04x  ", m->R_BP );
    debug_printf(m,"SI=%04x  ", m->R_SI );
    debug_printf(m,"DI=%04x\n", m->R_DI );
    debug_printf(m,"DS=%04x  ", m->R_DS );
    debug_printf(m,"ES=%04x  ", m->R_ES );
    debug_printf(m,"SS=%04x  ", m->R_SS );
    debug_printf(m,"CS=%04x  ", m->R_CS );
    debug_printf(m,"IP=%04x   ", m->R_IP );
    if (ACCESS_FLAG(m,F_OF)) debug_printf(m,"OV ");     /* CHECKED... */
    else                 debug_printf(m,"NV ");
    if (ACCESS_FLAG(m,F_DF)) debug_printf(m,"DN ");
    else                 debug_printf(m,"UP ");
    if (ACCESS_FLAG(m,F_IF)) debug_printf(m,"EI ");
    else                 debug_printf(m,"DI ");
    if (ACCESS_FLAG(m,F_SF)) debug_printf(m,"NG ");
    else                 debug_printf(m,"PL ");
    if (ACCESS_FLAG(m,F_ZF)) debug_printf(m,"ZR ");
    else                 debug_printf(m,"NZ ");
    if (ACCESS_FLAG(m,F_AF)) debug_printf(m,"AC ");
    else                 debug_printf(m,"NA ");
    if (ACCESS_FLAG(m,F_PF)) debug_printf(m,"PE ");    
    else                 debug_printf(m,"PO ");
    if (ACCESS_FLAG(m,F_CF)) debug_printf(m,"CY ");
    else                 debug_printf(m,"NC ");
    debug_printf(m,"\n");
 }    


void PC_ENV::debug_init(void)
 {
    dbg_stream = stderr;
 }
    
