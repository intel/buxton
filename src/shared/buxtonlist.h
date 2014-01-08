/*
 * This file is part of buxton.
 *
 * Copyright (C) 2013 Intel Corporation
 *
 * buxton is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#pragma once

#include <stdlib.h>
#include <stdbool.h>

#define _cleanup_list_ __attribute__ ((cleanup(buxton_free_list)))
#define _cleanup_list_all_ __attribute__ ((cleanup(buxton_free_list_all)))

/**
 * A singly-linked list
 */
typedef struct BuxtonList {
	void *data; /**<Data for this item in the list */
	struct BuxtonList *next; /**<Next item in the list */
} BuxtonList;

/**
 * Iterate all elements. May return NULL
 */
#define buxton_list_foreach(list, elem) for (elem = list; elem; elem = elem->next)

/**
 * Append data to an existing list, or create a new list if NULL
 *
 * @note May return false if memory allocation errors exist
 * @param list Pointer to BuxtonList pointer
 * @param data Data pointer to store in the list
 * @return a boolean value, indicating success of the operation
 */
bool buxton_list_append(BuxtonList **list, void *data);

/*
 * Prepend data to an existing list, or create a new list if NULL
 * Much faster than append
 *
 * @note May return false if memory allocation errors exist
 * @param list Pointer to BuxtonList pointer
 * @param data Data pointer to store in the list
 * @return a boolean value, indicating success of the operation
 */
bool buxton_list_prepend(BuxtonList **list, void *data);

/**
 * Remove the given element from the list
 *
 * @param list Pointer to a BuxtonList pointer
 * @param data Remove item with the matching data pointer
 * @param do_free Whether to free the item
 * @return a boolean value, indicating success of the operation
 */
bool buxton_list_remove(BuxtonList **list, void *data, bool do_free);

/**
 * Return the tail item's data in the list
 * @param list BuxtonList instance
 * @return the elements data
 */
void *buxton_list_get_tail(BuxtonList *list);

/**
 * Get the length of the given list
 * @param list BuxtonList instance
 * @return length of the list, or -1 if an error occurred
 */
int buxton_list_get_length(BuxtonList *list);

/**
 * Free all items in the list
 */
inline void buxton_free_list(void *p)
{
	BuxtonList *list = *(BuxtonList**)p;
	if (!list)
		return;
	BuxtonList *elem, *node = NULL;
	elem = list;
	while (elem != NULL) {
		node = elem->next;
		free(elem);
		elem = node;
	}
}

/**
 * Free all items in the list and their items
 */
inline void buxton_free_list_all(void *p)
{
	BuxtonList *list = *(BuxtonList**)p;
	if (!list)
		return;
	BuxtonList *elem, *node = NULL;
	elem = list;
	while (elem != NULL) {
		free(elem->data);
		node = elem->next;
		free(elem);
		elem = node;
	}
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
