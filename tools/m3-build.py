#!/usr/bin/env python3
# SPDX-License-Identifier: ISC
"""Explicit, offline, signature-checked M3 target build. No implicit downloads."""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import tarfile
import tempfile

FINGERPRINT = 'FE91486443B0F4EB9ECC36524D868A34EDF3FDCA'
ARCHIVE_HASH = 'b1b285b8785db7557eba6d0665dd4fbf2f265ea5c0b39d6f91ae3dcc43d2c424'


def run(argv):
    print('+', ' '.join(map(str, argv)), flush=True)
    subprocess.run(list(map(str, argv)), check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', required=True, type=Path)
    parser.add_argument('--archive', required=True, type=Path)
    parser.add_argument('--signature', required=True, type=Path)
    parser.add_argument('--key', required=True, type=Path)
    parser.add_argument('--cc', default='gcc', choices=['gcc', 'clang'])
    args = parser.parse_args()
    sdk = args.sdk.resolve()
    if hashlib.sha256(args.archive.read_bytes()).hexdigest() != ARCHIVE_HASH:
        raise SystemExit('SDK archive SHA-256 mismatch')
    with tempfile.TemporaryDirectory(prefix='janus-m3-gpg-') as home:
        os.chmod(home, 0o700)
        gpg = ['gpg', '--homedir', home, '--batch']
        keys = subprocess.check_output(gpg + ['--with-colons', '--show-keys', str(args.key)], text=True)
        fingerprints = [line.split(':')[9] for line in keys.splitlines() if line.startswith('fpr:')]
        if not fingerprints or fingerprints[0] != FINGERPRINT:
            raise SystemExit('SDK signing key fingerprint mismatch')
        run(gpg + ['--import', args.key])
        status = subprocess.check_output(gpg + ['--status-fd', '1', '--verify', str(args.signature), str(args.archive)], text=True)
        if '[GNUPG:] VALIDSIG ' + FINGERPRINT + ' ' not in status:
            raise SystemExit('Expected SDK signature absent')
        print(status, flush=True)
    # Compare the installed build inputs against the signed archive, not a marker.
    verified = set()
    with tarfile.open(args.archive, 'r:gz') as archive:
        for member in archive:
            parts = Path(member.name).parts
            if len(parts) < 2:
                continue
            relative = Path(*parts[1:])
            if not (str(relative).startswith('board/x86_64_generic/release/') or
                    str(relative) in ('bin/microkit', 'VERSION')):
                continue
            if member.isdir():
                continue
            local = sdk / relative
            if not member.isfile() or local.is_symlink() or not local.is_file():
                raise SystemExit(f'Unexpected SDK input type: {relative}')
            stream = archive.extractfile(member)
            if stream is None or hashlib.sha256(stream.read()).digest() != hashlib.sha256(local.read_bytes()).digest():
                raise SystemExit(f'Extracted SDK input differs: {relative}')
            verified.add(relative)
    for directory in ('board/x86_64_generic/release',):
        for local in (sdk / directory).rglob('*'):
            if local.is_file() and local.relative_to(sdk) not in verified:
                raise SystemExit(f'Extra SDK input: {local}')
    if Path('bin/microkit') not in verified or Path('VERSION') not in verified:
        raise SystemExit('SDK inputs missing')
    print(f'PASS signed SDK inputs: {len(verified)} files', flush=True)
    board = sdk / 'board/x86_64_generic/release'
    output = Path('out/m3-release')
    output.mkdir(parents=True, exist_ok=True)
    # SDK libseL4 headers require GNU asm syntax; portable logic uses strict C17.
    flags = ['-std=gnu17', '-Wall', '-Wextra', '-Werror', '-Wconversion', '-Wshadow',
             '-Wstrict-prototypes', '-Wmissing-prototypes', '-O2', '-g',
             '-ffreestanding', '-nostdlib', '-fno-stack-protector', '-fno-pie',
             '-mno-red-zone', '-march=x86-64', '-mtune=generic',
             '-isystem', str(board / 'include'), '-Iinclude', '-Iplatform/microkit']
    for name in ('control', 'object', 'world', 'attacker'):
        sources = [Path(f'platform/microkit/{name}.c')]
        if name == 'object':
            sources.append(Path('lib/substrate/model.c'))
        objects = []
        for source in sources:
            obj = output / f'{name}-{source.stem}.o'
            run([args.cc, *flags, '-c', source, '-o', obj])
            objects.append(obj)
        run(['ld', '-L', board / 'lib', *objects, '-lmicrokit', '-Tmicrokit.ld', '-o', output / f'{name}.elf'])
    run([sdk / 'bin/microkit', 'platform/microkit/janus.system', '--search-path', output,
         '--board', 'x86_64_generic', '--config', 'release', '-o', output / 'loader.img',
         '-r', output / 'report.txt', '--capdl-json', output / 'capdl.json'])
    run(['python3', 'tools/m3-topology.py'])
    run(['size', *(output / f'{name}.elf' for name in ('control', 'object', 'world', 'attacker'))])


if __name__ == '__main__':
    main()
