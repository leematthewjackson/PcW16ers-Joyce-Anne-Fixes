/* 
 * Dos/PC Emulator
 * Copyright (C) 1991 Jim Hudgens
 * Copyright 2005,2007,2008,2022 John Elliott 
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

#include "../guest.hxx"
#include "../guestimpl.hxx"

class PC_ENV : public GuestImpl
{
private:
	long	m_timers, m_ticks, m_delay;
	unsigned char *m_ram;
	GuestState m_state;
protected:
	unsigned short m_first, m_last;
	std::string m_res_global, m_res_local;
	int m_curdrive;

	int loadres(std::string filename, unsigned addr);
public:
	PC_ENV(Host *host); /*  : Guest(host) {} */
	virtual ~PC_ENV();

	virtual int reset();
	virtual void set_xferbuf(unsigned short first, unsigned short last);
	virtual void set_resourcedirs(const char *global, const char *local);
	virtual void on_timer();
	virtual void clock_tick();

	virtual void sys_out8(u_int16 port, u_int8 value);
	virtual void sys_out16(u_int16 port, u_int16 value);
	virtual u_int16 sys_in16(u_int16 port);
	virtual u_int8 sys_in8(u_int16 port);
	virtual void sys_lock(void);

	virtual u_int8 peek(unsigned offset);
	virtual void poke(unsigned offset, u_int8 value);

	virtual void set_state(GuestState st);
	virtual GuestState get_state();

/* Return a shallow copy of this object */
	virtual PC_ENV *clone();

	virtual bool halted(void);

/* Deprecated. Use only for divide overflow and NMI. */
	void	i86_intr_raise(u_int8 intrnum);
/* All other interrupts should do this instead: */
	void	i86_intr_raise(IntAck *device);
	void	i86_intr_lower(IntAck *device);
/* Set the processor going */
	int	i86_exec(void);

protected:
	void debug_init();
	void copy_to(PC_ENV *m);

public:
	void i86_intr_handle();
   /*	The registers!!		*/
   struct i386_general_regs  Gn_regs;
   struct i386_special_regs  Sp_regs;
   struct i386_segment_regs  Sg_regs;
   /* our flags structrure.  This contains information on
           REPE prefix           2 bits  repe,repne
           SEGMENT overrides     5 bits  normal,DS,SS,CS,ES
           Delayed flag set      3 bits  (zero, signed, parity)
	   reserved              6 bits
           interrupt #           8 bits  instruction raised interrupt
	   BIOS video segregs    4 bits  
	   Interrupt Pending     1 bits  
           Extern interrupt      1 bits
	   Halted                1 bits
    */
   int m_cputicks;	   
   long   sysmode;
   int	  m_intNo;
   IntAck *m_intDev;

   inline void store(u_int16 seg, u_int16 off, u_int8 value)
   {
	store(  ((((u_int32)seg) << 4) + off) & 0xFFFFF, value); 
   }

   inline u_int8 fetch(u_int16 seg, u_int16 off)
   {
	return fetch(  ((((u_int32)seg) << 4) + off) & 0xFFFFF);
   }

   virtual void store(u_int32 address, u_int8 value);
   virtual u_int8 fetch(u_int32 address);
   
   inline u_int16 fetch16(u_int16 seg, u_int16 off)
   {
/* Note 2 calls to fetch, not one to fetch16, so that wrap at off=0xFFFF
 * works */
	return fetch(seg, off) | (((u_int16)fetch(seg, off+1)) << 8);
   }

   inline void store16(u_int16 seg, u_int16 off, u_int16 v)
   {
	store(seg, off,   (v & 0xFF));
	store(seg, off+1, (v >> 8));
   }
   
   inline void store16(u_int32 address, u_int16 v)
   {
	store(address,   (v & 0xFF));
	store(address+1, (v >> 8));
   }

   inline u_int16 fetch16(u_int32 address)
   {
	return fetch(address) | (((u_int16)fetch(address+1)) << 8);
   }
   
   inline u_int32 fetch32(u_int32 address)
   {
	return fetch16(address) | (((u_int32)fetch16(address+2)) << 16);
   }
  
   /* Get a segment:offset pair as an absolute offset */ 
   inline u_int32 fetch20(u_int32 address)
   {
	return fetch16(address) + (((u_int32)fetch16(address+2)) << 4);
   }


   /* Illegal opcode. Return true to abort, false to continue (possibly with
    * fixed-up registers) */
   virtual bool illegal_op();


   /* system memory information */
   u_int8 *	m_mem_base;
	
#ifdef DEBUG
   int			check;
   u_int16		saved_ip;
   u_int16		saved_cs;
   int                  enc_pos;
   int                  enc_str_pos;
   char                 decode_buf[32];      /* encoded byte stream  */
   char                 decoded_buf[256];   /* disassembled strings */
#endif
   /* needs to be defined always. */
   FILE                 *dbg_stream;
};

extern IS_VOLATILE int intr;

