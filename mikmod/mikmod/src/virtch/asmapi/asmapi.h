#ifndef _ASMAPI_H_
#define _ASMAPI_H_

// Define external Assembly Language Prototypes
// ============================================

#ifdef __cplusplus
extern "C" {
#endif

void __cdecl AsmStereoNormal(SBYTE *srce,SLONG *dest,INT64S index,INT64S increment,SLONG todo);
void __cdecl AsmStereoInterp(SBYTE *srce,SLONG *dest,INT64S index,INT64S increment,SLONG todo);
void __cdecl AsmSurroundNormal(SBYTE *srce,SLONG *dest,INT64S index,INT64S increment,SLONG todo);
void __cdecl AsmSurroundInterp(SBYTE *srce,SLONG *dest,INT64S index,INT64S increment,SLONG todo);
void __cdecl AsmMonoNormal(SBYTE *srce,SLONG *dest,INT64S index,INT64S increment,SLONG todo);
void __cdecl AsmMonoInterp(SBYTE *srce,SLONG *dest,INT64S index,INT64S increment,SLONG todo);

void __cdecl Asm16StereoInterp(SBYTE *srce,SLONG *dest,INT64S index,INT64S increment,SLONG todo);
void __cdecl Asm16SurroundInterp(SBYTE *srce,SLONG *dest,INT64S index,INT64S increment,SLONG todo);
void __cdecl Asm16MonoInterp(SBYTE *srce,SLONG *dest,INT64S index,INT64S increment,SLONG todo);
void __cdecl Asm16StereoNormal(SBYTE *srce,SLONG *dest,INT64S index,INT64S increment,SLONG todo);
void __cdecl Asm16SurroundNormal(SBYTE *srce,SLONG *dest,INT64S index,INT64S increment,SLONG todo);
void __cdecl Asm16MonoNormal(SBYTE *srce,SLONG *dest,INT64S index,INT64S increment,SLONG todo);

void __cdecl AsmStereoSS(SBYTE *srce,SLONG *dest,INT64S index,INT64S increment,SLONG todo);
void __cdecl AsmStereoSSI(SBYTE *srce,SLONG *dest,INT64S index,INT64S increment,SLONG todo);
void __cdecl AsmMonoSS(SBYTE *srce,SLONG *dest,INT64S index,INT64S increment,SLONG todo);
void __cdecl AsmMonoSSI(SBYTE *srce,SLONG *dest,INT64S index,INT64S increment,SLONG todo);

void __cdecl Asm16StereoSS(SBYTE *srce,SLONG *dest,INT64S index,INT64S increment,SLONG todo);
void __cdecl Asm16StereoSSI(SBYTE *srce,SLONG *dest,INT64S index,INT64S increment,SLONG todo);
void __cdecl Asm16MonoSS(SBYTE *srce,SLONG *dest,INT64S index,INT64S increment,SLONG todo);
void __cdecl Asm16MonoSSI(SBYTE *srce,SLONG *dest,INT64S index,INT64S increment,SLONG todo);

#ifdef __cplusplus
};
#endif

#endif
