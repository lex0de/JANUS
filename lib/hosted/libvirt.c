/* SPDX-License-Identifier: ISC */
/* JANUS bounded libvirt adapter; Copyright (c) 2026 Danyal A. Samak. */
#define _GNU_SOURCE
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <libvirt/libvirt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "janus/hosted/hosted.h"

static int
read_path(const char *path, char *buf, size_t cap)
{
	size_t n;
	int fd = open(path, O_RDONLY | O_CLOEXEC);
	if (fd < 0)
		return -1;
	int rc = jh_read_file(fd, buf, cap - 1, &n);
	if (close(fd) < 0)
		rc = -1;
	return rc;
}

static int
daemon_epoch(const struct jh_profile *p, char *epoch, size_t cap)
{
	struct sockaddr_un a;
	struct ucred cred;
	socklen_t len = sizeof(cred);
	char boot[64], path[128], exe[128], statbuf[4096], *tok, *save, *end;
	unsigned int i;
	int k, fd, rc = -1;
	memset(&a, 0, sizeof(a));
	a.sun_family = AF_UNIX;
	int session = !strcmp(p->uri, "qemu:///session");
	k = session ? snprintf(a.sun_path, sizeof(a.sun_path),
	                       "/run/user/%lu/libvirt/libvirt-sock",
	                       (unsigned long)getuid())
	            : snprintf(a.sun_path, sizeof(a.sun_path),
	                       "/run/libvirt/libvirt-sock");
	if (k < 0 || (size_t)k >= sizeof(a.sun_path))
		return -1;
	fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (fd < 0)
		return -1;
	if (connect(fd, (struct sockaddr *)&a, sizeof(a)) < 0 ||
	    getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &len) < 0 ||
	    len != sizeof(cred) || cred.uid != (session ? getuid() : 0))
		goto done;
	k = snprintf(path, sizeof(path), "/proc/%ld/exe", (long)cred.pid);
	if (k < 0 || (size_t)k >= sizeof(path))
		goto done;
	ssize_t n = readlink(path, exe, sizeof(exe) - 1);
	if (n < 0 || (size_t)n >= sizeof(exe) - 1)
		goto done;
	exe[n] = '\0';
	if (strcmp(exe, "/usr/sbin/libvirtd"))
		goto done;
	k = snprintf(path, sizeof(path), "/proc/%ld/stat", (long)cred.pid);
	if (k < 0 || (size_t)k >= sizeof(path) ||
	    read_path(path, statbuf, sizeof(statbuf)) ||
	    read_path("/proc/sys/kernel/random/boot_id", boot, sizeof(boot)))
		goto done;
	boot[strcspn(boot, "\n")] = '\0';
	if (!jh_uuid(boot) || !(end = strrchr(statbuf, ')')) || end[1] != ' ')
		goto done;
	tok = strtok_r(end + 2, " ", &save);
	for (i = 3; i < 22 && tok; i++)
		tok = strtok_r(NULL, " ", &save);
	if (!tok || !*tok || strspn(tok, "0123456789") != strlen(tok))
		goto done;
	k = snprintf(epoch, cap, "%s:%ld:%s", boot, (long)cred.pid, tok);
	if (k >= 0 && (size_t)k < cap)
		rc = 0;
done:
	close(fd);
	return rc;
}

static int
query(const struct jh_profile *p, int start, struct jh_snapshot *s)
{
	virConnectPtr conn = NULL;
	virDomainPtr dom = NULL;
	char uuid[VIR_UUID_STRING_BUFLEN], epoch[128], after[128];
	char *uri = NULL;
	int active, state, reason, rc = -1, n;
	unsigned int id;
	conn = virConnectOpen(p->uri);
	if (!conn)
		goto done;
	uri = virConnectGetURI(conn);
	if (!uri || strcmp(uri, p->uri) ||
	    daemon_epoch(p, epoch, sizeof(epoch)))
		goto done;
	dom = virDomainLookupByUUIDString(conn, p->uuid);
	if (!dom || virDomainGetUUIDString(dom, uuid) < 0 ||
	    strcmp(uuid, p->uuid))
		goto done;
	active = virDomainIsActive(dom);
	if (active < 0)
		goto done;
	if (start) {
		/* Refuse an execution that appeared after the caller's
		 * reconciliation. */
		if (active || virDomainCreate(dom) < 0)
			goto done;
		active = virDomainIsActive(dom);
		if (active != 1)
			goto done;
	}
	if (virDomainGetState(dom, &state, &reason, 0) < 0)
		goto done;
	if (!active) {
		if (state != VIR_DOMAIN_SHUTOFF && state != VIR_DOMAIN_CRASHED)
			goto done;
		s->execution = JH_INACTIVE;
		s->epoch[0] = '\0';
	} else {
		if (state == VIR_DOMAIN_PAUSED ||
		    state == VIR_DOMAIN_PMSUSPENDED)
			s->execution = JH_PAUSED;
		else if (state == VIR_DOMAIN_RUNNING ||
		         state == VIR_DOMAIN_BLOCKED ||
		         state == VIR_DOMAIN_SHUTDOWN)
			s->execution = JH_ACTIVE;
		else
			goto done;
		id = virDomainGetID(dom);
		if (id == (unsigned int)-1)
			goto done;
		n = snprintf(s->epoch, sizeof(s->epoch), "%s:%u", epoch, id);
		if (n < 0 || (size_t)n >= sizeof(s->epoch))
			goto done;
	}
	if (daemon_epoch(p, after, sizeof(after)) || strcmp(epoch, after))
		goto done;
	rc = 0;
done:
	free(uri);
	if (dom && virDomainFree(dom) < 0)
		rc = -1;
	if (conn && virConnectClose(conn) < 0)
		rc = -1;
	return rc;
}

int
jh_backend(void *arg, const struct jh_profile *p, int start,
           struct jh_snapshot *out)
{
	int pipes[2], status, consumed = 0, state;
	pid_t child, parent = getpid();
	char buf[256], epoch[JH_EPOCH];
	size_t n;
	struct jh_host *host = arg;
	if (host->worker > 0) {
		if (jh_alive(NULL, host->worker) == 1)
			return -1;
		host->worker = 0;
	}
	if (pipe2(pipes, O_CLOEXEC) < 0)
		return -1;
	child = fork();
	if (child < 0) {
		close(pipes[0]);
		close(pipes[1]);
		return -1;
	}
	if (!child) {
		struct jh_snapshot snapshot = {0};
		close(pipes[0]);
		if (prctl(PR_SET_PDEATHSIG, SIGKILL) < 0 || getppid() != parent)
			_exit(1);
		/* No inherited listener, lock or client descriptors in the
		 * worker. */
		if ((pipes[1] > 3 &&
		     close_range(3, (unsigned int)pipes[1] - 1, 0) < 0) ||
		    close_range((unsigned int)pipes[1] + 1, ~0u, 0) < 0)
			_exit(1);
		if (query(p, start, &snapshot))
			_exit(1);
		int k =
		    snprintf(buf, sizeof(buf), "%d %s\n", snapshot.execution,
		             snapshot.epoch[0] ? snapshot.epoch : "-");
		if (k < 0 || (size_t)k >= sizeof(buf) ||
		    jh_write(pipes[1], buf, (size_t)k))
			_exit(1);
		_exit(0);
	}
	close(pipes[1]);
	int rc = jh_wait(child, 4000, &status);
	if (rc != 1) {
		if (jh_stop(NULL, child))
			host->worker = child;
		close(pipes[0]);
		return -1;
	}
	rc = jh_read_file(pipes[0], buf, sizeof(buf) - 1, &n);
	close(pipes[0]);
	if (rc || !WIFEXITED(status) || WEXITSTATUS(status) ||
	    sscanf(buf, "%d %159s\n%n", &state, epoch, &consumed) != 2 ||
	    consumed < 0 || (size_t)consumed != n || state < 0 ||
	    state > JH_PAUSED)
		return -1;
	out->execution = (enum jh_execution)state;
	if (!strcmp(epoch, "-"))
		epoch[0] = '\0';
	memcpy(out->epoch, epoch, strlen(epoch) + 1);
	return 0;
}
