/* SPDX-License-Identifier: ISC */
/* JANUS trusted local files; Copyright (c) 2026 Danyal A. Samak. */
#define _GNU_SOURCE
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "janus/hosted/hosted.h"

int
jh_write(int fd, const void *data, size_t n)
{
	const char *p = data;
	while (n) {
		ssize_t k = write(fd, p, n);
		if (k < 0 && errno == EINTR)
			continue;
		if (k <= 0)
			return -1;
		p += (size_t)k;
		n -= (size_t)k;
	}
	return 0;
}

int
jh_read_file(int fd, char *buf, size_t cap, size_t *len)
{
	size_t n = 0;
	while (n < cap) {
		ssize_t k = read(fd, buf + n, cap - n);
		if (k < 0 && errno == EINTR)
			continue;
		if (k < 0)
			return -1;
		if (!k) {
			if (memchr(buf, '\0', n))
				return -1;
			buf[n] = '\0';
			*len = n;
			return 0;
		}
		n += (size_t)k;
	}
	return -1;
}

static int
regular(int fd)
{
	struct stat st;
	return fstat(fd, &st) == 0 && S_ISREG(st.st_mode) &&
	               st.st_uid == getuid() && (st.st_mode & 077) == 0 &&
	               st.st_nlink == 1
	           ? 0
	           : -1;
}

int
jh_directory(const char *path)
{
	struct stat st;
	int fd = open(path, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
	if (fd < 0)
		return -1;
	if (fstat(fd, &st) < 0 || st.st_uid != getuid() ||
	    (st.st_mode & 0777) != 0700) {
		close(fd);
		return -1;
	}
	return fd;
}

int
jh_profiles(const char *path, struct jh_profile *out, size_t *count)
{
	struct jh_profile temp[JH_PROFILES];
	char buf[4096], *line, *next, *uri, *uuid;
	size_t n, used = 0, i;
	int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
	if (fd < 0)
		return -1;
	int rc = regular(fd) || jh_read_file(fd, buf, sizeof(buf) - 1, &n);
	if (close(fd) < 0 || rc || !n || buf[n - 1] != '\n')
		return -1;
	memset(temp, 0, sizeof(temp));
	for (line = buf; *line; line = next) {
		next = strchr(line, '\n');
		if (!next || used == JH_PROFILES)
			return -1;
		*next++ = '\0';
		uri = strchr(line, ' ');
		if (!uri)
			return -1;
		*uri++ = '\0';
		uuid = strchr(uri, ' ');
		if (!uuid)
			return -1;
		*uuid++ = '\0';
		if (!jh_id(line) || !jh_uuid(uuid) ||
		    (strcmp(uri, "qemu:///session") &&
		     strcmp(uri, "qemu:///system")))
			return -1;
		for (i = 0; i < used; i++) {
			if (!strcmp(line, temp[i].id) ||
			    (!strcmp(uri, temp[i].uri) &&
			     !strcmp(uuid, temp[i].uuid)))
				return -1;
		}
		memcpy(temp[used].id, line, strlen(line) + 1);
		memcpy(temp[used].uri, uri, strlen(uri) + 1);
		memcpy(temp[used].uuid, uuid, 37);
		used++;
	}
	memcpy(out, temp, used * sizeof(*out));
	*count = used;
	return 0;
}

static int
filename(char *buf, size_t n, const struct jh_profile *p)
{
	int k = snprintf(buf, n, "%s.state", p->id);
	return k < 0 || (size_t)k >= n ? -1 : 0;
}

int
jh_load(int dir, const struct jh_profile *p, struct jh_record *r)
{
	char name[64], buf[512], id[33], uri[24], uuid[37], epoch[JH_EPOCH];
	char number[32], phase[2], extra;
	struct jh_record temp = {0};
	size_t n;
	int fd, rc;
	char *end;
	uintmax_t value;
	if (filename(name, sizeof(name), p))
		return -1;
	fd = openat(dir, name, O_RDONLY | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK);
	if (fd < 0) {
		if (errno != ENOENT)
			return -1;
		*r = temp;
		return 0;
	}
	rc = regular(fd) || jh_read_file(fd, buf, sizeof(buf) - 1, &n);
	if (close(fd) < 0 || rc || !n || buf[n - 1] != '\n')
		return -1;
	if (sscanf(buf, "J1 %32s %23s %36s %1s %31s %159s %c", id, uri, uuid,
	           phase, number, epoch, &extra) != 6 ||
	    strcmp(id, p->id) || strcmp(uri, p->uri) || strcmp(uuid, p->uuid) ||
	    phase[0] < '1' || phase[0] > '3' ||
	    strspn(number, "0123456789") != strlen(number))
		return -1;
	errno = 0;
	value = strtoumax(number, &end, 10);
	if (errno || *end || value > UINT64_MAX)
		return -1;
	temp.phase = (enum jh_phase)(phase[0] - '0');
	temp.incarnation = (uint64_t)value;
	if ((temp.phase != JH_STOPPED && !temp.incarnation) ||
	    (temp.phase == JH_STARTED && !strcmp(epoch, "-")))
		return -1;
	memcpy(temp.epoch, epoch, strlen(epoch) + 1);
	*r = temp;
	return 0;
}

int
jh_save(void *arg, const struct jh_profile *p, const struct jh_record *r)
{
	struct jh_host *host = arg;
	char name[64], tmp[80], buf[512];
	int fd, k, rc = -1;
	if (filename(name, sizeof(name), p))
		return -1;
	k = snprintf(tmp, sizeof(tmp), "%s.tmp.%ld", p->id, (long)getpid());
	if (k < 0 || (size_t)k >= sizeof(tmp))
		return -1;
	k = snprintf(buf, sizeof(buf), "J1 %s %s %s %d %" PRIu64 " %s\n", p->id,
	             p->uri, p->uuid, r->phase, r->incarnation,
	             r->phase == JH_STARTED ? r->epoch : "-");
	if (k < 0 || (size_t)k >= sizeof(buf))
		return -1;
	fd = openat(host->dirfd, tmp,
	            O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
	if (fd < 0)
		return -1;
	if (jh_write(fd, buf, (size_t)k) == 0 && fsync(fd) == 0)
		rc = 0;
	if (close(fd) < 0)
		rc = -1;
	if (!rc && renameat(host->dirfd, tmp, host->dirfd, name) < 0)
		rc = -1;
	if (!rc && fsync(host->dirfd) < 0)
		rc = -1;
	if (rc)
		(void)unlinkat(host->dirfd, tmp, 0);
	return rc;
}
