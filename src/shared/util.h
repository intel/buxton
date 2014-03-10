/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

#pragma once

/***
  This file is part of systemd.

  Copyright 2010 Lennart Poettering

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include <alloca.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sched.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/resource.h>
#include <stddef.h>
#include <unistd.h>
#include <locale.h>

#include "macro.h"
#include "buxton.h"
#include "buxtonkey.h"
#include "backend.h"

size_t page_size(void);
#define PAGE_ALIGN(l) ALIGN_TO((l), page_size())

#define buxton_string_pack(s) ((BuxtonString){(s), (uint32_t)strlen(s) + 1})

#define streq(a,b) (strcmp((a),(b)) == 0)
#define strneq(a, b, n) (strncmp((a), (b), (n)) == 0)
#define strcaseeq(a,b) (strcasecmp((a),(b)) == 0)
#define strncaseeq(a, b, n) (strncasecmp((a), (b), (n)) == 0)

bool streq_ptr(const char *a, const char *b) _pure_;

static inline void freep(void *p)
{
	free(*(void**) p);
}

static inline void free_buxton_data(void *p)
{
	BuxtonData *s = (*(void**) p);
	if (s && s->type == STRING) {
		free(s->store.d_string.value);
	}
	free(s);
}
static inline void free_buxton_string(void *p)
{
	BuxtonString *s = (*(void**) p);
	if (s) {
		free(s->value);
	}
	free(s);
}

static inline void free_buxton_key(void *p)
{
	_BuxtonKey *k = (*(void**) p);
	if (k) {
		if (k->group.value) {
			free(k->group.value);
		}
		if (k->name.value) {
			free(k->name.value);
		}
		if (k->layer.value) {
			free(k->layer.value);
		}
	}
	free(k);
}

#define _cleanup_free_ _cleanup_(freep)
#define _cleanup_buxton_data_ _cleanup_(free_buxton_data)
#define _cleanup_buxton_string_ _cleanup_(free_buxton_string)
#define _cleanup_buxton_key_ _cleanup_(free_buxton_key)

#define new(t, n) ((t*) malloc_multiply(sizeof(t), (n)))

#define new0(t, n) ((t*) calloc((n), sizeof(t)))

#define newa(t, n) ((t*) alloca(sizeof(t)*(n)))

#define newdup(t, p, n) ((t*) memdup_multiply(p, sizeof(t), (n)))

#define malloc0(n) (calloc((n), 1))

_malloc_  _alloc_(1, 2) static inline void *malloc_multiply(size_t a, size_t b) {
	if (_unlikely_(b == 0 || a > ((size_t) -1) / b)) {
		return NULL;
	}

	return malloc(a * b);
}
void* greedy_realloc(void **p, size_t *allocated, size_t need);

/**
 * Get the string representation of a BuxtonDataType
 * @param type The BuxtonDataType to query
 * @return A string representation of the BuxtonDataType
 */
const char* buxton_type_as_string(BuxtonDataType type)
	__attribute__((warn_unused_result));

/**
 * Retrieve the filesystem path for the given layer
 * @param layer The layer in question
 * @return a string containing the filesystem path
 */
char* get_layer_path(BuxtonLayer *layer)
	__attribute__((warn_unused_result));

/**
 * Perform a deep copy of one BuxtonData to another
 * @param original The data being copied
 * @param copy Pointer where original should be copied to
 * @return A boolean indicating success or failure
 */
bool buxton_data_copy(BuxtonData *original, BuxtonData *copy);

/**
 * Perform a deep copy of one BuxtonString to another
 * @param original The BuxtonString being copied
 * @param copy Pointer where original should be copied to
 */
bool buxton_string_copy(BuxtonString *original, BuxtonString *copy)
	__attribute__((warn_unused_result));

/**
 * Perform a deep copy of one _BuxtonKey to another
 * @param original The _BuxtonKey being copied
 * @param copy Pointer where original should be copied to
 */
bool buxton_key_copy(_BuxtonKey *original, _BuxtonKey *copy)
	__attribute__((warn_unused_result));

/**
 * Perform a partial deep copy of a _BuxtonKey, omitting the 'name' member
 * @param original The _BuxtonKey being copied
 * @param group Pointer to the destination of the partial copy
 */
bool buxton_copy_key_group(_BuxtonKey *original, _BuxtonKey *group)
	__attribute__((warn_unused_result));

/**
 * Perform a deep free of BuxtonData
 * @param data The BuxtonData being free'd
 */
void data_free(BuxtonData *data);

/**
 * Perform a deep free of BuxtonString
 * @param string The BuxtonString being free'd
 */
void string_free(BuxtonString *string);

/**
 * Perform a deep free of _BuxtonKey
 * @param string The _BuxtonKey being free'd
 */
void key_free(_BuxtonKey *key);

/**
 * Get the group portion of a buxton key
 * @param key Pointer to _BuxtonKey
 * @return A pointer to a character string containing the key's group
 */
char *get_group(_BuxtonKey *key)
	__attribute__((warn_unused_result));

/**
 * Get the name portion of a buxton key
 * @param key Pointer to _BuxtonKey
 * @return A pointer to a character string containing the key's name
 */
char *get_name(_BuxtonKey *key)
	__attribute__((warn_unused_result));

/**
 * Get the layer portion of a buxton key
 * @param key Pointer to _BuxtonKey
 * @return A pointer to a character string containing the key's layer
 */
char *get_layer(_BuxtonKey *key)
	__attribute__((warn_unused_result));

/**
 * Wrapper for nonblocking write, handles EAGAIN
 * @param fd File descriptor to write to
 * @param buf Buffer containing data to write
 * @param len Length of buffer to write
 * @return A boolean indicating the success of the operation
 */
bool _write(int fd, uint8_t *buf, size_t len)
	__attribute__((warn_unused_result));

/*
 * Editor modelines  -	http://www.wireshark.org/tools/modelines.html
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
