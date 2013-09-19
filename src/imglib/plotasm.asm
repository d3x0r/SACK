
%ifdef BCC32
SECTION _DATA class=DATA ALIGN=4 USE32 
%else
SECTION .data
%endif
%include "ImageFile.asm"
%include "c32.mac"
%include "alphamac.asm"
%include "mmx.asm"

%ifdef BCC32
SECTION _TEXT class=CODE ALIGN=4 USE32 
%else
SECTION .text ; section of text needed to export code routines (COFF)
%endif

;---------------------------------------------------------------------------

proc asmplot
arg %$image
arg %$x
arg %$y
arg %$d
 	 save_di
basicplot:         
.begin:
      	;db 0xcc;//int 3;
      mov edi, [sbptr + %$image];
 		mov eax, [edi + IMGFILE_pwidth]
 		mov ecx, [sbptr + %$y]
		sub ecx, [edi + IMGFILE_effy]
 		test ecx, 80000000h
 		jnz  .done
%ifdef INVERT_IMAGE
		mov edx, [edi + IMGFILE_h]
		sub ecx, edx
		not ecx
		cmp ecx, edx
%else
 		cmp ecx, [edi + IMGFILE_h]
%endif
 		jnb .done
 		mul ecx	 ; multiply Y * pwidth
 		mov ecx, [sbptr + %$x]
                sub ecx, [edi + IMGFILE_effx]
 		test ecx, 80000000h
 		jnz .done
 		cmp ecx, [edi + IMGFILE_w]
 		jnb .done
 		add eax, ecx
 		mov ecx, [sbptr + %$d]
      mov edx, [edi + IMGFILE_image]; ;// don't change the format.....
 		mov [edx + eax*4], ecx
.done:
 	 load_di
endp

;---------------------------------------------------------------------------

proc asmplotraw
arg %$image
arg %$x
arg %$y
arg %$d
 	 save_di
      mov edi, [sbptr + %$image];
 		mov eax, [edi + IMGFILE_pwidth]
 		mov ecx, [sbptr + %$y]
		sub ecx, [edi + IMGFILE_effy]
 		mul ecx	 ; multiply Y * pwidth

 		add eax, [sbptr + %$x]
                sub eax, [edi + IMGFILE_effx]
 		mov ecx, [sbptr + %$d]
      mov edx, [edi + IMGFILE_image]; ;// don't change the format.....
 		mov [edx + eax*4], ecx
.done:
 	 load_di
endp

;---------------------------------------------------------------------------

proc asmgetpixel
arg %$image
arg %$x
arg %$y
 	 save_di
      mov edi, [sbptr + %$image];
 		mov eax, [edi + IMGFILE_pwidth]
 		mov ecx, [sbptr + %$y]
		sub ecx, [edi + IMGFILE_effy]
 		test ecx, 80000000h
 		jnz  .done
%ifdef INVERT_IMAGE
		mov edx, [edi + IMGFILE_h]
		sub ecx, edx
		not ecx
		cmp ecx, edx
%else
 		cmp ecx, [edi + IMGFILE_h]
%endif
 		jnb .done
 		mul ecx	 ; multiply Y * pwidth
 		mov ecx, [sbptr + %$x]
                sub ecx, [edi + IMGFILE_effx]
 		test ecx, 80000000h
 		jnz .done
 		cmp ecx, [edi + IMGFILE_w]
 		jnb .done
 		add eax, ecx
      mov edx, [edi + IMGFILE_image]; ;// don't change the format.....
 		mov eax, [edx + eax*4]
.done:
 	 load_di
endp
;---------------------------------------------------------------------------

proc asmplotalpha
arg %$image
arg %$x
arg %$y
arg %$color
	save_di
 	 	cmp byte [sbptr + %$color + 3], 255
 	 	jne .alphaok
 	 	jmp basicplot ; this plot will be solid anyhow
.alphaok:
      mov edi, [sbptr + %$image];
 		mov eax, [edi + IMGFILE_pwidth]
 		mov ecx, [sbptr + %$y]
		sub ecx, [edi + IMGFILE_effy]
 		test ecx, 80000000h
 		jz  .ypos
 		jmp  .done
.ypos:
%ifdef INVERT_IMAGE
		mov edx, [edi + IMGFILE_h]
		sub ecx, edx
		not ecx
		cmp ecx, edx
%else
 		cmp ecx, [edi + IMGFILE_h]
%endif
 		jb .yok
 		jmp .done
.yok:
 		mul ecx	 ; multiply Y * pwidth
 		mov ecx, [sbptr + %$x]
                sub ecx, [edi + IMGFILE_effx]
 		test ecx, 80000000h
 		jz  .xokay
 		jmp .done
	.xokay:
 		cmp ecx, [edi + IMGFILE_w]
 		jl  .xokay2
 		jmp .done
	.xokay2:
 		add eax, ecx
 		mov ecx, [sbptr + %$color]
      mov edx, [edi + IMGFILE_image]; ;// don't change the format.....
      lea edi, [edx+eax*4]
	save_bx
      asmalpha_solid
	load_bx
.done:
 	load_di
endp
;---------------------------------------------------------------------------
proc asmplotalphaMMX
arg %$image
arg %$x
arg %$y
arg %$color
 	 save_di
 	 	cmp byte [sbptr + %$color + 3], 255
 	 	jne .alphaok
 	 	jmp basicplot ; this plot will be solid anyhow
.alphaok:
      mov edi, [sbptr + %$image];
 		mov eax, [edi + IMGFILE_pwidth]
 		mov ecx, [sbptr + %$y]
		sub ecx, [edi + IMGFILE_effy]
 		test ecx, 80000000h
 		jnz  .done
%ifdef INVERT_IMAGE
		mov edx, [edi + IMGFILE_h]
		sub ecx, edx
		not ecx
		cmp ecx, edx
%else
 		cmp ecx, [edi + IMGFILE_h]
%endif
 		jnb .done

 		mul ecx	 ; multiply Y * pwidth
 		mov ecx, [sbptr + %$x]
                sub ecx, [edi + IMGFILE_effx]
 		test ecx, 80000000h
 		jnz .done
 		cmp ecx, [edi + IMGFILE_w]
 		jnb .done
 		add eax, ecx
      mov edx, [edi + IMGFILE_image]; ;// don't change the format.....
      lea edi, [edx+eax*4]
		mmxalpha_solid_init
		mmxalpha_solid
.done:
	emms
 	load_di
endp
;---------------------------------------------------------------------------

proc IsMMX
	;identify existence of CPUID instruction ... ... 
	; identify Intel processor .... 
	save_bx
	mov EAX, 1 ; request for feature flags 
	CPUID ; 0Fh, 0A2h CPUID instruction 
	xor eax,eax
	test EDX, 00800000h ; Is IA MMX technology bit (Bit 23 of EDX) 
							  ; in feature flags set? 
	jz .done
.MMX_Technology_Found:
	inc eax
.done:
	load_bx
endp
