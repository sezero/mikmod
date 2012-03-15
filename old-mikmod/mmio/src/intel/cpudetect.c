//#include <shlobj.h>

#include "mmio.h"

static int has_mmx, has_3dnow, has_pro;

#ifndef EXCEPTION_EXECUTE_HANDLER
#define EXCEPTION_EXECUTE_HANDLER 1
#endif

// ======================================================================================
    static void __inline newGetProcessorType(ULONG e, ULONG *a, ULONG *d)
// ======================================================================================
{ 
    uint retval1,retval2;

    __try
    {   _asm
        {
            mov eax, e              // set up CPUID to return processor version and features
                                    //      0 = vendor string, 1 = version info, 2 = cache info
            _emit 0x0f              // code bytes = 0fh,  0a2h
            _emit 0xa2
            mov retval1, eax                
            mov retval2, edx
        }       
    } __except(EXCEPTION_EXECUTE_HANDLER) { retval1 = retval2= 0;}

    *a = retval1;
    *d = retval2;
}

// ======================================================================================
    uint _mm_cpudetect(void)
// ======================================================================================
{
    ULONG a, d;

    newGetProcessorType(1,&a,&d);
    if (!a) has_mmx = has_3dnow = has_pro = 0;
    else
    {   has_mmx = (d & 0x800000) ? 1 : 0;
        has_pro = (d & (1<<15))  ? 1 : 0;
        newGetProcessorType(0x80000000,&a,&d);
        if (a > 0x80000000)
        {   newGetProcessorType(0x80000001,&a,&d);
            has_3dnow = (d & 0x80000000) ? 1 : 0;
        } else has_3dnow = 0;
    }

    return (has_mmx ? CPU_MMX : 0) | (has_3dnow ? CPU_3DNOW : 0);
}


