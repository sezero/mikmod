/*
 
 Mikmod Portable System Management Facilities (the MMIO)

  By Jake Stine of Hour 13 Studios (1996-2002) and

 Support:
  If you find problems with this code, send mail to:
    air@hour13.com

 Distribution / Code rights:
  Use this source code in any fashion you see fit.  Giving me credit where
  credit is due is optional, depending on your own levels of integrity and
  honesty.

 ------------------------------------------------------
 node.c

 Air's own node system.
 I should rename MMIO to 'MMSTL' .. muahaha!

*/

#include "mmio.h"
#include <assert.h>

// ================================================================================
    void _mm_node_delete(MM_NODE *node)
// ================================================================================
// Removes the node from the list is currently bound to.
{
    if(node)
    {
        if(!node->processing)
        {
            if(node->prev)
                node->prev->next = node->next;
            if(node->next)
                node->next->prev = node->prev;
            _mm_free(node->allochandle, node);

        } else
            node->killswitch = TRUE;
    }
}


// ================================================================================
    MM_NODE *_mm_node_create(MM_ALLOC *allochandle, MM_NODE *insafter, void *data)
// ================================================================================
{
    MM_NODE     *newnode;

    newnode              = _mmobj_allocblock(allochandle, MM_NODE);
    newnode->allochandle = allochandle;

    if(insafter)
    {
        if(insafter->next)
        {   insafter->next->prev = newnode;
            newnode->next        = insafter->next;
        }
        newnode->prev    = insafter;
        insafter->next   = newnode;
    }

    return newnode;
}


// ================================================================================
    static void _mm_node_process(MM_NODE *node)
// ================================================================================
// This should be called prior to FOR loops and WHILE loops, when the node or other
// nodes in the list are subject to removal.
{
    if(node)
    {
        node->processing++;
        _mm_node_process(node->next);
    }
}


// ================================================================================
    void _mm_node_cleanup(MM_NODE *node)
// ================================================================================
// Processes the entire list starting from the specified node, deleting any nodes flagged
// for removal.  This can be called at any time, but is generally called at the end of
// a FOR or WHILE loop that processed nodes using _mm_node_process and _mm_node_post
// commands.
{
    if(node)
    {
        assert(node->processing);
        node->processing--;
        if(!node->processing && node->killswitch)
            _mm_node_delete(node);
        _mm_node_cleanup(node->next);
    }
}