
;--------------------------------------------------------------------
;    MACROS DEFINED HERE
; all routines retain original pixel's (eax) alpha
;
; asmshadepixel - destroys ebx, modifies eax
;
; mmxshadestart - sets up color scalar  - compatible with alpha
; mmxshadepixel - modifies eax
;
; asm_multishadepixel - destroyes ebx, modifies eax
;
; MMXMShadeSetup - sets up multishader MMX registers - compatible with alpha
; MMXMultiShade - modifies eax
;--------------------------------------------------------------------


%macro asmshadepixel 0
; input eax == input pixel
      save_ax

		mov al, [sptr+2]
      mov bl, [sbptr+%$color+2];
      mul bl           ; scale highest input with highest scalar
      add al, [sptr+2]
      adc ah, 0
      mov [sptr+2], ah  ; store result back over pixel

      mov al, [sptr+1]  ; get middle byte
      mov bl, [sbptr+%$color+1];
      mul bl           ; scale mid input with mid scalar
      add al, [sptr+1]
      adc ah, 0
      mov [sptr+1], ah  ; store result over pixel

      mov al, [sptr]    ; get low color input
      mov bl, [sbptr+%$color];
      mul bl           ; scale low input with low scalar
      add al, [sptr]
      adc ah, 0
      mov [sptr], ah    ; store low over pixel

      load_ax          ; get original (now shaded) pixel into ax
%endm

%macro mmxshadestart 0
	; set up the color multiplier
	; and a zero register for expansions
   xor eax,eax              ; clear a register (max alpha)
   movd mm0, eax            ; set a 0 mmx register
   movd mm5, [sbptr+%$color]  ; load color scalar
   PUNPCKLBW mm5,mm0        ; mm2(low) IS 0.... high byte alpha on scalar is 256
   paddusw mm5, [mmx_one]   ; scalar is bytes + 1 so 255 = max scale, 0 is 0
%endm

%macro mmxshadepixel 0
   movd mm1, eax     ; store input pixel into mm0
   PUNPCKLBW mm1,mm0 ; mm0 IS 0, expand 0 fill high bytes
   pmullw mm1, mm5   ; muliply pixel with scalar
   psrlw mm1, 8      ; shift right to align for low re-pack
   packuswb mm1,mm0  ; pack bytes
   movd eax, mm1     ; put result back into eax
%endm


%macro asm_multishadepixel 0
	; parameter parametes colorR, colorG, colorB
	; assume that eax is the source color
	; retains alpha of source pixel 
	; generates output pixel in eax
	; esi, edi, ecx, edx remain unchanged
	; destroys ebx
		save_ax      ; save passed color
      save_dx		  ; save the line counter

      mov edx, eax; ; copy input pixel to edx
      ror edx, 16;  ; dl = top of 24 bit color

      mov ebx, [sbptr+%$colorR]; ; get red color scalar    
      mov al, dl;   ; mov high color byte into al
      ror ebx, 16;  ; move high color into bl
      mul bl;       ; sets ax with high byte multiplication (ah = real)
      add al, dl;   ; (multiply is bl+1) but bl+1 at 255 would overflow
      adc ah, 0;    ; blah - need carry 

      shl eax, 16;  ; save high result in heax
      mov al, dl;   ; reload high color byte
      rol ebx, 8;   ; get next scale byte into al
      mul bl		  ; multiply high color with scalar
      add al, dl;   ; (multiply is bl+1) but bl+1 at 255 would overflow
      adc ah, 0;    ; blah - need carry 
      xchg ah, al   ; move most significant low
      rol eax, 8;   ; top of eax is two bytes multipled
      xchg ah, al;  ; swap order (ok this bit was badly coded redundant order)
      shl eax, 16;  ; !!bad code
      mov al, dl;   ; !!bad code
      rol ebx, 8;
      mul bl;
      add al, dl;   ; (multiply is bl+1) but bl+1 at 255 would overflow
      adc ah, 0;    ; blah - need carry 
      shr eax, 8;

      save_ax;     ; save first scalar times first byte

      rol edx, 8;  ; dl = middle of 24 bit color

      mov ebx, [sbptr+%$colorG];    
      mov al, dl;  
      ror ebx, 16;
      mul bl;
      add al, dl;   ; (multiply is bl+1) but bl+1 at 255 would overflow
      adc ah, 0;    ; blah - need carry 

      shl eax, 16;
      mov al, dl;
      rol ebx, 8;
      mul bl;
      add al, dl;   ; (multiply is bl+1) but bl+1 at 255 would overflow
      adc ah, 0;    ; blah - need carry 
      xchg ah, al; ; keep most significant
      rol eax, 8;
      xchg ah, al;
      shl eax, 16;
      mov al, dl;
      rol ebx, 8;
      mul bl;
      add al, dl;   ; (multiply is bl+1) but bl+1 at 255 would overflow
      adc ah, 0;    ; blah - need carry 
      shr eax, 8;
      save_ax;    ; save second scalar times second byte

      rol edx, 8;  ; dl = low of 24 bit color

      mov ebx, [sbptr+%$colorB];    
      mov al, dl;  
      ror ebx, 16;
      mul bl;
      add al, dl;   ; (multiply is bl+1) but bl+1 at 255 would overflow
      adc ah, 0;    ; blah - need carry 

      shl eax, 16;
      mov al, dl;
      rol ebx, 8;
      mul bl;
      add al, dl;   ; (multiply is bl+1) but bl+1 at 255 would overflow
      adc ah, 0;    ; blah - need carry 
      xchg ah, al; ; keep most significant
      rol eax, 8;
      xchg ah, al;
      shl eax, 16;
      mov al, dl;
      rol ebx, 8;
      mul bl;
      add al, dl;   ; (multiply is bl+1) but bl+1 at 255 would overflow
      adc ah, 0;    ; blah - need carry 
      shr eax, 8;

      ; at this point edx contains the original pixel in original alignment
      ; perform adc and test overflow of each byte...


      load_bx;
      load_dx;
      ; at this point
      ;   eax = blue scalar times blue byte
      ;   ebx is green scalar times green byte
      ;   edx is red scalar times red byte
      adc al, bl
      jc %%MaxLow
      adc al, dl
      jnc %%NotMaxLow
      %%MaxLow:
      mov al, 0ffh
      %%NotMaxLow:
      ror eax, 8
      ror ebx, 8
      ror edx, 8
      adc al, bl
      jc %%MaxMid
      adc al, dl
      jnc %%NotMaxMid
      %%MaxMid:
      mov al, 0ffh
      %%NotMaxMid:
      ror eax, 8
      ror ebx, 8
      ror edx, 8
      adc al, bl
      jc %%MaxHigh
      adc al, dl
      jnc %%NotMaxHigh
      %%MaxHigh:
      mov al, 0ffh
      %%NotMaxHigh:
      load_dx				; restore line counter
      mov ah, [sptr + 3] ; save source alpha in out pixel
      ror eax, 16       ; align output pixel for store
      add sptr, 1*ssize        ; trash saved color
      ;result is now in eax ready for store...
%endm

%macro MMXMShadeSetup 0
	; results
	;		mm0 being 0
	;     mm5 blue scalar
	;     mm6 green scalar
	;     mm7 red scalar
      xor eax,eax                         ;
      movd mm0, eax                       ;
      movd mm5, [sbptr+%$colorB]            ;
      PUNPCKLBW mm5,mm0 ; mm0 IS 0....    ;
      paddusw mm5, [mmx_one];             ;
      movd mm6, [sbptr+%$colorG]            ;
      PUNPCKLBW mm6,mm0 ; mm0 IS 0....    ;
      paddusw mm6, [mmx_one];             ;
      movd mm7, [sbptr+%$colorR]            ;
      PUNPCKLBW mm7,mm0 ; mm0 IS 0....    ;
      paddusw mm7, [mmx_one];             ;
		
%endm

mmx_alpha_mask dd 0FF000000h, 00h

%macro MMXMultiShade 0
				; note - since the scalars SHOULD be 0 alpha values
				; then it is safe to assume the the result of this code
				; will be 0 filled
      movd mm1, eax;       ; load color into mm1 
      pand mm1, [mmx_alpha_mask] ; going to mask the alpha in mm1

      movd mm2, eax;       ; load color into mm2
      punpcklbw mm2, mm2   ; spam bytes 0 0 | r r || g g | b b 

      movq mm4, mm2        ; store spam in mm4
      psrlq mm4, 32        ; move upper half to lower half of reger
      punpcklbw mm4, mm4   ; spam bytes 0 0 0 0 | r r r r
      movq mm3, mm2        ; move spam into mm3
      psrlq mm3, 16        ; shift middle to low of mm3
      punpcklbw mm3, mm3   ; spam mm3 r r r r g g g g
      punpcklbw mm2, mm2   ; spam mm2 g g g g b b b b
      punpcklbw mm2, mm0   ; zero pad spam 0 b 0 b 0 b 0 b
      punpcklbw mm3, mm0   ; zero pad spam 0 g 0 g 0 g 0 g
      punpcklbw mm4, mm0   ; zero pad spam 0 r 0 r 0 r 0 r 
      pmullw mm2, mm5      ; mult blue times blue scalar
      pmullw mm3, mm6      ; mult green times green scalar
      pmullw mm4, mm7      ; mult red times red scalar
      psrlw mm2, 8         ; align register - chop low byte 
      psrlw mm3, 8         ; align register - chop low byte 
      psrlw mm4, 8         ; align register - chop low byte 
      packuswb mm2, mm0     ; pack blue value
      packuswb mm3, mm0     ; pack red value
      packuswb mm4, mm0     ; pack green value
      paddusb mm2, mm3      ; add red to blue
      paddusb mm2, mm4      ; add green to blue
      paddusb mm2, mm1     ; add original alpha back into pixel
      movd eax, mm2        ; set total value into out pixel
%endm
