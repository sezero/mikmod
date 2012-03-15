/*

 Mikmod Sound System - The Legend Continues

  By Jake Stine and Hour 13 Studios (1996-2002)
  Original code & concepts by Jean-Paul Mikkers (1993-1996)

 Support:
  If you find problems with this code, send mail to:
    air@hour13.com
  For additional information and updates, see our website:
    http://www.hour13.com

 ---------------------------------------------------
 dec_vorbis.c

 Ogg Vorbis, one of the more brilliantly convoluted APIs of all time, at your service.
 This baby makes PNG and JPEG look *SIMPLE*.

*/

#include "mikmod.h"
#include "vorbis\vorbisfile.h"

#include <assert.h>

//#define _VORBIS_COMMENTS_

// -------------------------------------------------------------------------------------
    typedef struct VORBIS_HANDLE
// -------------------------------------------------------------------------------------
{
    uint             length;  // remaining length of the vorbis buffer in bytes
    int              samples;
    float          **pcm;

    ogg_sync_state   oy;      // sync and verify incoming physical bitstream
    ogg_stream_state os;      // take physical pages, weld into a logical stream of packets
    ogg_page         og;      // one Ogg bitstream page.  Vorbis packets are inside
    ogg_packet       op;      // one raw packet of data for decode

    vorbis_info      vi;      // struct that stores all the static vorbis bitstream settings
#ifdef _VORBIS_COMMENTS_
    vorbis_comment   vc;      // struct that stores all the bitstream user comments
#endif
    vorbis_dsp_state vd;      // central working state for the packet->PCM decoder
    vorbis_block     vb;      // local working space for packet->PCM decode

} VORBIS_HANDLE;


// =====================================================================================
    static VORBIS_HANDLE *Vorbis_Init(MMSTREAM *mmfp)
// =====================================================================================
// It's assumed that the module has ocnfirmed our sample type, so no detection code
// is needed.  Just error out if bad things happen!
{
    VORBIS_HANDLE   *vh     = _mmobj_array(NULL, 1, VORBIS_HANDLE);
    UBYTE           *buffer;
    uint             synclen, i;

    vh->length  = _mm_read_I_ULONG(mmfp);

    ogg_sync_init(&vh->oy);
 
    // submit a 4k block to libvorbis' Ogg layer

    buffer      = ogg_sync_buffer(&vh->oy, 4096);
    synclen     = MIN(4096, vh->length);
    _mm_read_UBYTES(buffer, synclen, mmfp);
    vh->length -= synclen;
    ogg_sync_wrote(&vh->oy, synclen);

    // Get the first page.

    if(ogg_sync_pageout(&vh->oy, &vh->og) != 1)
    {
        _mmlog("Mikmod > dec_vorbis > Bad Mojo Error!");
        assert(FALSE);
        return NULL;
    }

    ogg_stream_init(&vh->os, ogg_page_serialno(&vh->og));

    /* extract the initial header from the first page and verify that the
       Ogg bitstream is in fact Vorbis data */

    vorbis_info_init(&vh->vi);
#ifdef _VORBIS_COMMENTS_
    vorbis_comment_init(&vh->vc);
#endif
    if(ogg_stream_pagein(&vh->os,&vh->og) < 0)
    { 
        _mmlog("Mikmod > dec_vorbis > Error reading first page of Ogg bitstream data.");
        assert(FALSE);
        return NULL;
    }

    if(ogg_stream_packetout(&vh->os,&vh->op) != 1)
    { 
        _mmlog("Mikmod > dec_vorbis > Error reading initial header packet.");
        assert(FALSE);
        return NULL;
    }

#ifdef _VORBIS_COMMENTS_
    if(vorbis_synthesis_headerin(&vh->vi, &vh->vc, &vh->op) < 0)
#else
    if(vorbis_synthesis_headerin(&vh->vi, &vh->op) < 0)
#endif
    {
        _mmlog("Mikmod > dec_vorbis > This Ogg bitstream does not contain Vorbis audio data.");
        assert(FALSE);
        return NULL;
    }

    /* The next two packets in order are the comment and codebook headers.
       They're likely large and may span multiple pages.  Thus we read
       and submit data until we get our two pacakets, watching that no
       pages are missing.  If a page is missing, error out; losing a
       header page is the only place where missing data is fatal. */

    i=0;
    while(i < 2)
    {
        while(i < 2)
        {
	        int     result  = ogg_sync_pageout(&vh->oy, &vh->og);
	        if(result==0) break;

	        /* Don't complain about missing or corrupt data yet.  We'll
	           catch it at the packet output phase */
	
            if(result == 1)
            {
	            ogg_stream_pagein(&vh->os,&vh->og);
	            while(i < 2)
                {
	                result  = ogg_stream_packetout(&vh->os, &vh->op);
                    if(result==0) break;
	    
                    if(result < 0)
                    {   _mmlog("Mikmod > dec_vorbis > More bad mojo .. check your data stream!");
                        assert(FALSE);
                        return NULL;
                    }
#ifdef _VORBIS_COMMENTS_
	                vorbis_synthesis_headerin(&vh->vi,&vh->vc,&vh->op);
#else
	                vorbis_synthesis_headerin(&vh->vi,&vh->op);
#endif
	                i++;
	            }
	        }
        }

        // no harm in not checking before adding more

        buffer       = ogg_sync_buffer(&vh->oy, 4096);
        synclen      = MIN(4096, vh->length);

        if(synclen==0 && (i < 2))
        {   _mmlog("Mikmod > dec_vorbis > End of file before finding all Vorbis headers!");
            assert(FALSE);
            return NULL;
        }

        _mm_read_UBYTES(buffer, synclen, mmfp);
        vh->length -= synclen;
        ogg_sync_wrote(&vh->oy, synclen);
    }

    // Initialize the Vorbis packet->PCM decoder.

    vorbis_synthesis_init(&vh->vd, &vh->vi);
    vorbis_block_init(&vh->vd, &vh->vb);    /* local state for most of the decode
                                       so multiple block decodes can
                                       proceed in parallel.  We could init
                                       multiple vorbis_block structures
                                       for vd here */

    return vh;
}


// =====================================================================================
    static void Vorbis_Cleanup(VORBIS_HANDLE *vh)
// =====================================================================================
{
    ogg_stream_clear(&vh->os);

    vorbis_block_clear(&vh->vb);
    vorbis_dsp_clear(&vh->vd);
#ifdef _VORBIS_COMMENTS_
	vorbis_comment_clear(&vh->vc);
#endif
    vorbis_info_clear(&vh->vi);

    /* OK, clean up the framer */
    ogg_sync_clear(&vh->oy);

    _mm_free(NULL, vh);
}


// =====================================================================================
    static BOOL Decompress16Bit(VORBIS_HANDLE *vh, SWORD *dest, int cbcount, MMSTREAM *mmfp)
// =====================================================================================
// cbcount - number of bytes to process this iteration!
{
    uint          synclen, cbrun = cbcount;
    UBYTE        *buffer;
    int           eos = 0;

    while(cbrun && !eos)
    {
        while(cbrun && !eos)
        {
	        int result;
            
            if(!vh->samples)
            {
                result  = ogg_sync_pageout(&vh->oy, &vh->og);
	            if(result==0) break; /* need more data */
	            assert(result >= 0);

                ogg_stream_pagein(&vh->os, &vh->og);
            }

            while(cbrun)
            {
	            while(cbrun && vh->samples)
                {
                    const int convsize  = cbrun / vh->vi.channels;
                    uint    bout        = MIN(vh->samples, convsize);
                    int     i;

                    /* convert floats to 16 bit signed ints (host order) and
                       interleave */

                    if(bout > cbrun)
                        bout = cbrun;

                    for(i=0; i<vh->vi.channels; i++)
                    {
    		            uint         j;
                        SWORD       *ptr    = dest + i;
                        float       *mono   = vh->pcm[i];

                        for(j=0; j<bout; j++)
                        {
	                        int val = mono[j] * 32767.f;
	                        *ptr    = _mm_boundscheck(val, -32768, 32767);
	                        ptr    += vh->vi.channels;
                        }
                    }

		            vorbis_synthesis_read(&vh->vd, bout);
                    cbrun        -= bout * vh->vi.channels;
                    dest         += bout * vh->vi.channels;
                    vh->samples  -= bout;
	            }
                
                if(!cbrun)
                    return cbcount;

                result  = ogg_stream_packetout(&vh->os, &vh->op);
	            if(result==0) break; /* need more data */
                assert(result >= 0);

	            if(vorbis_synthesis(&vh->vb, &vh->op) == 0)
		            vorbis_synthesis_blockin(&vh->vd,&vh->vb);

                /* pcm is a multichannel float vector.  In stereo, for
                   example, pcm[0] is left, and pcm[1] is right.  samples is
                   the size of each channel.  Convert the float values
                   (-1.<=range<=1.) to whatever PCM format and write it out */

                vh->samples = vorbis_synthesis_pcmout(&vh->vd, &vh->pcm);
	        }
    	    //if(ogg_page_eos(&vh->og))
                //eos = 1;
        }

        if(!eos && cbrun)
        {
            buffer      = ogg_sync_buffer(&vh->oy, 4096);
            synclen     = MIN(4096, vh->length);
            if(synclen > cbrun) synclen = cbrun;

            if(synclen == 0)
            {   eos = 1;
                //assert(FALSE);
                break;  }

            _mm_read_UBYTES(buffer, synclen, mmfp);
            vh->length -= synclen;
            ogg_sync_wrote(&vh->oy, synclen);
        }
    }

    return cbcount;
}


SL_DECOMPRESS_API dec_vorbis =
{
    NULL,
    SL_COMPRESS_VORBIS,
    Vorbis_Init,
    Vorbis_Cleanup,
    Decompress16Bit,
    NULL,
};
