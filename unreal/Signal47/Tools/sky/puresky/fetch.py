#!/usr/bin/env python3
"""Fetch the pinned, CC0 Pure Sky source; no editor or project asset mutations."""
from pathlib import Path
import hashlib
import json
import urllib.request

root = Path(__file__).resolve().parent
manifest = json.loads((root / 'source.json').read_text())
target = Path.home() / manifest['local_source_relative']
target.parent.mkdir(parents=True, exist_ok=True)
if not target.exists():
    partial = target.with_suffix('.download')
    request = urllib.request.Request(manifest['download_url'], headers={'User-Agent': 'Signal47 asset integration (local development)'})
    with urllib.request.urlopen(request, timeout=120) as source, partial.open('wb') as output:
        while chunk := source.read(1024 * 1024):
            output.write(chunk)
    assert partial.stat().st_size == manifest['bytes']
    with partial.open('rb') as stream:
        assert hashlib.file_digest(stream, 'sha256').hexdigest() == manifest['sha256']
    partial.rename(target)
with target.open('rb') as stream:
    assert hashlib.file_digest(stream, 'sha256').hexdigest() == manifest['sha256'], 'Existing source differs; preserved for inspection'
print(target)
