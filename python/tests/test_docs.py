"""The published issue-code table matches the header."""

import subprocess
import sys
import unittest
from pathlib import Path

_ROOT = Path(__file__).resolve().parents[2]
_SCRIPT = _ROOT / "docs" / "generate_issue_codes.py"


class IssueCodeTableTest(unittest.TestCase):
    def test_table_matches_the_header(self):
        completed = subprocess.run(
            [sys.executable, str(_SCRIPT), "--check"],
            cwd=_ROOT,
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(completed.returncode, 0, completed.stdout + completed.stderr)
