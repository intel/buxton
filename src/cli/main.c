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

/**
 * \file cli/main.c Buxton command line interface
 *
 * Provides a CLI to Buxton through which a variety of operations can be
 * carried out
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <getopt.h>

#include "buxton.h"
#include "backend.h"
#include "client.h"
#include "configurator.h"
#include "direct.h"
#include "hashmap.h"
#include "protocol.h"
#include "util.h"

static Hashmap *commands;
static BuxtonControl control;

static void print_version(void)
{
	printf("buxtonctl " PACKAGE_VERSION "\n"
	       "Copyright (C) 2013-2014 Intel Corporation\n"
	       "buxton is free software; you can redistribute it and/or modify\n"
	       "it under the terms of the GNU Lesser General Public License as\n"
	       "published by the Free Software Foundation; either version 2.1\n"
	       "of the License, or (at your option) any later version.\n");
}

static bool print_help(void)
{
	const char *key;
	Iterator iterator;
	Command *command;

	printf("buxtonctl: Usage\n\n");

	HASHMAP_FOREACH_KEY(command, key, commands, iterator) {
		printf("\t%12s - %s\n", key, command->description);
	};

	return true;
}

static void print_usage(Command *command)
{
	if (command->min_arguments == command->max_arguments) {
		printf("%s takes %d arguments - %s\n", command->name, command->min_arguments, command->usage);
	} else {
		printf("%s takes at least %d arguments - %s\n", command->name, command->min_arguments, command->usage);
	}
}

/**
 * Entry point into buxtonctl
 * @param argc Number of arguments passed
 * @param argv An array of string arguments
 * @returns EXIT_SUCCESS if the operation succeeded, otherwise EXIT_FAILURE
 */
int main(int argc, char **argv)
{
	bool ret = false;
	Command c_check;
	Command c_get;
	Command c_get_string, c_set_string;
	Command c_get_int32, c_set_int32;
	Command c_get_uint32, c_set_uint32;
	Command c_get_int64, c_set_int64;
	Command c_get_uint64, c_set_uint64;
	Command c_get_float, c_set_float;
	Command c_get_double, c_set_double;
	Command c_get_bool, c_set_bool;
	Command c_get_label, c_set_label;
	Command c_create_group, c_remove_group;
	Command c_unset_value;
	Command c_create_db;
	Command c_list_groups, c_list_keys;
	Command *command;
	int i = 0;
	int c;
	bool direct = false;
	bool server = false;
	bool help = false;
	bool version = false;
	control.client.direct = false;
	char *conf_path = NULL;
	BuxtonClient client = NULL;

	/* libtool bites my twinkie */
	include_configurator();
	include_protocol();
	include_serialize();

	/* Build a command list */
	commands = hashmap_new(string_hash_func, string_compare_func);
	if (!commands) {
		exit(EXIT_FAILURE);
	}

	/* check access */
	c_check = (Command) { "check", "Check the availability of Buxton",
				   0, 0, "", &cli_check_availability, BUXTON_TYPE_UNSET };
	hashmap_put(commands, c_check.name, &c_check);

	/* unknowns */
	c_get = (Command) { "get", "Get a value by key",
				   2, 3, "[layer] group name", &cli_get_value, BUXTON_TYPE_UNSET };
	hashmap_put(commands, c_get.name, &c_get);

	/* Strings */
	c_get_string = (Command) { "get-string", "Get a string value by key",
				   2, 3, "[layer] group name", &cli_get_value, BUXTON_TYPE_STRING };
	hashmap_put(commands, c_get_string.name, &c_get_string);

	c_set_string = (Command) { "set-string", "Set a key with a string value",
				   4, 4, "layer group name value", &cli_set_value, BUXTON_TYPE_STRING };
	hashmap_put(commands, c_set_string.name, &c_set_string);

	/* 32bit Integers */
	c_get_int32 = (Command) { "get-int32", "Get an int32_t value by key",
				  2, 3, "[layer] group name", &cli_get_value, BUXTON_TYPE_INT32 };
	hashmap_put(commands, c_get_int32.name, &c_get_int32);

	c_set_int32 = (Command) { "set-int32", "Set a key with an int32_t value",
				  4, 4, "layer group name value", &cli_set_value, BUXTON_TYPE_INT32 };
	hashmap_put(commands, c_set_int32.name, &c_set_int32);

	/* Unsigned 32bit Integers */
	c_get_uint32 = (Command) { "get-uint32", "Get an uint32_t value by key",
				   2, 3, "[layer] group name", &cli_get_value, BUXTON_TYPE_UINT32 };
	hashmap_put(commands, c_get_uint32.name, &c_get_uint32);

	c_set_uint32 = (Command) { "set-uint32", "Set a key with an uint32_t value",
				   4, 4, "layer group name value", &cli_set_value, BUXTON_TYPE_UINT32 };
	hashmap_put(commands, c_set_uint32.name, &c_set_uint32);

	/* 32bit Integers */
	c_get_int64 = (Command) { "get-int64", "Get an int64_t value by key",
				  2, 3, "[layer] group name", &cli_get_value, BUXTON_TYPE_INT64};
	hashmap_put(commands, c_get_int64.name, &c_get_int64);

	c_set_int64 = (Command) { "set-int64", "Set a key with an int64_t value",
				  4, 4, "layer group name value", &cli_set_value, BUXTON_TYPE_INT64 };
	hashmap_put(commands, c_set_int64.name, &c_set_int64);

	/* Unsigned 32bit Integers */
	c_get_uint64 = (Command) { "get-uint64", "Get an uint64_t value by key",
				   2, 3, "[layer] group name", &cli_get_value, BUXTON_TYPE_UINT64};
	hashmap_put(commands, c_get_uint64.name, &c_get_uint64);

	c_set_uint64 = (Command) { "set-uint64", "Set a key with an uint64_t value",
				   4, 4, "layer group name value", &cli_set_value, BUXTON_TYPE_UINT64 };
	hashmap_put(commands, c_set_uint64.name, &c_set_uint64);

	/* Floats */
	c_get_float = (Command) { "get-float", "Get a float point value by key",
				  2, 3, "[layer] group name", &cli_get_value, BUXTON_TYPE_FLOAT };
	hashmap_put(commands, c_get_float.name, &c_get_float);

	c_set_float = (Command) { "set-float", "Set a key with a floating point value",
				  4, 4, "layer group name value", &cli_set_value, BUXTON_TYPE_FLOAT };
	hashmap_put(commands, c_set_float.name, &c_set_float);

	/* Doubles */
	c_get_double = (Command) { "get-double", "Get a double precision value by key",
				   2, 3, "[layer] group name", &cli_get_value, BUXTON_TYPE_DOUBLE };
	hashmap_put(commands, c_get_double.name, &c_get_double);

	c_set_double = (Command) { "set-double", "Set a key with a double precision value",
				   4, 4, "layer group name value", &cli_set_value, BUXTON_TYPE_DOUBLE };
	hashmap_put(commands, c_set_double.name, &c_set_double);

	/* Booleans */
	c_get_bool = (Command) { "get-bool", "Get a boolean value by key",
				 2, 3, "[layer] group name", &cli_get_value, BUXTON_TYPE_BOOLEAN };
	hashmap_put(commands, c_get_bool.name, &c_get_bool);

	c_set_bool = (Command) { "set-bool", "Set a key with a boolean value",
				 4, 4, "layer group name value", &cli_set_value, BUXTON_TYPE_BOOLEAN };
	hashmap_put(commands, c_set_bool.name, &c_set_bool);

	/* SMACK labels */
	c_get_label = (Command) { "get-label", "Get a value's label",
				  2, 3, "layer group [name]", &cli_get_label, BUXTON_TYPE_UNSET };
	hashmap_put(commands, c_get_label.name, &c_get_label);

	c_set_label = (Command) { "set-label", "Set a value's label",
				  3, 4, "layer group [name] label", &cli_set_label, BUXTON_TYPE_UNSET };
	hashmap_put(commands, c_set_label.name, &c_set_label);

	/* Group management */
	c_create_group = (Command) { "create-group", "Create a group in a layer",
				     2, 2, "layer group", &cli_create_group, BUXTON_TYPE_STRING };
	hashmap_put(commands, c_create_group.name, &c_create_group);

	c_remove_group = (Command) { "remove-group", "Remove a group from a layer",
				     2, 2, "layer group", &cli_remove_group, BUXTON_TYPE_STRING };
	hashmap_put(commands, c_remove_group.name, &c_remove_group);

	/* Unset value */
	c_unset_value = (Command) { "unset-value", "Unset a value by key",
				    3, 3, "layer group name", &cli_unset_value, BUXTON_TYPE_UNSET };
	hashmap_put(commands, c_unset_value.name, &c_unset_value);

	/* Create db for layer */
	c_create_db = (Command) { "create-db", "Create the database file for a layer",
				    1, 1, "layer", &cli_create_db, BUXTON_TYPE_STRING };
	hashmap_put(commands, c_create_db.name, &c_create_db);

	/* Listing of names */
	c_list_groups = (Command) { "list-groups", "List the groups for a layer",
				    1, 2, "layer [prefix-filter]", &cli_list_names, 0 };
	hashmap_put(commands, c_list_groups.name, &c_list_groups);

	c_list_keys = (Command) { "list-keys", "List the keys for a layer",
				    2, 3, "layer group [prefix-filter]", &cli_list_names, 1 };
	hashmap_put(commands, c_list_keys.name, &c_list_keys);

	static struct option opts[] = {
		{ "config-file", 1, NULL, 'c' },
		{ "direct",	 0, NULL, 'd' },
		{ "server",	 0, NULL, 's' },
		{ "help",	 0, NULL, 'h' },
		{ "version", 0, NULL, 'v' },
		{ NULL, 0, NULL, 0 }
	};

	while (true) {
		c = getopt_long(argc, argv, "c:sdvh", opts, &i);

		if (c == -1) {
			break;
		}

		switch (c) {
		case 'c':
			conf_path = strdup(optarg);
			if (!conf_path) {
				goto end;
			}
			break;
		case 'd':
			direct = true;
			break;
		case 's':
			server = true;
			break;
		case 'v':
			version = true;
			break;
		case 'h':
			help = true;
			break;
		}
	}

	if (version) {
		print_version();
		goto end;
	}

	if (optind == argc) {
		print_help();
		goto end;
	}

	if ((command = hashmap_get(commands, argv[optind])) == NULL) {
		printf("Unknown command: %s\n", argv[optind]);
		goto end;
	}

	/* We now execute the command */
	if (command->method == NULL) {
		printf("Not yet implemented: %s\n", command->name);
		goto end;
	}

	if (help) {
		/* Ensure we cleanup and abort when using help */
		print_usage(command);
		ret = false;
		goto end;
	}

	/* Check the argument count */
	if ((argc - optind - 1 < command->min_arguments) ||
	    (argc - optind - 1 > command->max_arguments)) {
		print_usage(command);
		print_help();
		ret = false;
		goto end;
	}

	/* Prepare opening */
	control.client.uid = geteuid();
	if (!direct && !server)
		direct = server = true; /* Set both opening mode by default */

	/* First, try to open the service */
	if (server) {
		if (conf_path) {
			if (buxton_set_conf_file(conf_path)) {
				printf("Failed to set configuration file path\n");
			}
		}
		if (buxton_open(&client) >= 0) {
			control.client = *(_BuxtonClient *)client;
			goto opened;
		}
	}

	/* Second, try to open directly */
	if (direct) {
		if (conf_path) {
			int r;
			struct stat st;

			r = stat(conf_path, &st);
			if (r == -1) {
				printf("Invalid configuration file path\n");
				goto end;
			} else {
				if (st.st_mode & S_IFDIR) {
					printf("Configuration file given is a directory\n");
					goto end;
				}
			}
			buxton_add_cmd_line(CONFIG_CONF_FILE, conf_path);
		}
		control.client.direct = true;
		if (buxton_direct_open(&(control))) {
			goto opened;
		}
	}

	/* Error on opening buxton */
	if (direct && server) {
		printf("Failed to contact the buxton service or directly open the database(s)\n");
	} else if (!direct && server) {
		printf("Failed to contact the Buxton service\n");
	} else if (direct && !server) {
		printf("Failed to use Buxton directly\n");
	}
	ret = false;
	goto end;

opened:

	/* Connected to buxton_client, execute method */
	ret = command->method(&control, command->type,
			      optind + 1 < argc ? argv[optind + 1] : NULL,
			      optind + 2 < argc ? argv[optind + 2] : NULL,
			      optind + 3 < argc ? argv[optind + 3] : NULL,
			      optind + 4 < argc ? argv[optind + 4] : NULL);

end:
	free(conf_path);
	hashmap_free(commands);
	if (control.client.direct) {
		buxton_direct_close(&control);
	} else {
		if (client) {
			buxton_close(client);
		}
	}

	if (ret) {
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

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
