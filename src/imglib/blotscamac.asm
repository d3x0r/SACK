

%macro StandardArgs 0
arg %$po
arg %$pi
arg %$i_errx
arg %$i_erry
arg %$wd
arg %$hd
arg %$dwd
arg %$dhd
arg %$dws
arg %$dhs
arg %$oo
arg %$srcwidth
%endm

%macro ScaleLoop 0-2  .junklabel1:, .junklabel2:

	mov edi, [sbptr + %$po]
	; subl	$16,%sptr
	mov  edx, [sbptr + %$hd]    ; set line counter to 0 
	mov  ecx, [sbptr + %$i_erry]; init ecx as an deltaerr counter lines
%%LineLoopBegin:	
	
	mov eax, [sbptr + %$wd]     ; set column counter to columns to step
	mov ebx, [sbptr + %$i_errx] ; init ebx as deltaerr counter columns
	mov esi, [sbptr + %$pi]     ; get source pointer
%%ColLoopBegin:
   save_ax                  ; save column counter
   save_bx                  ; save column deltaerr
	mov eax, [esi]            ; get input pixel
	%1                        ; do loop core step 1
	%2                        ; do loop core step 2
	stosd                     ; store eax and inc edi
	load_bx                   ; restore deltaerr
	load_ax                   ; restore column counter
	add ebx, [sbptr + %$dws]    ; add widthsrc to err counter
	ja  %%ColErrDone          ; if err counter (not carry, not zero)
%%ColErrContinue:
	add esi, 4                ; increment source pointer to next pixel
	sub ebx, [sbptr + %$dwd]    ; subtract widthdest from err counter
	jnc %%ColErrContinue      ; while error above 0 (not carry)
%%ColErrDone:
	dec eax                   ; step x position counter
	jz  %%StepLineInfo        ; ignore delta fixup if done already
	jmp %%ColLoopBegin	 

%%StepLineInfo:
	add edi, [sbptr + %$oo]     ; fixup dest pointer - next line
	add ecx, [sbptr + %$dhs]    ; add heightsrc to err counter
	ja  %%LineErrDone         ; if (not zero, not carry) no overflow...
%%LineErrContinue:
	mov esi, [sbptr + %$pi]     ; get source pointer
	add esi, [sbptr + %$srcwidth]; fixup source pointer to next input
	mov [sbptr + %$pi], esi     ; save source pointer
	sub ecx, [sbptr + %$dhd]    ; subtract heightdest from error counter
	jnc %%LineErrContinue     ; while err counter above 0
%%LineErrDone:
	dec edx                   ; step line counter on destination
	jz  %%EndLineLoop
	jmp  %%LineLoopBegin

%%EndLineLoop:
	
%endm

%macro ScaleLoopSkip0 0-2  .junklabel1:, .junklabel2:

	mov edi, [sbptr + %$po]
	; subl	$16,%sptr
	mov  edx, [sbptr + %$hd]    ; set line counter to 0 
	mov  ecx, [sbptr + %$i_erry]; init ecx as an deltaerr counter lines

%%LineLoopBegin:	
	
	mov eax, [sbptr + %$wd]     ; set column counter to columns to step
	mov ebx, [sbptr + %$i_errx] ; init ebx as deltaerr counter columns
	mov esi, [sbptr + %$pi]     ; get source pointer
%%ColLoopBegin:
   save_ax                  ; save column counter
   save_bx                  ; save column deltaerr
	mov eax, [esi]            ; get input pixel
	or eax, eax               ; compare 0 pixel 
	jnz %%DoCore              ; if not absolute 0 do normal process
	add edi, 4                ; else increment destination
	jmp %%SkipCore            ; skip typical core processing
%%DoCore:
	%1                        ; do loop core step 1
	%2                        ; do loop core step 2
	stosd                     ; store eax and inc edi
%%SkipCore:
	load_bx                   ; restore deltaerr
	load_ax                   ; restore column counter
	add ebx, [sbptr + %$dws]    ; add widthsrc to err counter
	ja  %%ColErrDone          ; if err counter (not carry, not zero)
%%ColErrContinue:
	add esi, 4                ; increment source pointer to next pixel
	sub ebx, [sbptr + %$dwd]    ; subtract widthdest from err counter
	jnc %%ColErrContinue      ; while error above 0 (not carry)
%%ColErrDone:
	dec eax                   ; step x position counter
	jz  %%StepLineInfo        ; ignore delta fixup if done already
	jmp %%ColLoopBegin	 

%%StepLineInfo:
	add edi, [sbptr + %$oo]     ; fixup dest pointer - next line
	add ecx, [sbptr + %$dhs]    ; add heightsrc to err counter
	ja  %%LineErrDone         ; if (not zero, not carry) no overflow...
%%LineErrContinue:
	mov esi, [sbptr + %$pi]     ; get source pointer
	add esi, [sbptr + %$srcwidth]; fixup source pointer to next input
	mov [sbptr + %$pi], esi     ; save source pointer
	sub ecx, [sbptr + %$dhd]    ; subtract heightdest from error counter
	jnc %%LineErrContinue     ; while err counter above 0
%%LineErrDone:
	dec edx                   ; step line counter on destination
	jz  %%EndLineLoop
	jmp  %%LineLoopBegin

%%EndLineLoop:
	
%endm

