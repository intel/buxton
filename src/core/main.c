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
 * \file core/main.c Buxton daemon
 *
 * This file provides the buxton daemon
 */
#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include <errno.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <systemd/sd-daemon.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <attr/xattr.h>

#include "config.h"
#include "util.h"
#include "log.h"
#include "list.h"
#include "smack.h"
#include "bt-daemon.h"
#include "protocol.h"


static BuxtonDaemon self;

static void add_pollfd(int fd, short events, bool a)
{
	assert(fd >= 0);

	if (!greedy_realloc((void **) &self.pollfds, &self.nfds_alloc,
	    (size_t)((self.nfds + 1) * (sizeof(struct pollfd))))) {
		buxton_log("realloc(): %m\n");
		exit(EXIT_FAILURE);
	}
	if (!greedy_realloc((void **) &self.accepting, &self.accepting_alloc,
	    (size_t)((self.nfds + 1) * (sizeof(self.accepting))))) {
		buxton_log("realloc(): %m\n");
		exit(EXIT_FAILURE);
	}
	self.pollfds[self.nfds].fd = fd;
	self.pollfds[self.nfds].events = events;
	self.pollfds[self.nfds].revents = 0;
	self.accepting[self.nfds] = a;
	self.nfds++;

	buxton_debug("Added fd %d to our poll list (accepting=%d)\n", fd, a);
}

static void del_pollfd(int i)
{
	assert(i <= self.nfds);
	assert(i >= 0);

	buxton_debug("Removing fd %d from our list\n", self.pollfds[i].fd);

	if (i != (self.nfds - 1)) {
		memmove(&self.pollfds[i],
			&self.pollfds[i + 1],
			(self.nfds - i - 1) * sizeof(struct pollfd));
		memmove(&self.accepting[i],
			&self.accepting[i + 1],
			(self.nfds - i - 1) * sizeof(self.accepting));
	}
	self.nfds--;
}

static bool identify_client(client_list_item *cl)
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

/**
 * Handle a client connection
 * @param cl The currently activate client
 * @param i The currently active file descriptor
 */
static void handle_client(client_list_item *cl, int i)
{
	ssize_t l;
	int slabel_len;
	char *slabel = NULL;

	if (!cl->data) {
		cl->data = malloc(BUXTON_CONTROL_LENGTH);
		cl->offset = 0;
		cl->size = BUXTON_CONTROL_LENGTH;
	}
	if (!cl->data)
		return;
	/* client closed the connection, or some error occurred? */
	if (recv(cl->fd, cl->data, cl->size, MSG_PEEK | MSG_DONTWAIT) <= 0) {
		del_pollfd(i);
		close(cl->fd);
		free(cl->smack_label);
		buxton_debug("Closed connection from fd %d\n", cl->fd);
		LIST_REMOVE(client_list_item, item, self.client_list, cl);
		free(cl);
		return;
	}

	/* need to authenticate the client? */
	if ((cl->cred.uid == 0) || (cl->cred.pid == 0)) {
		if (!identify_client(cl)) {
			del_pollfd(i);
			close(cl->fd);
			free(cl->smack_label);
			buxton_debug("Closed untrusted connection from fd %d\n", cl->fd);
			LIST_REMOVE(client_list_item, item, self.client_list, cl);
			return;
		}

		slabel_len = fgetxattr(cl->fd, SMACK_ATTR_NAME, slabel, 0);
		if (slabel_len <= 0) {
			buxton_log("fgetxattr(): no " SMACK_ATTR_NAME " label\n");
			exit(EXIT_FAILURE);
		}

		slabel = malloc0(slabel_len + 1);
		if (!slabel) {
			buxton_log("malloc0(): %m\n");
			exit(EXIT_FAILURE);
		}
		slabel_len = fgetxattr(cl->fd, SMACK_ATTR_NAME, slabel, SMACK_LABEL_LEN);
		if (!slabel_len) {
			buxton_log("fgetxattr(): %m\n");
			exit(EXIT_FAILURE);
		}

		buxton_debug("fgetxattr(): label=\"%s\"\n", slabel);
		cl->smack_label = strdup(slabel);
	}
	buxton_debug("New packet from UID %ld, PID %ld\n", cl->cred.uid, cl->cred.pid);

	/* Hand off any read data */
	/*
	 * TODO: Need to handle partial messages, read total message
	 * size of the data, keep reading until we get that amount.
	 * Probably need a timer to stop waiting and just move to the
	 * next client at some point as well.
	 */
	while ((l = read(self.pollfds[i].fd, &(cl->data) + cl->offset, cl->size - cl->offset)) > 0) {
		cl->offset += l;
		if (cl->offset < BUXTON_CONTROL_LENGTH) {
			continue;
		}
		if (cl->size == BUXTON_CONTROL_LENGTH) {
			cl->size = buxton_get_message_size(cl->data, cl->offset);
			if (cl->size == 0 || cl->size > BUXTON_CONTROL_LENGTH_MAX)
				goto cleanup;
		}
		if (cl->size != BUXTON_CONTROL_LENGTH) {
			cl->data = realloc(cl->data, cl->size);
			if (!cl->data)
				goto cleanup;
		}
		if (cl->size != cl->offset)
			continue;
		bt_daemon_handle_message(&self, cl, l);

		/* reset in case there are more messages */
		cl->data = realloc(cl->data, BUXTON_CONTROL_LENGTH);
		if (!cl->data)
			goto cleanup;
		cl->size = BUXTON_CONTROL_LENGTH;
		cl->offset = 0;
	}

	/* Not done with this message so don't cleanup */
	if (l == 0 && cl->offset < cl->size)
		return;
cleanup:
	if (cl->data)
		free(cl->data);
	cl->data = NULL;
	cl->size = BUXTON_CONTROL_LENGTH;
	cl->offset = 0;
}

static BuxtonData* get_value(client_list_item *client, BuxtonData *list, int n_params, BuxtonStatus *status)
{
	/* TODO: Implement */
	*status = BUXTON_STATUS_FAILED;
	return NULL;
}

static BuxtonData*  set_value(client_list_item *client, BuxtonData *list, int n_params, BuxtonStatus *status)
{
	/* TODO: Implement */
	*status = BUXTON_STATUS_FAILED;
	return NULL;
}

/**
 * Entry point into bt-daemon
 * @param argc Number of arguments passed
 * @param argv An array of string arguments
 * @returns EXIT_SUCCESS if the operation succeeded, otherwise EXIT_FAILURE
 */
int main(int argc, char *argv[])
{
	int fd;
	int smackfd;
	socklen_t addr_len;
	struct sockaddr_un remote;
	int descriptors;
	int ret;

	bool manual_start = false;

	if (!buxton_cache_smack_rules())
		exit(EXIT_FAILURE);
	smackfd = buxton_watch_smack_rules();
	if (smackfd < 0)
		exit(EXIT_FAILURE);

	self.nfds_alloc = 0;
	self.accepting_alloc = 0;
	self.nfds = 0;
	self.buxton.direct = true;
	self.set_value = &set_value;
	self.get_value = &get_value;
	if (!buxton_direct_open(&self.buxton))
		exit(EXIT_FAILURE);

	/* Store a list of connected clients */
	LIST_HEAD_INIT(client_list_item, self.client_list);

	descriptors = sd_listen_fds(0);
	if (descriptors < 0) {
		buxton_log("sd_listen_fds: %m\n");
		exit(EXIT_FAILURE);
	} else if (descriptors == 0) {
		/* Manual invocation */
		int r;
		manual_start = true;
		union {
			struct sockaddr sa;
			struct sockaddr_un un;
		} sa;

		fd = socket(AF_UNIX, SOCK_STREAM, 0);
		if (fd < 0) {
			buxton_log("socket(): %m\n");
			exit(EXIT_FAILURE);
		}

		memset(&sa, 0, sizeof(sa));
		sa.un.sun_family = AF_UNIX;
		strncpy(sa.un.sun_path, BUXTON_SOCKET, sizeof(sa.un.sun_path) - 1);

		r = unlink(sa.un.sun_path);
		if (r == -1 && errno != ENOENT) {
			exit(EXIT_FAILURE);
		}

		if (bind(fd, &sa.sa, sizeof(sa)) < 0) {
			buxton_log("bind(): %m\n");
			exit(EXIT_FAILURE);
		}

		chmod(sa.un.sun_path, 0666);

		if (listen(fd, SOMAXCONN) < 0) {
			buxton_log("listen(): %m\n");
			exit(EXIT_FAILURE);
		}
		add_pollfd(fd, POLLIN | POLLPRI, true);
	} else {
		/* systemd socket activation */
		for (fd = SD_LISTEN_FDS_START + 0; fd < SD_LISTEN_FDS_START + descriptors; fd++) {
			if (sd_is_fifo(fd, NULL)) {
				add_pollfd(fd, POLLIN, false);
				buxton_debug("Added fd %d type FIFO\n", fd);
			} else if (sd_is_socket_unix(fd, SOCK_STREAM, -1, BUXTON_SOCKET, 0)) {
				add_pollfd(fd, POLLIN | POLLPRI, true);
				buxton_debug("Added fd %d type UNIX\n", fd);
			} else if (sd_is_socket(fd, AF_UNSPEC, 0, -1)) {
				add_pollfd(fd, POLLIN | POLLPRI, true);
				buxton_debug("Added fd %d type SOCKET\n", fd);
			}
		}
	}

	/* add Smack rule fd to pollfds */
	add_pollfd(smackfd, POLLIN | POLLPRI, false);

	buxton_log("%s: Started\n", argv[0]);

	/* Enter loop to accept clients */
	for (;;) {
		ret = poll(self.pollfds, self.nfds, -1);

		if (ret < 0) {
			buxton_log("poll(): %m\n");
			break;
		}
		if (ret == 0)
			continue;

		for (nfds_t i=0; i<self.nfds; i++) {
			client_list_item *cl = NULL;
			char discard[256];

			if (self.pollfds[i].revents == 0)
				continue;

			if (self.pollfds[i].fd == -1) {
				/* TODO: Remove client from list  */
				buxton_debug("Removing / Closing client for fd %d\n", self.pollfds[i].fd);
				del_pollfd(i);
				continue;
			}

			if (self.pollfds[i].fd == smackfd) {
				if (!buxton_cache_smack_rules())
					exit(EXIT_FAILURE);
				buxton_log("Reloaded Smack access rules\n");
				/* discard inotify data itself */
				while (read(smackfd, &discard, 256) == 256);
				continue;
			}

			if (self.accepting[i] == true) {
				int fd;
				int on = 1;

				addr_len = sizeof(remote);

				if ((fd = accept(self.pollfds[i].fd,
				    (struct sockaddr *)&remote, &addr_len)) == -1) {
					buxton_log("accept(): %m\n");
					break;
				}

				buxton_debug("New client fd %d connected through fd %d\n", fd, self.pollfds[i].fd);

				cl = malloc0(sizeof(client_list_item));
				if (!cl)
					exit(EXIT_FAILURE);

				LIST_INIT(client_list_item, item, cl);

				cl->fd = fd;
				cl->cred = (struct ucred) {0, 0, 0};
				LIST_PREPEND(client_list_item, item, self.client_list, cl);

				/* poll for data on this new client as well */
				add_pollfd(cl->fd, POLLIN | POLLPRI, false);

				/* Mark our packets as high prio */
				if (setsockopt(cl->fd, SOL_SOCKET, SO_PRIORITY, &on, sizeof(on)) == -1)
					buxton_log("setsockopt(SO_PRIORITY): %m\n");

				/* check if this is optimal or not */
				break;
			}

			assert(self.accepting[i] == 0);
			assert(self.pollfds[i].fd != smackfd);

			/* handle data on any connection */
			/* TODO: Replace with hash table lookup */
			LIST_FOREACH(item, cl, self.client_list)
				if (self.pollfds[i].fd == cl->fd)
					break;

			assert(cl);
			handle_client(cl, i);
		}
	}

	buxton_log("%s: Closing all connections");

	if (manual_start)
		unlink(BUXTON_SOCKET);
	for (int i = 0; i < self.nfds; i++) {
		close(self.pollfds[i].fd);
	}
	for (client_list_item *i = self.client_list; i;) {
		client_list_item *j = i->item_next;
		free(i);
		i = j;
	}
	buxton_client_close(&self.buxton);
	return EXIT_SUCCESS;
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
