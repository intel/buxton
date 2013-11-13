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

#include <stdio.h>
#include <string.h>
#include <sys/inotify.h>
#include "constants.h"
#include "hashmap.h"
#include "log.h"
#include "smack.h"
#include "util.h"

static Hashmap *_smackrules = NULL;

bool buxton_cache_smack_rules(void)
{
	FILE *load_file = NULL;
	char *rule_pair = NULL;
	int ret = true;

	if (_smackrules)
		hashmap_free(_smackrules);

	_smackrules = hashmap_new(string_hash_func, string_compare_func);

	if (!_smackrules) {
		buxton_log("Failed to allocate Smack access table: %m\n");
		return false;
	}

	load_file = fopen(SMACK_LOAD_FILE, "r");

	if (!load_file) {
		buxton_log("fopen(): %m\n");
		ret = false;
		goto end;
	}

	while (!feof(load_file)) {
		int r;
		int chars;
		BuxtonKeyAccessType *accesstype;

		char subject[SMACK_LABEL_LEN] = { 0, };
		char object[SMACK_LABEL_LEN] = { 0, };
		char access[ACC_LEN] = { 0, };

		/* read contents from load2 */
		chars = fscanf(load_file, "%s %s %s\n", subject, object, access);
		if (chars != 3) {
			buxton_log("fscanf(): %m\n");
			ret = false;
			goto end;
		}

		r = asprintf(&rule_pair, "%s %s", subject, object);
		if (r == -1) {
			buxton_log("asprintf(): %m\n");
			ret = false;
			goto end;
		}

		accesstype = malloc0(sizeof(BuxtonKeyAccessType));
		if (!accesstype) {
			buxton_log("malloc0(): %m\n");
			ret = false;
			goto end;
		}

		*accesstype = ACCESS_NONE;

		if (strchr(access, 'r'))
			*accesstype |= ACCESS_READ;

		if (strchr(access, 'w'))
			*accesstype |= ACCESS_WRITE;

		hashmap_put(_smackrules, rule_pair, accesstype);
	}

end:
	if (load_file)
		fclose(load_file);

	return ret;
}

bool buxton_check_smack_access(BuxtonString *subject, BuxtonString *object, BuxtonKeyAccessType request)
{
	char *key;
	int r;
	BuxtonKeyAccessType *access;

	assert(subject->value);
	assert(object->value);
	assert((request == ACCESS_READ) || (request == ACCESS_WRITE));
	assert(_smackrules);

	buxton_debug("Subject: %s\n", subject->value);
	buxton_debug("Object: %s\n", object->value);

	/* check the builtin Smack rules first */
	if (streq(subject->value, "*"))
		return false;

	if (streq(object->value, "@") || streq(subject->value, "@"))
		return true;

	if (streq(object->value, "*"))
		return true;

	if (streq(subject->value, object->value))
		return true;

	if (request == ACCESS_READ) {
		if (streq(object->value, "_"))
			return true;
		if (streq(subject->value, "^"))
			return true;
	}

	/* finally, check the loaded rules */
	r = asprintf(&key, "%s %s", subject->value, object->value);
	if (r == -1) {
		buxton_log("asprintf(): %m\n");
		exit(1);
	}

	buxton_debug("Key: %s\n", key);

	access = hashmap_get(_smackrules, key);
	if (!access) {
		/* corruption */
		buxton_log("Value of key '%s' is NULL\n", key);
		free(key);
		exit(1);
	}

	free(key);

	/* After debugging, change this code to: */
	/* return ((*access) & request); */
	if (access)
		buxton_debug("Value: %x\n", *access);

	if ((*access) & request) {
		buxton_debug("Access granted!\n");
		return true;
	}

	buxton_debug("Access denied!\n");
	return false;
}

int buxton_watch_smack_rules(void)
{
	int fd;

	fd = inotify_init1(IN_NONBLOCK);
	if (fd < 0) {
		buxton_log("inotify_init(): %m\n");
		return -1;
	}
	if (inotify_add_watch(fd, SMACK_LOAD_FILE, IN_CLOSE_WRITE) < 0) {
		buxton_log("inotify_add_watch(): %m\n");
		return -1;
	}
	return fd;
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
