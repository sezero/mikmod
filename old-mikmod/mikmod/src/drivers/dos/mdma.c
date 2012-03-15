/*

Name: MDMA.C

Description:
DMA transfer and allocation routines

Portability:

MSDOS:  BC(y)   Watcom(y)   DJGPP(y)
Win95:  n
Os2:    n
Linux:  n

(y) - yes
(n) - no (not possible or not useful)
(?) - may be possible, but not tested

*/
#include <dos.h>
#include <conio.h>
#include "mmio.h"
#include "mdma.h"

#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>



// DMA Controler #1 (8-bit controller)
#define DMA1_STAT       0x08            // read status register
#define DMA1_WCMD       0x08            // write command register
#define DMA1_WREQ       0x09            // write request register
#define DMA1_SNGL       0x0A            // write single bit register
#define DMA1_MODE       0x0B            // write mode register
#define DMA1_CLRFF      0x0C            // clear byte ptr flip/flop
#define DMA1_MCLR       0x0D            // master clear register
#define DMA1_CLRM       0x0E            // clear mask register
#define DMA1_WRTALL     0x0F            // write all mask register

// DMA Controler #2 (16-bit controller)
#define DMA2_STAT       0xD0            // read status register
#define DMA2_WCMD       0xD0            // write command register
#define DMA2_WREQ       0xD2            // write request register
#define DMA2_SNGL       0xD4            // write single bit register
#define DMA2_MODE       0xD6            // write mode register
#define DMA2_CLRFF      0xD8            // clear byte ptr flip/flop
#define DMA2_MCLR       0xDA            // master clear register
#define DMA2_CLRM       0xDC            // clear mask register
#define DMA2_WRTALL     0xDE            // write all mask register

#define DMA0_ADDR       0x00            // chan 0 base adddress
#define DMA0_CNT        0x01            // chan 0 base count
#define DMA1_ADDR       0x02            // chan 1 base adddress
#define DMA1_CNT        0x03            // chan 1 base count
#define DMA2_ADDR       0x04            // chan 2 base adddress
#define DMA2_CNT        0x05            // chan 2 base count
#define DMA3_ADDR       0x06            // chan 3 base adddress
#define DMA3_CNT        0x07            // chan 3 base count
#define DMA4_ADDR       0xC0            // chan 4 base adddress
#define DMA4_CNT        0xC2            // chan 4 base count
#define DMA5_ADDR       0xC4            // chan 5 base adddress
#define DMA5_CNT        0xC6            // chan 5 base count
#define DMA6_ADDR       0xC8            // chan 6 base adddress
#define DMA6_CNT        0xCA            // chan 6 base count
#define DMA7_ADDR       0xCC            // chan 7 base adddress
#define DMA7_CNT        0xCE            // chan 7 base count

#define DMA0_PAGE       0x87            // chan 0 page register (refresh)
#define DMA1_PAGE       0x83            // chan 1 page register
#define DMA2_PAGE       0x81            // chan 2 page register
#define DMA3_PAGE       0x82            // chan 3 page register
#define DMA4_PAGE       0x8F            // chan 4 page register (unuseable)
#define DMA5_PAGE       0x8B            // chan 5 page register
#define DMA6_PAGE       0x89            // chan 6 page register
#define DMA7_PAGE       0x8A            // chan 7 page register

#define MAX_DMA         8

#define DMA_DECREMENT   0x20    // mask to make DMA hardware go backwards

typedef struct
{   UBYTE dma_disable;      // bits to disable dma channel
    UBYTE dma_enable;       // bits to enable dma channel
    UWORD page;             // page port location
    UWORD addr;             // addr port location
    UWORD count;            // count port location
    UWORD single;           // single mode port location
    UWORD mode;             // mode port location
    UWORD clear_ff;         // clear flip-flop port location
    UBYTE write;            // bits for write transfer
    UBYTE read;             // bits for read transfer
} DMA_ENTRY;

/* Variables needed ... */

static DMA_ENTRY mydma[MAX_DMA] =
{
// DMA channel 0 
       {0x04,0x00,DMA0_PAGE,DMA0_ADDR,DMA0_CNT,
        DMA1_SNGL,DMA1_MODE,DMA1_CLRFF,0x48,0x44},

// DMA channel 1
       {0x05,0x01,DMA1_PAGE,DMA1_ADDR,DMA1_CNT,
        DMA1_SNGL,DMA1_MODE,DMA1_CLRFF,0x49,0x45},

// DMA channel 2
       {0x06,0x02,DMA2_PAGE,DMA2_ADDR,DMA2_CNT,
        DMA1_SNGL,DMA1_MODE,DMA1_CLRFF,0x4A,0x46},

// DMA channel 3
       {0x07,0x03,DMA3_PAGE,DMA3_ADDR,DMA3_CNT,
        DMA1_SNGL,DMA1_MODE,DMA1_CLRFF,0x4B,0x47},

// DMA channel 4
       {0x04,0x00,DMA4_PAGE,DMA4_ADDR,DMA4_CNT,
        DMA2_SNGL,DMA2_MODE,DMA2_CLRFF,0x48,0x44},

// DMA channel 5
       {0x05,0x01,DMA5_PAGE,DMA5_ADDR,DMA5_CNT,
        DMA2_SNGL,DMA2_MODE,DMA2_CLRFF,0x49,0x45},

// DMA channel 6
       {0x06,0x02,DMA6_PAGE,DMA6_ADDR,DMA6_CNT,
        DMA2_SNGL,DMA2_MODE,DMA2_CLRFF,0x4A,0x46},

// DMA channel 7
       {0x07,0x03,DMA7_PAGE,DMA7_ADDR,DMA7_CNT,
        DMA2_SNGL,DMA2_MODE,DMA2_CLRFF,0x4B,0x47}
};


/*
Each specialised DMA code part should provide the following things:

In MDMA.H:

  - a DMAMEM typedef, which should contain all the data that the
    routines need for maintaining/allocating/freeing dma memory.


In MDMA.C:

  - 2 macros ENTER_CRITICAL and LEAVE_CRITICAL

  - A function 'static BOOL MDma_AllocMem0(DMAMEM *dm,UWORD size)'
    which should perform the actual dma-memory allocation. It should
    use DMAMEM *dm to store all it's information.

  - A function 'static void MDma_FreeMem0(DMAMEM *dm)' to free the memory

  - A function 'static ULONG MDma_GetLinearPtr(DMAMEM *dm)' which should
    return the linear 20 bits pointer to the actual dmabuffer.. this
    function is used by MDma_Start

  - A function 'void *MDma_GetPtr(DMAMEM *dm)' which should return a pointer
    to the dmabuffer. If the dma memory can't be accessed directly it should
    return a pointer to a FAKE dma buffer (DJGPP!!)

  - A function 'void MDma_Commit(DMAMEM *dm,UWORD index,UWORD count)'. This
    function will be called each time a routine wrote something to the
    dmabuffer (returned by MDma_GetPtr()). In the case of a FAKE dmabuffer
    this routine should take care of copying the data from the fake buffer to
    the real dma memory ('count' bytes from byteoffset 'index').

*/



#ifdef __WATCOMC__

/****************************************************************************
********************* Watcom C specialised DMA code: ************************
****************************************************************************/

#define ENTER_CRITICAL IRQ_PUSH_OFF()
extern void IRQ_PUSH_OFF (void);
#pragma aux IRQ_PUSH_OFF =      \
    "pushfd",                   \
    "cli"                       \
    modify [esp];

#define LEAVE_CRITICAL IRQ_POP()
extern void IRQ_POP (void);
#pragma aux IRQ_POP =   \
        "popfd"         \
        modify [esp];


static BOOL MDma_AllocMem0(DMAMEM *dm,UWORD size)
// Allocates a dma buffer of 'size' bytes.
// returns FALSE if failed.
{
    static union REGS r;
    ULONG p;

    // allocate TWICE the size of the requested dma buffer..
    // this fixes the 'page-crossing' bug of previous versions 

    r.x.eax = 0x0100;               // DPMI allocate DOS memory
    r.x.ebx = ((size*2) + 15) >> 4; // Number of paragraphs requested

    int386 (0x31, &r, &r);

    if( r.x.cflag ) return 0;       // failed

    dm->raw_selector = r.x.edx;

    // convert the segment into a linear address
    p = (r.x.eax & 0xffff) << 4;

    // if the first half of the allocated memory crosses a page
    // boundary, return the second half which is then guaranteed
    // to be page-continuous

    if( (p>>16) != ((p+size-1)>>16) ) p+=size;
    dm->continuous = (void *)p;

    return 1;
}


static void MDma_FreeMem0(DMAMEM *dm)
{
    static union REGS r;
    r.x.eax = 0x0101;               // DPMI free DOS memory
    r.x.edx = dm->raw_selector;     // base selector
    int386 (0x31, &r, &r);
}


static ULONG MDma_GetLinearPtr(DMAMEM *dm)
{
    return (ULONG)dm->continuous;
}


void *MDma_GetPtr(DMAMEM *dm)
{
    return(dm->continuous);
}


void MDma_Commit(DMAMEM *dm,UWORD index,UWORD count)
{
    // This function doesnt do anything here (WATCOM C
    // can access dma memory directly)
}


#elif defined(__DJGPP__)
/****************************************************************************
*********************** DJGPP specialised DMA code: *************************
****************************************************************************/

/*
typedef struct {
   unsigned long size;
   unsigned long pm_offset;
   unsigned short pm_selector;
   unsigned short rm_offset;
   unsigned short rm_segment;
} _go32_dpmi_seginfo;
*/

#define ENTER_CRITICAL __asm__( "pushf \n\t cli" )
#define LEAVE_CRITICAL __asm__( "popf \n\t" )
#include <sys/farptr.h>
#include <sys/nearptr.h>

// Starts off similar to Watcom, but finishes with selector allocation.
static BOOL MDma_AllocMem0(DMAMEM *dm,UWORD size)
// Allocates a dma buffer of 'size' bytes.
// returns FALSE if failed.
{
    static union REGS r;
    ULONG p;

    // allocate TWICE the size of the requested dma buffer..
    // this fixes the 'page-crossing' bug of previous versions 

    r.d.eax = 0x0100;               // DPMI allocate DOS memory 
    r.d.ebx = ((size*2) + 15) >> 4; // Number of paragraphs requested

    int386 (0x31, &r, &r);

    if( r.d.cflag ) return 0;       // failed

    dm->dos_selector = (UWORD)r.d.edx & 0xFFFF;

    // convert the segment into a linear address
    p = (r.d.eax & 0xffff) << 4;

    // if the first half of the allocated memory crosses a page
    // boundary, return the second half which is then guaranteed
    // to be page-continuous

    if( (p>>16) != ((p+size-1)>>16) ) p+=size;

    dm->lin_address = p;
    dm->raw_selector = __dpmi_create_alias_descriptor(dm->dos_selector);
    __dpmi_set_segment_base_address(dm->raw_selector, p);

/*  Now using __djgpp_nearptr_enable instead
    // For optimization, force dword alignment
    dm->continuous = _mm_malloc(size + 4);
    if (((ULONG) dm->continuous) % 4)
        dm->continuous = (void *) ((((ULONG) dm->continuous) & 0xFFFFFFFC)
            + 4); */

    __djgpp_nearptr_enable();
    dm->continuous = (void *) ((ULONG) __djgpp_conventional_base +
        dm->lin_address);
    return 1;
}

static void MDma_FreeMem0(DMAMEM *dm)
{
    static union REGS r;
    __djgpp_nearptr_disable();
    __dpmi_free_ldt_descriptor(dm->raw_selector);
    r.d.eax = 0x0101;               // DPMI free DOS memory
    r.d.edx = dm->dos_selector;     // base selector
    int386 (0x31, &r, &r);
//    free(dm->continuous);
}

static ULONG MDma_GetLinearPtr(DMAMEM *dm)
{
    return (ULONG)dm->lin_address;
}

void *MDma_GetPtr(DMAMEM *dm)
{
    return(dm->continuous);
}

void MDma_Commit(DMAMEM *dm,UWORD index,UWORD count)
{
/* Unneeded (I hope)
      __asm__ (
        "pushw %%es\n\t"          // push es
        "movw %%ax,%%es\n\t"      // mov es,ax
        "movl %%edx,%%ecx\n\t"    // mov ecx,edx
        "shrl $2,%%ecx\n\t"       // shr ecx,2
        "cld\n\t"                 // cld
        "andl $0x3,%%edx\n\t"     // and edx,3
        "rep\n\t"
        "movsl\n\t"               // rep movsd
        "movl %%edx,%%ecx\n\t"    // mov ecx,edx
        "rep\n\t"
        "movsb\n\t"               // rep movsb
        "popw %%es\n"             // pop es
        :
        : "S" ((ULONG) dm->continuous + index), "a" (dm->raw_selector),
                "d" ((ULONG) count), "D" ((ULONG) index)
        : "ecx", "edi"
    );   */
}

#else

/****************************************************************************
********************* Borland C specialised DMA code: ***********************
****************************************************************************/

#define ENTER_CRITICAL asm{ pushf; cli }
#define LEAVE_CRITICAL asm{ popf }

#define LPTR(ptr) (((ULONG)FP_SEG(ptr)<<4)+FP_OFF(ptr))
#define NPTR(ptr) MK_FP(FP_SEG(p)+(FP_OFF(p)>>4),FP_OFF(p)&15)


static BOOL MDma_AllocMem0(DMAMEM *dm,UWORD size)
/*
    Allocates a dma buffer of 'size' bytes.
    returns FALSE if failed.
*/
{
    char huge *p;
    ULONG s;

    // allocate TWICE the size of the requested dma buffer..
    // so we can always get a page-contiguous dma buffer

    if((dm->raw=_mm_malloc((ULONG)size*2)) == NULL) return 0;

    p = (char huge *)dm->raw;
    s = LPTR(p);

    // if the first half of the allocated memory crosses a page
    // boundary, return the second half which is then guaranteed to
    // be page-continuous

    if( (s>>16) != ((s+size-1)>>16) ) p+=size;

    // put the page-continuous pointer into DMAMEM

    dm->continuous = NPTR(p);

    return 1;
}


static void MDma_FreeMem0(DMAMEM *dm)
{
    free(dm->raw);
}


static ULONG MDma_GetLinearPtr(DMAMEM *dm)
{
    return LPTR(dm->continuous);
}


void *MDma_GetPtr(DMAMEM *dm)
{
    return(dm->continuous);
}

#pragma argsused

void MDma_Commit(DMAMEM *dm,UWORD index,UWORD count)
{
    // This function doesnt do anything here (BORLAND C
    // can access dma memory directly)
}

#endif


/****************************************************************************
************************* General DMA code: *********************************
****************************************************************************/


DMAMEM *MDma_AllocMem(UWORD size)
{
    DMAMEM *p;

    // allocate dma memory structure
    if(!(p=(DMAMEM *)_mm_malloc(sizeof(DMAMEM)))) return NULL;

    // allocate dma memory

    if(!MDma_AllocMem0(p,size))
    {   // didn't succeed? -> free everything & return NULL
        free(p);

        _mm_errno = MMERR_ALLOCATING_DMA;
        if(_mm_errorhandler!=NULL) _mm_errorhandler();
        return NULL;
    }

    return p;
}


void MDma_FreeMem(DMAMEM *p)
{
    MDma_FreeMem0(p);
    free(p);
}


int MDma_Start(int channel, DMAMEM *dm, UWORD size, int type)
{
    DMA_ENTRY *tdma;
    ULONG      s_20bit,e_20bit;
    UWORD      spage,saddr,tcount;
    UWORD      epage;
    UBYTE      cur_mode;

    tdma = &mydma[channel];           // point to this dma data

    // Convert the pc address to a 20 bit physical
    // address that the DMA controller needs

    s_20bit = MDma_GetLinearPtr(dm);

    e_20bit = s_20bit + size - 1;
    spage = s_20bit >> 16;
    epage = e_20bit >> 16;

    if(spage != epage) return 0;

    if(channel>=4)
    {   // if 16-bit xfer, then addr,count & size are divided by 2
        s_20bit = s_20bit >> 1;
        e_20bit = e_20bit >> 1;
        size = size >> 1;
    }

    saddr  = s_20bit&0xffff;
    tcount = size-1;

    switch (type)
    {   case READ_DMA:
            cur_mode = tdma->read;
        break;

        case WRITE_DMA:
            cur_mode = tdma->write;
        break;

        case INDEF_READ:
            cur_mode = tdma->read | 0x10;   // turn on auto init
        break;

        case INDEF_WRITE:
            cur_mode = tdma->write | 0x10;  // turn on auto init
        break;
    }

    ENTER_CRITICAL;
    outportb(tdma->single,tdma->dma_disable);          // disable channel
    outportb(tdma->mode,cur_mode);                     // set mode
    outportb(tdma->clear_ff,0);                        // clear f/f
    outportb(tdma->addr,saddr & 0xff);                 // LSB
    outportb(tdma->addr,saddr >> 8);                   // MSB
    outportb(tdma->page,spage);                        // page #
    outportb(tdma->clear_ff, 0);                       // clear f/f
    outportb(tdma->count,tcount & 0x0ff);              // LSB count
    outportb(tdma->count,tcount >> 8);                 // MSB count
    outportb(tdma->single,tdma->dma_enable);           // enable
    LEAVE_CRITICAL;

    return 1;
}


void MDma_Stop(int channel)
{
    DMA_ENTRY *tdma;
    tdma = &mydma[channel];                     // point to this dma data
    outportb(tdma->single,tdma->dma_disable);   // disable chan
}


UWORD MDma_Todo(int channel)
{
    UWORD creg;
    UWORD val1,val2;

    DMA_ENTRY *tdma = &mydma[channel];

    creg = tdma->count;

    ENTER_CRITICAL;

    outportb(tdma->clear_ff,0xff);

redo:
    val1  = inportb(creg);
    val1 |= inportb(creg)<<8;
    val2  = inportb(creg);
    val2 |= inportb(creg)<<8;

    val1-=val2;
    if((SWORD)val1 >  64) goto redo;
    if((SWORD)val1 <- 64) goto redo;

    LEAVE_CRITICAL;

    if(channel>3) val2 <<= 1;

    return val2;
}

