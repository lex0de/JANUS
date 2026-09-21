/* SPDX-License-Identifier: ISC */
/* Copyright (c) 2026 Danyal A. Samak. */
#define _GNU_SOURCE
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "janus/native/native.h"
static int
call(struct jn_msg *q, struct jn_msg *r)
{
	memset(r, 0, sizeof(*r));
	int rc = jn_call(3, q, r);
	jn_print(r, rc);
	return rc;
}
static int
acquire(const char *id, struct jn_msg *r)
{
	struct jn_msg q = {.op = JN_ACQUIRE};
	if (jn_unhex(id, q.id))
		return JN_INVALID;
	return call(&q, r);
}
static int
read_object(const char *id)
{
	struct jn_msg r = {0}, q = {.op = JN_GET};
	int rc = acquire(id, &r);
	if (rc)
		return rc;
	memcpy(q.handle, r.handle, 16);
	return call(&q, &r);
}
static void
bypass(const char *label, const char *path)
{
	errno = 0;
	int fd = open(path, O_RDONLY | O_CLOEXEC);
	int error = errno;
	printf("probe=%s result=%d errno=%d\n", label, fd, error);
	if (fd >= 0)
		close(fd);
}
int
main(int argc, char **argv)
{
	struct jn_msg q = {.op = JN_SESSION}, r = {0};
	if (call(&q, &r))
		return 1;
	if (argc == 3 && !strcmp(argv[1], "read"))
		return read_object(argv[2]) ? 1 : 0;
	if (argc == 5 && !strcmp(argv[1], "write")) {
		if (acquire(argv[2], &r))
			return 1;
		memset(&q, 0, sizeof(q));
		q.op = JN_PUT;
		memcpy(q.handle, r.handle, 16);
		if (jn_number(argv[3], &q.revision) ||
		    strlen(argv[4]) > JN_CONTENT)
			return 2;
		q.length = strlen(argv[4]);
		memcpy(q.data, argv[4], q.length);
		return call(&q, &r) ? 1 : 0;
	}
	if (argc == 4 && !strcmp(argv[1], "stale")) {
		memset(&q, 0, sizeof(q));
		q.op = JN_GET;
		if (jn_unhex(argv[3], q.handle))
			return 2;
		int old = call(&q, &r);
		int fresh = read_object(argv[2]);
		return old == JN_STALE && fresh == JN_OK ? 0 : 1;
	}
	if (argc == 3 && !strcmp(argv[1], "conflict")) {
		if (acquire(argv[2], &r))
			return 1;
		memset(&q, 0, sizeof(q));
		q.op = JN_GET;
		memcpy(q.handle, r.handle, 16);
		if (call(&q, &r))
			return 1;
		q.op = JN_PUT;
		q.revision = r.revision;
		q.length = 5;
		memcpy(q.data, "first", 5);
		if (call(&q, &r))
			return 1;
		memcpy(q.data, "other", 5);
		return call(&q, &r) == JN_CONFLICT ? 0 : 1;
	}
	if (argc == 3 && !strcmp(argv[1], "exhaust")) {
		for (size_t i = 0; i < JN_HANDLES; i++)
			if (acquire(argv[2], &r))
				return 1;
		return acquire(argv[2], &r) == JN_LIMIT ? 0 : 1;
	}
	if (argc == 9 && !strcmp(argv[1], "probe")) {
		int granted = read_object(argv[2]);
		int denied = read_object(argv[3]);
		bypass("store", argv[4]);
		bypass("owner-key", argv[5]);
		bypass("repository", argv[8]);
		uint64_t pid;
		if (jn_number(argv[6], &pid) || pid > INT_MAX)
			return 2;
		char path[128];
		int n = snprintf(path, sizeof(path), "/proc/%llu/mem",
		                 (unsigned long long)pid);
		if (n < 0 || (size_t)n >= sizeof(path))
			return 2;
		bypass("service-memory", path);
		errno = 0;
		long traced = ptrace(PTRACE_ATTACH, (pid_t)pid, NULL, NULL);
		printf("probe=ptrace result=%ld errno=%d\n", traced, errno);
		struct rlimit limit;
		errno = 0;
		int limited = prlimit((pid_t)pid, RLIMIT_NOFILE, NULL, &limit);
		printf("probe=service-prlimit result=%d errno=%d\n", limited,
		       errno);
		char byte;
		errno = 0;
		ssize_t console = read(STDOUT_FILENO, &byte, 1);
		printf("probe=stdout-read result=%zd errno=%d\n", console,
		       errno);
		struct iovec local = {&byte, 1}, remote = {(void *)1, 1};
		errno = 0;
		ssize_t copied =
		    process_vm_readv((pid_t)pid, &local, 1, &remote, 1, 0);
		printf("probe=process-vm result=%zd errno=%d\n", copied, errno);
		errno = 0;
		int fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
		printf("probe=socket result=%d errno=%d\n", fd, errno);
		if (fd >= 0)
			close(fd);
		struct sockaddr_un addr = {0};
		addr.sun_family = AF_UNIX;
		n = snprintf(addr.sun_path, sizeof(addr.sun_path), "%s",
		             argv[7]);
		if (n < 0 || (size_t)n >= sizeof(addr.sun_path))
			return 2;
		errno = 0;
		int connected =
		    connect(3, (struct sockaddr *)&addr, sizeof(addr));
		printf("probe=owner-connect result=%d errno=%d\n", connected,
		       errno);
		for (fd = 4; fd < 16; fd++) {
			errno = 0;
			ssize_t k = read(fd, &byte, 1);
			printf("probe=fd-%d result=%zd errno=%d\n", fd, k,
			       errno);
		}
		memset(&q, 0, sizeof(q));
		q.op = JN_CREATE;
		q.length = 4;
		memcpy(q.data, "evil", 4);
		int owner = call(&q, &r);
		return granted == JN_OK && denied == JN_DENIED &&
		               owner == JN_INVALID
		           ? 0
		           : 1;
	}
	fprintf(stderr, "unsupported note operation\n");
	return 2;
}
