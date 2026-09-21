/* SPDX-License-Identifier: ISC */
/* Copyright (c) 2026 Danyal A. Samak. Independent M2 expectations. */
#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "janus/native/native.h"
static unsigned int checks;
#define CHECK(x)                                                               \
	do {                                                                   \
		checks++;                                                      \
		if (!(x)) {                                                    \
			fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x);   \
			exit(1);                                               \
		}                                                              \
	} while (0)
static void
sql(struct jn_store *s, const char *query)
{
	sqlite3_stmt *q = NULL;
	CHECK(sqlite3_prepare_v2(s->db, query, -1, &q, NULL) == SQLITE_OK);
	CHECK(sqlite3_step(q) == SQLITE_DONE);
	CHECK(sqlite3_finalize(q) == SQLITE_OK);
}
static void
wire(void)
{
	struct jn_msg m = {.op = JN_GET}, out, before;
	unsigned char packet[JN_PACKET + 1], expected[64] = {0};
	size_t n;
	memset(&out, 0x5a, sizeof(out));
	before = out;
	m.handle[15] = 7;
	memcpy(expected, "JAN2", 4);
	expected[4] = 1;
	expected[5] = 2;
	expected[63] = 7;
	CHECK(!jn_encode(&m, packet, &n) && n == 64 &&
	      !memcmp(packet, expected, 64));
	CHECK(!jn_decode(expected, 64, &out) && out.op == JN_GET &&
	      out.handle[15] == 7);
	CHECK(jn_request_valid(&out, 0));
	static const struct {
		size_t at;
		unsigned char value;
	} bad[] = {{0, 0},  {4, 0},  {4, 2},  {6, 255},  {7, 1},   {8, 1},
	           {11, 1}, {12, 1}, {15, 8}, {16, 128}, {24, 128}};
	for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
		memcpy(packet, expected, 64);
		packet[bad[i].at] = bad[i].value;
		out = before;
		CHECK(jn_decode(packet, 64, &out) == -1);
		CHECK(!memcmp(&out, &before, sizeof(out)));
	}
	for (size_t i = 0; i < 64; i++)
		CHECK(jn_decode(expected, i, &out) == -1);
	memcpy(packet, expected, 64);
	packet[64] = 0;
	CHECK(jn_decode(packet, 65, &out) == -1);
	memset(&m, 0, sizeof(m));
	m.op = JN_CREATE;
	m.length = JN_CONTENT;
	memset(m.data, 'z', JN_CONTENT);
	CHECK(!jn_encode(&m, packet, &n));
	CHECK(!jn_decode(packet, n, &out) && out.length == JN_CONTENT);
	CHECK(jn_request_valid(&out, 1));
	m.length++;
	CHECK(jn_encode(&m, packet, &n) == -1);
	m = (struct jn_msg){.op = JN_GET};
	CHECK(!jn_request_valid(&m, 0));
	m.handle[0] = 1;
	m.arg = 1;
	CHECK(!jn_request_valid(&m, 0));
	m.arg = 0;
	m.rights = 1;
	CHECK(!jn_request_valid(&m, 0));
	m.rights = 0;
	m.length = 1;
	CHECK(!jn_request_valid(&m, 0));
	m = (struct jn_msg){.op = JN_CREATE};
	CHECK(!jn_request_valid(&m, 0));
	m.op = 255;
	CHECK(!jn_request_valid(&m, 0) && !jn_request_valid(&m, 1));
	unsigned char id[16];
	CHECK(jn_unhex("00000000000000000000000000000000", id) == -1);
	CHECK(jn_unhex("112233445566778899aabbccddeeff0G", id) == -1);
	CHECK(jn_unhex("1", id) == -1);
	CHECK(!jn_unhex("0123456789abcdef0123456789abcdef", id));
	char text[33];
	jn_hex(id, text);
	CHECK(!strcmp(text, "0123456789abcdef0123456789abcdef"));
	uint32_t seed = 0x4a324950;
	for (size_t i = 0; i < 2000; i++) {
		for (size_t j = 0; j < JN_PACKET + 1; j++) {
			seed = seed * 1664525u + 1013904223u;
			packet[j] = (unsigned char)(seed >> 24);
		}
		out = before;
		size_t len = i % (JN_PACKET + 1);
		int rc = jn_decode(packet, len, &out);
		CHECK(rc == -1);
		CHECK(!memcmp(&out, &before, sizeof(out)));
	}
	int pair[2];
	CHECK(!socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, pair));
	m = (struct jn_msg){.op = JN_GET};
	m.handle[0] = 1;
	CHECK(!jn_send(pair[0], &m, -1));
	CHECK(!jn_receive(pair[1], &out, NULL));
	int file = open("/dev/null", O_RDONLY | O_CLOEXEC);
	CHECK(file >= 0);
	CHECK(!jn_send(pair[0], &m, file));
	CHECK(jn_receive(pair[1], &out, NULL) == -1);
	CHECK(!close(file));
	CHECK(!close(pair[0]));
	CHECK(!close(pair[1]));
	puts("PASS wire: independent bytes, bounds/reserved fields, 2000 "
	     "malformed packets, FD rejection");
}
static void
world(struct jn_store *s, uint64_t id)
{
	struct jn_msg q = {.op = JN_WORLD, .arg = id}, r;
	CHECK(jn_store_owner(s, &q, &r) == JN_OK);
}
static void
create(struct jn_store *s, const char *text, unsigned char *id)
{
	struct jn_msg q = {.op = JN_CREATE}, r;
	q.length = strlen(text);
	memcpy(q.data, text, q.length);
	CHECK(jn_store_owner(s, &q, &r) == JN_OK && r.revision == 1);
	memcpy(id, r.id, 16);
}
static void
grant(struct jn_store *s, uint64_t w, const unsigned char *id, uint32_t rights)
{
	struct jn_msg q = {.op = JN_GRANT, .arg = w, .rights = rights}, r;
	memcpy(q.id, id, 16);
	CHECK(jn_store_owner(s, &q, &r) == JN_OK);
}
static void
launch(struct jn_store *s, uint64_t w, struct jn_session *session)
{
	struct jn_msg q = {.op = JN_LAUNCH, .arg = w}, r;
	CHECK(jn_store_launch(s, &q, &r, session) == JN_OK && r.arg > 0);
}
static void
handle(struct jn_store *s, struct jn_session *session, const unsigned char *id,
       unsigned char *value)
{
	struct jn_msg q = {.op = JN_ACQUIRE}, r;
	memcpy(q.id, id, 16);
	CHECK(jn_world_request(s, session, &q, &r) == JN_OK);
	memcpy(value, r.handle, 16);
}
static int
deny_insert(void *arg, int action, const char *a, const char *b, const char *c,
            const char *d)
{
	(void)arg;
	(void)b;
	(void)c;
	(void)d;
	return action == SQLITE_INSERT && a && !strcmp(a, "revisions")
	           ? SQLITE_DENY
	           : SQLITE_OK;
}
static void
store_tests(const char *path)
{
	struct jn_store s = {0};
	struct jn_session a, b, old;
	struct jn_msg q = {0}, r;
	unsigned char first[16], second[16], h[16], activity[16];
	CHECK(jn_store_open(&s, path) == JN_OK);
	world(&s, 1);
	world(&s, 2);
	create(&s, "original", first);
	create(&s, "secret", second);
	CHECK(memcmp(first, second, 16));
	grant(&s, 1, first, 3);
	grant(&s, 2, first, 1);
	launch(&s, 1, &a);
	launch(&s, 2, &b);
	handle(&s, &a, first, h);
	q = (struct jn_msg){.op = JN_GET};
	memcpy(q.handle, h, 16);
	CHECK(jn_world_request(&s, &a, &q, &r) == JN_OK && r.length == 8 &&
	      !memcmp(r.data, "original", 8));
	CHECK(jn_world_request(&s, &b, &q, &r) == JN_STALE);
	q = (struct jn_msg){.op = JN_ACQUIRE};
	memcpy(q.id, second, 16);
	CHECK(jn_world_request(&s, &a, &q, &r) == JN_DENIED);
	q = (struct jn_msg){.op = JN_DELEGATE};
	CHECK(jn_world_request(&s, &a, &q, &r) == JN_UNSUPPORTED);
	q = (struct jn_msg){.op = JN_GRANT, .arg = 1, .rights = 5};
	memcpy(q.id, first, 16);
	CHECK(jn_store_owner(&s, &q, &r) == JN_UNSUPPORTED);
	q = (struct jn_msg){.op = JN_PUT, .revision = 1, .length = 5};
	memcpy(q.handle, h, 16);
	memcpy(q.data, "first", 5);
	CHECK(jn_world_request(&s, &a, &q, &r) == JN_OK && r.revision == 2);
	memcpy(q.data, "other", 5);
	CHECK(jn_world_request(&s, &a, &q, &r) == JN_CONFLICT && r.length == 0);
	q = (struct jn_msg){.op = JN_GET};
	memcpy(q.handle, h, 16);
	CHECK(jn_world_request(&s, &a, &q, &r) == JN_OK && r.revision == 2 &&
	      r.length == 5 && !memcmp(r.data, "first", 5));
	unsigned char ro[16];
	handle(&s, &b, first, ro);
	q = (struct jn_msg){.op = JN_PUT, .revision = 2};
	memcpy(q.handle, ro, 16);
	CHECK(jn_world_request(&s, &b, &q, &r) == JN_DENIED);
	old = a;
	launch(&s, 1, &a);
	q = (struct jn_msg){.op = JN_GET};
	memcpy(q.handle, h, 16);
	CHECK(jn_world_request(&s, &a, &q, &r) == JN_STALE);
	CHECK(jn_world_request(&s, &old, &q, &r) == JN_STALE);
	handle(&s, &a, first, h);
	q = (struct jn_msg){.op = JN_SAVE, .arg = 1, .revision = 1};
	memcpy(q.id, first, 16);
	CHECK(jn_store_owner(&s, &q, &r) == JN_OK);
	memcpy(activity, r.id, 16);
	q = (struct jn_msg){.op = JN_REVOKE, .arg = 1};
	memcpy(q.id, first, 16);
	CHECK(jn_store_owner(&s, &q, &r) == JN_OK);
	q = (struct jn_msg){.op = JN_GET};
	memcpy(q.handle, h, 16);
	CHECK(jn_world_request(&s, &a, &q, &r) == JN_DENIED);
	q = (struct jn_msg){.op = JN_LAUNCH, .arg = 1};
	memcpy(q.id, activity, 16);
	CHECK(jn_store_launch(&s, &q, &r, &a) == JN_OK && r.revision == 1 &&
	      !memcmp(r.id, first, 16));
	q = (struct jn_msg){.op = JN_ACQUIRE};
	memcpy(q.id, first, 16);
	CHECK(jn_world_request(&s, &a, &q, &r) == JN_DENIED);
	grant(&s, 1, first, 2);
	handle(&s, &a, first, h);
	q = (struct jn_msg){.op = JN_GET};
	memcpy(q.handle, h, 16);
	CHECK(jn_world_request(&s, &a, &q, &r) == JN_DENIED);
	q.op = JN_PUT;
	q.revision = 1;
	CHECK(jn_world_request(&s, &a, &q, &r) == JN_CONFLICT && r.length == 0);
	q.revision = 2;
	q.length = JN_CONTENT;
	memset(q.data, 'x', JN_CONTENT);
	CHECK(jn_world_request(&s, &a, &q, &r) == JN_OK && r.revision == 3);
	q.revision = 3;
	CHECK(sqlite3_set_authorizer(s.db, deny_insert, NULL) == SQLITE_OK);
	CHECK(jn_world_request(&s, &a, &q, &r) == JN_IO);
	CHECK(sqlite3_set_authorizer(s.db, NULL, NULL) == SQLITE_OK);
	sql(&s, "PRAGMA query_only=ON");
	CHECK(jn_world_request(&s, &a, &q, &r) == JN_IO);
	sql(&s, "PRAGMA query_only=OFF");
	q = (struct jn_msg){.op = JN_INSPECT};
	memcpy(q.id, first, 16);
	CHECK(jn_store_owner(&s, &q, &r) == JN_OK && r.revision == 3 &&
	      r.length == JN_CONTENT);
	for (size_t i = a.used; i < JN_HANDLES; i++)
		handle(&s, &a, first, ro);
	q = (struct jn_msg){.op = JN_ACQUIRE};
	memcpy(q.id, first, 16);
	CHECK(jn_world_request(&s, &a, &q, &r) == JN_LIMIT);
	q = (struct jn_msg){.op = JN_PUT, .revision = 3};
	memcpy(q.handle, h, 16);
	for (uint64_t i = 3; i < JN_REVISIONS; i++) {
		q.revision = i;
		CHECK(jn_world_request(&s, &a, &q, &r) == JN_OK &&
		      r.revision == i + 1);
	}
	q.revision = JN_REVISIONS;
	CHECK(jn_world_request(&s, &a, &q, &r) == JN_LIMIT);
	for (size_t i = 2; i < JN_OBJECTS; i++) {
		unsigned char id[16];
		create(&s, "", id);
		q = (struct jn_msg){.op = JN_GRANT, .arg = 2, .rights = 1};
		memcpy(q.id, id, 16);
		CHECK(jn_store_owner(&s, &q, &r) == (i < 9 ? JN_OK : JN_LIMIT));
	}
	q = (struct jn_msg){.op = JN_CREATE};
	CHECK(jn_store_owner(&s, &q, &r) == JN_LIMIT);
	/* Exhausted generations must never prevent withdrawing authority. */
	sql(&s,
	    "UPDATE grants SET generation=9223372036854775807 WHERE world=1");
	q = (struct jn_msg){.op = JN_GRANT, .arg = 1, .rights = 1};
	memcpy(q.id, first, 16);
	CHECK(jn_store_owner(&s, &q, &r) == JN_LIMIT);
	q.op = JN_REVOKE;
	q.rights = 0;
	CHECK(jn_store_owner(&s, &q, &r) == JN_OK);
	launch(&s, 1, &a);
	q = (struct jn_msg){.op = JN_ACQUIRE};
	memcpy(q.id, first, 16);
	CHECK(jn_world_request(&s, &a, &q, &r) == JN_DENIED);
	q = (struct jn_msg){.op = JN_SAVE, .arg = 1, .revision = 1};
	memcpy(q.id, first, 16);
	for (size_t i = 1; i < 8; i++)
		CHECK(jn_store_owner(&s, &q, &r) == JN_OK);
	CHECK(jn_store_owner(&s, &q, &r) == JN_LIMIT);
	sql(&s, "UPDATE worlds SET inc=9223372036854775807 WHERE id=1");
	q = (struct jn_msg){.op = JN_LAUNCH, .arg = 1};
	CHECK(jn_store_launch(&s, &q, &r, &a) == JN_LIMIT);
	CHECK(jn_store_close(&s) == JN_OK);
	CHECK(jn_store_open(&s, path) == JN_OK);
	q = (struct jn_msg){.op = JN_INSPECT};
	memcpy(q.id, first, 16);
	CHECK(jn_store_owner(&s, &q, &r) == JN_OK &&
	      r.revision == JN_REVISIONS && r.length == 0);
	CHECK(jn_store_close(&s) == JN_OK);
	puts("PASS store: ID/authority, rights/delegation, conflict, stale "
	     "session, revoked activity, limits and SQLite failures");
}
struct checkpoint {
	int fd, stage;
};
static void
halt_stage(void *arg, int stage)
{
	struct checkpoint *c = arg;
	if (stage != c->stage)
		return;
	char byte = 'R';
	if (write(c->fd, &byte, 1) != 1)
		_exit(90);
	for (;;)
		pause();
}
static void
crashes(const char *path)
{
	struct jn_store s = {0};
	unsigned char id[16];
	CHECK(jn_store_open(&s, path) == JN_OK);
	world(&s, 1);
	create(&s, "old-complete", id);
	grant(&s, 1, id, 3);
	CHECK(jn_store_close(&s) == JN_OK);
	for (int stage = 0; stage < 4; stage++) {
		int pipes[2], pair[2], status;
		CHECK(!pipe2(pipes, O_CLOEXEC));
		CHECK(!socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0,
		                  pair));
		pid_t pid = fork();
		CHECK(pid >= 0);
		if (!pid) {
			close(pipes[0]);
			close(pair[0]);
			struct jn_store child = {0};
			struct jn_session session;
			struct jn_msg q, r;
			struct checkpoint point = {pipes[1], stage};
			if (jn_store_open(&child, path))
				_exit(91);
			q = (struct jn_msg){.op = JN_LAUNCH, .arg = 1};
			if (jn_store_launch(&child, &q, &r, &session))
				_exit(92);
			q = (struct jn_msg){.op = JN_ACQUIRE};
			memcpy(q.id, id, 16);
			if (jn_world_request(&child, &session, &q, &r) ||
			    jn_send(pair[1], &r, -1))
				_exit(93);
			child.hook = halt_stage;
			child.hook_arg = &point;
			/* Blocking test wait; production descriptor remains
			 * nonblocking. */
			struct pollfd poller = {pair[1], POLLIN, 0};
			if (poll(&poller, 1, 3000) <= 0 ||
			    jn_receive(pair[1], &q, NULL))
				_exit(94);
			r.status =
			    (uint8_t)jn_world_request(&child, &session, &q, &r);
			(void)jn_send(pair[1], &r, -1);
			_exit(95);
		}
		close(pipes[1]);
		close(pair[1]);
		struct pollfd poller = {pair[0], POLLIN, 0};
		CHECK(poll(&poller, 1, 3000) > 0);
		struct jn_msg r,
		    q = {.op = JN_PUT, .revision = 1, .length = 12};
		CHECK(!jn_receive(pair[0], &r, NULL));
		memcpy(q.handle, r.handle, 16);
		memcpy(q.data, "new-complete", 12);
		CHECK(!jn_send(pair[0], &q, -1));
		char byte;
		CHECK(read(pipes[0], &byte, 1) == 1 && byte == 'R');
		CHECK(!kill(pid, SIGKILL));
		CHECK(waitpid(pid, &status, 0) == pid && WIFSIGNALED(status));
		CHECK(jn_receive(pair[0], &r, NULL) == -1);
		CHECK(!close(pipes[0]));
		CHECK(!close(pair[0]));
		CHECK(jn_store_open(&s, path) == JN_OK);
		q = (struct jn_msg){.op = JN_INSPECT};
		memcpy(q.id, id, 16);
		CHECK(jn_store_owner(&s, &q, &r) == JN_OK);
		CHECK(r.revision == (stage == 3 ? 2u : 1u) && r.length == 12 &&
		      !memcmp(r.data,
		              stage == 3 ? "new-complete" : "old-complete",
		              12));
		CHECK(jn_store_close(&s) == JN_OK);
		printf("PASS crash stage=%d: missing acknowledgement "
		       "UNCERTAIN; reopened revision=%llu complete\n",
		       stage, (unsigned long long)r.revision);
	}
}
int
main(int argc, char **argv)
{
	char root[] = "/tmp/janus-m2-unit.XXXXXX", path[256], crash[256];
	CHECK(getuid() != 0);
	if (argc == 3 && !strcmp(argv[1], "--crash")) {
		struct stat st;
		CHECK(lstat(argv[2], &st) < 0 && errno == ENOENT);
		umask(077);
		crashes(argv[2]);
		printf("PASS native storage-process crashes: %u assertions\n",
		       checks);
		return 0;
	}
	CHECK(argc == 1);
	CHECK(mkdtemp(root) != NULL);
	int n = snprintf(path, sizeof(path), "%s/store.db", root);
	CHECK(n > 0 && (size_t)n < sizeof(path));
	n = snprintf(crash, sizeof(crash), "%s/crash.db", root);
	CHECK(n > 0 && (size_t)n < sizeof(crash));
	wire();
	store_tests(path);
	crashes(crash);
	struct jn_store s = {0};
	CHECK(!chmod(path, 0400));
	CHECK(jn_store_open(&s, path) == JN_OK);
	struct jn_msg q = {.op = JN_WORLD, .arg = 3}, r;
	CHECK(jn_store_owner(&s, &q, &r) == JN_IO);
	CHECK(jn_store_close(&s) == JN_OK);
	CHECK(!chmod(path, 0600));
	int fd = open(path, O_WRONLY | O_TRUNC | O_CLOEXEC);
	CHECK(fd >= 0);
	CHECK(write(fd, "corrupt", 7) == 7);
	CHECK(!close(fd));
	CHECK(jn_store_open(&s, path) == JN_IO);
	CHECK(!unlink(path));
	CHECK(!unlink(crash));
	CHECK(!rmdir(root));
	printf("PASS native: %u assertions\n", checks);
	return 0;
}
