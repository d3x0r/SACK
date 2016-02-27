
;---------------------------------------------------------------------------
;     MACROS DEFINED HERE
;  all - destroy ebx, and modify eax with result
;        _flat uses the alpha parameter from the stack only
;        _image uses the alpha paramter plus the image source pixel alpha
;        _invert_image uses the source pixel alpha minus the alpha paramter
;        _solid uses the alpha parameter of %$color only.
;      - expect edi to be the destination pointer, and eax to be the
;        pixel to write with source pixel alpha in it...
;	asmalpha_flat - eax, edi input, eax modified
; 
;  mmxalpha_flat - modifies eax
;
;  asmalpha_image - modifies eax
;
;  mmxalpha_image - modifies eax
;
;  asmalphainvert_image - modifies eax
;
;  mmxalphainvert_image - modifies eax
;
;---------------------------------------------------------------------------


%ifdef __LINUX__
extern AlphaTable ; precomputed table of alpha overlays
%define _AlphaTable AlphaTable
%else
extern _AlphaTable ; precomputed table of alpha overlays
%endif

;---------------------------------------------------------------------------
; these macros could probably be better reduced... was
; expected to reduce code amount to maintain, however,
; due to lazyness was mostly mass copied...

%macro asmalpha_flat 0
	   ; assume eax is input pixel
	   ; 		this will allow sharing code with multi and mono shade
      ; compute ebx * 256-alpha + ecx * alpha
      ; 
      ;db 0cch;
      save_dx
      save_cx
      save_di    ; save the pointer...
      mov ecx, [edi]
      save_cx    ; sptr+4 == original dest pixel
      mov ecx, eax
      save_cx    ; sptr == source pixel
      or ecx, ecx
      jnz %%not_trans  ; pixel was absolutely transparent
      add sptr, 3*ssize
      jmp %%abstrans
%%not_trans:
      mov cx, [sbptr + %$alpha]
      xor ch, ch
      mov bx, 100h
      sub bx, cx
      mov di, cx
      inc di
      mov cx, di  ; get the alpha
      shl cx, 8   ; shift it to ch
      ;mov cx, 0ff00h ; set image alpha to solid while plotting?

   	mov al, [sptr + 2]
   	xor ah, ah
   	mul di      ; 256 - alpha * low
   	mov cl, ah
   	mov al, [sptr + 6]
   	xor ah, ah
   	mul bx
   	add cl, ah
   	jnc %%lowok
   	mov cl, 0ffh;
%%lowok:
   	shl ecx, 8

   	mov al, [sptr + 1]
   	xor ah, ah
   	mul di      ; 256 - alpha * low
   	mov cl, ah
   	mov al, [sptr + 5]
   	xor ah, ah
   	mul bx
   	add cl, ah
   	jnc %%midok
   	mov cl, 0ffh;
%%midok:
   	shl ecx, 8

      mov al, [sptr ]
   	xor ah, ah
   	mul di      ; 256 - alpha * low
   	mov cl, ah
   	mov al, [sptr + 4]
   	xor ah, ah
   	mul bx
   	add cl, ah
   	jnc %%highok
   	mov cl, 0ffh;
%%highok:
		add sptr, 2*ssize ; junk pixel values
		load_di
		mov eax, ecx
%%abstrans:         
		load_cx
		load_dx
%endm


%macro asmalpha_fix 0
	mov ebx, eax;
	shr ebx, 24;
        cmp ebx, 0
        jnz %%skip_auto_null
        mov eax, ebx
%%skip_auto_null:
%endm

;---------------------------------------------------------------------------
%macro asmalpha_solid 0
	   ; assume [edi] is the original pixel to cover - updated.
	   ;        [sbptr+%$color] is input pixel
	   ; 		this will allow sharing code with multi and mono shade
      ; compute ebx * 256-alpha + ecx * alpha
      ; 
      ;db 0cch;
      save_dx
      save_cx
      save_di    ; save the pointer...
      mov ecx, [edi]
      save_cx    ; sptr+4 == original dest pixel
      mov ecx, [sbptr + %$color]
      save_cx    ; sptr == source pixel
      or ecx, ecx
      jnz %%not_trans  ; pixel was absolutely transparent
      add sptr, 3*ssize
      jmp %%abstrans
%%not_trans:
      mov cl, [sptr + 3] ; get alpha from saved color
      xor ch, ch
      mov bx, 100h
      sub bx, cx
      mov di, cx
      inc di
      mov cx, di  ; get the alpha
      shl cx, 8   ; shift it to ch
      ;;mov cx, 0ff00h ; set image alpha to solid while plotting?

   	mov al, [sptr + 2]
   	xor ah, ah
   	mul di      ; 256 - alpha * low
   	mov cl, ah
   	mov al, [sptr + 6]
   	xor ah, ah
   	mul bx
   	add cl, ah
   	jnc %%lowok
   	mov cl, 0ffh;
%%lowok:
   	shl ecx, 8

   	mov al, [sptr + 1]
   	xor ah, ah
   	mul di      ; 256 - alpha * low
   	mov cl, ah
   	mov al, [sptr + 5]
   	xor ah, ah
   	mul bx
   	add cl, ah
   	jnc %%midok
   	mov cl, 0ffh;
%%midok:
   	shl ecx, 8

      mov al, [sptr ]
   	xor ah, ah
   	mul di      ; 256 - alpha * low
   	mov cl, ah
   	mov al, [sptr + 4]
   	xor ah, ah
   	mul bx
   	add cl, ah
   	jnc %%highok
   	mov cl, 0ffh;
%%highok:
		add sptr, 2*ssize ; junk pixel values
		load_di
		mov [edi], ecx
%%abstrans:         
		load_cx
		load_dx
%endm

;---------------------------------------------------------------------------


%macro mmxalpha_flat 0
; incoming register: eax : color
;                    edi : destination pixel
; this clobbers eax, ebx
; however all other regsiters other than MMX regs are preserved
		save_dx;
      movd mm4, eax    ; get new color
      mov eax, 01010101h        ; fix spam alpha 
      xor ebx, ebx;
      movd mm0, ebx             ; make a 0 mmx register
      mov bl, [sbptr + %$alpha]   ; get alpha parameter
      mul ebx                   ; make eax be spammed alpha
      load_dx

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
      psrlw   mm3, 8

      packuswb mm3,mm0
      movd eax, mm3   ; set new composite out
%endm

;---------------------------------------------------------------------------

%macro mmxalpha_solid_init 0
; incoming register: [%$color + sbptr] : color
; this clobbers eax 
; however all other regsiters other than MMX regs are preserved
		save_dx;
      movd mm4, [sbptr+%$color]   ; get new color
      mov eax, 01010101h        ; fix spam alpha 
      xor edx, edx;
      movd mm0, edx             ; make a 0 mmx register
      mov dl, [sbptr + %$color +3]; get alpha parameter
      mul edx                   ; make eax be spammed alpha
      load_dx

      movd mm1, eax;            ; put spammed alpha in mm1
      movq mm2, [mmx_scaledone] ; put spammed 100h into mmx
      PUNPCKLBW mm1,mm0         ; unpack spammed alpha into words
      psubusw mm2, mm1          ; make spammed 256 - alpha

      paddusw mm1, [mmx_one];   ; add one to spammed alpha

      punpcklbw mm4, mm0 ; unpack color to words

      pmullw mm4, mm1    ; multiply color by 256-alpha
%endm

%macro mmxalpha_image_init 0
	movd mm0, [mmx_zero]       ; make a 0 mmx register
	movq mm5, [mmx_one]
	movq mm6, [mmx_scaledone] ; put spammed 100h into mmx
%endm

;---------------------------------------------------------------------------

%macro mmxalpha_solid 0
; incoming register: [edi] : destination pixel - updated.
; this clobbers [edi]
; however all other regsiters other than MMX regs are preserved
      movd mm3, [edi]    ; get existing color...
      PUNPCKLBW mm3,mm0  ; unpack existing color to words
      ; mm2 = 256-alpha
      ; mm1 = alpha+1
      ; mm0 = 0
      ; mm4 = input color
      ; mm3 = over color

      pmullw mm3, mm2   ; multiply existing by alpha


      paddusw mm3, mm4  ; add new color plus existing color
      psrlw   mm3, 8

      packuswb mm3,mm0
      movd [edi], mm3   ; set new composite out
%endm
;---------------------------------------------------------------------------

%macro asmalpha_image 0
      ; compute ebx * 256-alpha + ecx * alpha
      ; input 
      ; 		eax is source color
      ;     edi is dest pointer 
      ; output
      ; 		eax composit source pixel on with destination
      ;db 0cch;
      save_dx
      save_cx
      save_di
      mov ecx, [edi]
      save_cx    ; sptr+4 == original dest pixel
      mov ecx, eax
      save_cx    ; sptr   == source pixel
      or ecx, ecx
      jnz %%not_trans  ; pixel was absolutely transparent
      add sptr, 3*ssize
      mov eax, [edi]   
      jmp %%abstrans

%%not_trans:
		xor ecx, ecx ; clear ecx.
      mov cl, [sbptr + %$alpha] ; new alpha
      add cl, [sptr + 3]       ; plus new pixel's alpha
      jnc %%nofixalpha      ; skip fix overflow?
      mov cl, 0ffh;
%%nofixalpha:
      mov bx, 100h
      sub bx, cx

      mov di, cx
      inc di

		mov ch, [sptr + 7] ; dest pixel alpha -> ch
	   mov ch, [_AlphaTable + ecx] ; load table value uhmm
      ;mov cx, 0ff00h ; set image alpha to solid while plotting?

   	mov al, [sptr + 2]
   	xor ah, ah
   	mul di      ; 256 - alpha * low
   	mov cl, ah
   	mov al, [sptr + 6]
   	xor ah, ah
   	mul bx      ; alpha * low
   	add cl, ah
   	jnc %%lowok
   	mov cl, 0ffh;
%%lowok:
   	shl ecx, 8

   	mov al, [sptr + 1]
   	xor ah, ah
   	mul di      ; 256 - alpha * low
   	mov cl, ah
   	mov al, [sptr + 5]
   	xor ah, ah
   	mul bx
   	add cl, ah
   	jnc %%midok
   	mov cl, 0ffh;
%%midok:
   	shl ecx, 8

      mov al, [sptr ]
   	xor ah, ah
   	mul di      ; 256 - alpha * low
   	mov cl, ah
   	mov al, [sptr + 4]
   	xor ah, ah
   	mul bx
   	add cl, ah
   	jnc %%highok
   	mov cl, 0ffh;
%%highok:
		add sptr, 2*ssize ; junk pixel values
		load_di
		mov eax, ecx
%%abstrans:
		load_cx
		load_dx
%endm

;---------------------------------------------------------------------------

%macro mmxalpha_image 0
      ; input 
      ; 		eax is source color
      ;     edi is dest pointer 
      ; output
      ; 		eax composit source pixel on with destination
; this clobbers eax, ebx
; however all other regsiters other than MMX regs are preserved
		or eax, eax     ; test input color for absolute 0
		jnz %%notabstrans
		mov eax, [edi]
		jmp %%abstrans
%%notabstrans:
      movd mm4, eax   ; get new color
		save_ax        ; save source pixel on stack for alpha reference
		save_dx        ; save line counter
      mov eax, 01010101h        ; fix spam alpha 
      xor ebx, ebx;
      movd mm0, ebx             ; make a 0 mmx register
      mov bl, [sbptr + %$alpha]   ; get alpha parameter
      add bl, [sptr + 7]         ; add source alpha
		jnc %%noalphafix
		mov bx, 0ffh
%%noalphafix:
      mul ebx                   ; make eax be spammed alpha
      mov bh, [edi+3]   ; get dest alpha
      mov bl, [_AlphaTable+ebx] ; resolve dest alpha and my alpha

      load_dx         ; restore line counter
      add sptr, 1*ssize      ; trash saved pixel from stack

      shl ebx, 24  ; align alpha correctly..

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
      psrlw   mm3, 8

      packuswb mm3,mm0
      movd eax, mm3   ; set new composite out

      and eax, 0ffffffh;
      or  eax, ebx

%%abstrans:
%endm

;---------------------------------------------------------------------------

%macro asmalphainvert_image 0
      ; compute ebx * 256-alpha + ecx * alpha
      ; 
      ;db 0cch;
      save_dx
      save_cx
      save_di
      mov ecx, [edi]
      save_cx    ; edi == original dest pixel
      mov ecx, eax
      save_cx    ; sptr == source pixel
      or ecx, ecx
      jnz %%not_trans  ; pixel was absolutely transparent
      add sptr, 3*ssize
      mov eax, [edi]
      jmp %%abstrans
%%not_trans:
      xor ecx, ecx
      mov cl, [sptr + 3] 
      sub cl, [sbptr + %$alpha]
      ja  %%alphaok
      add sptr, 3*ssize
      mov eax, [edi]
		jmp %%abstrans
%%alphaok:
      mov bx, 100h
      sub bx, cx
      mov di, cx
      inc di
		
		mov ch, [sptr + 7] ; dest pixel alpha -> ch
	   mov ch, [_AlphaTable + ecx] ; load table value uhmm
      ;mov cx, 0ff00h ; set image alpha to solid while plotting?

   	mov al, [sptr + 2]
   	xor ah, ah
   	mul di      ; 256 - alpha * low
   	mov cl, ah
   	mov al, [sptr + 6]
   	xor ah, ah
   	mul bx
   	add cl, ah
   	jnc %%lowok
   	mov cl, 0ffh;
%%lowok:
   	shl ecx, 8

   	mov al, [sptr + 1]
   	xor ah, ah
   	mul di      ; 256 - alpha * low
   	mov cl, ah
   	mov al, [sptr + 5]
   	xor ah, ah
   	mul bx
   	add cl, ah
   	jnc %%midok
   	mov cl, 0ffh;
%%midok:
   	shl ecx, 8

      mov al, [sptr ]
   	xor ah, ah
   	mul di      ; 256 - alpha * low
   	mov cl, ah
   	mov al, [sptr + 4]
   	xor ah, ah
   	mul bx
   	add cl, ah
   	jnc %%highok
   	mov cl, 0ffh;
%%highok:
		add sptr, 2*ssize ; junk pixel values
		load_di
 		mov eax, ecx
%%abstrans:         
		load_cx
		load_dx
%endm

;---------------------------------------------------------------------------

%macro mmxalphainvert_image 0
; this clobbers eax, ebx
; however all other regsiters other than MMX regs are preserved
		or eax, eax
		jnz %%notabsolutetrans
		mov eax, [edi]
		jmp %%absolutetrans
%%notabsolutetrans:
      movd mm4, eax   ; get new color
		save_ax                  ; save pixel for alpha reference
		save_dx                  ; save line counter
      mov eax, 01010101h        ; fix spam alpha 
      xor ebx, ebx;
      movd mm0, ebx             ; make a 0 mmx register
      mov bl, [sptr + 7]         ; get source alpha
      sub bl, [sbptr + %$alpha]   ; subtract alpha parameter
		ja %%notalphaunderflow    ; alpha is within range?
      mov eax, [edi]            ; 0 alpha or underflow return dest pixel
      add sptr, 2*ssize ; unpop our stack!
		jmp %%absolutetrans       ; get outa here.
%%notalphaunderflow:
      mul ebx                   ; make eax be spammed alpha
      mov bh, [edi+3]   ; get dest alpha
      mov bl, [_AlphaTable+ebx] ; resolve dest alpha and my alpha
		shl ebx, 24

      load_dx                   ; restore line counter
      add sptr, 1*ssize                ; trash saved input pixel

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
      psrlw   mm3, 8

      packuswb mm3,mm0
      movd eax, mm3   ; set new composite out

      and eax, 0ffffffh;
      or  eax, ebx

%%absolutetrans:
	    
%endm

;---------------------------------------------------------------------------
