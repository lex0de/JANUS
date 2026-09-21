/* SPDX-License-Identifier: ISC */
/* Copyright (c) 2026 Danyal A. Samak. */
#define _GNU_SOURCE
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/landlock.h>
#include <linux/seccomp.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>
#include "janus/native/native.h"
int
jn_protect(void)
{
	return prctl(PR_SET_DUMPABLE, 0, 0, 0, 0) < 0 ? -1 : 0;
}
int
jn_landlock_abi(void)
{
	long n = syscall(SYS_landlock_create_ruleset, NULL, 0,
	                 LANDLOCK_CREATE_RULESET_VERSION);
	return n < 0 || n > 1000 ? -1 : (int)n;
}
int
jn_sandbox(int executable)
{
#if !defined(__x86_64__)
	(void)executable;
	return -1;
#else
	if (jn_landlock_abi() < 6)
		return -1;
	struct landlock_ruleset_attr rules = {0};
	rules.handled_access_fs = (UINT64_C(1) << 16) - 1;
	rules.handled_access_net =
	    LANDLOCK_ACCESS_NET_BIND_TCP | LANDLOCK_ACCESS_NET_CONNECT_TCP;
	rules.scoped =
	    LANDLOCK_SCOPE_ABSTRACT_UNIX_SOCKET | LANDLOCK_SCOPE_SIGNAL;
	int fd =
	    (int)syscall(SYS_landlock_create_ruleset, &rules, sizeof(rules), 0);
	if (fd < 0)
		return -1;
	struct landlock_path_beneath_attr path = {
	    .allowed_access =
	        LANDLOCK_ACCESS_FS_EXECUTE | LANDLOCK_ACCESS_FS_READ_FILE,
	    .parent_fd = executable};
	int rc = -1;
	if (syscall(SYS_landlock_add_rule, fd, LANDLOCK_RULE_PATH_BENEATH,
	            &path, 0) < 0 ||
	    prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) < 0 ||
	    syscall(SYS_landlock_restrict_self, fd, 0) < 0)
		goto done;
	/* Default-deny syscall surface; openat remains for explicit Landlock
	 * tests. No socket/connect, process inspection or
	 * filesystem mutation outside Landlock's handled actions. x32 is not
	 * accepted. */
#define ALLOW(n)                                                               \
	BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, (n), 0, 1),                        \
	    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW)
	struct sock_filter filter[] = {
	    BPF_STMT(BPF_LD | BPF_W | BPF_ABS,
	             offsetof(struct seccomp_data, arch)),
	    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, AUDIT_ARCH_X86_64, 1, 0),
	    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),
	    BPF_STMT(BPF_LD | BPF_W | BPF_ABS,
	             offsetof(struct seccomp_data, nr)),
	    /* prlimit64 may inspect only this process, never another
	       world/service. */
	    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_prlimit64, 0, 4),
	    BPF_STMT(BPF_LD | BPF_W | BPF_ABS,
	             offsetof(struct seccomp_data, args[0])),
	    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0, 0, 1),
	    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
	    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | (unsigned int)EPERM),
	    ALLOW(SYS_read), ALLOW(SYS_write), ALLOW(SYS_close),
	    ALLOW(SYS_fstat), ALLOW(SYS_newfstatat), ALLOW(SYS_lseek),
	    ALLOW(SYS_mmap), ALLOW(SYS_mprotect), ALLOW(SYS_munmap),
	    ALLOW(SYS_brk), ALLOW(SYS_rt_sigaction), ALLOW(SYS_rt_sigprocmask),
	    ALLOW(SYS_rt_sigreturn), ALLOW(SYS_arch_prctl),
	    ALLOW(SYS_set_tid_address), ALLOW(SYS_set_robust_list),
	    ALLOW(SYS_rseq),

	    ALLOW(SYS_getrandom), ALLOW(SYS_getpid), ALLOW(SYS_getppid),
	    ALLOW(SYS_poll), ALLOW(SYS_clock_gettime), ALLOW(SYS_futex),
	    ALLOW(SYS_exit), ALLOW(SYS_exit_group), ALLOW(SYS_openat),
	    ALLOW(SYS_readlink), ALLOW(SYS_readlinkat), ALLOW(SYS_access),
	    ALLOW(SYS_recvfrom), ALLOW(SYS_sendto), ALLOW(SYS_recvmsg),
	    ALLOW(SYS_sendmsg), ALLOW(SYS_execveat),
	    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | (unsigned int)EPERM)};
#undef ALLOW
	struct sock_fprog program = {
	    .len = (unsigned short)(sizeof(filter) / sizeof(filter[0])),
	    .filter = filter};
	if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &program) < 0)
		goto done;
	rc = 0;
done:
	if (close(fd) < 0)
		rc = -1;
	return rc;
#endif
}
