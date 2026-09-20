/* SPDX-License-Identifier: ISC */
/* JANUS local client; Copyright (c) 2026 Danyal A. Samak. */
#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "janus/hosted/hosted.h"

int
main(int argc, char **argv)
{
	struct sockaddr_un addr;
	struct jh_request request;
	char buf[JH_REPLY];
	size_t len;
	int fd, n, rc = 1, dir;
	if (argc != 4) {
		fprintf(stderr,
		        "usage: janusctl runtime-dir operation profile\n");
		return 2;
	}
	n = snprintf(buf, sizeof(buf), "%s %s\n", argv[2], argv[3]);
	if (n < 0 || (size_t)n >= sizeof(buf) ||
	    jh_parse(buf, (size_t)n, &request) ||
	    jh_address(argv[1], &addr, &len))
		return 2;
	dir = jh_directory(argv[1]);
	if (dir < 0)
		return 2;
	close(dir);
	fd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
	if (fd < 0)
		return 1;
	if (connect(fd, (struct sockaddr *)&addr, (socklen_t)len) < 0 ||
	    jh_peer(fd, getuid()))
		goto done;
	ssize_t sent;
	do {
		sent = send(fd, buf, (size_t)n, MSG_NOSIGNAL);
	} while (sent < 0 && errno == EINTR);
	if (sent != n)
		goto done;
	struct pollfd p = {fd, POLLIN, 0};
	int ready;
	do {
		ready = poll(&p, 1, 15000);
	} while (ready < 0 && errno == EINTR);
	if (ready <= 0)
		goto done;
	ssize_t got;
	do {
		got = recv(fd, buf, sizeof(buf), MSG_TRUNC);
	} while (got < 0 && errno == EINTR);
	if (got <= 0 || (size_t)got >= sizeof(buf))
		goto done;
	if (jh_write(STDOUT_FILENO, buf, (size_t)got))
		goto done;
	rc = got >= 3 && !memcmp(buf, "ok ", 3) ? 0 : 1;
done:
	close(fd);
	return rc;
}
