#!/usr/bin/env python3
"""Dale's own Freeform takes as the isolated "dale" voice bank (DALE-WIRE-20261109).

The game (FS47DaleVoice) looks a line of his up in Content/Audio/Raw/voices.json under "dale",
by the manifest id or by its exact text, and plays the take through FS47RecordedDialogue on the
walkie or the service line, with the caption timed by the take's measured speech. The bank holds
the Freeform lines of manifest.json with status voice (Story keeps its own banks), each with a
take that is there and agrees with takes.json (verify.py: hash, length, speech start and end).
A line without one stays text in the game, as before. No takes at all: an empty bank, which the
game treats as no bank. The committed catalog.json holds no audio.

  python3 catalog.py                       # catalog.json from takes.json and the synced WAVs
  python3 catalog.py --check               # fail if catalog.json is out of date
  python3 catalog.py --import-raw ../../Signal47/Content/Audio/Raw
                                           # copy the WAVs to Raw/voice/dale, publish voices.json["dale"]
  python3 catalog.py --voice dale_de_design --import-raw ../../Signal47/Content/Audio/Raw
                                           # an audition candidate's takes from the cache, for a listen
  python3 catalog.py --self-test           # made-up takes in a temporary folder; nothing else touched

Takes are read from --wavs, else Signal47/Content/Audio/DaleVoice (generate.py --sync) for the
voice dale, else ~/.cache/signal47-dale-voice/<voice>/takes. Other banks in voices.json are kept
as they are; audio-raw.py calls import_raw after it has rebuilt the file.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import shutil
import sys
import tempfile
import wave

HERE = Path(__file__).resolve().parent
UNREAL = HERE.parent.parent
SYNCED = UNREAL / 'Signal47/Content/Audio/DaleVoice'
CACHE = Path.home() / '.cache/signal47-dale-voice'
BANK = 'dale'
FOLDER = '/Game/Audio/Voice/Dale'
RATE = 48000


def clean(text):
    """As FS47RadioVoice and FS47DaleVoice keep a text: whitespace made single, the DALE: head off."""
    text = ' '.join(text.split())
    return ' '.join(text[5:].split()) if text.startswith('DALE:') else text


def meta_for(voice):
    return HERE if voice == 'dale' else CACHE / voice / 'meta'


def wavs_for(voice):
    if voice == 'dale' and SYNCED.is_dir() and any(SYNCED.glob('*.wav')):
        return SYNCED
    return CACHE / voice / 'takes'


def read_json(path, default=None):
    return json.loads(path.read_text(encoding='utf-8')) if path.exists() else default


def catalog(manifest, takes, wav_dir, voice='dale'):
    """The "dale" bank entry. Takes missing, stale or of the wrong format are left out and named."""
    lines = [l for l in manifest['lines'] if l.get('pack') == 'freeform' and l.get('status') == 'voice']
    ids = [l['id'] for l in lines]
    if len(set(ids)) != len(ids):
        raise ValueError('Duplicate Freeform line ids in manifest.json')
    measured = (takes or {}).get('takes', {})
    clips, skipped, lookup = [], {}, {}
    for line in lines:
        line_id = line['id']
        key = clean(line['text'])
        if key in lookup:
            raise ValueError(f'Text lookup collision: {line_id} and {lookup[key]}')
        lookup[key] = line_id
        meta = measured.get(line_id)
        path = Path(wav_dir) / f'{line_id}.wav'
        if not meta:
            skipped[line_id] = 'no take in takes.json'
            continue
        if not path.is_file():
            skipped[line_id] = 'no WAV'
            continue
        try:
            with wave.open(str(path), 'rb') as wav:
                if (wav.getnchannels(), wav.getsampwidth(), wav.getframerate(), wav.getcomptype()) != (1, 2, RATE, 'NONE'):
                    skipped[line_id] = 'not 48 kHz PCM16 mono'
                    continue
                frames = wav.getnframes()
                if len(wav.readframes(frames)) != frames * 2:
                    skipped[line_id] = 'truncated'
                    continue
        except (wave.Error, EOFError) as exc:
            skipped[line_id] = f'unreadable: {exc}'
            continue
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        duration = frames / RATE
        if digest != meta.get('sha256') or abs(float(meta.get('duration_seconds', -1)) - duration) > 1 / RATE:
            skipped[line_id] = 'stale: WAV differs from takes.json'
            continue
        start, end = meta.get('speech_start_seconds'), meta.get('speech_end_seconds')
        if not (isinstance(start, (int, float)) and isinstance(end, (int, float)) and 0 <= start < end <= duration):
            skipped[line_id] = 'no valid speech window'
            continue
        clips.append({'id': line_id, 'text': line['text'], 'voice': 'dale', 'channel': line['channel'],
                      'file': f'{line_id}.wav', 'asset': f'v_{line_id}', 'raw': f'voice/{BANK}/{line_id}.wav',
                      'duration': duration, 'frames': frames, 'sha256': digest,
                      'speech_start_seconds': start, 'speech_end_seconds': end,
                      'kind': line.get('kind'), 'source': line.get('source'), 'reuse_of': None})
    return {BANK: {'folder': FOLDER, 'catalog': {'version': 1, 'voice': f'Dale, fictional synthetic voice ({voice})', 'clips': clips},
                   'validation': {'pack_id': 'signal47_dale_freeform', 'voice': voice, 'line_count': len(lines),
                                  'clip_count': len(clips), 'skipped': skipped,
                                  'sample_rate_hz': RATE, 'sample_format': 'PCM16', 'channels': 1, 'human_listened': False,
                                  'scope': 'Freeform lines with status voice; templates and Story lines are not in this bank.'}}}


def build(voice='dale', wav_dir=None, takes_path=None):
    manifest = read_json(HERE / 'manifest.json')
    takes = read_json(Path(takes_path) if takes_path else meta_for(voice) / 'takes.json', {'takes': {}})
    return catalog(manifest, takes, Path(wav_dir) if wav_dir else wavs_for(voice), voice)


def lookup(entry, text_or_id, channel=None):
    """The clip the game would find: by id, else by exact text (DALE: head allowed), on that channel."""
    for clip in entry[BANK]['catalog']['clips']:
        if (clip['id'] == text_or_id or clean(clip['text']) == clean(text_or_id)) and (channel is None or clip['channel'] == channel):
            return clip
    return None


def import_raw(entry, target, wav_dir=None, voice='dale'):
    """Copy the takes into target/voice/dale and publish voices.json["dale"]; every other bank stays as it was."""
    target = Path(target)
    source = Path(wav_dir) if wav_dir else wavs_for(voice)
    index = target / 'voices.json'
    before = read_json(index, {})
    folder = target / 'voice' / BANK
    folder.mkdir(parents=True, exist_ok=True)
    keep = set()
    for clip in entry[BANK]['catalog']['clips']:
        destination = target / clip['raw']
        shutil.copyfile(source / clip['file'], destination)
        if hashlib.sha256(destination.read_bytes()).hexdigest() != clip['sha256']:
            raise ValueError(f'Imported recording hash mismatch: {clip["id"]}')
        keep.add(destination.name)
    for old in folder.glob('*.wav'):
        if old.name not in keep:
            old.unlink()
    after = {**before, **entry}
    with tempfile.NamedTemporaryFile(mode='w', encoding='utf-8', dir=target, prefix='.voices-dale-', delete=False) as stream:
        json.dump(after, stream, ensure_ascii=False, indent=1, allow_nan=False)
        stream.write('\n')
        temporary = Path(stream.name)
    os.replace(temporary, index)
    actual = read_json(index)
    if any(actual.get(k) != v for k, v in before.items() if k != BANK):
        raise ValueError('Unrelated voice bank changed')
    return len(entry[BANK]['catalog']['clips'])


def self_test():
    ok = True

    def check(cond, what):
        nonlocal ok
        print(('ok' if cond else 'FAIL') + ' dale catalog: ' + what)
        ok &= bool(cond)

    manifest = read_json(HERE / 'manifest.json')
    hello = next(l for l in manifest['lines'] if l['id'] == 'FF_BARK_hello')
    with tempfile.TemporaryDirectory(prefix='s47-dale-catalog-') as tmp:
        tmp = Path(tmp)
        wavs, raw = tmp / 'takes', tmp / 'Raw'
        wavs.mkdir()
        raw.mkdir()
        empty = catalog(manifest, {'takes': {}}, wavs)
        check(empty[BANK]['catalog']['clips'] == [] and empty[BANK]['folder'] == FOLDER, 'no takes: an empty bank in the same pattern as the others')
        check(lookup(empty, hello['text']) is None, 'no takes: nothing found, the line stays text')
        path = wavs / 'FF_BARK_hello.wav'
        with wave.open(str(path), 'wb') as wav:
            wav.setnchannels(1); wav.setsampwidth(2); wav.setframerate(RATE); wav.writeframes(b'\0\0' * RATE)
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        takes = {'takes': {'FF_BARK_hello': {'file': 'DaleVoice/FF_BARK_hello.wav', 'sha256': digest, 'duration_seconds': 1.0,
                                              'speech_start_seconds': 0.08, 'speech_end_seconds': 0.8}}}
        one = catalog(manifest, takes, wavs)
        clips = one[BANK]['catalog']['clips']
        check(len(clips) == 1 and clips[0]['id'] == 'FF_BARK_hello' and clips[0]['channel'] == 'walkie'
              and clips[0]['raw'] == 'voice/dale/FF_BARK_hello.wav' and clips[0]['speech_end_seconds'] == 0.8, 'one take: one clip with its id, channel and speech window')
        check(lookup(one, 'FF_BARK_hello') is clips[0] and lookup(one, hello['text']) is clips[0]
              and lookup(one, 'DALE: ' + hello['text']) is clips[0] and lookup(one, 'DALE:' + hello['text']) is clips[0],
              'found by id, by exact text and with the DALE: head')
        check(lookup(one, hello['text'], 'phone') is None, 'not on the telephone')
        stale = json.loads(json.dumps(takes)); stale['takes']['FF_BARK_hello']['sha256'] = '0' * 64
        check(catalog(manifest, stale, wavs)[BANK]['validation']['skipped'].get('FF_BARK_hello', '').startswith('stale'), 'a stale take is left out and named')
        (raw / 'voices.json').write_text(json.dumps({'kinski': {'folder': '/Game/Audio/Voice/Kinski', 'catalog': {'clips': []}}}))
        (raw / 'voice' / BANK).mkdir(parents=True)
        (raw / 'voice' / BANK / 'FF_OLD.wav').write_bytes(b'old')
        check(import_raw(one, raw, wavs) == 1, 'imported one take')
        after = json.loads((raw / 'voices.json').read_text())
        check(after['kinski'] == {'folder': '/Game/Audio/Voice/Kinski', 'catalog': {'clips': []}} and after[BANK]['catalog']['clips'][0]['id'] == 'FF_BARK_hello',
              'voices.json: the other banks as they were, and the dale bank')
        check(sorted(p.name for p in (raw / 'voice' / BANK).iterdir()) == ['FF_BARK_hello.wav'], 'Raw/voice/dale: the take, and no take left over from before')
    return ok


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--voice', default='dale')
    p.add_argument('--wavs', type=Path)
    p.add_argument('--takes', type=Path)
    p.add_argument('--output', type=Path, default=HERE / 'catalog.json')
    p.add_argument('--check', action='store_true')
    p.add_argument('--import-raw', type=Path)
    p.add_argument('--self-test', action='store_true')
    a = p.parse_args()
    if a.self_test:
        raise SystemExit(0 if self_test() else 1)
    entry = build(a.voice, a.wavs, a.takes)
    text = json.dumps(entry, ensure_ascii=False, indent=2, allow_nan=False) + '\n'
    if a.check:
        if not a.output.exists() or a.output.read_text(encoding='utf-8') != text:
            raise SystemExit('Catalog differs from the takes and the manifest: run catalog.py')
    elif a.voice == 'dale':
        a.output.write_text(text, encoding='utf-8')
    if a.import_raw:
        print('Imported recordings:', import_raw(entry, a.import_raw, a.wavs, a.voice))
    v = entry[BANK]['validation']
    print(f'{"PASS" if v["clip_count"] else "EMPTY"}: {v["clip_count"]} of {v["line_count"]} Freeform lines with a take ({a.voice}); {len(v["skipped"])} as text')
