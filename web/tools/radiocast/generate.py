#!/usr/bin/env python3
"""Resumable, serial VoiceStudio generation for the fictional offline radio cast.

Run extract.mjs first. API access and cached WAVs are build-time only. No source
reference audio, service URL or local machine paths are put in the runtime catalog.
"""
from pathlib import Path
import argparse
import array
import datetime
import hashlib
import json
import math
import subprocess
import urllib.request
import uuid

ROOT = Path(__file__).resolve().parents[2]
PRODUCTION = ROOT / 'production/radiocast'
ASSETS = ROOT / 'src/assets/audio/radiocast'
SETTINGS = {
    'language': 'en', 'engine': 'omnivoice',
    'num_step': 32, 'seed': 42, 'speed': 1, 'denoise': True, 'stream': True,
    'effect_preset': 'broadcast',
}
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


def generate(endpoint, fields):
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


def decoded(path):
    raw = subprocess.check_output(['ffmpeg', '-hide_banner', '-loglevel', 'error', '-i', str(path),
                                   '-f', 'f32le', '-ac', '1', '-ar', '24000', '-'])
    samples = array.array('f')
    samples.frombytes(raw)
    return samples


def metrics(path):
    samples = decoded(path)
    if not samples:
        raise ValueError(f'Empty audio: {path.name}')
    peak = max(abs(sample) for sample in samples)
    rms = math.sqrt(sum(sample * sample for sample in samples) / len(samples))
    blocks = [samples[i:i+480] for i in range(0, len(samples), 480)]
    non_silent = [block for block in blocks
                  if math.sqrt(sum(x*x for x in block) / len(block)) > 10 ** (-50/20)]
    count = sum(len(block) for block in non_silent)
    active_rms = math.sqrt(sum(x*x for block in non_silent for x in block) / count) if count else 0
    db = lambda x: 20 * math.log10(x) if x > 0 else None
    return {
        'duration_seconds': len(samples) / 24000, 'peak_dbfs': db(peak),
        'rms_dbfs': db(rms), 'nonsilent_rms_dbfs': db(active_rms),
        'nonsilent_seconds': count / 24000,
        'samples_at_or_above_full_scale': sum(abs(x) >= 1 for x in samples),
    }


def encode(wav, mp3):
    raw = metrics(wav)
    if raw['nonsilent_seconds'] < 0.03 or raw['peak_dbfs'] is None:
        raise ValueError(f'No meaningful speech energy: {wav.name}')
    # One constant gain preserves timing, inflection and dynamic shape. Check the
    # decoded MP3 too, since codec reconstruction can overshoot the input peak.
    gain_db = min(-20 - raw['nonsilent_rms_dbfs'], -2.2 - raw['peak_dbfs'])
    for attempt in range(3):
        subprocess.run(['ffmpeg', '-hide_banner', '-loglevel', 'error', '-y', '-i', str(wav),
                        '-af', f'volume={gain_db:.8f}dB', '-ac', '1', '-ar', '24000',
                        '-c:a', 'libmp3lame', '-b:a', '64k', '-map_metadata', '-1', str(mp3)], check=True)
        result = metrics(mp3)
        if result['peak_dbfs'] <= -2:
            break
        gain_db += -2.1 - result['peak_dbfs']
    else:
        raise ValueError(f'MP3 peak cap failed: {mp3.name}')
    probe = json.loads(subprocess.check_output(['ffprobe', '-v', 'error', '-show_entries',
                         'stream=codec_name,sample_rate,channels,bit_rate', '-of', 'json', str(mp3)]))
    stream = probe['streams'][0]
    if (stream['codec_name'], stream['sample_rate'], stream['channels']) != ('mp3', '24000', 1):
        raise ValueError(f'Wrong MP3 format: {stream}')
    result.update({'constant_gain_db': gain_db, 'bytes': mp3.stat().st_size,
                   'sha256': hashlib.sha256(mp3.read_bytes()).hexdigest(), 'ffprobe': stream})
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--endpoint', default='http://127.0.0.1:3900')
    parser.add_argument('--cache-dir', type=Path, default=Path.home() / '.cache/signal47-radiocast')
    parser.add_argument('--limit', type=int, default=0)
    parser.add_argument('--voice', choices=['woman','man','old','boy','news'])
    args = parser.parse_args()
    corpus = json.loads((PRODUCTION / 'corpus.json').read_text())
    casting = json.loads((PRODUCTION / 'casting.json').read_text())
    profiles = {p['id']: p for p in json.load(urllib.request.urlopen(args.endpoint + '/profiles'))}
    references, identities = {}, {}
    for role, value in casting['roles'].items():
        profile = profiles[value['profile_id']]
        reference = urllib.request.urlopen(args.endpoint + f"/profiles/{profile['id']}/audio").read()
        reference_hash = hashlib.sha256(reference).hexdigest()
        if reference_hash != value['profile_reference_sha256']:
            raise ValueError(f'Profile reference changed for {role}')
        references[role] = reference_hash
        identities[role] = hashlib.sha256(json.dumps({'settings': SETTINGS, 'role': role,
             'profile_id': profile['id'], 'reference_sha256': reference_hash}, sort_keys=True).encode()).hexdigest()[:16]
    ASSETS.mkdir(parents=True, exist_ok=True)
    catalog = {'version': 1, 'voice': 'Fictional synthetic radio cast', 'clips': [
        {'id': c['id'], 'voice': c['voice'], 'text': c['text'], 'duration': 0, 'file': c['id'] + '.mp3'} for c in corpus['clips']]}
    previous = PRODUCTION / 'generation.json'
    report = json.loads(previous.read_text()) if previous.exists() else {'clips': {}}
    report.update({'settings': SETTINGS, 'roles': casting['roles'], 'cache_identities': identities,
                   'human_listened': False, 'role_suitability': 'UNVERIFIED by listening'})
    for clip in catalog['clips']:
        item = report['clips'].get(clip['id']); file = ASSETS / clip['file']
        if item and item.get('cache_identity') == identities[clip['voice']] and file.exists():
            if hashlib.sha256(file.read_bytes()).hexdigest() != item['audio']['sha256']:
                raise ValueError(f'Asset changed outside generator: {file.name}')
            clip['duration'] = item['audio']['duration_seconds']
    atomic_json(ASSETS / 'catalog.json', catalog)
    print(json.dumps({'resumed': sum(c['duration'] > 0 for c in catalog['clips']), 'total': len(catalog['clips'])}), flush=True)
    count = 0
    for index, clip in enumerate(corpus['clips']):
        role = clip['voice']; identity = identities[role]
        if args.voice and args.voice != role: continue
        item = report['clips'].get(clip['id'])
        if catalog['clips'][index]['duration'] > 0: continue
        if args.limit and count >= args.limit: break
        cache = args.cache_dir / identity; cache.mkdir(parents=True, exist_ok=True)
        wav = cache / (clip['id'] + '.wav'); mp3 = ASSETS / (clip['id'] + '.mp3')
        response_file = cache / (clip['id'] + '.result.json')
        request = {**SETTINGS, 'profile_id': casting['roles'][role]['profile_id'], 'text': clip['text']}
        if not wav.exists():
            result = generate(args.endpoint, request)
            audio = urllib.request.urlopen(args.endpoint + '/audio/' + result['audio_path'], timeout=60).read()
            temporary = wav.with_suffix('.tmp'); temporary.write_bytes(audio); temporary.replace(wav)
            atomic_json(response_file, result)
        else:
            result = json.loads(response_file.read_text())
        audio_metrics = encode(wav, mp3)
        report['clips'][clip['id']] = {'id': clip['id'], 'voice': role, 'text': clip['text'], 'request': request,
             'response': result, 'audio': audio_metrics, 'cache_identity': identity, 'reference_sha256': references[role]}
        catalog['clips'][index]['duration'] = audio_metrics['duration_seconds']
        report['updated_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        atomic_json(PRODUCTION / 'generation.json', report); atomic_json(ASSETS / 'catalog.json', catalog)
        count += 1
        print(json.dumps({'done': sum(c['duration'] > 0 for c in catalog['clips']), 'total': len(catalog['clips']),
            'voice': role, 'id': clip['id'], 'seconds': round(audio_metrics['duration_seconds'], 2), 'text': clip['text'][:70]}), flush=True)
    print(json.dumps({'complete': sum(c['duration'] > 0 for c in catalog['clips']), 'total': len(catalog['clips'])}), flush=True)

if __name__ == '__main__':
    main()
