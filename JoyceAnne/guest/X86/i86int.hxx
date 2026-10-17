
#include "ansidecl.h"
#include "../sysdep.h"
#include "i86.hxx"
#include "i87.hxx"
#include "i86guest.hxx"
#include "i86_externs.h"
#include "i87_externs.h"
#include "sys_debug.h"

/* I/O ports */
enum IOPorts
{
	EMU_SD = 0xAEEB,	/* SELDSK2 */
	EMU_RD = 0xAEEC,	/* READ2 */
	EMU_WR = 0xAEED,	/* WRITE2 */
	EMU_DRV= 0xAEEE,	/* Get list of host drives */
	EMU_UC1= 0xAEEF		/* Fast console write */
};

enum Segments
{
	SEG_TPA		= 0xE000,	/* TPA mapped into x86 memory */
	SEG_VIDEO	= 0xF000	/* Video RAM mapped into x86 mem */
};
