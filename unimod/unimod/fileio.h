
/*
    Useful things for file i/o in an endian independant manner.

    Further, it's just better syntax. ^_^
*/

#pragma once

#include <iostream>
#include "types.h"

namespace MikMod
{
    // input only.  Consider an output version?

    class BinaryStream
    {
    private:
        std::istream& stream;
    public:

        BinaryStream(std::istream& s);

        BinaryStream& operator >> (u8& dest);
        BinaryStream& operator >> (u16& dest);
        BinaryStream& operator >> (u32& dest);

        void read(void* dest,int numbytes);
        
    };
};