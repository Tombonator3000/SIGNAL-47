#!/usr/bin/env python3
"""Validate Content/Director/director.json and embed it, so that a missing or broken file on disk gives the same rhythm and a log line."""
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
import director_rules
root = Path(__file__).resolve().parents[2]
src = root / 'Content/Director/director.json'
doc = director_rules.load(src)
errors, warnings = director_rules.check(doc)
for w in warnings: print('warning:', w)
for e in errors: print('error:', e)
if errors: sys.exit(1)
out = root / 'Source/Signal47/Private/S47RhythmDefaults.inl'
out.write_text('// Generated from Content/Director/director.json. Rebuild with Tools/director/generate.py.\nstatic const TCHAR* S47RhythmDefaultJson=TEXT(R"S47JSON(' + src.read_text(encoding='utf-8') + ')S47JSON");\n', encoding='utf-8')
print(f"{len(doc['events'])} rhythm events validated and embedded")
