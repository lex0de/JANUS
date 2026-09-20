/* SPDX-License-Identifier: ISC */
/* Copyright (c) 2026 Danyal A. Samak. */
#include <sqlite3.h>
#include <limits.h>
#include <string.h>
#include "janus/native/native.h"
static int
finish(sqlite3_stmt *s, int result)
{
	return sqlite3_finalize(s) == SQLITE_OK ? result : JN_IO;
}
static int
prepare(struct jn_store *s, const char *sql, sqlite3_stmt **q)
{
	return sqlite3_prepare_v2(s->db, sql, -1, q, NULL) == SQLITE_OK ? JN_OK
	                                                                : JN_IO;
}
static int
execute(struct jn_store *s, const char *sql)
{
	sqlite3_stmt *q = NULL;
	if (prepare(s, sql, &q))
		return JN_IO;
	int rc = sqlite3_step(q) == SQLITE_DONE ? JN_OK : JN_IO;
	return finish(q, rc);
}
static int
number(struct jn_store *s, const char *sql, uint64_t *out)
{
	sqlite3_stmt *q = NULL;
	if (prepare(s, sql, &q))
		return JN_IO;
	int rc = JN_IO;
	if (sqlite3_step(q) == SQLITE_ROW &&
	    sqlite3_column_type(q, 0) == SQLITE_INTEGER &&
	    sqlite3_column_int64(q, 0) >= 0) {
		*out = (uint64_t)sqlite3_column_int64(q, 0);
		rc = sqlite3_step(q) == SQLITE_DONE ? JN_OK : JN_IO;
	}
	return finish(q, rc);
}
static int
text_result(struct jn_store *s, const char *sql, const char *expected)
{
	sqlite3_stmt *q = NULL;
	if (prepare(s, sql, &q))
		return JN_IO;
	int rc = JN_IO;
	if (sqlite3_step(q) == SQLITE_ROW &&
	    sqlite3_column_type(q, 0) == SQLITE_TEXT) {
		const unsigned char *v = sqlite3_column_text(q, 0);
		if (v && !strcmp((const char *)v, expected) &&
		    sqlite3_step(q) == SQLITE_DONE)
			rc = JN_OK;
	}
	return finish(q, rc);
}
int
jn_store_open(struct jn_store *s, const char *path)
{
	static const char *schema[] = {
	    "CREATE TABLE worlds(id INTEGER PRIMARY KEY CHECK(id BETWEEN 1 AND "
	    "4),app TEXT NOT NULL CHECK(app='janus-note'),inc INTEGER NOT NULL "
	    "CHECK(inc>=0))",
	    "CREATE TABLE objects(id BLOB PRIMARY KEY "
	    "CHECK(length(id)=16),kind INTEGER NOT NULL CHECK(kind=1),current "
	    "INTEGER NOT NULL CHECK(current BETWEEN 1 AND 16))",
	    "CREATE TABLE revisions(object BLOB NOT NULL REFERENCES "
	    "objects(id),rev INTEGER NOT NULL CHECK(rev BETWEEN 1 AND "
	    "16),content BLOB NOT NULL CHECK(length(content)<=1024),PRIMARY "
	    "KEY(object,rev))",
	    "CREATE TABLE grants(world INTEGER NOT NULL REFERENCES "
	    "worlds(id),object BLOB NOT NULL REFERENCES objects(id),rights "
	    "INTEGER NOT NULL CHECK(rights BETWEEN 1 AND 3),delegable INTEGER "
	    "NOT NULL CHECK(delegable=0),live INTEGER NOT NULL CHECK(live IN "
	    "(0,1)),generation INTEGER NOT NULL CHECK(generation>0),issuer "
	    "TEXT NOT NULL CHECK(issuer='OWNER'),PRIMARY KEY(world,object))",
	    "CREATE TABLE activities(id BLOB PRIMARY KEY "
	    "CHECK(length(id)=16),world INTEGER NOT NULL REFERENCES "
	    "worlds(id),object BLOB NOT NULL,rev INTEGER NOT NULL,FOREIGN "
	    "KEY(object,rev) REFERENCES revisions(object,rev))"};
	uint64_t n = 0;
	int enabled = 0;
	s->db = NULL;
	if (sqlite3_open_v2(path, &s->db,
	                    SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
	                        SQLITE_OPEN_NOMUTEX,
	                    NULL) != SQLITE_OK)
		goto fail;
	if (sqlite3_busy_timeout(s->db, 100) != SQLITE_OK ||
	    sqlite3_db_config(s->db, SQLITE_DBCONFIG_DEFENSIVE, 1, &enabled) !=
	        SQLITE_OK ||
	    enabled != 1)
		goto fail;
	(void)sqlite3_limit(s->db, SQLITE_LIMIT_LENGTH, 16384);
	(void)sqlite3_limit(s->db, SQLITE_LIMIT_SQL_LENGTH, 8192);
	if (execute(s, "PRAGMA foreign_keys=ON") ||
	    number(s, "PRAGMA foreign_keys", &n) || n != 1 ||
	    execute(s, "PRAGMA trusted_schema=OFF") ||
	    execute(s, "PRAGMA temp_store=MEMORY") ||
	    text_result(s, "PRAGMA journal_mode=DELETE", "delete") ||
	    execute(s, "PRAGMA synchronous=FULL") ||
	    number(s, "PRAGMA synchronous", &n) || n != 2 ||
	    number(s, "PRAGMA max_page_count=2048", &n) || n > 2048 ||
	    number(s, "PRAGMA user_version", &n))
		goto fail;
	if (n == 0) {
		if (number(s,
		           "SELECT count(*) FROM sqlite_schema WHERE name NOT "
		           "LIKE 'sqlite_%'",
		           &n) ||
		    n || execute(s, "BEGIN IMMEDIATE"))
			goto fail;
		for (size_t i = 0; i < sizeof(schema) / sizeof(schema[0]); i++)
			if (execute(s, schema[i]))
				goto rollback;
		if (execute(s, "PRAGMA user_version=2") || execute(s, "COMMIT"))
			goto rollback;
	} else if (n != 2)
		goto fail;
	if (text_result(s, "PRAGMA quick_check", "ok") ||
	    execute(s, "PRAGMA foreign_key_check") ||
	    number(s,
	           "SELECT count(*) FROM objects o LEFT JOIN revisions r ON "
	           "o.id=r.object AND o.current=r.rev WHERE r.object IS NULL",
	           &n) ||
	    n || number(s, "SELECT count(*) FROM objects", &n) ||
	    n > JN_OBJECTS || number(s, "SELECT count(*) FROM worlds", &n) ||
	    n > JN_WORLDS || number(s, "SELECT count(*) FROM activities", &n) ||
	    n > 8)
		goto fail;
	return JN_OK;
rollback:
	if (execute(s, "ROLLBACK"))
		goto fail;
fail:
	if (s->db) {
		int rc = sqlite3_close(s->db);
		if (rc != SQLITE_OK)
			return JN_IO;
		s->db = NULL;
	}
	return JN_IO;
}
int
jn_store_close(struct jn_store *s)
{
	if (!s->db)
		return JN_OK;
	int rc = sqlite3_close(s->db);
	if (rc != SQLITE_OK)
		return JN_IO;
	s->db = NULL;
	return JN_OK;
}
static int
object(struct jn_store *s, const unsigned char *id, struct jn_msg *r)
{
	sqlite3_stmt *q = NULL;
	int rc = JN_IO;
	if (prepare(
	        s,
	        "SELECT o.current,r.content FROM objects o JOIN revisions r ON "
	        "r.object=o.id AND r.rev=o.current WHERE o.id=?1 AND o.kind=1",
	        &q))
		return rc;
	if (sqlite3_bind_blob(q, 1, id, 16, SQLITE_TRANSIENT) != SQLITE_OK)
		return finish(q, rc);
	int step = sqlite3_step(q);
	if (step == SQLITE_DONE)
		rc = JN_DENIED;
	else if (step == SQLITE_ROW &&
	         sqlite3_column_type(q, 0) == SQLITE_INTEGER &&
	         sqlite3_column_int64(q, 0) > 0 &&
	         sqlite3_column_int64(q, 0) <= JN_REVISIONS &&
	         sqlite3_column_type(q, 1) == SQLITE_BLOB) {
		int n = sqlite3_column_bytes(q, 1);
		const void *p = sqlite3_column_blob(q, 1);
		if (n >= 0 && n <= JN_CONTENT && (p || !n)) {
			r->revision = (uint64_t)sqlite3_column_int64(q, 0);
			r->length = (size_t)n;
			if (n)
				memcpy(r->data, p, (size_t)n);
			memcpy(r->id, id, 16);
			rc = sqlite3_step(q) == SQLITE_DONE ? JN_OK : JN_IO;
		}
	}
	return finish(q, rc);
}
static int
permission(struct jn_store *s, uint64_t world, const unsigned char *id,
           uint32_t *rights, uint64_t *generation)
{
	sqlite3_stmt *q = NULL;
	int rc = JN_IO;
	if (prepare(s,
	            "SELECT rights,generation,delegable FROM grants WHERE "
	            "world=?1 AND object=?2 AND live=1 AND issuer='OWNER'",
	            &q))
		return rc;
	if (sqlite3_bind_int64(q, 1, (sqlite3_int64)world) != SQLITE_OK ||
	    sqlite3_bind_blob(q, 2, id, 16, SQLITE_TRANSIENT) != SQLITE_OK)
		return finish(q, rc);
	int step = sqlite3_step(q);
	if (step == SQLITE_DONE)
		rc = JN_DENIED;
	else if (step == SQLITE_ROW &&
	         sqlite3_column_type(q, 0) == SQLITE_INTEGER &&
	         sqlite3_column_int(q, 0) >= 1 &&
	         sqlite3_column_int(q, 0) <= 3 &&
	         sqlite3_column_type(q, 1) == SQLITE_INTEGER &&
	         sqlite3_column_int64(q, 1) > 0 &&
	         sqlite3_column_type(q, 2) == SQLITE_INTEGER &&
	         sqlite3_column_int(q, 2) == 0) {
		*rights = (uint32_t)sqlite3_column_int(q, 0);
		*generation = (uint64_t)sqlite3_column_int64(q, 1);
		rc = sqlite3_step(q) == SQLITE_DONE ? JN_OK : JN_IO;
	}
	return finish(q, rc);
}
static int
create_object(struct jn_store *s, const struct jn_msg *m, struct jn_msg *r)
{
	uint64_t count;
	sqlite3_stmt *q = NULL;
	int rc = JN_IO;
	if (number(s, "SELECT count(*) FROM objects", &count))
		return rc;
	if (count >= JN_OBJECTS)
		return JN_LIMIT;
	if (jn_random(r->id, 16) || jn_zero(r->id, 16) ||
	    execute(s, "BEGIN IMMEDIATE"))
		return rc;
	if (prepare(s, "INSERT INTO objects VALUES(?1,1,1)", &q))
		goto fail;
	if (sqlite3_bind_blob(q, 1, r->id, 16, SQLITE_TRANSIENT) != SQLITE_OK ||
	    sqlite3_step(q) != SQLITE_DONE) {
		rc = finish(q, JN_IO);
		q = NULL;
		goto fail;
	}
	rc = finish(q, JN_OK);
	q = NULL;
	if (rc)
		goto fail;
	if (prepare(s, "INSERT INTO revisions VALUES(?1,1,?2)", &q))
		goto fail;
	if (sqlite3_bind_blob(q, 1, r->id, 16, SQLITE_TRANSIENT) != SQLITE_OK ||
	    sqlite3_bind_blob(q, 2, m->data, (int)m->length,
	                      SQLITE_TRANSIENT) != SQLITE_OK ||
	    sqlite3_step(q) != SQLITE_DONE) {
		rc = finish(q, JN_IO);
		q = NULL;
		goto fail;
	}
	rc = finish(q, JN_OK);
	q = NULL;
	if (rc)
		goto fail;
	if (execute(s, "COMMIT")) {
		rc = JN_UNCERTAIN;
		goto fail;
	}
	r->revision = 1;
	return JN_OK;
fail:
	if (execute(s, "ROLLBACK"))
		return JN_UNCERTAIN;
	return rc ? rc : JN_IO;
}
static int
update(struct jn_store *s, const struct jn_msg *m, const unsigned char *id,
       struct jn_msg *r)
{
	sqlite3_stmt *q = NULL;
	int rc;
	struct jn_msg current = {0};
	if (s->hook)
		s->hook(s->hook_arg, 0);
	if (execute(s, "BEGIN IMMEDIATE"))
		return JN_IO;
	rc = object(s, id, &current);
	r->revision = current.revision;
	if (rc)
		goto fail;
	if (r->revision != m->revision) {
		rc = JN_CONFLICT;
		goto fail;
	}
	if (r->revision >= JN_REVISIONS) {
		rc = JN_LIMIT;
		goto fail;
	}
	r->length = 0;
	rc = JN_IO;
	if (prepare(s, "INSERT INTO revisions VALUES(?1,?2,?3)", &q))
		goto fail;
	if (sqlite3_bind_blob(q, 1, id, 16, SQLITE_TRANSIENT) != SQLITE_OK ||
	    sqlite3_bind_int64(q, 2, (sqlite3_int64)(m->revision + 1)) !=
	        SQLITE_OK ||
	    sqlite3_bind_blob(q, 3, m->data, (int)m->length,
	                      SQLITE_TRANSIENT) != SQLITE_OK ||
	    sqlite3_step(q) != SQLITE_DONE) {
		rc = finish(q, JN_IO);
		q = NULL;
		goto fail;
	}
	rc = finish(q, JN_OK);
	q = NULL;
	if (rc)
		goto fail;
	if (s->hook)
		s->hook(s->hook_arg, 1);
	rc = JN_IO;
	if (prepare(s,
	            "UPDATE objects SET current=?1 WHERE id=?2 AND current=?3",
	            &q))
		goto fail;
	if (sqlite3_bind_int64(q, 1, (sqlite3_int64)(m->revision + 1)) !=
	        SQLITE_OK ||
	    sqlite3_bind_blob(q, 2, id, 16, SQLITE_TRANSIENT) != SQLITE_OK ||
	    sqlite3_bind_int64(q, 3, (sqlite3_int64)m->revision) != SQLITE_OK ||
	    sqlite3_step(q) != SQLITE_DONE || sqlite3_changes(s->db) != 1) {
		rc = finish(q, JN_IO);
		q = NULL;
		goto fail;
	}
	rc = finish(q, JN_OK);
	q = NULL;
	if (rc)
		goto fail;
	if (s->hook)
		s->hook(s->hook_arg, 2);
	if (execute(s, "COMMIT")) {
		rc = JN_UNCERTAIN;
		goto fail;
	}
	if (s->hook)
		s->hook(s->hook_arg, 3);
	r->revision = m->revision + 1;
	memcpy(r->id, id, 16);
	return JN_OK;
fail:
	if (execute(s, "ROLLBACK"))
		return JN_UNCERTAIN;
	return rc;
}
int
jn_store_owner(struct jn_store *s, const struct jn_msg *m, struct jn_msg *r)
{
	sqlite3_stmt *q = NULL;
	uint64_t count = 0;
	int rc = JN_IO;
	memset(r, 0, sizeof(*r));
	r->op = m->op;
	if (!jn_request_valid(m, 1))
		return JN_INVALID;
	switch (m->op) {
	case JN_CREATE:
		return create_object(s, m, r);
	case JN_INSPECT:
		return object(s, m->id, r);
	case JN_WORLD:
		if (prepare(s,
		            "INSERT INTO worlds VALUES(?1,'janus-note',0) ON "
		            "CONFLICT(id) DO NOTHING",
		            &q))
			return rc;
		if (sqlite3_bind_int64(q, 1, (sqlite3_int64)m->arg) !=
		        SQLITE_OK ||
		    sqlite3_step(q) != SQLITE_DONE)
			return finish(q, rc);
		return finish(q, sqlite3_changes(s->db) == 1 ? JN_OK
		                                             : JN_CONFLICT);
	case JN_GRANT:
		if (m->rights & JN_DELEGABLE)
			return JN_UNSUPPORTED;
		if (prepare(s,
		            "SELECT count(*) FROM grants WHERE world=?1 AND "
		            "object!=?2",
		            &q))
			return rc;
		if (sqlite3_bind_int64(q, 1, (sqlite3_int64)m->arg) !=
		        SQLITE_OK ||
		    sqlite3_bind_blob(q, 2, m->id, 16, SQLITE_TRANSIENT) !=
		        SQLITE_OK ||
		    sqlite3_step(q) != SQLITE_ROW)
			return finish(q, rc);
		count = (uint64_t)sqlite3_column_int64(q, 0);
		if (sqlite3_step(q) != SQLITE_DONE)
			return finish(q, rc);
		rc = finish(q, JN_OK);
		q = NULL;
		if (rc)
			return rc;
		if (count >= JN_HANDLES)
			return JN_LIMIT;
		if (prepare(s,
		            "INSERT INTO grants VALUES(?1,?2,?3,0,1,1,'OWNER') "
		            "ON CONFLICT(world,object) DO UPDATE SET "
		            "rights=excluded.rights,live=1,generation="
		            "generation+1 WHERE generation<9223372036854775807",
		            &q))
			return JN_IO;
		if (sqlite3_bind_int64(q, 1, (sqlite3_int64)m->arg) !=
		        SQLITE_OK ||
		    sqlite3_bind_blob(q, 2, m->id, 16, SQLITE_TRANSIENT) !=
		        SQLITE_OK ||
		    sqlite3_bind_int(q, 3, (int)m->rights) != SQLITE_OK ||
		    sqlite3_step(q) != SQLITE_DONE)
			return finish(q, JN_IO);
		return finish(q,
		              sqlite3_changes(s->db) == 1 ? JN_OK : JN_LIMIT);
	case JN_REVOKE:
		if (prepare(s,
		            "UPDATE grants SET live=0,generation=CASE WHEN "
		            "generation<9223372036854775807 THEN generation+1 "
		            "ELSE generation END WHERE world=?1 AND object=?2",
		            &q))
			return rc;
		if (sqlite3_bind_int64(q, 1, (sqlite3_int64)m->arg) !=
		        SQLITE_OK ||
		    sqlite3_bind_blob(q, 2, m->id, 16, SQLITE_TRANSIENT) !=
		        SQLITE_OK ||
		    sqlite3_step(q) != SQLITE_DONE)
			return finish(q, rc);
		return finish(q,
		              sqlite3_changes(s->db) == 1 ? JN_OK : JN_DENIED);
	case JN_SAVE:
		if (number(s, "SELECT count(*) FROM activities", &count))
			return rc;
		if (count >= 8)
			return JN_LIMIT;
		if (jn_random(r->id, 16) || jn_zero(r->id, 16))
			return rc;
		if (prepare(s, "INSERT INTO activities VALUES(?1,?2,?3,?4)",
		            &q))
			return rc;
		if (sqlite3_bind_blob(q, 1, r->id, 16, SQLITE_TRANSIENT) !=
		        SQLITE_OK ||
		    sqlite3_bind_int64(q, 2, (sqlite3_int64)m->arg) !=
		        SQLITE_OK ||
		    sqlite3_bind_blob(q, 3, m->id, 16, SQLITE_TRANSIENT) !=
		        SQLITE_OK ||
		    sqlite3_bind_int64(q, 4, (sqlite3_int64)m->revision) !=
		        SQLITE_OK ||
		    sqlite3_step(q) != SQLITE_DONE)
			return finish(q, rc);
		return finish(q, JN_OK);
	default:
		return JN_UNSUPPORTED;
	}
}
int
jn_store_launch(struct jn_store *s, const struct jn_msg *m, struct jn_msg *r,
                struct jn_session *session)
{
	sqlite3_stmt *q = NULL;
	struct jn_session next = {0};
	int rc = JN_IO;
	memset(r, 0, sizeof(*r));
	r->op = m->op;
	if (m->op != JN_LAUNCH || !jn_request_valid(m, 1))
		return JN_INVALID;
	if (!jn_zero(m->id, 16)) {
		if (prepare(s,
		            "SELECT object,rev FROM activities WHERE id=?1 AND "
		            "world=?2",
		            &q))
			return rc;
		if (sqlite3_bind_blob(q, 1, m->id, 16, SQLITE_TRANSIENT) !=
		        SQLITE_OK ||
		    sqlite3_bind_int64(q, 2, (sqlite3_int64)m->arg) !=
		        SQLITE_OK)
			return finish(q, rc);
		int step = sqlite3_step(q);
		if (step == SQLITE_DONE)
			return finish(q, JN_DENIED);
		if (step != SQLITE_ROW ||
		    sqlite3_column_type(q, 0) != SQLITE_BLOB ||
		    sqlite3_column_bytes(q, 0) != 16 ||
		    sqlite3_column_type(q, 1) != SQLITE_INTEGER ||
		    sqlite3_column_int64(q, 1) < 1 ||
		    sqlite3_column_int64(q, 1) > JN_REVISIONS)
			return finish(q, rc);
		const void *id = sqlite3_column_blob(q, 0);
		if (!id)
			return finish(q, rc);
		memcpy(r->id, id, 16);
		r->revision = (uint64_t)sqlite3_column_int64(q, 1);
		if (sqlite3_step(q) != SQLITE_DONE)
			return finish(q, rc);
		rc = finish(q, JN_OK);
		q = NULL;
		if (rc)
			return rc;
	}
	if (prepare(
	        s,
	        "UPDATE worlds SET inc=inc+1 WHERE id=?1 AND app='janus-note' "
	        "AND inc<9223372036854775807 RETURNING inc",
	        &q))
		return JN_IO;
	if (sqlite3_bind_int64(q, 1, (sqlite3_int64)m->arg) != SQLITE_OK)
		return finish(q, JN_IO);
	int step = sqlite3_step(q);
	if (step == SQLITE_DONE)
		return finish(q, JN_LIMIT);
	if (step != SQLITE_ROW || sqlite3_column_type(q, 0) != SQLITE_INTEGER ||
	    sqlite3_column_int64(q, 0) < 1)
		return finish(q, JN_IO);
	next.world = m->arg;
	next.incarnation = (uint64_t)sqlite3_column_int64(q, 0);
	if (sqlite3_step(q) != SQLITE_DONE)
		return finish(q, JN_IO);
	rc = finish(q, JN_OK);
	if (rc)
		return rc;
	*session = next;
	r->arg = next.incarnation;
	return JN_OK;
}
int
jn_world_request(struct jn_store *s, struct jn_session *session,
                 const struct jn_msg *m, struct jn_msg *r)
{
	uint32_t rights = 0;
	uint64_t generation = 0;
	int rc;
	memset(r, 0, sizeof(*r));
	r->op = m->op;
	if (!jn_request_valid(m, 0))
		return JN_INVALID;
	if (!session->world || session->world > JN_WORLDS ||
	    !session->incarnation || session->used > JN_HANDLES)
		return JN_STALE;
	sqlite3_stmt *check = NULL;
	if (prepare(s,
	            "SELECT inc FROM worlds WHERE id=?1 AND app='janus-note'",
	            &check))
		return JN_IO;
	if (sqlite3_bind_int64(check, 1, (sqlite3_int64)session->world) !=
	    SQLITE_OK)
		return finish(check, JN_IO);
	int step = sqlite3_step(check);
	if (step != SQLITE_ROW ||
	    sqlite3_column_type(check, 0) != SQLITE_INTEGER ||
	    sqlite3_column_int64(check, 0) < 1 ||
	    (uint64_t)sqlite3_column_int64(check, 0) != session->incarnation)
		return finish(check, JN_STALE);
	if (sqlite3_step(check) != SQLITE_DONE)
		return finish(check, JN_IO);
	if (finish(check, JN_OK))
		return JN_IO;
	if (m->op == JN_DELEGATE)
		return JN_UNSUPPORTED;
	if (m->op == JN_SESSION) {
		r->arg = session->incarnation;
		return JN_OK;
	}
	if (m->op == JN_ACQUIRE) {
		rc = permission(s, session->world, m->id, &rights, &generation);
		if (rc)
			return rc;
		if (session->used >= JN_HANDLES)
			return JN_LIMIT;
		struct jn_handle h = {0};
		if (jn_random(h.value, 16) || jn_zero(h.value, 16))
			return JN_IO;
		for (size_t i = 0; i < session->used; i++)
			if (!memcmp(session->handles[i].value, h.value, 16))
				return JN_IO;
		memcpy(h.object, m->id, 16);
		h.generation = generation;
		session->handles[session->used++] = h;
		memcpy(r->handle, h.value, 16);
		r->rights = rights;
		return JN_OK;
	}
	struct jn_handle *h = NULL;
	for (size_t i = 0; i < session->used; i++)
		if (!memcmp(session->handles[i].value, m->handle, 16)) {
			h = &session->handles[i];
			break;
		}
	if (!h)
		return JN_STALE;
	rc = permission(s, session->world, h->object, &rights, &generation);
	if (rc)
		return rc;
	if (generation != h->generation)
		return JN_STALE;
	if (m->op == JN_GET) {
		if (!(rights & JN_READ))
			return JN_DENIED;
		return object(s, h->object, r);
	}
	if (m->op == JN_PUT) {
		if (!(rights & JN_WRITE))
			return JN_DENIED;
		return update(s, m, h->object, r);
	}
	return JN_INVALID;
}
