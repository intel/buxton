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

#include <assert.h>
#include <iniparser.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "configurator.h"
#include "log.h"

/**
 * Section name in ini file for our configuration
 */
#define CONFIG_SECTION "Configuration"

#ifndef HAVE_SECURE_GETENV
#  ifdef HAVE___SECURE_GETENV
#    define secure_getenv __secure_getenv
#  else
#    error neither secure_getenv nor __secure_getenv is available
#  endif
#endif

/**
 * @internal
 * internal state of configurator
 */
static struct _conf {
	bool initialized;	/**< track if it's been init'd */
	char* keys[CONFIG_MAX];	/**< bag of values */
	dictionary* ini;	/**< ini parser dictionary */
} conf = {
	.initialized = false,
	.keys = {0},
	.ini = NULL
};

/**
 * @internal
 * config keys as in environment
 * @fixme name sucks
 */
static const char *KS[CONFIG_MAX] = {
	NULL,
	"BUXTON_CONF_FILE",
	"BUXTON_MODULE_DIR",
	"BUXTON_DB_PATH",
	"BUXTON_SMACK_LOAD_FILE",
	"BUXTON_BUXTON_SOCKET"
};

/**
 * @internal
 * keys as they are used in config file
 */
static const char *config_keys[CONFIG_MAX] = {
	NULL,
	NULL,			/**< conf file entry in config file is meaningless */
	"ModuleDirectory",
	"DatabasePath",
	"SmackLoadFile",
	"SocketPath"
};

static const char *COMPILE_DEFAULT[CONFIG_MAX] = {
	NULL,
	_DEFAULT_CONFIGURATION_FILE,
	_MODULE_DIRECTORY,
	_DB_PATH,
	_SMACK_LOAD_FILE,
	_BUXTON_SOCKET
};

/**
 * @internal
 * wrap strdup and die if malloc fails
 *
 * @note This may seem lame and stupid at first glance. The former is
 * right the latter is not. This function may abort()
 *
 * @return pointer to dup'd string
 */
__attribute__ ((pure))
static inline char *_strdup(const char* string)
{
	char* s;

	s = strdup(string);
	if (s == NULL) {
		abort();
	}
	return s;
}

/**
 * @internal
 * grab a string from the ini file
 *
 * @param section the section of the ini file
 * @param name the name of the key
 * @param required if key required or not
 * @param def default value for nonrequired setting
 *
 * @note This function may abort()
 *
 * @return The value as a string. The the string is internal to
 * libini, and if at this stage of the game it is null, just die.
 * Something is really wrong and even if we could recover, the system
 * is not working correctly.
 */
static char *get_ini_string(char *section, char *name, bool required,
	char *def)
{
	char buf[PATH_MAX];
	char *s;

	assert(conf.ini);
	snprintf(buf, sizeof(buf), "%s:%s", section, name);
	s = iniparser_getstring(conf.ini, buf, def);
	if (s == NULL && required) {
		abort();
	}
	return s;
}

/**
 * analagous method to get_ini_string()
 *
 * @param section the section of the ini file
 * @param name the name of the key
 * @param required if key required or not
 * @param def default value for nonrequired setting
 *
 * @note inlined b/c only used once and why not.
 *
 * @return the value
 */
static inline int get_ini_int(char *section, char *name, bool required,
	int def)
{
	char buf[PATH_MAX];
	int exists;

	assert(conf.ini);
	snprintf(buf, sizeof(buf), "%s:%s", section, name);
	exists = iniparser_find_entry(conf.ini, buf);
	if (!exists && required) {
		abort();
	}
	return iniparser_getint(conf.ini, buf, def);
}

/**
 * @internal
 * Initialize conf
 *
 */
static void initialize(void)
{
	if (conf.initialized) {
		return;
	}

	for (int i= CONFIG_MIN+1; i < CONFIG_MAX; i++) {
		char *envkey;

		/* hasn't been set through command line */
		if (conf.keys[i] == NULL) {
			/* second priority is env */
			envkey = secure_getenv(KS[i]);
			if (envkey) {
				conf.keys[i] = _strdup(envkey);
			}
		}

		/* third priority is conf file */
		if (conf.keys[i] == NULL && conf.ini) {
			/* for once config file is loaded */
			char key[PATH_MAX+strlen(CONFIG_SECTION)+1+1]; /* one for null and one for : */
			char *value;

			snprintf(key, sizeof(key), "%s:%s", CONFIG_SECTION, config_keys[i]);
			value = iniparser_getstring(conf.ini, key, NULL);
			if (value != NULL) {
				conf.keys[i] = _strdup(value);
			}
		}

		if (conf.keys[i] == NULL) {
			/* last priority is compile time defaults */
			conf.keys[i] = _strdup(COMPILE_DEFAULT[i]);
		}

		/* if config file is not loaded, load */
		if (i == CONFIG_CONF_FILE) {
			char *path = conf.keys[CONFIG_CONF_FILE];

			conf.ini = iniparser_load(path);
			if (conf.ini == NULL) {
				buxton_log("Failed to load buxton conf file: %s\n", path);
			}
		}
	}
	conf.initialized = true;
}

/**
 * @internal
 * use a destructor to free everything rather than yucky at_exit
 * nonsense
 *
 * @return
 */
__attribute__ ((destructor)) static void free_conf(void)
{
	if (!conf.initialized) {
		return;
	}
	for (int i= CONFIG_MIN+1; i < CONFIG_MAX; i++) {
		free(conf.keys[i]);
	}
	iniparser_freedict(conf.ini);
}

void buxton_add_cmd_line(ConfigKey confkey, char* data)
{
	if (confkey >= CONFIG_MAX || confkey <= CONFIG_MIN) {
		buxton_log("invalid confkey");
		return;
	}
	if (!data) {
		buxton_log("invalid data");
		return;
	}

	conf.keys[confkey] = _strdup(data);
}

const char* buxton_module_dir(void)
{
	initialize();
	return (const char*)conf.keys[CONFIG_MODULE_DIR];
}

const char* buxton_conf_file(void)
{
	initialize();
	return (const char*)conf.keys[CONFIG_CONF_FILE];
}

const char* buxton_db_path(void)
{
	initialize();
	return (const char*)conf.keys[CONFIG_DB_PATH];
}

const char* buxton_smack_load_file(void)
{
	initialize();
	return (const char*)conf.keys[CONFIG_SMACK_LOAD_FILE];
}

const char* buxton_socket(void)
{
	initialize();
	return (const char*)conf.keys[CONFIG_BUXTON_SOCKET];
}

int buxton_key_get_layers(ConfigLayer **layers)
{
	ConfigLayer *_layers;
	int n;
	int j = 0;

	assert(layers);
	initialize();
	if (conf.ini == NULL) {
		buxton_log("config file not loaded when calling buxton_key_get_layers()");
		abort();
	}
	n = iniparser_getnsec(conf.ini);
	_layers = (ConfigLayer*)calloc((size_t)n, sizeof(ConfigLayer));
	if (_layers == NULL) {
		abort();
	}
	for (int i= 0; i < n; i++) {
		char *section_name;

		section_name = iniparser_getsecname(conf.ini, i);
		if (!section_name) {
			abort();
		}
		if (!strcasecmp(section_name, CONFIG_SECTION)) {
			continue;
		}
		_layers[j].name = section_name;
		_layers[j].description = get_ini_string(section_name,
			"Description", true, NULL);
		_layers[j].backend = get_ini_string(section_name, "Backend",
			true, NULL);
		_layers[j].type = get_ini_string(section_name, "Type", true,
			NULL);
		_layers[j].priority = get_ini_int(section_name, "Priority",
			true, 0);
		_layers[j].access = get_ini_string(section_name, "Access",
			false, "read-write");
		j++;
	}
	*layers = _layers;
	return j;
}

void include_configurator(void)
{
	;
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
