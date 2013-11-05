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
 * \file client.h Internal header
 * This file is used internally by buxton to provide functionality
 * used for buxtonctl
 */
#pragma once

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include "hashmap.h"

/**
 * Store a command reference in Buxton CLI
 * @param type Type of data to operate on
 * @param one Pointer to char for first parameter
 * @param two Pointer to char for second parameter
 * @param three Pointer to char for third parameter
 * @return a boolean value, indicating success of the operation
 */
typedef bool (*command_method) (BuxtonClient *self, BuxtonDataType type, char *one, char *two, char *three);

/**
 * Defines a command within the buxtonctl cli
 */
typedef struct Command {
	const char     *name; /**<name of the command*/
	const char     *description; /**<one line description of the command*/
	unsigned int   min_arguments; /**minimum number of arguments */
	unsigned int   max_arguments; /**<maximum number of arguments */
	const char     *usage; /**<correct usage of the command */
	command_method method; /**<pointer to a method */
	BuxtonDataType type; /**<type of data to operate on */
} Command;

/**
 * Set a value in Buxton
 * @param self Client instance being run
 * @param type Type of data being set
 * @param one Layer of data being set
 * @param two Key of the data being set
 * @param three Value of the data to set
 * @returns bool indicating success or failure
 */
bool cli_set_value(BuxtonClient *self, BuxtonDataType type, char *one, char *two, char *three);

/**
 * Get a value from Buxton
 * @param self Client instance being run
 * @param type Type of data being sought
 * @param one Layer or key of data being set
 * @param two Key if one is Layer
 * @param three NULL (unused)
 * @returns bool indicating success or failure
 */
bool cli_get_value(BuxtonClient *self, BuxtonDataType type, char *one, char *two, __attribute__((unused)) char *three);

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
