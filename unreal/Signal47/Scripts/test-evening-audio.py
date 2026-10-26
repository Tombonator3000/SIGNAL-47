#!/usr/bin/env python3
"""Offline contract tests for strict native voice import; no Unreal process."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import wave

spec = importlib.util.spec_from_file_location("evening_catalog", Path(__file__).with_name("evening-voice-catalog.py"))
catalog = importlib.util.module_from_spec(spec)
spec.loader.exec_module(catalog)


class EveningImportTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.wavs = self.root / "takes"
        self.wavs.mkdir()
        with wave.open(str(self.wavs / "D01_TEST_001.wav"), "wb") as wav:
            wav.setparams((1, 2, 48000, 0, "NONE", "not compressed"))
            wav.writeframes(b"\x01\x00" * 48000)
        self.format = {"sample_rate_hz": 48000, "sample_format": "PCM16", "channels": 1}
        self.manifest = {"pack_id": "signal47_evening_shift", "planned_format": self.format,
                         "cue_count": 2, "required_recording_count": 1, "recordings": []}
        self.subtitles = {"pack_id": "signal47_evening_shift", "scenes": []}
        for cue, scene, alias in (("D01_TEST_001", "D01_TEST", False), ("D61_ALIAS_001", "D61_ALIAS", True)):
            self.manifest["recordings"].append({"line_id": cue, "scene_id": scene, "actor": "DALE",
                "channel": "phone", "voice_profile": "kinski", "recording_required": not alias,
                "reuse_of": "D01_TEST_001" if alias else None, "audio_file": "EveningShift/D01_TEST_001.wav",
                "duration_seconds": 1})
            self.subtitles["scenes"].append({"scene_id": scene, "channel": "phone", "lines": [{
                "line_id": cue, "speaker": "DALE", "text": "Test cue.", "voiced": True,
                "start_seconds": .08, "end_seconds": .9, "timing_status": "from_audio_clip_relative"}]})
        self.takes = {"format": self.format, "takes": {"D01_TEST_001": {
            "file": "EveningShift/D01_TEST_001.wav", "sha256": catalog.wav_info(self.wavs / "D01_TEST_001.wav")["sha256"],
            "duration_seconds": 1, "speech_start_seconds": .08, "speech_end_seconds": .9}}}

    def entry(self):
        return catalog.catalog(self.manifest, self.subtitles, self.wavs, self.takes)

    def test_alias_shares_exact_original(self):
        clips = self.entry()["evening"]["catalog"]["clips"]
        self.assertEqual(clips[0]["raw"], clips[1]["raw"])
        self.assertEqual(clips[0]["sha256"], clips[1]["sha256"])
        self.assertEqual(clips[1]["speech_start_seconds"], .08)

    def test_reject_changed_take(self):
        self.takes["takes"]["D01_TEST_001"]["sha256"] = "0" * 64
        with self.assertRaisesRegex(catalog.CatalogError, "SHA-256"):
            self.entry()

    def test_reject_stale_subtitle_timing(self):
        self.subtitles["scenes"][1]["lines"][0]["end_seconds"] = .91
        with self.assertRaisesRegex(catalog.CatalogError, "subtitle timing"):
            self.entry()

    def test_reject_wrong_alias_identity(self):
        self.manifest["recordings"][1]["voice_profile"] = "waitress"
        with self.assertRaisesRegex(catalog.CatalogError, "alias identity"):
            self.entry()

    def test_reject_extra_or_missing_recording(self):
        (self.wavs / "D61_ALIAS_001.wav").write_bytes((self.wavs / "D01_TEST_001.wav").read_bytes())
        with self.assertRaisesRegex(catalog.CatalogError, "WAV set mismatch"):
            self.entry()
        (self.wavs / "D01_TEST_001.wav").unlink()
        with self.assertRaisesRegex(catalog.CatalogError, "WAV set mismatch"):
            self.entry()

    def test_preserve_other_banks_and_pcm(self):
        raw = self.root / "Raw"
        raw.mkdir()
        old = {"artbell": {"catalog": {"clips": [{"id": "earl", "text": "Old programme."}]}}, "other": {"sentinel": 47}}
        (raw / "voices.json").write_text(json.dumps(old))
        (raw / "untouched.wav").write_bytes(b"old raw data")
        catalog.import_bank(self.entry(), self.wavs, raw)
        result = catalog.read_json(raw / "voices.json")
        self.assertEqual({key: result[key] for key in old}, old)
        self.assertEqual((raw / "untouched.wav").read_bytes(), b"old raw data")
        self.assertEqual((raw / "voice/evening/D01_TEST_001.wav").read_bytes(), (self.wavs / "D01_TEST_001.wav").read_bytes())
        self.assertEqual(len(list((raw / "voice/evening").glob("*.wav"))), 1)


if __name__ == "__main__":
    unittest.main()
