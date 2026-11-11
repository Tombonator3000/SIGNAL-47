#!/usr/bin/env python3
"""Create the Dale voice candidates in the local VoiceStudio, and pin the one Tom chooses.

The candidates are written in EveningShift/voice/casting.json (profiles.dale.candidates):
two pure voice designs (no human recording) and one CC0 donor clone. Creating a design
profile renders its identity sample (seed 42) and creating a clone transcribes the
reference: both use the GPU, so run this through q.py and never while Tom plays.

  python3 create_profiles.py                     # create every candidate not created yet
  python3 create_profiles.py --only dale_de_design
  python3 create_profiles.py --pin dale_de_design   # Tom's choice -> casting.json profiles.dale

profiles.json records what was created: profile id, name, kind, reference hash and the
reference transcript. A copy of each reference is kept outside Git in
~/.cache/signal47-dale-voice/profiles. Nothing existing is changed or deleted in VoiceStudio.
"""
from pathlib import Path
import argparse
import datetime
import hashlib
import json
import re
import sys
import urllib.request
import uuid

HERE = Path(__file__).resolve().parent
CASTING = HERE.parent / 'EveningShift/voice/casting.json'
CACHE = Path.home() / '.cache/signal47-dale-voice'
DONOR_DIR = CACHE / 'donor-kyutai-323332d3'
sys.path.insert(0, str(HERE))


def form(fields, files=()):
    boundary = 'signal47-dale-' + uuid.uuid4().hex
    body = b''
    for key, value in fields.items():
        body += f'--{boundary}\r\nContent-Disposition: form-data; name="{key}"\r\n\r\n{value}\r\n'.encode()
    for key, name, data in files:
        body += (f'--{boundary}\r\nContent-Disposition: form-data; name="{key}"; filename="{name}"\r\n'
                 'Content-Type: audio/wav\r\n\r\n').encode() + data + b'\r\n'
    return body + f'--{boundary}--\r\n'.encode(), 'multipart/form-data; boundary=' + boundary


def post_profile(endpoint, fields, files=()):
    body, content_type = form(fields, files)
    request = urllib.request.Request(endpoint + '/profiles', data=body, headers={'Content-Type': content_type}, method='POST')
    return json.load(urllib.request.urlopen(request, timeout=600))


def candidates():
    return json.loads(CASTING.read_text())['profiles']['dale']['candidates']


def create(endpoint, name, spec, donor_dir):
    profile_name = f'SIGNAL 47 Dale candidate {name} (synthetic {"design" if spec["kind"] == "design" else "CC0 donor clone"})'
    fields = {'name': profile_name, 'kind': spec['kind'], 'language': spec['language']}
    files = []
    if spec['kind'] == 'design':
        # The server renders the identity sample from instruct + this script and keeps it as the reference.
        fields.update({'instruct': spec['instruct'], 'vd_states': json.dumps(spec['vd_states']), 'ref_text': spec['sample_script']})
    else:
        source = donor_dir / Path(spec['reference_file']).name
        data = source.read_bytes()
        digest = hashlib.sha256(data).hexdigest()
        if digest != spec['reference_original_sha256']:
            raise SystemExit(f'{source}: sha256 {digest} is not the pinned CC0 donor file')
        files.append(('ref_audio', source.name, data))  # ref_text left empty: VoiceStudio transcribes it once
    record = post_profile(endpoint, fields, files)
    reference = urllib.request.urlopen(endpoint + f"/profiles/{record['id']}/audio", timeout=600).read()
    if len(reference) < 1000:
        raise SystemExit(f'{name}: no reference audio came back')
    record = json.load(urllib.request.urlopen(endpoint + f"/profiles/{record['id']}", timeout=60))
    keep = CACHE / 'profiles'
    keep.mkdir(parents=True, exist_ok=True)
    (keep / f'{name}.{record["id"]}.wav').write_bytes(reference)
    return {'profile_id': record['id'], 'profile_name': record['name'], 'kind': record['kind'],
            'reference_sha256': hashlib.sha256(reference).hexdigest(), 'reference_bytes': len(reference),
            'reference_text': record.get('ref_text', ''), 'instruct': record.get('instruct', ''),
            'language': record.get('language'), 'created_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
            'human_listened': False}


def pin(name, registry):
    created = json.loads(registry.read_text())['profiles'] if registry.exists() else {}
    if name not in created:
        raise SystemExit(f'{name} has not been created yet')
    chosen = created[name]
    text = CASTING.read_text()
    start = text.index('    "dale": {')
    end = text.index('      "candidates": {', start)
    head = text[start:end]
    replace = {'status': json.dumps('pinned'), 'profile_id': json.dumps(chosen['profile_id']),
               'profile_name': json.dumps(chosen['profile_name'], ensure_ascii=False), 'kind': json.dumps(chosen['kind']),
               'reference_sha256': json.dumps(chosen['reference_sha256']), 'proposed_candidate': json.dumps(name)}
    for key, value in replace.items():
        head, count = re.subn(rf'("{key}": )("(?:[^"\\]|\\.)*"|null|true|false)', lambda m: m.group(1) + value, head, count=1)
        if count != 1:
            raise SystemExit(f'casting.json: field {key} not found in profiles.dale')
    result = text[:start] + head + text[end:]
    if json.loads(result)['profiles']['dale']['profile_id'] != chosen['profile_id']:
        raise SystemExit('casting.json: pin did not take')
    CASTING.write_text(result)
    print(json.dumps({'pinned': name, **{k: chosen[k] for k in ('profile_id', 'profile_name', 'kind', 'reference_sha256')}}, indent=1))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--endpoint', default='http://127.0.0.1:3900')
    parser.add_argument('--only', nargs='*', default=[])
    parser.add_argument('--pin')
    parser.add_argument('--donor-dir', type=Path, default=DONOR_DIR)
    parser.add_argument('--registry', type=Path, default=HERE / 'profiles.json')
    args = parser.parse_args()
    if args.pin:
        pin(args.pin, args.registry)
        return
    import generate  # noqa: E402  (the same busy guard as the takes)
    generate.refuse_while_busy([])
    registry = json.loads(args.registry.read_text()) if args.registry.exists() else {
        'version': 1, 'note_nb': 'Laget av create_profiles.py. Kandidatprofiler for Dales egen stemme; se casting.json profiles.dale.candidates.',
        'profiles': {}}
    live = {p['id'] for p in generate.base.profile_list(args.endpoint)}
    for name, spec in candidates().items():
        if args.only and name not in args.only:
            continue
        if registry['profiles'].get(name, {}).get('profile_id') in live:
            print(json.dumps({'candidate': name, 'exists': registry['profiles'][name]['profile_id']}))
            continue
        registry['profiles'][name] = create(args.endpoint, name, spec, args.donor_dir)
        temporary = args.registry.with_suffix('.tmp')
        temporary.write_text(json.dumps(registry, ensure_ascii=False, indent=2) + '\n')
        temporary.replace(args.registry)
        print(json.dumps({'candidate': name, **registry['profiles'][name]}, ensure_ascii=False), flush=True)


if __name__ == '__main__':
    main()
