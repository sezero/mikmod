#ifndef MDMA_H
#define MDMA_H

#include "mmtypes.h"

#define READ_DMA                0
#define WRITE_DMA               1
#define INDEF_READ              2
#define INDEF_WRITE             3

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __WATCOMC__

typedef struct
{   void *continuous;   // the pointer to a page-continous dma buffer
    UWORD raw_selector; // the raw allocated dma selector
} DMAMEM;

#elif defined(__DJGPP__)

typedef struct
{   void    *continuous;  // the pointer to a page-continous dma buffer
    int     raw_selector; // points to the memory that was allocated
    UWORD   dos_selector;
    ULONG   lin_address;
} DMAMEM;

#else

typedef struct
{   void *continuous;   // the pointer to a page-continous dma buffer
    void *raw;          // points to the memory that was allocated
} DMAMEM;

#endif

DMAMEM *MDma_AllocMem(UWORD size);
void    MDma_FreeMem(DMAMEM *dm);
int     MDma_Start(int channel,DMAMEM *dm,UWORD size,int type);
void    MDma_Stop(int channel);
void   *MDma_GetPtr(DMAMEM *dm);
void    MDma_Commit(DMAMEM *dm,UWORD index,UWORD count);
UWORD   MDma_Todo(int channel);

#ifdef __cplusplus
}
#endif

#endif
