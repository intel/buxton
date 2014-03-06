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

#include "backend.h"
#include "hashmap.h"

/**
 * Store a command reference in Buxton CLI
 * @param type Type of data to operate on
 * @param one Pointer to char for first parameter
 * @param two Pointer to char for second parameter
 * @param three Pointer to char for third parameter
 * @param four Pointer to char for fourth parameter
 * @return a boolean value, indicating success of the operation
 */
typedef bool (*command_method) (BuxtonControl *control, BuxtonDataType type,
				char *one, char *two, char *three,
				char *four);

/**
 * Defines a command within the buxtonctl cli
 */
typedef struct Command {
	const char     *name; /**<name of the command*/
	const char     *description; /**<one line description of the command*/
	unsigned int   min_arguments; /**<minimum number of arguments */
	unsigned int   max_arguments; /**<maximum number of arguments */
	const char     *usage; /**<correct usage of the command */
	command_method method; /**<pointer to a method */
	BuxtonDataType type; /**<type of data to operate on */
} Command;

/**
 * Create an initialized, empty db
 * @param control An initialized control structure
 * @param type Unused
 * @param one Layer of label being set
 * @param two Unused
 * @param three Unused
 * @param four Unused
 * @returns bool indicating success or failure
 */
bool cli_create_db(BuxtonControl *control,
		   BuxtonDataType type,
		   char *one,
		   char *two,
		   char *three,
		   char *four)
	__attribute__((warn_unused_result));

/**
 * Set a label in Buxton
 * @param control An initialized control structure
 * @param type Type of label being set (unused)
 * @param one Layer of label being set
 * @param two Group of the label being set
 * @param three Name of the label to set
 * @param four Label of the label to set
 * @returns bool indicating success or failure
 */
bool cli_set_label(BuxtonControl *control,
		   __attribute__((unused)) BuxtonDataType type,
		   char *one, char *two, char *three, char *four)
	__attribute__((warn_unused_result));

/**
 * Create a group in Buxton
 * @param control An initialized control structure
 * @param type Type of group being created (unused)
 * @param one Layer for the group being created
 * @param two Name of the group
 * @returns bool indicating success or failure
 */
bool cli_create_group(BuxtonControl *control,
		      __attribute__((unused)) BuxtonDataType type,
		      char *one, char *two,
		      __attribute__((unused)) char *three,
		      __attribute__((unused)) char *four)
	__attribute__((warn_unused_result));

/**
 * Remove a group in Buxton
 * @param control An initialized control structure
 * @param type Type of group being removed (unused)
 * @param one Layer for the group being removed
 * @param two Name of the group
 * @returns bool indicating success or failure
 */
bool cli_remove_group(BuxtonControl *control,
		      __attribute__((unused)) BuxtonDataType type,
		      char *one, char *two,
		      __attribute__((unused)) char *three,
		      __attribute__((unused)) char *four)
	__attribute__((warn_unused_result));

/**
 * Get a label from Buxton
 * @param control An initialized control structure
 * @param type Type of label being sought (unused)
 * @param one Layer of the label being sought
 * @param two Group of the label being sought
 * @param two Name of the label being sought
 * @param four NULL (unused)
 * @returns bool indicating success or failure
 */
bool cli_get_label(BuxtonControl *control,
		   __attribute__((unused)) BuxtonDataType type,
		   char *one, char *two, char *three,
		   __attribute__((unused)) char *four)
	__attribute__((warn_unused_result));

/**
 * Set a value in Buxton
 * @param control An initialized control structure
 * @param type Type of data being set
 * @param one Layer of data being set
 * @param two Group of the data being set
 * @param three Name of the data to set
 * @param three Value of the data to set
 * @returns bool indicating success or failure
 */
bool cli_set_value(BuxtonControl *control, BuxtonDataType type, char *one,
		   char *two, char *three, char *four)
	__attribute__((warn_unused_result));

/**
 * Get a value from Buxton
 * @param control An initialized control structure
 * @param type Type of data being sought
 * @param one Layer or Group of data being set
 * @param two  or key of data being set
 * @param two Key if one is Layer
 * @param three NULL (unused)
 * @returns bool indicating success or failure
 */
bool cli_get_value(BuxtonControl *control, BuxtonDataType type, char *one,
		   char *two, char *three,
		   __attribute__((unused)) char *four)
	__attribute__((warn_unused_result));

/*
 * List keys for a layer in Buxton
 * @param control An initialized control structure
 * @param type Type of data (unused)
 * @param one Layer to query
 * @param two NULL (unused)
 * @param three NULL (unusued)
 * @param four NULL (unused)
 * @returns bool indicating success or failure
 */
bool cli_list_keys(BuxtonControl *control,
		   __attribute__((unused))BuxtonDataType type,
		   char *one, char *two, char *three,
		   __attribute__((unused)) char *four)
	__attribute__((warn_unused_result));

/**
 * Unset a value from Buxton
 * @param control An initialized control structure
 * @param type Type of value (unused)
 * @param one Layer of data being unset
 * @param two Group of key to unset
 * @param three Key of value to unset
 * @param four NULL (unused)
 * @returns bool indicating success or failure
 */
bool cli_unset_value(BuxtonControl *control,
		     __attribute__((unused))BuxtonDataType type,
		     char *one, char *two, char *three,
		     __attribute__((unused)) char *four)
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
