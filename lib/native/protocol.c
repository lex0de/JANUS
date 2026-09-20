/* SPDX-License-Identifier: ISC */
/* Copyright (c) 2026 Danyal A. Samak. */
#define _GNU_SOURCE
#include <sys/random.h>
#include <sys/socket.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>
#include "janus/native/native.h"
static uint64_t
get_be(const unsigned char *p, size_t n)
{
	uint64_t v = 0;
	for (size_t i = 0; i < n; i++)
		v = (v << 8) | p[i];
	return v;
}
static void
put_be(unsigned char *p, uint64_t v, size_t n)
{
	for (size_t i = n; i > 0; i--) {
		p[i - 1] = (unsigned char)(v & 255);
		v >>= 8;
	}
}
int
jn_random(void *data, size_t n)
{
	unsigned char *p = data;
	while (n) {
		ssize_t k = getrandom(p, n, 0);
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
jn_zero(const unsigned char *p, size_t n)
{
	unsigned char v = 0;
	for (size_t i = 0; i < n; i++)
		v |= p[i];
	return v == 0;
}
int
jn_unhex(const char *s, unsigned char *out)
{
	unsigned char tmp[JN_ID];
	if (strlen(s) != JN_ID * 2)
		return -1;
	for (size_t i = 0; i < JN_ID * 2; i++) {
		unsigned int v;
		if (s[i] >= '0' && s[i] <= '9')
			v = (unsigned int)(s[i] - '0');
		else if (s[i] >= 'a' && s[i] <= 'f')
			v = (unsigned int)(s[i] - 'a' + 10);
		else
			return -1;
		if (!(i % 2))
			tmp[i / 2] = (unsigned char)(v << 4);
		else
			tmp[i / 2] |= (unsigned char)v;
	}
	if (jn_zero(tmp, sizeof(tmp)))
		return -1;
	memcpy(out, tmp, sizeof(tmp));
	return 0;
}
void
jn_hex(const unsigned char *p, char *out)
{
	static const char hex[] = "0123456789abcdef";
	for (size_t i = 0; i < JN_ID; i++) {
		out[2 * i] = hex[p[i] >> 4];
		out[2 * i + 1] = hex[p[i] & 15];
	}
	out[JN_ID * 2] = '\0';
}
int
jn_encode(const struct jn_msg *m, unsigned char *out, size_t *n)
{
	if (m->length > JN_CONTENT || m->status > JN_UNCERTAIN ||
	    m->rights > 7 || m->revision > INT64_MAX || m->arg > INT64_MAX)
		return -1;
	memset(out, 0, JN_HEADER);
	memcpy(out, "JAN2", 4);
	out[4] = 1;
	out[5] = m->op;
	out[6] = m->status;
	put_be(out + 8, m->length, 4);
	put_be(out + 12, m->rights, 4);
	put_be(out + 16, m->revision, 8);
	put_be(out + 24, m->arg, 8);
	memcpy(out + 32, m->id, 16);
	memcpy(out + 48, m->handle, 16);
	memcpy(out + 64, m->data, m->length);
	*n = 64 + m->length;
	return 0;
}
int
jn_decode(const unsigned char *p, size_t n, struct jn_msg *out)
{
	struct jn_msg m = {0};
	if (n < 64 || n > JN_PACKET || memcmp(p, "JAN2", 4) || p[4] != 1 ||
	    p[7] || p[6] > JN_UNCERTAIN)
		return -1;
	uint64_t length = get_be(p + 8, 4);
	if (length > JN_CONTENT || length != n - 64)
		return -1;
	m.op = p[5];
	m.status = p[6];
	m.length = (size_t)length;
	m.rights = (uint32_t)get_be(p + 12, 4);
	m.revision = get_be(p + 16, 8);
	m.arg = get_be(p + 24, 8);
	if (m.rights > 7 || m.revision > INT64_MAX || m.arg > INT64_MAX)
		return -1;
	memcpy(m.id, p + 32, 16);
	memcpy(m.handle, p + 48, 16);
	memcpy(m.data, p + 64, m.length);
	*out = m;
	return 0;
}
int
jn_request_valid(const struct jn_msg *m, int owner)
{
	if (m->status || m->length > JN_CONTENT || m->revision > INT64_MAX ||
	    m->arg > INT64_MAX || m->rights > 7)
		return 0;
	int id = !jn_zero(m->id, 16), h = !jn_zero(m->handle, 16);
	if (!owner) {
		if (m->arg || m->rights)
			return 0;
		switch (m->op) {
		case JN_ACQUIRE:
			return id && !h && !m->revision && !m->length;
		case JN_GET:
			return !id && h && !m->revision && !m->length;
		case JN_PUT:
			return !id && h && m->revision > 0;
		case JN_DELEGATE:
		case JN_SESSION:
			return !id && !h && !m->revision && !m->length;
		default:
			return 0;
		}
	}
	if (h)
		return 0;
	switch (m->op) {
	case JN_CREATE:
		return !id && !m->rights && !m->revision && !m->arg;
	case JN_GRANT:
		return id && m->arg >= 1 && m->arg <= JN_WORLDS && m->rights &&
		       !m->revision && !m->length;
	case JN_REVOKE:
		return id && m->arg >= 1 && m->arg <= JN_WORLDS && !m->rights &&
		       !m->revision && !m->length;
	case JN_WORLD:
	case JN_STOP:
		return !id && m->arg >= 1 && m->arg <= JN_WORLDS &&
		       !m->rights && !m->revision && !m->length;
	case JN_SAVE:
		return id && m->arg >= 1 && m->arg <= JN_WORLDS &&
		       m->revision > 0 && !m->rights && !m->length;
	case JN_LAUNCH:
		return !m->rights && !m->revision && !m->length &&
		       (m->arg >= 1 && m->arg <= JN_WORLDS);
	case JN_INSPECT:
		return id && !m->arg && !m->rights && !m->revision &&
		       !m->length;
	default:
		return 0;
	}
}
int
jn_send(int fd, const struct jn_msg *m, int pass)
{
	unsigned char bytes[JN_PACKET];
	size_t n;
	if (jn_encode(m, bytes, &n))
		return -1;
	struct iovec iov = {bytes, n};
	char ancillary[CMSG_SPACE(sizeof(int))];
	struct msghdr hdr = {0};
	hdr.msg_iov = &iov;
	hdr.msg_iovlen = 1;
	if (pass >= 0) {
		memset(ancillary, 0, sizeof(ancillary));
		hdr.msg_control = ancillary;
		hdr.msg_controllen = sizeof(ancillary);
		struct cmsghdr *c = CMSG_FIRSTHDR(&hdr);
		c->cmsg_level = SOL_SOCKET;
		c->cmsg_type = SCM_RIGHTS;
		c->cmsg_len = CMSG_LEN(sizeof(int));
		memcpy(CMSG_DATA(c), &pass, sizeof(pass));
	}
	ssize_t k;
	do {
		k = sendmsg(fd, &hdr, MSG_NOSIGNAL | MSG_DONTWAIT);
	} while (k < 0 && errno == EINTR);
	return k == (ssize_t)n ? 0 : -1;
}
int
jn_receive(int fd, struct jn_msg *m, int *pass)
{
	unsigned char buf[JN_PACKET];
	char ancillary[CMSG_SPACE(sizeof(int) * 8)];
	struct iovec iov = {buf, sizeof(buf)};
	struct msghdr hdr = {0};
	hdr.msg_iov = &iov;
	hdr.msg_iovlen = 1;
	hdr.msg_control = ancillary;
	hdr.msg_controllen = sizeof(ancillary);
	ssize_t n;
	do {
		n = recvmsg(fd, &hdr, MSG_CMSG_CLOEXEC | MSG_DONTWAIT);
	} while (n < 0 && errno == EINTR);
	if (n < 0)
		return -1;
	int received = -1, bad = 0;
	for (struct cmsghdr *c = CMSG_FIRSTHDR(&hdr); c;
	     c = CMSG_NXTHDR(&hdr, c)) {
		if (c->cmsg_level != SOL_SOCKET || c->cmsg_type != SCM_RIGHTS) {
			bad = 1;
			continue;
		}
		size_t count = (c->cmsg_len - CMSG_LEN(0)) / sizeof(int);
		for (size_t i = 0; i < count; i++) {
			int f;
			memcpy(&f, CMSG_DATA(c) + i * sizeof(int), sizeof(f));
			if (received < 0 && pass)
				received = f;
			else {
				close(f);
				bad = 1;
			}
		}
	}
	if (hdr.msg_flags & (MSG_TRUNC | MSG_CTRUNC))
		bad = 1;
	if (bad || jn_decode(buf, (size_t)n, m)) {
		if (received >= 0)
			close(received);
		return -1;
	}
	if (pass)
		*pass = received;
	return 0;
}
int
jn_call(int fd, const struct jn_msg *q, struct jn_msg *r)
{
	if (jn_send(fd, q, -1))
		return JN_UNCERTAIN;
	struct pollfd p = {fd, POLLIN, 0};
	int k;
	do {
		k = poll(&p, 1, 3000);
	} while (k < 0 && errno == EINTR);
	if (k <= 0 || jn_receive(fd, r, NULL))
		return JN_UNCERTAIN;
	return r->status;
}
