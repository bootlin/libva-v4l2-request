/*
 * Copyright (C) 2007 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>

#include "object_heap.h"

static int object_heap_expand(struct object_heap *heap)
{
	struct object_base *object;
	void *new_heap_index;
	int new_heap_size = heap->heap_size + heap->heap_increment;
	int bucket_index = new_heap_size / heap->heap_increment - 1;
	int next_free;
	int i;

	if (bucket_index >= heap->num_buckets) {
		int new_num_buckets = heap->num_buckets + 8;
		void **new_bucket;

		new_bucket = realloc(heap->bucket,
				     new_num_buckets * sizeof(void *));
		if (new_bucket == NULL)
			return -1;

		heap->num_buckets = new_num_buckets;
		heap->bucket = new_bucket;
	}

	new_heap_index = malloc(heap->heap_increment * heap->object_size);
	if (new_heap_index == NULL)
		return -1;

	heap->bucket[bucket_index] = new_heap_index;
	next_free = heap->next_free;

	for (i = new_heap_size; i-- > heap->heap_size;) {
		object = (struct object_base *)(new_heap_index +
						(i - heap->heap_size) *
							heap->object_size);
		object->id = i + heap->id_offset;
		object->next_free = next_free;
		next_free = i;
	}

	heap->next_free = next_free;
	heap->heap_size = new_heap_size;

	return 0;
}

static int object_heap_allocate_unlocked(struct object_heap *heap)
{
	struct object_base *object;
	int bucket_index, object_index;

	if (heap->next_free == OBJECT_HEAP_LAST)
		if (object_heap_expand(heap) == -1)
			return -1;

	if (heap->next_free < 0)
		return -1;

	bucket_index = heap->next_free / heap->heap_increment;
	object_index = heap->next_free % heap->heap_increment;

	object = (struct object_base *)(heap->bucket[bucket_index] +
					object_index * heap->object_size);
	heap->next_free = object->next_free;
	object->next_free = OBJECT_HEAP_ALLOCATED;

	return object->id;
}

int object_heap_init(struct object_heap *heap, int object_size, int id_offset)
{
	pthread_mutex_init(&heap->mutex, NULL);

	heap->object_size = object_size;
	heap->id_offset = id_offset & OBJECT_HEAP_OFFSET_MASK;
	heap->heap_size = 0;
	heap->heap_increment = 16;
	heap->next_free = OBJECT_HEAP_LAST;
	heap->num_buckets = 0;
	heap->bucket = NULL;

	return object_heap_expand(heap);
}

int object_heap_allocate(struct object_heap *heap)
{
	int rc;

	pthread_mutex_lock(&heap->mutex);
	rc = object_heap_allocate_unlocked(heap);
	pthread_mutex_unlock(&heap->mutex);

	return rc;
}

static struct object_base *object_heap_lookup_unlocked(struct object_heap *heap,
						       int id)
{
	struct object_base *object;
	int bucket_index, object_index;

	if ((id < heap->id_offset) ||
	    (id > (heap->heap_size + heap->id_offset)))
		return NULL;

	id &= OBJECT_HEAP_ID_MASK;
	bucket_index = id / heap->heap_increment;
	object_index = id % heap->heap_increment;

	object = (struct object_base *)(heap->bucket[bucket_index] +
					object_index * heap->object_size);

	if (object->next_free != OBJECT_HEAP_ALLOCATED)
		return NULL;

	return object;
}

struct object_base *object_heap_lookup(struct object_heap *heap, int id)
{
	struct object_base *object;

	pthread_mutex_lock(&heap->mutex);
	object = object_heap_lookup_unlocked(heap, id);
	pthread_mutex_unlock(&heap->mutex);

	return object;
}

struct object_base *object_heap_first(struct object_heap *heap, int *iterator)
{
	*iterator = -1;

	return object_heap_next(heap, iterator);
}

static struct object_base *object_heap_next_unlocked(struct object_heap *heap,
						     int *iterator)
{
	struct object_base *object;
	int bucket_index, object_index;
	int i = *iterator + 1;

	while (i < heap->heap_size) {
		bucket_index = i / heap->heap_increment;
		object_index = i % heap->heap_increment;

		object = (struct object_base *)(heap->bucket[bucket_index] +
						object_index *
							heap->object_size);
		if (object->next_free == OBJECT_HEAP_ALLOCATED) {
			*iterator = i;
			return object;
		}

		i++;
	}

	*iterator = i;

	return NULL;
}

struct object_base *object_heap_next(struct object_heap *heap, int *iterator)
{
	struct object_base *object;

	pthread_mutex_lock(&heap->mutex);
	object = object_heap_next_unlocked(heap, iterator);
	pthread_mutex_unlock(&heap->mutex);

	return object;
}

static void object_heap_free_unlocked(struct object_heap *heap,
				      struct object_base *object)
{
	object->next_free = heap->next_free;
	heap->next_free = object->id & OBJECT_HEAP_ID_MASK;
}

void object_heap_free(struct object_heap *heap, struct object_base *object)
{
	if (!object)
		return;

	pthread_mutex_lock(&heap->mutex);
	object_heap_free_unlocked(heap, object);
	pthread_mutex_unlock(&heap->mutex);
}

void object_heap_destroy(struct object_heap *heap)
{
	int i;

	for (i = 0; i < heap->heap_size / heap->heap_increment; i++)
		free(heap->bucket[i]);

	pthread_mutex_destroy(&heap->mutex);

	free(heap->bucket);
	heap->bucket = NULL;
	heap->heap_size = 0;
	heap->next_free = OBJECT_HEAP_LAST;
}
