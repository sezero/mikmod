
#include "unimod.h"

u16 MikMod::FineTune(u16 w)
{
    static u16 finetunexlat[] = 
    {   
        8363,   8413,   8463,   8529,   8581,   8651,   8723,   8757,
        7895,   7941,   7985,   8046,   8107,   8169,   8232,   8280
    };

    return finetunexlat[w&0x0F];
}