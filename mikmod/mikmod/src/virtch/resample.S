/* GCC port of mixing code by Barubary (barubary@dal.net)
   This should work on both Linux and DJGPP...  I have confirmed that
   self-modifying code works just fine in Linux without causing signals.
   This may change in the future, so I recommend that the Linux MikMod
   developers look out for new kernel releases.  The critical thing is that
   CS.base = DS.base in Linux, which makes DS an alias for CS, and that the
   code pages remain writable. */

.data          /* self-modifying code... keep in data segment */

#ifdef OSS
#define _AsmStereoNormal AsmStereoNormal
#define _AsmStereoSurround AsmStereoSurround
#define _AsmMonoNormal AsmMonoNormal
#define _lvoltab lvoltab
#define _rvoltab rvoltab
#endif

    .globl  _AsmStereoNormal
    .globl  _AsmStereoSurround
    .globl  _AsmMonoNormal


#define STUBSTART \
    pushl   %eax; \
    pushl   %ebx; \
    pushl   %ecx; \
    pushl   %edx; \
    pushl   %esi; \
    pushl   %edi; \
    pushl   %ebp

#define STUBEND \
    popl    %ebp; \
    popl    %edi; \
    popl    %esi; \
    popl    %edx; \
    popl    %ecx; \
    popl    %ebx; \
    popl    %eax

#define SS2F(index,lab1,lab2,lab3,lab4) \
    movb (%edx),%al; \
    .byte 0x081,0x0c3; \
    lab1: .int 0; \
    .byte 0x081,0x0d2; \
    lab2: .int 0; \
    movl index*8(%edi),%esi; \
    .byte 0x08b,0x0c,0x085; \
    lab3: .int 0; \
    movl index*8+4(%edi),%ebp; \
    addl %ecx,%esi; \
    .byte 0x08b,0x0c,0x085; \
    lab4: .int 0; \
    movl %esi,index*8(%edi); \
    addl %ecx,%ebp

#define SS2M(index,lab1,lab2,lab3,lab4) \
    movb (%edx),%al; \
    movl %ebp,index*8-4(%edi); \
    .byte 0x081,0x0c3; \
    lab1: .int 0; \
    .byte 0x081,0x0d2; \
    lab2: .int 0; \
    movl index*8(%edi),%esi; \
    .byte 0x08b,0x0c,0x085; \
    lab3: .int 0; \
    movl index*8+4(%edi),%ebp; \
    addl %ecx,%esi; \
    .byte 0x08b,0x0c,0x085; \
    lab4: .int 0; \
    movl %esi,index*8(%edi); \
    addl %ecx,%ebp

#define SS3F(index,lab1,lab2,lab3) \
    movb (%edx),%al; \
    .byte 0x081,0x0c3; \
    lab1: .int 0; \
    .byte 0x081,0x0d2; \
    lab2: .int 0; \
    movl index*8(%edi),%esi; \
    .byte 0x08b,0x0c,0x085; \
    lab3: .int 0; \
    movl index*8+4(%edi),%ebp; \
    addl %ecx,%esi; \
    subl %ecx,%ebp; \
    movl %esi,index*8(%edi); \

#define SS3M(index,lab1,lab2,lab3) \
    movb (%edx),%al; \
    movl %ebp,index*8-4(%edi); \
    .byte 0x081,0x0c3; \
    lab1: .int 0; \
    .byte 0x081,0x0d2; \
    lab2: .int 0; \
    movl index*8(%edi),%esi; \
    .byte 0x08b,0x0c,0x085; \
    lab3: .int 0; \
    movl index*8+4(%edi),%ebp; \
    addl %ecx,%esi; \
    subl %ecx,%ebp; \
    movl %esi,index*8(%edi); \

#define SS2L(index) \
    movl %ebp,index*8-4(%edi)

#define SM2F(index,lab1,lab2,lab3) \
    movb (%edx),%al; \
    .byte 0x081,0x0c3; \
    lab1: .int 0; \
    .byte 0x081,0x0d2; \
    lab2: .int 0; \
    movl index*4(%edi),%esi; \
    .byte 0x08b,0x0c,0x085; \
    lab3: .int 0

#define SM2M(index,lab1,lab2,lab3) \
    movb (%edx),%al; \
    addl %ecx,%esi; \
    .byte 0x081,0x0c3; \
    lab1: .int 0; \
    movl %esi,index*4-4(%edi); \
    .byte 0x081,0x0d2; \
    lab2: .int 0; \
    movl index*4(%edi),%esi; \
    .byte 0x08b,0x0c,0x085; \
    lab3: .int 0

#define SM2L(index) \
    addl %ecx,%esi; \
    movl %esi,index*4-4(%edi)


; -----------------------------------------
;        Stereo Interpolation Macros
; -----------------------------------------

IS2F MACRO index,lab1,lab2,lab3,lab4 ; 486+



_AsmStereoNormal:
    STUBSTART       
    movl    32(%esp),%esi   /* get src */
    movl    36(%esp),%edi   /* get dst */
    movl    40(%esp),%ebx   /* get index_lo */
    movl    44(%esp),%edx   /* get index_hi */
    movl    48(%esp),%ecx   /* get increment_lo */
    movl    52(%esp),%eax   /* get increment_hi */
    movl    56(%esp),%ebp   /* get todo */
    addl    %esi,%edx
    movl    %eax,shi1
    movl    %eax,shi2
    movl    %eax,shi3
    movl    %eax,shi4
    movl    %eax,shi5
    movl    _lvoltab,%eax
    movl    %ecx,slo1
    movl    %ecx,slo2
    movl    %ecx,slo3
    movl    %ecx,slo4
    movl    %ecx,slo5
    movl    %eax,sltab1
    movl    %eax,sltab2
    movl    %eax,sltab3
    movl    %eax,sltab4
    movl    %eax,sltab5
    movl    _rvoltab,%eax
    pushl   %ebp
    movl    %eax,srtab1
    movl    %eax,srtab2
    movl    %eax,srtab3
    movl    %eax,srtab4
    movl    %eax,srtab5
    xorl    %eax,%eax
    jmp s1  /* flush code cache */
s1:             
    shrl    $2,%ebp
    jz      sskip16
    pushl   %ebp
sagain16:               
    SS2F(0,slo1,shi1,sltab1,srtab1)
    SS2M(1,slo2,shi2,sltab2,srtab2)
    SS2M(2,slo3,shi3,sltab3,srtab3)
    SS2M(3,slo4,shi4,sltab4,srtab4)
    SS2L(4)
    addl    $32,%edi
    decl    (%esp)
    jnz     sagain16
    popl    %ebp
sskip16:                
    popl    %ebp
    andl    $3,%ebp
    jz      sskip1
    pushl   %ebp
sagain1:                
    SS2F(0,slo5,shi5,sltab5,srtab5)
    SS2L(1)
    addl    $8,%edi
    decl (%esp)
    jnz     sagain1
    popl    %ebp
sskip1:         
    STUBEND 
    ret     


_AsmStereoSurround:
    STUBSTART       
    movl    32(%esp),%esi   /* get src */
    movl    36(%esp),%edi   /* get dst */
    movl    40(%esp),%ebx   /* get index_lo */
    movl    44(%esp),%edx   /* get index_hi */
    movl    48(%esp),%ecx   /* get increment_lo */
    movl    52(%esp),%eax   /* get increment_hi */
    movl    56(%esp),%ebp   /* get todo */
    addl    %esi,%edx
    movl    %eax,s2hi1
    movl    %eax,s2hi2
    movl    %eax,s2hi3
    movl    %eax,s2hi4
    movl    %eax,s2hi5
    movl    _lvoltab,%eax
    movl    %ecx,s2lo1
    movl    %ecx,s2lo2
    movl    %ecx,s2lo3
    movl    %ecx,s2lo4
    movl    %ecx,s2lo5
    movl    %eax,s2ltab1
    movl    %eax,s2ltab2
    movl    %eax,s2ltab3
    movl    %eax,s2ltab4
    movl    %eax,s2ltab5
    pushl   %ebp
    xorl    %eax,%eax
    jmp s3  /* flush code cache */
s3:             
    shrl    $2,%ebp
    jz      s2skip16
    pushl   %ebp
s2again16:               
    SS3F(0,s2lo1,s2hi1,s2ltab1)
    SS3M(1,s2lo2,s2hi2,s2ltab2)
    SS3M(2,s2lo3,s2hi3,s2ltab3)
    SS3M(3,s2lo4,s2hi4,s2ltab4)
    SS2L(4)
    addl    $32,%edi
    decl    (%esp)
    jnz     s2again16
    popl    %ebp
s2skip16:                
    popl    %ebp
    andl    $3,%ebp
    jz      s2skip1
    pushl   %ebp
s2again1:                
    SS3F(0,s2lo5,s2hi5,s2ltab5)
    SS2L(1)
    addl    $8,%edi
    decl    (%esp)
    jnz     s2again1
    popl    %ebp
s2skip1:         
    STUBEND 
    ret     


_AsmMonoNormal:
    STUBSTART       
    movl    32(%esp),%esi   /* get src */
    movl    36(%esp),%edi   /* get dst */
    movl    40(%esp),%ebx   /* get index_lo */
    movl    44(%esp),%edx   /* get index_hi */
    movl    48(%esp),%ecx   /* get increment_lo */
    movl    52(%esp),%eax   /* get increment_hi */
    movl    56(%esp),%ebp   /* get todo */
    addl    %esi,%edx
    movl    %eax,mhi1
    movl    %eax,mhi2
    movl    %eax,mhi3
    movl    %eax,mhi4
    movl    %eax,mhi5
    movl    _lvoltab,%eax
    movl    %ecx,mlo1
    movl    %ecx,mlo2
    movl    %ecx,mlo3
    movl    %ecx,mlo4
    movl    %ecx,mlo5
    movl    %eax,mltab1
    movl    %eax,mltab2
    movl    %eax,mltab3
    movl    %eax,mltab4
    movl    %eax,mltab5
    xorl    %eax,%eax

    jmp m1  /* flush code cache */
m1:             
    pushl   %ebp
    shrl    $2,%ebp
    jz      mskip16
magain16:               
    SM2F(0,mlo1,mhi1,mltab1)
    SM2M(1,mlo2,mhi2,mltab2)
    SM2M(2,mlo3,mhi3,mltab3)
    SM2M(3,mlo4,mhi4,mltab4)
    SM2L(4)
    addl    $16,%edi
    decl    %ebp
    jnz     magain16
mskip16:                
    popl    %ebp
    andl    $3,%ebp
    jz      mskip1
magain1:                
    SM2F(0,mlo5,mhi5,mltab5)
    SM2L(1)
    addl    $4,%edi
    decl    %ebp
    jnz     magain1
mskip1:         
    STUBEND 
    ret     


