/* SPDX-License-Identifier: ISC */
/* JANUS independent hosted tests; Copyright (c) 2026 Danyal A. Samak. */
#define _GNU_SOURCE
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "janus/hosted/hosted.h"
static unsigned int checks;
#define CHECK(x)                                                               \
	do {                                                                   \
		checks++;                                                      \
		if (!(x)) {                                                    \
			fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x);        \
			exit(1);                                               \
		}                                                              \
	} while (0)
static const char uuid[] = "cbe6cc97-2c72-4c6f-bf7a-18a10df9a811";
struct fake {
	enum jh_execution state;
	int lookfail, startfail, spawnfail, savefail, stopfail, alive;
	unsigned int starts, spawns, stops;
	const char *epoch;
};
static int
backend(void *arg, const struct jh_profile *p, int start, struct jh_snapshot *s)
{
	struct fake *f = arg;
	CHECK(!strcmp(p->uuid, uuid));
	if (f->lookfail)
		return -1;
	if (start) {
		f->starts++;
		if (f->startfail)
			return -1;
		f->state = JH_ACTIVE;
	}
	s->execution = f->state;
	strcpy(s->epoch, f->epoch);
	return 0;
}
static int
spawn(void *arg, const struct jh_profile *p, pid_t *pid)
{
	struct fake *f = arg;
	(void)p;
	if (f->spawnfail) {
		if (f->spawnfail == 2) {
			*pid = 123;
			f->alive = 1;
		}
		return -1;
	}
	f->spawns++;
	f->alive = 1;
	*pid = 123;
	return 0;
}
static int
stop(void *arg, pid_t pid)
{
	struct fake *f = arg;
	CHECK(pid == 123);
	f->stops++;
	if (f->stopfail)
		return -1;
	f->alive = 0;
	return 0;
}
static int
alive(void *arg, pid_t pid)
{
	struct fake *f = arg;
	CHECK(pid == 123);
	return f->alive;
}
static int
save(void *arg, const struct jh_profile *p, const struct jh_record *r)
{
	struct fake *f = arg;
	(void)p;
	(void)r;
	return f->savefail ? -1 : 0;
}
static void
fresh(struct jh_world *w, struct fake *f)
{
	memset(w, 0, sizeof(*w));
	memset(f, 0, sizeof(*f));
	strcpy(w->profile.id, "lab");
	strcpy(w->profile.uri, "qemu:///session");
	strcpy(w->profile.uuid, uuid);
	f->epoch = "boot:daemon:birth:1";
}
static void
state_tests(void)
{
	struct jh_world w, before;
	struct fake f;
	struct jh_adapters a = {&f, backend, spawn, stop, alive, save};
	static const struct {
		enum jh_command op;
		const char *result;
		unsigned int starts, spawns;
		int focus;
	} table[] = {{JH_SELECT, "ok", 0, 0, 0}, {JH_RUN, "ok", 1, 1, 1},
	             {JH_RUN, "ok", 1, 1, 1},    {JH_LEAVE, "ok", 1, 1, 0},
	             {JH_RUN, "ok", 1, 2, 1},    {JH_RECOVER, "ok", 1, 2, 0}};
	fresh(&w, &f);
	for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
		CHECK(!strcmp(jh_handle(&w, table[i].op, &a), table[i].result));
		CHECK(f.starts == table[i].starts &&
		      f.spawns == table[i].spawns);
		CHECK(w.foreground == table[i].focus);
	}
	CHECK(w.record.incarnation == 1 && f.state == JH_ACTIVE);
	CHECK(!strcmp(jh_handle(&w, JH_RUN, &a), "ok"));
	f.alive = 0;
	CHECK(!strcmp(jh_handle(&w, JH_STATUS, &a), "ok"));
	CHECK(!w.viewer && !w.foreground && w.observed.execution == JH_ACTIVE);
	CHECK(w.record.incarnation == 1);
	/* Restart never imports a viewer; active record must match exactly. */
	w.viewer = 0;
	w.foreground = 0;
	CHECK(!strcmp(jh_handle(&w, JH_STATUS, &a), "ok"));
	f.epoch = "boot:daemon:birth:2";
	CHECK(!strcmp(jh_handle(&w, JH_RUN, &a), "reconciliation-required"));
	CHECK(f.starts == 1 && f.spawns == 3);
	f.state = JH_INACTIVE;
	CHECK(!strcmp(jh_handle(&w, JH_STATUS, &a), "ok"));
	CHECK(!strcmp(jh_handle(&w, JH_RUN, &a), "ok"));
	CHECK(w.record.incarnation == 2 && f.starts == 2);
	f.state = JH_PAUSED;
	CHECK(!strcmp(jh_handle(&w, JH_RUN, &a), "paused"));
	before = w;
	CHECK(!strcmp(jh_handle(&w, (enum jh_command) - 1, &a), "invalid"));
	CHECK(!memcmp(&w, &before, sizeof(w)));
	fresh(&w, &f);
	f.state = JH_ACTIVE;
	CHECK(!strcmp(jh_handle(&w, JH_RUN, &a), "reconciliation-required"));
	CHECK(!f.starts && !f.spawns);
	fresh(&w, &f);
	f.lookfail = 1;
	CHECK(!strcmp(jh_handle(&w, JH_RUN, &a), "backend-error"));
	CHECK(!f.starts && !f.spawns);
	fresh(&w, &f);
	f.startfail = 1;
	CHECK(!strcmp(jh_handle(&w, JH_RUN, &a), "start-uncertain"));
	CHECK(w.record.phase == JH_STARTING && !f.spawns);
	f.state = JH_ACTIVE;
	CHECK(!strcmp(jh_handle(&w, JH_RUN, &a), "reconciliation-required"));
	CHECK(f.starts == 1);
	fresh(&w, &f);
	f.spawnfail = 1;
	CHECK(!strcmp(jh_handle(&w, JH_RUN, &a), "viewer-error"));
	CHECK(w.record.phase == JH_STARTED && !w.viewer && !w.foreground);
	f.spawnfail = 0;
	CHECK(!strcmp(jh_handle(&w, JH_RUN, &a), "ok"));
	CHECK(f.starts == 1 && f.spawns == 1);
	f.stopfail = 1;
	CHECK(!strcmp(jh_handle(&w, JH_RECOVER, &a), "viewer-error"));
	CHECK(!w.foreground && w.viewer == 123 && f.state == JH_ACTIVE);
	f.stopfail = 0;
	f.lookfail = 1;
	CHECK(!strcmp(jh_handle(&w, JH_RECOVER, &a), "backend-error"));
	CHECK(!w.viewer && !w.foreground);
	fresh(&w, &f);
	f.spawnfail = 2;
	CHECK(!strcmp(jh_handle(&w, JH_RUN, &a), "viewer-error"));
	CHECK(w.viewer == 123 && !w.foreground);
	f.stopfail = 1;
	CHECK(!strcmp(jh_handle(&w, JH_RUN, &a), "viewer-error"));
	CHECK(!w.foreground && f.starts == 1);
	f.stopfail = 0;
	f.spawnfail = 0;
	CHECK(!strcmp(jh_handle(&w, JH_RUN, &a), "ok"));
	CHECK(w.foreground && f.starts == 1 && f.spawns == 1);
	fresh(&w, &f);
	f.savefail = 1;
	CHECK(!strcmp(jh_handle(&w, JH_RUN, &a), "record-error"));
	CHECK(!f.starts && !f.spawns && w.record.phase == JH_BAD);
	fresh(&w, &f);
	w.record.phase = JH_STOPPED;
	w.record.incarnation = UINT64_MAX;
	CHECK(!strcmp(jh_handle(&w, JH_RUN, &a), "limit"));
	CHECK(!f.starts);
	puts("PASS state: idempotence, lifecycle, failures, uncertainty, "
	     "limits");
}
static void
protocol_tests(void)
{
	static const char *bad[] = {"",
	                            "run lab",
	                            "run lab\nextra",
	                            "run lab \n",
	                            "run  lab\n",
	                            "run ../x\n",
	                            "run lab;id\n",
	                            "run $HOME\n",
	                            "run lab\r\n",
	                            "qmp lab\n",
	                            "xml lab\n",
	                            "run lab\nrun lab\n",
	                            "run\tlab\n"};
	static const char *good[] = {"select lab\n", "status lab\n",
	                             "run lab\n", "leave lab\n",
	                             "recover lab\n"};
	struct jh_request r, before;
	memset(&r, 0x5a, sizeof(r));
	before = r;
	for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
		CHECK(jh_parse(bad[i], strlen(bad[i]), &r) == -1);
		CHECK(!memcmp(&r, &before, sizeof(r)));
	}
	for (size_t i = 0; i < 5; i++) {
		CHECK(!jh_parse(good[i], strlen(good[i]), &r));
		CHECK((size_t)r.command == i);
	}
	char buf[128];
	memset(buf, 'a', sizeof(buf));
	buf[127] = '\n';
	CHECK(jh_parse(buf, sizeof(buf), &r) == -1);
	CHECK(jh_parse("run lab\0\n", 9, &r) == -1);
	CHECK(!jh_uuid("00000000-0000-0000-0000-000000000000"));
	CHECK(!jh_uuid("cbe6cc97-2c72-4c6f-bf7a-18a10df9a81x"));
	CHECK(jh_uuid(uuid));
	int sockets[2];
	CHECK(!socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, sockets));
	CHECK(!jh_peer(sockets[0], getuid()));
	CHECK(jh_peer(sockets[0], getuid() + 1) == -1);
	close(sockets[0]);
	close(sockets[1]);
	puts(
	    "PASS protocol: exact grammar, lengths, UUID and peer credentials");
}
static void
file_tests(void)
{
	char dir[] = "/tmp/janus-unit.XXXXXX", path[128], buf[512];
	struct jh_profile profiles[JH_PROFILES];
	size_t count = 0;
	struct jh_world w;
	struct fake f;
	CHECK(mkdtemp(dir) != NULL);
	int n = snprintf(path, sizeof(path), "%s/profiles", dir);
	CHECK(n > 0 && (size_t)n < sizeof(path));
	int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
	CHECK(fd >= 0);
	n = snprintf(buf, sizeof(buf), "lab qemu:///session %s\n", uuid);
	CHECK(n > 0 && (size_t)n < sizeof(buf));
	CHECK(!jh_write(fd, buf, (size_t)n));
	CHECK(!close(fd));
	CHECK(!jh_profiles(path, profiles, &count) && count == 1);
	fd = open(path, O_WRONLY | O_APPEND);
	CHECK(fd >= 0);
	CHECK(!jh_write(fd, buf, (size_t)n));
	CHECK(!close(fd));
	CHECK(jh_profiles(path, profiles, &count) == -1);
	/* Malformed profile and count exhaustion preserve caller output. */
	fd = open(path, O_WRONLY | O_TRUNC);
	CHECK(fd >= 0);
	const char bad_profile[] = "lab qemu:///session invalid\n";
	CHECK(!jh_write(fd, bad_profile, sizeof(bad_profile) - 1));
	CHECK(!close(fd));
	CHECK(jh_profiles(path, profiles, &count) == -1 && count == 1);
	fd = open(path, O_WRONLY | O_TRUNC);
	CHECK(fd >= 0);
	for (unsigned int i = 0; i < JH_PROFILES + 1; i++) {
		n = snprintf(
		    buf, sizeof(buf),
		    "p%u qemu:///session cbe6cc97-2c72-4c6f-bf7a-%012u\n", i,
		    i + 1);
		CHECK(n > 0 && (size_t)n < sizeof(buf));
		CHECK(!jh_write(fd, buf, (size_t)n));
	}
	CHECK(!close(fd));
	CHECK(jh_profiles(path, profiles, &count) == -1 && count == 1);
	CHECK(!chmod(path, 0644));
	CHECK(jh_profiles(path, profiles, &count) == -1);
	CHECK(!unlink(path));
	CHECK(!symlink("/dev/null", path));
	CHECK(jh_profiles(path, profiles, &count) == -1);
	CHECK(!unlink(path));
	struct jh_host h = {jh_directory(dir), NULL, NULL, 0};
	CHECK(h.dirfd >= 0);
	fresh(&w, &f);
	struct jh_record r = {JH_STARTED, 7, "boot:daemon:birth:1"}, got;
	CHECK(!jh_load(h.dirfd, &w.profile, &got) && got.phase == JH_MISSING);
	CHECK(!jh_save(&h, &w.profile, &r));
	CHECK(!jh_load(h.dirfd, &w.profile, &got));
	CHECK(got.phase == r.phase && got.incarnation == 7 &&
	      !strcmp(got.epoch, r.epoch));
	strcpy(w.profile.uuid, "dbe6cc97-2c72-4c6f-bf7a-18a10df9a811");
	CHECK(jh_load(h.dirfd, &w.profile, &got) == -1);
	CHECK(!unlinkat(h.dirfd, "lab.state", 0));
	CHECK(!close(h.dirfd));
	CHECK(!chmod(dir, 0755));
	CHECK(jh_directory(dir) == -1);
	CHECK(!rmdir(dir));
	puts("PASS files: ownership, permissions, duplicate profiles, record "
	     "identity");
}
static void
process_tests(void)
{
	pid_t pid = 0;
	int status;
	char *argv[] = {"/bin/sleep", "20", NULL},
	     *env[] = {"PATH=/usr/bin:/bin", NULL};
	CHECK(jh_exec("/does-not-exist", argv, env, &pid) == -1 && !pid);
	CHECK(!jh_exec(argv[0], argv, env, &pid));
	CHECK(jh_alive(NULL, pid) == 1);
	CHECK(!jh_stop(NULL, pid));
	CHECK(waitpid(pid, &status, WNOHANG) == -1 && errno == ECHILD);
	pid = fork();
	CHECK(pid >= 0);
	if (!pid)
		_exit(0);
	CHECK(jh_wait(pid, 1000, &status) == 1 && WIFEXITED(status));
	pid = fork();
	CHECK(pid >= 0);
	if (!pid) {
		raise(SIGKILL);
		_exit(1);
	}
	CHECK(jh_wait(pid, 1000, &status) == 1 && WIFSIGNALED(status));
	int pipes[2];
	CHECK(!pipe2(pipes, O_CLOEXEC));
	pid = fork();
	CHECK(pid >= 0);
	if (!pid) {
		close(pipes[0]);
		signal(SIGTERM, SIG_IGN);
		(void)jh_write(pipes[1], "R", 1);
		for (;;)
			pause();
	}
	close(pipes[1]);
	char byte;
	CHECK(read(pipes[0], &byte, 1) == 1);
	close(pipes[0]);
	CHECK(!jh_stop(NULL, pid));
	CHECK(waitpid(pid, &status, WNOHANG) == -1 && errno == ECHILD);
	struct rlimit old, limit;
	CHECK(!getrlimit(RLIMIT_NPROC, &old));
	limit = old;
	limit.rlim_cur = 0;
	CHECK(!setrlimit(RLIMIT_NPROC, &limit));
	pid = 0;
	CHECK(jh_exec(argv[0], argv, env, &pid) == -1 && !pid);
	CHECK(!setrlimit(RLIMIT_NPROC, &old));
	CHECK(!getrlimit(RLIMIT_NOFILE, &old));
	limit = old;
	limit.rlim_cur = 0;
	CHECK(!setrlimit(RLIMIT_NOFILE, &limit));
	CHECK(jh_exec(argv[0], argv, env, &pid) == -1);
	CHECK(!setrlimit(RLIMIT_NOFILE, &old));
	puts("PASS processes: exec/fork failure, clean/crash exit, forced "
	     "recovery, FD exhaustion");
}
int
main(void)
{
	CHECK(getuid() != 0);
	state_tests();
	protocol_tests();
	file_tests();
	process_tests();
	printf("PASS hosted: %u assertions\n", checks);
	return 0;
}
