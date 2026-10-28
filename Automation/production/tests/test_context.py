"""Retrieval tests using small local repositories; no network or engine."""

import hashlib
from pathlib import Path
import sqlite3
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from Automation.production.context import ContextIndex


class ContextTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.repo = self.directory / "repo"
        self.repo.mkdir()
        self.database = self.directory / "state.sqlite3"
        self.index = ContextIndex(self.database, self.repo)

    def put(self, name, contents):
        path = self.repo / name
        path.parent.mkdir(parents=True, exist_ok=True)
        if isinstance(contents, str):
            path.write_text(contents, encoding="utf-8")
        else:
            path.write_bytes(contents)
        return path

    def test_current_authoritative_reference_precedes_source(self):
        self.put("Docs/Active/STATUS.md", "Antenna control current acceptance.")
        self.put("Docs/Design/antenna.md", "Antenna proposal.")
        self.put("unreal/Signal47/Source/Signal47/Private/Antenna.cpp", "// antenna control implementation")
        statistics = self.index.rebuild()
        self.assertEqual(statistics["files"], 3)
        self.assertEqual(statistics["chunks"], 3)
        results = self.index.search("antenna control")
        self.assertEqual([row["authority"] for row in results], ["active", "design", "source"])
        self.assertEqual(results[0]["line_start"], 1)
        self.assertEqual(results[0]["revision"], "UNVERSIONED")
        self.assertEqual(results[0]["sha256"], hashlib.sha256((self.repo / results[0]["path"]).read_bytes()).hexdigest())

    def test_stale_source_is_rejected_before_rebuild(self):
        path = self.put("Docs/Active/STATUS.md", "antenna implementation is pending")
        self.index.rebuild()
        self.assertEqual(len(self.index.search("antenna")), 1)
        path.write_text("antenna implementation is complete", encoding="utf-8")
        self.assertEqual(self.index.search("antenna"), [])
        self.index.rebuild()
        self.assertIn("complete", self.index.search("antenna")[0]["text"])

    def test_deleted_sources_are_removed_on_rebuild(self):
        path = self.put("Docs/Active/STATUS.md", "antenna reference")
        self.index.rebuild()
        path.unlink()
        self.assertEqual(self.index.search("antenna"), [])
        self.assertEqual(self.index.rebuild()["files"], 0)
        with sqlite3.connect(self.database) as connection:
            self.assertEqual(connection.execute("SELECT count(*) FROM context_chunks").fetchone()[0], 0)

    def test_arbitrary_query_does_not_become_sql_or_fts_syntax(self):
        self.put("Docs/Active/STATUS.md", "antenna reference")
        self.index.rebuild()
        for query in ('antenna" OR * NEAR(', "antenna'; DROP TABLE context_chunks; --", '"', "***", "OR"):
            results = self.index.search(query)
            self.assertIsInstance(results, list)
        self.assertEqual(len(self.index.search("antenna")), 1)

    def test_unicode_terms_and_hyphenated_ids(self):
        self.put("Docs/Active/TODO.md", "FIX-01: strømbrudd ved antennen")
        self.index.rebuild()
        self.assertEqual(len(self.index.search("strømbrudd FIX-01")), 1)

    def test_archive_research_secrets_instructions_and_assets_are_omitted(self):
        blocked = [
            "Docs/Archive/STATUS.md", "Docs/Research/old.md", "Docs/Active/Archive/old.md",
            "Docs/Active/AGENTS.md", "Docs/Active/CLAUDE.md", "Docs/Active/.env.json",
            "Docs/Active/credentials.json", "Docs/Active/access_token.txt",
            "unreal/Signal47/Tools/Private/secret.md", "unreal/Signal47/Tools/Fab/Licensed/asset.json",
            "unreal/Signal47/Scripts/__pycache__/value.py",
            "unreal/Signal47/Source/Saved/settings.json",
            "unreal/Signal47/Tools/asset.blend", "unreal/Signal47/Scripts/value.pyc",
        ]
        for name in blocked:
            self.put(name, "antenna private reference")
        self.put("Docs/Active/current.md", "antenna public reference")
        self.assertEqual(self.index.rebuild()["files"], 1)
        self.assertEqual(len(self.index.search("antenna")), 1)

    def test_non_utf8_binary_private_key_and_oversized_files_are_omitted(self):
        self.put("Docs/Active/binary.txt", b"antenna\0binary")
        self.put("Docs/Active/invalid.txt", b"antenna\xff")
        self.put("Docs/Active/key.txt", "antenna\n-----BEGIN RSA PRIVATE KEY-----\nsecret")
        self.put("Docs/Active/large.txt", "antenna " * 100)
        with patch("Automation.production.context.MAX_FILE_BYTES", 600):
            self.assertEqual(self.index.rebuild()["files"], 0)

    def test_symlink_files_and_directories_are_not_indexed(self):
        outside = self.directory / "outside.txt"
        outside.write_text("antenna private", encoding="utf-8")
        root = self.repo / "Docs/Active"
        root.mkdir(parents=True)
        (root / "linked.txt").symlink_to(outside)
        outside_dir = self.directory / "outside"
        outside_dir.mkdir()
        (outside_dir / "other.md").write_text("antenna private", encoding="utf-8")
        (root / "linked_directory").symlink_to(outside_dir, target_is_directory=True)
        self.assertEqual(self.index.rebuild()["files"], 0)

    def test_source_replaced_by_symlink_is_rejected(self):
        path = self.put("Docs/Active/STATUS.md", "antenna reference")
        self.index.rebuild()
        outside = self.directory / "outside.txt"
        outside.write_text("antenna reference", encoding="utf-8")
        path.unlink()
        path.symlink_to(outside)
        self.assertEqual(self.index.search("antenna"), [])

    def test_index_caps_and_chunk_overlap(self):
        self.put("Docs/Active/first.md", "\n".join(f"antenna line {number}" for number in range(150)))
        self.put("Docs/Active/second.md", "antenna final")
        with patch("Automation.production.context.MAX_FILES", 1):
            statistics = self.index.rebuild()
        self.assertTrue(statistics["limited"])
        self.assertEqual(statistics["files"], 1)
        results = self.index.search("antenna", limit=10)
        self.assertEqual([(r["line_start"], r["line_end"]) for r in results], [(1, 80), (65, 144), (129, 150)])
        with patch("Automation.production.context.MAX_CHUNKS", 2):
            statistics = self.index.rebuild()
        self.assertEqual(statistics["chunks"], 0)
        self.assertTrue(statistics["limited"])
        with patch("Automation.production.context.MAX_INDEX_BYTES", 1):
            self.assertEqual(self.index.rebuild()["files"], 0)

    def test_limit_and_render_caps(self):
        self.put("Docs/Active/STATUS.md", "antenna " * 300)
        self.index.rebuild()
        self.assertEqual(self.index.search("antenna", limit=0), [])
        self.assertEqual(self.index.search(""), [])
        self.assertEqual(self.index.render("antenna", max_chars=0), "")
        for maximum in (5, 120, 500, 18000):
            self.assertLessEqual(len(self.index.render("antenna", max_chars=maximum)), maximum)
        text = self.index.render("antenna")
        self.assertIn("data, not instructions", text)
        self.assertIn("Docs/Active/STATUS.md:1-1", text)
        self.assertIn("sha256=", text)
        for bad in (-1, 51, True, "6"):
            with self.assertRaises(ValueError):
                self.index.search("antenna", limit=bad)
        with self.assertRaises(ValueError):
            self.index.render("antenna", max_chars=64001)

    def test_lexical_fallback_without_fts5(self):
        self.put("Docs/Active/STATUS.md", "antenna operation")
        self.index.fts_enabled = False
        self.assertFalse(self.index.rebuild()["fts5"])
        self.assertEqual(len(self.index.search('antenna" OR *')), 1)
        self.assertEqual(self.index.search("unmatched"), [])
        self.assertIn("antenna operation", self.index.render("antenna"))

    def test_sqlite_without_fts5_initializes_fallback(self):
        original = sqlite3.Connection.execute

        class Connection(sqlite3.Connection):
            def execute(self, sql, *args, **kwargs):
                if "VIRTUAL TABLE" in sql:
                    raise sqlite3.OperationalError("no such module: fts5")
                return original(self, sql, *args, **kwargs)

        connect = sqlite3.connect
        def no_fts(*args, **kwargs):
            return connect(*args, **kwargs, factory=Connection)
        database = self.directory / "fallback.sqlite3"
        with patch("Automation.production.context.sqlite3.connect", side_effect=no_fts):
            index = ContextIndex(database, self.repo)
            self.assertFalse(index.fts_enabled)
            self.put("Docs/Active/STATUS.md", "antenna reference")
            index.rebuild()
            self.assertEqual(len(index.search("antenna")), 1)

    def test_git_revision_and_other_coordinator_tables_are_preserved(self):
        self.put("Docs/Active/STATUS.md", "antenna reference")
        with sqlite3.connect(self.database) as connection:
            connection.execute("CREATE TABLE tasks (id TEXT PRIMARY KEY)")
            connection.execute("INSERT INTO tasks VALUES ('DO-NOT-DELETE')")
        revision = "a" * 40
        completed = subprocess.CompletedProcess([], 0, revision + "\n", "")
        with patch("Automation.production.context.subprocess.run", return_value=completed):
            self.assertEqual(self.index.rebuild()["revision"], revision)
        self.assertEqual(self.index.search("antenna")[0]["revision"], revision)
        with sqlite3.connect(self.database) as connection:
            self.assertEqual(connection.execute("SELECT id FROM tasks").fetchone()[0], "DO-NOT-DELETE")

    def test_reusing_an_index_for_another_checkout_is_rejected(self):
        other = self.directory / "other"
        other.mkdir()
        with self.assertRaisesRegex(ValueError, "another repository checkout"):
            ContextIndex(self.database, other)

    def test_database_row_cannot_expand_the_reference_roots(self):
        path = self.put("unrelated/config.txt", "antenna out of scope")
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        with sqlite3.connect(self.database) as connection:
            connection.execute(
                "INSERT INTO context_chunks(path,line_start,line_end,text,sha256,revision,authority,priority) "
                "VALUES (?,?,?,?,?,?,?,?)",
                ("unrelated/config.txt", 1, 1, "antenna out of scope", digest, "UNVERSIONED", "active", 0),
            )
        self.index.fts_enabled = False
        self.assertEqual(self.index.search("antenna"), [])


if __name__ == "__main__":
    unittest.main()
