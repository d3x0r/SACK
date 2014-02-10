
%ifdef BCC32
SECTION _DATA class=DATA ALIGN=4 USE32 
%else
SECTION .data
%endif

%include "ImageFile.asm"
%include "c32.mac"
%include "mmx.asm"
%include "blotscamac.asm"
%include "alphamac.asm"
%include "blotshademac.asm"

%ifdef BCC32
SECTION _TEXT class=CODE ALIGN=4 USE32 
%else
SECTION .text ; section of text needed to export code routines (COFF)
%endif

;----------------------------------------------------------------------------
; ASM Loops
;----------------------------------------------------------------------------

proc asmBlotScaledMultiT0
StandardArgs
arg %$colorR
arg %$colorG
arg %$colorB
	save_di
	save_si
	save_bx

	ScaleLoop asm_multishadepixel

	load_bx
	load_si
	load_di
endp

proc asmBlotScaledMultiT1
StandardArgs
arg %$colorR
arg %$colorG
arg %$colorB
	save_di
	save_si
	save_bx

	ScaleLoopSkip0 asm_multishadepixel

	load_bx
	load_si
	load_di
endp


proc asmBlotScaledMultiTA
StandardArgs
arg %$alpha
arg %$colorR
arg %$colorG
arg %$colorB
	save_di
	save_si
	save_bx

	ScaleLoopSkip0  asm_multishadepixel, asmalpha_flat

	load_bx
	load_si
	load_di
endp

proc asmBlotScaledMultiTImgA
StandardArgs
arg %$alpha
arg %$colorR
arg %$colorG
arg %$colorB
	save_di
	save_si
	save_bx

	ScaleLoopSkip0 asm_multishadepixel, asmalpha_image

	load_bx
	load_si
	load_di
endp

proc asmBlotScaledMultiTImgAI
StandardArgs
arg %$alpha
arg %$colorR
arg %$colorG
arg %$colorB
	save_di
	save_si
	save_bx

	ScaleLoopSkip0 asm_multishadepixel, asmalphainvert_image

	load_bx
	load_si
	load_di
endp

;----------------------------------------------------------------------------
; MMX Loops
;----------------------------------------------------------------------------

proc asmBlotScaledMultiT0MMX
StandardArgs
arg %$colorR
arg %$colorG
arg %$colorB
	save_di
	save_si
	save_bx

	MMXMShadeSetup
	ScaleLoop MMXMultiShade

	emms
	load_bx
	load_si
	load_di
endp


proc asmBlotScaledMultiT1MMX
StandardArgs
arg %$colorR
arg %$colorG
arg %$colorB
	save_di
	save_si
	save_bx

	MMXMShadeSetup
	ScaleLoopSkip0 MMXMultiShade

	emms
	load_bx
	load_si
	load_di
endp


proc asmBlotScaledMultiTAMMX
StandardArgs
arg %$alpha
arg %$colorR
arg %$colorG
arg %$colorB
	save_di
	save_si
	save_bx
	MMXMShadeSetup
	ScaleLoopSkip0 MMXMultiShade, mmxalpha_flat

	emms
	load_bx
	load_si
	load_di
endp

proc asmBlotScaledMultiTImgAMMX
StandardArgs
arg %$alpha
arg %$colorR
arg %$colorG
arg %$colorB
	save_di
	save_si
	save_bx

	MMXMShadeSetup
	ScaleLoopSkip0 MMXMultiShade, mmxalpha_image

	emms
	load_bx
	load_si
	load_di
endp

proc asmBlotScaledMultiTImgAIMMX
StandardArgs
arg %$alpha
arg %$colorR
arg %$colorG
arg %$colorB
	save_di
	save_si
	save_bx

	MMXMShadeSetup
	ScaleLoopSkip0 MMXMultiShade, mmxalphainvert_image

	emms
	load_bx
	load_si
	load_di
endp
