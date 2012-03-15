/*

 Mikmod Portable System Management Facilities (the MMIO)

  By Jake Stine of Divine Entertainment (1996-2000) and

 Support:
  If you find problems with this code, send mail to:
    air@divent.org

 Distribution / Code rights:
  Use this source code in any fashion you see fit.  Giving me credit where
  credit is due is optional, depending on your own levels of integrity and
  honesty.

 ------------------------------------------------------
 module: mmalloc.c  (that's mm - alloc!)
 
 The Mikmod Portable Memory Allocation Layer

 Generally speaking, Mikmod only allocates nice and simple blocks of
 memory.  None-the-less, I allow the user the option to assign their own
 memory manage routines, for whatever reason it may be that they want them
 used.
 
 Notes:
  - All default allocation procedures can now ensure a fixed byte alignment on
    any allocated memory block.  The alignment is adjustable via the _MM_ALIGN
    compiler define.

*/

#include "mmio.h"
#include <string.h>

#ifdef _MSC_VER
#include <assert.h>
#else
#define assert(x) ((void)0);
#endif

#ifdef _DEBUG

//#define MM_CHECKALLBLOCKS       // uncomment to check only local blocks during malloc/free

#ifndef _MM_OVERWRITE_BUFFER
#define _MM_OVERWRITE_BUFFER    16
#endif

#ifndef _MM_OVERWRITE_FILL
#define _MM_OVERWRITE_FILL      0x99999999
#endif

#else

#ifndef _MM_OVERWRITE_BUFFER
#define _MM_OVERWRITE_BUFFER     0
#endif

#endif

#ifdef MM_NO_NAMES
static const CHAR *msg_fail = "%s > Failure allocating block of size: %d";
#else
static const CHAR *msg_fail = "%s > In allochandle '%s' > Failure allocating block of size: %d";
#endif
static const CHAR *msg_set  = "General memory allocation failure.  Close some other applications, increase swapfile size, or buy some more ram... and then try again.";
static const CHAR *msg_head = "Out of memory!";


// Add the buffer threshold value and the size of the 'pointer storage space'
// onto the buffer size. Then, align the buffer size to the set alignment.
//
// TODO  : Make this code more thread-safe.. ?

#define BLOCKHEAP_THRESHOLD     3072

static MM_ALLOC      *globalalloc = NULL;
static MM_BLOCKINFO  *blockheap   = NULL; 
static uint           heapsize;               // number of blocks in the heap
static uint           lastblock;              // last block free'd (or first free block)

#ifdef _DEBUG


// =====================================================================================
    static BOOL checkunder(MM_ALLOC *type, MM_BLOCKINFO *block, BOOL doassert)
// =====================================================================================
{
    ULONG   *ptr = block->block;
    uint     i;

    if(!block->block) return FALSE;

    // Check for Underwrites
    // ---------------------

    for(i=0; i<_MM_OVERWRITE_BUFFER; i++, ptr++)
    {   if(*ptr != _MM_OVERWRITE_FILL)
        {   if(doassert)
            {   _mmlog("** _mm_checkunder > Memory underwrite on block type '%s'", type->name);
                assert(FALSE);
            }
            return FALSE;
        }
    }
    return TRUE;
}


// =====================================================================================
    static BOOL checkover(MM_ALLOC *type, MM_BLOCKINFO *block, BOOL doassert)
// =====================================================================================
{
    ULONG   *ptr = block->block;
    uint     i;

    if(!block->block) return FALSE;

    // Check for Overwrites
    // --------------------

    ptr = (ULONG *)block->block;
    ptr = &ptr[block->size + _MM_OVERWRITE_BUFFER];

    for(i=0; i<_MM_OVERWRITE_BUFFER; i++, ptr++)
    {   if(*ptr != _MM_OVERWRITE_FILL)
        {   if(doassert)
            {   _mmlog("** _mm_checkover > Memory overwrite on block type '%s'", type->name);
                assert(FALSE);
            }
            return FALSE;
        }
    }
    return TRUE;
}


// =====================================================================================
    static void __inline recurse_checkallblocks(MM_ALLOC *type)
// =====================================================================================
{
    int            cruise = type->blocklist;
    MM_ALLOC      *typec  = type->children;

    while(typec)
    {   recurse_checkallblocks(typec);
        typec = typec->next;
    }

    while(cruise != -1)
    {   MM_BLOCKINFO   *block = &blockheap[cruise];
        checkover(type, block, TRUE);
        cruise = block->next;
    }

    cruise = type->blocklist;
    while(cruise != -1)
    {   MM_BLOCKINFO   *block = &blockheap[cruise];
        checkunder(type, block, TRUE);
        cruise = block->next;
    }
}


// =====================================================================================
    void _mm_checkallblocks(MM_ALLOC *type)
// =====================================================================================
// Does a routine check on all blocks in the specified allocation type for possible mem-
// ory corruption (via the overflow and underflow buffers or, as some people call them,
// 'No Mans Land Buffers').
{
    if(!type) type = globalalloc;
    if(type) recurse_checkallblocks(type);
}


// =====================================================================================
    void _mm_allocreportall(void)
// =====================================================================================
// Dumps a memory report to logfile on all memory types.
{

}

#else
#define checkover(x,y,z)  TRUE
#define checkunder(x,y,z)  TRUE
#endif


// =====================================================================================
    static void tallyblocks(MM_ALLOC *type, uint *runsum, uint *blkcnt)
// =====================================================================================
{
    int cruise = type->blocklist;

    if(type->children)
    {   MM_ALLOC *child = type->children;
        while(child)
        {   tallyblocks(child, runsum, blkcnt);
            child = child->next;
        }
    }

    while(cruise != -1)
    {   MM_BLOCKINFO   *block = &blockheap[cruise];
        *runsum += block->size;
        (*blkcnt)++;
        cruise = block->next;
    }

    *blkcnt += type->size;
}


// =====================================================================================
    void _mmalloc_reportchild(MM_ALLOC *type, uint tabsize)
// =====================================================================================
{
    CHAR   tab[60];
    uint   i;

    if(!type) return;

    for(i=0; i<tabsize; i++) tab[i] = 32;
    tab[i] = 0;

    if(type)
    {   int  cruise = type->blocklist;

        uint    runsum = 0;
        uint    blkcnt = 0;

        _mmlog("\n%sChild AllocReport for %s", tab, type->name);

        if(type->children)
        {   tallyblocks(type, &runsum, &blkcnt);
            _mmlog("%sTotal Blocks    : %d\n"
                   "%sTotal Memory    : %dK", tab, blkcnt, tab, runsum/256);
        }

        runsum = 0;
        blkcnt = 0;

        while(cruise != -1)
        {   MM_BLOCKINFO   *block = &blockheap[cruise];
            runsum += block->size;
            blkcnt++;
            cruise = block->next;
        }

        _mmlog("%sLocal Blocks    : %d\n"
               "%sLocal Memory    : %dK", tab, blkcnt, tab, runsum/256);

#ifdef _DEBUG
        blkcnt = 0;
        cruise = type->blocklist;
        while(cruise != -1)
        {
            MM_BLOCKINFO   *block = &blockheap[cruise];

            if(type->checkplease)
            {   if(!checkover(type, block, FALSE) || !checkunder(type, block, FALSE))
                    blkcnt++;
            }

            cruise = block->next;
        }
        if(blkcnt) _mmlog("%sBad Blocks      : %d", tab,blkcnt);
#endif

        if(type->children)
        {
            MM_ALLOC *child = type->children;
            _mmlog("%sChildren types:", tab);

            while(child)
            {   _mmlog("%s - %s", tab, child->name);
                child = child->next;
            }

            child = type->children;
            while(child)
            {   _mmalloc_reportchild(child, tabsize+4);
                child = child->next;
            }
        }
    }
}


// =====================================================================================
    void _mmalloc_report(MM_ALLOC *type)
// =====================================================================================
// Dumps a memory report to logfile on the specified block type.
{
    if(!type)
        type = globalalloc;

    if(type)
    {   uint    runsum = 0;
        uint    blkcnt = 0;
        int     cruise = type->blocklist;

        _mmlog("\nAllocReport for %s\n--------------------------------------\n", type->name);

        if(type->children)
        {   tallyblocks(type, &runsum, &blkcnt);
            _mmlog("Total Blocks    : %d\n"
                   "Total Memory    : %dK", blkcnt, runsum/256);
        }

        runsum = 0;
        blkcnt = 0;

        while(cruise != -1)
        {   MM_BLOCKINFO   *block = &blockheap[cruise];
            runsum += block->size;
            blkcnt++;
            cruise = block->next;
        }

        _mmlog("Local Blocks    : %d\n"
               "Local Memory    : %dK", blkcnt, runsum/256);

#ifdef _DEBUG
        blkcnt = 0;
        cruise = type->blocklist;
        while(cruise != -1)
        {
            MM_BLOCKINFO   *block = &blockheap[cruise];

            if(type->checkplease)
            {   if(!checkover(type, block, FALSE) || !checkunder(type, block, FALSE))
                    blkcnt++;
            }

            cruise = block->next;
        }
        if(blkcnt) _mmlog("Bad Blocks      : %d", blkcnt);
#endif

        if(type->children)
        {
            MM_ALLOC *child = type->children;
            _mmlog("Children types:");

            while(child)
            {   _mmlog(" - %s", child->name);
                child = child->next;
            }

            child = type->children;
            while(child)
            {   _mmalloc_reportchild(child, 4);
                child = child->next;
            }
        }

        _mmlog("--------------------------------------\n", type->name);
    }
}


// =====================================================================================
    static MM_ALLOC *createglobal(void)
// =====================================================================================
// Create an mmio allocation handle for allocating memory in groups/pages.
{
    MM_ALLOC   *type;

    if(!blockheap)
    {   blockheap = malloc(sizeof(MM_BLOCKINFO) * BLOCKHEAP_THRESHOLD);
        heapsize  = BLOCKHEAP_THRESHOLD;
        {   uint  i;
            MM_BLOCKINFO  *block = blockheap;
            for(i=0; i<heapsize; i++, block++) block->block = NULL;
        }
    }

    type = (MM_ALLOC *)calloc(1, sizeof(MM_ALLOC));
    strcpy(type->name, "globalalloc");

#ifdef _DEBUG
    type->checkplease = TRUE;
#endif

    type->blocklist = -1;

    return type;
}

#define myretval(b)    return &(b)->block[_MM_OVERWRITE_BUFFER]
#define myblockval(b)  &(b)->block[_MM_OVERWRITE_BUFFER]

// =====================================================================================
    static ULONG __inline *mymalloc(MM_ALLOC *type, uint size)
// =====================================================================================
{
#ifdef _DEBUG
    if(type->checkplease)
#ifdef MM_CHECKALLBLOCKS
        _mm_checkallblocks(NULL);
#else
        _mm_checkallblocks(type);
#endif
#endif
    return (ULONG *)malloc((size + (_MM_OVERWRITE_BUFFER*2)) * 4);
}


// =====================================================================================
    static ULONG __inline *myrealloc(MM_ALLOC *type, ULONG *old_blk, uint size)
// =====================================================================================
{
#ifdef _DEBUG
    if(type->checkplease)
#ifdef MM_CHECKALLBLOCKS
        _mm_checkallblocks(NULL);
#else
        _mm_checkallblocks(type);
#endif
#endif
    return (ULONG *)realloc(old_blk, (size + (_MM_OVERWRITE_BUFFER*2)) * 4);
}


// =====================================================================================
#ifndef MM_NO_NAMES
    static MM_BLOCKINFO *addblock(const CHAR *name, MM_ALLOC *type, ULONG *ptr, uint size, void (*deallocate)(void *block, uint count))
#else
    static MM_BLOCKINFO *addblock(MM_ALLOC *type, ULONG *ptr, uint size, void (*deallocate)(void *block, uint count))
#endif
// =====================================================================================
{
    MM_BLOCKINFO *block;
    uint          i;

    for(i=lastblock, block = &blockheap[lastblock]; i<heapsize; i++, block++)
        if(!block->block) break;

    if(!block)
    {   for(i=0, block = blockheap; i<lastblock; i++, block++)
            if(!block->block) break;
    }

    lastblock = i+1;

    if(!block || (lastblock >= heapsize))
    {   
        // Woops! No open blocks.
        // ----------------------
        // Lets just make some more room, shall we?

        blockheap = realloc(blockheap, (heapsize+BLOCKHEAP_THRESHOLD) * sizeof(MM_BLOCKINFO));
        block     = &blockheap[i = heapsize];
        lastblock = heapsize+1;

        heapsize += BLOCKHEAP_THRESHOLD;

        {   uint  t;
            MM_BLOCKINFO  *cruise = &blockheap[lastblock];
            for(t=lastblock; t<heapsize; t++, cruise++) cruise->block = NULL;
        }

    }

#ifndef MM_NO_NAMES
    _mm_strcpy(block->name, name, MM_NAME_LENGTH);
#endif
    block->size       = size;
    block->nitems      = 0;
    block->deallocate = deallocate;
    block->block      = ptr;
    block->next       = type->blocklist;
    block->prev       = -1;

    if(type->blocklist != -1)
        blockheap[type->blocklist].prev = i;

    type->blocklist = i;

    return block;
}


// =====================================================================================
    static void removeblock(MM_ALLOC *type, int blkidx)
// =====================================================================================
// Deallocates the given block and removes it from the allocation list.
{
    MM_BLOCKINFO *block;

#ifdef _DEBUG
    if(type->checkplease) 
#ifdef MM_CHECKALLBLOCKS
        _mm_checkallblocks(NULL);
#else
        _mm_checkallblocks(type);
#endif
#endif

    block = &blockheap[blkidx];
    if(block->deallocate) block->deallocate(myblockval(block), block->nitems);

    // NOTE: Re-fetch block pointer incase realloc dicked with it!

    block = &blockheap[blkidx];
    if(block->prev != -1)
        blockheap[block->prev].next = block->next;
    else
        type->blocklist = block->next;

    if(block->next != -1) blockheap[block->next].prev = block->prev;

    free(block->block);
    //free(block);
}


// =====================================================================================
    static void fillblock(MM_BLOCKINFO *block, BOOL wipe)
// =====================================================================================
// Fills the underrun and overrun buffers with the magic databyte!
// if 'wipe' is set TRUE, then it works like calloc,a nd clears out the dataspace.
{
    ULONG *tptr = block->block;

#ifdef _DEBUG
    uint  i;
    for(i=0; i<_MM_OVERWRITE_BUFFER; i++, tptr++) *tptr = _MM_OVERWRITE_FILL;
#endif

    if(wipe) memset(tptr, 0, block->size*4);
    tptr+=block->size;

#ifdef _DEBUG
    for(i=0; i<_MM_OVERWRITE_BUFFER; i++, tptr++) *tptr = _MM_OVERWRITE_FILL;
#endif
}


// =====================================================================================
    static void __inline _addtolist(MM_ALLOC *parent, MM_ALLOC *type)
// =====================================================================================
// adds the block 'type' to the specified block 'parent'
{
    if(!parent)
    {   if(!globalalloc) globalalloc = createglobal();
        parent = globalalloc;
    }

    type->next = parent->children;
    if(parent->children) parent->children->prev = type;
    parent->children = type;
    type->parent     = parent;
    type->blocklist = -1;
}


// =====================================================================================
#ifndef MM_NO_NAMES
    MMEXPORT MM_ALLOC *_mmalloc_create(const CHAR *name, MM_ALLOC *parent)
#else
    MMEXPORT MM_ALLOC *mmalloc_create(MM_ALLOC *parent)
#endif
// =====================================================================================
// Create an mmio allocation handle for allocating memory in groups/pages.
// TODO : Add overwrite checking for these blocks
{
    MM_ALLOC   *type;

    type        = (MM_ALLOC *)calloc(1, sizeof(MM_ALLOC));
    type->size  = (sizeof(MM_ALLOC) / 4) + 1;

#ifndef MM_NO_NAMES
    _mm_strcpy(type->name, name, MM_NAME_LENGTH);
#endif
#ifdef _DEBUG
    type->checkplease = TRUE;
#endif

    _addtolist(parent, type);

    return type;
}


// =====================================================================================
#ifndef MM_NO_NAMES
    MMEXPORT MM_ALLOC *_mmalloc_create_ex(const CHAR *name, MM_ALLOC *parent, uint size)
#else
    MMEXPORT MM_ALLOC *mmalloc_create_ex(MM_ALLOC *parent, uint size)
#endif
// =====================================================================================
// Create an mmio allocation handle for allocating memory in groups/pages.
// TODO : Add overwrite checking for these blocks
{
    MM_ALLOC   *type;

    assert(size >= sizeof(MM_ALLOC));            // this can't be good!
    if(!size) size = sizeof(MM_ALLOC);

    type        = (MM_ALLOC *)calloc(1, size);
    type->size  = (size / 4) + 1;

#ifndef MM_NO_NAMES
    _mm_strcpy(type->name, name, MM_NAME_LENGTH);
#endif

#ifdef _DEBUG
    type->checkplease = TRUE;
#endif

    _addtolist(parent, type);

    return type;
}


// =====================================================================================
    MMEXPORT void _mmalloc_setshutdown(MM_ALLOC *type, void (*shutdown)(void *block), void *block)
// =====================================================================================
{
    if(type)
    {   type->shutdown = shutdown;
        type->shutdata = block;
    }
}


// =====================================================================================
    MMEXPORT void _mmalloc_closeall(void)
// =====================================================================================
// Pretty self-explainitory, as usual: closes *ALL* allocated memory.  For use during a
// complete shutdown or restart of the application.
{
    _mmalloc_close(globalalloc);
    globalalloc = NULL;
}


// =====================================================================================
    static void __inline recurse_passive_close(MM_ALLOC *type)
// =====================================================================================
{
    MM_ALLOC     *typec;

    type->killswitch = TRUE;
    if(type->shutdown) type->shutdown(type->shutdata);

    typec  = type->children;

    while(typec)
    {   if(!typec->killswitch) recurse_passive_close(typec);
        typec = typec->next;
    }
}


// =====================================================================================
    MMEXPORT void __inline _mmalloc_passive_close(MM_ALLOC *type)
// =====================================================================================
// Flags the given block for closure but does NOT free the block.  It is the responsibility
// of the caller to use _mmalloc_passive_cleanup to process all blocks and free ones
// which have been properly marked for termination.  This call processes the block 
// and all children, calling shutdown procedures-- but not freeing blocks.
{
    if(type && !type->killswitch)
        recurse_passive_close(type);
}


// =====================================================================================
    static void __inline _forcefree(MM_ALLOC *type)
// =====================================================================================
// frees all the blocks of the given type block handle.  No recursive action. (WOW)
// Does not unload Type or emove it from the parental linked list.
{
    int         cruise = type->blocklist;
    while(cruise != -1)
    {   MM_BLOCKINFO *block = &blockheap[cruise];

#if (_MM_OVERWRITE_BUFFER != 0)
        if(type->checkplease)
        {   checkover(type, block, TRUE);
            checkunder(type, block, TRUE);
        }
#endif
        if(block->deallocate)
            block->deallocate(myblockval(block), block->nitems);

        // NOTE: Re-get block address, incase deallocate changed it.
        block = &blockheap[cruise];
        if(block->block) free(block->block);
        lastblock = cruise;
        cruise = block->next;
    }
}


// =====================================================================================
    MMEXPORT BOOL _mmalloc_cleanup(MM_ALLOC *type)
// =====================================================================================
// processes the given allocblock and all of it's children, freeing all blocks which
// have been marked for termination (type->killswitch).  This method of block removal
// is the safest method for avoiding linked list errors caused by blocks that kill them
// selves during callbacks.
{
    MM_ALLOC   *typec  = type->children;

    while(typec)
    {
        MM_ALLOC *cnext = typec->next;
        if(!typec->processing)
        {
            typec->processing = TRUE;           // prevent recursive _mmalloc_close calls
            if(!_mmalloc_cleanup(typec)) typec->processing = FALSE;
        }
        typec = cnext;
    }

    if(type->killswitch)
    {
        // Unload Blocks & Remove From List!
        // ---------------------------------

        _forcefree(type);
        if(type->parent)
        {   if(type->prev)
                type->prev->next = type->next;
            else
                type->parent->children = type->next;

            if(type->next)
                type->next->prev = type->prev;
        }
        free(type);
        return TRUE;
    }
    return FALSE;
}


// =====================================================================================
    MMEXPORT void _mmalloc_freeall(MM_ALLOC *type)
// =====================================================================================
// deallocates all memory bound to the given allocation type, but does not close the
// handle - memory can be re-allocated to it at any time.
// TODO - Code me, if I'm ever needed! ;)
{
    if(type)
    {
        MM_ALLOC     *typec = type->children;

        while(typec)
        {   if(!typec->killswitch) recurse_passive_close(typec);
            typec = typec->next;
        }

        if(!type->processing)
            if(!_mmalloc_cleanup(type)) type->processing = FALSE;
    }
}


// =====================================================================================
    MMEXPORT void _mmalloc_close(MM_ALLOC *type)
// =====================================================================================
// Shuts down the given alloc handle/type.  Frees all memory associated with the handle
// and optionally checks for buffer under/overruns (if they have been flagged).
// NULL is not a valid parameter-- use _mmalloc_closeall instead!
{
    if(type)
    {   BOOL    processing = type->processing;
        type->processing = TRUE;
        if(!type->killswitch) recurse_passive_close(type);
        if(!processing)
            if(!_mmalloc_cleanup(type)) type->processing = FALSE;
    }
}


// =====================================================================================
#ifndef MM_NO_NAMES
    MMEXPORT void *_mm_mallocx(const CHAR *name, MM_ALLOC *type, size_t size, void (*deinitialize)(void *block, uint nitems))
#else
    MMEXPORT void *mm_mallocx(MM_ALLOC *type, size_t size, void (*deinitialize)(void *block, uint nitems))
#endif
// =====================================================================================
{
    ULONG   *d;

    if(!type)
    {   if(!globalalloc) globalalloc = createglobal();
        type = globalalloc;
    }

#ifndef MMALLOC_NO_FUNCPTR
    if(type->fptr_malloc)
        d = type->fptr_malloc(size);
    else
#endif
    {   size = (size/4)+1;
        d = mymalloc(type, size);

        if(!d)
        {   
            #ifdef MM_NO_NAMES
                _mmlog(msg_fail, "_mm_malloc", size);
            #else
                _mmlog(msg_fail, "_mm_malloc", type->name, size);
            #endif
            _mmerr_set(MMERR_OUT_OF_MEMORY, msg_head, msg_set);
        } else
        {
        #ifndef MM_NO_NAMES
            MM_BLOCKINFO *block = addblock(name, type, d, size, deinitialize);
        #else
            MM_BLOCKINFO *block = addblock(type, d, size, deinitialize);
        #endif
            fillblock(block, FALSE);
            myretval(block);
        }
    }

    return d;
}


// =====================================================================================
    static MM_ALLOC *slowfree(MM_ALLOC *type, ULONG *d)
// =====================================================================================
// this is used internally only for recursive searching for a deallocated block in the
// event that the initial basic search for the block fails.  Removes the block in the event
// that it is found, and returns the alloctype it was found in (so we can report it to the
// logfile).  If not found, returns NULL!
{
    MM_ALLOC   *found = NULL;

    while(type && !found)
    {   int   cruise = type->blocklist;
        while((cruise != -1) && (blockheap[cruise].block != d)) cruise = blockheap[cruise].next;
        
        if(cruise != -1)
        {   removeblock(type, cruise); //&blockheap[]);
            lastblock = cruise;
            break;
        }

        found = slowfree(type->children, d);
        type = type->next;
    }

    return found ? found : type;
}


// =====================================================================================
    MMEXPORT void _mm_a_free(MM_ALLOC *type, void **d)
// =====================================================================================
// Free the memory specified by the given pointer.  Checks the given type first and if
// not found there checks the entire hierarchy of memory allocations.  If the type is
// NULL, the all memory is checked reguardless.
{
    if(d && *d)
    {
        if(!type)
        {   if(!globalalloc) return;
            type = globalalloc;
        }

#ifndef MMALLOC_NO_FUNCPTR
        if(type->fptr_free)
            type->fptr_free(*d);
        else
#endif
        {
            int    cruise;
            ULONG *dd = *d;

            dd -= _MM_OVERWRITE_BUFFER;

            // Note to Self / TODO
            // -------------------
            // I may want to change it so that free checks all children of the specified
            // blocktype without warning or error before resorting to a full search.
            // [...]

            cruise = type->blocklist;
            while((cruise != -1) && (blockheap[cruise].block != dd))
                cruise = blockheap[cruise].next;

            if(cruise != -1)
            {   removeblock(type, cruise);
                lastblock = cruise;
            } else
            {
                MM_ALLOC    *ctype;

                // Check All Memory Regions
                // ------------------------
                // This is a very simple process thanks to some skilled use of recursion!

                ctype = slowfree(globalalloc->children, dd);

                if(ctype)
                {   _mmlog("_mm_free > (Warning) Freed block was outside specified alloctype!\n"
                           "         > (Warning) Type specified: \'%s\'\n"
                           "         > (Warning) Found in      : \'%s\'", type->name, ctype->name);

                    assert(FALSE);
                } else
                {   _mmlog("** _mm_free > Specified block not found! (type %s, address = %d)", type->name, dd);
                    assert(FALSE);
                }
            }

            *d = NULL;
        }
    }
}


// =====================================================================================
#ifndef MM_NO_NAMES
    MMEXPORT void *_mm_callocx(const CHAR *name, MM_ALLOC *type, size_t nitems, size_t size, void (*deallocate)(void *block, uint count))
#else
    MMEXPORT void *mm_callocx(MM_ALLOC *type, size_t nitems, size_t size, void (*deallocate)(void *block, uint count))
#endif
// =====================================================================================
{
    ULONG *d;

    if(!type)
    {   if(!globalalloc) globalalloc = createglobal();
        type = globalalloc;
    }

#ifndef MMALLOC_NO_FUNCPTR
    if(type->fptr_calloc)
    {   d = type->fptr_calloc(nitems, size);
    } else
#endif
    {   
        // The purpose of calloc : to save others the work of the multiply!

        size = nitems * size;
        size = (size/4)+1;

        d = mymalloc(type,size);
        if(d)
        {
        #ifndef MM_NO_NAMES
            MM_BLOCKINFO *block = addblock(name, type, d, size, deallocate);
        #else
            MM_BLOCKINFO *block = addblock(type, d, size, deallocate);
        #endif
            block->nitems      = nitems;
            fillblock(block, TRUE);
            myretval(block);
        } else
        {
            #ifdef MM_NO_NAMES
                _mmlog(msg_fail, "_mm_calloc", size);
            #else
                _mmlog(msg_fail, "_mm_calloc", type->name, size);
            #endif
            _mmerr_set(MMERR_OUT_OF_MEMORY, msg_head, msg_set);
        }
    }

    return d;
}


// =====================================================================================
#ifndef MM_NO_NAMES
    MMEXPORT void *_mm_reallocx(const CHAR *name, MM_ALLOC *type, void *old_blk, size_t size, void (*deallocate)(void *block, uint nitems))
#else
    MMEXPORT void *mm_reallocx(MM_ALLOC *type, void *old_blk, size_t size, void (*deallocate)(void *block, uint nitems))
#endif
// =====================================================================================
{
    ULONG *d = NULL;    

    if(!type)
    {   if(!globalalloc) globalalloc = createglobal();
        type = globalalloc;
    }

#ifndef MMALLOC_NO_FUNCPTR
    if(type->fptr_realloc)
        d = type->fptr_realloc(old_blk, size);
    else
#endif
    {
        MM_BLOCKINFO   *block;

        size = (size/4)+1;

        if(old_blk)
        {
            int             cruise;
            ULONG          *dd = old_blk;

            dd -= _MM_OVERWRITE_BUFFER;

            cruise = type->blocklist;
            while((cruise != -1) && (blockheap[cruise].block != dd))
                cruise = blockheap[cruise].next;

            if(cruise != -1)
            {   d = myrealloc(type, dd, size);

                block = &blockheap[cruise];

                block->size       = size;
                block->block      = d;
                block->deallocate = deallocate;
            } else
            {   _mmlog("** _mm_realloc > Block not found...");
                assert(FALSE);
            }
        } else
        {   d = mymalloc(type,size);
        #ifndef MM_NO_NAMES
            block = addblock(name, type, d, size, deallocate);
        #else
            block = addblock(type, d, size, deallocate);
        #endif
        }

        if(d)
        {   fillblock(block, FALSE);
            myretval(block);
        } else
        {
            #ifdef MM_NO_NAMES
                _mmlog(msg_fail, "_mm_realloc", size);
            #else
                _mmlog(msg_fail, "_mm_realloc", type->name, size);
            #endif
            _mmerr_set(MMERR_OUT_OF_MEMORY, msg_head, msg_set);
        }
    }

    return d;
}


// =====================================================================================
#ifndef MM_NO_NAMES
    MMEXPORT void *_mm_recallocx(const CHAR *name, MM_ALLOC *type, void *old_blk, size_t nitems, size_t size, void (*deallocate)(void *block, uint count))
#else
    MMEXPORT void *mm_recallocx(MM_ALLOC *type, void *old_blk, size_t nitems, size_t size, void (*deallocate)(void *block, uint count))
#endif
// =====================================================================================
// My own special reallocator, which works like calloc by allocating memory by both block-
// size and numitems.
{
    ULONG   *d = NULL;

    if(!type)
    {   if(!globalalloc) globalalloc = createglobal();
        type = globalalloc;
    }

#ifndef MMALLOC_NO_FUNCPTR
    if(type->fptr_recalloc)
    {   d = type->fptr_recalloc(old_blk, nitems, size);
    } else
#endif
    {
        MM_BLOCKINFO   *block;
        // The purpose of calloc : to save others the work of the multiply!

        size = nitems * size;
        size = (size/4)+1;

        if(old_blk)
        {
            int             cruise;
            ULONG          *dd = old_blk;

            dd -= _MM_OVERWRITE_BUFFER;

            cruise = type->blocklist;
            while((cruise != -1) && (blockheap[cruise].block != dd))
                cruise = blockheap[cruise].next;

            if(cruise != -1)
            {   d = myrealloc(type, dd, size);

                block = &blockheap[cruise];

                if(block->size < size)
                {   ULONG   *dd = (ULONG *)d;
                    memset(&dd[block->size+_MM_OVERWRITE_BUFFER], 0, (size - block->size)*4);
                }

                block->size       = size;
                block->block      = d;
                block->deallocate = deallocate;
            } else
            {   _mmlog("** _mm_recalloc > Block not found...");
                assert(FALSE);
            }
        } else
        {   d = mymalloc(type,size);
        #ifndef MM_NO_NAMES
            block = addblock(name, type, d, size, deallocate);
        #else
            block = addblock(type, d, size, deallocate);
        #endif
            memset(myblockval(block), 0, size*4);
        }

        if(d)
        {   block->nitems  = nitems;
            fillblock(block, FALSE);
            myretval(block);
        } else
        {
            #ifdef MM_NO_NAMES
                _mmlog(msg_fail, "_mm_recalloc", size);
            #else
                _mmlog(msg_fail, "_mm_recalloc", type->name, size);
            #endif
            _mmerr_set(MMERR_OUT_OF_MEMORY, msg_head, msg_set);
        }
    }

    return d;
}


// =====================================================================================
    MM_VECTOR *_mm_vector_create(MM_ALLOC *allochandle, uint strucsize, uint increment)
// =====================================================================================
// Creates a vector thingie (like the C STL thingie, only not stupid or something).
// If you want a custom threshold value, set it manually after this call.  Simple enough.
// Use _mm_vector_free to feee the vector handle and memory allocated to it.
{
    MM_VECTOR    *array;
    if(!strucsize) return NULL;

    array               = _mmobj_allocblock(allochandle, MM_VECTOR);
    if(!array) return NULL;

    array->allochandle  = allochandle;
    array->strucsize    = strucsize;
    array->increment    = increment ? increment : 256;
    array->threshold    = 8;

    return array;
}


// =====================================================================================
    void _mm_vector_init(MM_VECTOR *array, MM_ALLOC *allochandle, uint strucsize, uint increment)
// =====================================================================================
// initializes a static vector thingie (like the C STL thingie, only not stupid or
// something). If you want a custom threshold value, set it manually after this call.
// Simple enough.
// Use _mm_vector_close to free the memory allocated to the vector.
{
    if(!array || !strucsize) return;
    array->allochandle  = allochandle;
    array->strucsize    = strucsize;
    array->increment    = increment ? increment : 256;
    array->threshold    = 8;
}


// =====================================================================================
    void _mm_vector_alloc(MM_VECTOR *array, void **ptr)
// =====================================================================================
// Reallocates an array as needed, as according to the specified array descriptor.
{
    if(array && (array->alloc <= (array->count + array->threshold)))
    {
        #ifdef _DEBUG
        if(array->ptr)
        {   assert(array->ptr == *ptr);   }
        #endif

        array->alloc = array->count + array->increment + array->threshold;
        *ptr         = _mm_recalloc(array->allochandle, *ptr, array->alloc, array->strucsize);
        #ifdef _DEBUG
        if(!array->ptr)
            array->ptr = *ptr;
        #endif
    }
}


// =====================================================================================
    void _mm_vector_close(MM_VECTOR *array, void **ptr)
// =====================================================================================
// Wipes the contents of the specified array and unloads allocated memory.
// If used on a dynamically-allocated vector handle, the handle remains intact.
{
    array->count        = 0;
    array->alloc        = 0;
    _mm_a_free(array->allochandle, ptr);
}


// =====================================================================================
    void _mm_vector_free(MM_VECTOR **array, void **ptr)
// =====================================================================================
// Frees a dynamically-allocated mm_vector!
{
    if(array)
    {
        _mm_a_free((*array)->allochandle, ptr);
        _mm_a_free((*array)->allochandle, array);
    }
}
