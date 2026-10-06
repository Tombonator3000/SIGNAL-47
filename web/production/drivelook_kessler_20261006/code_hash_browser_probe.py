#!/usr/bin/env python3
"""Small real-Playwright response/close probe; no SIGNAL / 47 gameplay run.

Usage: S47_CHROMIUM=/path/to/chromium python code_hash_browser_probe.py RESULT.json
"""
import asyncio
from datetime import datetime, timezone
import hashlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import os
from pathlib import Path
import sys
from threading import Thread

from code_hash_check import capture, trip

FILES = {'/': ('text/html', b'<!doctype html><link rel="icon" href="data:,"><script src="/test.js"></script><p>Hash fixture</p>'),
         '/test.js': ('text/javascript', b'window.fixtureReady = true;')}


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *_): pass
    def do_GET(self):
        mime, body = FILES.get(self.path, ('text/plain', b'missing'))
        self.send_response(200 if self.path in FILES else 404)
        self.send_header('Content-Type', mime); self.end_headers(); self.wfile.write(body)


class ForcedReadFailure:
    def __init__(self, response):
        self.url, self.status, self.request = response.url, response.status, response.request
    async def body(self): raise RuntimeError('forced real-response wrapper body failure')


async def probe(url):
    from playwright.async_api import async_playwright
    outcomes = []
    async with async_playwright() as p:
        opts = {'headless': True}
        if os.environ.get('S47_CHROMIUM'): opts['executable_path'] = os.environ['S47_CHROMIUM']
        browser = await p.chromium.launch(**opts)
        try:
            for forced in (False, True):
                context = await browser.new_context(); page = await context.new_page()
                t, tasks = trip(), []
                def observe(response):
                    if forced and response.request.resource_type == 'script': response = ForcedReadFailure(response)
                    capture.observe_response(t, response, tasks)
                page.on('response', observe)
                await page.goto(url, wait_until='load')
                # Resolve bodies before close, then verify removal on a closed real Page.
                blocked = False
                try: await capture.wait_code_hashes(t, tasks)
                except RuntimeError: blocked = True
                await context.close(); t['code_observation_closed'] = True
                page.remove_listener('response', observe); t['code_observation_sealed'] = True
                try: await capture.wait_code_hashes(t, tasks)
                except RuntimeError: blocked = True
                expected = {url + path.lstrip('/'): hashlib.sha256(body).hexdigest() for path, (_, body) in FILES.items()}
                ok = (blocked and any(r['hash_state'] == 'failed' for r in t['observed_code_responses'])) if forced else (
                    not blocked and not capture.code_hash_errors(t, require_closed=True) and t['loaded_code_sha256'] == expected)
                frozen = json.dumps(t, sort_keys=True)
                await asyncio.sleep(0)
                ok = ok and frozen == json.dumps(t, sort_keys=True) and all(task.done() for task in tasks)
                outcomes.append({'check': 'forced script body failure blocks' if forced else 'real document/script bytes and closed-page listener cleanup',
                                 'status': 'PASS' if ok else 'FAIL', 'blocked': blocked, 'trip': t})
        finally: await browser.close()
    return outcomes


server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
thread = Thread(target=server.serve_forever, daemon=True); thread.start()
try: outcomes = asyncio.run(probe(f'http://127.0.0.1:{server.server_port}/'))
finally: server.shutdown(); server.server_close(); thread.join()
result = {'checked_utc': datetime.now(timezone.utc).isoformat(),
          'status': 'PASS' if all(o['status'] == 'PASS' for o in outcomes) else 'FAIL',
          'scope': 'Two tiny real-browser response/closure probes, no game build or rendering.',
          'files_sha256': {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in (Path(__file__), capture.WEB / 'tools/drivelook.py')},
          'outcomes': outcomes}
Path(sys.argv[1]).write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
print(result['status'] + ': 2 real-browser response probes')
raise SystemExit(0 if result['status'] == 'PASS' else 1)
