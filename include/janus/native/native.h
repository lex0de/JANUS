/* SPDX-License-Identifier: ISC */
/* Copyright (c) 2026 Danyal A. Samak. EXPERIMENTAL hosted interfaces. */
#ifndef JANUS_NATIVE_H
#define JANUS_NATIVE_H
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#define JN_ID 16
#define JN_CONTENT 1024
#define JN_HEADER 64
#define JN_PACKET (JN_HEADER + JN_CONTENT)
#define JN_WORLDS 4
#define JN_OBJECTS 16
#define JN_HANDLES 8
#define JN_REVISIONS 16
#define JN_SECRET 32
#define JN_READ 1u
#define JN_WRITE 2u
#define JN_DELEGABLE 4u
/* Encoding is explicit; no struct is sent or persisted. */
enum jn_op {
	JN_ACQUIRE = 1,
	JN_GET,
	JN_PUT,
	JN_DELEGATE,
	JN_SESSION,
	JN_CREATE = 32,
	JN_GRANT,
	JN_REVOKE,
	JN_WORLD,
	JN_SAVE,
	JN_LAUNCH,
	JN_INSPECT,
	JN_STOP
};
enum jn_status {
	JN_OK,
	JN_INVALID,
	JN_DENIED,
	JN_CONFLICT,
	JN_LIMIT,
	JN_IO,
	JN_STALE,
	JN_UNSUPPORTED,
	JN_BUSY,
	JN_UNCERTAIN
};
struct jn_msg {
	uint8_t op, status;
	uint32_t rights;
	uint64_t revision, arg;
	unsigned char id[JN_ID], handle[JN_ID], data[JN_CONTENT];
	size_t length;
};
struct sqlite3;
struct jn_store {
	struct sqlite3 *db;
	void (*hook)(void *, int);
	void *hook_arg;
};
struct jn_handle {
	unsigned char value[JN_ID], object[JN_ID];
	uint64_t generation;
};
struct jn_session {
	uint64_t world, incarnation;
	size_t used;
	struct jn_handle handles[JN_HANDLES];
};
int jn_random(void *, size_t);
int jn_zero(const unsigned char *, size_t);
int jn_unhex(const char *, unsigned char *);
void jn_hex(const unsigned char *, char *);
int jn_encode(const struct jn_msg *, unsigned char *, size_t *);
int jn_decode(const unsigned char *, size_t, struct jn_msg *);
int jn_request_valid(const struct jn_msg *, int);
int jn_send(int, const struct jn_msg *, int);
int jn_receive(int, struct jn_msg *, int *);
int jn_call(int, const struct jn_msg *, struct jn_msg *);
int jn_store_open(struct jn_store *, const char *);
int jn_store_close(struct jn_store *);
int jn_store_owner(struct jn_store *, const struct jn_msg *, struct jn_msg *);
int jn_store_launch(struct jn_store *, const struct jn_msg *, struct jn_msg *,
                    struct jn_session *);
int jn_world_request(struct jn_store *, struct jn_session *,
                     const struct jn_msg *, struct jn_msg *);
int jn_runtime(const char *);
int jn_secret_write(int, unsigned char *);
int jn_owner_call(const char *, const struct jn_msg *, struct jn_msg *, int *);
int jn_landlock_abi(void);
int jn_sandbox(int);
int jn_protect(void);
int jn_number(const char *, uint64_t *);
void jn_print(const struct jn_msg *, int);
#endif
