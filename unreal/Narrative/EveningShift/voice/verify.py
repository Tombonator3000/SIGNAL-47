#!/usr/bin/env python3
"""Technical and ASR check of the finished Evening Shift takes; writes verification.json and takes.json.

Run only after all TTS work is finished (ASR uses the same GPU through VoiceStudio's
/transcribe, faster-whisper). takes.json is the committed, audio-derived timing table that
build_pack.py reads to fill recording_manifest.json and subtitles.json.

  python3 verify.py            # format/PCM checks + ASR of every take
  python3 verify.py --no-asr   # format/PCM checks, keep previous ASR results
"""
from pathlib import Path
import argparse
import datetime
import hashlib
import json
import re
import subprocess
import sys
import urllib.request
import uuid

HERE = Path(__file__).resolve().parent
sys.dont_write_bytecode = True
sys.path.insert(0, str(HERE))
from generate import HOME_DATA, RATE, PEAK_CAP_DBFS, TARGET_ACTIVE_RMS_DBFS, cues, metrics, atomic_json  # noqa: E402

ONES = 'zero one two three four five six seven eight nine ten eleven twelve thirteen fourteen fifteen sixteen seventeen eighteen nineteen'.split()
TENS = 'twenty thirty forty fifty sixty seventy eighty ninety'.split()
EQUIVALENT = {'ok': 'okay', 'alright': 'all right'}


def number_words(n):
    if n < 20:
        return ONES[n]
    if n < 100:
        return TENS[n // 10 - 2] + ('' if n % 10 == 0 else ' ' + ONES[n % 10])
    return str(n)


def tokens(text):
    text = text.lower().replace('-', ' - ')
    text = re.sub(r'(?<![\w])- (\d)', r'minus \1', text)
    text = re.sub(r'\d+', lambda m: number_words(int(m.group())), text)
    for a, b in EQUIVALENT.items():
        text = re.sub(rf'\b{a}\b', b, text)
    return re.findall(r"[a-z]+(?:'[a-z]+)?", text)


def strict_tokens(text):
    return re.findall(r"[\w]+(?:'[\w]+)?", text.lower())


def edit_distance(left, right):
    previous = list(range(len(right) + 1))
    for i, a in enumerate(left, 1):
        current = [i]
        for j, b in enumerate(right, 1):
            current.append(min(current[-1] + 1, previous[j] + 1, previous[j - 1] + (a != b)))
        previous = current
    return previous[-1]


def transcribe(endpoint, file):
    boundary = 'signal47-asr-' + uuid.uuid4().hex
    body = (f'--{boundary}\r\nContent-Disposition: form-data; name="audio"; filename="sample.wav"\r\n'
            'Content-Type: application/octet-stream\r\n\r\n').encode() + file.read_bytes() + b'\r\n'
    for key, value in {'language': 'en', 'mode': 'accurate', 'refine': 'false'}.items():
        body += f'--{boundary}\r\nContent-Disposition: form-data; name="{key}"\r\n\r\n{value}\r\n'.encode()
    body += f'--{boundary}--\r\n'.encode()
    request = urllib.request.Request(endpoint + '/transcribe', data=body,
                                     headers={'Content-Type': 'multipart/form-data; boundary=' + boundary})
    return json.load(urllib.request.urlopen(request, timeout=180))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--endpoint', default='http://127.0.0.1:3900')
    parser.add_argument('--data', type=Path, default=HOME_DATA)
    parser.add_argument('--no-asr', action='store_true')
    parser.add_argument('--only', nargs='*', default=[])
    args = parser.parse_args()
    generation = json.loads((HERE / 'generation.json').read_text())
    review = json.loads((HERE / 'asr-review.json').read_text()) if (HERE / 'asr-review.json').exists() else {'cues': {}}
    previous_path = HERE / 'verification.json'
    previous = json.loads(previous_path.read_text()) if previous_path.exists() else {'takes': {}}
    rows, failures, missing = {}, [], []
    todo = cues()
    for cue in todo:
        line_id = cue['line_id']
        wav = args.data / 'takes' / f'{line_id}.wav'
        item = generation['takes'].get(line_id)
        if not wav.exists() or not item:
            missing.append(line_id)
            continue
        digest = hashlib.sha256(wav.read_bytes()).hexdigest()
        probe = json.loads(subprocess.check_output(['ffprobe', '-v', 'error', '-show_entries',
                           'stream=codec_name,sample_rate,channels,bits_per_sample:format=format_name', '-of', 'json', str(wav)]))
        stream = probe['streams'][0]
        measured = metrics(wav)
        problems = []
        if (stream['codec_name'], stream['sample_rate'], stream['channels'], probe['format']['format_name']) != ('pcm_s16le', str(RATE), 1, 'wav'):
            problems.append(f'format {stream}')
        if digest != item['audio']['sha256']:
            problems.append('sha256 differs from generation.json')
        if measured['peak_dbfs'] > PEAK_CAP_DBFS + 1e-6 or measured['samples_at_or_above_full_scale']:
            problems.append('peak above cap / clipping')
        if abs(measured['nonsilent_rms_dbfs'] - TARGET_ACTIVE_RMS_DBFS) > 3.0:
            problems.append(f"active RMS {measured['nonsilent_rms_dbfs']:.2f} dBFS outside ±3 dB of target")
        if measured['first_2ms_peak'] > 0.01 or measured['last_2ms_peak'] > 0.01:
            problems.append('edge not faded (click risk)')
        if not measured['speech_start_seconds'] or measured['speech_start_seconds'] < 0.03:
            problems.append('speech starts at the file edge')
        if measured['duration_seconds'] - (measured['speech_end_seconds'] or 0) < 0.05:
            problems.append('speech runs into the file end')
        old = previous['takes'].get(line_id, {})
        if args.no_asr or (args.only and line_id not in args.only):
            asr = old.get('asr') if old.get('sha256') == digest else None
        else:
            result = transcribe(args.endpoint, wav)
            text = result.get('text') or ' '.join(s.get('text', '') for s in result.get('segments', []))
            expected, heard = tokens(cue['text']), tokens(text)
            asr = {'engine': 'faster-whisper via VoiceStudio /transcribe (accurate, en)', 'text': text.strip(),
                   'strict_word_edits': edit_distance(strict_tokens(cue['text']), strict_tokens(text)),
                   'normalized_word_edits': edit_distance(expected, heard), 'expected_tokens': len(expected)}
        if asr is None:
            problems.append('no ASR for this audio')
        accepted = review['cues'].get(line_id)
        if asr and asr['normalized_word_edits']:
            if accepted and accepted.get('sha256') == digest:
                asr['review'] = accepted['verdict']
            else:
                problems.append(f"ASR differs: {asr['text']!r}")
        rows[line_id] = {'actor': cue['actor'], 'voice_profile': cue['voice_profile'], 'file': wav.name,
                         'sha256': digest, 'seed': item['request']['seed'],
                         'duration_seconds': measured['duration_seconds'],
                         'speech_start_seconds': measured['speech_start_seconds'],
                         'speech_end_seconds': measured['speech_end_seconds'],
                         'peak_dbfs': round(measured['peak_dbfs'], 3), 'nonsilent_rms_dbfs': round(measured['nonsilent_rms_dbfs'], 3),
                         'integrated_lufs': item['audio']['integrated_lufs'],
                         'longest_internal_pause_seconds': measured['longest_internal_pause_seconds'],
                         'asr': asr, 'problems': problems}
        if problems:
            failures.append(line_id)
        print(json.dumps({'id': line_id, 'ok': not problems, 'asr': (asr or {}).get('text', '')[:70], 'problems': problems}), flush=True)
    by_actor = {}
    for row in rows.values():
        entry = by_actor.setdefault(row['actor'], {'takes': 0, 'seconds': 0.0})
        entry['takes'] += 1
        entry['seconds'] = round(entry['seconds'] + row['duration_seconds'], 3)
    asr_rows = [r['asr'] for r in rows.values() if r['asr']]
    lufs = [r['integrated_lufs'] for r in rows.values() if r['integrated_lufs'] is not None]
    report = {
        'version': 1, 'completed_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'status': 'PASS' if rows and not failures and not missing else 'FAIL',
        'word_status': ('CHECKED_WITH_UNRESOLVED_ASR' if any(r['asr'] and str(r['asr'].get('review', '')).startswith('unresolved') for r in rows.values())
                        else 'CHECKED'),
        'unresolved_asr': sorted(k for k, r in rows.items() if r['asr'] and str(r['asr'].get('review', '')).startswith('unresolved')),
        'required_takes': len(todo), 'present_takes': len(rows), 'missing': missing, 'failures': failures,
        'format': f'WAV PCM16 mono {RATE} Hz', 'loudness_target': f'{TARGET_ACTIVE_RMS_DBFS} dBFS active RMS, peak <= {PEAK_CAP_DBFS} dBFS',
        'by_actor': by_actor, 'total_seconds': round(sum(r['duration_seconds'] for r in rows.values()), 3),
        'integrated_lufs_range': [min(lufs), max(lufs)] if lufs else None,
        'asr_summary': {'checked': len(asr_rows), 'strict_exact': sum(a['strict_word_edits'] == 0 for a in asr_rows),
                        'normalized_exact': sum(a['normalized_word_edits'] == 0 for a in asr_rows),
                        'reviewed_differences': sorted(k for k, r in rows.items() if r['asr'] and r['asr'].get('review'))},
        'alias': {'D61_FIXED_FRAGMENT_001': 'D01_HANDOVER_001 (same file, no new take)'},
        'human_listened': False, 'speaker_similarity': 'UNVERIFIED', 'emotional_delivery': 'UNVERIFIED',
        'takes': rows}
    atomic_json(previous_path, report)
    takes = {'version': 1, 'note_nb': 'Avledet fra de ferdige WAV-filene av verify.py. Tider er sekunder i hvert klipp (tale start/slutt).',
             'format': {'sample_rate_hz': RATE, 'sample_format': 'PCM16', 'channels': 1},
             'takes': {k: {'file': f"EveningShift/{r['file']}", 'sha256': r['sha256'], 'duration_seconds': r['duration_seconds'],
                           'speech_start_seconds': r['speech_start_seconds'], 'speech_end_seconds': r['speech_end_seconds']}
                       for k, r in rows.items() if not r['problems']}}
    atomic_json(HERE / 'takes.json', takes)
    print(json.dumps({k: report[k] for k in ('status', 'word_status', 'unresolved_asr', 'present_takes', 'missing', 'failures', 'by_actor', 'asr_summary')}, indent=1))
    return 0 if report['status'] == 'PASS' else 1


if __name__ == '__main__':
    raise SystemExit(main())
