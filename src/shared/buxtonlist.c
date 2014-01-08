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

#include "buxtonlist.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

bool buxton_list_append(BuxtonList **list, void *data)
{
	BuxtonList *head = *list;

	BuxtonList *node = NULL;
	BuxtonList *parent = NULL;
	BuxtonList *next = NULL;

	if (!head) {
		/* New head generation */
		head = calloc(1, sizeof(BuxtonList));
		next = parent = head;
		if (!head)
			return false;
	}

	if (!parent) {
		parent = node = head;
		/* Find potential parent */
		while (node != NULL) {
			parent = node;
			node = node->next;
		}
	}

	/* New element in existing list */
	if (!next) {
		next = calloc(1, sizeof(BuxtonList));
		if (!next)
			return false;
		parent->next = next;
	}
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
	} else {
		/* New item */
		prev = calloc(1, sizeof(BuxtonList));
		if (!prev)
			return false;
		prev->next = head;
	}
	/* Previous item is now the head */
	prev->data = data;
	*list = prev;

	return true;
}

bool buxton_list_prepend2(BuxtonList **list, void *data, void *data2)
{
	BuxtonList *head = *list;
	BuxtonList *prev = NULL;

	if (!head) {
		/* New head generation */
		head = calloc(1, sizeof(BuxtonList));
		if (!head)
			return false;
		prev = head;
	} else {
		/* New item */
		prev = calloc(1, sizeof(BuxtonList));
		if (!prev)
			return false;
		prev->next = head;
	}
	/* Previous item is now the head */
	prev->data = data;
	prev->data2 = data2;
	*list = prev;

	return true;
}

bool buxton_list_remove(BuxtonList **list, void *data, bool do_free)
{
	return buxton_list_remove2(list, data, do_free, false);
}

bool buxton_list_remove2(BuxtonList **list, void *data, bool one, bool two)
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
	if (!current)
		return false;

	/* Data on the head */
	if (current == head)
		head = head->next;
	else
		prev->next = current->next;

	/* Should free? */
	if (one)
		free(current->data);
	if (two)
		free(current->data2);
	free(current);
	*list = head;
	return true;
}

void *buxton_list_get_tail(BuxtonList *list)
{
	BuxtonList *elem = NULL;
	BuxtonList *ret = NULL;

	if (!list)
		return NULL;
	buxton_list_foreach(list, elem) {
		ret = elem;
	}
	if (ret)
		return ret->data;
	return NULL;
}

int buxton_list_get_length(BuxtonList *list)
{
	int length = 0;
	BuxtonList *elem = NULL;

	if (!list)
		return -1;

	buxton_list_foreach(list, elem) {
		length+=1;
	}
	return length;
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
