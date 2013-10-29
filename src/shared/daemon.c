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

#include <stdlib.h>

#include "bt-daemon.h"
#include "daemon.h"
#include "log.h"
#include "protocol.h"

/* TODO: Add Smack support */
BuxtonData* set_value(BuxtonDaemon *self, client_list_item *client, BuxtonData *list,
		      int n_params, BuxtonStatus *status)
{
	BuxtonData layer, key, value;
	*status = BUXTON_STATUS_FAILED;

	/* Require layer, key and value */
	if (n_params != 3)
		return NULL;

	layer = list[0];
	key = list[1];
	value = list[2];

	/* We only accept strings for layer and key names */
	if (layer.type != STRING && key.type != STRING)
		return NULL;

	/* Use internal library to set value */
	if (!buxton_client_set_value(&(self->buxton), layer.store.d_string, key.store.d_string, &value)) {
		*status = BUXTON_STATUS_FAILED;
		return NULL;
	}

	*status = BUXTON_STATUS_OK;
	buxton_debug("Setting value of [%s][%s]\n", layer.store.d_string, key.store.d_string);
	return NULL;
}

/* TODO: Add smack support */
BuxtonData* get_value(BuxtonDaemon *self, client_list_item *client, BuxtonData *list,
		      int n_params, BuxtonStatus *status)
{
	BuxtonData layer, key;
	*status = BUXTON_STATUS_FAILED;
	BuxtonData* ret = NULL;

	if (n_params < 1)
		goto end;

	/* Optional layer */
	if (n_params == 2) {
		layer = list[0];
		if (layer.type != STRING)
			goto end;
		key = list[1];
	} else  if (n_params == 1) {
		key = list[0];
	} else {
		goto end;
	}

	/* We only accept strings for layer and key names */
	if (key.type != STRING)
		goto end;

	ret = malloc(sizeof(BuxtonData));
	if (!ret)
		goto end;

	/* Attempt to retrieve key */
	if (n_params == 2) {
		/* Layer + key */
		if (!buxton_client_get_value_for_layer(&(self->buxton), layer.store.d_string, key.store.d_string, ret))
			goto fail;
	} else {
		/* Key only */
		if (!buxton_client_get_value(&(self->buxton), key.store.d_string, ret))
			goto fail;
	}
	*status = BUXTON_STATUS_OK;
	goto end;
fail:
	if (ret)
		free(ret);
	ret = NULL;
end:

	return ret;
}

bool identify_client(client_list_item *cl)
{
	/* Identity handling */
	int nr, data;
	struct msghdr msgh;
	struct iovec iov;
	__attribute__((unused)) struct ucred *ucredp;
	struct cmsghdr *cmhp;
        socklen_t len = sizeof(struct ucred);
	int on = 1;

	union {
		struct cmsghdr cmh;
		char control[CMSG_SPACE(sizeof(struct ucred))];
	} control_un;

	setsockopt(cl->fd, SOL_SOCKET, SO_PASSCRED, &on, sizeof(on));

	control_un.cmh.cmsg_len = CMSG_LEN(sizeof(struct ucred));
	control_un.cmh.cmsg_level = SOL_SOCKET;
	control_un.cmh.cmsg_type = SCM_CREDENTIALS;

	msgh.msg_control = control_un.control;
	msgh.msg_controllen = sizeof(control_un.control);

	msgh.msg_iov = &iov;
	msgh.msg_iovlen = 1;
	iov.iov_base = &data;
	iov.iov_len = sizeof(int);

	msgh.msg_name = NULL;
	msgh.msg_namelen = 0;

	nr = recvmsg(cl->fd, &msgh, MSG_PEEK | MSG_DONTWAIT);
	if (nr == -1)
		return false;

	cmhp = CMSG_FIRSTHDR(&msgh);

	if (cmhp == NULL || cmhp->cmsg_len != CMSG_LEN(sizeof(struct ucred)))
		return false;

	if (cmhp->cmsg_level != SOL_SOCKET || cmhp->cmsg_type != SCM_CREDENTIALS)
		return false;

	ucredp = (struct ucred *) CMSG_DATA(cmhp);

	if (getsockopt(cl->fd, SOL_SOCKET, SO_PEERCRED, &cl->cred, &len) == -1)
		return false;

	return true;
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
