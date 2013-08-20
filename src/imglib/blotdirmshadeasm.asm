
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

proc asmCopyPixelsMultiT0
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$colorR
arg %$colorG
arg %$colorB
	save_si
	save_di
	save_bx
		BlotLoopSet asm_multishadepixel
	load_bx
	load_di
	load_si
endp

;---------------------------------------------------------------------------

proc asmCopyPixelsMultiT1
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$colorR
arg %$colorG
arg %$colorB
	save_si
	save_di
	save_bx

		BlotLoop asm_multishadepixel

	load_bx
	load_di
	load_si
endp


;---------------------------------------------------------------------------

proc asmCopyPixelsMultiTA
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$alpha
arg %$colorR
arg %$colorG
arg %$colorB
	save_si
	save_di
	save_bx

		BlotLoop asm_multishadepixel, asmalpha_flat

	load_bx
	load_di
	load_si
endp

;---------------------------------------------------------------------------

proc asmCopyPixelsMultiTImgA
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$alpha
arg %$colorR
arg %$colorG
arg %$colorB
	save_si
	save_di
	save_bx

		BlotLoop asm_multishadepixel, asmalpha_image

	load_bx
	load_di
	load_si
endp

;---------------------------------------------------------------------------

proc asmCopyPixelsMultiTImgAI
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$alpha
arg %$colorR
arg %$colorG
arg %$colorB
	save_si
	save_di
	save_bx

		BlotLoop asm_multishadepixel, asmalphainvert_image

	load_bx
	load_di
	load_si
endp

;---------------------------------------------------------------------------
;   MMX Loops
;---------------------------------------------------------------------------

proc asmCopyPixelsMultiT0MMX
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$colorR
arg %$colorG
arg %$colorB
	save_si
	save_di
	save_bx

      MMXMShadeSetup
		BlotLoopSet MMXMultiShade 

	emms
	load_bx
	load_di
	load_si
endp

;---------------------------------------------------------------------------

proc asmCopyPixelsMultiT1MMX
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$colorR
arg %$colorG
arg %$colorB
	save_si
	save_di
	save_bx

      MMXMShadeSetup
		BlotLoop MMXMultiShade 

	emms
	load_bx
	load_di
	load_si
endp

;---------------------------------------------------------------------------

proc asmCopyPixelsMultiTAMMX
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$alpha
arg %$colorR
arg %$colorG
arg %$colorB
	save_si
	save_di
	save_bx

      MMXMShadeSetup
		BlotLoop MMXMultiShade, mmxalpha_flat

	emms
	load_bx
	load_di
	load_si
endp

;---------------------------------------------------------------------------

proc asmCopyPixelsMultiTImgAMMX
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$alpha
arg %$colorR
arg %$colorG
arg %$colorB
	save_si
	save_di
	save_bx

      MMXMShadeSetup
		BlotLoop MMXMultiShade, mmxalpha_image

	emms
	load_bx
	load_di
	load_si
endp

;---------------------------------------------------------------------------

proc asmCopyPixelsMultiTImgAIMMX
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$alpha
arg %$colorR
arg %$colorG
arg %$colorB
	save_si
	save_di
	save_bx

      MMXMShadeSetup
		BlotLoop MMXMultiShade, mmxalphainvert_image

	emms
	load_bx
	load_di
	load_si
endp

