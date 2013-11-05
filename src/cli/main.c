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
#include <errno.h>
#include <getopt.h>

#include "bt-daemon.h"
#include "client.h"
#include "hashmap.h"
#include "log.h"
#include "util.h"

static Hashmap *commands;
static BuxtonClient client;

static bool print_help(void)
{
	const char *key;
	Iterator iterator;
	Command *command;

	printf("buxtonctl: Usage\n\n");

	HASHMAP_FOREACH_KEY(command, key, commands, iterator) {
		printf("\t%10s - %s\n", key, command->description);
	};

	return true;
}

static void print_usage(Command *command)
{
	if (command->min_arguments == command->max_arguments)
		printf("%s takes %d arguments - %s\n", command->name, command->min_arguments, command->usage);
	else
		printf("%s takes at least %d arguments - %s\n", command->name, command->min_arguments, command->usage);
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
	Command c_get_string, c_set_string;
	Command c_get_bool, c_set_bool;
	Command c_get_float, c_set_float;
	Command c_get_double, c_set_double;
	Command c_get_int, c_set_int;
	Command c_get_long, c_set_long;
	Command *command;
	int i = 0;
	int c;
	bool help = false;

	/* Build a command list */
	commands = hashmap_new(string_hash_func, string_compare_func);

	/* Strings */
	c_get_string = (Command) { "get-string", "Get a string value by key",
				   1, 2, "[layer] key", &cli_get_value, STRING };
	hashmap_put(commands, c_get_string.name, &c_get_string);

	c_set_string = (Command) { "set-string", "Set a key with a string value",
				   3, 3, "layer key value", &cli_set_value, STRING };
	hashmap_put(commands, c_set_string.name, &c_set_string);

	/* Booleans */
	c_get_bool = (Command) { "get-bool", "Get a boolean value by key",
				   1, 2, "[layer] key", &cli_get_value, BOOLEAN };
	hashmap_put(commands, c_get_bool.name, &c_get_bool);

	c_set_bool = (Command) { "set-bool", "Set a key with a boolean value",
				   3, 3, "layer key value", &cli_set_value, BOOLEAN };
	hashmap_put(commands, c_set_bool.name, &c_set_bool);

	/* Floats */
	c_get_float = (Command) { "get-float", "Get a float point value by key",
				   1, 2, "[layer] key", &cli_get_value, FLOAT };
	hashmap_put(commands, c_get_float.name, &c_get_float);

	c_set_float = (Command) { "set-float", "Set a key with a floating point value",
				   3, 3, "layer key value", &cli_set_value, FLOAT };
	hashmap_put(commands, c_set_float.name, &c_set_float);

	/* Doubles */
	c_get_double = (Command) { "get-double", "Get a double precision value by key",
				   1, 2, "[layer] key", &cli_get_value, DOUBLE };
	hashmap_put(commands, c_get_double.name, &c_get_double);

	c_set_double = (Command) { "set-double", "Set a key with a double precision value",
				   3, 3, "layer key value", &cli_set_value, DOUBLE };
	hashmap_put(commands, c_set_double.name, &c_set_double);

	/* Longs */
	c_get_long = (Command) { "get-long", "Get a long integer value by key",
				   1, 2, "[layer] key", &cli_get_value, LONG };
	hashmap_put(commands, c_get_long.name, &c_get_long);

	c_set_long = (Command) { "set-long", "Set a key with a long integer value",
				   3, 3, "layer key value", &cli_set_value, LONG };
	hashmap_put(commands, c_set_long.name, &c_set_long);

	/* Integers */
	c_get_int = (Command) { "get-int", "Get an integer value by key",
				1, 2, "[layer] key", &cli_get_value, INT };
	hashmap_put(commands, c_get_int.name, &c_get_int);

	c_set_int = (Command) { "set-int", "Set a key with an integer value",
				3, 3, "layer key value", &cli_set_value, INT };
	hashmap_put(commands, c_set_int.name, &c_set_int);

	static struct option opts[] = {
		{ "direct", 0, NULL, 'd' },
		{ "help",   0, NULL, 'h' },
		{ NULL, 0, NULL, 0 }
	};

	while(true) {
		c = getopt_long(argc, argv, "dh", opts, &i);

		if (c == -1)
			break;

		switch (c) {
		case 'd':
			if (geteuid() != 0) {
				printf("Only root may use --direct\n");
				goto end;
			}
			client.direct = true;
			break;
		case 'h':
			help = true;
			break;
		}
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

	if ((argc - optind - 1 < command->min_arguments) ||
	    (argc - optind - 1 > command->max_arguments)) {
		print_usage(command);
		print_help();
		ret = false;
		goto end;
	}

	if (client.direct) {
		if (!buxton_direct_open(&client)){
			buxton_log("Failed to directly talk to Buxton\n");
			ret = false;
			goto end;
		}
	} else {
		if (!buxton_client_open(&client)) {
			buxton_log("Failed to talk to Buxton\n");
			ret = false;
			goto end;
		}
	}

	/* Connected to buxton_client, execute method */
	ret = command->method(&client, command->type,
		optind + 1 < argc ? argv[optind + 1] : NULL,
		optind + 2 < argc ? argv[optind + 2] : NULL,
		optind + 3 < argc ? argv[optind + 3] : NULL);

end:
	hashmap_free(commands);
	buxton_client_close(&client);
	if (ret)
		return EXIT_SUCCESS;
	return EXIT_FAILURE;
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
