# JANUS: non-mutating helper argument-contract tests. See LICENSING.md.
import hashlib
import subprocess
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HELPERS = (
	'tools/host-preflight.sh',
	'tools/install-deps.sh',
	'tools/probe-toolchain.sh',
)


class HelperArgumentTests(unittest.TestCase):
	def run_helper(self, helper: str, *args: str) -> subprocess.CompletedProcess[str]:
		return subprocess.run(
			['sh', str(ROOT / helper), *args],
			cwd=ROOT,
			stdin=subprocess.DEVNULL,
			capture_output=True,
			text=True,
			timeout=10,
			check=False,
		)

	def test_help_is_non_mutating(self):
		for helper in HELPERS:
			with self.subTest(helper=helper):
				path = ROOT / helper
				before = hashlib.sha256(path.read_bytes()).digest()
				result = self.run_helper(helper, '--help')
				self.assertEqual(result.returncode, 0, result.stderr)
				self.assertIn('usage:', result.stdout)
				self.assertEqual(before, hashlib.sha256(path.read_bytes()).digest())

	def test_unknown_option_rejected(self):
		for helper in HELPERS:
			with self.subTest(helper=helper):
				result = self.run_helper(helper, '--not-an-option')
				self.assertEqual(result.returncode, 2)
				self.assertIn('usage:', result.stderr)

	def test_extra_arguments_rejected(self):
		for helper in HELPERS:
			with self.subTest(helper=helper):
				result = self.run_helper(helper, '--help', 'unexpected')
				self.assertEqual(result.returncode, 2)


if __name__ == '__main__':
	unittest.main()
