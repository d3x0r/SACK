
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

%ifdef BCC32
SECTION _TEXT class=CODE ALIGN=4 USE32 
%else
SECTION .text ; section of text needed to export code routines (COFF)
%endif

; This better be defined in alphamac.asm
;%ifndef mmxalpha_image_init
;%macro mmxalpha_image_init 0
;%endm
;%endif
;----------------------------------------------------------------------------
; ASM Loops
;----------------------------------------------------------------------------


mproc asmBlotScaledT0MMX
proc asmBlotScaledT0
StandardArgs
	save_di
	save_si
	save_bx

	ScaleLoop 
; alphafix used to be passed... so transparent pixels were put as transparent...
;asmalpha_fix

	load_bx
	load_si
	load_di
endp


mproc asmBlotScaledT1MMX
proc asmBlotScaledT1
StandardArgs
	save_di
	save_si
	save_bx

	ScaleLoopSkip0 

	load_bx
	load_si
	load_di
endp


proc asmBlotScaledTA
StandardArgs
arg %$alpha
	save_di
	save_si
	save_bx

	ScaleLoopSkip0 asmalpha_flat

	load_bx
	load_si
	load_di
endp

proc asmBlotScaledTImgA
StandardArgs
arg %$alpha
	save_di
	save_si
	save_bx

	ScaleLoopSkip0 asmalpha_image

	load_bx
	load_si
	load_di
endp

proc asmBlotScaledTImgAI
StandardArgs
arg %$alpha
	save_di
	save_si
	save_bx

	ScaleLoopSkip0 asmalphainvert_image

	load_bx
	load_si
	load_di
endp

;----------------------------------------------------------------------------
; MMX Loops
;----------------------------------------------------------------------------


proc asmBlotScaledTAMMX
StandardArgs
arg %$alpha
	save_di
	save_si
	save_bx
	
        mmxalpha_image_init
	ScaleLoopSkip0 mmxalpha_flat

	emms
	load_bx
	load_si
	load_di
endp

proc asmBlotScaledTImgAMMX
StandardArgs
arg %$alpha
	save_di
	save_si
	save_bx

	mmxalpha_image_init
	ScaleLoopSkip0 mmxalpha_image

	emms
	load_bx
	load_si
	load_di
endp

proc asmBlotScaledTImgAIMMX
StandardArgs
arg %$alpha
	save_di
	save_si
	save_bx

	mmxalpha_image_init
	ScaleLoopSkip0 mmxalphainvert_image

	emms
	load_bx
	load_si
	load_di
endp
