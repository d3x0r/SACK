%ifdef BCC32
SECTION _DATA class=DATA ALIGN=4 USE32 
%else
SECTION .data
%endif

%include "ImageFile.asm"
%include "c32.mac"
%include "mmx.asm"
%include "alphamac.asm"

%ifdef BCC32
SECTION _TEXT class=CODE ALIGN=4 USE32 
%else
SECTION .text ; section of text needed to export code routines (COFF)
%endif

proc asmBlatColor
arg %$po
arg %$oo
arg %$w
arg %$h
arg %$color
	save_di
BasicBlat:        
.BlatSolid:
      mov edi, [sbptr+%$po];
      mov edx, [sbptr+%$h];
      mov eax, [sbptr+%$color];
      cld;
.LoopTop:
      mov ecx, [sbptr+%$w]
      rep stosd;
      add edi, [sbptr+%$oo]; // skip offset to next portion
      dec edx;
      jnz .LoopTop;
	load_di
endp

;---------------------------------------------------------------------------

proc asmBlatColorAlpha
arg %$po
arg %$oo
arg %$w
arg %$h
arg %$color

	save_di
.alpha_okay:

      mov edi, [sbptr+%$po];
      mov edx, [sbptr+%$h];
      cld;
.LoopTop:
      mov ecx, [sbptr+%$w];
.MainLoop:
		save_dx
    ;  mov eax, [sbptr+%$color];
    ;---------------------
      mov cl, [sbptr + %$color + 3]
      xor ch, ch
      mov bx, 100h
      sub bx, cx
      mov di, cx
      inc di
      mov cx, 0ff00h ; set image alpha to solid while plotting?

   	mov al, [edi + 2]
   	xor ah, ah
   	mul bx      ; 256 - alpha * low
   	mov cl, ah
   	mov al, [sbptr + %$color + 2]
   	xor ah, ah
   	mul di
   	add cl, ah
   	jnc .lowok
   	mov cl, 0ffh;
.lowok:
   	shl ecx, 8

   	mov al, [edi + 1]
   	xor ah, ah
   	mul bx      ; 256 - alpha * low
   	mov cl, ah
   	mov al, [sbptr + %$color + 1]
   	xor ah, ah
   	mul di
   	add cl, ah
   	jnc .midok
   	mov cl, 0ffh;
.midok:
   	shl ecx, 8

      mov al, [edi ]
   	xor ah, ah
   	mul bx      ; 256 - alpha * low
   	mov cl, ah
   	mov al, [sbptr + %$color]
   	xor ah, ah
   	mul di
   	add cl, ah
   	jnc .highok
   	mov cl, 0ffh;
.highok:
 		mov eax, ecx
    ;---------------------
    	load_dx
		stosd;
		dec ecx
		jz .LineDone
		jmp .MainLoop
.LineDone:
      add edi, [sbptr+%$oo]; // skip offset to next portion
      dec edx;
      jz .AllDone;
      jmp .LoopTop
.AllDone:
	load_di
endp

;---------------------------------------------------------------------------

proc mmxBlatColorAlpha
arg %$po
arg %$oo
arg %$w
arg %$h
arg %$color
	save_di
		cmp byte [sbptr + %$color + 3 ], 255
		jne .checkalpha_0
		jmp BasicBlat
.checkalpha_0:
		cmp byte [sbptr+%$color +3], 0 ; total transparent useless to store...
		jne .alpha_okay
		jmp .ReallyAllDone
.alpha_okay:
	save_bx

      mov edi, [sbptr+%$po];
      mov ebx, [sbptr+%$h];
      cld;
.LoopTop:
      mov ecx, [sbptr+%$w];
.MainLoop:
  ;    mov eax, [sbptr+%$color];
  ;---------------------
      movd mm4, [sbptr + %$color]    ; get new color
      mov eax, 01010101h        ; fix spam alpha
      xor edx, edx;
      movd mm0, edx             ; make a 0 mmx register
      mov dl, [sbptr + %$color + 3]   ; get alpha parameter
      mul edx                   ; make eax be spammed alpha

      movd mm1, eax;            ; put spammed alpha in mm1
      movq mm2, [mmx_scaledone] ; put spammed 100h into mmx
      PUNPCKLBW mm1,mm0         ; unpack spammed alpha into words
      psubusw mm2, mm1          ; make spammed 256 - alpha

      paddusw mm1, [mmx_one];   ; add one to spammed alpha

      punpcklbw mm4, mm0 ; unpack new color to words

      movd mm3, [edi]    ; get existing color...
      PUNPCKLBW mm3,mm0  ; unpack existing color to words
      ; mm2 = 256-alpha
      ; mm1 = alpha+1
      ; mm0 = 0
      ; mm4 = input color
      ; mm3 = over color

      pmullw mm4, mm1   ; multiply color by 256-alpha

      pmullw mm3, mm2   ; multiply existing by alpha

      paddusw mm3, mm4  ; add new color plus existing color

      movq   mm2, mm0   ; this code may in fact save us a shift...
      packuswb mm2, mm3 ; have to investigate repacking...

      psrlw   mm3, 8
      packuswb mm3,mm0
      movd eax, mm3   ; set new composite out
   ; -- do the store ---
		stosd;
		dec ecx
		jz .LineDone
		jmp .MainLoop
.LineDone:
      add edi, [sbptr+%$oo]; // skip offset to next portion
      dec ebx;
      jz .AllDone;
      jmp .LoopTop
.AllDone:
	emms
	load_bx
.ReallyAllDone:
	load_di
endp
