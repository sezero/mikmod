/*
    Sound driver interface.


    Currently, Unimods own their samples and instruments exclusively.  There is no way to share them between mods at present.
    Must implement a better way of storing them, so that it can be done.

    I'm not sure I like this setup.  The ISoundDriver implementation will have to cast an ISoundBuffer to whatever implementation matches
    the driver.  Icky.  Ordinal handles? (ickier, but safer too)
*/

#pragma once

namespace MikMod
{

    class ISoundBuffer;

    class ISoundDriver
    {
    public:
        // Sample factory
        virtual ISoundBuffer* CreateSoundBuffer(u8* sampledata,u32 length,std::bitset<32> flags)=0;
        virtual void FreeSoundBuffer(ISoundBuffer* s)=0;

        // Driver stuffs
        virtual void Update()=0;

        // Cleanup.
        virtual ~ISoundDriver() {}
    };

    // Platform/API specific buffer for holding a hunk of sound data.
    class ISoundBuffer
    {
        // uhm... @_x
    };

};