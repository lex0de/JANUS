#!/usr/bin/env python3
# SPDX-License-Identifier: ISC
# JANUS explicit disposable integration harness; Copyright (c) 2026 Danyal A. Samak.
import argparse
import hashlib
import json
import os
import pathlib
import shutil
import signal
import socket
import stat
import subprocess
import sys
import time
import uuid
import xml.etree.ElementTree as ET

REPO = pathlib.Path(__file__).resolve().parents[2]
URI = 'qemu:///session'


def run(argv, **kwargs):
	'''Run bounded explicit argv and retain failures in the harness log.'''
	print('COMMAND', repr([str(v) for v in argv]), flush=True)
	result = subprocess.run(argv, text=True, capture_output=True, timeout=30, **kwargs)
	print('EXIT', result.returncode, result.stdout, result.stderr, flush=True)
	if result.returncode:
		raise RuntimeError('command failed')
	return result.stdout.strip()


def wait_for(test, seconds=12):
	'''Wait a bounded time for an observed condition.'''
	end = time.monotonic() + seconds
	while time.monotonic() < end:
		if test():
			return
		time.sleep(0.1)
	raise RuntimeError('condition timed out')


def archive_entry(name, mode, data=b'', major=0, minor=0):
	'''Encode a newc entry for an isolated guest, never a host device node.'''
	name = name.encode() + b'\0'
	fields = [1, mode, 0, 0, 1, 0, len(data), 0, 0, major, minor, len(name), 0]
	header = b'070701' + b''.join(f'{x:08x}'.encode() for x in fields)
	chunk = header + name
	chunk += b'\0' * (-len(chunk) % 4)
	chunk += data + b'\0' * (-len(data) % 4)
	return chunk


def build_init(root):
	'''Compile repository PID1 probe; no downloaded guest userspace.'''
	path = root / 'initramfs'
	if path.exists(): path.rename(path.with_name('initramfs.' + str(time.time_ns())))
	run(['gcc', '-std=c17', '-Wall', '-Wextra', '-Werror', '-static', '-O1', str(REPO / 'tests/hosted/guest_init.c'), '-o', str(root / 'init')])
	data = archive_entry('dev', stat.S_IFDIR | 0o755)
	data += archive_entry('dev/console', stat.S_IFCHR | 0o600, major=5, minor=1)
	data += archive_entry('init', stat.S_IFREG | 0o755, (root / 'init').read_bytes())
	data += archive_entry('TRAILER!!!', 0)
	(root / 'initramfs').write_bytes(data)
	(root / 'initramfs').chmod(0o400)


def create(root):
	'''Create exactly one new UUID-bound fixture and preserve its provenance.'''
	root.mkdir(mode=0o700, parents=False)
	for name in ['runtime', 'display']:
		(root / name).mkdir(mode=0o700)
	identity = str(uuid.uuid4())
	kernel = pathlib.Path('/boot') / ('vmlinuz-' + os.uname().release)
	shutil.copyfile(kernel, root / 'kernel')
	build_init(root)
	run(['qemu-img', 'create', '-f', 'qcow2', str(root / 'base.qcow2'), '64M'])
	(root / 'base.qcow2').chmod(0o400)
	run(['qemu-img', 'create', '-f', 'qcow2', '-F', 'qcow2', '-b', str(root / 'base.qcow2'), str(root / 'overlay.qcow2')])
	for name in ['kernel', 'initramfs']:
		(root / name).chmod(0o400)
	meta = {'uuid': identity, 'uri': URI, 'name': 'janus-m1-disposable', 'root': str(root),
	        'init_source_sha256': hashlib.sha256((REPO / 'tests/hosted/guest_init.c').read_bytes()).hexdigest(),
	        'kernel_source': str(kernel), 'kernel_package': run(['dpkg-query', '-W', 'linux-image-' + os.uname().release]),
	        'hashes': {name: hashlib.sha256((root / name).read_bytes()).hexdigest() for name in ['kernel', 'initramfs', 'base.qcow2']}}
	(root / 'fixture.json').write_text(json.dumps(meta, indent=2) + '\n')
	(root / 'profiles').write_text(f'lab {URI} {identity}\n')
	(root / 'profiles').chmod(0o600)
	domain = ET.Element('domain', type='kvm')
	ET.SubElement(domain, 'name').text = meta['name']
	ET.SubElement(domain, 'uuid').text = identity
	ET.SubElement(domain, 'memory', unit='MiB').text = '256'
	ET.SubElement(domain, 'vcpu').text = '1'
	osnode = ET.SubElement(domain, 'os')
	ET.SubElement(osnode, 'type', arch='x86_64', machine='pc').text = 'hvm'
	ET.SubElement(osnode, 'kernel').text = str(root / 'kernel')
	ET.SubElement(osnode, 'initrd').text = str(root / 'initramfs')
	ET.SubElement(osnode, 'cmdline').text = 'console=tty0 console=ttyS0,115200 panic=-1'
	ET.SubElement(ET.SubElement(domain, 'features'), 'acpi')
	ET.SubElement(domain, 'on_reboot').text = 'restart'
	ET.SubElement(domain, 'on_poweroff').text = 'destroy'
	devices = ET.SubElement(domain, 'devices')
	ET.SubElement(devices, 'emulator').text = '/usr/bin/qemu-system-x86_64'
	disk = ET.SubElement(devices, 'disk', type='file', device='disk')
	ET.SubElement(disk, 'driver', name='qemu', type='qcow2')
	ET.SubElement(disk, 'source', file=str(root / 'overlay.qcow2'))
	ET.SubElement(disk, 'target', dev='vda', bus='virtio')
	serial = ET.SubElement(devices, 'serial', type='unix')
	ET.SubElement(serial, 'source', mode='bind', path=str(root / 'serial.sock'))
	ET.SubElement(serial, 'target', port='0')
	ET.SubElement(devices, 'graphics', type='vnc', socket=str(root / 'vnc.sock'), autoport='no')
	video = ET.SubElement(devices, 'video')
	ET.SubElement(video, 'model', type='vga', heads='1')
	(root / 'domain.xml').write_text(ET.tostring(domain, encoding='unicode'))
	# A collision is a blocker, not permission to replace an existing fixture.
	names = run(['virsh', '-c', URI, 'list', '--all', '--name']).splitlines()
	if meta['name'] in names:
		raise RuntimeError('fixture name already exists; no domain modified')
	run(['virsh', '-c', URI, 'define', str(root / 'domain.xml')])
	return meta


def exercise(root, meta):
	'''Exercise live scenarios only against the freshly recorded disposable UUID.'''
	identity = meta['uuid']
	logs = []
	weston = broker = None
	serial = None
	results = {}
	def ctl(op, good=True):
		p = subprocess.run([str(REPO / 'out/janusctl'), str(root / 'runtime'), op, 'lab'], capture_output=True, text=True, timeout=20)
		print('CTL', op, p.returncode, p.stdout, p.stderr, flush=True)
		if good and p.returncode:
			raise RuntimeError('client failed')
		words = p.stdout.strip().split()
		return words[0], dict(item.split('=', 1) for item in words[1:])
	def launch_broker():
		log = open(root / 'broker.log', 'a'); logs.append(log)
		p = subprocess.Popen([str(REPO / 'out/janusd'), str(root / 'profiles'), str(root / 'runtime'), str(root / 'display'), 'wayland-janus'], stdout=log, stderr=log)
		try:
			wait_for(lambda: (root / 'runtime/control').exists())
			time.sleep(0.2)
			if p.poll() is not None:
				raise RuntimeError('broker startup failed')
		except BaseException:
			if p.poll() is None: p.terminate(); p.wait(timeout=10)
			raise
		return p
	def active():
		return run(['virsh', '-c', URI, 'domstate', identity]) == 'running'
	def serial_until(marker):
		data = b''; serial.settimeout(15)
		while marker not in data:
			chunk = serial.recv(4096)
			if not chunk: raise RuntimeError('serial disconnected')
			data += chunk
		with open(root / 'serial.log', 'ab') as log: log.write(data)
		print('GUEST MARKER', marker.decode(), flush=True)
	try:
		log = open(root / 'weston.log', 'w'); logs.append(log)
		env = {'PATH': '/usr/bin:/bin', 'XDG_RUNTIME_DIR': str(root / 'display')}
		weston = subprocess.Popen(['/usr/bin/weston', '--backend=headless', '--renderer=pixman', '--socket=wayland-janus', '--idle-time=0', '--width=800', '--height=600', '--no-config'], env=env, stdout=log, stderr=log)
		wait_for(lambda: (root / 'display/wayland-janus').exists())
		broker = launch_broker()
		_, a = ctl('select'); assert a['uuid'] == identity and a['execution'] == 'inactive'
		results['A'] = 'PASS'
		_, b = ctl('run'); assert active() and b['incarnation'] == '1' and int(b['viewer']) > 0
		serial = socket.socket(socket.AF_UNIX); serial.connect(str(root / 'serial.sock'))
		serial_until(b'JANUS_M1_BOOT')
		time.sleep(1)
		_, b = ctl('status'); assert int(b['viewer']) > 0
		vnc = json.loads(run(['virsh', '-c', URI, 'qemu-monitor-command', identity, '{"execute":"query-vnc"}']))
		assert len(vnc['return']['clients']) == 1
		results['B'] = 'PASS'
		_, c = ctl('run'); assert c == b
		results['C'] = 'PASS'
		_, d = ctl('leave'); assert d['viewer'] == '0' and d['foreground'] == '0' and active() and d['incarnation'] == '1'
		results['D'] = 'PASS'
		_, e = ctl('run'); assert e['incarnation'] == '1' and int(e['viewer']) > 0
		results['E'] = 'PASS'
		# Verify viewer parentage before signalling it; never signal state read from disk.
		def owned_viewer(pid):
			fields = pathlib.Path(f'/proc/{pid}/stat').read_text().rsplit(')', 1)[1].split()
			assert int(fields[1]) == broker.pid
			return pid
		def signal_viewer(pid, sig):
			fd = os.pidfd_open(pid)
			try:
				owned_viewer(pid)
				signal.pidfd_send_signal(fd, sig)
			finally: os.close(fd)
		signal_viewer(int(e['viewer']), signal.SIGKILL)
		wait_for(lambda: ctl('status')[1]['viewer'] == '0')
		assert active(); results['F'] = 'PASS'
		_, g = ctl('run'); signal_viewer(int(g['viewer']), signal.SIGSTOP)
		_, g = ctl('recover'); assert g['viewer'] == '0' and g['foreground'] == '0' and active()
		results['G'] = 'PASS'
		_, h = ctl('run'); viewer = owned_viewer(int(h['viewer']))
		broker.kill(); broker.wait(timeout=5)
		wait_for(lambda: not pathlib.Path(f'/proc/{viewer}').exists() or pathlib.Path(f'/proc/{viewer}/stat').read_text().rsplit(')', 1)[1].split()[0] == 'Z')
		assert active(); broker = launch_broker()
		_, h = ctl('status'); assert h['viewer'] == '0' and h['foreground'] == '0' and h['incarnation'] == '1'
		results['H'] = 'PASS'
		serial.sendall(b'reboot\n'); serial_until(b'JANUS_M1_REBOOT'); serial_until(b'JANUS_M1_BOOT')
		_, i = ctl('status'); assert active() and i['incarnation'] == '1'
		results['I'] = 'PASS'
		serial.sendall(b'poweroff\n'); serial_until(b'JANUS_M1_POWEROFF'); serial.close(); serial = None
		wait_for(lambda: ctl('status')[1]['execution'] == 'inactive')
		_, j = ctl('run'); assert j['incarnation'] == '2' and active()
		results['J'] = 'PASS'
		# Rename only the fixture: UUID routing must continue independently of name.
		ctl('recover'); run(['virsh', '-c', URI, 'destroy', identity]); ctl('status')
		collision = 'janus-m1-collision'
		assert collision not in run(['virsh', '-c', URI, 'list', '--all', '--name']).splitlines()
		run(['virsh', '-c', URI, 'domrename', identity, collision])
		broker.terminate(); broker.wait(timeout=10)
		missing = '00000000-0000-4000-8000-000000000001'
		assert missing not in run(['virsh', '-c', URI, 'list', '--all', '--uuid']).splitlines()
		(root / 'profiles').write_text(f'{collision} {URI} {missing}\n')
		broker = launch_broker()
		p = subprocess.run([str(REPO / 'out/janusctl'), str(root / 'runtime'), 'run', collision], capture_output=True, text=True, timeout=20)
		print('COLLISION', p.returncode, p.stdout, flush=True)
		assert p.returncode == 1 and p.stdout.startswith('backend-error ')
		assert run(['virsh', '-c', URI, 'domstate', identity]) == 'shut off'
		broker.terminate(); broker.wait(timeout=10)
		(root / 'profiles').write_text(f'lab {URI} {identity}\n')
		broker = launch_broker()
		_, k = ctl('select'); assert k['uuid'] == identity and k['execution'] == 'inactive'
		_, k = ctl('run'); assert k['uuid'] == identity and active()
		results['K'] = 'PASS (one fixture name matches a profile with an absent UUID; no fallback)'
		_, before = ctl('status')
		for data in [b'xml lab\n', b'run lab\nextra', b'run lab\nrun lab\n', b'run ../x\n', b'run missing\n', b'x' * 4096]:
			with socket.socket(socket.AF_UNIX, socket.SOCK_SEQPACKET) as s:
				s.settimeout(5); s.connect(str(root / 'runtime/control')); s.sendall(data)
				reply = s.recv(512); assert not reply.startswith(b'ok ')
		_, after = ctl('status'); assert before == after
		# Fill bounded pending-client slots; idle connections expire without actions.
		clients = []
		for unused in range(9):
			s = socket.socket(socket.AF_UNIX, socket.SOCK_SEQPACKET); s.settimeout(2)
			s.connect(str(root / 'runtime/control')); clients.append(s)
		time.sleep(1.5)
		for s in clients: s.close()
		assert ctl('status')[1] == after
		results['L'] = 'PASS'
		print('LIVE RESULTS', json.dumps(results, sort_keys=True), flush=True)
	finally:
		(root / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
		if serial is not None: serial.close()
		if broker is not None and broker.poll() is None:
			broker.terminate(); broker.wait(timeout=10)
		if weston is not None and weston.poll() is None:
			weston.terminate(); weston.wait(timeout=10)
		for log in logs: log.close()


def teardown(root, meta):
	'''Stop/undefine only the fixture UUID after verifying its exact image path.'''
	xml = run(['virsh', '-c', URI, 'dumpxml', meta['uuid']])
	domain = ET.fromstring(xml)
	assert domain.findtext('uuid') == meta['uuid']
	assert domain.find('devices/disk/source').get('file') == str(root / 'overlay.qcow2')
	state = run(['virsh', '-c', URI, 'domstate', meta['uuid']])
	if state != 'shut off': run(['virsh', '-c', URI, 'destroy', meta['uuid']])
	run(['virsh', '-c', URI, 'undefine', meta['uuid']])
	print('TEARDOWN PASS; images/logs retained:', root, flush=True)


def main():
	'''Require an explicit new disposable fixture directory under artifacts/.'''
	parser = argparse.ArgumentParser()
	group = parser.add_mutually_exclusive_group(required=True)
	group.add_argument('--create', type=pathlib.Path)
	group.add_argument('--retry', type=pathlib.Path)
	args = parser.parse_args()
	root = (args.create or args.retry).absolute()
	if root.parent != REPO / 'artifacts' or (args.create and root.exists()) or root.is_symlink():
		raise SystemExit('use a NEW immediate child of repository artifacts/')
	os.umask(0o077)
	if args.create:
		meta = create(root)
	else:
		meta = json.loads((root / 'fixture.json').read_text())
		assert meta['root'] == str(root) and meta['uri'] == URI
		for name, digest in meta['hashes'].items():
			assert hashlib.sha256((root / name).read_bytes()).hexdigest() == digest
		assert meta['uuid'] not in run(['virsh', '-c', URI, 'list', '--all', '--uuid']).splitlines()
		assert meta['name'] not in run(['virsh', '-c', URI, 'list', '--all', '--name']).splitlines()
		(root / 'fixture.json').rename(root / ('fixture.json.' + str(time.time_ns())))
		build_init(root)
		meta['hashes']['initramfs'] = hashlib.sha256((root / 'initramfs').read_bytes()).hexdigest()
		meta['init_source_sha256'] = hashlib.sha256((REPO / 'tests/hosted/guest_init.c').read_bytes()).hexdigest()
		(root / 'fixture.json').write_text(json.dumps(meta, indent=2) + '\n')
		domain = ET.fromstring((root / 'domain.xml').read_text())
		assert domain.findtext('uuid') == meta['uuid']
		if domain.find('features') is None:
			ET.SubElement(ET.SubElement(domain, 'features'), 'acpi')
			(root / 'domain.xml').write_text(ET.tostring(domain, encoding='unicode'))
		(root / 'profiles').write_text(f"lab {URI} {meta['uuid']}\n")
		# Only with the exact fixture absent: preserve old experimental records.
		stamp = str(time.time_ns())
		for name in ['runtime/lab.state', 'results.json', 'serial.log']:
			path = root / name
			if path.exists(): path.rename(path.with_name(path.name + '.' + stamp))
		run(['virsh', '-c', URI, 'define', str(root / 'domain.xml')])
	try: exercise(root, meta)
	finally: teardown(root, meta)
	return 0


if __name__ == '__main__':
	raise SystemExit(main())
