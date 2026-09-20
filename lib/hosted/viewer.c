/* SPDX-License-Identifier: ISC */
/* JANUS owned child adapter; Copyright (c) 2026 Danyal A. Samak. */
#define _GNU_SOURCE
#include <sys/prctl.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "janus/hosted/hosted.h"

int
jh_wait(pid_t pid, int milliseconds, int *status)
{
	struct timespec pause = {0, 10000000};
	int elapsed = 0;
	for (;;) {
		pid_t rc = waitpid(pid, status, WNOHANG);
		if (rc == pid)
			return 1;
		if (rc < 0 && errno != EINTR)
			return -1;
		if (elapsed >= milliseconds)
			return 0;
		(void)nanosleep(&pause, NULL);
		elapsed += 10;
	}
}

int
jh_alive(void *arg, pid_t pid)
{
	int status, rc;
	(void)arg;
	if (pid <= 0)
		return -1;
	rc = jh_wait(pid, 0, &status);
	return rc == 0 ? 1 : rc == 1 ? 0 : -1;
}

int
jh_stop(void *arg, pid_t pid)
{
	int rc, status;
	(void)arg;
	if (pid <= 0)
		return -1;
	rc = jh_wait(pid, 0, &status);
	if (rc != 0)
		return rc == 1 ? 0 : -1;
	if (kill(pid, SIGTERM) < 0 && errno != ESRCH)
		return -1;
	rc = jh_wait(pid, 300, &status);
	if (rc != 0)
		return rc == 1 ? 0 : -1;
	if (kill(pid, SIGKILL) < 0 && errno != ESRCH)
		return -1;
	return jh_wait(pid, 1000, &status) == 1 ? 0 : -1;
}

int
jh_exec(const char *path, char *const argv[], char *const env[], pid_t *out)
{
	int pipefd[2], status, rc;
	pid_t parent = getpid(), child;
	char byte;
	if (pipe2(pipefd, O_CLOEXEC) < 0)
		return -1;
	child = fork();
	if (child < 0) {
		close(pipefd[0]);
		close(pipefd[1]);
		return -1;
	}
	if (!child) {
		sigset_t empty;
		close(pipefd[0]);
		if (prctl(PR_SET_PDEATHSIG, SIGKILL) < 0 || getppid() != parent)
			goto fail;
		sigemptyset(&empty);
		if (sigprocmask(SIG_SETMASK, &empty, NULL) < 0)
			goto fail;
		if ((pipefd[1] > 3 &&
		     close_range(3, (unsigned int)pipefd[1] - 1, 0) < 0) ||
		    close_range((unsigned int)pipefd[1] + 1, ~0u, 0) < 0)
			goto fail;
		execve(path, argv, env);
	fail:
		(void)jh_write(pipefd[1], "E", 1);
		_exit(127);
	}
	close(pipefd[1]);
	struct pollfd p = {pipefd[0], POLLIN | POLLHUP, 0};
	do {
		rc = poll(&p, 1, 2000);
	} while (rc < 0 && errno == EINTR);
	ssize_t n = -1;
	if (rc > 0) {
		do {
			n = read(pipefd[0], &byte, 1);
		} while (n < 0 && errno == EINTR);
	}
	close(pipefd[0]);
	if (n != 0) {
		if (jh_stop(NULL, child))
			*out = child;
		return -1;
	}
	rc = jh_wait(child, 0, &status);
	if (rc != 0)
		return -1;
	*out = child;
	return 0;
}

int
jh_spawn(void *arg, const struct jh_profile *p, pid_t *out)
{
	struct jh_host *h = arg;
	char runtime[128], display[512];
	int n =
	    snprintf(runtime, sizeof(runtime), "XDG_RUNTIME_DIR=/run/user/%lu",
	             (unsigned long)getuid());
	int k = snprintf(display, sizeof(display), "WAYLAND_DISPLAY=%s/%s",
	                 h->display_dir, h->display_name);
	if (h->display_dir[0] != '/' || n < 0 || k < 0 ||
	    (size_t)n >= sizeof(runtime) || (size_t)k >= sizeof(display))
		return -1;
	char *argv[] = {"/usr/bin/virt-viewer",
	                "--verbose",
	                "--connect",
	                (char *)p->uri,
	                "--uuid",
	                "--attach",
	                "--kiosk",
	                "--kiosk-quit=on-disconnect",
	                (char *)p->uuid,
	                NULL};
	char *env[] = {"PATH=/usr/bin:/bin", "GDK_BACKEND=wayland", runtime,
	               display, NULL};
	return jh_exec(argv[0], argv, env, out);
}
