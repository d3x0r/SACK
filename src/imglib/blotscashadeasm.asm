
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

proc asmBlotScaledShadedT0
StandardArgs
arg %$color
	save_di
	save_si
	save_bx

	ScaleLoop asmshadepixel

	load_bx
	load_si
	load_di
endp


proc asmBlotScaledShadedT1
StandardArgs
arg %$color
	save_di
	save_si
	save_bx

	ScaleLoopSkip0 asmshadepixel

	load_bx
	load_si
	load_di
endp


proc asmBlotScaledShadedTA
StandardArgs
arg %$alpha
arg %$color
	save_di
	save_si
	save_bx

	ScaleLoopSkip0  asmshadepixel, asmalpha_flat

	load_bx
	load_si
	load_di
endp

proc asmBlotScaledShadedTImgA
StandardArgs
arg %$alpha
arg %$color
	save_di
	save_si
	save_bx

	ScaleLoopSkip0 asmshadepixel, asmalpha_image

	load_bx
	load_si
	load_di
endp

proc asmBlotScaledShadedTImgAI
StandardArgs
arg %$alpha
arg %$color
	save_di
	save_si
	save_bx

	ScaleLoopSkip0 asmshadepixel, asmalphainvert_image

	load_bx
	load_si
	load_di
endp

;----------------------------------------------------------------------------
; MMX Loops
;----------------------------------------------------------------------------

proc asmBlotScaledShadedT0MMX
StandardArgs
arg %$color
	save_di
	save_si
	save_bx

	mmxshadestart
	ScaleLoop mmxshadepixel

	emms
	load_bx
	load_si
	load_di
endp


proc asmBlotScaledShadedT1MMX
StandardArgs
arg %$color
	save_di
	save_si
	save_bx

	mmxshadestart
	ScaleLoopSkip0 mmxshadepixel

	emms
	load_bx
	load_si
	load_di
endp


proc asmBlotScaledShadedTAMMX
StandardArgs
arg %$alpha
arg %$color
	save_di
	save_si
	save_bx
	mmxshadestart
	ScaleLoopSkip0 mmxshadepixel, mmxalpha_flat

	emms
	load_bx
	load_si
	load_di
endp

proc asmBlotScaledShadedTImgAMMX
StandardArgs
arg %$alpha
arg %$color
	save_di
	save_si
	save_bx

	mmxshadestart
	ScaleLoopSkip0 mmxshadepixel, mmxalpha_image

	emms
	load_bx
	load_si
	load_di
endp

proc asmBlotScaledShadedTImgAIMMX
StandardArgs
arg %$alpha
arg %$color
	save_di
	save_si
	save_bx

	mmxshadestart
	ScaleLoopSkip0 mmxshadepixel, mmxalphainvert_image

	emms
	load_bx
	load_si
	load_di
endp
