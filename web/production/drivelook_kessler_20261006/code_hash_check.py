#!/usr/bin/env python3
"""Targeted PR78 code-hash regressions and read-only historical revalidation.

Run with the capture venv: python code_hash_check.py --result code_hash_checks.json
Mock trips test reporting/error paths, not gameplay. Original evidence is not edited.
"""
from __future__ import annotations
import argparse
import asyncio
from datetime import datetime, timezone
import hashlib
import importlib.util
import io
import json
from pathlib import Path
from types import SimpleNamespace
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parent
WEB = ROOT.parent.parent


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


capture = load('drivelook_hash_regression', WEB / 'tools/drivelook.py')
packer = load('kessler_hash_packer', ROOT / 'package_evidence.py')


def trip():
    return {'warnings': [], 'errors': [], 'failed_http': [], 'loaded_code_sha256': {},
            'observed_code_responses': [], 'code_hash_policy': 'observed_responses_v1'}


class Response:
    def __init__(self, url='http://test/assets/code.js', kind='script', body=b'code',
                 fail=False, gate=None, during=None):
        self.url = url
        self.request = SimpleNamespace(resource_type=kind)
        self.status = 200
        self.value, self.fail, self.gate, self.during = body, fail, gate, during

    async def body(self):
        if self.gate: await self.gate.wait()
        if self.during: self.during()
        if self.fail: raise RuntimeError('forced response.body failure')
        return self.value


class Page:
    """Minimal real runner control flow; deliberately no game rendering."""
    def __init__(self, fail_at='goto'):
        self.url = 'http://test/'
        self.events, self.fail_at, self.ticks, self.screenshots = {}, fail_at, 0, 0

    def set_default_timeout(self, _): pass
    def on(self, kind, callback): self.events[kind] = callback
    def remove_listener(self, kind, callback):
        if self.events.get(kind) is callback: del self.events[kind]
    async def add_init_script(self, _): pass
    async def goto(self, url, **_):
        self.url = url
        self.events['response'](Response(url, 'document', b'html'))
        self.events['response'](Response(fail=self.fail_at == 'goto'))
    async def wait_for_function(self, *_): pass
    async def click(self, *_): pass
    async def screenshot(self, *, path, **_):
        self.screenshots += 1
        Path(path).write_bytes(b'mock screenshot, not visual evidence')
    async def evaluate(self, code, arg=None):
        if code == capture.STATE:
            arrived = self.ticks >= 2
            return {'map_xyz': [0, 0, 0], 'road_local_z': 0, 'mile': 2.91,
                    'clock_seconds': 0, 'area': 'diner' if arrived else 'roswell',
                    'leg': 'roswell', 'driving': not arrived, 'held': True,
                    'chapter6_stage': 'drive', 'truck_at': 'diner'}
        if code == capture.KESSLER_EXTRA:
            return {'quality': 'high', 'picture': 'vhs', 'ultra_enabled': False,
                    'post': {'supported': True}}
        if code.startswith('([s,f])'): self.ticks += 1
        if 'getContext()' in code: return {'renderer': 'mock, not a GPU test'}
        return True


class Context:
    def __init__(self, page): self.page, self.closed = page, False
    async def new_page(self): return self.page
    async def close(self):
        if self.page.fail_at == 'close':
            self.page.events['response'](Response(url='http://test/late.js', fail=True))
        elif self.page.fail_at == 'close_console':
            self.page.events['console'](SimpleNamespace(type='error', text='late console error', location={}))
        elif self.page.fail_at == 'close_error':
            self.page.events['response'](Response(url='http://test/late.js', gate=asyncio.Event()))
            raise OSError('forced context.close failure')
        self.closed = True


class Browser:
    def __init__(self, page): self.context = Context(page)
    async def new_context(self, **_): return self.context


class HashRegressions(unittest.IsolatedAsyncioTestCase):
    async def test_document_script_and_non_code(self):
        t, tasks = trip(), []
        for r in (Response('http://test/', 'document', b'html'), Response(),
                  Response('http://test/image.png', 'image', fail=True)):
            capture.observe_response(t, r, tasks)
        self.assertEqual([r['hash_state'] for r in t['observed_code_responses']], ['pending'] * 2)
        await capture.wait_code_hashes(t, tasks)
        self.assertEqual(len(t['loaded_code_sha256']), 2)
        self.assertTrue(all(r['hash_state'] == 'hashed' for r in t['observed_code_responses']))
        self.assertFalse(t['errors'])

    async def test_failed_body_and_duplicate_url_cannot_hide_failure(self):
        t, tasks = trip(), []
        capture.observe_response(t, Response(), tasks)
        await capture.wait_code_hashes(t, tasks)
        capture.observe_response(t, Response(fail=True), tasks)
        with self.assertRaises(RuntimeError): await capture.wait_code_hashes(t, tasks)
        self.assertEqual(len(t['observed_code_responses']), 2)
        self.assertEqual(t['observed_code_responses'][-1]['hash_state'], 'failed')
        self.assertTrue(t['errors'])
        self.assertFalse(t['warnings'])

    async def test_response_added_during_drain_is_awaited(self):
        t, tasks = trip(), []
        late = Response('http://test/late.js', fail=True)
        capture.observe_response(t, Response(during=lambda: capture.observe_response(t, late, tasks)), tasks)
        with self.assertRaises(RuntimeError): await capture.wait_code_hashes(t, tasks)
        self.assertTrue(all(task.done() for task in tasks))
        self.assertEqual(t['observed_code_responses'][-1]['hash_state'], 'failed')

    async def test_timeout_cancels_and_awaits_pending_body(self):
        t, tasks = trip(), []
        capture.observe_response(t, Response(gate=asyncio.Event()), tasks)
        with self.assertRaises(TimeoutError):
            await asyncio.wait_for(capture.wait_code_hashes(t, tasks), timeout=.02)
        self.assertTrue(all(task.done() for task in tasks))
        self.assertEqual(t['observed_code_responses'][0]['hash_state'], 'cancelled')
        frozen = json.dumps(t, sort_keys=True)
        await asyncio.sleep(0)
        self.assertEqual(json.dumps(t, sort_keys=True), frozen)
        self.assertTrue(capture.code_hash_errors(t))

    async def test_both_runners_fail_before_screenshot_on_body_error(self):
        for identity in ('kessler', 'diner_oldroad'):
            with self.subTest(identity=identity), tempfile.TemporaryDirectory() as tmp:
                page = Page(); browser = Browser(page)
                report = {'trips': [], 'status': 'CAPTURING', 'failures': [],
                          'source': {'git_head': 'mock'}}
                args = SimpleNamespace(size=(844, 390), quality='high')
                with self.assertRaisesRegex(RuntimeError, 'code hash'):
                    await capture.run_trip(browser, args, report, identity, 'http://test/', Path(tmp))
                saved = json.loads((Path(tmp) / 'manifest.json').read_text())['trips'][0]
                self.assertEqual(saved['status'], 'FAIL')
                self.assertTrue(saved['errors'])
                self.assertTrue(browser.context.closed)
                self.assertEqual(page.screenshots, 0)

    async def test_close_response_reverts_captured_status_before_final_write(self):
        with tempfile.TemporaryDirectory() as tmp:
            page = Page('close'); browser = Browser(page)
            report = {'trips': [], 'status': 'CAPTURING', 'failures': [],
                      'source': {'git_head': 'mock'}}
            args = SimpleNamespace(size=(844, 390), quality='high')
            with self.assertRaisesRegex(RuntimeError, 'code hash'):
                await capture.run_trip(browser, args, report, 'saro_diner', 'http://test/', Path(tmp))
            saved = json.loads((Path(tmp) / 'manifest.json').read_text())['trips'][0]
            self.assertEqual(saved['status'], 'FAIL')
            self.assertEqual(saved['observed_code_responses'][-1]['hash_state'], 'failed')
            self.assertTrue(saved['code_observation_closed'])
            self.assertEqual(page.screenshots, 2)

    async def test_late_console_and_close_failure_block_and_detach(self):
        for mode in ('close_console', 'close_error'):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory() as tmp:
                page = Page(mode); browser = Browser(page)
                report = {'trips': [], 'status': 'CAPTURING', 'failures': [], 'source': {'git_head': 'mock'}}
                args = SimpleNamespace(size=(844, 390), quality='high')
                with self.assertRaises((RuntimeError, OSError)):
                    await capture.run_trip(browser, args, report, 'saro_diner', 'http://test/', Path(tmp))
                saved = json.loads((Path(tmp) / 'manifest.json').read_text())['trips'][0]
                self.assertEqual(saved['status'], 'FAIL')
                self.assertFalse(page.events)
                self.assertTrue(saved['code_observation_sealed'])
                self.assertFalse([r for r in saved['observed_code_responses'] if r['hash_state'] == 'pending'])
                self.assertFalse([t for t in asyncio.all_tasks() if t is not asyncio.current_task() and not t.done()])


class ValidationRegressions(unittest.TestCase):
    def test_unknown_missing_pending_and_malformed_inventory_block(self):
        for change in ({'code_hash_policy': 'unknown'}, {'code_hash_policy': None}, {'observed_code_responses': None},
                       {'observed_code_responses': 'wrong'},
                       {'observed_code_responses': [{'url': 'http://test/', 'hash_state': 'pending'}]}):
            with self.subTest(change=change):
                t = trip(); t.update(change)
                self.assertTrue(capture.code_hash_errors(t, require_closed=True))

    def test_complete_inventory_requires_closed_observation_and_exact_registry(self):
        t = trip(); digest = hashlib.sha256(b'html').hexdigest()
        t['observed_code_responses'] = [{'url': 'http://test/', 'resource_type': 'document',
                                       'status': 200, 'hash_state': 'hashed', 'sha256': digest}]
        t['loaded_code_sha256'] = {'http://test/': digest}
        self.assertTrue(capture.code_hash_errors(t, require_closed=True))
        t['code_observation_closed'] = True
        self.assertFalse(capture.code_hash_errors(t, require_closed=True))
        t['code_observation_closed'] = 'false'
        self.assertTrue(capture.code_hash_errors(t, require_closed=True))
        t['code_observation_closed'] = True
        t['loaded_code_sha256']['http://test/hidden.js'] = digest
        self.assertTrue(capture.code_hash_errors(t, require_closed=True))

    def test_late_http_failure_is_blocking_for_kessler(self):
        t = trip()
        t['failed_http'].append({'url': 'http://test/image.png', 'status': 500, 'resource_type': 'image'})
        with self.assertRaisesRegex(RuntimeError, 'non-favicon HTTP'):
            capture.check_capture_diagnostics(t, 'http://test/', strict_http=True)

    def test_historical_hash_warning_blocks_packer(self):
        folder = ROOT / 'low_844x390'
        m = json.loads((folder / 'manifest.json').read_text())
        m['trips'][0]['warnings'].append('Could not hash loaded code: http://test/missing.js: forced')
        checks, _, _ = packer.validate(folder, m, 'low', (844, 390), BUILD, capture)
        failed = [c for c in checks if c['status'] == 'FAIL']
        self.assertEqual([c['check'] for c in failed], ['no missing code hashes or historical hash-read warnings'])

    def test_new_inventory_missing_hash_blocks_packer(self):
        folder = ROOT / 'low_844x390'
        m = json.loads((folder / 'manifest.json').read_text()); t = m['trips'][0]
        t['code_hash_policy'] = 'observed_responses_v1'; t['code_observation_closed'] = True
        t['observed_code_responses'] = [{'url': u, 'resource_type': 'script', 'status': 200,
                                       'hash_state': 'hashed', 'sha256': d}
                                      for u, d in t['loaded_code_sha256'].items()]
        valid, _, _ = packer.validate(folder, m, 'low', (844, 390), BUILD, capture)
        self.assertFalse([c for c in valid if c['status'] == 'FAIL'])
        t['observed_code_responses'].append({'url': 'http://test/missing.js',
                                           'resource_type': 'script', 'status': 200,
                                           'hash_state': 'pending'})
        invalid, _, _ = packer.validate(folder, m, 'low', (844, 390), BUILD, capture)
        self.assertTrue([c for c in invalid if c['status'] == 'FAIL'])

    def test_capture_cli_returns_nonzero_and_keeps_failure_report(self):
        async def fake_capture(args, report, out):
            t = trip(); t.update(id='kessler', status='CAPTURING', shots=[])
            report['trips'].append(t); tasks = []
            capture.observe_response(t, Response(fail=True), tasks)
            try: await capture.wait_code_hashes(t, tasks)
            except RuntimeError:
                t['status'] = 'FAIL'
                raise
        with tempfile.TemporaryDirectory() as tmp, patch.object(capture, 'capture', fake_capture), \
                patch.object(capture, 'source_info', return_value={'git_head': 'mock'}), \
                patch('sys.stdout', new=io.StringIO()):
            out = Path(tmp) / 'new'
            self.assertEqual(capture.main([str(out), '--trip', 'kessler']), 1)
            self.assertEqual(json.loads((out / 'manifest.json').read_text())['status'], 'FAIL')


BUILD = json.loads((ROOT / 'build.json').read_text())


def main():
    p = argparse.ArgumentParser(description=__doc__); p.add_argument('--result', type=Path)
    args = p.parse_args()
    stream = io.StringIO()
    result = unittest.TextTestRunner(stream=stream, verbosity=2).run(
        unittest.defaultTestLoader.loadTestsFromModule(sys.modules[__name__]))
    revalidation = []
    for size in packer.SIZES:
        for quality in packer.QUALITIES:
            name = f'{quality}_{size[0]}x{size[1]}'; folder = ROOT / name
            before = packer.sha(folder / 'manifest.json')
            m = json.loads((folder / 'manifest.json').read_text())
            checks, originals, data = packer.validate(folder, m, quality, size, BUILD, capture)
            failures = [c for c in checks if c['status'] == 'FAIL']
            unchanged = all(packer.sha(path) == digest for path, digest in originals.items())
            revalidation.append({'config': name, 'registered_hash_and_image_integrity': 'FAIL' if failures or not unchanged else 'PASS',
                                 'code_response_coverage': 'UNVERIFIED: legacy capture has no observed response inventory',
                                 'checks_passed': sum(c['status'] == 'PASS' for c in checks),
                                 'checks_total': len(checks), 'failures': failures,
                                 'manifest_sha256': before, 'originals_unchanged': unchanged,
                                 'registered_code_files': len(data['resource_hashes'])})
    passed = result.wasSuccessful() and all(r['registered_hash_and_image_integrity'] == 'PASS' for r in revalidation)
    report = {'checked_utc': datetime.now(timezone.utc).isoformat(), 'status': 'PASS' if passed else 'FAIL',
              'scope': 'Targeted mocked response/error paths and read-only original revalidation. No game/browser integration rerun.',
              'regressions_run': result.testsRun, 'regression_failures': len(result.failures),
              'regression_errors': len(result.errors), 'regression_output': stream.getvalue(),
              'files_sha256': {str(path.relative_to(WEB)): packer.sha(path) for path in
                               (WEB / 'tools/drivelook.py', ROOT / 'package_evidence.py', Path(__file__))},
              'historical_revalidation': revalidation}
    if args.result: args.result.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(stream.getvalue(), end='')
    print(f'{report["status"]}: {result.testsRun} regressions, {len(revalidation)} historical configs; response coverage UNVERIFIED')
    return 0 if passed else 1


if __name__ == '__main__': raise SystemExit(main())
