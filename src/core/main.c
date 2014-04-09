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
#include <getopt.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <systemd/sd-daemon.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <attr/xattr.h>

#include "buxton.h"
#include "backend.h"
#include "daemon.h"
#include "direct.h"
#include "list.h"
#include "log.h"
#include "smack.h"
#include "util.h"
#include "configurator.h"
#include "buxtonlist.h"

#define SOCKET_TIMEOUT 5

static BuxtonDaemon self;

static void print_usage(char *name)
{
	printf("%s: Usage\n\n", name);

	printf("  -c, --config-file	   Path to configuration file\n");
	printf("  -h, --help		   Display this help message\n");
}

/**
 * Entry point into buxtond
 * @param argc Number of arguments passed
 * @param argv An array of string arguments
 * @returns EXIT_SUCCESS if the operation succeeded, otherwise EXIT_FAILURE
 */
int main(int argc, char *argv[])
{
	int fd;
	int smackfd = -1;
	socklen_t addr_len;
	struct sockaddr_un remote;
	int descriptors;
	int ret;
	bool manual_start = false;
	sigset_t mask;
	int sigfd;
	bool leftover_messages = false;
	struct stat st;
	bool help = false;
	BuxtonList *map_list = NULL;
	Iterator iter;
	char *notify_key;

	static struct option opts[] = {
		{ "config-file", 1, NULL, 'c' },
		{ "help",	 0, NULL, 'h' },
		{ NULL, 0, NULL, 0 }
	};

	while (true) {
		int c;
		int i;
		c = getopt_long(argc, argv, "c:h", opts, &i);

		if (c == -1) {
			break;
		}

		switch (c) {
		case 'c':
			ret = stat(optarg, &st);
			if (ret == -1) {
				buxton_log("Invalid configuration file path\n");
				exit(EXIT_FAILURE);
			} else {
				if (st.st_mode & S_IFDIR) {
					buxton_log("Configuration file given is a directory\n");
					exit(EXIT_FAILURE);
				}
			}

			buxton_add_cmd_line(CONFIG_CONF_FILE, optarg);
			break;
		case 'h':
			help = true;
			break;
		}
	}

	if (help) {
		print_usage(argv[0]);
		exit(EXIT_SUCCESS);
	}

	if (!buxton_cache_smack_rules()) {
		exit(EXIT_FAILURE);
	}
	smackfd = buxton_watch_smack_rules();
	if (smackfd < 0 && errno) {
		exit(EXIT_FAILURE);
	}

	self.nfds_alloc = 0;
	self.accepting_alloc = 0;
	self.nfds = 0;
	self.buxton.client.direct = true;
	self.buxton.client.uid = geteuid();
	if (!buxton_direct_open(&self.buxton)) {
		exit(EXIT_FAILURE);
	}

	sigemptyset(&mask);
	ret = sigaddset(&mask, SIGINT);
	if (ret != 0) {
		exit(EXIT_FAILURE);
	}
	ret = sigaddset(&mask, SIGTERM);
	if (ret != 0) {
		exit(EXIT_FAILURE);
	}
	ret = sigaddset(&mask, SIGPIPE);
	if (ret != 0) {
		exit(EXIT_FAILURE);
	}

	ret = sigprocmask(SIG_BLOCK, &mask, NULL);
	if (ret == -1) {
		exit(EXIT_FAILURE);
	}

	sigfd = signalfd(-1, &mask, 0);
	if (sigfd == -1) {
		exit(EXIT_FAILURE);
	}

	add_pollfd(&self, sigfd, POLLIN, false);

	/* For client notifications */
	self.notify_mapping = hashmap_new(string_hash_func, string_compare_func);
	/* Store a list of connected clients */
	LIST_HEAD_INIT(client_list_item, self.client_list);

	descriptors = sd_listen_fds(0);
	if (descriptors < 0) {
		buxton_log("sd_listen_fds: %m\n");
		exit(EXIT_FAILURE);
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
			exit(EXIT_FAILURE);
		}

		memzero(&sa, sizeof(sa));
		sa.un.sun_family = AF_UNIX;
		strncpy(sa.un.sun_path, buxton_socket(), sizeof(sa.un.sun_path) - 1);
		sa.un.sun_path[sizeof(sa.un.sun_path)-1] = 0;

		ret = unlink(sa.un.sun_path);
		if (ret == -1 && errno != ENOENT) {
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
		add_pollfd(&self, fd, POLLIN | POLLPRI, true);
	} else {
		/* systemd socket activation */
		for (fd = SD_LISTEN_FDS_START + 0; fd < SD_LISTEN_FDS_START + descriptors; fd++) {
			if (sd_is_fifo(fd, NULL)) {
				add_pollfd(&self, fd, POLLIN, false);
				buxton_debug("Added fd %d type FIFO\n", fd);
			} else if (sd_is_socket_unix(fd, SOCK_STREAM, -1, buxton_socket(), 0)) {
				add_pollfd(&self, fd, POLLIN | POLLPRI, true);
				buxton_debug("Added fd %d type UNIX\n", fd);
			} else if (sd_is_socket(fd, AF_UNSPEC, 0, -1)) {
				add_pollfd(&self, fd, POLLIN | POLLPRI, true);
				buxton_debug("Added fd %d type SOCKET\n", fd);
			}
		}
	}

	if (smackfd >= 0) {
		/* add Smack rule fd to pollfds */
		add_pollfd(&self, smackfd, POLLIN | POLLPRI, false);
	}

	buxton_log("%s: Started\n", argv[0]);

	/* Enter loop to accept clients */
	for (;;) {
		ret = poll(self.pollfds, self.nfds, leftover_messages ? 0 : -1);

		if (ret < 0) {
			buxton_log("poll(): %m\n");
			break;
		}
		if (ret == 0) {
			if (!leftover_messages) {
				continue;
			}
		}

		leftover_messages = false;

		/* check sigfd if the daemon was signaled */
		if (self.pollfds[0].revents != 0) {
			ssize_t sinfo;
			struct signalfd_siginfo si;

			sinfo = read(self.pollfds[0].fd, &si, sizeof(struct signalfd_siginfo));
			if (sinfo != sizeof(struct signalfd_siginfo)) {
				exit(EXIT_FAILURE);
			}

			if (si.ssi_signo == SIGINT || si.ssi_signo == SIGTERM) {
				break;
			}
		}

		for (nfds_t i = 1; i < self.nfds; i++) {
			client_list_item *cl = NULL;
			char discard[256];

			if (self.pollfds[i].revents == 0) {
				continue;
			}

			if (self.pollfds[i].fd == -1) {
				/* TODO: Remove client from list  */
				buxton_debug("Removing / Closing client for fd %d\n", self.pollfds[i].fd);
				del_pollfd(&self, i);
				continue;
			}

			if (smackfd >= 0) {
				if (self.pollfds[i].fd == smackfd) {
					if (!buxton_cache_smack_rules()) {
						exit(EXIT_FAILURE);
					}
					buxton_log("Reloaded Smack access rules\n");
					/* discard inotify data itself */
					while (read(smackfd, &discard, 256) == 256);
					continue;
				}
			}

			if (self.accepting[i] == true) {
				struct timeval tv;
				int fd;
				int on = 1;

				addr_len = sizeof(remote);

				if ((fd = accept(self.pollfds[i].fd,
						 (struct sockaddr *)&remote, &addr_len)) == -1) {
					buxton_log("accept(): %m\n");
					break;
				}

				buxton_debug("New client fd %d connected through fd %d\n", fd, self.pollfds[i].fd);

				if (fcntl(fd, F_SETFL, O_NONBLOCK)) {
					close(fd);
					break;
				}

				cl = malloc0(sizeof(client_list_item));
				if (!cl) {
					exit(EXIT_FAILURE);
				}

				LIST_INIT(client_list_item, item, cl);

				cl->fd = fd;
				cl->cred = (struct ucred) {0, 0, 0};
				LIST_PREPEND(client_list_item, item, self.client_list, cl);

				/* poll for data on this new client as well */
				add_pollfd(&self, cl->fd, POLLIN | POLLPRI, false);

				/* Mark our packets as high prio */
				if (setsockopt(cl->fd, SOL_SOCKET, SO_PRIORITY, &on, sizeof(on)) == -1) {
					buxton_log("setsockopt(SO_PRIORITY): %m\n");
				}

				/* Set socket recv timeout */
				tv.tv_sec = SOCKET_TIMEOUT;
				tv.tv_usec = 0;
				if (setsockopt(cl->fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,
					       sizeof(struct timeval)) == -1) {
					buxton_log("setsockopt(SO_RCVTIMEO): %m\n");
				}

				/* check if this is optimal or not */
				break;
			}

			assert(self.accepting[i] == 0);
			if (smackfd >= 0) {
				assert(self.pollfds[i].fd != smackfd);
			}

			/* handle data on any connection */
			/* TODO: Replace with hash table lookup */
			LIST_FOREACH(item, cl, self.client_list)
				if (self.pollfds[i].fd == cl->fd) {
					break;
				}

			assert(cl);
			if (handle_client(&self, cl, i)) {
				leftover_messages = true;
			}
		}
	}

	buxton_log("%s: Closing all connections\n", argv[0]);

	if (manual_start) {
		unlink(buxton_socket());
	}
	for (int i = 0; i < self.nfds; i++) {
		close(self.pollfds[i].fd);
	}
	for (client_list_item *i = self.client_list; i;) {
		client_list_item *j = i->item_next;
		free(i);
		i = j;
	}
	/* Clean up notification lists */
	HASHMAP_FOREACH_KEY(map_list, notify_key, self.notify_mapping, iter) {
		hashmap_remove(self.notify_mapping, notify_key);
		BuxtonList *elem;
		BUXTON_LIST_FOREACH(map_list, elem) {
			BuxtonNotification *notif = (BuxtonNotification*)elem->data;
			if (notif->old_data) {
				free_buxton_data(&(notif->old_data));
			}
		}
		free(notify_key);
		buxton_list_free_all(&map_list);
	}

	hashmap_free(self.notify_mapping);
	buxton_direct_close(&self.buxton);
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
