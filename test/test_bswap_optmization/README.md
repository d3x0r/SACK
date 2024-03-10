
test.cc is a copy of test.c

# visual studio results

test.c, test.cc, with Release build, gets bswap used for 1_3, 1_4, 2_3 and 2_3(arr)

1_3 is a macro from https://developercommunity.visualstudio.com/t/MSVC-Not-using-bswap-when-possible/917784

1_4 is supposedly a visual studio intrinsic

Debug builds do not respect optize flags... 

2_3(arr) is a short routine unoptimized

```

; 76   :     uint32_t b, c;                                                                                          ; 31   :     uint32_t b, c;                        
; 77   :     // C - bswap (only c=b)   ; 66   :     uint32_t b, c;            ; 56   :     uint32_t b, c;            ; 32   :     // C - bswap                        ; 20   :     uint32_t b, c;            ; 9    :     uint32_t b, c;           
; 78   :     NTOHL2_3( b, a );         ; 67   :     NTOHL2_2( b, a );         ; 57   :     NTOHL2_1( b, a );         ; 33   :     b = NTOHL3( a );                    ; 21   :     b = NTOHL2( a );          ; 10   :     b = NTOHL1( a );         
                                                                                                                                                                                                                                                   
   mov   eax, DWORD PTR a$[rsp]           mov   eax, 4                           mov   eax, 1                           mov   eax, DWORD PTR a$[rsp]                     mov   eax, 1                            mov   eax, 1                       
   mov   DWORD PTR bb$6[rsp], eax         imul   rax, rax, 0                     imul   rax, rax, 3                     and   eax, 255            ; 000000ffH            imul   rax, rax, 0                      imul   rax, rax, 0                  
   mov   eax, 1                           mov   ecx, DWORD PTR a$[rsp]           mov   ecx, 1                           shl   eax, 24                                    movzx   eax, BYTE PTR a$[rsp+rax]       movzx   eax, BYTE PTR a$[rsp+rax]    
   imul   rax, rax, 3                     mov   DWORD PTR bb$8[rsp+rax], ecx     imul   rcx, rcx, 0                     mov   ecx, DWORD PTR a$[rsp]                     shl   eax, 8                            shl   eax, 24                      
   movzx   ecx, BYTE PTR bb$6[rsp]        mov   eax, 1                           movzx   eax, BYTE PTR a$[rsp+rax]      and   ecx, 65280            ; 0000ff00H          mov   ecx, 1                            mov   ecx, 1                       
   mov   BYTE PTR ab$5[rsp+rax], cl       imul   rax, rax, 3                     mov   BYTE PTR b$[rsp+rcx], al         shl   ecx, 8                                     imul   rcx, rcx, 1                      imul   rcx, rcx, 1                  
   mov   eax, DWORD PTR bb$6[rsp]         mov   ecx, 1                           mov   eax, 1                           or   eax, ecx                                    movzx   ecx, BYTE PTR a$[rsp+rcx]       movzx   ecx, BYTE PTR a$[rsp+rcx]    
   shr   eax, 8                           imul   rcx, rcx, 0                     imul   rax, rax, 2                     mov   ecx, DWORD PTR a$[rsp]                     or   eax, ecx                           shl   ecx, 16                      
   mov   ecx, 1                           movzx   eax, BYTE PTR bb$8[rsp+rax]    mov   ecx, 1                           and   ecx, 16711680            ; 00ff0000H       shl   eax, 8                            or   eax, ecx                        
   imul   rcx, rcx, 2                     mov   BYTE PTR ab$7[rsp+rcx], al       imul   rcx, rcx, 1                     shr   ecx, 8                                     mov   ecx, 1                            mov   ecx, 1                       
   mov   BYTE PTR ab$5[rsp+rcx], al       mov   eax, 1                           movzx   eax, BYTE PTR a$[rsp+rax]      or   eax, ecx                                    imul   rcx, rcx, 2                      imul   rcx, rcx, 2                  
   mov   eax, DWORD PTR bb$6[rsp]         imul   rax, rax, 2                     mov   BYTE PTR b$[rsp+rcx], al         mov   ecx, DWORD PTR a$[rsp]                     movzx   ecx, BYTE PTR a$[rsp+rcx]       movzx   ecx, BYTE PTR a$[rsp+rcx]    
   shr   eax, 16                          mov   ecx, 1                           mov   eax, 1                           and   ecx, -16777216            ; ff000000H      or   eax, ecx                           shl   ecx, 8                       
   mov   ecx, 1                           imul   rcx, rcx, 1                     imul   rax, rax, 1                     shr   ecx, 24                                    shl   eax, 8                            or   eax, ecx                        
   imul   rcx, rcx, 1                     movzx   eax, BYTE PTR bb$8[rsp+rax]    mov   ecx, 1                           or   eax, ecx                                    mov   ecx, 1                            mov   ecx, 1                       
   mov   BYTE PTR ab$5[rsp+rcx], al       mov   BYTE PTR ab$7[rsp+rcx], al       imul   rcx, rcx, 2                     mov   DWORD PTR b$[rsp], eax                     imul   rcx, rcx, 3                      imul   rcx, rcx, 3                  
   mov   eax, DWORD PTR bb$6[rsp]         mov   eax, 1                           movzx   eax, BYTE PTR a$[rsp+rax]                                                       movzx   ecx, BYTE PTR a$[rsp+rcx]       movzx   ecx, BYTE PTR a$[rsp+rcx]    
   shr   eax, 24                          imul   rax, rax, 1                     mov   BYTE PTR b$[rsp+rcx], al                                                          or   eax, ecx                           or   eax, ecx                        
   mov   ecx, 1                           mov   ecx, 1                           mov   eax, 1                                                                            mov   DWORD PTR b$[rsp], eax            mov   DWORD PTR b$[rsp], eax       
   imul   rcx, rcx, 0                     imul   rcx, rcx, 2                     imul   rax, rax, 0                
   mov   BYTE PTR ab$5[rsp+rcx], al       movzx   eax, BYTE PTR bb$8[rsp+rax]    mov   ecx, 1                     
   mov   eax, 4                           mov   BYTE PTR ab$7[rsp+rcx], al       imul   rcx, rcx, 3                
   imul   rax, rax, 0                     mov   eax, 1                           movzx   eax, BYTE PTR a$[rsp+rax]  
   mov   eax, DWORD PTR ab$5[rsp+rax]     imul   rax, rax, 0                     mov   BYTE PTR b$[rsp+rcx], al   
   mov   DWORD PTR b$[rsp], eax           mov   ecx, 1                        
   xor   eax, eax                         imul   rcx, rcx, 3                   
   test   eax, eax                        movzx   eax, BYTE PTR bb$8[rsp+rax]   
   jne   SHORT $LN4@test2_3               mov   BYTE PTR ab$7[rsp+rcx], al    
$LN7@test2_3:                             mov   eax, 4                        
                                          imul   rax, rax, 0                   
                                          mov   eax, DWORD PTR ab$7[rsp+rax]  
                                          mov   DWORD PTR b$[rsp], eax        
                                          xor   eax, eax                      
                                          test   eax, eax                      
                                          jne   $LN4@test2_2                  
                                       $LN7@test2_2:                          
                                                                              



```

```
                                                                                                            ; 80   :     uint32_t b, c;                           
; 56   :     uint32_t b, c;                           ; 68   :     uint32_t b, c;                           ; 81   :     // C - bswap                             
; 57   :     b = NTOHL1( ((uint32_t*)(a+ofs))[0] );   ; 69   :     b = NTOHL2( ((uint32_t*)(a+ofs))[0] );   ; 82   :     b = NTOHL3( ((uint32_t*)(a+ofs))[0] );   
                                                                                                                                                                  
   movsxd   rax, DWORD PTR ofs$[rsp]                     movsxd   rax, DWORD PTR ofs$[rsp]                     movsxd   rax, DWORD PTR ofs$[rsp]                  
   lea   rax, QWORD PTR a$[rsp+rax]                      lea   rax, QWORD PTR a$[rsp+rax]                      lea   rax, QWORD PTR a$[rsp+rax]                   
   mov   ecx, 4                                          mov   ecx, 4                                          mov   ecx, 4                                       
   imul   rcx, rcx, 0                                     imul   rcx, rcx, 0                                     imul   rcx, rcx, 0                                  
   add   rax, rcx                                        add   rax, rcx                                        mov   eax, DWORD PTR [rax+rcx]                     
   mov   ecx, 1                                          mov   ecx, 1                                          and   eax, 255            ; 000000ffH                
   imul   rcx, rcx, 0                                     imul   rcx, rcx, 0                                     shl   eax, 24                                      
   movzx   eax, BYTE PTR [rax+rcx]                         movzx   eax, BYTE PTR [rax+rcx]                         movsxd   rcx, DWORD PTR ofs$[rsp]                  
   shl   eax, 24                                         shl   eax, 8                                          lea   rcx, QWORD PTR a$[rsp+rcx]                   
   movsxd   rcx, DWORD PTR ofs$[rsp]                     movsxd   rcx, DWORD PTR ofs$[rsp]                     mov   edx, 4                                       
   lea   rcx, QWORD PTR a$[rsp+rcx]                      lea   rcx, QWORD PTR a$[rsp+rcx]                      imul   rdx, rdx, 0                                  
   mov   edx, 4                                          mov   edx, 4                                          mov   ecx, DWORD PTR [rcx+rdx]                     
   imul   rdx, rdx, 0                                     imul   rdx, rdx, 0                                     and   ecx, 65280            ; 0000ff00H             
   add   rcx, rdx                                        add   rcx, rdx                                        shl   ecx, 8                                       
   mov   edx, 1                                          mov   edx, 1                                          or   eax, ecx                                        
   imul   rdx, rdx, 1                                     imul   rdx, rdx, 1                                     movsxd   rcx, DWORD PTR ofs$[rsp]                  
   movzx   ecx, BYTE PTR [rcx+rdx]                         movzx   ecx, BYTE PTR [rcx+rdx]                         lea   rcx, QWORD PTR a$[rsp+rcx]                   
   shl   ecx, 16                                         or   eax, ecx                                           mov   edx, 4                                       
   or   eax, ecx                                           shl   eax, 8                                          imul   rdx, rdx, 0                                  
   movsxd   rcx, DWORD PTR ofs$[rsp]                     movsxd   rcx, DWORD PTR ofs$[rsp]                     mov   ecx, DWORD PTR [rcx+rdx]                     
   lea   rcx, QWORD PTR a$[rsp+rcx]                      lea   rcx, QWORD PTR a$[rsp+rcx]                      and   ecx, 16711680            ; 00ff0000H          
   mov   edx, 4                                          mov   edx, 4                                          shr   ecx, 8                                       
   imul   rdx, rdx, 0                                     imul   rdx, rdx, 0                                     or   eax, ecx                                        
   add   rcx, rdx                                        add   rcx, rdx                                        movsxd   rcx, DWORD PTR ofs$[rsp]                  
   mov   edx, 1                                          mov   edx, 1                                          lea   rcx, QWORD PTR a$[rsp+rcx]                   
   imul   rdx, rdx, 2                                     imul   rdx, rdx, 2                                     mov   edx, 4                                       
   movzx   ecx, BYTE PTR [rcx+rdx]                         movzx   ecx, BYTE PTR [rcx+rdx]                         imul   rdx, rdx, 0                                  
   shl   ecx, 8                                          or   eax, ecx                                           mov   ecx, DWORD PTR [rcx+rdx]                     
   or   eax, ecx                                           shl   eax, 8                                          and   ecx, -16777216            ; ff000000H          
   movsxd   rcx, DWORD PTR ofs$[rsp]                     movsxd   rcx, DWORD PTR ofs$[rsp]                     shr   ecx, 24                                      
   lea   rcx, QWORD PTR a$[rsp+rcx]                      lea   rcx, QWORD PTR a$[rsp+rcx]                      or   eax, ecx                                        
   mov   edx, 4                                          mov   edx, 4                                          mov   DWORD PTR b$[rsp], eax                       
   imul   rdx, rdx, 0                                     imul   rdx, rdx, 0                                
   add   rcx, rdx                                        add   rcx, rdx                                   
   mov   edx, 1                                          mov   edx, 1                                     
   imul   rdx, rdx, 3                                     imul   rdx, rdx, 3                                
   movzx   ecx, BYTE PTR [rcx+rdx]                         movzx   ecx, BYTE PTR [rcx+rdx]                    
   or   eax, ecx                                           or   eax, ecx                                      
   mov   DWORD PTR b$[rsp], eax                          mov   DWORD PTR b$[rsp], eax                     


```


```

; 163  :     uint32_t b, c;                           ; 174  :     uint32_t b, c;                               ; 185  :     uint32_t b, c;                             ; 197  :     uint32_t b, c;                           ; 208  :     uint32_t b, c;                           
; 164  :     NTOHL2_1( b, ((uint32_t*)(a+ofs))[0]);   ; 175  :     NTOHL2_2( b, ((uint32_t*)(a+ofs))[0]);       ; 186  :     NTOHL2_3( b, ((uint32_t*)(a+ofs))[0]);     ; 198  :     NTOHL2_4( b, ((uint32_t*)(a+ofs))[0]);   ; 209  :     NTOHL2_5( b, ((uint32_t*)(a+ofs))[0]);  
                                                                                                                                                                                                                                                                                   
   movsxd   rax, DWORD PTR ofs$[rsp]                     movsxd   rax, DWORD PTR ofs$[rsp]                         movsxd   rax, DWORD PTR ofs$[rsp]                       lea   rax, QWORD PTR b$[rsp+3]                         lea   rax, QWORD PTR b$[rsp+3]                    
   lea   rax, QWORD PTR a$[rsp+rax]                      lea   rax, QWORD PTR a$[rsp+rax]                          lea   rax, QWORD PTR a$[rsp+rax]                        mov   QWORD PTR ab$6[rsp], rax                         mov   QWORD PTR ab$6[rsp], rax                    
   mov   ecx, 4                                          mov   ecx, 4                                              mov   ecx, 4                                            movsxd   rax, DWORD PTR ofs$[rsp]                      movsxd   rax, DWORD PTR ofs$[rsp]                 
   imul   rcx, rcx, 0                                     imul   rcx, rcx, 0                                         imul   rcx, rcx, 0                                    lea   rax, QWORD PTR a$[rsp+rax]                          lea   rax, QWORD PTR a$[rsp+rax]                  
   add   rax, rcx                                        mov   edx, 4                                              mov   eax, DWORD PTR [rax+rcx]                          mov   ecx, 4                                           mov   ecx, 4                                      
   mov   ecx, 1                                          imul   rdx, rdx, 0                                         mov   DWORD PTR bb$7[rsp], eax                         imul   rcx, rcx, 0                                       imul   rcx, rcx, 0                                 
   imul   rcx, rcx, 3                                     mov   eax, DWORD PTR [rax+rcx]                            mov   eax, 1                                           add   rax, rcx                                          add   rax, rcx                                    
   mov   edx, 1                                          mov   DWORD PTR bb$9[rsp+rdx], eax                        imul   rax, rax, 3                                      mov   QWORD PTR bb$7[rsp], rax                          mov   QWORD PTR bb$7[rsp], rax                    
   imul   rdx, rdx, 0                                     mov   eax, 1                                              movzx   ecx, BYTE PTR bb$7[rsp]                        mov   eax, 1                                              mov   eax, 1                                      
   movzx   eax, BYTE PTR [rax+rcx]                         imul   rax, rax, 3                                         mov   BYTE PTR ab$6[rsp+rax], cl                     imul   rax, rax, 0                                         imul   rax, rax, 0                                 
   mov   BYTE PTR b$[rsp+rdx], al                        mov   ecx, 1                                              mov   eax, DWORD PTR bb$7[rsp]                          mov   ecx, 1                                           mov   ecx, 1                                      
   movsxd   rax, DWORD PTR ofs$[rsp]                     imul   rcx, rcx, 0                                         shr   eax, 8                                           imul   rcx, rcx, 0                                       imul   rcx, rcx, 0                                 
   lea   rax, QWORD PTR a$[rsp+rax]                      movzx   eax, BYTE PTR bb$9[rsp+rax]                         mov   ecx, 1                                          mov   rdx, QWORD PTR ab$6[rsp]                           mov   rdx, QWORD PTR ab$6[rsp]                    
   mov   ecx, 4                                          mov   BYTE PTR ab$8[rsp+rcx], al                          imul   rcx, rcx, 2                                      mov   r8, QWORD PTR bb$7[rsp]                           mov   r8, QWORD PTR bb$7[rsp]                     
   imul   rcx, rcx, 0                                     mov   eax, 1                                              mov   BYTE PTR ab$6[rsp+rcx], al                       movzx   eax, BYTE PTR [r8+rax]                            movzx   eax, BYTE PTR [r8+rax]                      
   add   rax, rcx                                        imul   rax, rax, 2                                         mov   eax, DWORD PTR bb$7[rsp]                         mov   BYTE PTR [rdx+rcx], al                            mov   BYTE PTR [rdx+rcx], al                      
   mov   ecx, 1                                          mov   ecx, 1                                              shr   eax, 16                                           mov   rax, QWORD PTR ab$6[rsp]                         mov   rax, QWORD PTR ab$6[rsp]                    
   imul   rcx, rcx, 2                                     imul   rcx, rcx, 1                                         mov   ecx, 1                                          dec   rax                                                dec   rax                                         
   mov   edx, 1                                          movzx   eax, BYTE PTR bb$9[rsp+rax]                         imul   rcx, rcx, 1                                    mov   QWORD PTR ab$6[rsp], rax                            mov   QWORD PTR ab$6[rsp], rax                    
   imul   rdx, rdx, 1                                     mov   BYTE PTR ab$8[rsp+rcx], al                          mov   BYTE PTR ab$6[rsp+rcx], al                       mov   rax, QWORD PTR bb$7[rsp]                          mov   rax, QWORD PTR bb$7[rsp]                    
   movzx   eax, BYTE PTR [rax+rcx]                         mov   eax, 1                                              mov   eax, DWORD PTR bb$7[rsp]                        inc   rax                                                inc   rax                                         
   mov   BYTE PTR b$[rsp+rdx], al                        imul   rax, rax, 1                                         shr   eax, 24                                          mov   QWORD PTR bb$7[rsp], rax                          mov   QWORD PTR bb$7[rsp], rax                    
   movsxd   rax, DWORD PTR ofs$[rsp]                     mov   ecx, 1                                              mov   ecx, 1                                            mov   eax, 1                                           mov   eax, 1                                      
   lea   rax, QWORD PTR a$[rsp+rax]                      imul   rcx, rcx, 2                                         imul   rcx, rcx, 0                                     imul   rax, rax, 0                                        imul   rax, rax, 0                                 
   mov   ecx, 4                                          movzx   eax, BYTE PTR bb$9[rsp+rax]                         mov   BYTE PTR ab$6[rsp+rcx], al                      mov   ecx, 1                                             mov   ecx, 1                                      
   imul   rcx, rcx, 0                                     mov   BYTE PTR ab$8[rsp+rcx], al                          mov   eax, 4                                           imul   rcx, rcx, 0                                       imul   rcx, rcx, 0                                 
   add   rax, rcx                                        mov   eax, 1                                              imul   rax, rax, 0                                      mov   rdx, QWORD PTR ab$6[rsp]                          mov   rdx, QWORD PTR ab$6[rsp]                    
   mov   ecx, 1                                          imul   rax, rax, 0                                         mov   eax, DWORD PTR ab$6[rsp+rax]                     mov   r8, QWORD PTR bb$7[rsp]                           mov   r8, QWORD PTR bb$7[rsp]                     
   imul   rcx, rcx, 1                                     mov   ecx, 1                                              mov   DWORD PTR b$[rsp], eax                           movzx   eax, BYTE PTR [r8+rax]                            movzx   eax, BYTE PTR [r8+rax]                      
   mov   edx, 1                                          imul   rcx, rcx, 3                                                                                                mov   BYTE PTR [rdx+rcx], al                           mov   BYTE PTR [rdx+rcx], al                      
   imul   rdx, rdx, 2                                     movzx   eax, BYTE PTR bb$9[rsp+rax]                                                                              mov   rax, QWORD PTR ab$6[rsp]                           mov   rax, QWORD PTR ab$6[rsp]                    
   movzx   eax, BYTE PTR [rax+rcx]                         mov   BYTE PTR ab$8[rsp+rcx], al                                                                                dec   rax                                               dec   rax                                         
   mov   BYTE PTR b$[rsp+rdx], al                        mov   eax, 4                                                                                                      mov   QWORD PTR ab$6[rsp], rax                        mov   QWORD PTR ab$6[rsp], rax                    
   movsxd   rax, DWORD PTR ofs$[rsp]                     imul   rax, rax, 0                                                                                                mov   rax, QWORD PTR bb$7[rsp]                         mov   rax, QWORD PTR bb$7[rsp]                    
   lea   rax, QWORD PTR a$[rsp+rax]                      mov   eax, DWORD PTR ab$8[rsp+rax]                                                                                inc   rax                                             inc   rax                                         
   mov   ecx, 4                                          mov   DWORD PTR b$[rsp], eax                                                                                      mov   QWORD PTR bb$7[rsp], rax                        mov   QWORD PTR bb$7[rsp], rax                    
   imul   rcx, rcx, 0                                     xor   eax, eax                                                                                                   mov   eax, 1                                           mov   eax, 1                                      
   add   rax, rcx                                                                                                                                                          imul   rax, rax, 0                                      imul   rax, rax, 0                                 
   mov   ecx, 1                                                                                                                                                            mov   ecx, 1                                           mov   ecx, 1                                      
   imul   rcx, rcx, 0                                                                                                                                                      imul   rcx, rcx, 0                                     imul   rcx, rcx, 0                                 
   mov   edx, 1                                                                                                                                                            mov   rdx, QWORD PTR ab$6[rsp]                        mov   rdx, QWORD PTR ab$6[rsp]                    
   imul   rdx, rdx, 3                                                                                                                                                      mov   r8, QWORD PTR bb$7[rsp]                          mov   r8, QWORD PTR bb$7[rsp]                     
   movzx   eax, BYTE PTR [rax+rcx]                                                                                                                                         movzx   eax, BYTE PTR [r8+rax]                            movzx   eax, BYTE PTR [r8+rax]                      
   mov   BYTE PTR b$[rsp+rdx], al                                                                                                                                          mov   BYTE PTR [rdx+rcx], al                          mov   BYTE PTR [rdx+rcx], al                      
                                                                                                                                                                           mov   rax, QWORD PTR ab$6[rsp]                        mov   rax, QWORD PTR ab$6[rsp]                    
                                                                                                                                                                           dec   rax                                             dec   rax                                         
                                                                                                                                                                           mov   QWORD PTR ab$6[rsp], rax                        mov   QWORD PTR ab$6[rsp], rax                    
                                                                                                                                                                           mov   rax, QWORD PTR bb$7[rsp]                        mov   rax, QWORD PTR bb$7[rsp]                    
                                                                                                                                                                           inc   rax                                             inc   rax                                         
                                                                                                                                                                           mov   QWORD PTR bb$7[rsp], rax                        mov   QWORD PTR bb$7[rsp], rax                    
                                                                                                                                                                           mov   eax, 1                                          mov   eax, 1                                      
                                                                                                                                                                           imul   rax, rax, 0                                     imul   rax, rax, 0                                 
                                                                                                                                                                           mov   ecx, 1                                          mov   ecx, 1                                      
                                                                                                                                                                           imul   rcx, rcx, 0                                     imul   rcx, rcx, 0                                 
                                                                                                                                                                           mov   rdx, QWORD PTR ab$6[rsp]                        mov   rdx, QWORD PTR ab$6[rsp]                    
                                                                                                                                                                           mov   r8, QWORD PTR bb$7[rsp]                         mov   r8, QWORD PTR bb$7[rsp]                     
                                                                                                                                                                           movzx   eax, BYTE PTR [r8+rax]                          movzx   eax, BYTE PTR [r8+rax]                      
                                                                                                                                                                           mov   BYTE PTR [rdx+rcx], al                          mov   BYTE PTR [rdx+rcx], al                      
                                                                                                                                                                           mov   eax, 4                                          mov   eax, 4                                      
                                                                                                                                                                           imul   rax, rax, 0                                     imul   rax, rax, 0                                 
                                                                                                                                                                           mov   rcx, QWORD PTR ab$6[rsp]                        mov   rcx, QWORD PTR ab$6[rsp]                    
                                                                                                                                                                           mov   eax, DWORD PTR [rcx+rax]                        mov   eax, DWORD PTR [rcx+rax]                    
                                                                                                                                                                           mov   DWORD PTR b$[rsp], eax                          mov   DWORD PTR b$[rsp], eax                      

```


# GCC

Results not completed.
