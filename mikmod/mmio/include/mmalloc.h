
#ifndef _MMALLOC_H_
#define _MMALLOC_H_

// ===================================================
//  Memory allocation with error handling - MMALLOC.C
// ===================================================

// Block Types In-Depth
// --------------------
// By default, a global blocktype is created which is the default block used when the
// user called _mm_malloc
//

// -------------------------------------------------------------------------------------
    typedef struct MM_BLOCKINFO
// -------------------------------------------------------------------------------------
{
#ifndef MM_NO_NAMES
    CHAR            name[MM_NAME_LENGTH];  // name of the block, only when names enabled.
#endif

    uint            size;                  // size of the memory allocation, in dwords
    uint            nitems;
    ULONG          *block;
    void          (*deallocate)(void *block, uint count);

    int            next, prev;             // you know what a linked list is
                                           // (this one is based on indexes, not pointers)
} MM_BLOCKINFO;


// -------------------------------------------------------------------------------------
    typedef struct MM_ALLOC
// -------------------------------------------------------------------------------------
// An allocation handle for use with the mmio allocation functions.
{
#ifdef _DEBUG
    BOOL               checkplease;
#endif
    int                blocklist;
    uint               ident;            // blocktype identifier!

    struct MM_ALLOC   *next, *prev;      // you know what a linked list is.
    struct MM_ALLOC   *children, *parent;

    BOOL               killswitch;
    BOOL               processing;       // Set TRUE if we're in list-processing callback state

#ifndef MM_NO_NAMES
    CHAR               name[MM_NAME_LENGTH]; // every allocation type has a name.  wowie zowie!
#endif

    uint               size;             // size of allochandle in dwords
    void             (*shutdown)(void *blockdata);
    void              *shutdata;

#ifndef MMALLOC_NO_FUNCPTR
    void *(*fptr_malloc)    (size_t size);
    void *(*fptr_calloc)    (size_t nitems, size_t size);
    void *(*fptr_realloc)   (void *old_blk, size_t size);
    void *(*fptr_recalloc)  (void *old_blk, size_t nitems, size_t size);
    void *(*fptr_free)      (void *d);
#endif
} MM_ALLOC;


// -------------------------------------------------------------------------------------
    typedef struct MM_VECTOR
// -------------------------------------------------------------------------------------
// Air's own version of the C Vector junk.  Simple alloc and count variables.
// So simple, dogs rejoice at the thought of knawling on this.
{
    MM_ALLOC    *allochandle;          // allocation handle we're bound to
    uint         alloc;
    uint         count;

    uint         increment;            // amount to increment array by
    uint         threshold;            // no fewer than this many free elements allowed
    uint         strucsize;            // structure size, in bytes

#ifdef _DEBUG
    void        *ptr;                  // set by first call to alloc, and used for assert checking on 
                                       // subsequeent calls.
#endif

} MM_VECTOR;


// -------------------------------------------------------------------------------------
    typedef struct MM_NODE
// -------------------------------------------------------------------------------------
// Ultra elite voidptr-based node lists!
{
    struct MM_NODE  *next, *prev;
    void            *data;              // data object bound to this node!
    int              usrval;            // additional user-attached value (handy dandy stuff)

    int              processing;        // incremented every time the node is accessed
    BOOL             killswitch;        // flagged if node deleted while in processing state

    MM_ALLOC        *allochandle;

} MM_NODE;


#define _mmalloc_callback_enter(t) \
    BOOL  _mma_processing = (t)->processing; \
    (t)->processing = TRUE;

#define _mmalloc_callback_exit(t)  { (t)->processing = _mma_processing; if((t)->killswitch) _mmalloc_cleanup(t); }
#define _mmalloc_callback_next(t) \
{   MM_ALLOC *_ttmp321 = (t)->next; \
    _mmalloc_callback_exit(t); \
    (t) = _ttmp321;  } \


#ifdef _DEBUG
#define _mmalloc_check_set(a)    (a)->checkplease = TRUE
#define _mmalloc_check_clear(a)  (a)->checkplease = FALSE
#else
#define _mmalloc_check_set(a)
#define _mmalloc_check_clear(a)
#endif

#define MMBLK_IDENT_ALLOCHANDLE      0
#define MMBLK_IDENT_USER         0x100

MMEXPORT void      _mmalloc_freeall(MM_ALLOC *type);
MMEXPORT void      _mmalloc_close(MM_ALLOC *type);
MMEXPORT void      _mmalloc_closeall(void);
MMEXPORT void      _mmalloc_passive_close(MM_ALLOC *type);
MMEXPORT BOOL      _mmalloc_cleanup(MM_ALLOC *type);

MMEXPORT void      _mmalloc_setshutdown(MM_ALLOC *type, void (*shutdown)(void *block), void *block);
MMEXPORT void      _mmalloc_report(MM_ALLOC *type);

MMEXPORT MM_VECTOR *_mm_vector_create(MM_ALLOC *allochandle, uint strucsize, uint increment);
MMEXPORT void      _mm_vector_init(MM_VECTOR *array, MM_ALLOC *allochandle, uint strucsize, uint increment);
MMEXPORT void      _mm_vector_alloc(MM_VECTOR *array, void **ptr);
MMEXPORT void      _mm_vector_close(MM_VECTOR *array, void **ptr);
MMEXPORT void      _mm_vector_free(MM_VECTOR **array, void **ptr);

#ifdef _DEBUG
MMEXPORT void      _mm_checkallblocks(MM_ALLOC *type);
MMEXPORT void      _mm_allocreportall(void);
#else
#define _mm_checkallblocks(x)
#endif

// mm_alloc macro parameters
// -------------------------
//
// a  - Allochandle (MM_ALLOC *)
// n  - Number of blocks
// s  - Size of Array (in blocks)
// o  - old/original pointer
//
// p  - Parent struct (must inherit allochandle)
// c  - number of elements within the array
// t  - Array type

#ifdef MM_NO_NAMES

    #define _mm_mallocx(crap,a,s,b)       mm_mallocx(a,s,b)
    #define _mm_callocx(crap,a,n,s)       mm_callocx(a,n,s, NULL)
    #define _mm_reallocx(crap,a,o,s)      mm_reallocx(a,o,s, NULL)
    #define _mm_recallocx(crap,a,o,n,s)   mm_recallocx(a,o,n,s, NULL)

    MMEXPORT void     *mm_mallocx(MM_ALLOC *type, size_t size, void (*deinitialize)(void *block, uint count));
    MMEXPORT void     *mm_callocx(MM_ALLOC *type, size_t nitems, size_t size, void (*deinitialize)(void *block, uint count));
    MMEXPORT void     *mm_reallocx(MM_ALLOC *type, void *old_blk, size_t size, void (*deinitialize)(void *block, uint count));
    MMEXPORT void     *mm_recallocx(MM_ALLOC *type, void *old_blk, size_t nitems, size_t size, void (*deinitialize)(void *block, uint count));

    MMEXPORT MM_ALLOC *mmalloc_create(MM_ALLOC *parent);
    MMEXPORT MM_ALLOC *mmalloc_create_ex(MM_ALLOC *parent, uint size);

    #define _mmalloc_create(name, p)    mmalloc_create(p)
    #define _mmalloc_create_ex(name, p) mmalloc_create_ex(p,s)

    #define _mm_malloc(a,s)         _mm_mallocx(a,s, NULL)
    #define _mm_calloc(a,n,s)       _mm_callocx(a,n,s, NULL)
    #define _mm_realloc(a,o,s)      _mm_reallocx(a,o,s, NULL)
    #define _mm_recalloc(a,o,n,s)   _mm_recallocx(a,o,n,s, NULL)

    #define _mmobj_new(p,t)         (t*)_mmalloc_create_ex((MM_ALLOC *)(p), sizeof(t))
    #define _mmobj_array(p,c,t)     (t*)_mm_calloc        ((MM_ALLOC *)(p), c, sizeof(t))
    #define _mmobj_allocblock(p,t)  (t*)_mm_calloc        (#t, (MM_ALLOC *)(p), 1, sizeof(t))

    //#define _mm_new(a,t)            (t*)_mm_calloc(a, 1, sizeof(t))
    //#define _mm_array(a,c,t)        (t*)_mm_calloc(a, c, sizeof(t))

#else

    MMEXPORT void     *_mm_mallocx(const CHAR *name, MM_ALLOC *type, size_t size, void (*deinitialize)(void *block, uint count));
    MMEXPORT void     *_mm_callocx(const CHAR *name, MM_ALLOC *type, size_t nitems, size_t size, void (*deinitialize)(void *block, uint count));
    MMEXPORT void     *_mm_reallocx(const CHAR *name, MM_ALLOC *type, void *old_blk, size_t size, void (*deinitialize)(void *block, uint count));
    MMEXPORT void     *_mm_recallocx(const CHAR *name, MM_ALLOC *type, void *old_blk, size_t nitems, size_t size, void (*deinitialize)(void *block, uint count));

    MMEXPORT MM_ALLOC *_mmalloc_create(const CHAR *name, MM_ALLOC *parent);
    MMEXPORT MM_ALLOC *_mmalloc_create_ex(const CHAR *name, MM_ALLOC *parent, uint size);

    #define _mm_malloc(a,s)         _mm_mallocx  (NULL, a, s,       NULL)
    #define _mm_calloc(a,n,s)       _mm_callocx  (NULL, a, n, s,    NULL)
    #define _mm_realloc(a,o,s)      _mm_reallocx (NULL, a, o, s,    NULL)
    #define _mm_recalloc(a,o,n,s)   _mm_recallocx(NULL, a, o, n, s, NULL)

    #define _mmobj_new(p,t)         (t*)_mmalloc_create_ex(#t, (MM_ALLOC *)(p), sizeof(t))
    #define _mmobj_array(p,c,t)     (t*)_mm_callocx       (#t, (MM_ALLOC *)(p), c, sizeof(t), NULL)
    #define _mmobj_allocblock(p,t)  (t*)_mm_callocx       (#t, (MM_ALLOC *)(p), 1, sizeof(t), NULL)

    //#define _mm_new(a,t)            (t*)_mm_callocx(#t, a, 1, sizeof(t), NULL)
    //#define _mm_array(a,c,t)        (t*)_mm_callocx(#t, a, c, sizeof(t), NULL)

#endif

MMEXPORT void      _mm_a_free(MM_ALLOC *type, void **d);

// Adding a cast to (void **) gets rid of a few dozen warnings.
#define _mm_free(b, a)      _mm_a_free(b, (void **)&(a))
#define _mmobj_free(b, a)   _mm_a_free((MM_ALLOC *)(b), (void **)&(a))
#define _mmobj_close(b)     _mmalloc_close((MM_ALLOC *)(b))

#ifdef MM_LOG_VERBOSE

#define _mmlogd         _mmlog
#define _mmlogd1        _mmlog
#define _mmlogd2        _mmlog
#define _mmlogd3        _mmlog
#define _mmlogd4        _mmlog
#define _mmlogd5        _mmlog

#else

#define _mmlogd(a)             __mmlogd()
#define _mmlogd1(a,b)          __mmlogd()
#define _mmlogd2(a,b,c)        __mmlogd()
#define _mmlogd3(a,b,c,d)      __mmlogd()
#define _mmlogd4(a,b,c,d,e)    __mmlogd()
#define _mmlogd5(a,b,c,d,e,f)  __mmlogd()

static void __inline __mmlogd(void)
{
}

#endif

#endif      // _MMALLOC_H_