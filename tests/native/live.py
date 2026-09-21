#!/usr/bin/env python3
# SPDX-License-Identifier: ISC
# Copyright (c) 2026 Danyal A. Samak. Explicit disposable M2 enforcement tests.
import argparse
import array
import ctypes
import errno
import json
import os
import pathlib
import shutil
import signal
import socket
import sqlite3
import struct
import subprocess
import time

REPO = pathlib.Path(__file__).resolve().parents[2]
ZERO = bytes(16)
HEADER = struct.Struct('!4sBBBBIIQQ16s16s')


def packet(op, ident=ZERO, handle=ZERO, revision=0, world=0, rights=0, data=b''):
	return HEADER.pack(b'JAN2', 1, op, 0, 0, len(data), rights, revision, world, ident, handle) + data


def decode(data):
	assert len(data) >= 64
	magic, version, op, status, reserved, size, rights, revision, world, ident, handle = HEADER.unpack(data[:64])
	assert magic == b'JAN2' and version == 1 and not reserved and size == len(data) - 64
	return dict(op=op, status=status, rights=rights, revision=revision, world=world, ident=ident, handle=handle, data=data[64:])


def run(argv, expected=0):
	print('COMMAND', repr([str(v) for v in argv]), flush=True)
	p = subprocess.run(argv, capture_output=True, text=True, timeout=30)
	print('EXIT', p.returncode, p.stdout, p.stderr, flush=True)
	assert p.returncode == expected
	return p.stdout


def wait_for(test):
	end = time.monotonic() + 5
	while time.monotonic() < end:
		if test(): return
		time.sleep(.02)
	raise RuntimeError('condition timeout')


def main():
	parser = argparse.ArgumentParser()
	parser.add_argument('--fixture', required=True, type=pathlib.Path)
	parser.add_argument('--cc', choices=['gcc', 'clang'], default='gcc')
	args = parser.parse_args()
	root = args.fixture.absolute()
	assert root.parent == REPO / 'artifacts' and not root.exists()
	os.umask(0o077)
	root.mkdir(mode=0o700)
	shutil.copyfile(REPO / 'out/janus-note', root / 'note')
	(root / 'note').chmod(0o500)
	m1root = root / 'm1'
	m1root.mkdir(mode=0o700)
	(m1root / 'display').mkdir(mode=0o700)
	(m1root / 'profiles').write_text('unused qemu:///session 00000000-0000-4000-8000-000000000099\n')
	(m1root / 'profiles').chmod(0o600)
	service = broker = None
	logs = []
	connections = []
	results = {}
	libc = ctypes.CDLL(None, use_errno=True)
	# Independent runtime ABI query on this explicitly x86-64 laboratory.
	assert os.uname().machine == 'x86_64'
	abi = libc.syscall(444, 0, 0, 1)
	print('LANDLOCK ABI', abi, flush=True)
	assert abi >= 6
	def start():
		log = open(root / 'service.log', 'a'); logs.append(log)
		p = subprocess.Popen([str(REPO / 'out/janus-objectd'), str(root)], stdout=log, stderr=log)
		try:
			wait_for(lambda: (root / 'owner.sock').exists())
			time.sleep(.1)
			assert p.poll() is None
		except BaseException:
			if p.poll() is None: p.terminate(); p.wait(timeout=5)
			raise
		return p
	def owner(message, expected=0, credential=None):
		key = (root / 'owner.key').read_bytes() if credential is None else credential
		with socket.socket(socket.AF_UNIX, socket.SOCK_SEQPACKET) as s:
			s.settimeout(3); s.connect(str(root / 'owner.sock'))
			s.sendall(key + message)
			data, ancillary, flags, address = s.recvmsg(2048, socket.CMSG_SPACE(4))
			assert not flags
			r = decode(data); fd = None
			for level, kind, value in ancillary:
				assert level == socket.SOL_SOCKET and kind == socket.SCM_RIGHTS
				fds = array.array('i'); fds.frombytes(value); assert len(fds) == 1
				fd = socket.socket(fileno=fds[0]); fd.settimeout(3); connections.append(fd)
			assert r['status'] == expected, (r['op'], r['status'], expected)
			return r, fd
	def ctl(*values, expected=0):
		text = run([str(REPO / 'out/janus-objectctl'), str(root), *values], expected)
		return dict(x.split('=', 1) for x in text.split())
	def native(world, activity, *values, expected=0):
		# STOP is an owner operation at a quiescent launch boundary.
		owner(packet(39, world=world))
		return run([str(REPO / 'out/janus-run'), str(root), str(world), activity, *values], expected)
	def rpc(s, message, expected=0):
		s.sendall(message); r = decode(s.recv(2048)); assert r['status'] == expected, (r['status'], expected); return r
	try:
		service = start()
		assert (root / 'owner.key').stat().st_mode & 0o777 == 0o600
		assert (root / 'owner.sock').stat().st_mode & 0o777 == 0o600
		for w in range(1, 5): ctl('world', str(w))
		a = ctl('create', 'granted-document')['id']; b = ctl('create', 'ungranted-private')['id']
		ai, bi = bytes.fromhex(a), bytes.fromhex(b)
		ctl('grant', '1', a, 'rw'); ctl('grant', '2', a, 'r')
		# A real M1 owner listener, no viewer, guest query or VM modification.
		log = open(root / 'm1.log', 'w'); logs.append(log)
		broker = subprocess.Popen([str(REPO / 'out/janusd'), str(m1root / 'profiles'), str(m1root), str(m1root / 'display'), 'unused'], stdout=log, stderr=log)
		wait_for(lambda: (m1root / 'control').exists())
		with socket.socket(socket.AF_UNIX, socket.SOCK_SEQPACKET) as s:
			s.settimeout(3); s.connect(str(m1root / 'control')); s.sendall(b'bad\n'); assert s.recv(512) == b'invalid\n'
		text = native(1, '-', 'probe', a, b, str(root / 'store.db'), str(root / 'owner.key'), str(service.pid), str(m1root / 'control'), str(REPO / 'README.md'))
		assert 'status=OK op=2' in text and 'status=DENIED op=1' in text
		for label in ['store', 'owner-key', 'repository', 'service-memory']:
			assert f'probe={label} result=-1 errno={errno.EACCES}' in text
		for label in ['ptrace', 'process-vm', 'socket', 'owner-connect', 'service-prlimit']:
			assert f'probe={label} result=-1 errno={errno.EPERM}' in text
		assert f'probe=stdout-read result=-1 errno={errno.EBADF}' in text
		for fd in range(4, 16): assert f'probe=fd-{fd} result=-1 errno={errno.EBADF}' in text
		assert 'status=INVALID op=32' in text
		results['A'] = results['B'] = 'PASS'
		# Parent can normally ptrace its child under Yama: DUMPABLE=0 denies here.
		ctypes.set_errno(0)
		traced = libc.ptrace(16, service.pid, 0, 0)
		if traced == 0:
			os.waitpid(service.pid, os.WUNTRACED); libc.ptrace(17, service.pid, 0, 0)
		assert traced == -1 and ctypes.get_errno() == errno.EPERM
		for key in [b'', bytes(32), b'x' * 32]: owner(packet(32, data=b'unauthorised'), expected=2, credential=key)
		results['K'] = 'PASS'
		native(1, '-', 'write', a, '1', 'complete-revision')
		r = ctl('inspect', a); assert r['revision'] == '2' and bytes.fromhex(r['content']) == b'complete-revision'
		results['C'] = 'PASS'
		text = native(1, '-', 'conflict', a); assert 'status=CONFLICT op=3' in text
		r = ctl('inspect', a); assert r['revision'] == '3' and bytes.fromhex(r['content']) == b'first'
		results['D'] = 'PASS'
		text = native(2, '-', 'write', a, '3', 'forbidden', expected=1); assert 'status=DENIED op=3' in text
		assert ctl('inspect', a)['revision'] == '3'; results['E'] = 'PASS'
		text = native(1, '-', 'read', a)
		lines = [dict(item.split('=', 1) for item in line.split()) for line in text.splitlines() if line.startswith('status=')]
		old = next(item['handle'] for item in lines if item['op'] == '1')
		incarnation = int(lines[0]['incarnation'])
		text = native(1, '-', 'stale', a, old); assert 'status=STALE op=2' in text
		assert int(text.splitlines()[0].split('incarnation=')[1].split()[0]) > incarnation
		results['F'] = 'PASS'
		activity = ctl('save', '1', a, '1')['id']
		text = native(1, activity, 'activity'); assert 'status=OK op=2 revision=3' in text
		results['G'] = 'PASS'
		ctl('revoke', '1', a)
		text = native(1, activity, 'activity', expected=1); assert 'status=DENIED op=1' in text
		# Restore only old activity metadata, never grant rows, then restart service.
		service.terminate(); service.wait(timeout=5)
		with sqlite3.connect(root / 'store.db') as db:
			db.execute('UPDATE activities SET rev=? WHERE id=?', (1, bytes.fromhex(activity)))
			assert db.execute('SELECT live FROM grants WHERE world=1 AND object=?', (ai,)).fetchone() == (0,)
		service = start()
		text = native(1, activity, 'activity', expected=1); assert 'status=DENIED op=1' in text
		results['H'] = 'PASS'
		ctl('grant', '1', a, 'rw')
		text = native(1, '-', 'exhaust', a); assert 'status=LIMIT op=1' in text
		owner(packet(39, world=1))
		# Four internally bound live endpoints; old/cross-world handles denied.
		live = []
		for w in range(1, 5):
			owner(packet(39, world=w)); _, s = owner(packet(37, world=w)); live.append(s)
		owner(packet(37, world=1), expected=8)
		h = rpc(live[0], packet(1, ident=ai))['handle']
		rpc(live[1], packet(2, handle=h), expected=6)
		for unused in range(7): rpc(live[0], packet(1, ident=ai))
		rpc(live[0], packet(1, ident=ai), expected=4)
		# Queue pressure remains bounded, with owner inspection on its own slot.
		for s in live: s.setblocking(False)
		for s in live:
			for unused in range(256):
				try: s.send(packet(5))
				except (BlockingIOError, BrokenPipeError): break
		assert ctl('inspect', a)['revision'] == '3'
		for w in range(1, 5): owner(packet(39, world=w))
		for s in live: s.close()
		# Authenticated malformed owner frames must not change object count.
		with sqlite3.connect(root / 'store.db') as db: before = db.execute('SELECT count(*) FROM objects').fetchone()
		for bad in [b'', packet(32, data=b'x') + b'extra', b'x' * 4096, packet(255), packet(32, rights=8)]:
			owner(bad, expected=1 if len(bad) <= 1088 else 2)
		with sqlite3.connect(root / 'store.db') as db: assert db.execute('SELECT count(*) FROM objects').fetchone() == before
		results['L'] = 'PASS'
		# Same compiled storage engine, real process kills and reopened on-disk DB.
		text = run([str(REPO / ('out/test-native-' + args.cc)), '--crash', str(root / 'crash.db')])
		for stage in range(4): assert f'PASS crash stage={stage}' in text
		results['I'] = results['J'] = 'PASS'
		with sqlite3.connect(root / 'crash.db') as db:
			assert db.execute('SELECT current FROM objects').fetchone() == (2,)
			assert db.execute('SELECT rev,content FROM revisions ORDER BY rev').fetchall() == [(1, b'old-complete'), (2, b'new-complete')]
		print('LIVE RESULTS', json.dumps(results, sort_keys=True), flush=True)
	finally:
		for s in connections: s.close()
		for p in [service, broker]:
			if p is not None and p.poll() is None: p.terminate(); p.wait(timeout=5)
		for log in logs: log.close()
		(root / 'results.json').write_text(json.dumps(dict(landlock_abi=abi, scenarios=results), indent=2) + '\n')
		print('TEARDOWN: owned service/broker stopped; private DB/logs retained', root, flush=True)
	return 0


if __name__ == '__main__':
	raise SystemExit(main())
