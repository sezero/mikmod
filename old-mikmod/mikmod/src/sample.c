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
 sample.c

 Sample Loading API

  This is a standard Hour 13 Studios / Divent style loader layer.  The end user reg-
  isters the loaders he will be using for the application in question, then uses
  calls to Sample_Load, Sample_LoadFP to autodetect the filetype and load it using
  the appropriate registered loader.
*/

#include "sample.h"

#include <string.h>


static SAMPLE_LOAD_API *list_smpload = NULL;

/*void VL_InfoLoader(void)
{
    int t;
    VLOADER *l;

    // list all registered devicedrivers:
    for(t=1,l=firstloader; l!=NULL; l=l->next, t++)
        printf("%d. %s\n",t,l->version);
}*/


// =====================================================================================
    void Sample_RegisterLoader(SAMPLE_LOAD_API *ldr)
// =====================================================================================
{
    if(list_smpload == NULL)
    {   list_smpload = ldr;
        ldr->next    = NULL;
    } else
    {   ldr->next    = list_smpload;
        list_smpload = ldr;
    }
}


// =====================================================================================
    void Sample_Free(SAMPLE *sample)
// =====================================================================================
{
    if(sample)
    {
        _mm_free(sample->allochandle, sample->data);
        _mm_free(sample->allochandle, sample);
    }
}


// =====================================================================================
    SAMPLE_LOAD_API *Sample_TestFP(MMSTREAM *fp)
// =====================================================================================
// Try to find a loader that recognizes the sample!
{
    SAMPLE_LOAD_API    *l;

    for(l=list_smpload; l; l=l->next)
    {   _mm_rewind(fp);
        if(l->Test(fp)) break;
    }

    return l;
}


// =====================================================================================
    BOOL Sample_Test(const CHAR *filename)
// =====================================================================================
// Try to find a loader that recognizes the sample...
{
    BOOL        retval;
    MMSTREAM   *fp;

    if(!filename || !strlen(filename))      return FALSE;
    if((fp=_mm_fopen(filename,"rb"))==NULL) return FALSE;

    retval = Sample_TestFP(fp) ? TRUE : FALSE;
    _mm_fclose(fp);

    return retval;
}


// =====================================================================================
    SAMPLE *Sample_LoadFP(MMSTREAM *fp, MM_ALLOC *allochandle)
// =====================================================================================
{
    void            *handle;
    SAMPLE_LOAD_API *l;
    SAMPLE          *si;
    BOOL             ok = FALSE;

    // Try to find a loader that recognizes the module
    l = Sample_TestFP(fp);

    if(!l)
    {   _mmerr_set(MMERR_UNSUPPORTED_FILE, "Could not load wavefile", "Mikmod Error: This is an unsupported sample type or a corrupted file.");
        return NULL;
    }

    si = _mmobj_array(allochandle, 1, SAMPLE);
    if(!si) return NULL;

    si->allochandle = allochandle;

    if(handle = l->Init())
    {   _mm_rewind(fp);
        if(l->LoadHeader(handle, fp))
            ok = l->Load(handle, si, fp);
    }

    // free loader allocations even if error!
    if(handle) l->Cleanup(handle);

    // In this case, the called procedure sets our error.
    if(!ok) return NULL;

    return si;
}


// =====================================================================================
    SAMPLE *Sample_Load(const CHAR *filename, MM_ALLOC *allochandle)
// =====================================================================================
// Loads the sample pecified by the given filename, and returns an SAMPLE struct.  The
// image must be one of the supported/registered sample types (see Sample_RegisterLoader).
// Returns NULL on failure.
{
    MMSTREAM *fp;
    SAMPLE   *si;
    
    if(!filename || !strlen(filename))      return NULL;
    if((fp=_mm_fopen(filename, "rb"))==NULL) return NULL;

    si = Sample_LoadFP(fp, allochandle);
    _mm_fclose(fp);

    return si;
}
