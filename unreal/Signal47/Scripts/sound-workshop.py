#!/usr/bin/env python3
"""Prepare local reference recordings for S47SoundMixer. No assets enter Content or Git."""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys
import tempfile
import wave

PROJECT = Path(__file__).resolve().parents[1]
RECIPES = PROJECT / 'Tools/sound-workshop/recipes.json'
DEFAULT_SOURCES = Path.home() / 'Documents/ChatGPT/Signal 47/lydreferanser'
MARKER = '.sound-workshop'
# Source start/duration, deliberately retaining only short useful textures except the handshake.
SOURCES = {
    'contact': ('-3OWC2640dU', 18, 12, False),
    'old_vhs': ('0TK2RWIIHJs', 4, 16, False),
    'vhs_noise': ('vTGcaK0LOto', 8, 16, False),
    'broken_radio': ('al0OUayfKZs', 2, 16, False),
    'radio_noise': ('pumgHCBx9iY', 4, 16, False),
    'modem': ('gsNaR6FRuO0', 0, 28, True),
}
USES = {'receiver', 'vhs', 'pc_transfer'}
ENVELOPES = {'constant', 'static', 'signal', 'pulse', 'activity'}
LAYER_FIELDS = {'source', 'gain_db', 'rate', 'highpass_hz', 'lowpass_hz',
                'start_s', 'length_s', 'loop', 'envelope'}


def read_json(path):
    return json.loads(path.read_text(encoding='utf-8'))


def write_json(path, data):
    temp = path.with_suffix(path.suffix + '.tmp')
    temp.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    temp.replace(path)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def number(value, low, high, label):
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ValueError(f'{label}: expected number')
    if not math.isfinite(value) or not low <= value <= high:
        raise ValueError(f'{label}: outside {low}..{high}')
    return value


def pcm_info(path):
    with wave.open(str(path), 'rb') as wav:
        if (wav.getnchannels(), wav.getsampwidth(), wav.getframerate(), wav.getcomptype()) != (1, 2, 48000, 'NONE'):
            raise ValueError(f'{path}: expected mono PCM16 / 48000 Hz')
        frames = wav.getnframes()
        if frames == 0 or len(wav.readframes(frames)) != frames * 2:
            raise ValueError(f'{path}: empty or truncated PCM')
        return frames / 48000


def validate(bank, root):
    if not isinstance(bank, dict) or bank.get('version') != 1:
        raise ValueError('bank version must be 1')
    sources = bank.get('sources')
    if not isinstance(sources, dict) or not sources:
        raise ValueError('sources must be a nonempty object')
    durations = {}
    for key, source in sources.items():
        if not isinstance(source, dict) or not isinstance(source.get('file'), str) or type(source.get('modem_only')) is not bool:
            raise ValueError(f'{key}: file and modem_only are required')
        relative = Path(source['file'])
        path = (root / relative).resolve()
        if relative.is_absolute() or not path.is_relative_to(root.resolve()):
            raise ValueError(f'{key}: source must remain inside bank directory')
        if 'modem' in key.lower() and not source['modem_only']:
            raise ValueError(f'{key}: modem sources cannot be unreserved')
        durations[key] = pcm_info(path)
    recipes = bank.get('recipes')
    if not isinstance(recipes, list) or not recipes:
        raise ValueError('recipes must be a nonempty list')
    seen = set()
    for recipe in recipes:
        if not isinstance(recipe, dict) or set(recipe) != {'name', 'use', 'layers'}:
            raise ValueError('recipe requires name, use and layers')
        name = recipe['name']
        if not isinstance(name, str) or not name or name in seen:
            raise ValueError('recipe names must be unique and nonempty')
        seen.add(name)
        if recipe['use'] not in USES:
            raise ValueError(f'{name}: unsupported use')
        layers = recipe['layers']
        if not isinstance(layers, list) or not 1 <= len(layers) <= 16:
            raise ValueError(f'{name}: expected 1..16 layers')
        for i, layer in enumerate(layers):
            label = f'{name} layer {i}'
            if not isinstance(layer, dict) or set(layer) - LAYER_FIELDS:
                raise ValueError(f'{label}: unknown layer fields')
            key = layer.get('source')
            if key not in sources:
                raise ValueError(f'{label}: unknown source {key}')
            if sources[key]['modem_only'] and recipe['use'] != 'pc_transfer':
                raise ValueError(f'{label}: modem is reserved for PC transfers')
            number(layer.get('gain_db', -18), -96, 0, label + ' gain_db')
            number(layer.get('rate', 1), 0.1, 4, label + ' rate')
            hp = number(layer.get('highpass_hz', 0), 0, 20000, label + ' highpass_hz')
            lp = number(layer.get('lowpass_hz', 0), 0, 20000, label + ' lowpass_hz')
            if hp and lp and hp >= lp:
                raise ValueError(f'{label}: highpass must be below lowpass')
            start = number(layer.get('start_s', 0), 0, durations[key], label + ' start_s')
            length = number(layer.get('length_s', 0), 0, durations[key], label + ' length_s')
            if start >= durations[key] or (length and start + length > durations[key] + 1 / 48000):
                raise ValueError(f'{label}: selection extends beyond source')
            if type(layer.get('loop', True)) is not bool or layer.get('envelope', 'constant') not in ENVELOPES:
                raise ValueError(f'{label}: invalid loop or envelope')
    return len(sources), len(recipes)


def locate(root, video_id):
    matches = [p for p in root.rglob('*.mp3') if video_id in p.name]
    # Contact already existed as contact-signal.mp3 in a directory containing the id.
    if not matches:
        matches = [p for p in root.rglob('*.mp3') if video_id in str(p.parent)]
    if len(matches) != 1:
        raise ValueError(f'{video_id}: expected exactly one MP3 source, found {len(matches)}')
    return matches[0]


def prepare(sources_root, output, recipe_path):
    sources_root = sources_root.resolve()
    output = output.resolve()
    if output == sources_root or output.is_relative_to(sources_root) or sources_root.is_relative_to(output):
        raise ValueError('generated bank and original recordings must use separate directories')
    if output.exists() and any(output.iterdir()) and not (output / MARKER).is_file():
        raise ValueError(f'{output}: nonempty directory is not an owned sound workshop')
    originals = {key: locate(sources_root, config[0]) for key, config in SOURCES.items()}
    hashes = {key: digest(path) for key, path in originals.items()}
    recipes = read_json(recipe_path)
    # Validate all recipe/category controls before touching existing prepared files.
    with tempfile.TemporaryDirectory(prefix='s47-sound-preflight-') as scratch:
        temp_root = Path(scratch)
        temp_bank = {'version': 1, 'sources': {}, 'recipes': recipes}
        for key, (_, _, duration, reserved) in SOURCES.items():
            path = temp_root / f'{key}.wav'
            with wave.open(str(path), 'wb') as wav:
                wav.setparams((1, 2, 48000, 0, 'NONE', 'not compressed'))
                wav.writeframes(b'\0\0' * int(duration * 48000))
            temp_bank['sources'][key] = {'file': path.name, 'modem_only': reserved}
        validate(temp_bank, temp_root)
    output.mkdir(parents=True, exist_ok=True)
    (output / MARKER).write_text('S47 local audio workshop v1\n', encoding='utf-8')
    bank = {'version': 1, 'sources': {}, 'recipes': recipes}
    for key, (video_id, start, duration, reserved) in SOURCES.items():
        source = originals[key]
        target = output / f'{key}.wav'
        temp = output / f'{key}.preparing.wav'
        subprocess.run(['ffmpeg', '-nostdin', '-v', 'error', '-y', '-ss', str(start), '-i', str(source),
                        '-t', str(duration), '-vn', '-ac', '1', '-ar', '48000', '-c:a', 'pcm_s16le', str(temp)], check=True)
        pcm_info(temp)
        temp.replace(target)
        bank['sources'][key] = {'file': target.name, 'modem_only': reserved,
                               'source_url': f'https://www.youtube.com/watch?v={video_id}',
                               'source_file': str(source), 'source_start_s': start,
                               'source_sha256': hashes[key]}
    counts = validate(bank, output)
    if any(digest(originals[key]) != sha for key, sha in hashes.items()):
        raise ValueError('original recording changed during preparation')
    write_json(output / 'bank.json', bank)
    print(f'Prepared {counts[0]} sources and {counts[1]} recipes: {output / "bank.json"}')
    print(f'Unreal argument for previews: -S47SoundWorkshop="{output / "bank.json"}"')
    print('Original recordings preserved. Generated references remain local in Saved, outside Content cooking.')


def self_test():
    with tempfile.TemporaryDirectory(prefix='s47-sound-test-') as scratch:
        root = Path(scratch)
        wav = root / 'test.wav'
        with wave.open(str(wav), 'wb') as out:
            out.setparams((1, 2, 48000, 0, 'NONE', 'not compressed'))
            out.writeframes(b'\0\0' * 48000)
        bank = {'version': 1, 'sources': {'modem': {'file': 'test.wav', 'modem_only': True}},
                'recipes': [{'name': 'test', 'use': 'pc_transfer', 'layers': [{'source': 'modem'}]}]}
        assert validate(bank, root) == (1, 1)
        rejected = 0
        for change in ('receiver', 'unreserve', 'escape', 'nan', 'outside', 'filter'):
            wrong = copy.deepcopy(bank)
            layer = wrong['recipes'][0]['layers'][0]
            if change == 'receiver': wrong['recipes'][0]['use'] = 'receiver'
            elif change == 'unreserve': wrong['sources']['modem']['modem_only'] = False
            elif change == 'escape': wrong['sources']['modem']['file'] = '../test.wav'
            elif change == 'nan': layer['gain_db'] = float('nan')
            elif change == 'outside': layer.update(start_s=0.8, length_s=0.5)
            elif change == 'filter': layer.update(highpass_hz=2000, lowpass_hz=1000)
            try:
                validate(wrong, root)
            except ValueError:
                rejected += 1
        assert rejected == 6
        assert wav.is_file() and pcm_info(wav) == 1
    print('Sound workshop self-test: valid bank and 6 invalid banks checked; source preserved.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sources-root', type=Path, default=DEFAULT_SOURCES)
    parser.add_argument('--output', type=Path, default=PROJECT / 'Saved/AudioWorkshop')
    parser.add_argument('--recipes', type=Path, default=RECIPES)
    parser.add_argument('--check', action='store_true', help='validate prepared bank without conversion')
    parser.add_argument('--self-test', action='store_true')
    args = parser.parse_args()
    if args.self_test:
        self_test()
    elif args.check:
        counts = validate(read_json(args.output / 'bank.json'), args.output)
        print(f'Valid PCM bank: {counts[0]} sources, {counts[1]} recipes; modem limited to pc_transfer.')
    else:
        prepare(args.sources_root, args.output, args.recipes)


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, subprocess.CalledProcessError, wave.Error) as exc:
        sys.exit(f'S47 sound workshop: {exc}')
