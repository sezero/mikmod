
#include "types.h"
#include "ISoundDriver.h"
#include <dsound.h>

namespace MikMod
{
    class DirectSoundBuffer;

    class DirectSoundDriver : public ISoundDriver
    {
        IDirectSound8*  pDirectsound;

        // keep a list of ISoundBuffers this object has created?
        int nBuffersallocated;

    public:
        DirectSoundDriver();
        ~DirectSoundDriver();

        virtual ISoundBuffer* CreateSoundBuffer(u8* sampledata,u32 length,std::bitset<32> flags);
        virtual void FreeSoundBuffer(ISoundBuffer* s);

        virtual void Update();
    };

    class DirectSoundBuffer : public ISoundBuffer
    {
        IDirectSoundBuffer8* pBuffer;
    public:

        DirectSoundBuffer(u8* sampledata,u32 length,std::bitset<32> flags);
        ~DirectSoundBuffer();
    };
};