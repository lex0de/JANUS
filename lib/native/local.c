/* SPDX-License-Identifier: ISC */
/* Copyright (c) 2026 Danyal A. Samak. */
#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "janus/native/native.h"
int
jn_runtime(const char *path)
{
	struct stat s;
	int fd = open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
	if (fd < 0)
		return -1;
	if (fstat(fd, &s) < 0 || s.st_uid != getuid() ||
	    (s.st_mode & 0777) != 0700) {
		close(fd);
		return -1;
	}
	return fd;
}
int
jn_secret_write(int dir, unsigned char *secret)
{
	if (jn_random(secret, JN_SECRET))
		return -1;
	int fd =
	    openat(dir, "owner.new",
	           O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
	if (fd < 0)
		return -1;
	size_t n = 0;
	int rc = 0;
	while (n < JN_SECRET) {
		ssize_t k = write(fd, secret + n, JN_SECRET - n);
		if (k < 0 && errno == EINTR)
			continue;
		if (k <= 0) {
			rc = -1;
			break;
		}
		n += (size_t)k;
	}
	if (fsync(fd) < 0)
		rc = -1;
	if (close(fd) < 0)
		rc = -1;
	if (!rc && renameat(dir, "owner.new", dir, "owner.key") < 0)
		rc = -1;
	if (!rc && fsync(dir) < 0)
		rc = -1;
	if (rc)
		(void)unlinkat(dir, "owner.new", 0);
	return rc;
}
int
jn_owner_call(const char *root, const struct jn_msg *q, struct jn_msg *r,
              int *passed)
{
	int fd = -1, key = -1, dir = -1, rc = JN_IO;
	struct stat st;
	unsigned char packet[JN_SECRET + JN_PACKET];
	size_t n = 0;
	struct sockaddr_un a = {0};
	struct ucred cred;
	socklen_t len = sizeof(cred);
	if (jn_protect() || !jn_request_valid(q, 1) ||
	    jn_encode(q, packet + JN_SECRET, &n))
		goto done;
	dir = jn_runtime(root);
	if (dir < 0)
		goto done;
	key = openat(dir, "owner.key",
	             O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
	if (key < 0 || fstat(key, &st) < 0 || !S_ISREG(st.st_mode) ||
	    st.st_uid != getuid() || (st.st_mode & 0777) != 0600 ||
	    st.st_nlink != 1 || st.st_size != JN_SECRET)
		goto done;
	size_t at = 0;
	while (at < JN_SECRET) {
		ssize_t k = read(key, packet + at, JN_SECRET - at);
		if (k < 0 && errno == EINTR)
			continue;
		if (k <= 0)
			goto done;
		at += (size_t)k;
	}
	a.sun_family = AF_UNIX;
	int k = snprintf(a.sun_path, sizeof(a.sun_path), "%s/owner.sock", root);
	if (k < 0 || (size_t)k >= sizeof(a.sun_path))
		goto done;
	fd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
	if (fd < 0 || connect(fd, (struct sockaddr *)&a, sizeof(a)) < 0 ||
	    getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &len) < 0 ||
	    len != sizeof(cred) || cred.uid != getuid())
		goto done;
	ssize_t sent;
	do {
		sent = send(fd, packet, n + JN_SECRET, MSG_NOSIGNAL);
	} while (sent < 0 && errno == EINTR);
	if (sent != (ssize_t)(n + JN_SECRET))
		goto done;
	struct pollfd p = {fd, POLLIN, 0};
	do {
		k = poll(&p, 1, 3000);
	} while (k < 0 && errno == EINTR);
	if (k <= 0 || jn_receive(fd, r, passed)) {
		rc = JN_UNCERTAIN;
		goto done;
	}
	rc = r->status;
done:
	explicit_bzero(packet, sizeof(packet));
	if (key >= 0)
		close(key);
	if (fd >= 0)
		close(fd);
	if (dir >= 0)
		close(dir);
	return rc;
}
