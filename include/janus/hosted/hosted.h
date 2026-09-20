/* SPDX-License-Identifier: ISC */
/* JANUS hosted experiment; Copyright (c) 2026 Danyal A. Samak. */
/* EXPERIMENTAL internal state, never a stable ABI or serialized struct. */
#ifndef JANUS_HOSTED_H
#define JANUS_HOSTED_H
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#define JH_PROFILES 8
#define JH_REQUEST 64
#define JH_REPLY 512
#define JH_EPOCH 160

enum jh_command { JH_SELECT, JH_STATUS, JH_RUN, JH_LEAVE, JH_RECOVER };
enum jh_execution { JH_INACTIVE, JH_ACTIVE, JH_PAUSED, JH_UNKNOWN };
enum jh_phase { JH_MISSING, JH_STOPPED, JH_STARTING, JH_STARTED, JH_BAD };
struct jh_request {
	enum jh_command command;
	char id[33];
};
struct jh_profile {
	char id[33], uri[24], uuid[37];
};
struct jh_snapshot {
	enum jh_execution execution;
	char epoch[JH_EPOCH];
};
struct jh_record {
	enum jh_phase phase;
	uint64_t incarnation;
	char epoch[JH_EPOCH];
};
struct jh_world {
	struct jh_profile profile;
	struct jh_record record;
	struct jh_snapshot observed;
	pid_t viewer;
	int foreground;
};
/* Spawn returns any still-owned child even on cleanup failure.
 * Serial, caller-owned state. Adapter failures are explicit, never fake
 * success. */
struct jh_adapters {
	void *arg;
	int (*backend)(void *, const struct jh_profile *, int,
	               struct jh_snapshot *);
	int (*spawn)(void *, const struct jh_profile *, pid_t *);
	int (*stop)(void *, pid_t);
	int (*alive)(void *, pid_t);
	int (*save)(void *, const struct jh_profile *,
	            const struct jh_record *);
};
struct jh_host {
	int dirfd;
	const char *display_dir, *display_name;
	pid_t worker;
};
int jh_id(const char *);
int jh_uuid(const char *);
int jh_parse(const char *, size_t, struct jh_request *);
int jh_profiles(const char *, struct jh_profile *, size_t *);
int jh_peer(int, uid_t);
int jh_directory(const char *);
int jh_address(const char *, void *, size_t *);
int jh_load(int, const struct jh_profile *, struct jh_record *);
int jh_save(void *, const struct jh_profile *, const struct jh_record *);
const char *jh_handle(struct jh_world *, enum jh_command,
                      const struct jh_adapters *);
int jh_refresh_viewer(struct jh_world *, const struct jh_adapters *);
int jh_reply(char *, size_t, const struct jh_world *, const char *);
int jh_backend(void *, const struct jh_profile *, int, struct jh_snapshot *);
int jh_spawn(void *, const struct jh_profile *, pid_t *);
int jh_exec(const char *, char *const[], char *const[], pid_t *);
int jh_stop(void *, pid_t);
int jh_alive(void *, pid_t);
int jh_wait(pid_t, int, int *);
int jh_write(int, const void *, size_t);
int jh_read_file(int, char *, size_t, size_t *);
#endif
