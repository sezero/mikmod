/*
 ******************************************************************
 * ADPCMod - MOD ADPCM library by RGR                             *
 * Decompresses ADPCM compressed MOD samples (MODPlug's ADPCM)    *
 *                                                                *
 * (C) 2001 by Ricardo Ramalho - No modifications                 *
 ******************************************************************
 */

void __cdecl DeADPCM(void *table, void *input, unsigned long size, void *output);

/* Decompresses an ADPCM sample using the 16 byte TABLE, and the SIZE byte
INPUT buffer. Uncompressed sample is placed on the pre-allocated OUTPUT buffer.
Output length is SIZE*2 bytes.*/

unsigned long ADPCModVer();
/* Returns version information about this lib. Format is: LSW - Build,
MSW: MSB - Major Version, LSB - Minor Version.
Eg. 0x01030063 is v1.3 build 99. */
