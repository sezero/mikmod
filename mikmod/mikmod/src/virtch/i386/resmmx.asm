.586p
.model flat
.data ; self-modifying code... keep in data segment

        NAME    resMMX


        EXTRN   _rvolsel:DWORD
        EXTRN   _lvolsel:DWORD

;        PUBLIC  _AsmStereoNormal
        PUBLIC  _AsmStereoInterpMMX
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

; -----------------------------------------
;        Stereo Interpolation Macros
; -----------------------------------------

IS2F MACRO lab1,lab2,lab3,lab4,lab5,lab6,lab7,lab8 ; 486+

; The first pass!  This is an annoyingly complex setup. Here is an overview of
; fuctionality, repeated four times:
;  a) take the index fractional (lower 32 bits), and shift down to 16 bit accuracy.
;  b) get the sample data (into al,cl)
;  c) put the index fractional (16 bit version) and the sample data into the quadwords.
;  d) increment the index to point to the next data.

 mov       esi, ebx
 shr       esi, 17
 movsx     ax, byte ptr [edx]
 movsx     cx, byte ptr +1H[edx]
 mov       word ptr [quad3], si
 mov       word ptr [quad1], ax
 mov       word ptr [quad2], cx

 db 081h,0c3h
lab1 dd 0 ; add ebx,lo
 db 081h,0d2h
lab2 dd 0 ; adc edx,hi

 ;  TWO -- 2 -- 2nd -- Get the second set of mixer values!
 
 mov       esi, ebx
 shr       esi, 17
 movsx     ax, byte ptr [edx]
 movsx     cx, byte ptr +1H[edx]
 mov       word ptr +2H[quad3], si
 mov       word ptr +2H[quad1], ax
 mov       word ptr +2H[quad2], cx

 db 081h,0c3h
lab3 dd 0 ; add ebx,lo
 db 081h,0d2h
lab4 dd 0 ; adc edx,hi

 ;  THREE -- 3 -- 3rd --

 mov       esi, ebx
 shr       esi, 17
 movsx     ax, byte ptr [edx]
 movsx     cx, byte ptr +1H[edx]
 mov       word ptr +4H[quad3], si
 mov       word ptr +4H[quad1], ax
 mov       word ptr +4H[quad2], cx

 db 081h,0c3h
lab5 dd 0 ; add ebx,lo
 db 081h,0d2h
lab6 dd 0 ; adc edx,hi

 ;  FOURTH -- 4 -- 4th --

 mov       esi, ebx
 shr       esi, 17
 movsx     ax, byte ptr [edx]
 movsx     cx, byte ptr +1H[edx]
 mov       word ptr +6H[quad3], si
 mov       word ptr +6H[quad1], ax
 mov       word ptr +6H[quad2], cx

; --- Setup done, time to get jiggy wit the MMX code.

 movq      mm0, [quad1]          ; src
 movq      mm1, [quad2]          ; next src

 psubw     mm1, mm0              ; next src - src
 psllw     mm1, 1                ; compensate for having to shift the fractional down an extra bit (damn signed mul).
 pmulhw    mm1, [quad3]          ; result * index frac (high result)
 db 081h,0c3h
lab7 dd 0 ; add ebx,lo
 paddw     mm1, mm0              ; result + src (Done: src + (next src - result) * fractional)
 db 081h,0d2h
lab8 dd 0 ; adc edx,hi

 movq      mm2, mm1
 pmullw    mm1, [q_lvolsel]        ; sample * left volume
 pmullw    mm2, [q_rvolsel]        ; sample * right volume
 
 movq      mm0,mm1
 movq      mm4,mm1
 movq      mm3,mm2
 punpcklwd mm0,mm2
 punpckhwd mm1,mm2
 punpcklwd mm2,mm4
 punpckhwd mm3,mm4

 pslld     mm0, 8                      ; make sure volumes match 16 bit mixers.
 pslld     mm1, 8
 pslld     mm2, 8
 pslld     mm3, 8
 
 paddd     mm0, [edi]
 paddd     mm1, +8[edi]
 paddd     mm2, +16[edi]
 paddd     mm3, +24[edi]
 
 movq      [edi], mm0
 movq      +8[edi], mm1
 movq      +16[edi], mm2
 movq      +24[edi], mm3

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

lvtmp      dq   0
rvtmp      dq   0

q_lvolsel  dq   0
q_rvolsel  dq   0

;andval     dq   00ff00ff00ff00ffh
; -----------------------------------------------
; ------- Actual Code Stuffs starts HERE! -------
; -----------------------------------------------

_AsmStereoInterpMMX:
        STUBSTART
        mov    esi,[esp+32] ; get src
        mov    edi,[esp+36] ; get dst
        mov    ebx,[esp+40] ; get index_lo
        mov    edx,[esp+44] ; get index_hi
        mov    ecx,[esp+48] ; get increment_lo
        mov    eax,[esp+52] ; get increment_hi
        mov    ebp,[esp+56] ; get todo

        add    edx,esi
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
        IS2F   silo1,sihi1,silo2,sihi2,silo3,sihi3,silo4,sihi4
        add    edi,(4*8)
        dec    ebp
        jnz    siagain16

        STUBEND
        ret

END
