
#include "fileio.h"

using namespace MikMod;

BinaryStream::BinaryStream(std::istream& s)
: stream(s)
{
}

BinaryStream& BinaryStream::operator >> (u8& dest)
{
    stream.read((char*)&dest,1);

    return *this;
}

BinaryStream& BinaryStream::operator >> (u16& dest)
{
    u8 b[2];

    stream.read((char*)b,2);

    dest=(b[1]<<16)|b[0];
    return *this;
}

BinaryStream& BinaryStream::operator >> (u32& dest)
{
    u8 b[4];

    stream.read((char*)b,4);

    dest=(b[3]<<24) | (b[2]<<16) | (b[1]<<8) | b[0];
    return *this;
}

void BinaryStream::read(void* dest,int numbytes)
{
    stream.read((char*)dest,numbytes);
}