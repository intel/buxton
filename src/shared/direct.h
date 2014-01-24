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

/**
 * \file direct.h Internal header
 * This file is used internally by buxton to provide functionality
 * used by the daemon and buxtonctl for talking to the backend
 */
#pragma once

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include <backend.h>
#include "buxton.h"
#include "hashmap.h"

/**
 * Open a direct connection to Buxton
 *
 * @param control Valid BuxtonControl instance
 * @return a boolean value, indicating success of the operation
 */
bool buxton_direct_open(BuxtonControl *control)
	__attribute__((warn_unused_result));

/**
 * Close direct Buxton management connection
 * @param control Valid BuxtonControl instance
 */
void buxton_direct_close(BuxtonControl *control);

/**
 * Set a value within Buxton
 * @param control An initialized control structure
 * @param layer The layer to manipulate
 * @param key The key name
 * @param label A BuxtonString containing the label to set
 * @return A boolean value, indicating success of the operation
 */
bool buxton_direct_set_label(BuxtonControl *control,
			     BuxtonString *layer,
			     BuxtonString *key,
			     BuxtonString *label)
	__attribute__((warn_unused_result));

/**
 * Set a value within Buxton
 * @param control An initialized control structure
 * @param layer The layer to manipulate
 * @param key The key name
 * @param data A struct containing the data to set
 * @param label The Smack label for the client
 * @return A boolean value, indicating success of the operation
 */
bool buxton_direct_set_value(BuxtonControl *control,
			     BuxtonString *layer_name,
			     BuxtonString *key,
			     BuxtonData *data,
			     BuxtonString *label)
	__attribute__((warn_unused_result));

/**
 * Retrieve a value from Buxton
 * @param control An initialized control structure
 * @param key The key to retrieve
 * @param data An empty BuxtonData, where data is stored
 * @param label The Smack label of the client
 * @return A boolean value, indicating success of the operation
 */
bool buxton_direct_get_value(BuxtonControl *control,
			     BuxtonString *key,
			     BuxtonData *data,
			     BuxtonString *label)
	__attribute__((warn_unused_result));

/**
 * Retrieve a value from Buxton
 * @param control An initialized control structure
 * @param layer The layer where the key is set
 * @param key The key to retrieve
 * @param data An empty BuxtonData, where data is stored
 * @param label The Smack label of the client
 * @return A boolean value, indicating success of the operation
 */
bool buxton_direct_get_value_for_layer(BuxtonControl *control,
				       BuxtonString *layer,
				       BuxtonString *key,
				       BuxtonData *data,
				       BuxtonString *label)
	__attribute__((warn_unused_result));

/**
 * Retrieve a list of keys from Buxton
 * @param control An initialized control structure
 * @param layer_name The layer to pquery
 * @param data An empty BuxtonArray, where results are stored
 * @return A boolean value, indicating success of the operation
 */
bool buxton_direct_list_keys(BuxtonControl *control,
			     BuxtonString *layer,
			     BuxtonArray **list)
	__attribute__((warn_unused_result));

/**
 * Unset a value by key in the given BuxtonLayer
 * @param control An initialized control structure
 * @param layer The layer to remove the value from
 * @param key The key to remove
 * @param label The Smack label of the client
 * @return a boolean value, indicating success of the operation
 */
bool buxton_direct_unset_value(BuxtonControl *control,
			       BuxtonString *layer,
			       BuxtonString *key,
			       BuxtonString *label)
	__attribute__((warn_unused_result));

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
