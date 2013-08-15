; Completed port Oct 2, 2001 1:55a
; that is all assembly/mmx routines written and tested...
; direct copy success...

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


%ifdef BCC32
SECTION _TEXT class=CODE ALIGN=4 USE32 
%else
SECTION .text ; section of text needed to export code routines (COFF)
%endif

; this BETTER be defined in alphamac.asm
;%ifndef mmxalpha_image_init
;%macro mmxalpha_image_init 0
;%endm
;%endif

;---------------------------------------------------------------------------

; MMX version is same as assembly version...
mproc asmCopyPixelsT0MMX
proc asmCopyPixelsT0
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
	save_si
	save_di

      mov esi, [sbptr+%$pi]  ; source image
      mov edi, [sbptr+%$po]  ; dest image
      mov edx, [sbptr+%$hs]  ; start line counter at total (assume more than 0) 
.LoopTop:
      mov ecx, [sbptr+%$ws]  ; set column counter
      rep movsd            ; move 32 bit values
      add edi, [sbptr+%$oo]  ; fix pointer next line dest
      add esi, [sbptr+%$oi]  ; fix pointer next line source 
      dec edx;
      jnz .LoopTop;

	load_di
	load_si
endp

;---------------------------------------------------------------------------
; again the MMX version is the same as the assembly version
mproc asmCopyPixelsT1MMX
proc asmCopyPixelsT1
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
	save_si
	save_di

		BlotLoop 

	load_di
	load_si
endp

;---------------------------------------------------------------------------

proc asmCopyPixelsTA
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$alpha
	save_si
	save_di
	save_bx

		BlotLoop asmalpha_flat

	load_bx
	load_di
	load_si
endp

;---------------------------------------------------------------------------

proc asmCopyPixelsTAMMX
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$alpha
	mmxalpha_image_init
	save_si
	save_di
	save_bx

		BlotLoop mmxalpha_flat
	emms
	load_bx
	load_di
	load_si
endp

;---------------------------------------------------------------------------

proc asmCopyPixelsTImgA
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$alpha
	save_si
	save_di
	save_bx

		BlotLoop asmalpha_image

	load_bx
	load_di
	load_si
endp

;---------------------------------------------------------------------------

proc asmCopyPixelsTImgAMMX
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$alpha
	mmxalpha_image_init
	save_si
	save_di
	save_bx

		BlotLoop mmxalpha_image

	emms
	load_bx
	load_di
	load_si
endp

;---------------------------------------------------------------------------

proc asmCopyPixelsTImgAI
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$alpha
	save_si
	save_di
	save_bx

		BlotLoop asmalphainvert_image

	load_bx
	load_di
	load_si
endp

;---------------------------------------------------------------------------

proc asmCopyPixelsTImgAIMMX
arg %$po
arg %$pi
arg %$oo
arg %$oi
arg %$ws
arg %$hs
arg %$alpha
	mmxalpha_image_init
	save_si
	save_di
	save_bx

		BlotLoop mmxalphainvert_image

	emms
	load_bx
	load_di
	load_si
endp

