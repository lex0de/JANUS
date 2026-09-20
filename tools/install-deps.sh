#!/bin/sh
# JANUS: tools/install-deps.sh
# Default: simulation. Explicit --apply: reviewed missing packages only.
# See docs/development/HOST.md and LICENSING.md.
set -eu
LC_ALL=C
export LC_ALL

usage()
{
	printf '%s\n' 'usage: install-deps.sh [--apply | --help]'
	printf '%s\n' 'No argument: simulate missing Debian 13 development packages.'
}

apply=no
case $# in
0) ;;
1)
	case $1 in
	--apply) apply=yes ;;
	--help) usage; exit 0 ;;
	*) usage >&2; exit 2 ;;
	esac
	;;
*) usage >&2; exit 2 ;;
esac

if [ ! -r /etc/os-release ]; then
	printf 'BLOCKED: cannot identify development host\n' >&2
	exit 1
fi
os=$(awk -F= '$1 == "ID" { gsub(/"/, "", $2); print $2 }' /etc/os-release)
version=$(awk -F= '$1 == "VERSION_ID" { gsub(/"/, "", $2); print $2 }' /etc/os-release)
if [ "$os" != debian ]; then
	printf 'BLOCKED: expected Debian 13, found %s %s\n' "$os" "$version" >&2
	exit 1
fi
case "$version" in
13|13.*) ;;
*)
	printf 'BLOCKED: expected Debian 13, found %s\n' "$version" >&2
	exit 1
	;;
esac

script_dir=$(CDPATH='' cd -P "$(dirname "$0")" && pwd)
manifest=$script_dir/debian13-packages.txt
if [ ! -r "$manifest" ]; then
	printf 'BLOCKED: missing package manifest\n' >&2
	exit 1
fi
for command in dpkg-query apt-get; do
	if ! command -v "$command" >/dev/null 2>&1; then
		printf 'BLOCKED: missing required tool: %s\n' "$command" >&2
		exit 1
	fi
done

# Build argv from a fixed, validated manifest. Never eval shell configuration.
set --
while IFS= read -r package || [ -n "$package" ]; do
	case $package in
	''|'#'*) continue ;;
	*[!a-z0-9+.-]*|-*|.*|+*)
		printf 'BLOCKED: invalid package name in manifest\n' >&2
		exit 1
		;;
	esac
	status=$(dpkg-query -W -f='${Status}' "$package" 2>/dev/null) || status=
	if [ "$status" != 'install ok installed' ]; then
		set -- "$@" "$package"
	fi
done < "$manifest"

if [ "$#" -eq 0 ]; then
	printf 'PASS: listed development packages are already installed\n'
	exit 0
fi
printf 'Missing package candidates:\n'
printf '  %s\n' "$@"
printf '\nSimulation; review changes before applying:\n'
apt-get --simulate --no-remove --no-install-recommends install "$@"
if [ "$apply" != yes ]; then
	printf '\nNo packages changed. Review, then use --apply if authorised.\n'
	exit 0
fi

printf '\nApplying only the listed missing packages; apt will ask for confirmation.\n'
printf 'No -y, apt-source edits, automatic upgrade, or autoremove.\n'
if [ "$(id -u)" -eq 0 ]; then
	apt-get --no-remove --no-install-recommends install "$@"
elif command -v sudo >/dev/null 2>&1; then
	sudo apt-get --no-remove --no-install-recommends install "$@"
elif command -v doas >/dev/null 2>&1; then
	doas apt-get --no-remove --no-install-recommends install "$@"
else
	printf 'BLOCKED: neither sudo nor doas is available\n' >&2
	exit 1
fi
printf 'PASS: apt transaction returned success; record installed versions.\n'
