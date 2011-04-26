/* Copyright 2011 Kevin Bulusek. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file. */

#ifndef _GC_H
#define _GC_H

#include "gc_config.h"

/* Simple explicit garbage collection, requiring co-operation from
 * involved data structures. */

typedef struct gc GC;

typedef struct gc_allocator GCAllocator;

struct gc_allocator {
	void *(* malloc)(void *rtdata, size_t size);
	void  (* free)(void *rtdata, void *ptr);

	void *rtdata;
};

/* Create a new garbage-collecting allocator.
 *
 * The contents of the GCAllocator argument are copied, so it need not
 * be persisted by the caller.
 *
 * A pool allocator could be used to create a very fast allocator.
 * See also GC_OVERHEAD.
 *
 * Returns NULL if the GC could not be allocated. */
GC *make_gc_full(const GCAllocator *);

/* Create a new garbage-collecting allocator.
 *
 * Uses malloc and free to allocate memory.
 *
 * Returns NULL if the GC could not be allocated. */
static inline GC *make_gc(void)
{
	extern const GCAllocator _gc_stdlib_allocator;

	return make_gc_full(&_gc_stdlib_allocator);
}

/* The GC_OVERHEAD macro gets the number of extra bytes the garbage
 * collector needs per allocation.  When GC_DEBUG is disabled, this is
 * typically the size of a pointer.  Some special purpose allocators
 * might need this information. */
#ifdef GC_DEBUG
# define GC_OVERHEAD() _gc_overhead()
size_t _gc_overhead(void);
#else
# define GC_OVERHEAD() (sizeof (void *))
#endif

/* Free a previously created GC allocator.
 *
 * Any memory allocated by the GC that has not yet been collected will
 * be freed. */
void free_gc(GC *gc);

/* Allocate a garbage collected block of memory.
 *
 * Returns NULL if the underlying allocation routine fails. */
void *gc_alloc(GC *gc, size_t size);

/* Mark a block of memory as in-use and not collectable.
 *
 * All memory that is in use must be marked before collecting.  The
 * visit argument specifies how to find referenced memory.  A block of
 * memory is only marked once, so cyclic data structures pose no
 * problem.
 *
 * Failure to mark in-use blocks of memory before calling gc_collect
 * will result in the premature collection of those blocks.
 *
 * `ptr' may be NULL, in which case it will be ignored.  Otherwise, it
 * must have been allocated by gc_alloc.
 *
 * `visit' may be NULL for atomic types (e.g. strings).  Otherwise, it
 * should be a function which calls gc_mark on each GC-allocated
 * reference held by `ptr'. */
void gc_mark(void *ptr, void (* visit)(void *));

/* Free all non-marked blocks of memory allocated by a specific
 * allocator.  Marked blocks are not collected, and they become
 * unmarked (in preparation for the next collection).
 *
 * gc_collect makes no attempt to automatically search the stack or
 * heap for memory references, thus any referenced blocks of memory
 * must be marked by gc_mark before collection.  Typically this only
 * consists of marking a small number of roots.  Passing the
 * appropriate visit functions to gc_mark will find and mark all
 * references.
 *
 * If GC_DEBUG is defined, blocks will be filled with 0xFE bytes
 * before being freed.  If GC_DEBUG_PRINT is defined some statistics
 * will be printed to stderr. */
void gc_collect(GC *gc);

#endif	/* _GC_H */
