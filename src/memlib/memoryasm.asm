
%include "c32.mac"

%ifdef BCC32
section _TEXT align=4 class=CODE USE32
%else
section .text
%endif

proc _DebugSTOP
   db 0cch
endproc

dllproc LockedExchange
arg %$p
arg %$val
   mov ecx, [ebp+%$p]
   mov eax, [ebp+%$val]
   xchg [ecx], eax
endproc

dllproc MemSet
arg %$p
arg %$val
arg %$size
	push edi
      mov ecx, [ebp + %$size];
      mov eax, [ebp + %$val];
      mov edi, [ebp + %$p];
      cld;
      mov   edx, ecx;
      shr 	ecx, 2;
      rep 	stosd;
      test 	edx, 2
      jz 	.store_one;
      stosw;
.store_one:
      test 	edx,1
      jz 	.store_none;
      stosb;
.store_none:
	pop edi
endproc

dllproc MemCpy
arg %$pTo
arg %$pFrom
arg %$sz
	push edi
	push esi
      mov   ecx, [ebp + %$sz];
      mov   edi, [ebp + %$pTo];
      mov   esi, [ebp + %$pFrom];
      cld;
      mov   edx, ecx;
      shr   ecx, 2;
      rep   movsd;
      test  edx,1;
      jz    .test_2;
      movsb;
.test_2:
      test edx, 2;
      jz .test_end;
      movsw;
.test_end:
	pop esi
	pop edi
endproc


dllproc MemCmp
arg %$p1
arg %$p2
arg %$sz
      push  edi
      push  esi
      mov   ecx, [ebp + %$sz];
      mov   edi, [ebp + %$p1];
      mov   esi, [ebp + %$p2];
      cld
      mov   edx, ecx;
      xor   eax, eax;
      shr   ecx, 2;
      or    ecx, ecx;
      jz    .compare_2
.compare_1:
	lodsd
        add  edi, 4
	sub  eax, [edi-4]
	loopz .compare_1
      jnz .done;
.compare_2:
      test  edx, 1;
      jz   .test_2
      add edi, 1
      lodsb 
      sub al, [edi-1]
      jnz .done;
.test_2:
      test  edx, 2
      jz   .done
      lodsw
      sub ax, [edi]
.done:
      ; if we flop the bytes around, should
      ; end up being a literal mathematical compare
      ; might fail on certain boundary conditions...
      ; but - for now the intent is equal or not,
      ; direction is not nessecarily even important.
      ;xchg ah, al
      ;rol eax, 16
      ;xchg ah, al
      
      pop esi
      pop edi
endproc
