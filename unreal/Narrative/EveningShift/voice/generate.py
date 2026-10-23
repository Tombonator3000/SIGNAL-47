#!/usr/bin/env python3
"""Resumable, serial VoiceStudio/OmniVoice production of the Evening Shift voice takes.

Reads the cue list from ../generated (recording_manifest.json + voice_lines.csv) and
the pinned profiles from casting.json. One GPU request at a time against the local
VoiceStudio API (default http://127.0.0.1:3900). Raw model WAVs are cached outside
the repository; finished clean takes (48 kHz PCM16 mono, trimmed, short fades,
constant gain, no channel filter) are written to --out. Only metadata is committed.

  python3 generate.py --limit 20          # next 20 missing takes
  python3 generate.py --only D01_FOOD_001 # one cue
  python3 generate.py --retake ID         # next seed for a broken take (logged in retakes.json)
  python3 generate.py --until-asr ID...   # retake with next seeds until ASR matches (max --attempts)
  python3 generate.py --sync DIR          # copy finished takes to DIR (e.g. Content/Audio/EveningShift)

speech-overrides.json may give a pronunciation spelling for one cue (same words,
e.g. 'Ray-yes?' for the name Reyes, which the model otherwise drops). The cue
text in the pack, manifest and subtitles stays exact.
"""
from pathlib import Path
import argparse
import array
import csv
import datetime
import hashlib
import json
import math
import shutil
import sys
import subprocess
import urllib.request
import uuid

HERE = Path(__file__).resolve().parent
PACK = HERE.parent
HOME_DATA = Path.home() / '.cache/signal47-eveningshift'
# Same request settings as the existing Kinski corpus (web/production/kinski).
# effect_preset 'broadcast' is VoiceStudio's documented default mastering chain
# (80 Hz highpass, gentle compression, mild shelf EQ, limiter); it is not a
# telephone/radio band filter. The channel filter is applied by the game.
BASE_SETTINGS = {'language': 'en', 'engine': 'omnivoice', 'num_step': 32, 'seed': 42,
                 'speed': 1, 'denoise': True, 'stream': True, 'effect_preset': 'broadcast'}
RATE = 48000
SOURCE_RATE = 24000
TARGET_ACTIVE_RMS_DBFS = -20.0   # Kinski/ensemble/radiocast corpus target
PEAK_CAP_DBFS = -2.0
GATE_DBFS = -50.0                # active-block gate, as in the precedent metrics
EDGE_GATE_DBFS = -45.0           # speech onset/offset detection for trimming
LEAD_PAD = 0.08
TAIL_PAD = 0.20
FADE_IN = 0.010
FADE_OUT = 0.030


def atomic_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n')
    temporary.replace(path)


def multipart(fields):
    boundary = 'signal47-' + uuid.uuid4().hex
    chunks = []
    for key, value in fields.items():
        if isinstance(value, bool):
            value = str(value).lower()
        chunks += [f'--{boundary}\r\nContent-Disposition: form-data; name="{key}"\r\n\r\n'.encode(),
                   str(value).encode(), b'\r\n']
    chunks.append(f'--{boundary}--\r\n'.encode())
    return b''.join(chunks), 'multipart/form-data; boundary=' + boundary


def api_generate(endpoint, fields):
    body, content_type = multipart(fields)
    request = urllib.request.Request(endpoint + '/generate', data=body,
                                     headers={'Content-Type': content_type}, method='POST')
    done = None
    with urllib.request.urlopen(request, timeout=300) as response:
        if 'application/json' in response.headers.get('Content-Type', ''):
            done = json.load(response)
        else:
            for raw in response:
                raw = raw.strip()
                if raw.startswith(b'data:'):
                    raw = raw[5:].strip()
                if not raw.startswith(b'{'):
                    continue
                message = json.loads(raw)
                if message.get('type') == 'error':
                    raise RuntimeError(message)
                if message.get('type') == 'done':
                    done = message
    if not done or not done.get('audio_path'):
        raise RuntimeError(f'Generation did not return audio_path: {done}')
    if done.get('dropped_chunks', 0):
        raise RuntimeError(f'Generation dropped chunks: {done}')
    return done


def decode(path, rate):
    raw = subprocess.check_output(['ffmpeg', '-hide_banner', '-loglevel', 'error', '-i', str(path),
                                   '-f', 'f32le', '-ac', '1', '-ar', str(rate), '-'])
    samples = array.array('f')
    samples.frombytes(raw)
    return samples


def db(x):
    return 20 * math.log10(x) if x > 0 else None


def block_levels(samples, rate, seconds=0.010):
    size = max(1, int(rate * seconds))
    return size, [math.sqrt(sum(x * x for x in samples[i:i + size]) / len(samples[i:i + size]))
                  for i in range(0, len(samples), size)]


def speech_bounds(samples, rate):
    size, levels = block_levels(samples, rate)
    gate = 10 ** (EDGE_GATE_DBFS / 20)
    active = [i for i, level in enumerate(levels) if level > gate]
    if not active:
        return None
    return active[0] * size / rate, min(len(samples), (active[-1] + 1) * size) / rate


def metrics(path, rate=RATE):
    samples = decode(path, rate)
    if not samples:
        raise ValueError(f'Empty audio: {path}')
    peak = max(abs(x) for x in samples)
    rms = math.sqrt(sum(x * x for x in samples) / len(samples))
    size = int(rate * 0.020)
    blocks = [samples[i:i + size] for i in range(0, len(samples), size)]
    active = [b for b in blocks if math.sqrt(sum(x * x for x in b) / len(b)) > 10 ** (GATE_DBFS / 20)]
    count = sum(len(b) for b in active)
    active_rms = math.sqrt(sum(x * x for b in active for x in b) / count) if count else 0
    bounds = speech_bounds(samples, rate)
    # Longest internal pause between detected speech blocks (garble/stall indicator).
    bsize, levels = block_levels(samples, rate)
    gate = 10 ** (EDGE_GATE_DBFS / 20)
    longest = run = 0
    started = False
    for level in levels:
        if level > gate:
            started = True
            longest = max(longest, run)
            run = 0
        elif started:
            run += 1
    edge = int(rate * 0.002)
    return {
        'duration_seconds': round(len(samples) / rate, 6),
        'peak_dbfs': db(peak), 'rms_dbfs': db(rms), 'nonsilent_rms_dbfs': db(active_rms),
        'nonsilent_seconds': round(count / rate, 4),
        'speech_start_seconds': round(bounds[0], 3) if bounds else None,
        'speech_end_seconds': round(bounds[1], 3) if bounds else None,
        'longest_internal_pause_seconds': round(longest * bsize / rate, 3),
        'first_2ms_peak': max(abs(x) for x in samples[:edge]),
        'last_2ms_peak': max(abs(x) for x in samples[-edge:]),
        'samples_at_or_above_full_scale': sum(abs(x) >= 1 for x in samples),
    }


def integrated_lufs(path):
    out = subprocess.run(['ffmpeg', '-hide_banner', '-nostats', '-i', str(path), '-af', 'ebur128=peak=true',
                          '-f', 'null', '-'], capture_output=True, text=True).stderr
    summary = out[out.rfind('Summary:'):]
    for line in summary.splitlines():
        line = line.strip()
        if line.startswith('I:'):
            return float(line.split()[1])
    return None


def clean_take(raw_wav, out_wav):
    """Trim to speech + pads, resample 24k->48k (soxr), constant gain, short fades, PCM16 mono."""
    source = decode(raw_wav, SOURCE_RATE)
    bounds = speech_bounds(source, SOURCE_RATE)
    if not bounds or bounds[1] - bounds[0] < 0.15:
        raise ValueError(f'No meaningful speech energy: {raw_wav.name}')
    total = len(source) / SOURCE_RATE
    start = max(0.0, bounds[0] - LEAD_PAD)
    end = min(total, bounds[1] + TAIL_PAD)
    length = end - start
    trimmed_raw = out_wav.with_suffix('.trim.wav')
    subprocess.run(['ffmpeg', '-hide_banner', '-loglevel', 'error', '-y', '-i', str(raw_wav),
                    '-af', f'atrim=start={start:.4f}:end={end:.4f},asetpts=PTS-STARTPTS',
                    '-ac', '1', '-c:a', 'pcm_f32le', str(trimmed_raw)], check=True)
    base = metrics(trimmed_raw, SOURCE_RATE)
    gain_db = min(TARGET_ACTIVE_RMS_DBFS - base['nonsilent_rms_dbfs'], PEAK_CAP_DBFS - 0.3 - base['peak_dbfs'])
    temporary = out_wav.with_suffix('.tmp.wav')
    for _ in range(4):
        chain = (f'aresample={RATE}:resampler=soxr:precision=28,volume={gain_db:.6f}dB,'
                 f'afade=t=in:st=0:d={FADE_IN},afade=t=out:st={length - FADE_OUT:.4f}:d={FADE_OUT}')
        subprocess.run(['ffmpeg', '-hide_banner', '-loglevel', 'error', '-y', '-i', str(trimmed_raw),
                        '-af', chain, '-ac', '1', '-ar', str(RATE), '-c:a', 'pcm_s16le',
                        '-map_metadata', '-1', '-fflags', '+bitexact', '-flags:a', '+bitexact',
                        str(temporary)], check=True)
        result = metrics(temporary)
        if result['peak_dbfs'] <= PEAK_CAP_DBFS:
            break
        gain_db += PEAK_CAP_DBFS - 0.1 - result['peak_dbfs']
    else:
        raise ValueError(f'Peak cap failed: {out_wav.name}')
    trimmed_raw.unlink()
    temporary.replace(out_wav)
    probe = json.loads(subprocess.check_output(['ffprobe', '-v', 'error', '-show_entries',
                       'stream=codec_name,sample_rate,channels,bits_per_sample', '-of', 'json', str(out_wav)]))
    stream = probe['streams'][0]
    if (stream['codec_name'], stream['sample_rate'], stream['channels']) != ('pcm_s16le', str(RATE), 1):
        raise ValueError(f'Wrong WAV format: {stream}')
    result.update({'trim_seconds': [round(start, 4), round(end, 4)], 'raw_duration_seconds': round(total, 4),
                   'constant_gain_db': round(gain_db, 4), 'integrated_lufs': integrated_lufs(out_wav),
                   'bytes': out_wav.stat().st_size, 'sha256': hashlib.sha256(out_wav.read_bytes()).hexdigest(),
                   'ffprobe': stream})
    return result


def cues():
    manifest = json.loads((PACK / 'generated/recording_manifest.json').read_text())
    texts = {row['line_id']: row for row in csv.DictReader((PACK / 'generated/voice_lines.csv').open(newline=''))}
    out = []
    for item in manifest['recordings']:
        if not item['recording_required']:
            continue
        out.append({**item, 'text': texts[item['line_id']]['text'], 'delivery_nb': texts[item['line_id']]['delivery_nb']})
    return out


def profile_list(endpoint):
    value = json.load(urllib.request.urlopen(endpoint + '/profiles', timeout=30))
    return value.get('profiles', []) if isinstance(value, dict) else value


def bind_profiles(endpoint, casting):
    live = {p['id']: p for p in profile_list(endpoint)}
    bound = {}
    for name, role in casting['profiles'].items():
        profile = live.get(role['profile_id'])
        if not profile or profile['name'] != role['profile_name'] or profile['kind'] != role['kind']:
            raise ValueError(f'Profile {role["profile_id"]} for {name} missing or renamed')
        reference = urllib.request.urlopen(endpoint + f"/profiles/{role['profile_id']}/audio", timeout=30).read()
        reference_hash = hashlib.sha256(reference).hexdigest()
        if reference_hash != role['reference_sha256']:
            raise ValueError(f'Reference for {name} changed: {reference_hash}')
        settings = {**BASE_SETTINGS, 'profile_id': role['profile_id']}
        identity = hashlib.sha256(json.dumps({'settings': settings, 'ref_sha256': reference_hash},
                                             sort_keys=True).encode()).hexdigest()[:16]
        bound[name] = {'settings': settings, 'reference_sha256': reference_hash, 'cache_identity': identity}
    return bound


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--endpoint', default='http://127.0.0.1:3900')
    parser.add_argument('--data', type=Path, default=HOME_DATA, help='raw cache and finished takes (outside Git)')
    parser.add_argument('--limit', type=int, default=0)
    parser.add_argument('--only', nargs='*', default=[])
    parser.add_argument('--retake', nargs='*', default=[], help='advance the seed by one for these cues')
    parser.add_argument('--reason', default='', help='why --retake was needed (stored in retakes.json)')
    parser.add_argument('--sync', type=Path, action='append', default=[])
    parser.add_argument('--until-asr', nargs='*', default=[])
    parser.add_argument('--attempts', type=int, default=6)
    args = parser.parse_args()
    takes_dir = args.data / 'takes'
    if args.sync:
        for target in args.sync:
            target.mkdir(parents=True, exist_ok=True)
            for wav in sorted(takes_dir.glob('*.wav')):
                shutil.copy2(wav, target / wav.name)
            print(json.dumps({'synced': len(list(takes_dir.glob('*.wav'))), 'to': str(target)}))
        return
    casting = json.loads((HERE / 'casting.json').read_text())
    retakes_path = HERE / 'retakes.json'
    retakes = json.loads(retakes_path.read_text()) if retakes_path.exists() else {'version': 1, 'cues': {}}
    for line_id in args.retake:
        entry = retakes['cues'].setdefault(line_id, {'seed': BASE_SETTINGS['seed'], 'history': []})
        entry['history'].append({'previous_seed': entry['seed'], 'reason': args.reason or 'unspecified',
                                 'utc': datetime.datetime.now(datetime.timezone.utc).isoformat()})
        entry['seed'] += 1
    if args.retake:
        atomic_json(retakes_path, retakes)
    overrides = json.loads((HERE / 'speech-overrides.json').read_text())['cues'] if (HERE / 'speech-overrides.json').exists() else {}
    bound = bind_profiles(args.endpoint, casting)
    report_path = HERE / 'generation.json'
    report = json.loads(report_path.read_text()) if report_path.exists() else {'takes': {}}
    report.update({'base_settings': BASE_SETTINGS, 'profiles': {k: {kk: v[kk] for kk in ('reference_sha256', 'cache_identity')}
                   | {'profile_id': v['settings']['profile_id']} for k, v in bound.items()},
                   'output_format': {'sample_rate_hz': RATE, 'sample_format': 'PCM16', 'channels': 1,
                                     'container': 'WAV'},
                   'human_listened': False, 'speaker_similarity': 'UNVERIFIED', 'emotional_delivery': 'UNVERIFIED'})
    takes_dir.mkdir(parents=True, exist_ok=True)
    count = 0
    todo = cues()
    for cue in todo:
        if args.only and cue['line_id'] not in args.only:
            continue
        profile = bound[cue['voice_profile']]
        seed = retakes['cues'].get(cue['line_id'], {}).get('seed', BASE_SETTINGS['seed'])
        spoken = overrides.get(cue['line_id'], {}).get('spoken', cue['text'])
        request = {**profile['settings'], 'seed': seed, 'text': spoken}
        take_identity = hashlib.sha256(json.dumps({'id': profile['cache_identity'], 'seed': seed, 'text': spoken},
                                                  sort_keys=True).encode()).hexdigest()[:16]
        out_wav = takes_dir / f"{cue['line_id']}.wav"
        item = report['takes'].get(cue['line_id'])
        if (item and item.get('take_identity') == take_identity and out_wav.exists()
                and hashlib.sha256(out_wav.read_bytes()).hexdigest() == item['audio']['sha256']):
            continue
        if args.limit and count >= args.limit:
            break
        cache = args.data / 'raw' / profile['cache_identity']
        cache.mkdir(parents=True, exist_ok=True)
        raw_wav = cache / f"{cue['line_id']}.{take_identity}.wav"
        response_file = raw_wav.with_suffix('.result.json')
        if not raw_wav.exists():
            result = api_generate(args.endpoint, request)
            audio = urllib.request.urlopen(args.endpoint + '/audio/' + result['audio_path'], timeout=60).read()
            temporary = raw_wav.with_suffix('.part')
            temporary.write_bytes(audio)
            temporary.replace(raw_wav)
            atomic_json(response_file, result)
        else:
            result = json.loads(response_file.read_text()) if response_file.exists() else {'cached': True}
        audio_metrics = clean_take(raw_wav, out_wav)
        report['takes'][cue['line_id']] = {
            'line_id': cue['line_id'], 'actor': cue['actor'], 'channel': cue['channel'],
            'voice_profile': cue['voice_profile'], 'text': cue['text'], 'request': request,
            'pronunciation_override': spoken != cue['text'],
            'response': result, 'raw_sha256': hashlib.sha256(raw_wav.read_bytes()).hexdigest(),
            'file': out_wav.name, 'audio': audio_metrics, 'take_identity': take_identity,
            'reference_sha256': profile['reference_sha256']}
        report['updated_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        report['takes'] = dict(sorted(report['takes'].items(), key=lambda kv: [c['line_id'] for c in todo].index(kv[0])))
        atomic_json(report_path, report)
        count += 1
        print(json.dumps({'done': len(report['takes']), 'total': len(todo), 'id': cue['line_id'], 'seed': seed,
                          'seconds': round(audio_metrics['duration_seconds'], 2), 'text': cue['text'][:50]}), flush=True)
    if args.until_asr:
        from verify import transcribe, tokens, edit_distance
        for line_id in args.until_asr:
            cue = next(c for c in todo if c['line_id'] == line_id)
            for attempt in range(args.attempts):
                heard = transcribe(args.endpoint, takes_dir / f'{line_id}.wav').get('text', '')
                if edit_distance(tokens(cue['text']), tokens(heard)) == 0:
                    print(json.dumps({'id': line_id, 'match': True, 'seed': report['takes'][line_id]['request']['seed'], 'asr': heard}), flush=True)
                    break
                print(json.dumps({'id': line_id, 'match': False, 'seed': report['takes'][line_id]['request']['seed'], 'asr': heard}), flush=True)
                if attempt == args.attempts - 1:
                    break
                subprocess.run([sys.executable, __file__, '--endpoint', args.endpoint, '--data', str(args.data),
                                '--retake', line_id, '--only', line_id, '--reason', f'ASR mismatch: {heard!r}'], check=True,
                               stdout=subprocess.DEVNULL)
                report = json.loads(report_path.read_text())
    print(json.dumps({'complete': sum((takes_dir / f"{c['line_id']}.wav").exists() for c in todo), 'total': len(todo)}), flush=True)


if __name__ == '__main__':
    main()
