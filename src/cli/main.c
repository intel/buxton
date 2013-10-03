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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../shared/log.h"
#include "../include/bt-daemon.h"
#include "../include/bt-daemon-private.h"
#include "../shared/hashmap.h"

static Hashmap *commands;
static BuxtonClient client;

typedef bool (*command_method) (int argc, char **argv);

typedef struct Command {
	const char     *name;
	const char     *description;
	command_method method;
} Command;

/* Print help message */
bool print_help(int argc, char **argv) {
	const char *key;
	Iterator iterator;
	Command *command;

	printf("buxtonctl: Usage\n\n");

	HASHMAP_FOREACH_KEY(command, key, commands, iterator) {
		printf("\t%10s - %s\n", key, command->description);
	};

	return true;
}

/* Set a string in Buxton */
bool set_string(int argc, char **argv) {

	return true;
}

/* Get a string from Buxton */
bool get_string(int argc, char **argv) {

	return true;
}

int main(int argc, char **argv) {
	bool ret = false;
	int arg_n = 1;
	Command c_get_string, c_set_string;
	Command c_help;
	Command *command;

	/* Build a command list */
	commands = hashmap_new(string_hash_func, string_compare_func);

	c_get_string.name = "get-string";
	c_get_string.description = "Get a string value by key";
	c_get_string.method = &get_string;
	hashmap_put(commands, c_get_string.name, &c_get_string);

	c_set_string.name = "set-string";
	c_set_string.description = "Set a key with a string value";
	c_set_string.method = &set_string;
	hashmap_put(commands, c_set_string.name, &c_set_string);

	c_help.name = "help";
	c_help.description = "Print this help message";
	c_help.method = &print_help;
	hashmap_put(commands, c_help.name, &c_help);

	if (argc > 1 && strncmp(argv[1], "--direct", 8) == 0) {
		if (geteuid() != 0) {
			printf("Only root may use --direct\n");
			goto end;
		}
		client.direct = true;
		arg_n++;
	}
		
	if (argc < 3) {
		/* Todo: print usage if a valid command name was given */
		print_help(argc, argv);
		goto end;
	}
	
	if ((command = hashmap_get(commands, argv[arg_n])) == NULL) {
		printf("Unknown command: %s\n", argv[arg_n]);
		goto end;
	}

	/* We now execute the command */
	if (command->method == NULL) {
		printf("Not yet implemented: %s\n", command->name);
		goto end;
	}

	if (strncmp(command->name, "help", 4) == 0) {
		/* Ensure we cleanup and abort when using help */
		command->method(argc, argv);
		ret = true;
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
	ret = command->method(argc, argv);

end:
	hashmap_free(commands);
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
