;
;    JOYCE v2.5.0 - Amstrad PCW emulator
;
;    Copyright (C) 1996, 2001-2, 2022  John Elliott <seasip.webmaster@gmail.com>
;
;    This program is free software; you can redistribute it and/or modify
;    it under the terms of the GNU General Public License as published by
;    the Free Software Foundation; either version 2 of the License, or
;    (at your option) any later version.
;
;    This program is distributed in the hope that it will be useful,
;    but WITHOUT ANY WARRANTY; without even the implied warranty of
;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;    GNU General Public License for more details.
;
;    You should have received a copy of the GNU General Public License along
;    with this program; if not, write to the Free Software Foundation, Inc.,
;    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
;==========================================================================
; BIOS for CP/M-86 v1.1 in JOYCE guest environment
;==========================================================================
;
CR	EQU	0Dh
LF	EQU	0Ah
;
; I/O ports to request help from native code
;
EMU_SD	equ	0AEEBh	;SELDSK
EMU_RD	equ	0AEECh	;READ
EMU_WR	equ	0AEEDh	;WRITE
EMU_DRV	equ	0AEEEh	;Host drives
EMU_UC1	equ	0AEEFh	;Console write
;
; Offsets in the CCP / BDOS
;
CCP_BOOT	equ	0	;CCP entry (cold boot)
CCP_ENTRY	equ	6	;CCP entry (warm boot)
CCP_COMLEN	equ	0Ah	;Length of initial CCP command
CCP_CMD		equ	0Bh	;Initial CCP command
CCP_BDOS	equ	008Bh	;CCP call to BDOS
CCP_CRLF	equ	00A6h	;CCP function that displays CR/LF
BDOS_ENTRY	equ	0B06h	;BDOS INT 0E0h dispatcher
;
; Offsets in the BDOS system variables area
;
BDOS_CCPDRV	equ	24B7h	;Boot / default drive
BDOS_TIME	equ	24BBh	;Current timestamp
BDOS_DAY	equ	24C0h	;ASCII time display
BDOS_MONTH	equ	24C3h
BDOS_YEAR	equ	24C6h
BDOS_HOUR	equ	24C9h
BDOS_MINUTE	equ	24CCh
BDOS_SECOND	equ	24CCh
BDOS_WIDTH	equ	24E0h	;Console width

	org	2500h

	jmp	BOOT		;BIOS jumpblock
	jmp	WBOOT
	jmp	CONST
	jmp	CONIN
	jmp	CONOUT
	jmp	LIST
	jmp	PUNCH
	jmp	READER
	jmp	HOME
	jmp	SELDSK
	jmp	SETTRK
	jmp	SETSEC
	jmp	SETDMA
	jmp	READ
	jmp	WRITE
	jmp	LISTST
	jmp	SECTRAN
	jmp	SETDMAB
	jmp	GETSEGB
	jmp	GETIOB
	jmp	SETIOB
	
	db	0,0,0,0,0	;Five reserved entries
	db	0,0,0,0,0
	db	0,0,0,0,0

	dw	0	;FUNKEY
	dw	BIOSDK
	dw	0	;USART
	dw	0	;dev_conin
	dw	0	;FIDDS
	dw	0	;SCREEN
;
; These are at a fixed offset so that the emulator can poke in the address of
; the transfer buffer.
xferbuf:
	DW	0	;255A transfer buffer offset
tpaseg:	DW	0	;255C transfer buffer segment
	DW	0	;255E CP/M-80 BDOS offset
;
clkbuf:	dw	0	;Address of date stamp in Z80 memory

;============================================================================
;8086 interrupt handlers
;============================================================================
;
; INT 0D0h: API (currently only exposes Terminate and CD INFO)
;
int_d0:	jmp	short id0_func
	db	'JOYCE',1,0
;
id0_func:
	or	ah,ah
	jnz	intd0_full
	call	terminate
	iret	

intd0_full:
	push	bp	;BP+2 ->IP
	mov	bp,sp	;BP+0 ->BP
	push	si	;BP-2 ->SI
	push	di	;BP-4 ->DI
	push	bx	;BP-6 ->BX
	push	dx	;BP-8 ->DX
	push	cx	;BP-A ->CX
	push	ax	;BP-C ->AX
	push	ds	;BP-E ->DS
	push	es	;BP-10->ES
	mov	bx,0FFh
	mov	[bp-12],bx
	cmp	ah,0E3h
	jnz	intd0_end

	mov	ax,[cs:tpaseg]
	mov	es,ax
	mov	di,[cs:xferbuf]
	mov	al, 10h
	call	extcall		;Do the CD VERSION call
	mov	ax,[es:di+1]	;Result: A
	xor	ah,ah
	mov	[bp-12],ax
	mov	ax,[es:di+2]	;Result: BC
	mov	[bp-10], ax
	mov	ax,[es:di+4]	;Result: DE
	mov	[bp-8], ax	
	mov	ax,[es:di+6]
	mov	[bp-6], ax

intd0_end:
	pop	es
	pop	ds
	pop	ax	
	pop	cx
	pop	dx
	pop	bx
	pop	di
	pop	si	
	pop	bp
	iret

;
; Timer interrupt handler
;
timer:	push	ds
	push	es
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	mov	ax,[cs:tpaseg]
	mov	ds,ax
	mov	si,[cs:clkbuf]
	and	si,si		;CLKBUF set? If not, skip.
	jz	timer9
	mov	ax,cs		;Copy CP/M timestamp from CP/M 3 SCB to
	mov	es,ax		;CP/M-86 sysvars.
	mov	di,BDOS_TIME	;Copy time stamp
	push	di
	mov	cx,5
	rep	movsb
	mov	ds,ax		;DS -> BDOS
;
; Now render the time stamp as text
;
	pop	si		;SI->timestamp
	mov	al,[si+4]	;Seconds
	cmp	al,[lastsec]
	jz	timer9		;Nothing has changed -- don't bother 
	mov	[lastsec],al	;updating the timestamp

	lodsw			;AX = day count
	call	computeyear	;Returns AX=year, DX = days remainder
	push	ax
	xor	bx,bx		;Leap year bias
	test	al,3
	jnz	nleap1
	cmp	dx,60
	jc	nleap1
	inc	bx		;
nleap1:	call	computemonth	;Sets CX = month DX=day
	mov	ax,dx
	mov	di,BDOS_DAY
	call	putdec
	mov	al,'/'
	stosb
	mov	ax,cx
	inc	ax
	call	putdec
	mov	al,'/'
	stosb
	pop	ax		;Retrieve year
	call	putdec
	mov	al,' '
	stosb	
	lodsb			;BCD hours
	call	putbcd	
	mov	al,':'
	stosb	
	lodsb
	call	putbcd		;BCD minutes
	mov	al,':'
	stosb	
	lodsb
	call	putbcd		;BCD seconds

timer9:
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	es
	pop	ds
	iret			;Currently: timer does nothing
;
; Based on CP/M 3 DATE.PLM. This is compute$year
;
computeyear:	
	mov	cx,1978		;Base year
cy0:	mov	bx,365
	test	cl,3
	jnz	notleap
	inc	bx		;BX = days in year
notleap:
	cmp	ax,bx
	jc	cyend		;Date falls in this year
	inc	cx
	sub	ax,bx
	jmp	short cy0

cyend:	push	ax		;AX = days remainder
	mov	ax,cx		;AX = year
	xor	dx,dx
	mov	bx,100
	div	bx		;AX = century DX=year
	mov	ax,dx
	pop	dx
	ret
;
; Based on compute$month from CP/M 3 DATE
;
computemonth:			;DX = day count  BX = leap bias
	push	si
	mov	cx,12
cmonth0:
	dec	cx
	cmp	cx,2
	jnc	cmonth1
	xor	bx,bx		;Clear leap bias once we're in Jan or Feb
cmonth1:
	mov	si,monthdays
	add	si,cx
	add	si,cx
	mov	ax,[si]		;AX = days to start of month
	add	ax,bx
	cmp	ax,dx		;DX = day count
	jc	cmonth2
	or	cx,cx
	jnz	cmonth0
cmonth2:
	sub	dx,ax		;DX = day
	sub	dx,bx
	pop	si	
	ret			;Returns CX=month 0-11


;
monthdays:
	dw	0,31,59,90,120,151,181,212,243,273,304,334	
;
putdec:	push	dx
	xor	ah,ah
	mov	bl,10
	div	bl		;AL = tens AH = units
	call	putdig
	mov	al,ah
	call	putdig
	pop	dx
	ret
;
putbcd:	push	ax
	shr	al,1
	shr	al,1
	shr	al,1
	shr	al,1
	call	putdig
	pop	ax
putdig:	and	al,0Fh
	add	al,'0'
	stosb
	ret
;
ukint:	call	dumpregs	;Unknown interrupt. Dump registers.
	mov	ax,cs
	mov	ds,ax
	mov	si,MUKINT	;Print "Unknown interrupt"
	call	print
	mov	si,REGS		;Print register dump
	jmp	diewith		;Shut down guest
;
diverr:	call	dumpregs
	mov	ax,cs
	mov	ds,ax
	mov	si,MDIVERR
	call	print
	mov	si,REGS
	jmp	diewith
;
dbreak:	call	dumpregs
	mov	ax,cs
	mov	ds,ax
	mov	si,MDBREAK
	call	print
	mov	si,REGS
	jmp	diewith
;
di_halt:
	call	dumpregs
	mov	ax,cs
	mov	ds,ax
	mov	si,MDI_HALT
	call	print
	mov	si,REGS
	jmp	diewith

invop:
	call	dumpregs
	mov	ax,cs
	mov	ds,ax
	mov	si,MINVOP
	call	print
	mov	si,REGS
	jmp	diewith
;
;============================================================================
; Write all 8086 registers into a buffer to be displayed later
;============================================================================
dumpregs:
	push	di
	mov	di,REGAX
	call	hexax
	mov	ax,bx
	mov	di,REGBX
	call	hexax
	mov	ax,cx
	mov	di,REGCX
	call	hexax
	mov	ax,dx
	mov	di,REGDX
	call	hexax
	mov	ax,si
	mov	di,REGSI
	call	hexax
	pop	ax
	mov	di,REGDI
	call	hexax
	mov	ax,bp
	mov	di,REGBP
	call	hexax
	mov	bp,sp
	mov	ax,bp
	add	ax,4
	mov	di,REGSP
	call	hexax
	mov	ax,[ss:bp+2]
	mov	di,REGIP
	call	hexax
	mov	ax,[ss:bp+4]
	mov	di,REGCS
	call	hexax
	mov	ax,[ss:bp+6]
	mov	di,REGPSW
	call	hexax
	mov	ax,ds
	mov	di,REGDS
	call	hexax
	mov	ax,es
	mov	di,REGES
	call	hexax
	mov	ax,ss
	mov	di,REGSS
	call	hexax
	ret
;
;============================================================================
; Render AX as hexadecimal
;============================================================================
hexax:	xchg	ah,al
	call	hexal
	xchg	ah,al
hexal:	push	ax
	shr	al,1
	shr	al,1
	shr	al,1
	shr	al,1
	call	hexa
	pop	ax
hexa:	and	al,0Fh
	cmp	al,10
	jc	hexa1
	add	al,7
hexa1:	add	al,'0'
	mov	[cs:di],al
	inc	di
	ret
;
;============================================================================
;Warm boot. Redo CCP patch; update console width and flush host buffers.
;============================================================================
WBOOT:	call	patchccp
	mov	ah,1Ah		;Get console width
	call	func1
	mov	[BDOS_WIDTH],al
	mov	ah,0Ch		;Flush disk buffers
	call	func1	
	jmp	CCP_ENTRY
;
; Apply two patches: One to make RUBOUT behave like BACKSPACE rather than
; echoing the character it deletes, and the other to display "[CP/M-86]" 
; before the prompt.
;
patchccp:
	mov	di,14F2h	;Patch BDOS to treat RUBOUT like BACKSPACE
	mov	ax,[di]
	cmp	ax,0ED0Ah	;Was there an OR CH,CH there?
	jnz	pccp1
				;Replace with
	mov	ax,08B0h	;MOV AX,8
	mov	[di],ax
	mov	al,0E9h
	mov	[di+2],al	;JMP 14E0h
	mov	ax,0FFE9h
	mov	[di+3],ax	
pccp1:
	mov	di,0365h	;Now look at the CCP.
	mov	ax,[di-2]
	cmp	ax,0FD41h	;Is there a call to CCP_CRLF where we expect
	jnz	nopatch		;it?
	mov	ax, myfunc
	sub	ax, di		;Change it to the offset to MYFUNC.
	mov	[di-2],ax
nopatch:
	ret
;
; Patch to the CCP to print "[CP/M-86] " before the A> prompt, so I don't
; get totes confused.
;
myfunc: call	CCP_CRLF
	mov	dx, mcpm86
	mov	cl,9
	jmp	CCP_BDOS	;CCP BDOS
;
mcpm86:	db	'[CP/M-86] $'
;
;============================================================================
; Drive access. Drives can either be provided by the host or by the 8086 
; emulator. Reading an emulator port will determine which, and perform any
; necessary actions.
;============================================================================
;
SELDSK: mov	dl,cl		;DL = drive
	mov	dh,0
	mov	si,alldrv
	add	si,dx
	add	si,dx	
	mov	bx,[si]		;BX -> DPH
	mov	di,[bx+10]	;DI -> CP/M-86 DPB
	mov	dx, EMU_SD	;Determine whether drive CL is operated by
	in	al,dx		;the emulator or the host.
	or	al,al
	jz	sdhost
	ret	
;	
sdhost: push	es
	mov	ax,[cs:tpaseg]
	mov	es,ax
	mov	di,[cs:xferbuf]
	mov	ch,0
	mov	[es:di+1],cx	;Drive
	mov	al,0Eh
	call	extcall
	mov	bx,[es:di+1]	;Result
	pop	es
	or	bx,bx
	jz	sdret
;
; SELDSK was successful. Populate CP/M-86 DPB from CP/M-80 DPB
;
	mov	dl,cl		;DL = drive
	mov	dh,0
	mov	si,alldrv
	add	si,dx
	add	si,dx	
	mov	bx,[si]		;BX -> DPH
	mov	si,di
	inc	si
	inc	si
	inc	si		;SI -> CP/M-80 DPB
	mov	di,[bx+10]	;DI -> CP/M-86 DPB
	push	di
	mov	cx,0Fh
	push	es
	push	ds
	mov	ax,[cs:tpaseg]
	mov	ds,ax
	mov	ax,cs
	mov	es,ax		
	rep	movsb		;Copy DPB from CP/M-80 to CP/M-86
	pop	ds
	pop	es
	pop	di		;DI->CP/M-86 DPB
	and	word [di+11], 7FFFh	;Mask off 'fixed disk' bit from CKS
sdret:	ret
;
SETTRK:	mov	[bios_track],cx
	ret
;
SETSEC:	mov	[bios_sector],cx
	ret
;
READ:	mov	bx,[bios_track]
	mov	cx,[bios_sector]
	mov	si,[bios_dma]
	mov	di,[bios_dmaseg]
	mov	dx,EMU_RD
	in	al,dx
	or	al,al
	jz	readhost
	mov	al,ah
	xor	ah,ah
	ret
;
readhost:
	push	es
	mov	ax,[cs:tpaseg]
	mov	es,ax
	mov	di,[cs:xferbuf]
	mov	bx,[bios_track]
	mov	[es:di+1],bx	;Track
	mov	bx,[bios_sector]
	mov	[es:di+3],bx	;Sector
	mov	al, 14h
	call	extcall		;READ
	mov	ax,[es:di+1]	;Result
	pop	es
	or	al,al
	jnz	readend
;
; Copy buffer out of Z80 memory
;
	push	ds
	push	es	
	mov	ax,[bios_dma]
	mov	di,ax
	mov	ax,[bios_dmaseg]
	mov	es,ax
	mov	si,[xferbuf]
	inc	si
	inc	si	;-> xferbuf+2
	mov	ax,[cs:tpaseg]
	mov	ds,ax
	mov	cx,64
	rep	movsw
	pop	es
	pop	ds
	xor	al,al
readend:
	xor	ah,ah
	ret	
;
WRITE:	push	cx
	mov	bx,[bios_track]
	mov	cx,[bios_sector]
	mov	si,[bios_dma]
	mov	di,[bios_dmaseg]
	mov	dx,EMU_WR
	in	al,dx
	pop	cx
	or	al,al
	jz	writehost
	mov	al,ah
	xor	ah,ah
	ret
;
writehost:
	push	es
	mov	ax,[cs:tpaseg]
	mov	es,ax
	mov	di,[xferbuf]
	mov	bx,[bios_track]
	mov	[es:di+1],bx	;Track
	mov	bx,[bios_sector]
	mov	[es:di+3],bx	;Sector
	mov	[es:di+5],cl	;Write mode
	add	di,6

	push	ds		;Copy data to xferbuf+6
	mov	ax,[bios_dma]
	mov	si,ax
	mov	ax,[bios_dmaseg]
	mov	ds,ax
	mov	cx,64
	rep	movsw
	pop	ds
	mov	di,[xferbuf]
	mov	al, 15h
	call	extcall		;WRITE
	mov	al,[es:di+1]	;Result
	mov	ah,0
	pop	es
	ret
;
SECTRAN:
	mov	bx,cx
	ret
;
SETDMA:	mov	[bios_dma], cx
	ret
;
SETDMAB:
	mov	[bios_dmaseg], cx
	ret
;
GETSEGB:
	mov	bx, SEGB
	ret
;
REGS:		db	'AX='
REGAX:		db	'0000 BX='
REGBX:		db	'0000 CX='
REGCX:		db	'0000 DX='
REGDX:		db	'0000 SI='
REGSI:		db	'0000 DI='
REGDI:		db	'0000 BP='
REGBP:		db	'0000 SP='
REGSP:		db	'0000 IP='
REGIP:		db	'0000 PSW='
REGPSW:		db	'0000',CR,LF, 'CS='
REGCS:		db	'0000 DS='
REGDS:		db	'0000 ES='
REGES:		db	'0000 SS='
REGSS:		db	'0000',CR,LF,'$'

MDIVERR:	db	'Divide error',CR,LF,'$'
MUKINT:		db	'Unknown X86 interrupt.',CR,LF,'$'	
MDBREAK:	db	'Debug breakpoint',CR,LF,'$'
MDI_HALT:	db	'8086 crash by HALT with interrupts disabled',CR,LF,'$'
MINVOP:		db	'Invalid 8086 opcode'
NEWLINE:	db	CR,LF,'$'

showdrvs:
	mov	di,dirbuf
	push	di
	mov	cx,1041h
showd0:
	test	bx,1
	jz	showd1
	mov	al,cl
	stosb
	mov	al,':'
	stosb
	mov	al,' '
	stosb
showd1:	shr	bx,1
	inc	cl
	dec	ch
	jnz	showd0
	mov	al,'$'
	stosb
	pop	si
	jmp	print
;
diewith:
	call	print
terminate:
	push	es
	mov	ax,[cs:tpaseg]
	mov	es,ax
	mov	di,[cs:xferbuf]
	xor	al, al
	jmp	extcall

GETIOB:	mov	al,[cs:bios_iob]
	ret
;
; These BIOS implementations are simple wrappers around Z80 operations.
;
SETIOB:	mov	al,cl		;Save the IOBYTE
	mov	[cs:bios_iob],al
	mov	ah,8		;And pass it on to the Z80 host.
	jmp	short func1

HOME:	mov	ah,0Dh
	jmp	short func1

CONST:	mov	ah,0Bh
	jmp	short func1
;
CONIN:	mov	ah,1
	jmp	short func1s
;
CONOUT:	mov	al,[cs:bios_iob]
	and	al,3
	cmp	al,3		;Is CON: -> UC1?
	jnz	conout1
	mov	al,cl		;If so do a fast write
	mov	dx,EMU_UC1
	out	dx,al
	ret

conout1:
	mov	ah,2
	jmp	short func1
;
LIST:	mov	ah,5
	jmp	short func1
;
LISTST:	mov	ah,6
	jmp	short func1
;
PUNCH:	mov	ah,4
	jmp	short func1
;
READER:	mov	ah,3
	jmp	short func1
;
; Call to host function AH, passing CX as parameter. Returns AL = result
;
func1:	push	es
	mov	di,[cs:tpaseg]
	mov	es,di
	mov	di,[cs:xferbuf]
	mov	al,ah
	mov	[es:di+1],cx
	call	extcall
	mov	al,[es:di+1]
	pop	es
	ret
;
; func1s is similar to func1, but tries to idle while waiting for a result.
;
func1s:	push	es
	mov	di,[cs:tpaseg]
	mov	es,di
	mov	di,[cs:xferbuf]
	mov	al,ah
	mov	[es:di+1],cx
	call	extslow
	mov	al,[es:di+1]
	pop	es
	ret
;
print:
	push	es
	mov	ax,[cs:tpaseg]
	mov	es,ax
	mov	di,[xferbuf]
	push	di
	inc	di
cpymsg:	lodsb
	stosb
	cmp	al,'$'
	jnz	cpymsg
	pop	di
	mov	al,9
	call	extcall
	pop	es
	ret
;
; General call-to-host method. Write the function byte (< 0x80), wait for 
; processing to finished (indicated by the host writing a byte >= 0x80) 
; and return.
;
extcall:
	mov	[es:di],al
extlp:	mov	al,[es:di]
	test	al,80h
	jz	extlp
	ret
;
; A similar call, but for an action expected to be time-consuming (eg, 
; waiting for console input). Tries to idle the 8086 until the result 
; is ready.
;
extslow:
	mov	[es:di],al
extslp:	mov	al,[es:di]
	test	al,80h
	jnz	esend
	hlt
	jmp	short extslp
esend:	ret
;
bios_iob:
	db	95h	;IOBYTE (LST:=LPT PUN:=PTP RDR:=PTR CON:=CRT)
BIOSDK:	db	0
bios_whichdrv:
	db	0
bios_driveno:
	db	0
bios_track:
	dw	0
bios_sector:
	dw	0
bios_dma:
	dw	0
bios_dmaseg:
	dw	0
	db	0	;Verify flag

	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
stack:
;
; Disk Parameter Headers for up to 16 drives
;
alldrv:	dw	dph_a
	dw	dph_b
	dw	dph_c
	dw	dph_d
	dw	dph_e
	dw	dph_f
	dw	dph_g
	dw	dph_h
	dw	dph_i
	dw	dph_j
	dw	dph_k
	dw	dph_l
	dw	dph_m
	dw	dph_n
	dw	dph_o
	dw	dph_p
;
dph_a:	dw	0	;XLT
	dw	0,0,0	;Workspace
	dw	dirbuf
	dw	dpb_a
	dw	csv_a
	dw	alv_a
dph_b:	dw	0	;XLT
	dw	0,0,0	;Workspace
	dw	dirbuf
	dw	dpb_b
	dw	csv_b
	dw	alv_b
dph_c:	dw	0	;XLT
	dw	0,0,0	;Workspace
	dw	dirbuf
	dw	dpb_c
	dw	csv_c
	dw	alv_c
dph_d:	dw	0	;XLT
	dw	0,0,0	;Workspace
	dw	dirbuf
	dw	dpb_d
	dw	csv_d
	dw	alv_d
dph_e:	dw	0	;XLT
	dw	0,0,0	;Workspace
	dw	dirbuf
	dw	dpb_e
	dw	csv_e
	dw	alv_e
dph_f:	dw	0	;XLT
	dw	0,0,0	;Workspace
	dw	dirbuf
	dw	dpb_f
	dw	csv_f
	dw	alv_f
dph_g:	dw	0	;XLT
	dw	0,0,0	;Workspace
	dw	dirbuf
	dw	dpb_g
	dw	csv_g
	dw	alv_g
dph_h:	dw	0	;XLT
	dw	0,0,0	;Workspace
	dw	dirbuf
	dw	dpb_h
	dw	csv_h
	dw	alv_h
dph_i:	dw	0	;XLT
	dw	0,0,0	;Workspace
	dw	dirbuf
	dw	dpb_i
	dw	csv_i
	dw	alv_i
dph_j:	dw	0	;XLT
	dw	0,0,0	;Workspace
	dw	dirbuf
	dw	dpb_j
	dw	csv_j
	dw	alv_j
dph_k:	dw	0	;XLT
	dw	0,0,0	;Workspace
	dw	dirbuf
	dw	dpb_k
	dw	csv_k
	dw	alv_k
dph_l:	dw	0	;XLT
	dw	0,0,0	;Workspace
	dw	dirbuf
	dw	dpb_l
	dw	csv_l
	dw	alv_l
dph_m:	dw	0	;XLT
	dw	0,0,0	;Workspace
	dw	dirbuf
	dw	dpb_m
	dw	csv_m
	dw	alv_m
dph_n:	dw	0	;XLT
	dw	0,0,0	;Workspace
	dw	dirbuf
	dw	dpb_n
	dw	csv_n
	dw	alv_n
dph_o:	dw	0	;XLT
	dw	0,0,0	;Workspace
	dw	dirbuf
	dw	dpb_o
	dw	csv_o
	dw	alv_o
dph_p:	dw	0	;XLT
	dw	0,0,0	;Workspace
	dw	dirbuf
	dw	dpb_p
	dw	csv_p
	dw	alv_p
;
; DPBs for up to 16 drives
;
dpb_a:	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
dpb_b:	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
dpb_c:	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
dpb_d:	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
dpb_e:	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
dpb_f:	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
dpb_g:	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
dpb_h:	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
dpb_i:	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
dpb_j:	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
dpb_k:	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
dpb_l:	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
dpb_m:	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
dpb_n:	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
dpb_o:	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
dpb_p:	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
;
; Allocation vectors for up to 16 drives
;
alv_a:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
alv_b:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
alv_c:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
alv_d:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
alv_e:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
alv_f:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
alv_g:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
alv_h:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
alv_i:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
alv_j:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
alv_k:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
alv_l:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
alv_m:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
alv_n:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
alv_o:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
alv_p:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	
;
; Checksum vectors for up to 16 drives
;
csv_a:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
csv_b:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
csv_c:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
csv_d:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
csv_e:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
csv_f:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
csv_g:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
csv_h:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
csv_i:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
csv_j:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
csv_k:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
csv_l:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
csv_m:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
csv_n:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
csv_o:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
csv_p:	dw	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

SEGB:	db	1	;One memory region
SEGB0B:	dw	0	;Base segment
SEGB0L:	dw	0	;Length (paras)

	dw	0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0	;Space for up to 7
							;other regions	

dirbuf:	

; Code / data after this point are used only at boot, so the space can be 
; freed afterwards and reused for buffers.
;
; Sign-on message
;
banner:	db	'JOYCE CP/M-86 guest BIOS  '
ntpa:	db	'000K TPA',CR,LF
	db	'Boot drive is '
bdrv:	db	'X:',CR,LF
	db	'Host drives:  $'
ldrv:	db	CR,LF,'Guest drives: $'
boot_drive:
	db	0	;Drive at CP/M-86 startup
lastsec: db	0FFh	;Clock seconds at last clock update
drvmap:	dw	0	;Bitmap of host drives
drvemu:	dw	0	;Bitmap of guest drives
;
; Cold start code. Firstly, initialise segment registers and stack.
;
BOOT:	cli
	mov	ax,cs
	mov	ds,ax
	mov	es,ax
	mov	ss,ax
	mov	sp, stack
	call	initvec		;Set up interrupt handlers.
	sti			;Enable interrupts in case we want to HLT.

	mov	dx,EMU_DRV	;Get list of local drives
	in	ax,dx
	mov	[drvemu],ax
;
; Wait for the host to become ready
; 
	push	es
	mov	ax,[cs:tpaseg]
	mov	es,ax
	mov	di,[xferbuf]
wait0:	mov	al,[es:di]
	test	al,80h
	jz	wait0
	mov	ah,0Fh		;Get address of SCB date stamp
	call	func1
	mov	ax,[es:di+1]
	mov	[clkbuf],ax
	mov	ah,18h		;Get list of host drives
	call	func1
	mov	ax,[es:di+1]
	mov	bx,[drvemu]
	not	bx		;Native drives override host drives
	and	ax,bx
	mov	[drvmap],ax
	pop	es
	mov	ah,19h		;Get host's current drive
	call	func1
	mov	[boot_drive],al
	mov	[BDOS_CCPDRV],al
	add	al,'A'
	mov	[bdrv], al	
;
	mov	ax,dirbuf	;Calculate size of guest TPA
	add	ax,8Fh		;Next paragraph beyond DIRBUF
	shr	ax,1
	shr	ax,1
	shr	ax,1
	shr	ax,1		;First paragraph of TPA
	mov	[SEGB0B],ax
	mov	bx,[cs:tpaseg]	;First paragraph beyond TPA
	sub	bx,ax		;Length of TPA
	mov	[SEGB0L],bx
	mov	ax,bx		;Get size of TPA in paragraphs
	mov	cl,6
	shr	ax,cl		;Convert to kbytes
	mov	di,ntpa		;Render as decimal
	xor	dx,dx
	mov	bx,100		;Hundreds
	div	bx
	add	al,'0'
	stosb
	mov	ax,dx
	xor	dx,dx
	mov	bx,10		;Tens
	div	bx
	add	al,'0'
	stosb
	mov	al,dl		;and units
	add	al,'0'	
	stosb

	mov	si, banner	;Display sign-on banner
	call	print
	mov	bx,[drvmap]
	call	showdrvs
	mov	si,ldrv
	call	print
	mov	bx,[drvemu]
	call	showdrvs
	mov	si,NEWLINE
	call	print

	mov	ah,1Ah		;Get host console width
	call	func1
	mov	[BDOS_WIDTH],al
	call	patchccp	;Patch the CCP to display "[CP/M-86] A>"
	mov	ah,0Ah
	call	func1		;Get initial command
	push	es
	mov	ax,[cs:tpaseg]	;Copy it from the host to the CCP_CMD buffer.
	mov	es,ax
	mov	si,[xferbuf]
	mov	di,CCP_CMD
	mov	cl,0		;CL will be its length
copycmd:
	inc	si
	mov	al,[es:si]
	mov	[di],al
	or	al,al
	jz	copycmd1
	inc	di
	inc	cl
	jmp	short copycmd
;
copycmd1:
	mov	[CCP_COMLEN],cl	;Store length

	mov	cl,[boot_drive]	;CCP should be entered with CL = drive
	jmp	CCP_BOOT	;Launch the CCP.
;
;============================================================================
; Initialise x86 interrupt vectors 
;============================================================================
;
initvec:
	push	es
	xor	ax,ax
	mov	es,ax
	mov	di,ax		;ES:DI = 0000:0000
	mov	cx,256
	mov	ax,cs		;Initialise 256 vectors
	mov	bx,ukint
initv1:
	mov	[es:di],bx	;Store offset
	inc	di
	inc	di
	stosw			;Store segment
	loop	initv1
	mov	bx, BDOS_ENTRY
	mov	di, 380h	;Point INT 0E0h at the BDOS
	mov	[es:di],bx
	mov	bx,diverr
	mov	di,0		;Point INT 0 to divide error
	mov	[es:di],bx
	mov	bx,dbreak
	mov	di,0Ch		;Point INT 3 to debug break
	mov	[es:di],bx
	mov	bx,int_d0
	mov	di, 340h	;Point INT D0 to internal handler
	mov	[es:di],bx
	mov	bx,timer
	mov	di, 344h	;Point INT D1 to timer
	mov	[es:di],bx
	mov	bx,invop
	mov	di, 348h	;Point INT D2 to invalid opcode
	mov	[es:di],bx
	mov	bx,di_halt
	mov	di, 34Ch	;Point INT D3 to DI/HALT
	mov	[es:di],bx
	pop	es
	ret




	
