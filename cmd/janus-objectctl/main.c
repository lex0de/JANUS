/* SPDX-License-Identifier: ISC */
/* Copyright (c) 2026 Danyal A. Samak. */
#include <stdio.h>
#include <string.h>
#include "janus/native/native.h"
int
main(int argc, char **argv)
{
	struct jn_msg q = {0}, r = {0};
	if (argc < 3)
		goto usage;
	if (!strcmp(argv[2], "create") && argc == 4) {
		q.op = JN_CREATE;
		q.length = strlen(argv[3]);
		if (q.length > JN_CONTENT)
			goto usage;
		memcpy(q.data, argv[3], q.length);
	} else if ((!strcmp(argv[2], "world") || !strcmp(argv[2], "stop")) &&
	           argc == 4) {
		q.op = !strcmp(argv[2], "world") ? JN_WORLD : JN_STOP;
		if (jn_number(argv[3], &q.arg))
			goto usage;
	} else if (!strcmp(argv[2], "inspect") && argc == 4) {
		q.op = JN_INSPECT;
		if (jn_unhex(argv[3], q.id))
			goto usage;
	} else if ((!strcmp(argv[2], "grant") || !strcmp(argv[2], "revoke") ||
	            !strcmp(argv[2], "save")) &&
	           argc >= 5) {
		if (jn_number(argv[3], &q.arg) || jn_unhex(argv[4], q.id))
			goto usage;
		if (!strcmp(argv[2], "grant") && argc == 6) {
			q.op = JN_GRANT;
			if (!strcmp(argv[5], "r"))
				q.rights = 1;
			else if (!strcmp(argv[5], "w"))
				q.rights = 2;
			else if (!strcmp(argv[5], "rw"))
				q.rights = 3;
			else if (!strcmp(argv[5], "delegate"))
				q.rights = 5;
			else
				goto usage;
		} else if (!strcmp(argv[2], "revoke") && argc == 5)
			q.op = JN_REVOKE;
		else if (!strcmp(argv[2], "save") && argc == 6) {
			q.op = JN_SAVE;
			if (jn_number(argv[5], &q.revision))
				goto usage;
		} else
			goto usage;
	} else
		goto usage;
	if (!jn_request_valid(&q, 1))
		goto usage;
	int rc = jn_owner_call(argv[1], &q, &r, NULL);
	jn_print(&r, rc);
	return rc == JN_OK ? 0 : 1;
usage:
	fprintf(
	    stderr,
	    "usage: janus-objectctl runtime {create text|world N|grant N id "
	    "r/w/rw|revoke N id|save N id revision|inspect id|stop N}\n");
	return 2;
}
