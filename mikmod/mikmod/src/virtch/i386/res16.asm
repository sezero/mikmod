.386p
.model flat
.data ; self-modifying code... keep in data segment

        NAME    resample16


        EXTRN   _rvolsel:DWORD
        EXTRN   _lvolsel:DWORD

        PUBLIC  _Asm16StereoNormal
        PUBLIC  _Asm16StereoInterp
        PUBLIC  _Asm16SurroundNormal
        PUBLIC  _Asm16SurroundInterp
        PUBLIC  _Asm16MonoNormal
        PUBLIC  _Asm16MonoInterp


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
;          Stereo Standard Macros
; -----------------------------------------

SS2F MACRO index,lab1,lab2 ; 486+
 movsx eax,word ptr[edx+edx]

 db 081h,0c3h
lab1 dd 0 ; add ebx,lo
 db 081h,0d2h
lab2 dd 0 ; adc edx,hi

 mov   ecx, eax
 imul  ecx,[_lvolsel]
 imul  eax,[_rvolsel]

 add    (index*8)[edi],ecx
 add    (index*8+4)[edi],eax
 ENDM

SS3F MACRO index,lab1,lab2,lab3 ; 486+
 movsx eax,word ptr [edx+edx]
 imul  eax,[_lvolsel]

 db 081h,0c3h
lab1 dd 0 ; add ebx,lo
 db 081h,0d2h
lab2 dd 0 ; adc edx,hi

 add    (index*8)[edi],eax
 sub    (index*8+4)[edi],eax
 ENDM

; -----------------------------------------
;        Stereo Interpolation Macros
; -----------------------------------------

IS2F MACRO index,lab1,lab2 ; 486+
 movsx  eax,word ptr +2H[edx+edx]
 movsx  ecx,word ptr[edx+edx]
  
 mov    esi,ebx
 sub    eax,ecx       ; next src - src
 shr    esi,18
 imul   eax,esi       ; result * frac
 sar    eax,14        ; shift off the fixed point portion

 db 081h,0c3h
lab1 dd 0 ; add ebx,lo
 db 081h,0d2h
lab2 dd 0 ; adc edx,hi
 add    eax,ecx       ; eax = src + ((next src - src) * fractional)

 mov    esi,eax
 imul   eax,[_lvolsel]
 imul   esi,[_rvolsel]

 add    (index*8)[edi],eax
 add    (index*8+4)[edi],esi
ENDM

; -----------------------------------------
;       Surround Interpolation Macros
; -----------------------------------------

IS3F MACRO index,lab1,lab2 ; 486+
 movsx  eax,word ptr +2H[edx+edx]
 movsx  ecx,word ptr[edx+edx]
  
 mov    esi,ebx
 sub    eax,ecx       ; next src - src
 shr    esi,18
 imul   eax,esi       ; result * frac
 sar    eax,14        ; shift off the fixed point portion

 db 081h,0c3h
lab1 dd 0 ; add ebx,lo
 db 081h,0d2h
lab2 dd 0 ; adc edx,hi
 add    eax,ecx       ; eax = src + ((next src - src) * fractional)

 imul   eax,[_lvolsel]

 add    (index*8)[edi],eax
 sub    (index*8+4)[edi],eax

 ENDM


; -----------------------------------------
;            Mono Standard Macros
; -----------------------------------------

SM2F MACRO index,lab1,lab2 ; 486+
 movsx eax,word ptr [edx+edx]

 imul  eax,[_lvolsel]

 db 081h,0c3h
lab1 dd 0 ; add ebx,lo
 db 081h,0d2h
lab2 dd 0 ; adc edx,hi
 add (index*4)[edi],eax

 ENDM


; -----------------------------------------
;         Mono Interpolation Macros
; -----------------------------------------

IM2F MACRO index,lab1,lab2 ; 486+
 movsx  eax,word ptr +2H[edx+edx]
 movsx  ecx,word ptr [edx+edx]
  
 mov    esi,ebx
 sub    eax,ecx       ; next src - src
 shr    esi,18
 imul   eax,esi       ; result * frac
 sar    eax,14        ; shift off the fixed point portion

 db 081h,0c3h
lab1 dd 0 ; add ebx,lo
 db 081h,0d2h
lab2 dd 0 ; adc edx,hi

 add    eax,ecx       ; eax = src + ((next src - src) * fractional)
 imul   eax,[_lvolsel]

 add    (index*4)[edi],eax
 ENDM

; -----------------------------------------------
; ------- Actual Code Stuffs starts HERE! -------
; -----------------------------------------------

_Asm16StereoNormal:
        STUBSTART
        mov    esi,[esp+32] ; get src
        mov    edi,[esp+36] ; get dst
        mov    ebx,[esp+40] ; get index_lo
        mov    edx,[esp+44] ; get index_hi
        mov    ecx,[esp+48] ; get increment_lo
        mov    eax,[esp+52] ; get increment_hi
        mov    ebp,[esp+56] ; get todo

        shl    edx,1
        mov    shi9,eax
        add    edx,esi
        mov    slo9,ecx
        sar    edx,1

        push   ebp
        shr    ebp,3
        jz     sskip16

        mov    shi1,eax
        mov    shi2,eax
        mov    shi3,eax
        mov    shi4,eax
        mov    shi5,eax
        mov    shi6,eax
        mov    shi7,eax
        mov    shi8,eax
        mov    slo1,ecx
        mov    slo2,ecx
        mov    slo3,ecx
        mov    slo4,ecx
        mov    slo5,ecx
        mov    slo6,ecx
        mov    slo7,ecx
        mov    slo8,ecx

        jmp    sagain16 ; flush code cache
sagain16:
        SS2F   0,slo1,shi1
        SS2F   1,slo2,shi2
        SS2F   2,slo3,shi3
        SS2F   3,slo4,shi4
        SS2F   4,slo5,shi5
        SS2F   5,slo6,shi6
        SS2F   6,slo7,shi7
        SS2F   7,slo8,shi8
        add    edi,(8*8)
        dec    ebp
        jnz    sagain16
sskip16:
        pop    ebp
        and    ebp,7
        jz     sskip1
sagain1:
        SS2F   0,slo9,shi9
        add    edi,8
        dec    ebp
        jnz    sagain1
sskip1:
        STUBEND
        ret

_Asm16StereoInterp:
        STUBSTART
        mov    esi,[esp+32] ; get src
        mov    edi,[esp+36] ; get dst
        mov    ebx,[esp+40] ; get index_lo
        mov    edx,[esp+44] ; get index_hi
        mov    ecx,[esp+48] ; get increment_lo
        mov    eax,[esp+52] ; get increment_hi
        mov    ebp,[esp+56] ; get todo

        shl    edx,1
        mov    sihi9,eax
        add    edx,esi
        mov    silo9,ecx
        sar    edx,1

        push   ebp
        shr    ebp,2
        jz     siskip16

        mov    sihi1,eax
        mov    sihi2,eax
        mov    sihi3,eax
        mov    sihi4,eax
        ;mov    sihi5,eax
        ;mov    sihi6,eax
        ;mov    sihi7,eax
        ;mov    sihi8,eax
        mov    silo1,ecx
        mov    silo2,ecx
        mov    silo3,ecx
        mov    silo4,ecx
        ;mov    silo5,ecx
        ;mov    silo6,ecx
        ;mov    silo7,ecx
        ;mov    silo8,ecx

        jmp    siagain16 ; flush code cache
siagain16:
        IS2F   0,silo1,sihi1
        IS2F   1,silo2,sihi2
        IS2F   2,silo3,sihi3
        IS2F   3,silo4,sihi4
        ;IS2F   4,silo5,sihi5
        ;IS2F   5,silo6,sihi6
        ;IS2F   6,silo7,sihi7
        ;IS2F   7,silo8,sihi8
        add    edi,(8*4)
        dec    ebp
        jnz    siagain16
siskip16:
        pop    ebp
        and    ebp,3
        jz     siskip1
siagain1:
        IS2F   0,silo9,sihi9
        add    edi,8
        dec    ebp
        jnz    siagain1
siskip1:
        STUBEND
        ret


_Asm16SurroundNormal:
        STUBSTART
        mov    esi,[esp+32] ; get src
        mov    edi,[esp+36] ; get dst
        mov    ebx,[esp+40] ; get index_lo
        mov    edx,[esp+44] ; get index_hi
        mov    ecx,[esp+48] ; get increment_lo
        mov    eax,[esp+52] ; get increment_hi
        mov    ebp,[esp+56] ; get todo

        shl    edx,1
        mov    s2hi9,eax
        add    edx,esi
        mov    s2lo9,ecx
        sar    edx,1

        push   ebp
        shr    ebp,3
        jz     s2skip16

        mov    s2hi1,eax
        mov    s2hi2,eax
        mov    s2hi3,eax
        mov    s2hi4,eax
        mov    s2hi5,eax
        mov    s2hi6,eax
        mov    s2hi7,eax
        mov    s2hi8,eax
        mov    s2lo1,ecx
        mov    s2lo2,ecx
        mov    s2lo3,ecx
        mov    s2lo4,ecx
        mov    s2lo5,ecx
        mov    s2lo6,ecx
        mov    s2lo7,ecx
        mov    s2lo8,ecx

        jmp    s2again16 ; flush code cache

s2again16:
        SS3F   0,s2lo1,s2hi1
        SS3F   1,s2lo2,s2hi2
        SS3F   2,s2lo3,s2hi3
        SS3F   3,s2lo4,s2hi4
        SS3F   4,s2lo5,s2hi5
        SS3F   5,s2lo6,s2hi6
        SS3F   6,s2lo7,s2hi7
        SS3F   7,s2lo8,s2hi8
        add    edi,(8*8)
        dec    ebp
        jnz    s2again16
s2skip16:
        pop    ebp
        and    ebp,7
        jz     s2skip1
s2again1:
        SS3F   0,s2lo9,s2hi9
        add    edi,8
        dec    ebp
        jnz    s2again1
s2skip1:
        STUBEND
        ret


_Asm16SurroundInterp:
        STUBSTART
        mov    esi,[esp+32] ; get src
        mov    edi,[esp+36] ; get dst
        mov    ebx,[esp+40] ; get index_lo
        mov    edx,[esp+44] ; get index_hi
        mov    ecx,[esp+48] ; get increment_lo
        mov    eax,[esp+52] ; get increment_hi
        mov    ebp,[esp+56] ; get todo

        shl    edx,1
        mov    si2hi9,eax
        add    edx,esi
        mov    si2lo9,ecx
        sar    edx,1

        push   ebp
        shr    ebp,2
        jz     si2skip16

        mov    si2hi1,eax
        mov    si2hi2,eax
        mov    si2hi3,eax
        mov    si2hi4,eax
        ;mov    si2hi5,eax
        ;mov    si2hi6,eax
        ;mov    si2hi7,eax
        ;mov    si2hi8,eax
        mov    si2lo1,ecx
        mov    si2lo2,ecx
        mov    si2lo3,ecx
        mov    si2lo4,ecx
        ;mov    si2lo5,ecx
        ;mov    si2lo6,ecx
        ;mov    si2lo7,ecx
        ;mov    si2lo8,ecx

        jmp    si2again16 ; flush code cache

si2again16:
        IS3F   0,si2lo1,si2hi1
        IS3F   1,si2lo2,si2hi2
        IS3F   2,si2lo3,si2hi3
        IS3F   3,si2lo4,si2hi4
        ;IS3F   4,si2lo5,si2hi5
        ;IS3F   5,si2lo6,si2hi6
        ;IS3F   6,si2lo7,si2hi7
        ;IS3F   7,si2lo8,si2hi8
        add    edi,(8*4)
        dec    ebp
        jnz    si2again16
si2skip16:
        pop    ebp
        and    ebp,3
        jz     si2skip1
si2again1:
        IS3F   0,si2lo9,si2hi9
        add    edi,8
        dec    ebp
        jnz    si2again1
sI2skip1:
        STUBEND
        ret


_Asm16MonoNormal:
        STUBSTART
        mov    esi,[esp+32] ; get src
        mov    edi,[esp+36] ; get dst
        mov    ebx,[esp+40] ; get index_lo
        mov    edx,[esp+44] ; get index_hi
        mov    ecx,[esp+48] ; get increment_lo
        mov    eax,[esp+52] ; get increment_hi
        mov    ebp,[esp+56] ; get todo

        shl    edx,1
        mov    mhi9,eax
        add    edx,esi
        mov    mlo9,ecx
        sar    edx,1

        push   ebp
        shr    ebp,3
        jz     mskip16

        mov    mhi1,eax
        mov    mhi2,eax
        mov    mhi3,eax
        mov    mhi4,eax
        mov    mhi5,eax
        mov    mhi6,eax
        mov    mhi7,eax
        mov    mhi8,eax
        mov    mlo1,ecx
        mov    mlo2,ecx
        mov    mlo3,ecx
        mov    mlo4,ecx
        mov    mlo5,ecx
        mov    mlo6,ecx
        mov    mlo7,ecx
        mov    mlo8,ecx

        jmp    magain16 ; flush code cache

magain16:
        SM2F   0,mlo1,mhi1
        SM2F   1,mlo2,mhi2
        SM2F   2,mlo3,mhi3
        SM2F   3,mlo4,mhi4
        SM2F   4,mlo5,mhi5
        SM2F   5,mlo6,mhi6
        SM2F   6,mlo7,mhi7
        SM2F   7,mlo8,mhi8
        add    edi,(8*4)
        dec    ebp
        jnz    magain16
mskip16:
        pop    ebp
        and    ebp,7
        jz     mskip1
magain1:
        SM2F   0,mlo9,mhi9
        add    edi,4
        dec    ebp
        jnz    magain1
mskip1:
        STUBEND
        ret


_Asm16MonoInterp:
        STUBSTART
        mov    esi,[esp+32] ; get src
        mov    edi,[esp+36] ; get dst
        mov    ebx,[esp+40] ; get index_lo
        mov    edx,[esp+44] ; get index_hi
        mov    ecx,[esp+48] ; get increment_lo
        mov    eax,[esp+52] ; get increment_hi
        mov    ebp,[esp+56] ; get todo

        shl    edx,1
        mov    mihi9,eax
        add    edx,esi
        mov    milo9,ecx
        sar    edx,1

        push   ebp
        shr    ebp,2
        jz     miskip16

        mov    mihi1,eax
        mov    mihi2,eax
        mov    mihi3,eax
        mov    mihi4,eax
        ;mov    mihi5,eax
        ;mov    mihi6,eax
        ;mov    mihi7,eax
        ;mov    mihi8,eax
        mov    milo1,ecx
        mov    milo2,ecx
        mov    milo3,ecx
        mov    milo4,ecx
        ;mov    milo5,ecx
        ;mov    milo6,ecx
        ;mov    milo7,ecx
        ;mov    milo8,ecx

        jmp    miagain16 ; flush code cache
miagain16:
        IM2F   0,milo1,mihi1
        IM2F   1,milo2,mihi2
        IM2F   2,milo3,mihi3
        IM2F   3,milo4,mihi4
        ;IM2F   4,milo5,mihi5
        ;IM2F   5,milo6,mihi6
        ;IM2F   6,milo7,mihi7
        ;IM2F   7,milo8,mihi8
        add    edi,(4*4)
        dec    ebp
        jnz    miagain16
miskip16:
        pop    ebp
        and    ebp,3
        jz     miskip1
miagain1:
        IM2F   0,milo9,mihi9
        add    edi,4
        dec    ebp
        jnz    miagain1
miskip1:
        STUBEND
        ret

        END
