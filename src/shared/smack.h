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

#pragma once

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include "bt-daemon.h"

/**
 * Maximum length for a Smack label
 */
#define SMACK_LABEL_LEN 255

/**
 * Smack label xattr key
 */
#define SMACK_ATTR_NAME "security.SMACK64"

/**
 * Maximum length of a Smack rule access string
 */
#define ACC_LEN 5

/**
 * Represents client access to a given resource
 */
typedef enum BuxtonKeyAccessType {
	ACCESS_NONE = 0, /**<No access permitted */
	ACCESS_READ = 1 << 0, /**<Read access permitted */
	ACCESS_WRITE = 1 << 1, /**<Write access permitted */
	ACCESS_MAXACCESSTYPES = 1 << 2
} BuxtonKeyAccessType;

/**
 * Load Smack rules from the kernel
 * @return a boolean value, indicating success of the operation
 */
bool buxton_cache_smack_rules(void);

/**
 * Check whether the smack access matches the buxton client access
 * @param subject Smack subject label
 * @param object Smack object label
 * @param request The buxton access type being queried
 * @return true if the smack access matches the given request, otherwise false
 */
bool buxton_check_smack_access(BuxtonString *subject,
			       BuxtonString *object,
			       BuxtonKeyAccessType request);

/**
 * Set up inotify to track Smack rule file for changes
 * @return an exit code for the operation
 */
int buxton_watch_smack_rules(void);

/**
 * Check whether the client has read access for a key in a layer
 * @param client The current BuxtonClient
 * @param layer The layer name for the key
 * @param key The key to check
 * @param data The data for the key
 * @param client_label The Smack label of the client
 * @return true if read access is granted, otherwise false
 */
bool buxton_check_read_access(BuxtonClient *client, BuxtonString *layer,
			      BuxtonString *key, BuxtonData *data,
			      BuxtonString *client_label);

/**
 * Check whether the client has write access for a key in a layer
 * @param client The current BuxtonClient
 * @param layer The layer name for the key
 * @param key The key to check
 * @param data The data for the key
 * @param client_label The Smack label of the client
 * @return true if write access is granted, otherwise false
 */
bool buxton_check_write_access(BuxtonClient *client, BuxtonString *layer,
			       BuxtonString *key, BuxtonData *data,
			       BuxtonString *client_label);


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
