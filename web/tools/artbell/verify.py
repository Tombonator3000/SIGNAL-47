#!/usr/bin/env python3
"""Verify catalog coverage/encoding and optionally a small dispersed ASR sample."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
import urllib.request
import uuid
from generate import metrics

ROOT = Path(__file__).resolve().parents[2]
PRODUCTION = ROOT / 'production/artbell'
ASSETS = ROOT / 'src/assets/audio/artbell'


def tokens(text):
    return re.findall(r"[\w]+(?:'[\w]+)?", text.lower())


def edit_distance(left, right):
    previous = list(range(len(right) + 1))
    for i, a in enumerate(left, 1):
        current = [i]
        for j, b in enumerate(right, 1):
            current.append(min(current[-1] + 1, previous[j] + 1, previous[j-1] + (a != b)))
        previous = current
    return previous[-1]


def transcribe(endpoint, file):
    boundary = 'signal47-asr-' + uuid.uuid4().hex
    body = (f'--{boundary}\r\nContent-Disposition: form-data; name="audio"; filename="sample.mp3"\r\n'
            'Content-Type: audio/mpeg\r\n\r\n').encode() + file.read_bytes() + b'\r\n'
    for key, value in {'language': 'en', 'mode': 'accurate', 'refine': 'false'}.items():
        body += f'--{boundary}\r\nContent-Disposition: form-data; name="{key}"\r\n\r\n{value}\r\n'.encode()
    body += f'--{boundary}--\r\n'.encode()
    request = urllib.request.Request(endpoint + '/transcribe', data=body,
                                     headers={'Content-Type': 'multipart/form-data; boundary=' + boundary})
    return json.load(urllib.request.urlopen(request, timeout=120))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--asr', type=int, default=0, help='Number of dispersed clips to transcribe, serially')
    parser.add_argument('--asr-new', type=int, default=0, help='Dispersed long clips absent from the previous verification report')
    parser.add_argument('--asr-text', action='append', default=[], help='Also transcribe this exact catalog text, repeatable')
    parser.add_argument('--keep-asr', action='store_true', help='Preserve historical ASR samples without retranscribing them')
    parser.add_argument('--endpoint', default='http://127.0.0.1:3900')
    args = parser.parse_args()
    corpus = json.loads((PRODUCTION / 'corpus.json').read_text())
    catalog = json.loads((ASSETS / 'catalog.json').read_text())
    generation = json.loads((PRODUCTION / 'generation.json').read_text())
    previous_file = PRODUCTION / 'verification.json'
    previous_report = json.loads(previous_file.read_text()) if previous_file.exists() else {}
    expected = {clip['id']: clip for clip in corpus['clips']}
    actual = {clip['id']: clip for clip in catalog['clips']}
    errors = []
    if catalog.get('templates') != corpus['templates'] or catalog.get('timeBank') != corpus['timeBank']:
        errors.append({'error': 'Runtime template/time-bank contract differs from corpus'})
    if catalog.get('voice') != 'Art Bell synthetic parody':
        errors.append({'error': 'Runtime voice label differs'})
    if any(path.is_file() and path.name != 'catalog.json' and path.suffix != '.mp3' for path in ASSETS.rglob('*')):
        errors.append({'error': 'Unexpected source/reference/model/utility file in runtime audio directory'})
    if len(actual) != len(catalog['clips']):
        errors.append({'error': 'Duplicate catalog ids'})
    if set(expected) != set(actual):
        errors.append({'missing_ids': sorted(set(expected)-set(actual)), 'extra_ids': sorted(set(actual)-set(expected))})
    expected_files = {clip['file'] for clip in catalog['clips']}
    orphan_files = sorted(file.name for file in ASSETS.glob('*.mp3') if file.name not in expected_files)
    if orphan_files:
        errors.append({'orphan_mp3_files': orphan_files})
    verified = []
    dropped_confirmed = 0
    dropped_unknown = []
    for clip in catalog['clips']:
        file = ASSETS / clip['file']
        if clip['file'] != clip['id'] + '.mp3' or not file.exists():
            errors.append({'id': clip['id'], 'error': 'Missing or invalid audio filename'})
            continue
        if clip['id'] != hashlib.sha256(clip['text'].encode()).hexdigest()[:16]:
            errors.append({'id': clip['id'], 'error': 'Text hash mismatch'})
        if clip['id'] not in expected:
            continue
        if clip['text'] != expected[clip['id']]['text']:
            errors.append({'id': clip['id'], 'error': 'Catalog text differs from corpus'})
        if clip['duration'] <= 0:
            errors.append({'id': clip['id'], 'error': 'Non-positive catalog duration'})
        audio = metrics(file)
        probe = json.loads(subprocess.check_output(['ffprobe', '-v', 'error', '-show_entries',
                     'stream=codec_name,sample_rate,channels', '-of', 'json', str(file)]))['streams'][0]
        if (probe['codec_name'], probe['sample_rate'], probe['channels']) != ('mp3', '24000', 1):
            errors.append({'id': clip['id'], 'error': 'Encoding differs', 'probe': probe})
        if audio['peak_dbfs'] > -2 or audio['nonsilent_seconds'] < 0.03:
            errors.append({'id': clip['id'], 'error': 'Peak or silence validation failed', 'audio': audio})
        if audio['duration_seconds'] <= 0:
            errors.append({'id': clip['id'], 'error': 'Non-positive decoded duration'})
        if abs(audio['duration_seconds'] - clip['duration']) > 0.05:
            errors.append({'id': clip['id'], 'error': 'Catalog duration differs'})
        if hashlib.sha256(file.read_bytes()).hexdigest() != generation['clips'][clip['id']]['audio']['sha256']:
            errors.append({'id': clip['id'], 'error': 'File hash differs from generation report'})
        response = generation['clips'][clip['id']]['response']
        if response.get('type') != 'done':
            errors.append({'id': clip['id'], 'error': 'No completed generation response'})
        if response.get('dropped_chunks') == 0:
            dropped_confirmed += 1
        elif response.get('dropped_chunks') is None:
            dropped_unknown.append(clip['id'])
            errors.append({'id': clip['id'], 'error': 'Dropped-chunk count was not recorded'})
        else:
            errors.append({'id': clip['id'], 'error': 'Generation reported dropped chunks', 'response': response})
        verified.append({'id': clip['id'], **audio})
    sample = list(previous_report.get('asr_sample', [])) if args.keep_asr else []
    selected = []
    if args.asr:
        # Avoid one-word acknowledgements: choose spread-out substantial passages.
        candidates = [clip for clip in corpus['clips'] if len(tokens(clip['text'])) >= 8]
        count = min(args.asr, len(candidates))
        indexes = sorted({round(i * (len(candidates)-1) / max(count-1, 1)) for i in range(count)})
        selected.extend(candidates[index] for index in indexes)
    if args.asr_new:
        previous_ids = {clip['id'] for clip in previous_report.get('audio', [])}
        candidates = [clip for clip in corpus['clips'] if clip['id'] not in previous_ids and len(tokens(clip['text'])) >= 30]
        count = min(args.asr_new, len(candidates))
        indexes = sorted({round(i * (len(candidates)-1) / max(count-1, 1)) for i in range(count)})
        selected.extend(candidates[index] for index in indexes)
    for text in args.asr_text:
        matches = [clip for clip in corpus['clips'] if clip['text'] == text]
        if not matches:
            raise ValueError(f'No exact catalog text for requested ASR sample: {text}')
        selected.extend(matches)
    if selected:
        unique = {clip['id']: clip for clip in selected}
        sample = [entry for entry in sample if entry['id'] not in unique]
        for clip in unique.values():
            response = transcribe(args.endpoint, ASSETS / (clip['id'] + '.mp3'))
            left, right = tokens(clip['text']), tokens(response['text'])
            distance = edit_distance(left, right)
            result = {'id': clip['id'], 'expected': clip['text'], 'recognized': response['text'],
                      'group': 'current_run',
                      'audio_sha256': hashlib.sha256((ASSETS / (clip['id'] + '.mp3')).read_bytes()).hexdigest(),
                      'strict_word_edits': distance, 'expected_word_count': len(left),
                      'strict_word_error_rate': distance / max(len(left), 1), 'response': response}
            sample.append(result)
            print(json.dumps({'asr_sample': clip['id'], 'strict_word_edits': distance}), flush=True)
    report = {
        'status': 'PASS' if not errors else 'FAIL', 'errors': errors,
        'clips': len(verified), 'bytes': sum((ASSETS / clip['file']).stat().st_size for clip in catalog['clips']),
        'duration_seconds': sum(clip['duration'] for clip in catalog['clips']),
        'audio': verified, 'asr_sample': sample,
        'catalog_ids_match_corpus': set(expected) == set(actual),
        'orphan_mp3_files': orphan_files,
        'dropped_chunks': {'confirmed_zero': dropped_confirmed, 'unrecorded_ids': dropped_unknown},
        'asr_interpretation': 'Automatic transcription is an imperfect word check, not a listening test. Strict edits also count number spelling and abbreviations.',
        'corpus_sha256': hashlib.sha256((PRODUCTION / 'corpus.json').read_bytes()).hexdigest(),
        'template_contract_matches': catalog.get('templates') == corpus['templates'] and catalog.get('timeBank') == corpus['timeBank'],
        'human_listened': False, 'speaker_similarity': 'UNVERIFIED', 'emotional_delivery': 'UNVERIFIED',
    }
    (PRODUCTION / 'verification.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ['status', 'clips', 'bytes', 'duration_seconds']}, indent=2))
    if errors:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
