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
#include "constants.h"
#include "bt-daemon.h"
#include "backend.h"

size_t page_size(void);
#define PAGE_ALIGN(l) ALIGN_TO((l), page_size())

#define streq(a,b) (strcmp((a),(b)) == 0)
#define strneq(a, b, n) (strncmp((a), (b), (n)) == 0)
#define strcaseeq(a,b) (strcasecmp((a),(b)) == 0)
#define strncaseeq(a, b, n) (strncasecmp((a), (b), (n)) == 0)

bool streq_ptr(const char *a, const char *b) _pure_;

#define new(t, n) ((t*) malloc_multiply(sizeof(t), (n)))

#define new0(t, n) ((t*) calloc((n), sizeof(t)))

#define newa(t, n) ((t*) alloca(sizeof(t)*(n)))

#define newdup(t, p, n) ((t*) memdup_multiply(p, sizeof(t), (n)))

#define malloc0(n) (calloc((n), 1))

_malloc_  _alloc_(1, 2) static inline void *malloc_multiply(size_t a, size_t b) {
        if (_unlikely_(b == 0 || a > ((size_t) -1) / b))
                return NULL;

        return malloc(a * b);
}
void* greedy_realloc(void **p, size_t *allocated, size_t need);

/**
 * Get the string representation of a BuxtonDataType
 * @param type The BuxtonDataType to query
 * @return A string representation of the BuxtonDataType
 */
const char* buxton_type_as_string(BuxtonDataType type);

/**
 * Retrieve the filesystem path for the given layer
 * @param layer The layer in question
 * @return a string containing the filesystem path
 */
char* get_layer_path(BuxtonLayer *layer);

/**
 * Perform a deep copy of one BuxtonData to another
 * @param original The data being copied
 * @param copy Pointer where original should be copied to
 */
void buxton_data_copy(BuxtonData* original, BuxtonData *copy);
