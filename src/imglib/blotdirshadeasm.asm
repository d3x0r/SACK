

%ifdef BCC32
SECTION _DATA class=DATA ALIGN=4 USE32 
%else
SECTION .data
%endif
%include "ImageFile.asm"
%include "c32.mac"
%include "mmx.asm"
%include "blotdirmac.asm"
%include "alphamac.asm"
%include "blotshademac.asm"

%ifdef BCC32
SECTION _TEXT class=CODE ALIGN=4 USE32 
%else
SECTION .text ; section of text needed to export code routines (COFF)
%endif

;---------------------------------------------------------------------------
;  ASM Loops
;---------------------------------------------------------------------------

proc asmCopyPixelsShadedT0
arg %$po  ;0x8
arg %$pi  ;0xc

arg %$oo  ;0x10
arg %$oi  ;0x14

arg %$ws  ;0x18
arg %$hs  ;0x1c

arg %$color ;0x20
	save_si
	save_di
	save_bx

	BlotLoopSet asmshadepixel	

	load_bx
	load_di
	load_si
endp

;---------------------------------------------------------------------------

proc asmCopyPixelsShadedT1
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$color
	save_si
	save_di
	save_bx

	BlotLoop asmshadepixel	

	load_bx
	load_di
	load_si
endp

;---------------------------------------------------------------------------

proc asmCopyPixelsShadedTA
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$alpha
arg %$color
	save_si
	save_di
	save_bx

	BlotLoop asmshadepixel, asmalpha_flat

	load_bx
	load_di
	load_si
endp

;---------------------------------------------------------------------------

proc asmCopyPixelsShadedTImgA
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$alpha
arg %$color
	save_si
	save_di
	save_bx

	BlotLoop asmshadepixel, asmalpha_image	

	load_bx
	load_di
	load_si
endp

;---------------------------------------------------------------------------

proc asmCopyPixelsShadedTImgAI
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$alpha
arg %$color
	save_si
	save_di
	save_bx

	BlotLoop asmshadepixel, asmalphainvert_image	

	load_bx
	load_di
	load_si
endp

;---------------------------------------------------------------------------
;   MMX Loops
;---------------------------------------------------------------------------

proc asmCopyPixelsShadedT0MMX
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$color
	save_si
	save_di
	save_bx

	mmxshadestart
	BlotLoopSet mmxshadepixel

	emms
	load_bx
	load_di
	load_si
endp

;---------------------------------------------------------------------------

proc asmCopyPixelsShadedT1MMX
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$color
	save_si
	save_di
	save_bx

	mmxshadestart
	BlotLoop mmxshadepixel

	emms
	load_bx
	load_di
	load_si
endp

;---------------------------------------------------------------------------

proc asmCopyPixelsShadedTAMMX
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$alpha
arg %$color
	save_si
	save_di
	save_bx

	mmxshadestart
	BlotLoop mmxshadepixel, mmxalpha_flat

	emms
	load_bx
	load_di
	load_si
endp

;---------------------------------------------------------------------------

proc asmCopyPixelsShadedTImgAMMX
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$alpha
arg %$color
	save_si
	save_di
	save_bx

	mmxshadestart

	BlotLoop mmxshadepixel, mmxalpha_image

	emms
	load_bx
	load_di
	load_si
endp

;---------------------------------------------------------------------------

proc asmCopyPixelsShadedTImgAIMMX
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$alpha
arg %$color
	save_si
	save_di
	save_bx

	mmxshadestart
	BlotLoop mmxshadepixel, mmxalphainvert_image

	emms
	load_bx
	load_di
	load_si
endp

;---------------------------------------------------------------------------
