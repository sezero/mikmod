; Mikmod Sound System
;  Intel (486) Optimized 8 bit STEREO SAMPLE mixers.
;
; These are just the the ones in resample.asm, only they mix stereo samples.
; Note that there is no support for surround sound mixers with these.  Surround
; sound is lame anyway, so get over it!

; Jake Stine of Divine Entertainment

.386p
.model flat
.data ; self-modifying code... keep in data segment

        NAME    resample8stereo

        EXTRN   _rvolsel:DWORD
        EXTRN   _lvolsel:DWORD

        PUBLIC  _AsmStereoSS
        PUBLIC  _AsmStereoSSI
        PUBLIC  _AsmMonoSS
        PUBLIC  _AsmMonoSSI


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

SS2F MACRO index,lab1,lab2 ; 486+
 movsx eax,byte ptr [edx+edx]
 movsx ecx,byte ptr +1H[edx+edx]

 imul  eax,[_lvolsel]
 imul  ecx,[_rvolsel]

 db 081h,0c3h
lab1 dd 0 ; add ebx,lo
 db 081h,0d2h
lab2 dd 0 ; adc edx,hi

 add    (index*8)[edi],eax
 add    (index*8+4)[edi],ecx
 ENDM

; -----------------------------------------
;        Stereo Interpolation Macros
; -----------------------------------------

IS2F MACRO index,lab1,lab2 ; 486+
 movsx  eax,byte ptr +2H[edx+edx]
 movsx  ecx,byte ptr [edx+edx]
 movsx  esi,byte ptr +1H[edx+edx]
 movsx  ebp,byte ptr +3H[edx+edx]
  
 sub    esi,ebp       ; next src - src
 mov    [idxshft],ebx ; load idxshft and shift it right 8 bits!
 sub    eax,ecx
 shr    [idxshft],18

 db 081h,0c3h
lab1 dd 0 ; add ebx,lo
 db 081h,0d2h
lab2 dd 0 ; adc edx,hi

 imul   eax,[idxshft] ; result * frac
 imul   esi,[idxshft] ; result * frac

 sar    eax,14        ; shift off the fixed point portion
 sar    esi,14

 add    eax,ecx       ; eax = src + ((next src - src) * fractional)
 add    esi,ebp
 
 imul   eax,[_lvolsel]
 imul   esi,[_rvolsel]

 add    (index*8)[edi],eax
 add    (index*8+4)[edi],esi
ENDM


; -----------------------------------------
;            Mono Standard Macros
; -----------------------------------------

SM2F MACRO index,lab1,lab2 ; 486+
 movsx eax,byte ptr [edx+edx]
 movsx esi,byte ptr +1H[edx+edx]

 add   eax,esi       ; merge stereo : (lsample + rsample) / 2

 db 081h,0c3h
lab1 dd 0 ; add ebx,lo
 db 081h,0d2h
lab2 dd 0 ; adc edx,hi
 sar   eax,1

 imul  eax,[_lvolsel]

 add (index*4)[edi],eax

 ENDM


; -----------------------------------------
;         Mono Interpolation Macros
; -----------------------------------------

IM2F MACRO index,lab1,lab2 ; 486+
 movsx  eax,byte ptr +2H[edx+edx]
 movsx  ecx,byte ptr [edx+edx]
 movsx  esi,byte ptr +3H[edx+edx]
 movsx  ebp,byte ptr +1H[edx+edx]

 sub    esi,ebp       ; next src - src
 mov    [idxshft],ebx ; load idxshft and shift it right 8 bits!
 sub    eax,ecx
 shr    [idxshft],18

 imul   eax,[idxshft] ; result * frac
 imul   esi,[idxshft] ; result * frac

 add    ecx,ebp
 db 081h,0c3h
lab1 dd 0 ; add ebx,lo
 db 081h,0d2h
lab2 dd 0 ; adc edx,hi
 shl    ecx,14

 add    eax,ecx       ; eax = src + ((next src - src) * fractional)
 add    eax,esi
 sar    eax,15

 imul   eax,[_lvolsel]
 add    (index*8)[edi],eax

 ENDM

; ------------------------------------------
; Data used by interpolation mixers only...
; ------------------------------------------

        dd   0       ; Extra room because we do weird things!
idxshft dd   0       ; index shift memory space!


; -----------------------------------------------
; ------- Actual Code Stuffs starts HERE! -------
; -----------------------------------------------

_AsmStereoSS:
        STUBSTART
        mov    esi,[esp+32] ; get src
        mov    edi,[esp+36] ; get dst
        mov    ebx,[esp+40] ; get index_lo
        mov    edx,[esp+44] ; get index_hi
        mov    ecx,[esp+48] ; get increment_lo
        mov    eax,[esp+52] ; get increment_hi
        mov    ebp,[esp+56] ; get todo

        mov    shi1,eax
        add    edx,esi
        mov    shi2,eax
        mov    shi3,eax
        mov    shi4,eax
        mov    shi5,eax
        mov    slo1,ecx
        mov    slo2,ecx
        mov    slo3,ecx
        mov    slo4,ecx
        mov    slo5,ecx

        push   ebp
        jmp    s1 ; flush code cache
s1:
        shr    ebp,2
        jz     sskip8
        push   ebp
sagain8:
        SS2F   0,slo1,shi1
        SS2F   1,slo2,shi2
        SS2F   2,slo3,shi3
        SS2F   3,slo4,shi4
        add    edi,(4*8)
        dec    dword ptr [esp]
        jnz    sagain8
        pop    ebp
sskip8:
        pop    ebp
        and    ebp,3
        jz     sskip1
        push   ebp
sagain1:
        SS2F   0,slo5,shi5
        add    edi,8
        dec    dword ptr [esp]
        jnz    sagain1
        pop    ebp
sskip1:
        STUBEND
        ret


_AsmStereoSSI:
        STUBSTART
        mov    esi,[esp+32] ; get src
        mov    edi,[esp+36] ; get dst
        mov    ebx,[esp+40] ; get index_lo
        mov    edx,[esp+44] ; get index_hi
        mov    ecx,[esp+48] ; get increment_lo
        mov    eax,[esp+52] ; get increment_hi
        mov    ebp,[esp+56] ; get todo

        mov    sihi1,eax
        add    edx,esi
        mov    sihi2,eax
        mov    sihi3,eax
        mov    sihi4,eax
        mov    sihi5,eax
        mov    silo1,ecx
        mov    silo2,ecx
        mov    silo3,ecx
        mov    silo4,ecx
        mov    silo5,ecx

        push   ebp
        jmp    si1 ; flush code cache
si1:
        shr    ebp,2
        jz     siskip8
        push   ebp
siagain8:
        IS2F   0,silo1,sihi1
        IS2F   1,silo2,sihi2
        IS2F   2,silo3,sihi3
        IS2F   3,silo4,sihi4
        add    edi,(4*8)
        dec    dword ptr [esp]
        jnz    siagain8
        pop    ebp
siskip8:
        pop    ebp
        and    ebp,3
        jz     siskip1
        push   ebp
siagain1:
        IS2F   0,silo5,sihi5
        add    edi,8
        dec    dword ptr [esp]
        jnz    siagain1
        pop    ebp
siskip1:
        STUBEND
        ret


_AsmMonoSS:
        STUBSTART
        mov    esi,[esp+32] ; get src
        mov    edi,[esp+36] ; get dst
        mov    ebx,[esp+40] ; get index_lo
        mov    edx,[esp+44] ; get index_hi
        mov    ecx,[esp+48] ; get increment_lo
        mov    eax,[esp+52] ; get increment_hi
        mov    ebp,[esp+56] ; get todo

        mov    mhi1,eax
        add    edx,esi
        mov    mhi2,eax
        mov    mhi3,eax
        mov    mhi4,eax
        mov    mhi5,eax
        mov    mlo1,ecx
        mov    mlo2,ecx
        mov    mlo3,ecx
        mov    mlo4,ecx
        mov    mlo5,ecx

        push   ebp
        jmp    m1 ; flush code cache
m1:
        shr    ebp,2
        jz     mskip8
magain8:
        SM2F   0,mlo1,mhi1
        SM2F   1,mlo2,mhi2
        SM2F   2,mlo3,mhi3
        SM2F   3,mlo4,mhi4
        add    edi,(4*4)
        dec    ebp
        jnz    magain8
mskip8:
        pop    ebp
        and    ebp,3
        jz     mskip1
magain1:
        SM2F   0,mlo5,mhi5
        add    edi,4
        dec    ebp
        jnz    magain1
mskip1:
        STUBEND
        ret


_AsmMonoSSI:
        STUBSTART
        mov    esi,[esp+32] ; get src
        mov    edi,[esp+36] ; get dst
        mov    ebx,[esp+40] ; get index_lo
        mov    edx,[esp+44] ; get index_hi
        mov    ecx,[esp+48] ; get increment_lo
        mov    eax,[esp+52] ; get increment_hi
        mov    ebp,[esp+56] ; get todo

        mov    mihi1,eax
        add    edx,esi
        mov    mihi2,eax
        mov    mihi3,eax
        mov    mihi4,eax
        mov    mihi5,eax
        mov    milo1,ecx
        mov    milo2,ecx
        mov    milo3,ecx
        mov    milo4,ecx
        mov    milo5,ecx

        push   ebp
        jmp    mi1 ; flush code cache
mi1:
        shr    ebp,2
        jz     miskip8
miagain8:
        IM2F   0,milo1,mihi1
        IM2F   1,milo2,mihi2
        IM2F   2,milo3,mihi3
        IM2F   3,milo4,mihi4
        add    edi,(4*4)
        dec    ebp
        jnz    miagain8
miskip8:
        pop    ebp
        and    ebp,3
        jz     miskip1
miagain1:
        IM2F   0,milo5,mihi5
        add    edi,4
        dec    ebp
        jnz    miagain1
miskip1:
        STUBEND
        ret

        END
