
; do_line( ImageFile *pImage, int x, int y
;				                , int xto, int yto, int d );
; do_lineExV( ImageFile *pImage, int x, int y
;                              , int xto, int yto, int d
;                              , int Plot( ImageFile*, int x, int y, int d ) );

%ifdef BCC32
SECTION _DATA class=DATA ALIGN=4 USE32 
%else
SECTION .data
%endif
%include "ImageFile.asm"
%include "c32.mac"
%include "mmx.asm"
%include "alphamac.asm"

%ifdef __LINUX__
extern asmplot
extern asmplotalpha
extern asmplotalphaMMX
%else
%define asmplot _asmplot
%define asmplotalpha _asmplotalpha
%define asmplotalphaMMX _asmplotalphaMMX
extern _asmplot
extern _asmplotalpha
extern _asmplotalphaMMX
%endif


%ifdef BCC32
SECTION _TEXT class=CODE ALIGN=4 USE32 
%else
SECTION .text ; section of text needed to export code routines (COFF)
%endif


;/* void do_line(BITMAP *bmp, int x1, y1, x2, y2, int d, void (*proc)())
; *  Calculates all the points along a line between x1, y1 and x2, y2, 
; *  calling the supplied function for each one. This will be passed a 
; *  copy of the bmp parameter, the x and y position, and a copy of the 
; *  d parameter (so do_line() can be used with putpixel()).
; */


;----------------------------------------------------------------------

%macro lineloop 1
      mov esi, [sbptr + %$image];
      mov edi, [esi + IMGFILE_image]; ;// don't change the format.....
      xor ecx, ecx
      mov eax, [sbptr + %$x2]   ; eax = x2
      sub eax, [sbptr + %$x1]   ; x2 - x1
      jge %%line_dx_pve_1      ; result >= 0 jump
      neg eax                 ; eax = abs(delx) 
      or  cl, 1               ; mark delx was negative
%%line_dx_pve_1:
      mov ebx, [sbptr + %$y2]   ; ebx = y2 
      sub ebx, [sbptr + %$y1]   ; y2 - y1 
      jge %%line_dy_pve_1      ; result >= 0 jump
      neg ebx                 ; ebx = abs(dely) 
      or  cl, 2               ; mark dely was negative
%%line_dy_pve_1:
		mov [delx], eax       ; store delx for iteration
		mov [dely], ebx       ; store dely for iteration
      cmp ebx, eax          ; dely (op) delx
      jng %%line_x_driven    ; delx >= dely ... jump
      jmp %%line_y_driven    ; else dely > delx ... jump

%%line_x_driven:
	;db 0cch
   test eax, eax             ; is delx 0 ?
   jnz %%line_x_continue     ; if not - continue
   jmp %%line_done_1         ; ok we're done - over and out.
%%line_x_continue:
   mov eax, [sbptr + %$y1]     ; load y1
   mov ebx, [sbptr + %$y2]     ; load y2
   test cl, 1                ; was delx negative when computed? (x2-x1)
   jz %%line_x_noswap_1      ; if x1 < x2 ... jump

	mov edx, [sbptr + %$x2]     ; otherwise move x2 to x1
	mov [sbptr + %$x1], edx     ;  "
   xchg eax, ebx             ; swap y1, y2
   mov [sbptr + %$y1], eax

%%line_x_noswap_1:
	mov dword [step], 1       ; now - after swap compute step direction
	cmp eax, ebx         	  ; compare y1 to y2
	jle %%line_x_incpos       ; y1 <= y2 - skip step inversion
	neg dword [step]          ; step = -1  ...
%%line_x_incpos:
	mov eax, [delx]           ; get x length (max of delx, dely)
	mov [len], eax            ; store as length to do...
	inc dword [len]           ; have to increment cause decrement doesn't carry
	shr eax, 1                ; divide by 2 (average error between ends)
	neg eax                   ; set err to -(delx/2)
	mov [err], eax            ; store error

   push stype [sbptr + %$D]    ; put data item on stack - keep it there.
%%line_x_loop_1:
   push stype [sbptr + %$y1]   ;/* y coord */
   push stype [sbptr + %$x1]   ;/* x coord */
   save_si                  ;/* ImageFile */
   %1                        ;/* call the appropriate routine */
   add sptr, 3*ssize               ; remove x, y, imagefile
   inc dword [sbptr + %$x1]    ; step x1 to next... 
	mov ebx, [err]   			  ; get error
	add ebx, [dely]    		  ; add dely
	ja %%line_x_no_errfix   ; if did not pass or equal zero jump to store
   mov eax, [step]           ; get step compute above
%%line_x_dofix:              
	add [sbptr + %$y1], eax     ; add step to y1 passed to plotting function
	sub ebx, [delx]           ; subtract delx from error
	jnc %%line_x_dofix        ; while above or equal zero loop
%%line_x_no_errfix:
	mov [err], ebx            ; store error back out
   dec dword [len]           ; decrement length counter
   jnz %%line_x_loop_1       ; while len != 0 jump (this should be jnc)
   add sptr, 1*ssize                ; remove data item from stack
   jmp %%line_done_1         ; jump to done.

   ;/* y-driven fixed point line draw */

%%line_y_driven:
	;db 0cch
		; if I got here - then dely is not zero...cause delx would have
		; been been longer - or would have also been zero
		; then delx would have shorted out in the loop above
   ;test ebx, ebx                 
   ;jnz %%line_y_continue
   ;jmp %%line_done_1
%%line_y_continue:
   mov eax, [sbptr + %$x1]
   mov ebx, [sbptr + %$x2]
   test cl, 2
   jz %%line_y_noswap_1

	mov edx, [sbptr + %$y2]
	mov [sbptr + %$y1], edx
   xchg eax, ebx
   mov [sbptr + %$x1], eax

%%line_y_noswap_1:
	mov dword [step], 1
	cmp eax, ebx         		; compare x1 to x2
	jle %%line_y_incpos
	neg dword [step]
%%line_y_incpos:
	mov eax, [dely]
	mov [len], eax            ; store as length to do...
	inc dword [len]           ; have to increment cause decrement doesn't carry
	shr eax, 1
	neg eax
	mov [err], eax

   push stype [sbptr + %$D]     ;/* data item */
%%line_y_loop_1:
   push stype [sbptr + %$y1]    ;/* y coord */
   push stype [sbptr + %$x1]    ;/* x coord */
   save_si                   ;/* bitmap */
   %1                        	;/* call the draw routine */
   add sptr, 3*ssize
   inc dword [sbptr + %$y1]
	mov ebx, [err]   				; get err
	add ebx, [delx]    			; subtract dely
	ja  %%line_y_no_errfix
   mov eax, [step]
%%line_y_dofix:
	add [sbptr + %$x1], eax
	sub ebx, [dely]
	jnc %%line_y_dofix
%%line_y_no_errfix:
	mov [err], ebx
   dec dword [len]
   jnz %%line_y_loop_1                   ;/* more? */
   add sptr, 1*ssize
                               ; remove data item from stack
%%line_done_1:

%endm

;----------------------------------------------------------------------

proc do_lineasm
arg %$image
arg %$x1
arg %$y1
arg %$x2
arg %$y2
arg %$D 
%define delx sbptr-4
%define dely sbptr-8
%define err  sbptr-12
%define step sbptr-16
%define len  sbptr-20
	sub sptr, 20 ; sbptr-4  - delx 
					; sbptr-8  - dely
					; sbptr-12 - err
					; sbptr-16 - step
					; sbptr-20 - len
	save_si
	save_di
	save_bx
		lineloop call asmplot
	load_bx
	load_di
	load_si
endp

;----------------------------------------------------------------------

proc do_lineAlphaasm
arg %$image
arg %$x1
arg %$y1
arg %$x2
arg %$y2
arg %$D 
	sub sptr, 20
	save_si
	save_di
	save_bx
		lineloop call asmplotalpha
	load_bx
	load_di
	load_si
endp

;----------------------------------------------------------------------

proc do_lineAlphaMMX
arg %$image
arg %$x1
arg %$y1
arg %$x2
arg %$y2
arg %$D 
	sub sptr, 20
	save_si
	save_di
	save_bx
		lineloop call asmplotalphaMMX
	load_bx
	load_di
	load_si
endp

;----------------------------------------------------------------------

proc do_lineExVasm
arg %$image
arg %$x1
arg %$y1
arg %$x2
arg %$y2
arg %$D 
arg %$func
	sub sptr, 20
	save_si
	save_di
	save_bx
		lineloop call [sbptr + %$func]
	load_bx
	load_di
	load_si
endp

;----------------------------------------------------------------------

%macro vlinesetup 0
	cmp dword [sbptr + %$x], 0
	jge .checkrange
	jmp .done
.checkrange:
	mov esi, [sbptr + %$image]
	mov eax, [esi + IMGFILE_w]
        cmp dword [esi + IMGFILE_image], 0
        jnz .image_okay
        jmp .done
.image_okay:
	cmp [sbptr + %$x], eax
	jl  .rangeok
	jmp .done
.rangeok:
	mov eax, [sbptr + %$yto]
	mov ebx, [sbptr + %$yfrom]
	cmp eax, ebx
	jge .noswap
	xchg eax, ebx
.noswap:
	cmp eax, 0
	jge .maxokay
	jmp .done
.maxokay:
	mov edx, [esi + IMGFILE_h]
	dec edx

	cmp ebx, edx
	jle .minokay
	jmp .done
.minokay:
   cmp ebx, 0
   jge .nominfix
	mov ebx, 0
.nominfix:
	cmp eax, edx
	jle .nomaxfix
	mov eax, edx
.nomaxfix:

	mov ecx, eax
	sub ecx, ebx
   ; zero length lines need to not be drawn
   jnb .okay
   jmp .done
   .okay:
	inc ecx

%ifdef INVERT_IMAGE
	sub edx, eax
%else
	mov edx, ebx
%endif
	mov eax, [esi + IMGFILE_pwidth]
	mov ebx, eax
	mul edx
	add eax, [sbptr + %$x]
	shl eax, 2
	mov edi, eax
	add edi, [esi + IMGFILE_image]
	shl ebx, 2
	mov edx, ebx
%endm

proc do_vlineasm
arg %$image
arg %$x
arg %$yfrom
arg %$yto
arg %$color
	save_bx
	save_si
	save_di
	vlinesetup
	mov eax, [sbptr + %$color]
.looptop:
	mov [edi], eax
	add edi, edx
	loop .looptop
.done:
	load_di
	load_si
	load_bx
endp

proc do_vlineAlphaasm
arg %$image
arg %$x
arg %$yfrom
arg %$yto
arg %$color
	save_bx
	save_si
	save_di
	vlinesetup
.looptop:
	asmalpha_solid

	add edi, edx
	dec ecx
	jz .done
	jmp .looptop
.done:
	load_di
	load_si
	load_bx
endp

proc do_vlineAlphaMMX
arg %$image
arg %$x
arg %$yfrom
arg %$yto
arg %$color
	save_bx
	save_si
	save_di
	vlinesetup
	mmxalpha_solid_init

.looptop:
	mmxalpha_solid

	add edi, edx
	loop .looptop
	emms
.done:
	load_di
	load_si
	load_bx
endp


%macro hlinesetup 0
	cmp dword [sbptr + %$y], 0
	jge .checkrange
	jmp .done
.checkrange:
	mov esi, [sbptr + %$image]
	mov eax, [esi + IMGFILE_h]
        cmp dword [esi + IMGFILE_image], 0
        jnz .image_okay
        jmp .done
.image_okay:
	cmp [sbptr + %$y], eax
	jl  .rangeok
	jmp .done
.rangeok:
	mov eax, [sbptr + %$xto]
	mov ebx, [sbptr + %$xfrom]
	cmp eax, ebx
	jge .noswap
	xchg eax, ebx
.noswap:
	cmp eax, 0
	jge .maxokay
	jmp .done
.maxokay:
	mov edx, [esi + IMGFILE_w]
	dec edx

	cmp ebx, edx
	jle .minokay
	jmp .done
.minokay:
   cmp ebx, 0
   jge .nominfix
	mov ebx, 0
.nominfix:
	cmp eax, edx
	jle .nomaxfix
	mov eax, edx
.nomaxfix:

	mov ecx, eax
	sub ecx, ebx
   jnb .okay
   jmp .done
   .okay:
	inc ecx

%ifdef INVERT_IMAGE
	mov edx, [esi + IMGFILE_h]
	dec edx
	sub edx, [sbptr + %$y]
%else
	mov edx, [sbptr + %$y]
%endif
	mov eax, [esi + IMGFILE_pwidth]
	mul edx
	add eax, ebx
	shl eax, 2
	mov edi, eax
	add edi, [esi + IMGFILE_image]

	mov edx, 4
%endm

proc do_hlineasm
arg %$image
arg %$y
arg %$xfrom
arg %$xto
arg %$color
	save_bx
	save_si
	save_di
	hlinesetup
	mov eax, [sbptr + %$color]
.looptop:
	mov [edi], eax
	add edi, edx
	loop .looptop
.done:
	load_di
	load_si
	load_bx
endp

proc do_hlineAlphaasm
arg %$image
arg %$y
arg %$xfrom
arg %$xto
arg %$color
	save_bx
	save_si
	save_di
	hlinesetup
.looptop:

	asmalpha_solid

	add edi, edx
	dec ecx
	jz .done
	jmp .looptop
.done:
	load_di
	load_si
	load_bx
endp

proc do_hlineAlphaMMX
arg %$image
arg %$y
arg %$xfrom
arg %$xto
arg %$color
	save_bx
	save_si
	save_di
	hlinesetup
	mmxalpha_solid_init
.looptop:
	mmxalpha_solid

	add edi, edx
	loop .looptop
	emms
.done:
	load_di
	load_si
	load_bx
endp
