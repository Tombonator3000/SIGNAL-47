#!/usr/bin/env python3
"""Focused regression checks for drivelook's console failure policy; no browser."""
import json
from pathlib import Path
import unittest

from drivelook import record_console


class ConsolePolicy(unittest.TestCase):
    def setUp(self):
        self.trip = {'console_messages': [], 'errors': [], 'warnings': []}
        self.missing = 'Failed to load resource: the server responded with a status of 404 ()'
        self.icon = {'url': 'https://tombonator3000.github.io/favicon.ico', 'lineNumber': 0}
        self.page_url = 'https://tombonator3000.github.io/SIGNAL-47/'

    def record(self, text=None, location=None, kind='error'):
        record_console(self.trip, kind, self.missing if text is None else text,
                       self.icon if location is None else location, page_url=self.page_url)

    def test_confirmed_favicon_remains_visible_without_blocking(self):
        self.record()
        event = self.trip['console_messages'][0]
        self.assertEqual(event['text'], self.missing)
        self.assertEqual(event['location'], self.icon)
        self.assertEqual(event['non_blocking_reason'], 'missing_favicon_http_404')
        self.assertFalse(self.trip['errors'])
        self.assertIn(self.icon['url'], self.trip['warnings'][0])
        self.assertEqual(json.loads(json.dumps(self.trip)), self.trip)

    def test_recorded_oldroad_messages_can_be_replayed(self):
        path = Path(__file__).resolve().parents[1] / 'production/drivelook_20261006/capture_oldroad.json'
        original = json.loads(path.read_text())['trips'][0]['console_messages']
        self.assertEqual(len(original), 2)
        for event in original:
            record_console(self.trip, event['type'], event['text'], event['location'], page_url=self.page_url)
        self.assertEqual(len(self.trip['warnings']), 2)
        self.assertFalse(self.trip['errors'])
        self.assertEqual([e['location'] for e in self.trip['console_messages']], [e['location'] for e in original])

    def test_unknown_source_still_blocks(self):
        for location in ({}, {'url': ''}):
            with self.subTest(location=location):
                self.record(location=location)
        self.assertEqual(self.trip['errors'], [self.missing] * 2)

    def test_game_resource_404_still_blocks(self):
        self.record(location={'url': 'https://tombonator3000.github.io/SIGNAL-47/assets/game.js'})
        self.assertEqual(self.trip['errors'], [self.missing])

    def test_similar_filename_is_not_a_favicon(self):
        for path in ('/favicon.ico.js', '/favicon.ico/other', '/assets/favicon.ico'):
            with self.subTest(path=path):
                self.record(location={'url': 'https://tombonator3000.github.io' + path})
        self.assertEqual(len(self.trip['errors']), 3)

    def test_other_favicon_failures_still_block(self):
        for message in (self.missing.replace('404', '500'), self.missing.replace('404', '403'), 'net::ERR_CONNECTION_REFUSED',
                        'Error in game: favicon missing', 'favicon.ico 404'):
            with self.subTest(message=message):
                self.record(text=message)
        self.assertEqual(len(self.trip['errors']), 5)

    def test_other_origin_still_blocks(self):
        for url in ('https://other.invalid/favicon.ico', 'http://tombonator3000.github.io/favicon.ico',
                    'https://tombonator3000.github.io:8443/favicon.ico'):
            with self.subTest(url=url):
                self.record(location={'url': url})
        self.assertEqual(len(self.trip['errors']), 3)

    def test_non_http_or_malformed_source_still_blocks(self):
        for url in ('file:///favicon.ico', 'data:/favicon.ico', '/favicon.ico', 'https://[invalid/favicon.ico'):
            with self.subTest(url=url):
                self.record(location={'url': url})
        self.assertEqual(len(self.trip['errors']), 4)

    def test_browser_reason_phrase_and_query_are_accepted(self):
        self.page_url = 'http://localhost:8765/SIGNAL-47/'
        self.record(text=self.missing.replace('()', '(Not Found)'),
                    location={'url': 'http://localhost:8765/favicon.ico?v=1'})
        self.assertFalse(self.trip['errors'])
        self.assertEqual(len(self.trip['warnings']), 1)

    def test_warnings_and_normal_logs_keep_their_policy(self):
        self.record(text='warning about the game', kind='warning')
        self.record(text='info', kind='log')
        self.assertEqual(self.trip['warnings'], ['warning about the game'])
        self.assertFalse(self.trip['errors'])
        self.assertEqual(len(self.trip['console_messages']), 1)

    def test_favicon_diagnostic_does_not_hide_later_game_error(self):
        self.record()
        self.record(text='TypeError: game.update is not a function',
                    location={'url': 'https://tombonator3000.github.io/SIGNAL-47/assets/game.js'})
        self.assertEqual(self.trip['errors'], ['TypeError: game.update is not a function'])
        self.assertEqual(len(self.trip['console_messages']), 2)


if __name__ == '__main__':
    unittest.main(verbosity=2)
