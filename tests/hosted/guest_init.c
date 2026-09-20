/* SPDX-License-Identifier: ISC */
/* JANUS disposable boot probe; Copyright (c) 2026 Danyal A. Samak. */
#define _GNU_SOURCE
#include <sys/mount.h>
#include <sys/reboot.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int
main(void)
{
	char line[32];
	size_t n = 0;
	if (getpid() != 1)
		return 1;
	if (mount("devtmpfs", "/dev", "devtmpfs", 0, NULL) < 0)
		return 1;
	puts("JANUS_M1_BOOT");
	fflush(stdout);
	for (;;) {
		char c;
		ssize_t k = read(STDIN_FILENO, &c, 1);
		if (k < 0 && errno == EINTR)
			continue;
		if (k <= 0) {
			sleep(1);
			continue;
		}
		if (c == '\n' || c == '\r') {
			line[n] = '\0';
			if (!strcmp(line, "reboot")) {
				puts("JANUS_M1_REBOOT");
				fflush(stdout);
				if (reboot(RB_AUTOBOOT) < 0)
					perror("reboot");
			}
			if (!strcmp(line, "poweroff")) {
				puts("JANUS_M1_POWEROFF");
				fflush(stdout);
				if (reboot(RB_POWER_OFF) < 0)
					perror("poweroff");
			}
			n = 0;
		} else if (n < sizeof(line) - 1)
			line[n++] = c;
		else
			n = 0;
	}
}
