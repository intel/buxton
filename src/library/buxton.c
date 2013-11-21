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
 * \file buxton.c Buxton client library
 */


#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include <stdlib.h>

#include "buxton.h"
#include "bt-daemon.h" /* Rename this part of the library */

/* Keep a global client instead of having the using application
 * refer to yet another client struct. Also consider being
 * able to export this client fd */
__attribute__((unused)) static BuxtonClient __client = NULL;


BuxtonValue* buxton_get_value(char *layer,
			      char *group,
			      char *key)
{
	return NULL;
}

bool buxton_set_value(char *layer,
		      char *group,
		      char *key,
		      BuxtonValue *data)
{
	return false;
}

bool buxton_unset_value(char *layer,
		        char *group,
		        char *key)
{
	return false;
}

void buxton_free_value(BuxtonValue *p)
{
	if (!p)
		return;
	free(p->store);
	free(p);
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
