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

#include <check.h>
#include <iniparser.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "configurator.h"

#ifdef NDEBUG
	#error "re-run configure with --enable-debug"
#endif

static void fail_strne(const char *value, char *correct_value, bool casecmp)
{
	char buf[PATH_MAX];
	int ret;

	snprintf(buf, sizeof(buf), "%s was not %s", value, correct_value);
	if (casecmp)
		ret = strcasecmp(value, correct_value);
	else
		ret = strcmp(value, correct_value);
	fail_unless(ret == 0, buf);
}

static void fail_ne(int a, int b)
{
	fail_unless(a == b, "%d is not %d", a, b);
}

static void default_test(const char *value, char *correct_value, char *symbolname)
{
	char buf[PATH_MAX];

	snprintf(buf, sizeof(buf), "%s returned null!", symbolname);
	fail_if(value == NULL, buf);

	snprintf(buf, sizeof(buf), "%s was not %s", value, correct_value);
	fail_strne(value, correct_value, true);
}

START_TEST(configurator_default_module_dir)
{
	default_test(buxton_module_dir(), (char*)_MODULE_DIRECTORY, "buxton_module_dir()");
}
END_TEST

START_TEST(configurator_default_conf_file)
{
	default_test(buxton_conf_file(), (char*)_DEFAULT_CONFIGURATION_FILE, "buxton_conf_file()");
}
END_TEST

START_TEST(configurator_default_db_path)
{
	default_test(buxton_db_path(), (char*)_DB_PATH, "buxton_db_path()");
}
END_TEST

START_TEST(configurator_default_smack_load_file)
{
	default_test(buxton_smack_load_file(), (char*)_SMACK_LOAD_FILE, "buxton_smack_load_file()");
}
END_TEST

START_TEST(configurator_default_buxton_socket)
{
	default_test(buxton_socket(), (char*)_BUXTON_SOCKET, "buxton_socket()");
}
END_TEST


START_TEST(configurator_env_conf_file)
{
	putenv("BUXTON_CONF_FILE=/nonexistant/buxton.conf");
	default_test(buxton_conf_file(), "/nonexistant/buxton.conf", "buxton_conf_file()");
}
END_TEST

START_TEST(configurator_env_module_dir)
{
	putenv("BUXTON_MODULE_DIR=/nonexistant/buxton/plugins");
	default_test(buxton_module_dir(), "/nonexistant/buxton/plugins", "buxton_module_dir()");
}
END_TEST

START_TEST(configurator_env_db_path)
{
	putenv("BUXTON_DB_PATH=/nonexistant/buxton.db");
	default_test(buxton_db_path(), "/nonexistant/buxton.db", "buxton_db_path()");
}
END_TEST

START_TEST(configurator_env_smack_load_file)
{
	putenv("BUXTON_SMACK_LOAD_FILE=/nonexistant/smack_load_file");
	default_test(buxton_smack_load_file(), "/nonexistant/smack_load_file", "buxton_smack_load_file()");
}
END_TEST

START_TEST(configurator_env_buxton_socket)
{
	putenv("BUXTON_BUXTON_SOCKET=/nonexistant/buxton_socket");
	default_test(buxton_socket(), "/nonexistant/buxton_socket", "buxton_socket()");
}
END_TEST


START_TEST(configurator_cmd_conf_file)
{
	char *correct = "herpyderpy";

	buxton_add_cmd_line(CONFIG_CONF_FILE, correct);
	putenv("BUXTON_CONF_FILE=/nonexistant/buxton.conf");
	default_test(buxton_conf_file(), correct, "buxton_conf_file()");
}
END_TEST

START_TEST(configurator_cmd_module_dir)
{
	char *correct = "herpyderpy";

	buxton_add_cmd_line(CONFIG_MODULE_DIR, correct);
	putenv("BUXTON_MODULE_DIR=/nonexistant/buxton/plugins");
	default_test(buxton_module_dir(), correct, "buxton_module_dir()");
}
END_TEST

START_TEST(configurator_cmd_db_path)
{
	char *correct = "herpyderpy";

	buxton_add_cmd_line(CONFIG_DB_PATH, correct);
	putenv("BUXTON_DB_PATH=/nonexistant/buxton.db");
	default_test(buxton_db_path(), correct, "buxton_db_path()");
}
END_TEST

START_TEST(configurator_cmd_smack_load_file)
{
	char *correct = "herpyderpy";

	buxton_add_cmd_line(CONFIG_SMACK_LOAD_FILE, correct);
	putenv("BUXTON_SMACK_LOAD_FILE=/nonexistant/smack_load_file");
	default_test(buxton_smack_load_file(), correct, "buxton_smack_load_file()");
}
END_TEST

START_TEST(configurator_cmd_buxton_socket)
{
	char *correct = "herpyderpy";

	buxton_add_cmd_line(CONFIG_BUXTON_SOCKET, correct);
	putenv("BUXTON_BUXTON_SOCKET=/nonexistant/buxton_socket");
	default_test(buxton_socket(), correct, "buxton_socket()");
}
END_TEST


START_TEST(configurator_conf_db_path)
{
	char *correct = "/you/are/so/suck";

	putenv("BUXTON_CONF_FILE=" ABS_TOP_SRCDIR "/test/test-configurator.conf");
	default_test(buxton_db_path(), correct, "buxton_db_path()");
}
END_TEST

START_TEST(configurator_conf_smack_load_file)
{
	char *correct = "/smack/smack/smack";

	putenv("BUXTON_CONF_FILE=" ABS_TOP_SRCDIR "/test/test-configurator.conf");
	default_test(buxton_smack_load_file(), correct, "smack_load_file()");
}
END_TEST

START_TEST(configurator_conf_buxton_socket)
{
	char *correct = "/hurp/durp/durp";

	putenv("BUXTON_CONF_FILE=" ABS_TOP_SRCDIR "/test/test-configurator.conf");
	default_test(buxton_socket(), correct, "buxton_socket()");
}
END_TEST

START_TEST(configurator_conf_module_dir)
{
	char *correct = "/shut/your/mouth";

	putenv("BUXTON_CONF_FILE=" ABS_TOP_SRCDIR "/test/test-configurator.conf");
	default_test(buxton_module_dir(), correct, "buxton_module_dir()");
}
END_TEST

START_TEST(configurator_get_layers)
{
	ConfigLayer *layers = NULL;
	int numlayers;

	putenv("BUXTON_CONF_FILE=" ABS_TOP_SRCDIR "/test/test-configurator.conf");
	numlayers = buxton_key_get_layers(&layers);
	fail_if(layers == NULL, "buxton_key_get_layers returned NULL");
	fail_if(numlayers != 7, "num layers is %d instead of %d", numlayers, 7);

	fail_strne(layers[0].name, "base", false);
	fail_strne(layers[0].type, "System", false);
	fail_strne(layers[0].backend, "gdbm", false);
	fail_strne(layers[0].description, "Operating System configuration layer", false);
	fail_ne(layers[0].priority, 0);

	fail_strne(layers[1].name, "isp", false);
	fail_strne(layers[1].type, "System", false);
	fail_strne(layers[1].backend, "gdbm", false);
	fail_strne(layers[1].description, "ISP specific settings", false);
	fail_ne(layers[1].priority, 1);

	/* ... */

	fail_strne(layers[6].name, "test-gdbm-user", false);
	fail_strne(layers[6].type, "User", false);
	fail_strne(layers[6].backend, "gdbm", false);
	fail_strne(layers[6].description, "GDBM test db for user", false);
	fail_ne(layers[6].priority, 6000);



}
END_TEST

START_TEST(ini_parse_check)
{
	char ini_good[] = "test/test-pass.ini";
	char ini_bad[] = "test/test-fail.ini";
	dictionary *ini = NULL;

	ini = iniparser_load(ini_good);
	fail_if(ini == NULL,
		"Failed to parse ini file");

	iniparser_dump(ini, stdout);
	iniparser_freedict(ini);

	ini = iniparser_load(ini_bad);
	fail_if(ini != NULL,
		"Failed to catch bad ini file");

	iniparser_dump(ini, stdout);
	iniparser_freedict(ini);
}
END_TEST

static Suite *
configurator_suite(void)
{
	Suite *s;
	TCase *tc;

	s = suite_create("configurator");

	tc = tcase_create("compilation defaults");
	tcase_add_test(tc, configurator_default_conf_file);
	tcase_add_test(tc, configurator_default_module_dir);
	tcase_add_test(tc, configurator_default_db_path);
	tcase_add_test(tc, configurator_default_smack_load_file);
	tcase_add_test(tc, configurator_default_buxton_socket);
	suite_add_tcase(s, tc);

	tc = tcase_create("env clobbers defaults");
	tcase_add_test(tc, configurator_env_conf_file);
	tcase_add_test(tc, configurator_env_module_dir);
	tcase_add_test(tc, configurator_env_db_path);
	tcase_add_test(tc, configurator_env_smack_load_file);
	tcase_add_test(tc, configurator_env_buxton_socket);
	suite_add_tcase(s, tc);

	tc = tcase_create("command line clobbers all");
	tcase_add_test(tc, configurator_cmd_conf_file);
	tcase_add_test(tc, configurator_cmd_module_dir);
	tcase_add_test(tc, configurator_cmd_db_path);
	tcase_add_test(tc, configurator_cmd_smack_load_file);
	tcase_add_test(tc, configurator_cmd_buxton_socket);
	suite_add_tcase(s, tc);

	tc = tcase_create("config file works");
	tcase_add_test(tc, configurator_conf_module_dir);
	tcase_add_test(tc, configurator_conf_db_path);
	tcase_add_test(tc, configurator_conf_smack_load_file);
	tcase_add_test(tc, configurator_conf_buxton_socket);
	suite_add_tcase(s, tc);

	tc = tcase_create("config file works");
	tcase_add_test(tc, configurator_get_layers);
	suite_add_tcase(s, tc);

	tc = tcase_create("ini_functions");
	tcase_add_test(tc, ini_parse_check);
	suite_add_tcase(s, tc);

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = configurator_suite();
	sr = srunner_create(s);
	srunner_run_all(sr, CK_VERBOSE);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
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
