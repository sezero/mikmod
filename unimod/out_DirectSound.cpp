
#include "out_DirectSound.h"

using namespace MikMod;

DirectSoundDriver::DirectSoundDriver()
{
    nBuffersallocated=0;        // keep a count, for debugging purposes.

    HRESULT result=DirectSoundCreate8(NULL,&pDirectsound,NULL);
    // if failed result blah blah
}

DirectSoundDriver::~DirectSoundDriver()
{
    pDirectsound->Release();
}

//---------------------------------------------------------------------------

DirectSoundBuffer::DirectSoundBuffer(u8* sampledata,u32 length,std::bitset<32> flags)
{
    CDSBUFFERDESC bufferdesc;

    bufferdesc.dwSize=sizeof bufferdesc;
    bufferdesc.dwFlags=DSBCAPS_CTRLFREQUENCY;
    bufferdesc.dwBufferBytes=length;
    bufferdesc.dwReserved=0;


    HRESULT result=pDirectsound->CreateSoundBuffer(&bufferdesc,&pBuffer,NULL);
}