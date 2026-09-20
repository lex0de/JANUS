/* SPDX-License-Identifier: ISC */
/* JANUS hosted protocol; Copyright (c) 2026 Danyal A. Samak. */
#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>
#include "janus/hosted/hosted.h"

int
jh_id(const char *s)
{
	size_t n = strlen(s), i;
	if (!n || n > 32 || s[0] < 'a' || s[0] > 'z')
		return 0;
	for (i = 1; i < n; i++) {
		if (!((s[i] >= 'a' && s[i] <= 'z') ||
		      (s[i] >= '0' && s[i] <= '9') || s[i] == '-' ||
		      s[i] == '_'))
			return 0;
	}
	return 1;
}

int
jh_uuid(const char *s)
{
	size_t i;
	if (strlen(s) != 36)
		return 0;
	for (i = 0; i < 36; i++) {
		if (i == 8 || i == 13 || i == 18 || i == 23) {
			if (s[i] != '-')
				return 0;
		} else if (!((s[i] >= '0' && s[i] <= '9') ||
		             (s[i] >= 'a' && s[i] <= 'f')))
			return 0;
	}
	return strcmp(s, "00000000-0000-0000-0000-000000000000") != 0;
}

int
jh_parse(const char *data, size_t n, struct jh_request *out)
{
	static const char *names[] = {"select", "status", "run", "leave",
	                              "recover"};
	char buf[JH_REQUEST + 1], *space;
	size_t i;
	struct jh_request r;
	if (!data || !out || n < 4 || n > JH_REQUEST || data[n - 1] != '\n' ||
	    memchr(data, '\0', n))
		return -1;
	memcpy(buf, data, n);
	buf[n - 1] = '\0';
	space = strchr(buf, ' ');
	if (!space)
		return -1;
	*space++ = '\0';
	if (!jh_id(space))
		return -1;
	for (i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
		if (!strcmp(buf, names[i])) {
			memset(&r, 0, sizeof(r));
			r.command = (enum jh_command)i;
			memcpy(r.id, space, strlen(space) + 1);
			*out = r;
			return 0;
		}
	}
	return -1;
}

int
jh_peer(int fd, uid_t owner)
{
	struct ucred cred;
	socklen_t n = sizeof(cred);
	return getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &n) == 0 &&
	               n == sizeof(cred) && cred.uid == owner
	           ? 0
	           : -1;
}

int
jh_address(const char *dir, void *out, size_t *len)
{
	struct sockaddr_un a;
	int n;
	memset(&a, 0, sizeof(a));
	a.sun_family = AF_UNIX;
	n = snprintf(a.sun_path, sizeof(a.sun_path), "%s/control", dir);
	if (n < 0 || (size_t)n >= sizeof(a.sun_path))
		return -1;
	memcpy(out, &a, sizeof(a));
	*len = sizeof(a);
	return 0;
}
