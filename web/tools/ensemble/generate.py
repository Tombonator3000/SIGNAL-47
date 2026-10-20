#!/usr/bin/env python3
"""Serial, resumable production with explicit real profile/reference identities."""
from pathlib import Path
import argparse
import datetime
import hashlib
import json
import re
import subprocess
import urllib.request
from audio_common import atomic_json, generate, encode
from asr import cardinal

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / 'production/ensemble'
SETTINGS = {'language': 'en', 'engine': 'omnivoice', 'num_step': 32,
            'seed': 42, 'speed': 1, 'denoise': True, 'stream': True,
            'effect_preset': 'broadcast'}
LETTERS = dict(zip('ABCDEFGHIJKLMNOPQRSTUVWXYZ', [
    'Ay.', 'Bee.', 'See.', 'Dee.', 'Eee.', 'Eff.', 'Gee.', 'Aitch.',
    'Eye.', 'Jay.', 'Kay.', 'Ell.', 'Em.', 'En.', 'Oh.', 'Pee.', 'Cue.',
    'Ar.', 'Ess.', 'Tee.', 'You.', 'Vee.', 'Double you.', 'Ex.', 'Why.', 'Zee.']))


def spoken_text(clip, bank, corpus):
    if bank != 'serling':
        return clip['text']
    text = clip['text']
    # Explicit pronunciation of the same abbreviated field, initial or number.
    # Runtime wording and IDs never change. Requests/cache preserve the spelling.
    if text == 'T.': return 'Tee.'
    if text == ', el': return 'Elevation.'
    if text in LETTERS: return LETTERS[text]
    if text in corpus.get('numberWords', []): return text.capitalize() + '.'
    # These glyphs encode a pause, an identifier or a signed value. Speak their
    # actual meaning, never an identifier hyphen as a negative number.
    text=re.sub(r'\s*//\s*','. ',text)
    text=re.sub(r'(?:\.\s*){2,}', '. ',text)
    text=re.sub(r'\s+\.(?!\d)', '.',text)
    # A fragment-leading full stop/comma belongs to the preceding runtime
    # segment; an isolated punctuation token can otherwise be spoken as "dot".
    text=re.sub(r'^(?:\s+|[,:;]|\.(?!\d))+','',text)
    def identifier(match):
        letters,digits,suffix=match.groups()
        names=' '.join(LETTERS[c].rstrip('.') for c in letters)
        number=' '.join(cardinal(int(c)) for c in digits) if digits.startswith('0') else cardinal(int(digits))
        return names+' '+number+(' '+LETTERS[suffix].rstrip('.') if suffix else '')
    text=re.sub(r'\b([A-Z]{1,3})-?(\d{1,3})([A-Z]?)\b',identifier,text)
    def negative(match):
        whole,fraction=match.groups()
        integer=' '.join(cardinal(int(c)) for c in whole) if len(whole)>1 and whole.startswith('0') else cardinal(int(whole))
        return 'minus '+integer+(' point '+' '.join(cardinal(int(c)) for c in fraction) if fraction else '')
    text=re.sub(r'(?<![\w])[-−](\d+)(?:\.(\d+))?\b',negative,text)
    text=re.sub(r'\bLY\b','light years',text)
    card_labels={value for card in corpus.get('chapterCards',[]) for key,values in card['fields'].items() if key in ('closed','next','title') for value in values}
    if clip['text'] in card_labels:
        # All-caps display labels are spoken as ordinary sentences. A printed
        # divider is a pause, not a word. AZ names the same azimuth field.
        text=text.replace(' / ', ', ')
        text=re.sub(r'\bAZ\b','azimuth',text)
        text=text[:1].upper()+text[1:].lower()
    return ' '.join(text.split())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--bank', choices=['ensemble', 'serling'], required=True)
    parser.add_argument('--voice')
    parser.add_argument('--endpoint', default='http://127.0.0.1:3900')
    parser.add_argument('--limit', type=int, default=0)
    parser.add_argument('--ids', help='Comma-separated IDs for targeted, documented word-quality corrections')
    parser.add_argument('--cache-dir', type=Path, default=Path.home() / '.cache/signal47-ensemble')
    args = parser.parse_args()
    production = BASE if args.bank == 'ensemble' else BASE / 'serling'
    assets = ROOT / 'src/assets/audio' / args.bank
    corpus = json.loads((production / 'corpus.json').read_text())
    overrides_file = production / 'request-overrides.json'
    overrides = json.loads(overrides_file.read_text()) if overrides_file.exists() else {}
    selected_ids = set(args.ids.split(',')) if args.ids else None
    config = json.loads((BASE / 'profiles.json').read_text())
    role_of = lambda clip: clip.get('voice', 'serling')
    roles = sorted({role_of(c) for c in corpus['clips']})
    identities = {}
    for role in roles:
        if args.voice and role != args.voice: continue
        if role not in config['roles']: raise ValueError('No validated clone profile: ' + role)
        saved = config['roles'][role]
        profile = json.load(urllib.request.urlopen(args.endpoint + '/profiles/' + saved['profile_id']))
        if profile['kind'] != 'clone': raise ValueError('Expected an actual reference-based clone: ' + role)
        reference = urllib.request.urlopen(args.endpoint + '/profiles/' + saved['profile_id'] + '/audio').read()
        ref_hash = hashlib.sha256(reference).hexdigest()
        if ref_hash != saved['profile_reference_sha256']: raise ValueError('Reference changed: ' + role)
        identities[role] = hashlib.sha256(json.dumps({'settings': SETTINGS, 'bank': args.bank,
            'role': role, 'profile_id': saved['profile_id'], 'reference_sha256': ref_hash}, sort_keys=True).encode()).hexdigest()[:16]
    report_file = production / 'generation.json'
    report = json.loads(report_file.read_text()) if report_file.exists() else {'clips': {}}
    report.update({'settings': SETTINGS, 'roles': {r: {key: config['roles'][r][key] for key in
        ['actor', 'profile_id', 'profile_reference_sha256']} for r in roles if r in config['roles']},
        'human_listened': False, 'speaker_similarity': 'UNVERIFIED', 'emotional_delivery': 'UNVERIFIED'})
    catalog = {'version': 1, 'voice': 'Synthetic character voice ensemble' if args.bank == 'ensemble' else 'Rod Serling synthetic parody narration',
        'clips': [{'id': c['id'], **({'voice': c['voice']} if args.bank == 'ensemble' else {}),
                   'text': c['text'], 'duration': 0, 'file': c['id'] + '.mp3'} for c in corpus['clips']]}
    assets.mkdir(parents=True, exist_ok=True)
    def identity(clip):
        role = role_of(clip)
        base = identities.get(role)
        if not base: return None
        spoken = spoken_text(clip, args.bank, corpus)
        suffix = '\0' + json.dumps(overrides[clip['id']],sort_keys=True) if clip['id'] in overrides else ''
        return hashlib.sha256((base + '\0' + spoken + suffix).encode()).hexdigest()[:16]
    for i, clip in enumerate(corpus['clips']):
        item = report['clips'].get(clip['id']); file = assets / (clip['id'] + '.mp3')
        # Unselected roles keep already completed rows, never lose their durations.
        good_identity = item and (role_of(clip) not in identities or item.get('cache_identity') == identity(clip))
        if good_identity and file.exists():
            if hashlib.sha256(file.read_bytes()).hexdigest() != item['audio']['sha256']: raise ValueError('Unexpected asset edit: ' + file.name)
            catalog['clips'][i]['duration'] = item['audio']['duration_seconds']
    atomic_json(assets / 'catalog.json', catalog)
    print(json.dumps({'bank': args.bank, 'resumed': sum(c['duration'] > 0 for c in catalog['clips']), 'total': len(catalog['clips'])}), flush=True)
    count = 0
    for i, clip in enumerate(corpus['clips']):
        role = role_of(clip)
        if args.voice and role != args.voice: continue
        if selected_ids and clip['id'] not in selected_ids: continue
        if catalog['clips'][i]['duration'] > 0: continue
        if args.limit and count >= args.limit: break
        cache_id = identity(clip)
        cache = args.cache_dir / args.bank / cache_id; cache.mkdir(parents=True, exist_ok=True)
        wav = cache / (clip['id'] + '.wav'); result_path = cache / (clip['id'] + '.result.json')
        request = {**SETTINGS, 'profile_id': config['roles'][role]['profile_id'], 'text': spoken_text(clip, args.bank, corpus)}
        request.update({k:v for k,v in overrides.get(clip['id'],{}).items() if k in ('seed','text')})
        if not wav.exists():
            response = generate(args.endpoint, request)
            audio = urllib.request.urlopen(args.endpoint + '/audio/' + response['audio_path'], timeout=60).read()
            temp = wav.with_suffix('.tmp'); temp.write_bytes(audio); temp.replace(wav)
            atomic_json(result_path, response)
        else: response = json.loads(result_path.read_text())
        override = overrides.get(clip['id'], {})
        trim = override.get('trim_seconds')
        rendered = wav
        if trim:
            if len(trim) != 2 or not 0 <= trim[0] < trim[1] <= response['duration'] + .001:
                raise ValueError('Invalid documented carrier trim: ' + clip['id'])
            rendered = cache / (clip['id'] + '.trimmed.wav')
            subprocess.run(['ffmpeg', '-v', 'error', '-y', '-i', str(wav),
                '-ss', str(trim[0]), '-t', str(trim[1]-trim[0]),
                '-ac', '1', '-ar', '24000', '-c:a', 'pcm_s16le', str(rendered)], check=True)
        pcm = encode(rendered, assets / (clip['id'] + '.mp3'))
        report['clips'][clip['id']] = {'id': clip['id'], 'voice': role, 'text': clip['text'], 'request': request,
            'pronunciation_override': request['text'] != clip['text'], 'response': response, 'audio': pcm,
            'cache_identity': cache_id, 'reference_sha256': config['roles'][role]['profile_reference_sha256']}
        if trim:
            report['clips'][clip['id']]['carrier_trim_seconds'] = trim
            report['clips'][clip['id']]['rendered_text'] = override['rendered_text']
            report['clips'][clip['id']]['uncut_wav_sha256'] = hashlib.sha256(wav.read_bytes()).hexdigest()
        catalog['clips'][i]['duration'] = pcm['duration_seconds']
        report['updated_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        atomic_json(report_file, report); atomic_json(assets / 'catalog.json', catalog)
        count += 1
        print(json.dumps({'bank': args.bank, 'done': sum(c['duration'] > 0 for c in catalog['clips']), 'total': len(catalog['clips']),
            'voice': role, 'id': clip['id'], 'seconds': round(pcm['duration_seconds'], 2), 'text': clip['text'][:70]}), flush=True)


if __name__ == '__main__': main()
