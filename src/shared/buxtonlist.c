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

#include "buxtonlist.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

bool buxton_list_append(BuxtonList **list, void *data)
{
	BuxtonList *head = *list;

	BuxtonList *parent = NULL;
	BuxtonList *next = NULL;

	if (!head) {
		/* New head generation */
		head = calloc(1, sizeof(BuxtonList));
		next = parent = head;
		if (!head)
			return false;
		head->size = 0;
		next->tail = NULL;
	}

	if (!next) {
		next = calloc(1, sizeof(BuxtonList));
		if (head->tail)
			parent = head->tail;
		else
			parent = head;
		parent->next = next;
		head->tail = next;
	}
	head->size += 1;
	next->data = data;
	*list = head;
	return true;
}

bool buxton_list_prepend(BuxtonList **list, void *data)
{
	BuxtonList *head = *list;
	BuxtonList *prev = NULL;

	if (!head) {
		/* New head generation */
		head = calloc(1, sizeof(BuxtonList));
		if (!head)
			return false;
		prev = head;
		head->size = 0;
		prev->tail = head;
		prev->next = NULL;
	} else {
		/* New item */
		prev = calloc(1, sizeof(BuxtonList));
		if (!prev)
			return false;
		prev->size = head->size+1;
		head->size = 0;
		prev->next = head;
		prev->tail = head->tail;
		head->tail = NULL;
	}
	/* Previous item is now the head */
	prev->data = data;
	*list = prev;
	
	return true;
}

bool buxton_list_remove(BuxtonList **list, void *data, bool do_free)
{
	BuxtonList *head = *list;
	BuxtonList *current = head;
	BuxtonList *prev = head;

	prev = current = head;
	/* Determine the node inside the list */
	while ((current != NULL) && (current->data != data)) {
		prev = current;
		current = current->next;
	}
	/* Not found */
	if (!current) {
		return false;
	}
	
	/* Data on the head (head removal)*/
	if (current == head) {
		if (head->next) {
			head->next->size = head->size -1;
			head->size = 0;
		}
		head = head->next;
	} else {
		/* Middle or end */
		prev->next = current->next;
		head->size--;
	}

	/* Update tail pointer */
	if (head && head->tail == current) {
		head->tail = prev;
		head->tail->next = NULL;
	}

	/* Should free? */
	if (do_free)
		free(current->data);
	free(current);

	*list = head;
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
