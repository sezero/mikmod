//#include "adpcmod.h"

        #ifdef MM_SUPPORT_ADPCM
        } else if(smp->decompress == DECOMPRESS_ADPCM)
        {
            UBYTE  table[16], *input;

            input = _mm_malloc(sl_allochandle, (length+1)/2);
            _mm_read_UBYTES(table,16,mmfp);
            _mm_read_UBYTES(input,(length+1)/2,mmfp);

            DeADPCM(table, input, (length+1)/2, (UBYTE *)sl_buffer);
            _mm_free(sl_allochandle, input);

            todo   = length;
            inlen  = outlen = length;
        #endif
