/*
 * This file is part of buxton.
 *
 * Copyright (C) 2014 Intel Corporation
 *
 * buxton is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "buxtonlist.h"

bool buxton_list_append(BuxtonList **list, void *data)
{
	BuxtonList *head;
	BuxtonList *item;

	/* create a link */
	item = calloc(1, sizeof(BuxtonList));
	if (!item) {
		return false;
	}
	item->data = data;

	head = *list;
	if (!head) {
		/* init the list */
		item->size = 1;
		item->tail = item;
		*list = item;
	} else {
		/* append to an existing list */
		head->tail->next = item;
		head->tail = item;
		head->size++;
	}
	return true;
}

bool buxton_list_prepend(BuxtonList **list, void *data)
{
	BuxtonList *head;
	BuxtonList *item;

	/* create a link */
	item = calloc(1, sizeof(BuxtonList));
	if (!item) {
		return false;
	}
	item->data = data;

	head = *list;
	if (!head) {
		/* init the list */
		item->size = 1;
		item->tail = item;
	} else {
		/* prepend the item to the list */
		item->next = head;
		item->tail = head->tail;
		item->size = head->size + 1;
	}
	*list = item;
	return true;
}

bool buxton_list_remove(BuxtonList **list, void *data, bool do_free)
{
	BuxtonList *head;
	BuxtonList *item;
	BuxtonList *prev;

	/* Determine the node inside the list */
	prev = 0;
	item = *list;
	while (item && (item->data != data)) {
		prev = item;
		item = item->next;
	}
	/* Not found */
	if (!item) {
		return false;
	}

	if (!prev) {
		/* removing the head */
		assert(head == item);
		head = item->next;
		if (head) {
			head->tail = item->tail;
			head->size = item->size - 1;
		}
		*list = head;
	} else {
		/* removing after the head */
		prev->next = item->next;
		head = *list;
		if (head->tail == item) {
			head->tail = prev;
		}
		head->size--;
	}

	/* freeing */
	if (do_free) {
		free(item->data);
	}
	free(item);

	return true;
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
