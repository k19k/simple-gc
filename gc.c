/* Copyright 2011 Kevin Bulusek. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file. */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "gc.h"

typedef struct _alloc _Alloc;

struct _alloc {
	_Alloc *next;
#ifdef GC_DEBUG
	size_t size;
#endif
	char data[];
};

struct gc {
	_Alloc *head;
	GCAllocator alloc;
#ifdef GC_DEBUG
	size_t current_allocs;
	size_t current_bytes;
	size_t lifetime_allocs;
	size_t lifetime_bytes;
#endif
};

static void *stdlib_malloc(void *rtdata, size_t size)
{
	return malloc(size);
}

static void stdlib_free(void *rtdata, void *ptr)
{
	free(ptr);
}

const GCAllocator _gc_stdlib_allocator = {
	stdlib_malloc,
	stdlib_free,
	NULL
};

#ifdef GC_DEBUG
size_t _gc_overhead(void)
{
	return sizeof (_Alloc);
}
#endif

static inline void *_alloc_mem(_Alloc *node)
{
	return node->data;
}

static inline _Alloc *_mem_alloc(void *ptr)
{
	return (_Alloc *) ((char *) ptr - offsetof(_Alloc, data));
}

static inline intptr_t _marked(_Alloc *node)
{
	return (intptr_t) node->next & 1;
}

static inline void _mark(_Alloc *node)
{
	node->next = (_Alloc *) ((intptr_t) node->next | 1);
}

static inline void _clear(_Alloc *node)
{
	node->next = (_Alloc *) ((intptr_t) node->next & ~1);
}

GC *make_gc_full(const GCAllocator *alloc)
{
	GC *gc = malloc(sizeof *gc);
	if (!gc) {
		return NULL;
	}
	memset(gc, 0, sizeof *gc);
	memcpy(&gc->alloc, alloc, sizeof *alloc);
	return gc;
}

void free_gc(GC *gc)
{
#ifdef GC_DEBUG_PRINT
	fprintf(stderr, "<gc %p> - lifetime statistics:\n", gc);
	fprintf(stderr, "    allocs: %lu (%lu active)\n",
		gc->lifetime_allocs, gc->current_allocs);
	fprintf(stderr, "    bytes:  %lu (%lu active)\n",
		gc->lifetime_bytes, gc->current_bytes);
#endif
	/* Collect twice in case some blocks are marked. */
	gc_collect(gc);
	gc_collect(gc);
#ifdef GC_DEBUG
	assert(gc->current_bytes == 0);
	assert(gc->current_allocs == 0);
	assert(gc->head == NULL);
#endif
	free(gc);
}

void *gc_alloc(GC *gc, size_t size)
{
	size += sizeof (_Alloc);
	_Alloc *node = gc->alloc.malloc(gc->alloc.rtdata, size);
	if (!node) {
		return NULL;
	}
	node->next = gc->head;
	gc->head = node;
#ifdef GC_DEBUG
	node->size = size;
	gc->current_bytes += size;
	gc->lifetime_bytes += size;
	gc->current_allocs++;
	gc->lifetime_allocs++;
#endif
	return _alloc_mem(node);
}

void gc_mark(void *ptr, void (* visit)(void *))
{
	if (!ptr) {
		return;
	}
	_Alloc *node = _mem_alloc(ptr);
	if (_marked(node)) {
		return;
	}
	_mark(node);
	if (visit) {
		visit(ptr);
	}
}

void gc_collect(GC *gc)
{
	_Alloc *node = gc->head;
	_Alloc **pnode = &gc->head;
#ifdef GC_DEBUG_PRINT
	size_t free_bytes = 0;
	size_t free_allocs = 0;
	size_t init_bytes = gc->current_bytes;
	size_t init_allocs = gc->current_allocs;
#endif
	while (node) {
		_Alloc *next;
		if (_marked(node)) {
			pnode = &node->next;
			_clear(node);
			next = node->next;
		} else {
			next = node->next;
#ifdef GC_DEBUG_PRINT
			free_bytes += node->size;
			free_allocs++;
#endif
#ifdef GC_DEBUG
			gc->current_allocs--;
			gc->current_bytes -= node->size;
			memset(node, 0xfe, node->size);
#endif
			*pnode = next;
			gc->alloc.free(gc->alloc.rtdata, node);
		}
		node = next;
	}
#ifdef GC_DEBUG_PRINT
	fprintf(stderr, "<gc %p> - collect statistics:\n", gc);
	fprintf(stderr, "    allocs: %lu (of %lu)\n", free_allocs, init_allocs);
	fprintf(stderr, "    bytes:  %lu (of %lu)\n", free_bytes, init_bytes);
#endif
}
