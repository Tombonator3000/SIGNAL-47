"""Lette, isolerte kilde-/hashporter. Ingen syntese, ASR-tjeneste eller motor."""
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
import wave

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))


def load(name, filename):
    spec = importlib.util.spec_from_file_location(name, HERE / filename)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


CATALOG = load('horror_catalog_contract_test', 'catalog.py')
VERIFY = load('horror_verify_contract_test', 'verify.py')
CONTRACT = CATALOG.text_contract


class RecordingEvidence(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='s47-voice-contract-')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.data = self.root / 'data'
        (self.data / 'takes').mkdir(parents=True)
        self.cues = [{'line_id': 'SC01_FIRST_REPORT_001', 'actor': 'DISPATCH', 'channel': 'phone',
                      'voice_profile': 'caliche_dispatch', 'text': 'Something struck the glass.'}]
        self.cue = self.cues[0]
        path = self.data / 'takes' / (self.cue['line_id'] + '.wav')
        with wave.open(str(path), 'wb') as wav:
            wav.setnchannels(1);wav.setsampwidth(2);wav.setframerate(48000)
            wav.writeframes(b'\0\0' * 24000)
        self.digest = hashlib.sha256(path.read_bytes()).hexdigest()
        self.item = {**self.cue, 'request': {'text': self.cue['text'], 'seed': 42}, 'pronunciation_override': False,
                     'audio': {'sha256': self.digest, 'integrated_lufs': -20}}
        self.previous = {k: self.cue[k] for k in ('actor', 'voice_profile', 'text')}
        self.previous.update(sha256=self.digest, technical_problems=[],
                             asr={'text': self.cue['text'], 'normalized_word_edits': 0, 'strict_word_edits': 0})
        self.generation = {'takes': {self.cue['line_id']: self.item}}
        self.verification = {'technical_status': 'PASS', 'word_status': 'CHECKED', 'takes': {self.cue['line_id']: self.previous}}
        self.timings = {'takes': {self.cue['line_id']: {'sha256': self.digest, 'duration_seconds': .5,
                                                      'speech_start_seconds': .08, 'speech_end_seconds': .3}}}
        for module in (CATALOG, VERIFY):
            for target, name, value in ((module, 'HERE', self.root), (module.generate, 'cues', lambda: self.cues),
                                        (module.generate, 'SCENARIO', 'SC01')):
                mock = patch.object(target, name, value);mock.start();self.addCleanup(mock.stop)
        # Målingene er stubbet for å isolere tekstporten. Dette er ikke lyd-QA.
        metrics = {'peak_dbfs': -3, 'samples_at_or_above_full_scale': 0, 'nonsilent_rms_dbfs': -20,
                   'first_2ms_peak': 0, 'last_2ms_peak': 0, 'speech_start_seconds': .08, 'speech_end_seconds': .3}
        mock = patch.object(VERIFY.generate, 'metrics', return_value=metrics);mock.start();self.addCleanup(mock.stop)
        self.persist()

    def persist(self):
        for filename, value in (('generation.json', self.generation), ('verification.json', self.verification), ('takes.json', self.timings)):
            (self.root / filename).write_text(json.dumps(value))

    def test_current_recording_catalog_and_legacy_asr_pass(self):
        result = CATALOG.catalog(self.data)
        self.assertEqual(result['horror']['validation']['cue_count'], 1)
        self.assertEqual(result['horror']['catalog']['clips'][0]['sha256'], self.digest)
        self.assertIs(CONTRACT.cached_asr(self.cue, self.previous, self.digest), self.previous['asr'])

    def test_current_scenario_identity_is_retained(self):
        with patch.object(CATALOG.generate, 'SCENARIO', 'SC02'):
            self.assertEqual(CATALOG.catalog(self.data)['horror']['validation']['scenario'], 'SC02')

    def test_old_generation_cannot_be_relabelled_as_new_cue(self):
        self.item['text'] = self.item['request']['text'] = 'You saw a moving hand.'
        self.persist()
        before = (self.root / 'takes.json').read_bytes()
        with self.assertRaisesRegex(ValueError, 'generation text differs'):
            CATALOG.catalog(self.data)
        self.assertEqual((self.root / 'takes.json').read_bytes(), before)

    def test_spoken_request_cannot_hide_behind_current_label(self):
        self.item['request']['text'] = 'You saw a moving hand.'
        self.persist()
        with self.assertRaisesRegex(ValueError, 'request text differs'):
            CATALOG.catalog(self.data)

    def test_only_explicit_current_pronunciation_override_is_allowed(self):
        changed = copy.deepcopy(self.item)
        changed['request']['text'] = 'Something struck the glaass.'
        changed['pronunciation_override'] = True
        self.assertTrue(CONTRACT.generation_problems(self.cue, changed, self.digest, {}))
        overrides = {self.cue['line_id']: {'spoken': changed['request']['text']}}
        self.assertFalse(CONTRACT.generation_problems(self.cue, changed, self.digest, overrides))

    def test_wrong_voice_generation_is_rejected(self):
        self.item['voice_profile'] = 'kinski';self.persist()
        with self.assertRaisesRegex(ValueError, 'voice_profile differs'):
            CATALOG.catalog(self.data)

    def test_verification_must_bind_current_text_and_actual_file(self):
        for field, bad in (('text', 'You saw a moving hand.'), ('sha256', '0' * 64)):
            with self.subTest(field=field):
                current = copy.deepcopy(self.previous);self.previous[field] = bad;self.persist()
                with self.assertRaises(ValueError):CATALOG.catalog(self.data)
                self.previous.clear();self.previous.update(current)

    def test_generation_hash_must_match_actual_wav(self):
        self.item['audio']['sha256'] = '0' * 64;self.persist()
        with self.assertRaisesRegex(ValueError, 'generation audio hash differs'):
            CATALOG.catalog(self.data)

    def test_no_asr_rejects_same_hash_with_old_text(self):
        self.previous['text'] = 'You saw a moving hand.';self.persist()
        with patch.object(VERIFY.asr_tools, 'transcribe') as transcribe:
            result = VERIFY.verify(self.data, 'http://unused', no_asr=True)
        transcribe.assert_not_called()
        self.assertIsNone(result['takes'][self.cue['line_id']]['asr'])
        self.assertEqual(result['status'], 'FAIL')

    def test_stale_generation_never_gets_asr_or_publishable_timing(self):
        self.item['text'] = 'You saw a moving hand.';self.persist()
        with patch.object(VERIFY.asr_tools, 'transcribe') as transcribe:
            result = VERIFY.verify(self.data, 'http://unused')
        transcribe.assert_not_called()
        self.assertEqual(result['technical_status'], 'FAIL')
        self.assertNotIn(self.cue['line_id'], json.loads((self.root / 'takes.json').read_text())['takes'])

    def test_fresh_asr_is_bound_to_text_and_file(self):
        with patch.object(VERIFY.asr_tools, 'transcribe', return_value={'text': self.cue['text']}):
            result = VERIFY.verify(self.data, 'http://unused')
        self.assertEqual(result['status'], 'PASS')
        asr = result['takes'][self.cue['line_id']]['asr']
        self.assertEqual(asr['expected_text'], self.cue['text'])
        self.assertEqual(asr['audio_sha256'], self.digest)

    def test_mismatched_new_asr_metadata_cannot_be_cached(self):
        for key, bad in (('expected_text', 'A different sentence.'), ('audio_sha256', '0' * 64)):
            value = copy.deepcopy(self.previous);value['asr'][key] = bad
            self.assertIsNone(CONTRACT.cached_asr(self.cue, value, self.digest))

    def test_empty_or_partial_catalog_is_rejected(self):
        with patch.object(CATALOG.generate, 'cues', return_value=[]):
            with self.assertRaisesRegex(ValueError, 'Missing or duplicate'):CATALOG.catalog(self.data)
        self.timings['takes'].clear();self.persist()
        with self.assertRaisesRegex(ValueError, 'Partial'):CATALOG.catalog(self.data)


if __name__ == '__main__':
    unittest.main()
