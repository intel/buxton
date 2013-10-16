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

#include <systemd/sd-daemon.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <attr/xattr.h>

#include "config.h"
#include "../shared/util.h"
#include "../shared/log.h"
#include "../include/bt-daemon.h"
#include "../include/bt-daemon-private.h"

static size_t nfds_alloc = 0;
static size_t accepting_alloc = 0;
static nfds_t nfds = 0;
static bool *accepting;
static struct pollfd *pollfds;

static void add_pollfd(int fd, short events, bool a)
{
	assert(fd >= 0);

	if (!greedy_realloc((void **) &pollfds, &nfds_alloc,
	    (size_t)((nfds + 1) * (sizeof(struct pollfd))))) {
		buxton_log("realloc(): %m\n");
		exit(EXIT_FAILURE);
	}
	if (!greedy_realloc((void **) &accepting, &accepting_alloc,
	    (size_t)((nfds + 1) * (sizeof(accepting))))) {
		buxton_log("realloc(): %m\n");
		exit(EXIT_FAILURE);
	}
	pollfds[nfds].fd = fd;
	pollfds[nfds].events = events;
	pollfds[nfds].revents = 0;
	accepting[nfds] = a;
	nfds++;

	buxton_debug("Added fd %d to our poll list (accepting=%d)\n", fd, a);
}

static void del_pollfd(int i)
{
	assert(i <= nfds);
	assert(i >= 0);

	buxton_debug("Removing fd %d from our list\n", pollfds[i].fd);

	if (i != (nfds - 1)) {
		memmove(&pollfds[i],
			&pollfds[i + 1],
			(nfds - i - 1) * sizeof(struct pollfd));
		memmove(&accepting[i],
			&accepting[i + 1],
			(nfds - i - 1) * sizeof(accepting));
	}
	nfds--;
}

static bool identify_socket(int fd, struct ucred *ucredr)
{
	/* Identity handling */
	int nr, data;
	struct msghdr msgh;
	struct iovec iov;
	__attribute__((unused)) struct ucred *ucredp;
	struct cmsghdr *cmhp;

	union {
		struct cmsghdr cmh;
		char control[CMSG_SPACE(sizeof(struct ucred))];
	} control_un;

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

	nr = recvmsg(fd, &msgh, MSG_PEEK | MSG_DONTWAIT);
	if (nr == -1)
		return false;

	cmhp = CMSG_FIRSTHDR(&msgh);

	if (cmhp == NULL || cmhp->cmsg_len != CMSG_LEN(sizeof(struct ucred)))
		return false;

	if (cmhp->cmsg_level != SOL_SOCKET || cmhp->cmsg_type != SCM_CREDENTIALS)
		return false;

	ucredp = (struct ucred *) CMSG_DATA(cmhp);

        socklen_t len = sizeof(struct ucred);
	if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, ucredr, &len) == -1)
		return false;

	return true;
}

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
		exit(1);
	smackfd = buxton_watch_smack_rules();
	if (smackfd < 0)
		exit(1);

	/* Store a list of connected clients */
	LIST_HEAD(client_list_item, client_list);
	LIST_HEAD_INIT(client_list_item, client_list);

	descriptors = sd_listen_fds(0);
	if (descriptors < 0) {
		buxton_log("sd_listen_fds: %m\n");
		exit(1);
	} else if (descriptors == 0) {
		/* Manual invocation */
		manual_start = true;
		union {
			struct sockaddr sa;
			struct sockaddr_un un;
		} sa;

		fd = socket(AF_UNIX, SOCK_STREAM, 0);
		if (fd < 0) {
			buxton_log("socket(): %m\n");
			exit(1);
		}

		memset(&sa, 0, sizeof(sa));
		sa.un.sun_family = AF_UNIX;
		strncpy(sa.un.sun_path, BUXTON_SOCKET, sizeof(sa.un.sun_path));

		if (bind(fd, &sa.sa, sizeof(sa)) < 0) {
			buxton_log("bind(): %m\n");
			exit(1);
		}

		if (listen(fd, SOMAXCONN) < 0) {
			buxton_log("listen(): %m\n");
			exit(1);
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
		ret = poll(pollfds, nfds, -1);

		if (ret < 0) {
			buxton_log("poll(): %m\n");
			break;
		}
		if (ret == 0)
			continue;

		for (nfds_t i=0; i<nfds; i++) {
			client_list_item *cl = NULL;
			char discard[256];
			ssize_t l;

			if (pollfds[i].revents == 0)
				continue;

			if (pollfds[i].fd == -1) {
				/* TODO: Remove client from list  */
				buxton_debug("Removing / Closing client for fd %d\n", pollfds[i].fd);
				del_pollfd(i);
				continue;
			}

			if (pollfds[i].fd == smackfd) {
				if (!buxton_cache_smack_rules())
					exit(1);
				buxton_log("Reloaded Smack access rules\n");
				/* discard inotify data itself */
				while (read(smackfd, &discard, 256) == 256);
				continue;
			}

			if (accepting[i] == 1) {
				int fd;
				struct ucred cr;

				addr_len = sizeof(remote);

				if ((fd = accept(pollfds[i].fd,
				    (struct sockaddr *)&remote, &addr_len)) == -1) {
					buxton_log("accept(): %m\n");
					break;
				}

				buxton_debug("New client fd %d connected through fd %d\n", fd, pollfds[i].fd);

				cl = malloc0(sizeof(client_list_item));
				if (!cl)
					exit(EXIT_FAILURE);

				LIST_INIT(client_list_item, item, cl);

				cl->fd = fd;
				cl->cred = (struct ucred) {0, 0, 0};
				LIST_PREPEND(client_list_item, item, client_list, cl);

				setsockopt(cl->fd, SOL_SOCKET, SO_PASSCRED, &cr, sizeof(struct ucred));

				/* poll for data on this new client as well */
				add_pollfd(cl->fd, POLLIN | POLLPRI, false);

				/* check if this is optimal or not */
				break;
			}

			assert(accepting[i] == 0);
			assert(pollfds[i].fd != smackfd);

			/* handle data on any connection */
			LIST_FOREACH(item, cl, client_list)
				if (pollfds[i].fd == cl->fd)
					break;

			assert(cl);

			/* client closed the connection, or some error occurred? */
			if (recv(cl->fd, discard, sizeof(discard), MSG_PEEK | MSG_DONTWAIT) <= 0) {
				del_pollfd(i);
				close(cl->fd);
				free(cl->smack_label);
				buxton_debug("Closed connection from fd %d\n", cl->fd);
				LIST_REMOVE(client_list_item, item, client_list, cl);
				continue;
			}

			/* need to authenticate the client? */
			if ((cl->cred.uid == 0) || (cl->cred.pid == 0)) {
				int slabel_len;
				char *slabel = NULL;
				struct ucred cr;

				if (!identify_socket(cl->fd, &cr)) {
					del_pollfd(i);
					close(cl->fd);
					free(cl->smack_label);
					buxton_debug("Closed untrusted connection from fd %d\n", cl->fd);
					LIST_REMOVE(client_list_item, item, client_list, cl);
					continue;
				}
				memcpy(&cl->cred, &cr, sizeof(struct ucred));

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

			/* we don't know what to do with the data yet */
			while ((l = read(pollfds[i].fd, &discard, 256) == 256))
				buxton_debug("Discarded %d bytes on fd %d\n", l, pollfds[i].fd);
		}
	}

	buxton_log("%s: Closing all connections");

	if (manual_start)
		unlink(BUXTON_SOCKET);
	for (int i = 0; i < nfds; i++) {
		close(pollfds[i].fd);
	}
	for (client_list_item *i = client_list; i;) {
		client_list_item *j = i->item_next;
		free(i);
		i = j;
	}
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
