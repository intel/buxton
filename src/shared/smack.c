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
#include "bt-daemon.h"
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
	bool have_rules = false;

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

	do {
		int r;
		int chars;
		BuxtonKeyAccessType *accesstype;

		char subject[SMACK_LABEL_LEN] = { 0, };
		char object[SMACK_LABEL_LEN] = { 0, };
		char access[ACC_LEN] = { 0, };

		/* read contents from load2 */
		chars = fscanf(load_file, "%s %s %s\n", subject, object, access);

		if (ferror(load_file)) {
			buxton_log("fscanf(): %m\n");
			ret = false;
			goto end;
		}

		if (!have_rules && chars == EOF && feof(load_file)) {
			buxton_debug("No loaded Smack rules found\n");
			goto end;
		}

		if (chars != 3) {
			buxton_log("Corrupt load file detected\n");
			ret = false;
			goto end;
		}

		have_rules = true;

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

	} while (!feof(load_file));

end:
	if (load_file)
		fclose(load_file);

	return ret;
}

bool buxton_check_smack_access(BuxtonString *subject, BuxtonString *object, BuxtonKeyAccessType request)
{
	_cleanup_free_ char *key = NULL;
	int r;
	BuxtonKeyAccessType *access;

	assert(subject);
	assert(object);
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
		exit(EXIT_FAILURE);
	}

	buxton_debug("Key: %s\n", key);

	access = hashmap_get(_smackrules, key);
	if (!access) {
		/* A null value is not an error, since clients may try to
		 * read/write keys with labels that are not in the loaded
		 * rule set. In this situation, access is simply denied,
		 * because there are no further rules to consider.
		 */
		buxton_debug("Value of key '%s' is NULL\n", key);
		return false;
	}

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

bool buxton_check_read_access(BuxtonClient *client,
			      BuxtonString *layer,
			      BuxtonString *key,
			      BuxtonData *data,
			      BuxtonString *client_label)
{
	assert(client);
	/* FIXME: reinstate the layer assert after get_value checks
	 * are in the correct spot
	 */
	assert(key);
	assert(data);
	assert(client_label);

	char *group = buxton_get_group(key);
	char *name = buxton_get_name(key);
	if (!group) {
		buxton_log("Invalid group or key: %s\n", key->value);
		return false;
	}

	if (!name) {
		/* Permit global read access for group labels, so
		 * do nothing here.
		 */
	} else {
		/* TODO: An additonal check is needed for labels:
		 * When checking read access for a key label, we
		 * should treat its group label as the object; if
		 * the group label is readable, the key label
		 * should also be readable.
		 */
		if (!buxton_check_smack_access(client_label,
					       &(data->label),
					       ACCESS_READ)) {
			buxton_debug("Smack: not permitted to get value\n");
			return false;
		}
	}

	return true;

}

bool buxton_check_write_access(BuxtonClient *client,
			       BuxtonString *layer,
			       BuxtonString *key,
			       BuxtonData *data,
			       BuxtonString *client_label)
{
	/* The data arg may be NULL, in case of a delete */
	assert(client);
	assert(layer);
	assert(key);
	assert(client_label);

	char *group = buxton_get_group(key);
	char *name = buxton_get_name(key);
	if (!group) {
		buxton_log("Invalid group or key: %s\n", key->value);
		return false;
	}

	if (!name) {
		/* TODO: Once non-direct clients are able to set
		 * group labels, we need an access check here.
		 */
	} else {
		BuxtonData *curr_data = malloc0(sizeof(BuxtonData));
		if (!curr_data) {
			buxton_log("malloc0: %m\n");
			return false;
		}

		bool valid = buxton_client_get_value_for_layer(client,
							       layer,
							       key,
							       curr_data);

		if (data && !valid && !buxton_check_smack_access(client_label,
								 &(data->label),
								 ACCESS_WRITE)) {
			buxton_debug("Smack: not permitted to set new value\n");
			free(curr_data);
			return false;
		}

		if (valid) {
			if (!buxton_check_smack_access(client_label,
						       &(curr_data->label),
						       ACCESS_WRITE)) {
				buxton_debug("Smack: not permitted to modify existing value\n");
				free(curr_data);
				return false;
			}

			if (data) {
				/* The existing label should be preserved */
				free(data->label.value);
				data->label.value = strdup(curr_data->label.value);
				if (!data->label.value) {
					buxton_log("strdup: %m\n");
					return false;
				}
				data->label.length = curr_data->label.length;
			}
		}

		free(curr_data);
	}

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
