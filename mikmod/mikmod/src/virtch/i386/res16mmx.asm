.586p
.model flat
.data ; self-modifying code... keep in data segment

        NAME    resMMX


        EXTRN   _rvolsel:DWORD
        EXTRN   _lvolsel:DWORD

;        PUBLIC  _AsmStereoNormal
        PUBLIC  _Asm16StereoInterpMMX
;        PUBLIC  _AsmSurroundNormal
;        PUBLIC  _AsmSurroundInterp
;        PUBLIC  _AsmMonoNormal
;        PUBLIC  _AsmMonoInterp


STUBSTART macro
  push eax
  push ebx
  push ecx
  push edx
  push esi
  push edi
  push ebp
  endm


STUBEND macro
  pop ebp
  pop edi
  pop esi
  pop edx
  pop ecx
  pop ebx
  pop eax
  endm

MMXINTRO MACRO lab1,lab2,lab3,lab4,lab5,lab6,lab7,lab8 ; 486+

; The first pass!  This is an annoyingly complex setup. Here is an overview of
; fuctionality, repeated four times:
;  a) take the index fractional (lower 32 bits), and shift down to 16 bit accuracy.
;  b) get the sample data (into al,cl)
;  c) put the index fractional (15 bit version) and the sample data into the quadwords.
;  d) increment the index to point to the next data.

 mov       esi, ebx
 mov       ax, word ptr [edx+edx]
 shr       esi, 17
 mov       cx, word ptr +2H[edx+edx]

 shl       eax,16
 shl       ecx,16

 db 081h,0c3h
lab1 dd 0 ; add ebx,lo
 db 081h,0d2h
lab2 dd 0 ; adc edx,hi

 mov       ax, word ptr [ed+edx]
 mov       cx, word ptr +2H[edx+edx]

 mov       dword ptr [quad3], esi
 mov       dword ptr [quad1], eax
 mov       dword ptr [quad2], ecx

 ;  TWO -- 2 -- 2nd -- Get the second set of mixer values!
 
 mov       esi, ebx
 mov       ax, word ptr [edx*2]
 shr       esi, 17
 mov       cx, word ptr +2H[edx*2]
 mov       word ptr +2H[quad3], si
 mov       word ptr +2H[quad1], ax
 mov       word ptr +2H[quad2], cx

 db 081h,0c3h
lab3 dd 0 ; add ebx,lo
 db 081h,0d2h
lab4 dd 0 ; adc edx,hi

 ;  THREE -- 3 -- 3rd --

 mov       esi, ebx
 mov       ax, word ptr [edx*2]
 shr       esi, 17
 mov       cx, word ptr +2H[edx*2]
 mov       word ptr +4H[quad3], si
 mov       word ptr +4H[quad1], ax
 mov       word ptr +4H[quad2], cx

 db 081h,0c3h
lab5 dd 0 ; add ebx,lo
 db 081h,0d2h
lab6 dd 0 ; adc edx,hi

 ;  FOURTH -- 4 -- 4th --

 mov       esi, ebx
 mov       ax, word ptr [edx*2]
 shr       esi, 17
 mov       cx, word ptr +2H[edx*2]
 mov       word ptr +6H[quad3], si
 mov       word ptr +6H[quad1], ax
 mov       word ptr +6H[quad2], cx

 db 081h,0c3h
lab7 dd 0 ; add ebx,lo
 db 081h,0d2h
lab8 dd 0 ; adc edx,hi

ENDM


 ; Interpolation math, done with MMX!
 ; quad1 = src, quad2 = next src, quad3 = index fractional;
 
 movq      mm0, [quad2]          ; next src
 movq      mm1, [quad5]          ; next src
 psubw     mm0, [quad1]          ; next src - src
 psubw     mm1, [quad4]          ; next src - src

 paddw     mm0, mm0              ; compensate for having to shift the fractional down an extra bit (damn signed mul).
 paddw     mm1, mm1

 pmulhw    mm0, [quad3]          ; result * index frac (high result)

 add       esi, 4*4*2
 add       edi, 4*4*2

 pmulhw    mm1, [quad6]          ; result * index frac (high result)
 paddw     mm0, [quad1]          ; result + src (Done: src + (next src - result) * fractional)

 movq      mm2, mm0
 paddw     mm1, [quad4]          ; result + src (Done: src + (next src - result) * fractional)

 pmulhw    mm0, [q1_volsel]      ; sample * volumes (left/right interleaved)
 movq      mm7, 0
 pmulhw    mm1, [q1_volsel]

 movq      mm4, mm0
 
 pmulhw    mm2, [q2_volsel]      ; sample * volumes (left/right interleaved)
 punpcklwd mm0, mm7              ; interleave it with 0's

 punpckhwd mm4, mm7
 paddq     [edi-64], mm0

 movq      mm5, mm1
 paddq     [edi-56], mm4
 
 pmulhw    mm3, [q2_volsel]
 punpcklwd mm1, mm7

 punpckhwd mm5, mm7 
 paddq     [edi-48], mm1

 movq      mm6, mm2
 paddq     [edi-40], mm5

 movq      mm4, mm3
 punpcklwd mm2, mm7

 punpckhwd mm6, mm7
 paddq     [edi-32], mm2

 punpcklwd mm3, mm7              ; interleave it with 0's
 paddq     [edi-24], mm6

 punpckhwd mm4, mm7
 paddq     [edi-16], mm3
 paddq     [edi-8], mm4


; -----------------------------------------
;        Stereo Interpolation Macros
; -----------------------------------------

IS2F MACRO  ; 486+

 movq      mm0, [quad1]          ; src
 movq      mm1, [quad2]          ; next src

 psubw     mm1, mm0              ; next src - src
 psllw     mm1, 1                ; compensate for having to shift the fractional down an extra bit (damn signed mul).
 pmulhw    mm1, [quad3]          ; result * index frac (high result)
 paddw     mm1, mm0              ; result + src (Done: src + (next src - result) * fractional)
 movq      mm2, mm1
 pmulhw    mm1, [q_lvolsel]        ; sample * left volume
 pmulhw    mm2, [q_rvolsel]        ; sample * right volume
 
 movq      [quad1],mm1
 movq      [quad2],mm2
 
 movsx     eax,word ptr [quad1]
 movsx     ecx,word ptr [quad2]
 mov       dword ptr [quad4],eax
 mov       dword ptr +4[quad4],ecx
 movsx     eax,word ptr +2[quad1]
 movsx     ecx,word ptr +2[quad2]
 mov       dword ptr [quad5],eax
 mov       dword ptr +4[quad5],ecx

 movsx     eax,word ptr +4[quad1]
 movsx     ecx,word ptr +4[quad2]
 mov       dword ptr [quad6],eax
 mov       dword ptr +4[quad6],ecx
 movsx     eax,word ptr +6[quad1]
 movsx     ecx,word ptr +6[quad2]
 mov       dword ptr [quad7],eax
 mov       dword ptr +4[quad7],ecx
 
 movq      mm0,[quad4]
 movq      mm1,[quad5]
 movq      mm2,[quad6]
 movq      mm3,[quad7]
 pslld     mm0, 9
 pslld     mm1, 9
 pslld     mm2, 9
 pslld     mm3, 9
 paddd     mm0, [edi]
 paddd     mm1, +8[edi]
 paddd     mm2, +16[edi]
 paddd     mm3, +24[edi]
 movq      [edi], mm0
 movq      +8[edi], mm1
 movq      +16[edi], mm2
 movq      +24[edi], mm3

ENDM


; -----------------------------------------
;        Surround Interpolation Macros
; -----------------------------------------

IS3F MACRO ; 486+

 movq      mm0, [quad1]          ; src
 movq      mm1, [quad2]          ; next src

 psubw     mm1, mm0              ; next src - src
 psllw     mm1, 1                ; compensate for having to shift the fractional down an extra bit (damn signed mul).
 pmulhw    mm1, [quad3]          ; result * index frac (high result)
 paddw     mm1, mm0              ; result + src (Done: src + (next src - result) * fractional)
 pmulhw    mm1, [q_lvolsel]      ; sample * left volume
 
 movq      [quad1],mm1
 
 movsx     eax,word ptr [quad1]
 movsx     ecx,word ptr +2[quad1]
 mov       dword ptr [quad4],eax
 mov       dword ptr [quad5],ecx
 neg       eax
 neg       ecx
 mov       dword ptr +4[quad4],eax
 mov       dword ptr +4[quad5],ecx

 movsx     eax,word ptr +4[quad1]
 movsx     ecx,word ptr +6[quad1]
 mov       dword ptr [quad6],eax
 mov       dword ptr [quad7],ecx
 neg       eax
 neg       ecx
 mov       dword ptr +4[quad6],eax
 mov       dword ptr +4[quad7],ecx
 
 movq      mm0,[quad4]
 movq      mm1,[quad5]
 movq      mm2,[quad6]
 movq      mm3,[quad7]
 pslld     mm0, 9
 pslld     mm1, 9
 pslld     mm2, 9
 pslld     mm3, 9
 paddd     mm0, [edi]
 paddd     mm1, +8[edi]
 paddd     mm2, +16[edi]
 paddd     mm3, +24[edi]
 movq      [edi], mm0
 movq      +8[edi], mm1
 movq      +16[edi], mm2
 movq      +24[edi], mm3

ENDM

; -----------------------------------------
;   Mono Interpolation Macros (Where the action is!)
; -----------------------------------------

IM2F MACRO ; 486+

 movq      mm0, [quad1]          ; src
 movq      mm1, [quad2]          ; next src

 psubw     mm1, mm0              ; next src - src
 psllw     mm1, 1                ; compensate for having to shift the fractional down an extra bit (damn signed mul).
 pmulhw    mm1, [quad3]          ; result * index frac (high result)
 paddw     mm1, mm0              ; result + src (Done: src + (next src - result) * fractional)
 pmulhw    mm1, [q_lvolsel]      ; sample * left volume
 
 movq      [quad1],mm1
 
 movsx     eax,word ptr [quad1]
 movsx     ecx,word ptr +2[quad1]
 mov       dword ptr [quad4],eax
 mov       dword ptr [quad5],ecx
 movsx     eax,word ptr +4[quad1]
 movsx     ecx,word ptr +6[quad1]
 mov       dword ptr +4[quad4],eax
 mov       dword ptr +4[quad5],ecx
 
 movq      mm0,[quad4]
 movq      mm1,[quad5]
 pslld     mm0, 9
 pslld     mm1, 9
 paddd     mm0, [edi]
 paddd     mm1, +8[edi]
 movq      [edi], mm0
 movq      +8[edi], mm1

ENDM

; ------------------------------------------
; Data used by MMX temp storage annoyances
; ------------------------------------------

quad1      dq   0
quad2      dq   0
quad3      dq   0
quad4      dq   0
quad5      dq   0
quad6      dq   0
quad7      dq   0

q_lvolsel  dq   0
q_rvolsel  dq   0

; -----------------------------------------------
; ------- Actual Code Stuffs starts HERE! -------
; -----------------------------------------------

_Asm16StereoInterpMMX:
        STUBSTART
        mov    esi,[esp+32] ; get src
        mov    edi,[esp+36] ; get dst
        mov    ebx,[esp+40] ; get index_lo
        mov    edx,[esp+44] ; get index_hi
        mov    ecx,[esp+48] ; get increment_lo
        mov    eax,[esp+52] ; get increment_hi
        mov    ebp,[esp+56] ; get todo

        shl    edx,1
        add    edx,esi
        shr    edx,1
        mov    sihi1,eax
        mov    sihi2,eax
        mov    sihi3,eax
        mov    sihi4,eax
        mov    silo1,ecx
        mov    silo2,ecx
        mov    silo3,ecx
        mov    silo4,ecx
        
        ;  set up the quadword versions of lvolsel and rvolsel
        
        mov    eax,[_lvolsel]
        mov    ecx,[_rvolsel]
        shl    eax,7
        shl    ecx,7
        mov    word ptr [q_lvolsel],ax
        mov    word ptr [q_rvolsel],cx
        mov    word ptr +2H[q_lvolsel],ax
        mov    word ptr +2H[q_rvolsel],cx
        mov    word ptr +4H[q_lvolsel],ax
        mov    word ptr +4H[q_rvolsel],cx
        mov    word ptr +6H[q_lvolsel],ax
        mov    word ptr +6H[q_rvolsel],cx
        
        jmp    si1 ; flush code cache
si1:
        shr    ebp,2
        inc    ebp
siagain16:
        MMXINTRO silo1,sihi1,silo2,sihi2,silo3,sihi3,silo4,sihi4
        IS2F   
        add    edi,(4*8)
        dec    ebp
        jnz    siagain16

        STUBEND
        ret


_Asm16SurroundInterpMMX:
        STUBSTART
        mov    esi,[esp+32] ; get src
        mov    edi,[esp+36] ; get dst
        mov    ebx,[esp+40] ; get index_lo
        mov    edx,[esp+44] ; get index_hi
        mov    ecx,[esp+48] ; get increment_lo
        mov    eax,[esp+52] ; get increment_hi
        mov    ebp,[esp+56] ; get todo

        shl    edx,1
        add    edx,esi
        shr    edx,1
        mov    s2ihi1,eax
        mov    s2ihi2,eax
        mov    s2ihi3,eax
        mov    s2ihi4,eax
        mov    s2ilo1,ecx
        mov    s2ilo2,ecx
        mov    s2ilo3,ecx
        mov    s2ilo4,ecx
        
        ;  set up the quadword versions of lvolsel and rvolsel
        
        mov    eax,[_lvolsel]
        mov    ecx,[_rvolsel]
        shl    eax,7
        shl    ecx,7
        mov    word ptr [q_lvolsel],ax
        mov    word ptr [q_rvolsel],cx
        mov    word ptr +2H[q_lvolsel],ax
        mov    word ptr +2H[q_rvolsel],cx
        mov    word ptr +4H[q_lvolsel],ax
        mov    word ptr +4H[q_rvolsel],cx
        mov    word ptr +6H[q_lvolsel],ax
        mov    word ptr +6H[q_rvolsel],cx
        
        jmp    si2 ; flush code cache
si2:
        shr    ebp,2
        inc    ebp
ssagain16:
        MMXINTRO s2ilo1,s2ihi1,s2ilo2,s2ihi2,s2ilo3,s2ihi3,s2ilo4,s2ihi4
        IS3F   
        add    edi,(4*8)
        dec    ebp
        jnz    ssagain16

        STUBEND
        ret


; -----------------------------------------------
; ------- Actual Code Stuffs starts HERE! -------
; -----------------------------------------------

_Asm16MonoInterpMMX:
        STUBSTART
        mov    esi,[esp+32] ; get src
        mov    edi,[esp+36] ; get dst
        mov    ebx,[esp+40] ; get index_lo
        mov    edx,[esp+44] ; get index_hi
        mov    ecx,[esp+48] ; get increment_lo
        mov    eax,[esp+52] ; get increment_hi
        mov    ebp,[esp+56] ; get todo

        shl    edx,1
        add    edx,esi
        shr    edx,1
        mov    mihi1,eax
        mov    mihi2,eax
        mov    mihi3,eax
        mov    mihi4,eax
        mov    milo1,ecx
        mov    milo2,ecx
        mov    milo3,ecx
        mov    milo4,ecx
        
        ;  set up the quadword versions of lvolsel and rvolsel
        
        mov    eax,[_lvolsel]
        mov    ecx,[_rvolsel]
        shl    eax,7
        shl    ecx,7
        mov    word ptr [q_lvolsel],ax
        mov    word ptr [q_rvolsel],cx
        mov    word ptr +2H[q_lvolsel],ax
        mov    word ptr +2H[q_rvolsel],cx
        mov    word ptr +4H[q_lvolsel],ax
        mov    word ptr +4H[q_rvolsel],cx
        mov    word ptr +6H[q_lvolsel],ax
        mov    word ptr +6H[q_rvolsel],cx
        
        jmp    mi1 ; flush code cache
mi1:
        shr    ebp,2
        inc    ebp
miagain16:
        MMXINTRO milo1,mihi1,milo2,mihi2,milo3,mihi3,milo4,mihi4
        IM2F   
        add    edi,(2*8)
        dec    ebp
        jnz    miagain16

        STUBEND
        ret
END

