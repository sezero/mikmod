
#include "fileio.h"
#include <algorithm>

using namespace MikMod;

BinaryStream::BinaryStream(std::istream& s)
: stream(s), bBigendian(false)
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

    (*this) >> b[0] >> b[1];

    if (bBigendian)
        std::swap(b[0],b[1]);

    dest=(b[1]<<8)|b[0];
    return *this;
}

BinaryStream& BinaryStream::operator >> (u32& dest)
{
    u8 b[4];

    stream.read((char*)b,4);

    if (bBigendian)
        dest=(b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[0];
    else
        dest=(b[3]<<24) | (b[2]<<16) | (b[1]<<8) | b[0];
    return *this;
}

void BinaryStream::read(void* dest,int numbytes)
{
    stream.read((char*)dest,numbytes);
}