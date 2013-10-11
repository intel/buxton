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

#include "config.h"
#include "../shared/util.h"
#include "../shared/log.h"
#include "../include/bt-daemon.h"
#include "../include/bt-daemon-private.h"

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

	nr = recvmsg(fd, &msgh, 0);
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
	int client;
	socklen_t addr_len;
	struct sockaddr_un remote;
	int descriptors;
	int accepting[32];
	struct pollfd pollfds[32];
	nfds_t nfds = 0;
	int ret;

	int credentials = 1;
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
		descriptors = 1;
		pollfds[0].events = POLLIN | POLLPRI;
		pollfds[0].fd = fd;
		accepting[0] = 1;
		nfds = 1;
	} else {
		/* systemd socket activation */

		for (fd = SD_LISTEN_FDS_START + 0; fd < SD_LISTEN_FDS_START + descriptors; fd++) {
			pollfds[nfds].fd = fd;
			if (sd_is_fifo(fd, NULL)) {
				pollfds[nfds].events = POLLIN;
				buxton_log("Activated via FIFO\n");
				accepting[nfds] = 0;
			} else if(sd_is_socket_unix(fd, SOCK_STREAM, -1, BUXTON_SOCKET, 0)) {
				pollfds[nfds].events = POLLIN | POLLPRI;
				buxton_log("Activated via UNIX Socket\n");
				accepting[nfds] = 1;
			} else if (sd_is_socket(fd, AF_UNSPEC, 0, -1)) {
				pollfds[nfds].events = POLLIN | POLLPRI;
				buxton_log("Activated via socket\n");
				accepting[nfds] = 1;
			}
			nfds++;
		}
	}

	/* add Smack rule fd to pollfds */
	pollfds[nfds].events = POLLIN | POLLPRI;
	pollfds[nfds].fd = smackfd;
	accepting[nfds] = 0;
	nfds++;

	buxton_log("%s: Started\n", argv[0]);

	/* Enter loop to accept clients */
	for (;;) {
		ret = poll(pollfds, nfds, TIMEOUT);
		if (ret < 0) {
			buxton_log("poll(): %m\n");
			break;
		}
		if (ret == 0) {
			buxton_log("poll(): timed out, trying again\n");
			continue;
		}

		for (nfds_t i=0; i<nfds; i++) {
			if (pollfds[i].fd == -1) {
				/* TODO: Remove client from list  */
				continue;
			}
			if (pollfds[i].revents == 0) {
				continue;
			}
			if (pollfds[i].fd == smackfd) {
				char discard[256];
				if (!buxton_cache_smack_rules())
					exit(1);
				/* discard inotify data itself */
				while (read(smackfd, &discard, 256) == 256);
				continue;
			}
			if (accepting[i] == 1) {
				addr_len = sizeof(remote);
				struct ucred cr;
				if ((client = accept(pollfds[i].fd, (struct sockaddr *)&remote, &addr_len)) == -1)
				{
					buxton_log("accept(): %m\n");
				}

				/* Ensure credentials are passed back from clients */
				setsockopt(client, SOL_SOCKET, SO_PASSCRED, &credentials, sizeof(credentials));
				if (!identify_socket(client, &cr)) {
					buxton_log("Untrusted socket\n");
					close(client);
					continue;
				}

				client_list_item *new_client = malloc0(sizeof(client_list_item));
				if (!new_client)
					return EXIT_FAILURE;

				LIST_INIT(client_list_item, item, new_client);

				new_client->client_socket = client;
				new_client->credentials = cr;
				LIST_PREPEND(client_list_item, item, client_list, new_client);

				buxton_log("New connection from UID %ld, PID %ld\n", cr.uid, cr.pid);

				/* poll for data on this new client as well */
				pollfds[nfds].fd = client;
				pollfds[nfds].events = POLLIN | POLLPRI;
				accepting[nfds] = 0;
				nfds++;

				/* check if this is optimal or not */
				continue;
			}

			/* handle data on any connection */
			buxton_log("Data on fd %d\n", i);
		}
	}

	buxton_log("%s: Closing all connections");

	if (manual_start)
		unlink(BUXTON_SOCKET);
	for (int i=0; i<descriptors; i++) {
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
