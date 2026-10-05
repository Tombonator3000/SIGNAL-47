"""Verify refusal paths that protect the original installation and existing copy."""
import hashlib
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import prepare


class CopyProtectionTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / "research"
        self.source = Path(self.temp.name) / "installation"
        self.root.mkdir()
        self.source.mkdir()
        self.original = self.source / "sample.dll"
        self.original.write_bytes(b"original")
        files = prepare.inventory(self.source)
        self.release = {
            "version": "test", "loose_file_count": 1, "loose_total_bytes": 8,
            "source_inventory_sha256": hashlib.sha256(json.dumps(files, sort_keys=True, separators=(",", ":")).encode()).hexdigest(),
            "files": files,
        }
        (self.root / "release.json").write_text(json.dumps(self.release))
        self.destination = self.root / "private/game/test/WindowsNoEditor"
        self.patch = patch.object(prepare, "ROOT", self.root)
        self.patch.start()
        self.addCleanup(self.patch.stop)

    def test_same_length_modified_release_is_rejected_before_copy(self):
        self.original.write_bytes(b"modified")
        with self.assertRaisesRegex(ValueError, "Complete source-file hash inventory"):
            prepare.prepare(self.source)
        self.assertFalse(self.destination.exists())
        self.assertEqual(self.original.read_bytes(), b"modified")

    def test_existing_different_copy_is_not_overwritten(self):
        self.destination.mkdir(parents=True)
        existing = self.destination / "sample.dll"
        existing.write_bytes(b"different")
        with self.assertRaisesRegex(ValueError, "has not been overwritten"):
            prepare.prepare(self.source)
        self.assertEqual(existing.read_bytes(), b"different")
        self.assertEqual(self.original.read_bytes(), b"original")

    def test_hardlink_is_rejected_without_changing_original_permissions(self):
        self.destination.mkdir(parents=True)
        os.link(self.original, self.destination / "sample.dll")
        mode = self.original.stat().st_mode
        with self.assertRaisesRegex(ValueError, "shares a file with the original"):
            prepare.prepare(self.source)
        self.assertEqual(self.original.stat().st_mode, mode)
        self.assertEqual(self.original.read_bytes(), b"original")


if __name__ == "__main__":
    unittest.main()
