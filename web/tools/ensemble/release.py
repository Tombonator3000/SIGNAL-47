#!/usr/bin/env python3
"""Publish the GPU-idle signal only after the final, current CPU reports pass."""
from pathlib import Path
import hashlib
import json
from audio_common import atomic_json

ROOT=Path(__file__).resolve().parents[2]
BASE=ROOT/'production/ensemble'
def sha(file): return hashlib.sha256(file.read_bytes()).hexdigest()

def main():
    verification=json.loads((BASE/'verification.json').read_text())
    preservation=json.loads((BASE/'preservation.json').read_text())
    provenance=json.loads((BASE/'provenance.json').read_text())
    assert verification['status']==preservation['status']==provenance['verification']['status']=='PASS'
    assert verification['clips']==1502
    hashes={bank:sha((BASE if bank=='ensemble' else BASE/'serling')/'corpus.json') for bank in ['ensemble','serling']}
    assert hashes==provenance['corpus_sha256']
    for bank in hashes:
        assert verification['banks'][bank]['corpus_sha256']==hashes[bank]
        assert verification['banks'][bank]['catalog_sha256']==sha(ROOT/'src/assets/audio'/bank/'catalog.json')
    ready={'gpu_idle':True,'verification':'PASS','clips':1502,'ensemble_clips':100,'serling_clips':1402,'corpus_sha256':hashes,'verification_sha256':sha(BASE/'verification.json'),'preservation_sha256':sha(BASE/'preservation.json'),'provenance_sha256':sha(BASE/'provenance.json'),'bytes':verification['bytes'],'duration_seconds':verification['duration_seconds']}
    atomic_json(BASE/'qa/tts-ready.json',ready)
    print(json.dumps(ready))

if __name__=='__main__': main()
