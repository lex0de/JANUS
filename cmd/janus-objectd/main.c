/* SPDX-License-Identifier: ISC */
/* Copyright (c) 2026 Danyal A. Samak. */
#define _GNU_SOURCE
#include <sys/file.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "janus/native/native.h"
struct live {
	int fd;
	struct jn_session session;
};
static void
forget(struct live *w)
{
	if (w->fd >= 0)
		close(w->fd);
	memset(w, 0, sizeof(*w));
	w->fd = -1;
}
static int
owned_file(int fd)
{
	struct stat st;
	return fstat(fd, &st) == 0 && S_ISREG(st.st_mode) &&
	               st.st_uid == getuid() && (st.st_mode & 0777) == 0600 &&
	               st.st_nlink == 1
	           ? 0
	           : -1;
}
static void
owner(int fd, const unsigned char *key, struct jn_store *store,
      struct live *worlds)
{
	unsigned char buf[JN_SECRET + JN_PACKET + 1];
	struct jn_msg q = {0}, r = {0};
	struct ucred cred;
	socklen_t len = sizeof(cred);
	struct pollfd p = {fd, POLLIN, 0};
	int pair[2] = {-1, -1}, pass = -1;
	ssize_t n;
	int ready;
	if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &len) < 0 ||
	    len != sizeof(cred) || cred.uid != getuid())
		return;
	do {
		ready = poll(&p, 1, 1000);
	} while (ready < 0 && errno == EINTR);
	if (ready <= 0)
		return;
	do {
		n = recv(fd, buf, sizeof(buf), MSG_TRUNC | MSG_DONTWAIT);
	} while (n < 0 && errno == EINTR);
	r.status = JN_DENIED;
	if (n < JN_SECRET || n > (ssize_t)(JN_SECRET + JN_PACKET))
		goto reply;
	unsigned int different = 0;
	for (size_t i = 0; i < JN_SECRET; i++)
		different |= (unsigned int)(key[i] ^ buf[i]);
	if (different)
		goto reply;
	r.status = JN_INVALID;
	if (jn_decode(buf + JN_SECRET, (size_t)n - JN_SECRET, &q) ||
	    !jn_request_valid(&q, 1))
		goto reply;
	r.op = q.op;
	if (q.op == JN_STOP) {
		forget(&worlds[q.arg - 1]);
		r.status = JN_OK;
	} else if (q.op == JN_LAUNCH) {
		struct live *w = &worlds[q.arg - 1];
		if (w->fd >= 0) {
			r.status = JN_BUSY;
			goto reply;
		}
		if (socketpair(AF_UNIX,
		               SOCK_SEQPACKET | SOCK_CLOEXEC | SOCK_NONBLOCK, 0,
		               pair) < 0) {
			r.status = JN_IO;
			goto reply;
		}
		int size = 4096;
		if (setsockopt(pair[0], SOL_SOCKET, SO_SNDBUF, &size,
		               sizeof(size)) < 0 ||
		    setsockopt(pair[1], SOL_SOCKET, SO_SNDBUF, &size,
		               sizeof(size)) < 0 ||
		    setsockopt(pair[0], SOL_SOCKET, SO_RCVBUF, &size,
		               sizeof(size)) < 0 ||
		    setsockopt(pair[1], SOL_SOCKET, SO_RCVBUF, &size,
		               sizeof(size)) < 0) {
			r.status = JN_IO;
			goto reply;
		}
		int rc = jn_store_launch(store, &q, &r, &w->session);
		r.status = (uint8_t)rc;
		if (!rc) {
			w->fd = pair[0];
			pair[0] = -1;
			pass = pair[1];
		}
	} else
		r.status = (uint8_t)jn_store_owner(store, &q, &r);
reply:
	if (jn_send(fd, &r, pass) && pass >= 0)
		forget(&worlds[q.arg - 1]);
	if (pair[0] >= 0)
		close(pair[0]);
	if (pair[1] >= 0)
		close(pair[1]);
	explicit_bzero(buf, sizeof(buf));
}
int
main(int argc, char **argv)
{
	struct jn_store store = {0};
	struct live worlds[JN_WORLDS];
	struct sockaddr_un addr = {0};
	unsigned char key[JN_SECRET] = {0};
	int dir = -1, lock = -1, db = -1, listener = -1, signals = -1, rc = 1,
	    bound = 0;
	char path[512];
	struct stat st;
	sigset_t mask;
	for (size_t i = 0; i < JN_WORLDS; i++)
		worlds[i].fd = -1;
	if (argc != 2 || !getuid() || geteuid() != getuid() ||
	    argv[1][0] != '/' || jn_protect()) {
		fprintf(stderr, "usage (ordinary user): janus-objectd "
		                "absolute-runtime-directory\n");
		return 2;
	}
	umask(077);
	dir = jn_runtime(argv[1]);
	if (dir < 0)
		goto done;
	lock = openat(dir, "object.lock",
	              O_RDWR | O_CREAT | O_NOFOLLOW | O_CLOEXEC, 0600);
	if (lock < 0 || owned_file(lock) || flock(lock, LOCK_EX | LOCK_NB) < 0)
		goto done;
	db = openat(dir, "store.db",
	            O_RDWR | O_CREAT | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK,
	            0600);
	if (db < 0 || owned_file(db))
		goto done;
	int n = snprintf(path, sizeof(path), "%s/store.db", argv[1]);
	if (n < 0 || (size_t)n >= sizeof(path) || jn_store_open(&store, path) ||
	    jn_secret_write(dir, key))
		goto done;
	if (close(db) < 0) {
		db = -1;
		goto done;
	}
	db = -1;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);
	if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0)
		goto done;
	signals = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
	if (signals < 0)
		goto done;
	addr.sun_family = AF_UNIX;
	n = snprintf(addr.sun_path, sizeof(addr.sun_path), "%s/owner.sock",
	             argv[1]);
	if (n < 0 || (size_t)n >= sizeof(addr.sun_path))
		goto done;
	if (fstatat(dir, "owner.sock", &st, AT_SYMLINK_NOFOLLOW) == 0) {
		if (!S_ISSOCK(st.st_mode) || st.st_uid != getuid() ||
		    unlinkat(dir, "owner.sock", 0) < 0)
			goto done;
	} else if (errno != ENOENT)
		goto done;
	listener =
	    socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
	if (listener < 0 ||
	    bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		goto done;
	bound = 1;
	if (fchmodat(dir, "owner.sock", 0600, 0) < 0 || listen(listener, 4) < 0)
		goto done;
	for (;;) {
		struct pollfd polls[JN_WORLDS + 2];
		polls[0] = (struct pollfd){signals, POLLIN, 0};
		polls[1] = (struct pollfd){listener, POLLIN, 0};
		for (size_t i = 0; i < JN_WORLDS; i++)
			polls[i + 2] = (struct pollfd){worlds[i].fd, POLLIN, 0};
		int ready = poll(polls, JN_WORLDS + 2, -1);
		if (ready < 0) {
			if (errno == EINTR)
				continue;
			goto done;
		}
		if (polls[0].revents & POLLIN) {
			rc = 0;
			break;
		}
		for (size_t i = 0; i < JN_WORLDS; i++) {
			if (polls[i + 2].revents &
			    (POLLHUP | POLLERR | POLLNVAL)) {
				forget(&worlds[i]);
				polls[i + 2].revents = 0;
			}
		}
		/* Independent owner slot has priority over bounded one-packet
		 * world work. */
		if (polls[1].revents & POLLIN) {
			int fd = accept4(listener, NULL, NULL,
			                 SOCK_CLOEXEC | SOCK_NONBLOCK);
			if (fd >= 0) {
				owner(fd, key, &store, worlds);
				close(fd);
			} else if (errno != EAGAIN && errno != EINTR)
				goto done;
		}
		for (size_t i = 0; i < JN_WORLDS; i++) {
			if (worlds[i].fd < 0 || worlds[i].fd != polls[i + 2].fd)
				continue;
			if (polls[i + 2].revents & POLLIN) {
				struct jn_msg q = {0}, r = {0};
				if (jn_receive(worlds[i].fd, &q, NULL)) {
					forget(&worlds[i]);
					continue;
				}
				r.status = (uint8_t)jn_world_request(
				    &store, &worlds[i].session, &q, &r);
				if (jn_send(worlds[i].fd, &r, -1))
					forget(&worlds[i]);
			} else if (polls[i + 2].revents &
			           (POLLHUP | POLLERR | POLLNVAL))
				forget(&worlds[i]);
		}
	}
done:
	for (size_t i = 0; i < JN_WORLDS; i++)
		forget(&worlds[i]);
	if (jn_store_close(&store))
		rc = 1;
	if (bound)
		(void)unlinkat(dir, "owner.sock", 0);
	if (listener >= 0)
		close(listener);
	if (signals >= 0)
		close(signals);
	if (db >= 0)
		close(db);
	if (lock >= 0)
		close(lock);
	if (dir >= 0)
		close(dir);
	explicit_bzero(key, sizeof(key));
	if (rc)
		fprintf(stderr, "object service startup/runtime failure\n");
	return rc;
}
