#!/usr/bin/env python3
# SPDX-License-Identifier: ISC
"""Run only a fresh child QEMU; no network, disks, shared files or hardware."""
import argparse
from pathlib import Path
import selectors
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--log', required=True, type=Path)
    args = parser.parse_args()
    args.log.parent.mkdir(parents=True, exist_ok=True)
    argv = ['qemu-system-x86_64', '-accel', 'tcg', '-smp', '1', '-cpu',
            'qemu64,+fsgsbase,+pdpe1gb,+xsaveopt,+xsave', '-m', '1G',
            '-display', 'none', '-monitor', 'none', '-serial', 'stdio',
            '-nic', 'none', '-no-reboot', '-kernel', 'out/m3-release/sel4_32.elf',
            '-initrd', 'out/m3-release/loader.img']
    print('+', ' '.join(argv), flush=True)
    data = bytearray()
    with args.log.open('xb') as log, selectors.DefaultSelector() as selector:
        child = subprocess.Popen(argv, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT, close_fds=True)
        try:
            selector.register(child.stdout, selectors.EVENT_READ)
            deadline = time.monotonic() + 120
            while time.monotonic() < deadline and len(data) < 1024 * 1024:
                events = selector.select(1)
                if events:
                    chunk = child.stdout.read1(min(8192, 1024 * 1024 - len(data)))
                    if not chunk:
                        break
                    data.extend(chunk)
                    log.write(chunk)
                    log.flush()
                    print(chunk.decode(errors='replace'), end='', flush=True)
                    if b'M3 COMPLETE PASS\n' in data or b'M3 FAIL' in data:
                        break
                elif child.poll() is not None:
                    break
        finally:
            if child.poll() is None:
                child.terminate()
                try:
                    child.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    child.kill()
                    child.wait()
            child.stdout.close()
            print(f'QEMU child exit={child.returncode} (harness termination after marker/timeout)', flush=True)
    required = [f'M3 PASS {letter}\n'.encode() for letter in 'ABCDEFGHIJKL']
    required += [b'M3 COMPLETE PASS\n', b'M3 PASS caller-sc control-independent',
		 b'M3 PASS denied-owner quota\n',
                 b'M3 FAULT child=1 unmapped=0x40000000']
    if b'M3 FAIL' in data or not all(marker in data for marker in required):
        raise SystemExit('FAIL: missing scenario/fault/accounting milestone')
    print('PASS: release serial scenarios A-L, fault and requester scheduling context')


if __name__ == '__main__':
    main()
