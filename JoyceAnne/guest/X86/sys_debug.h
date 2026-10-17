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


static char debug_h_rcsid[]=
  "$Id: debug.h,v 1.2 1991/07/30 01:48:25 hudgens Exp hudgens $";

/* $Log: debug.h,v $
 * Revision 1.2  1991/07/30  01:48:25  hudgens
 * added copyright.
 *
 * Revision 1.1  1991/07/20  04:05:09  hudgens
 * Initial revision
 *
 *
 */



/* checks to be enabled for "runtime" */
#define CHECK_IP_FETCH_F		0x1
#define CHECK_SP_ACCESS_F		0x2
#define CHECK_MEM_ACCESS_F	        0x4 /*using regular linear pointer */
#define CHECK_DATA_ACCESS_F         	0x8 /*using segment:offset*/

#ifdef DEBUG
#define CHECK_IP_FETCH(m)		((m)->check & CHECK_IP_FETCH_F)
#define CHECK_SP_ACCESS(m)		((m)->check & CHECK_SP_ACCESS_F)
#define CHECK_MEM_ACCESS(m)		((m)->check & CHECK_MEM_ACCESS_F)
#define CHECK_DATA_ACCESS(m)     	((m)->check & CHECK_DATA_ACCESS_F)
#else
#define CHECK_IP_FETCH(m)		
#define CHECK_SP_ACCESS(m)		
#define CHECK_MEM_ACCESS(m)		
#define CHECK_DATA_ACCESS(m)	
#endif

/* debug options */

#define DEBUG_DECODE_F		0x1	/* print out decoded instruction  */
#define DEBUG_TRACE_F		0x2	/* dump regs before/after execution ala debug */
#define DEBUG_STEP_F        0x4
#define DEBUG_DISASSEMBLE_F 0x8
#define DEBUG_BREAK_F      0x10
#define DEBUG_SVC_F        0x20
#define DEBUG_FS_F         0x80
#define DEBUG_PROC_F      0x100
#define DEBUG_SYSINT_F    0x200     /* bios system interrupts. */
#define DEBUG_TRACECALL_F 0x400
#define DEBUG_INSTRUMENT_F 0x800
#define DEBUG_SYS_F        (DEBUG_SVC_F|DEBUG_FS_F|DEBUG_PROC_F)

#ifdef DEBUG
#define DEBUG_INSTRUMENT(m)     ((m)->m_settings.debug & DEBUG_INSTRUMENT_F)
#define DEBUG_DECODE(m)		((m)->m_settings.debug & DEBUG_DECODE_F)
#define DEBUG_TRACE(m)		((m)->m_settings.debug & DEBUG_TRACE_F)
#define DEBUG_STEP(m)		((m)->m_settings.debug & DEBUG_STEP_F)
#define DEBUG_DISASSEMBLE(m)   	((m)->m_settings.debug & DEBUG_DISASSEMBLE_F)
#define DEBUG_BREAK(m)		((m)->m_settings.debug & DEBUG_BREAK_F)
#define DEBUG_SVC(m)		((m)->m_settings.debug & DEBUG_SVC_F)
#define DEBUG_FS(m)		((m)->m_settings.debug & DEBUG_FS_F)
#define DEBUG_PROC(m)		((m)->m_settings.debug & DEBUG_PROC_F)
#define DEBUG_SYSINT(m)		((m)->m_settings.debug & DEBUG_SYSINT_F)
#define DEBUG_TRACECALL(m)	((m)->m_settings.debug & DEBUG_TRACECALL_F)
#define DEBUG_SYS(m)		((m)->m_settings.debug & DEBUG_SYS_F)
#else
#define DEBUG_INSTRUMENT(m)
#define DEBUG_DECODE(m)			
#define DEBUG_TRACE(m)			
#define DEBUG_STEP(m)			
#define DEBUG_DISASSEMBLE(m)	        
#define DEBUG_BREAK(m)			
#define DEBUG_SVC(m)			
#define DEBUG_FS(m)			
#define DEBUG_PROC(m)			
#define DEBUG_SYSINT(m)			
#define DEBUG_TRACECALL(m)		
#define DEBUG_SYS(m)			
#endif



#ifdef DEBUG

#define DECODE_PRINTF(m,x)         if (DEBUG_DECODE(m)) decode_printf(m,x)
#define DECODE_PRINTF2(m,x,y)      if (DEBUG_DECODE(m)) decode_printf2(m,x,y)


/* the following allow us to look at the bytes of an instruction.
   The first INCR_INSTRN_LEN, is
   called everytime bytes are consumed in the decoding process. 
   The SAVE_IP_CS is called initially when the major opcode of 
   the instruction is accessed. 
 */

#define INC_DECODED_INST_LEN(m,x)    if (DEBUG_DECODE(m)) \
                                          inc_decoded_inst_len(m,x)
#define SAVE_IP_CS(m,x,y) \
  if (DEBUG_DECODE(m)|DEBUG_TRACECALL(m)) {  \
      (m)->saved_cs = x;\
      (m)->saved_ip = y;\
   }

#else
#define INC_DECODED_INST_LEN(m,x)
#define DECODE_PRINTF(m,x) 
#define DECODE_PRINTF2(m,x,y) 
#define SAVE_IP_CS(m,x,y)        
#endif


/*  if (DEBUG_INSTRUMENT(x))\
         {\
	    instrument_instruction(x);\
	 }\ */

#ifdef DEBUG

#define TRACE_REGS(x)\
  if (DEBUG_DISASSEMBLE(x))  \
         {\
	       just_disassemble(x);\
               goto EndOfTheInstructionProcedure;\
	 }\
  if (DEBUG_TRACE(x)) trace_regs(x)

#else 
#define   TRACE_REGS(x)
#endif
    
#ifdef DEBUG
#define   SINGLE_STEP(x)   if (DEBUG_STEP(x))  single_step(x)
#else
#define   SINGLE_STEP(x)   
#endif
 
#ifdef DEBUG
#define  START_OF_INSTR(m)
#define  END_OF_INSTR(m)              EndOfTheInstructionProcedure:end_instr(m);
#else
#define  START_OF_INSTR(m)
#define  END_OF_INSTR(m)
#endif

#ifdef DEBUG
#define  CALL_TRACE(m,u,v,w)    if (DEBUG_TRACECALL(m))\
  debug_printf(m, "%x:%x: CALL %x:%x\n", v, u ,v, w);
#define  RETURN_TRACE(m,u)       if (DEBUG_TRACECALL(m))\
  debug_printf(m, "%x: RET\n",u);
#else
#define  CALL_TRACE(m,u,v,w)    
#define  RETURN_TRACE(m,u)   
#endif

/*	Prototypes	*/

void debug_printf(PC_ENV *m, const char *fmt, ...);
void EXFUN(inc_decoded_inst_len,(PC_ENV *m, int x));
void EXFUN(decode_printf,(PC_ENV *m,const  char *x));
void EXFUN(decode_printf2,(PC_ENV *m, const char *x, u_int16 y));
void EXFUN(just_disassemble,(PC_ENV *m));
void EXFUN(trace_regs,(PC_ENV *m));
void EXFUN(single_step,(PC_ENV *m));
void EXFUN(end_instr,(PC_ENV *m));
void EXFUN(dump_regs,(PC_ENV *m));
void EXFUN(dump_memory,(PC_ENV *m, u_int16 seg, u_int16 off, u_int32 amt));
void EXFUN(print_int_vect,(PC_ENV *m, u_int16 iv));

void EXFUN(instrument_instruction,(PC_ENV *m));

void EXFUN(check_ip_access,(PC_ENV *m));
void EXFUN(check_sp_access,(PC_ENV *m));
void EXFUN(check_mem_access,(PC_ENV *m, u_int32 p));
void EXFUN(check_data_access,(PC_ENV *m, u_int16 s, u_int16 o));


  

int EXFUN(trace_on,(PC_ENV *m));
int EXFUN(trace_off,(PC_ENV *m));
int EXFUN(step_on,(PC_ENV *m));
int EXFUN(step_off,(PC_ENV *m));
int EXFUN(dump_memory,(PC_ENV *m));


