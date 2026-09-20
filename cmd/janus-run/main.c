/* SPDX-License-Identifier: ISC */
/* Copyright (c) 2026 Danyal A. Samak. */
#define _GNU_SOURCE
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "janus/native/native.h"
int
main(int argc, char **argv)
{
	struct jn_msg q = {.op = JN_LAUNCH}, r = {0};
	int endpoint = -1, dir = -1, program = -1, status, rc = 1;
	int output[2] = {-1, -1};
	char report[16384];
	size_t used = 0;
	pid_t child = -1, parent = getpid();
	struct stat st;
	if (argc < 5 || argc > 16 || !getuid() || geteuid() != getuid() ||
	    jn_protect() || jn_number(argv[2], &q.arg) || q.arg < 1 ||
	    q.arg > JN_WORLDS)
		goto usage;
	if (strcmp(argv[3], "-") && jn_unhex(argv[3], q.id))
		goto usage;
	dir = jn_runtime(argv[1]);
	if (dir < 0)
		goto done;
	program = openat(dir, "note", O_PATH | O_CLOEXEC | O_NOFOLLOW);
	if (program < 0 || fstat(program, &st) < 0 || !S_ISREG(st.st_mode) ||
	    st.st_uid != getuid() || (st.st_mode & 07777) != 0500 ||
	    st.st_nlink != 1)
		goto done;
	if (jn_landlock_abi() < 6) {
		fprintf(stderr, "BLOCKED: Landlock ABI >=6 required\n");
		goto done;
	}
	rc = jn_owner_call(argv[1], &q, &r, &endpoint);
	if (rc || endpoint < 0) {
		jn_print(&r, rc ? rc : JN_IO);
		rc = 1;
		goto done;
	}
	rc = 1;
	char selected[33];
	char *args[16] = {"janus-note", NULL};
	char *env[] = {"LC_ALL=C", NULL};
	if (!strcmp(argv[4], "activity")) {
		if (argc != 5 || jn_zero(r.id, 16))
			goto done;
		jn_hex(r.id, selected);
		args[1] = "read";
		args[2] = selected;
	} else
		for (int i = 4; i < argc; i++)
			args[i - 3] = argv[i];
	if (pipe2(output, O_CLOEXEC | O_NONBLOCK) < 0)
		goto done;
	child = fork();
	if (child < 0)
		goto done;
	if (!child) {
		if (prctl(PR_SET_PDEATHSIG, SIGKILL) < 0 || getppid() != parent)
			_exit(120);
		/* Duplicate above all reserved descriptors before assigning
		 * fixed slots. */
		int app = fcntl(program, F_DUPFD_CLOEXEC, 10),
		    ipc = fcntl(endpoint, F_DUPFD_CLOEXEC, 10),
		    print = fcntl(output[1], F_DUPFD_CLOEXEC, 10),
		    null = open("/dev/null", O_RDONLY | O_CLOEXEC);
		int slots[5] = {-1, -1, -1, -1, -1};
		int failure = 121;
		if (app < 0 || ipc < 0 || print < 0 || null < 0)
			goto child_fail;
		slots[0] = dup2(null, STDIN_FILENO);
		slots[1] = dup2(print, STDOUT_FILENO);
		slots[2] = dup2(print, STDERR_FILENO);
		slots[3] = dup3(ipc, 3, 0);
		slots[4] = dup3(app, 4, O_CLOEXEC);
		for (size_t i = 0; i < 5; i++)
			if (slots[i] < 0)
				goto child_fail;
		if (close_range(5, ~0u, 0) < 0)
			goto child_fail;
		failure = 122;
		struct rlimit cpu = {3, 3},
		              memory = {64 * 1024 * 1024, 64 * 1024 * 1024},
		              fds = {16, 16}, core = {0, 0};
		if (setrlimit(RLIMIT_CPU, &cpu) < 0 ||
		    setrlimit(RLIMIT_AS, &memory) < 0 ||
		    setrlimit(RLIMIT_NOFILE, &fds) < 0 ||
		    setrlimit(RLIMIT_CORE, &core) < 0 || chdir("/") < 0 ||
		    jn_sandbox(4))
			goto child_fail;
		execveat(slots[4], "", args, env, AT_EMPTY_PATH);
		fprintf(stderr, "native exec errno=%d\n", errno);
		failure = 123;
	child_fail:
		for (size_t i = 0; i < 5; i++)
			if (slots[i] >= 0)
				close(slots[i]);
		_exit(failure);
	}
	close(endpoint);
	endpoint = -1;
	close(output[1]);
	output[1] = -1;
	rc = 1;
	for (int i = 0; i < 600; i++) {
		ssize_t count =
		    read(output[0], report + used, sizeof(report) - used);
		if (count > 0)
			used += (size_t)count;
		else if (count < 0 && errno != EAGAIN && errno != EINTR)
			break;
		if (used == sizeof(report))
			break;
		pid_t got = waitpid(child, &status, WNOHANG);
		if (got == child) {
			if (!WIFEXITED(status) || WEXITSTATUS(status))
				fprintf(stderr, "native exit=%d signal=%d\n",
				        WIFEXITED(status) ? WEXITSTATUS(status)
				                          : -1,
				        WIFSIGNALED(status) ? WTERMSIG(status)
				                            : 0);
			rc = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
			for (;;) {
				count = read(output[0], report + used,
				             sizeof(report) - used);
				if (count > 0) {
					used += (size_t)count;
					if (used == sizeof(report)) {
						rc = 1;
						break;
					}
				} else if (count < 0 && errno == EINTR)
					continue;
				else
					break;
			}
			goto done;
		}
		if (got < 0 && errno != EINTR)
			goto done;
		struct timespec delay = {0, 10000000};
		(void)nanosleep(&delay, NULL);
	}
	/* Child remains unreaped and owned; never a PID loaded from persistent
	 * state. */
	if (kill(child, SIGKILL) < 0 && errno != ESRCH)
		goto done;
	for (int i = 0; i < 100; i++) {
		pid_t got = waitpid(child, &status, WNOHANG);
		if (got == child || (got < 0 && errno == ECHILD)) {
			child = -1;
			break;
		}
		if (got < 0 && errno != EINTR)
			break;
		struct timespec delay = {0, 10000000};
		(void)nanosleep(&delay, NULL);
	}
	if (child > 0)
		fprintf(stderr, "native termination pending after SIGKILL\n");
done:
	if (output[0] >= 0)
		close(output[0]);
	if (output[1] >= 0)
		close(output[1]);
	for (size_t at = 0; at < used;) {
		ssize_t n = write(STDOUT_FILENO, report + at, used - at);
		if (n < 0 && errno == EINTR)
			continue;
		if (n <= 0) {
			rc = 1;
			break;
		}
		at += (size_t)n;
	}
	if (endpoint >= 0)
		close(endpoint);
	if (program >= 0)
		close(program);
	if (dir >= 0)
		close(dir);
	return rc == 0 ? 0 : 1;
usage:
	fprintf(stderr, "usage: janus-run runtime world activity-id-or-- {read "
	                "id|write id revision text|activity|probe ...|stale id "
	                "handle|conflict id|exhaust id}\n");
	return 2;
}
