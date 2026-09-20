#!/bin/sh
# JANUS: tools/host-preflight.sh
# Read-only inventory. Keep raw output private. See LICENSING.md.
set -u
LC_ALL=C
export LC_ALL

usage()
{
	printf '%s\n' 'usage: host-preflight.sh [--no-libvirt | --help]'
}

use_libvirt=yes
case $# in
0) ;;
1)
	case $1 in
	--help) usage; exit 0 ;;
	--no-libvirt) use_libvirt=no ;;
	*) usage >&2; exit 2 ;;
	esac
	;;
*) usage >&2; exit 2 ;;
esac

run()
{
	label=$1
	shift
	printf '\n## %s\n' "$label"
	if ! command -v "$1" >/dev/null 2>&1; then
		printf 'NOT RUN: missing command: %s\n' "$1"
		return
	fi
	if ! command -v timeout >/dev/null 2>&1; then
		printf 'NOT RUN: timeout unavailable; no unbounded probe\n'
		return
	fi
	if timeout 15 "$@" </dev/null; then
		printf 'PASS: query completed (not a readiness verdict)\n'
	else
		rc=$?
		printf 'BLOCKED: query exit %s; inspect diagnostics\n' "$rc"
	fi
}

printf '# JANUS host inventory\n'
printf 'Inventory only. No nested guest boot or hardware isolation test.\n'
run 'UTC time' date -u '+%Y-%m-%dT%H:%M:%SZ'
run 'Hostname' hostname
run 'Operating system' cat /etc/os-release
run 'Kernel and architecture' uname -srmo
run 'Current identity (private)' id
run 'Working directory (private)' pwd -P
run 'Execution layer (exit 1 can mean no virtualisation detected)' systemd-detect-virt
run 'Memory' free -m
run 'Workspace filesystem space (private)' df -Pk .

printf '\n## CPU virtualisation indicators\n'
if [ -r /proc/cpuinfo ]; then
	awk '
	/^model name[[:space:]]*:/ && !model { print; model = 1 }
	/^flags[[:space:]]*:/ && !flags {
		for (i = 3; i <= NF; i++)
			if ($i ~ /^(vmx|svm|ept|npt)$/) print $i
		flags = 1
	}' /proc/cpuinfo
else
	printf 'NOT RUN: /proc/cpuinfo unavailable\n'
fi

printf '\n## KVM access indicators\n'
if [ -c /dev/kvm ]; then
	printf '/dev/kvm exists as a character device\n'
	if [ -r /dev/kvm ] && [ -w /dev/kvm ]; then
		printf 'Current user has apparent read/write access; ioctl/boot NOT RUN\n'
	else
		printf 'BLOCKED: current user lacks apparent read/write access\n'
	fi
else
	printf 'BLOCKED: no /dev/kvm character device\n'
fi
for vendor in intel amd; do
	path=/sys/module/kvm_${vendor}/parameters/nested
	if [ -r "$path" ]; then
		printf '%s: ' "$path"
		cat "$path"
	fi
done
printf 'NOT RUN: actual L1-to-L2 KVM boot\n'

printf '\n## IOMMU topology indicator\n'
count=0
for group in /sys/kernel/iommu_groups/[0-9]*; do
	if [ -d "$group" ]; then
		count=$((count + 1))
	fi
done
printf 'Visible IOMMU group directories: %s\n' "$count"
printf 'NOT RUN: DMA/interrupt isolation or device reset validation\n'

for tool in gcc g++ clang clang++ make python3 git gh qemu-system-x86_64 \
    qemu-img virsh virt-viewer shellcheck clang-format; do
	run "$tool version" "$tool" --version
done

if [ "$use_libvirt" = yes ]; then
	for uri in qemu:///session qemu:///system; do
		run "Read-only domain list: $uri (private)" \
		    virsh --readonly --connect "$uri" list --all
	done
else
	printf '\nNOT RUN: libvirt queries omitted by request\n'
fi
printf '\nNOT RUN: GitHub authentication/repository creation\n'
printf 'Inventory finished. Review every blocked query before host setup.\n'
