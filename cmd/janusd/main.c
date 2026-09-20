/* SPDX-License-Identifier: ISC */
/* JANUS owner-local broker; Copyright (c) 2026 Danyal A. Samak. */
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
#include <time.h>
#include <unistd.h>
#include "janus/hosted/hosted.h"
#define CLIENTS 8

static long long
now(void)
{
	struct timespec t;
	if (clock_gettime(CLOCK_MONOTONIC, &t) < 0)
		return -1;
	return (long long)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

static void
answer(int fd, struct jh_world *worlds, size_t count,
       const struct jh_adapters *a)
{
	char buf[JH_REPLY];
	struct jh_request r;
	ssize_t n;
	int k = 0;
	const char *error = "invalid\n";
	do {
		n = recv(fd, buf, JH_REQUEST + 1, MSG_TRUNC);
	} while (n < 0 && errno == EINTR);
	if (n > 0 && n <= JH_REQUEST && !jh_parse(buf, (size_t)n, &r)) {
		size_t i, j;
		error = "unknown-profile\n";
		for (i = 0; i < count; i++) {
			if (strcmp(r.id, worlds[i].profile.id))
				continue;
			if (r.command == JH_RUN) {
				for (j = 0; j < count; j++) {
					if (j != i && worlds[j].viewer > 0) {
						error = "presentation-busy\n";
						goto reply;
					}
				}
			}
			const char *result =
			    jh_handle(&worlds[i], r.command, a);
			k = jh_reply(buf, sizeof(buf), &worlds[i], result);
			break;
		}
	}
reply:
	if (k <= 0) {
		k = snprintf(buf, sizeof(buf), "%s", error);
		if (k < 0 || (size_t)k >= sizeof(buf))
			return;
	}
	/* Nonblocking, one bounded packet; a client that will not read is
	 * closed. */
	do {
		n = send(fd, buf, (size_t)k, MSG_NOSIGNAL);
	} while (n < 0 && errno == EINTR);
	if (n != k)
		fprintf(stderr, "client response not delivered\n");
}

int
main(int argc, char **argv)
{
	struct jh_world worlds[JH_PROFILES] = {0};
	struct jh_profile profiles[JH_PROFILES];
	struct jh_host host = {-1, NULL, NULL, 0};
	struct jh_adapters a = {&host,   jh_backend, jh_spawn,
	                        jh_stop, jh_alive,   jh_save};
	struct sockaddr_un addr;
	struct stat st;
	struct pollfd fds[CLIENTS + 2];
	long long deadlines[CLIENTS] = {0};
	size_t count = 0, len, i;
	int lock = -1, listener = -1, signals = -1, display = -1, rc = 1,
	    bound = 0;
	sigset_t mask;
	if (argc != 5 || !getuid() || geteuid() != getuid()) {
		fprintf(stderr, "usage (non-root): janusd profiles runtime-dir "
		                "display-runtime wayland-name\n");
		return 2;
	}
	umask(077);
	if (!jh_id(argv[4]) || jh_profiles(argv[1], profiles, &count) ||
	    jh_address(argv[2], &addr, &len))
		goto done;
	host.dirfd = jh_directory(argv[2]);
	display = jh_directory(argv[3]);
	if (host.dirfd < 0 || display < 0)
		goto done;
	host.display_dir = argv[3];
	host.display_name = argv[4];
	lock = openat(host.dirfd, "broker.lock",
	              O_RDWR | O_CREAT | O_NOFOLLOW | O_CLOEXEC, 0600);
	if (lock < 0 || fstat(lock, &st) < 0 || !S_ISREG(st.st_mode) ||
	    st.st_uid != getuid() || (st.st_mode & 077) || st.st_nlink != 1 ||
	    flock(lock, LOCK_EX | LOCK_NB) < 0)
		goto done;
	for (i = 0; i < count; i++) {
		worlds[i].profile = profiles[i];
		worlds[i].observed.execution = JH_UNKNOWN;
		if (jh_load(host.dirfd, &profiles[i], &worlds[i].record))
			worlds[i].record.phase = JH_BAD;
	}
	sigemptyset(&mask);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGCHLD);
	if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0)
		goto done;
	signals = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
	listener =
	    socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
	if (signals < 0 || listener < 0)
		goto done;
	if (fstatat(host.dirfd, "control", &st, AT_SYMLINK_NOFOLLOW) == 0) {
		if (!S_ISSOCK(st.st_mode) || st.st_uid != getuid() ||
		    unlinkat(host.dirfd, "control", 0) < 0)
			goto done;
	} else if (errno != ENOENT)
		goto done;
	if (bind(listener, (struct sockaddr *)&addr, (socklen_t)len) < 0)
		goto done;
	bound = 1;
	if (fchmodat(host.dirfd, "control", 0600, 0) < 0 ||
	    listen(listener, CLIENTS) < 0)
		goto done;
	for (i = 0; i < CLIENTS + 2; i++)
		fds[i] = (struct pollfd){-1, POLLIN, 0};
	fds[0].fd = listener;
	fds[1].fd = signals;
	for (;;) {
		long long time = now();
		if (time < 0)
			break;
		for (i = 0; i < count; i++)
			(void)jh_refresh_viewer(&worlds[i], &a);
		int ready = poll(fds, CLIENTS + 2, 100);
		if (ready < 0) {
			if (errno == EINTR)
				continue;
			break;
		}
		if (fds[1].revents & POLLIN) {
			struct signalfd_siginfo info;
			ssize_t n = read(signals, &info, sizeof(info));
			if (n != sizeof(info))
				break;
			if (info.ssi_signo == SIGINT ||
			    info.ssi_signo == SIGTERM) {
				rc = 0;
				break;
			}
		}
		if (fds[0].revents & POLLIN) {
			int fd = accept4(listener, NULL, NULL,
			                 SOCK_CLOEXEC | SOCK_NONBLOCK);
			if (fd >= 0) {
				for (i = 2; i < CLIENTS + 2; i++)
					if (fds[i].fd < 0)
						break;
				if (i == CLIENTS + 2 || jh_peer(fd, getuid()))
					close(fd);
				else {
					fds[i].fd = fd;
					deadlines[i - 2] = time + 1000;
				}
			} else if (errno != EAGAIN && errno != EINTR)
				break;
		}
		for (i = 2; i < CLIENTS + 2; i++) {
			if (fds[i].fd < 0)
				continue;
			if (fds[i].revents & POLLIN)
				answer(fds[i].fd, worlds, count, &a);
			else if (!fds[i].revents && time < deadlines[i - 2])
				continue;
			close(fds[i].fd);
			fds[i].fd = -1;
		}
	}
	for (i = 2; i < CLIENTS + 2; i++)
		if (fds[i].fd >= 0)
			close(fds[i].fd);
done:
	if (host.worker > 0)
		(void)jh_stop(NULL, host.worker);
	for (i = 0; i < count; i++)
		if (worlds[i].viewer > 0)
			(void)jh_stop(NULL, worlds[i].viewer);
	if (bound)
		(void)unlinkat(host.dirfd, "control", 0);
	if (listener >= 0)
		close(listener);
	if (signals >= 0)
		close(signals);
	if (lock >= 0)
		close(lock);
	if (display >= 0)
		close(display);
	if (host.dirfd >= 0)
		close(host.dirfd);
	if (rc)
		fprintf(stderr, "broker startup/runtime failure\n");
	return rc;
}
