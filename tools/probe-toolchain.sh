#!/bin/sh
# JANUS: tools/probe-toolchain.sh
# Hosted compiler/runtime probes only. See LICENSING.md.
set -u
LC_ALL=C
export LC_ALL

usage()
{
	printf '%s\n' 'usage: probe-toolchain.sh [--plain | --sanitizers | --help]'
	printf '%s\n' 'Default: GCC/Clang C17/C++17, plain and ASan/UBSan probes.'
}

modes='plain sanitizers'
case $# in
0) ;;
1)
	case $1 in
	--plain) modes=plain ;;
	--sanitizers) modes=sanitizers ;;
	--help) usage; exit 0 ;;
	*) usage >&2; exit 2 ;;
	esac
	;;
*) usage >&2; exit 2 ;;
esac

script_dir=$(CDPATH='' cd -P "$(dirname "$0")" && pwd) || exit 1
root=$(CDPATH='' cd -P "$script_dir/.." && pwd) || exit 1
work=$(mktemp -d "${TMPDIR:-/tmp}/janus-toolchain.XXXXXXXX") || exit 1
cleanup()
{
	# Only the unique directory created by this invocation is eligible.
	case "$work" in
	*/janus-toolchain.????????) rm -rf -- "$work" ;;
	*) printf 'BLOCKED: unexpected temporary path; not deleting\n' >&2 ;;
	esac
}
trap 'cleanup' EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

if ! command -v timeout >/dev/null 2>&1; then
	printf 'BLOCKED: timeout is required for bounded compiler probes\n' >&2
	exit 1
fi

failed=0
for compiler in gcc clang g++ clang++; do
	if ! command -v "$compiler" >/dev/null 2>&1; then
		printf 'BLOCKED: missing compiler %s\n' "$compiler"
		failed=1
		continue
	fi
	case "$compiler" in
	gcc|clang) source=$root/tests/toolchain/smoke.c; standard=c17 ;;
	*) source=$root/tests/toolchain/smoke.cc; standard=c++17 ;;
	esac
	for mode in $modes; do
		binary=$work/$compiler-$mode
		set -- -std="$standard" -Wall -Wextra -Wpedantic -Werror -g -O1
		case "$standard" in
		c17) set -- "$@" -Wstrict-prototypes -Wmissing-prototypes ;;
		esac
		if [ "$mode" = sanitizers ]; then
			set -- "$@" -fno-omit-frame-pointer -fsanitize=address,undefined \
			    -fno-sanitize-recover=all
		fi
		printf '\nPROBE: %s %s %s\n' "$compiler" "$standard" "$mode"
		printf 'compiler argv:'
		printf ' <%s>' "$compiler" "$@" "$source" -o "$binary"
		printf '\n'
		if timeout 60 "$compiler" "$@" "$source" -o "$binary"; then
			if ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
			    UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
			    timeout 15 "$binary"; then
				printf 'PASS: %s %s compile and execute\n' "$compiler" "$mode"
			else
				rc=$?
				printf 'FAIL: %s %s execution exit %s\n' "$compiler" "$mode" "$rc"
				failed=1
			fi
		else
			rc=$?
			printf 'FAIL: %s %s compile exit %s\n' "$compiler" "$mode" "$rc"
			failed=1
		fi
	done
done
printf '\nNOT RUN: JANUS runtime, contract models, guest boots and hardware.\n'
cleanup
trap - EXIT
exit "$failed"
