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
 * \file configurator.h Internal header
 * This file is used internally by buxton to provide functionality
 * used to handle the configuration
 */
#pragma once

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

typedef enum ConfigKey {
	CONFIG_MIN = 0,
	CONFIG_CONF_FILE,
	CONFIG_MODULE_DIR,
	CONFIG_DB_PATH,
	CONFIG_SMACK_LOAD_FILE,
	CONFIG_BUXTON_SOCKET,
	CONFIG_MAX
} ConfigKey;

/**
 * Slightly duplicative of BuxtonLayer, but defined here instead of
 * there. This will probably be deprecated for BuxtonLayer once
 * things are integrated.
 */
typedef struct ConfigLayer {
	char *name;
	char *type;
	char *backend;
	char *description;
	char *access;
	int priority;
} ConfigLayer;

/**
 * @internal
 * @brief Add command line data
 *
 * @note This API is draft
 */
void buxton_add_cmd_line(ConfigKey confkey, char* data);

/**
 * @internal
 * @brief Get the directory of plugin modules
 *
 * @return the path to the plugin module. Do not free this pointer.
 * It belongs to configurator.
 */
const char *buxton_module_dir(void)
	__attribute__((warn_unused_result));

/**
 * @internal
 * @brief Get the path of the config file
 *
 * @return the path of the config file. Do not free this pointer.
 * It belongs to configurator.
 */
const char *buxton_conf_file(void)
	__attribute__((warn_unused_result));

/**
 * @internal
 * @brief Get the path of the buxton database
 *
 *
 * @return the path of the database file. Do not free this pointer.
 * It belongs to configurator.
 */
const char *buxton_db_path(void)
	__attribute__((warn_unused_result));

/**
 * @internal
 * @brief Get the path of the smack load file.
 *
 *
 * @return the path of the smack load file. Do not free this pointer.
 * It belongs to configurator.
 */
const char *buxton_smack_load_file(void)
	__attribute__((warn_unused_result));

/**
 * @internal
 * @brief Get the path of the buxton socket.
 *
 *
 * @return the path of the buxton socket. Do not free this pointer.
 * It belongs to configurator.
 */
const char *buxton_socket(void)
	__attribute__((warn_unused_result));

/**
 * @internal
 * @brief Get an array of ConfigLayers from the conf file
 *
 * @param layers pointer to a pointer where the array of ConfigLayers
 * will be stored. Callers should free this pointer with free when
 * they are done with it.
 *
 * @return an integer that indicates the number of layers.
 */
int buxton_key_get_layers(ConfigLayer **layers)
	__attribute__((warn_unused_result));

void include_configurator(void);

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
