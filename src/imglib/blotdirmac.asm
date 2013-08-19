;---------------------------------------------------------------------------

%macro BlotLoopSet 1
      mov esi, [sbptr+%$pi];
      mov edi, [sbptr+%$po];
      mov edx, [sbptr+%$hs];
%%LoopTop:
      mov ecx, [sbptr+%$ws];
%%LineLoop:
      lodsd;

	   %1

      stosd;
      dec ecx;
      jz  %%LineDone;
      jmp %%LineLoop
%%LineDone:      
      add edi, [sbptr+%$oo]; 
      add esi, [sbptr+%$oi];
      dec edx
      jz  %%end
      jmp %%LoopTop;
%%end:
%endm

%macro BlotLoop  0-2 
      mov esi, [sbptr+%$pi];
      mov edi, [sbptr+%$po];
      mov edx, [sbptr+%$hs];
%%LoopTop:
      mov ecx, [sbptr+%$ws];
%%LineLoop:
      lodsd;
      or eax, eax;
      jnz %%doshade;
      jmp %%skippixel
%%doshade:

	   %1
	   %2

      stosd;
      dec ecx;
      jz  %%LineDone;
      jmp %%LineLoop
%%skippixel:
      add edi, 4;
      dec ecx;
      jz  %%LineDone
      jmp %%LineLoop;
%%LineDone:      
      add edi, [sbptr+%$oo]; 
      add esi, [sbptr+%$oi];
      dec edx
      jz  %%end
      jmp %%LoopTop;
%%end:
%endm
