#!/usr/bin/env python3
# JANUS: bootstrap validation, not an OS/security test. See LICENSING.md.
import hashlib
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

try:
	import tomllib
except ModuleNotFoundError:
	print('BLOCKED: Python 3.11+ is required for TOML validation', file=sys.stderr)
	raise SystemExit(1)

ROOT = Path(__file__).resolve().parent.parent
REQUIRED = (
	'AGENTS.md', 'AI_CONTEXT.md', 'BOOTSTRAP_PLAN.md', 'CODEX_START.md',
	'README.md', 'LICENSING.md', 'Makefile', '.codex/config.toml',
	'.editorconfig', '.clang-format', '.gitignore',
	'docs/REQUIREMENTS.md', 'docs/SOURCES.md', 'docs/PLANS.md',
	'docs/development/CODING.md', 'docs/development/CODEX.md',
	'docs/development/HOST.md', 'docs/development/VM_LAB.md',
	'docs/development/GITHUB.md', 'docs/decisions/README.md',
	'docs/templates/ADR.md', 'docs/templates/EVIDENCE.md',
	'docs/architecture/JANUS_Whitepaper_v0.1.md',
	'tools/host-preflight.sh', 'tools/install-deps.sh',
	'tools/probe-toolchain.sh', 'tools/debian13-packages.txt',
	'tests/toolchain/smoke.c', 'tests/toolchain/smoke.cc',
	'tests/bootstrap/test_helpers.py',
)


def run_check(label: str, argv: list[str]) -> bool:
	print(f'CHECK: {label}', flush=True)
	try:
		result = subprocess.run(argv, cwd=ROOT, timeout=60, check=False)
	except (OSError, subprocess.TimeoutExpired) as error:
		print(f'FAIL: {label}: {error}')
		return False
	print(f'{"PASS" if result.returncode == 0 else "FAIL"}: {label}; exit {result.returncode}')
	return result.returncode == 0


def main() -> int:
	failures: list[str] = []
	for name in REQUIRED:
		path = ROOT / name
		if not path.is_file() or path.is_symlink():
			failures.append(f'missing/non-regular required file: {name}')
	if failures:
		for message in failures:
			print(f'FAIL: {message}')
		return 1

	for name in REQUIRED:
		data = (ROOT / name).read_bytes()
		if not data.endswith(b'\n') or b'\r' in data or b'\x00' in data:
			failures.append(f'line-ending/NUL issue: {name}')
		try:
			data.decode('utf-8')
		except UnicodeDecodeError:
			failures.append(f'invalid UTF-8: {name}')

	instruction_size = (ROOT / 'AGENTS.md').stat().st_size
	print(f'INFO: root AGENTS.md is {instruction_size} bytes; ancestors not included')
	if instruction_size >= 32768:
		failures.append('root AGENTS.md exhausts the default 32 KiB budget')

	try:
		config = tomllib.loads((ROOT / '.codex/config.toml').read_text())
		if config.get('approval_policy') != 'on-request':
			failures.append('unexpected bootstrap approval policy')
		if config.get('sandbox_mode') != 'workspace-write':
			failures.append('unexpected bootstrap sandbox mode')
		if config.get('sandbox_workspace_write', {}).get('network_access') is not False:
			failures.append('shell networking is not explicitly disabled')
	except (OSError, ValueError) as error:
		failures.append(f'Codex TOML parse: {error}')

	for skill_dir in sorted((ROOT / '.agents/skills').iterdir()):
		if not skill_dir.is_dir() or skill_dir.is_symlink():
			failures.append(f'invalid skill directory: {skill_dir.name}')
			continue
		path = skill_dir / 'SKILL.md'
		if not path.is_file() or path.is_symlink():
			failures.append(f'missing SKILL.md: {skill_dir.name}')
			continue
		text = path.read_text(encoding='utf-8')
		match = re.match(r'\A---\nname: ([a-z0-9-]+)\ndescription: ([^\n]+)\n---\n', text)
		if not match or match.group(1) != skill_dir.name:
			failures.append(f'invalid simple skill frontmatter: {skill_dir.name}')

	# The whitepaper is an immutable source, unlike the evolving working docs.
	sources = (ROOT / 'docs/SOURCES.md').read_text(encoding='utf-8')
	digest_match = re.search(r'- SHA-256: `([0-9a-f]{64})`', sources)
	actual = hashlib.sha256((ROOT / 'docs/architecture/JANUS_Whitepaper_v0.1.md').read_bytes()).hexdigest()
	if not digest_match or digest_match.group(1) != actual:
		failures.append('retained whitepaper digest mismatch')

	# Validate relative Markdown links in our README (URLs/anchors are not fetched).
	readme = (ROOT / 'README.md').read_text(encoding='utf-8')
	for target in re.findall(r'\]\(([^)]+)\)', readme):
		if '://' not in target and not target.startswith('#'):
			if not (ROOT / target.split('#', 1)[0]).is_file():
				failures.append(f'broken README link: {target}')

	for script in sorted((ROOT / 'tools').glob('*.sh')):
		if script.is_symlink() or not os.access(script, os.X_OK):
			failures.append(f'shell helper is symlinked/not executable: {script.name}')
		if not run_check(f'sh syntax: {script.name}', ['sh', '-n', str(script)]):
			failures.append(f'shell syntax: {script.name}')

	if not run_check('helper argument contracts', [sys.executable, '-B', '-m',
	    'unittest', 'discover', '-s', 'tests/bootstrap', '-v']):
		failures.append('helper argument tests')

	if shutil.which('shellcheck'):
		scripts = [str(p) for p in sorted((ROOT / 'tools').glob('*.sh'))]
		if not run_check('shellcheck', ['shellcheck', '-s', 'sh', *scripts]):
			failures.append('shellcheck')
	else:
		print('NOT RUN: shellcheck unavailable; install M0 tools on the target host')

	if shutil.which('clang-format'):
		# Parse with the installed formatter without reformatting any source file.
		try:
			result = subprocess.run(['clang-format', '--style=file',
			    '--assume-filename=tests/toolchain/smoke.c', '--dump-config'],
			    cwd=ROOT, capture_output=True, text=True, timeout=15, check=False)
			if result.returncode:
				failures.append(f'clang-format config: {result.stderr.strip()}')
			else:
				print('PASS: installed clang-format accepts configuration')
		except (OSError, subprocess.TimeoutExpired) as error:
			failures.append(f'clang-format config: {error}')
	else:
		print('NOT RUN: clang-format configuration acceptance (tool unavailable)')

	for message in failures:
		print(f'FAIL: {message}')
	if failures:
		return 1
	print('PASS: available bootstrap checks; any NOT RUN checks remain unverified')
	print('NOT RUN: JANUS M0 models/runtime, native kernel, VM boots and physical hardware')
	return 0


if __name__ == '__main__':
	raise SystemExit(main())
