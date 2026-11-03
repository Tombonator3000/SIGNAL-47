#!/usr/bin/env python3
"""Prepare local reference recordings for S47SoundMixer. Prepared assets remain local; --stage-runtime explicitly includes them in local cooking."""
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
DEFAULT_OBSERVATORY = Path.home() / 'scratch-bm/Signal47-array-sounds-20261028'
DEFAULT_NEBULA = Path.home() / 'Assets/ultimate-sfx/Ultimate SFX for Games - $25 Content/bt25_ultimatesfxforgames_softwarebundle/Ambience & SFX/Ambience SciFi/FL_GFX_Ambience_SciFi_Space_Nebula_Cm.wav'
# Preserved observatory recordings, with provenance from that collection's SOURCES.md.
SCIENTIFIC = {
    'nasa_cassini_jupiter': ('noise_cassini_jupiter_2001.wav',
        'https://www.jpl.nasa.gov/jupiterflyby/news/detected_sounds.html',
        'NASA/JPL/University of Iowa, Cassini RPWS'),
    'nasa_saturn_eerie': ('noise_saturn_pia07966.wav',
        'https://photojournal.jpl.nasa.gov/catalog/PIA07966', 'NASA/JPL/University of Iowa'),
    'nasa_saturn_bizarre': ('noise_saturn_pia07967.wav',
        'https://photojournal.jpl.nasa.gov/catalog/PIA07967', 'NASA/JPL/University of Iowa'),
    'nasa_saturn_lightning': ('noise_saturn_lightning_pia14310.wav',
        'https://photojournal.jpl.nasa.gov/catalog/PIA14310', 'NASA/JPL-Caltech/University of Iowa'),
    'nasa_earth_chorus': ('noise_emfisis_chorus.wav',
        'https://www.nasa.gov/mp3/693857main_emfisis_chorus_1.mp3',
        'NASA/Van Allen Probes EMFISIS (University of Iowa)'),
}
EQUIPMENT = {
    'fluorescent_hum': ('lysstoffror-brum-loop.wav', 'cd5Ttel-wqM', 16),
    'fluorescent_flicker': ('lysstoffror-flimmer.wav', 'cd5Ttel-wqM', 9.682729),
    'electrical_hum': ('elektrisk-anlegg-brum-loop.wav', '9hvJMXRzEzU', 16),
}
EQUIPMENT_RECIPES = {'fluorescent_hum', 'fluorescent_start', 'electrical_hum'}
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
USES = {'receiver', 'vhs', 'pc_transfer', 'equipment'}
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
        if key in SCIENTIFIC and source.get('role') != 'scientific_recording':
            raise ValueError(f'{key}: scientific provenance role is required')
        if key == 'space_nebula' and source.get('role') != 'designed_ambience':
            raise ValueError('space_nebula: designed ambience must not be labelled a scientific recording')
        if key in EQUIPMENT and source.get('role') != 'designed_equipment':
            raise ValueError(f'{key}: equipment provenance role is required')
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


def optional_sources(observatory_root=None, nebula_file=None):
    """Discover preserved sources; missing defaults are optional, explicit invalid paths are errors."""
    observatory = (observatory_root or DEFAULT_OBSERVATORY).resolve()
    nebula = (nebula_file or DEFAULT_NEBULA).resolve()
    if observatory_root is not None and not observatory.is_dir():
        raise ValueError(f'--observatory-root is not a directory: {observatory}')
    if nebula_file is not None and not nebula.is_file():
        raise ValueError(f'--nebula-file is not a file: {nebula}')
    found = {}
    for key, (filename, url, credit) in SCIENTIFIC.items():
        candidates = (observatory / 'wav' / filename, observatory / filename)
        source = next((path for path in candidates if path.is_file()), None)
        if source is None:
            print(f'Warning: optional source unavailable: {key} ({candidates[0]})', file=sys.stderr)
            continue
        found[key] = {'path': source, 'start': 0, 'duration': 12, 'modem_only': False,
                      'source_url': url, 'credit': credit, 'role': 'scientific_recording',
                      'source_evidence': str(observatory / 'SOURCES.md')}
    if nebula.is_file():
        found['space_nebula'] = {'path': nebula, 'start': 0, 'duration': 16, 'modem_only': False,
                                'source_url': None, 'role': 'designed_ambience',
                                'credit': 'Ultimate SFX for Games, FL_GFX_Ambience_SciFi_Space_Nebula_Cm.wav (local sound library; author not independently confirmed)'}
    else:
        print(f'Warning: optional source unavailable: space_nebula ({nebula})', file=sys.stderr)
    return found


def optional_recipes(keys):
    """Scans retain a single source's timing, rather than turning a mix into a claimed recording."""
    recipes = []
    saturn = next((key for key in ('nasa_saturn_eerie', 'nasa_saturn_bizarre', 'nasa_saturn_lightning') if key in keys), None)
    for name, key, loop, gain in (('nasa_saturn_scan', saturn, False, -21),
                                 ('nasa_jupiter_scan', 'nasa_cassini_jupiter', False, -21),
                                 ('nasa_saturn_bizarre_scan', 'nasa_saturn_bizarre', False, -21),
                                 ('nasa_saturn_lightning_scan', 'nasa_saturn_lightning', False, -24),
                                 ('nasa_earth_chorus_scan', 'nasa_earth_chorus', False, -21),
                                 ('nebula_ambience', 'space_nebula', True, -25)):
        if key and key in keys:
            recipes.append({'name': name, 'use': 'receiver', 'layers': [
                {'source': key, 'gain_db': gain, 'rate': 1, 'start_s': 0,
                 'length_s': 0, 'loop': loop, 'envelope': 'constant'}]})
    return recipes


def equipment_sources(sources_root):
    found = {}
    folder = sources_root / '2026-11-03-elektrisk'
    for key, (filename, video_id, duration) in EQUIPMENT.items():
        source = folder / filename
        if not source.is_file():
            print(f'Warning: optional source unavailable: {key} ({source})', file=sys.stderr)
            continue
        found[key] = {'path': source, 'start': 0, 'duration': duration, 'modem_only': False,
                      'source_url': f'https://www.youtube.com/watch?v={video_id}',
                      'source_evidence': str(folder / 'lydkandidater.json'),
                      'credit': 'Local designed equipment derivative, original synthesis with processed YouTube reference; source identities retained in lydkandidater.json',
                      'role': 'designed_equipment'}
    return found


def require_runtime_sources(inputs):
    required = set(SOURCES) | set(SCIENTIFIC) | set(EQUIPMENT) | {'space_nebula'}
    missing = sorted(required - set(inputs))
    if missing:
        raise ValueError('runtime staging requires every prepared source; missing: ' + ', '.join(missing))


def selected_recipes(recipe_path, inputs):
    recipes = []
    for recipe in read_json(recipe_path):
        if recipe.get('name') in EQUIPMENT_RECIPES and any(layer.get('source') not in inputs for layer in recipe.get('layers', [])):
            print(f"Warning: optional equipment recipe unavailable: {recipe['name']}", file=sys.stderr)
            continue
        recipes.append(recipe)
    return recipes + optional_recipes(inputs.keys())


def source_duration(path):
    result = subprocess.run(['ffprobe', '-v', 'error', '-show_entries', 'format=duration',
                             '-of', 'json', str(path)], capture_output=True, text=True, check=True)
    duration = float(json.loads(result.stdout)['format']['duration'])
    if not math.isfinite(duration) or duration <= 0:
        raise ValueError(f'{path}: invalid source duration')
    return duration


def prepare(sources_root, output, recipe_path, observatory_root=None, nebula_file=None, stage_runtime=False):
    sources_root = sources_root.resolve()
    output = output.resolve()
    if output == sources_root or output.is_relative_to(sources_root) or sources_root.is_relative_to(output):
        raise ValueError('generated bank and original recordings must use separate directories')
    if output.exists() and any(output.iterdir()) and not (output / MARKER).is_file():
        raise ValueError(f'{output}: nonempty directory is not an owned sound workshop')
    inputs = {}
    for key, (video_id, start, duration, reserved) in SOURCES.items():
        inputs[key] = {'path': locate(sources_root, video_id), 'start': start,
                       'duration': duration, 'modem_only': reserved,
                       'source_url': f'https://www.youtube.com/watch?v={video_id}',
                       'credit': 'User-selected YouTube reference; distribution rights unverified',
                       'role': 'reference_recording'}
    inputs.update(optional_sources(observatory_root, nebula_file))
    inputs.update(equipment_sources(sources_root))
    if stage_runtime:
        require_runtime_sources(inputs)
    originals = {key: spec['path'] for key, spec in inputs.items()}
    for key, spec in inputs.items():
        source = spec['path'].resolve()
        if source == output or source.is_relative_to(output):
            raise ValueError(f'{key}: original recording must remain outside generated bank directory')
        available = source_duration(source) - spec['start']
        if available <= 0:
            raise ValueError(f'{key}: selection starts beyond source')
        spec['duration'] = min(spec['duration'], available)
    hashes = {key: digest(path) for key, path in originals.items()}
    recipes = selected_recipes(recipe_path, inputs)
    # Validate all recipe/category controls before touching existing prepared files.
    with tempfile.TemporaryDirectory(prefix='s47-sound-preflight-') as scratch:
        temp_root = Path(scratch)
        temp_bank = {'version': 1, 'sources': {}, 'recipes': recipes}
        for key, spec in inputs.items():
            path = temp_root / f'{key}.wav'
            with wave.open(str(path), 'wb') as wav:
                wav.setparams((1, 2, 48000, 0, 'NONE', 'not compressed'))
                wav.writeframes(b'\0\0' * int(spec['duration'] * 48000))
            temp_bank['sources'][key] = {'file': path.name, 'modem_only': spec['modem_only'],
                                        'role': spec['role']}
        validate(temp_bank, temp_root)
    output.mkdir(parents=True, exist_ok=True)
    (output / MARKER).write_text('S47 local audio workshop v1\n', encoding='utf-8')
    bank = {'version': 1, 'sources': {}, 'recipes': recipes}
    for key, spec in inputs.items():
        source = originals[key]
        target = output / f'{key}.wav'
        temp = output / f'{key}.preparing.wav'
        subprocess.run(['ffmpeg', '-nostdin', '-v', 'error', '-y', '-ss', str(spec['start']), '-i', str(source),
                        '-t', str(spec['duration']), '-vn', '-ac', '1', '-ar', '48000', '-c:a', 'pcm_s16le', str(temp)], check=True)
        duration = pcm_info(temp)
        temp.replace(target)
        bank['sources'][key] = {field: value for field, value in spec.items() if field not in ('path', 'start', 'duration')}
        bank['sources'][key].update(file=target.name, source_file=str(source), source_start_s=spec['start'],
                                    source_sha256=hashes[key], prepared_duration_s=duration)
    counts = validate(bank, output)
    if any(digest(originals[key]) != sha for key, sha in hashes.items()):
        raise ValueError('original recording changed during preparation')
    write_json(output / 'bank.json', bank)
    print(f'Prepared {counts[0]} sources and {counts[1]} recipes: {output / "bank.json"}')
    print(f'Unreal argument for previews: -S47SoundWorkshop="{output / "bank.json"}"')
    if stage_runtime:
        print('Runtime sound-design bank staged for local cooking. Other Audio/Raw banks and original recordings preserved.')
    else:
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
        for change in ('receiver', 'equipment', 'unreserve', 'escape', 'nan', 'outside', 'filter'):
            wrong = copy.deepcopy(bank)
            layer = wrong['recipes'][0]['layers'][0]
            if change == 'receiver': wrong['recipes'][0]['use'] = 'receiver'
            elif change == 'equipment': wrong['recipes'][0]['use'] = 'equipment'
            elif change == 'unreserve': wrong['sources']['modem']['modem_only'] = False
            elif change == 'escape': wrong['sources']['modem']['file'] = '../test.wav'
            elif change == 'nan': layer['gain_db'] = float('nan')
            elif change == 'outside': layer.update(start_s=0.8, length_s=0.5)
            elif change == 'filter': layer.update(highpass_hz=2000, lowpass_hz=1000)
            try:
                validate(wrong, root)
            except ValueError:
                rejected += 1
        assert rejected == 7
        assert wav.is_file() and pcm_info(wav) == 1
        optional = optional_recipes({'nasa_cassini_jupiter', 'nasa_saturn_eerie', 'space_nebula'})
        assert len(optional) == 3
        assert all(r['use'] == 'receiver' and r['layers'][0]['rate'] == 1 for r in optional)
        assert all(not r['layers'][0]['loop'] for r in optional if r['name'].startswith('nasa_'))
        assert optional_recipes(set()) == []
        try:
            optional_sources(root / 'missing-observatory', None)
        except ValueError:
            pass
        else:
            raise AssertionError('explicit invalid observatory path was accepted')
        try:
            optional_sources(root, root / 'missing-nebula.wav')
        except ValueError:
            pass
        else:
            raise AssertionError('explicit invalid Nebula file was accepted')
        for key, role in (('nasa_cassini_jupiter', 'designed_ambience'), ('space_nebula', 'scientific_recording')):
            wrong = copy.deepcopy(bank)
            wrong['sources'][key] = {'file': 'test.wav', 'modem_only': False, 'role': role}
            try:
                validate(wrong, root)
            except ValueError:
                pass
            else:
                raise AssertionError('scientific/designed provenance was confused')
        complete = set(SOURCES) | set(SCIENTIFIC) | set(EQUIPMENT) | {'space_nebula'}
        require_runtime_sources(complete)
        try:
            require_runtime_sources(complete - {'space_nebula'})
        except ValueError:
            pass
        else:
            raise AssertionError('incomplete runtime staging was accepted')
        equipment_bank = copy.deepcopy(bank)
        equipment_bank['sources']['equipment'] = {'file': 'test.wav', 'modem_only': False}
        equipment_bank['recipes'][0].update(use='equipment', layers=[{'source': 'equipment', 'envelope': 'activity'}])
        assert validate(equipment_bank, root) == (2, 1)
    print('Sound workshop self-test: valid banks, 7 invalid mixes, complete-runtime/provenance/path checks; source preserved.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sources-root', type=Path, default=DEFAULT_SOURCES)
    parser.add_argument('--output', type=Path, help='generated bank directory (default Saved/AudioWorkshop, or Content/Audio/Raw/sound_design with --stage-runtime)')
    parser.add_argument('--stage-runtime', action='store_true', help='explicitly stage all 15 sources for local cooking; missing originals are errors')
    parser.add_argument('--recipes', type=Path, default=RECIPES)
    parser.add_argument('--observatory-root', type=Path, help='optional preserved observatory collection (contains wav/ and SOURCES.md)')
    parser.add_argument('--nebula-file', type=Path, help='optional designed Nebula ambience recording')
    parser.add_argument('--check', action='store_true', help='validate prepared bank without conversion')
    parser.add_argument('--self-test', action='store_true')
    args = parser.parse_args()
    output = args.output or (PROJECT / 'Content/Audio/Raw/sound_design' if args.stage_runtime else PROJECT / 'Saved/AudioWorkshop')
    if args.self_test:
        self_test()
    elif args.check:
        bank = read_json(output / 'bank.json')
        if args.stage_runtime:
            require_runtime_sources(bank.get('sources', {}))
        counts = validate(bank, output)
        print(f'Valid PCM bank: {counts[0]} sources, {counts[1]} recipes; modem limited to pc_transfer.')
    else:
        prepare(args.sources_root, output, args.recipes, args.observatory_root, args.nebula_file, args.stage_runtime)


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, subprocess.CalledProcessError, wave.Error) as exc:
        sys.exit(f'S47 sound workshop: {exc}')
