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

#include "backend.h"
#include "buxton.h"

/**
 * Maximum length for a Smack label
 */
#define SMACK_LABEL_LEN 255

/**
 * Smack label xattr key
 */
#define SMACK_ATTR_NAME "security.SMACK64"

/**
 * Smackfs mount directory
 */
#define SMACK_MOUNT_DIR "/sys/fs/smackfs"

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
 * Check whether Smack is enabled in buxtond
 * @return a boolean value, indicating whether Smack is enabled
 */
bool buxton_smack_enabled(void)
	__attribute__((warn_unused_result));

/**
 * Load Smack rules from the kernel
 * @return a boolean value, indicating success of the operation
 */
bool buxton_cache_smack_rules(void)
	__attribute__((warn_unused_result));

/**
 * Check whether the smack access matches the buxton client access
 * @param subject Smack subject label
 * @param object Smack object label
 * @param request The buxton access type being queried
 * @return true if the smack access matches the given request, otherwise false
 */
bool buxton_check_smack_access(BuxtonString *subject,
			       BuxtonString *object,
			       BuxtonKeyAccessType request)
	__attribute__((warn_unused_result));

/**
 * Set up inotify to track Smack rule file for changes
 * @return an exit code for the operation
 */
int buxton_watch_smack_rules(void)
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
