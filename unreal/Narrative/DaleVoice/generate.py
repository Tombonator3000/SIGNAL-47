#!/usr/bin/env python3
"""Dale's own voice over every voiced line of manifest.json, through the existing pipeline.

All synthesis, trimming, levelling and resampling is EveningShift/voice/generate.py
(OmniVoice, English, seed 42 and retakes.json, broadcast mastering, -20 dBFS active
RMS, peak <= -2 dBFS, trimmed with 80/200 ms air, 48 kHz PCM16 mono). Only the cue
list and the profile differ:

  --voice dale      the pinned profile "dale" in EveningShift/voice/casting.json
                    (create_profiles.py --pin). Metadata here, takes in
                    ~/.cache/signal47-dale-voice/dale/takes.
  --voice CANDIDATE a candidate made by create_profiles.py (profiles.json), for
                    auditions. Metadata and takes in ~/.cache/signal47-dale-voice/CANDIDATE.

The old Kinski takes (~/.cache/signal47-eveningshift, ~/.cache/signal47-horror,
Content/Audio/EveningShift) are never read or written.

  python3 generate.py --voice dale_de_design --only FF_BARK_hello D01_HANDOVER_002
  python3 generate.py --limit 40                    # next 40 missing takes, voice dale
  python3 generate.py --until-asr ID...             # next seeds until ASR matches
  python3 generate.py --sync ../../Signal47/Content/Audio/DaleVoice

It refuses to start (ReservationBusy, exit 75) while the game or an Unreal editor is
running, so ~/.claude/skills/signal47-nattskift/scripts/q.py waits for its turn.
"""
from pathlib import Path
import importlib.util
import json
import os
import shutil
import subprocess
import sys

HERE = Path(__file__).resolve().parent
NARRATIVE = HERE.parent
BASE_PATH = NARRATIVE / 'EveningShift/voice/generate.py'
CASTING = NARRATIVE / 'EveningShift/voice/casting.json'
CREATED = HERE / 'profiles.json'
CACHE = Path.home() / '.cache/signal47-dale-voice'
BUSY_PROCESSES = ('Signal47', 'UnrealEditor', 'UnrealEditor-Cmd')


def take_voice(argv):
    """Remove --voice NAME from argv (the base parser does not know it); the choice also goes to the
    environment, so the base's own retake subprocess (--until-asr) runs with the same voice."""
    voice = os.environ.get('SIGNAL47_DALE_VOICE', 'dale')
    if '--voice' in argv:
        at = argv.index('--voice')
        if at + 1 >= len(argv):
            raise SystemExit('--voice needs a name: dale or a candidate in profiles.json')
        voice, argv = argv[at + 1], argv[:at] + argv[at + 2:]
    os.environ['SIGNAL47_DALE_VOICE'] = voice
    return voice, argv


def role_for(voice):
    """The VoiceStudio profile the takes are bound to; the base checks name, kind and reference hash live."""
    if voice == 'dale':
        role = json.loads(CASTING.read_text())['profiles']['dale']
        if not role.get('profile_id'):
            raise SystemExit('Profile dale is still a proposal: choose a candidate and run create_profiles.py --pin NAME')
        return role
    created = json.loads(CREATED.read_text())['profiles'] if CREATED.exists() else {}
    if voice not in created:
        raise SystemExit(f'No created candidate {voice!r}: run create_profiles.py first (profiles.json)')
    return {**created[voice], 'actors': ['DALE']}


def meta_dir(voice):
    """generation.json, retakes.json and verification.json: committed here for dale, in the cache for auditions."""
    if voice == 'dale':
        return HERE
    target = CACHE / voice / 'meta'
    target.mkdir(parents=True, exist_ok=True)
    overrides = HERE / 'speech-overrides.json'
    if overrides.exists():
        shutil.copy2(overrides, target / 'speech-overrides.json')
    return target


def manifest_cues():
    manifest = json.loads((HERE / 'manifest.json').read_text())
    return [{'line_id': e['id'], 'actor': 'DALE', 'channel': e['channel'], 'voice_profile': 'dale', 'text': e['text'],
             'source': e['source'], 'pack': e['pack']}
            for e in manifest['lines'] if e['status'] == 'voice']


def refuse_while_busy(argv):
    if '--sync' in argv:
        return
    for name in BUSY_PROCESSES:
        if subprocess.run(['pgrep', '-x', name], capture_output=True).returncode == 0:
            print(f'ReservationBusy: {name} is running; no GPU work while Tom plays or Unreal works', file=sys.stderr)
            raise SystemExit(75)


VOICE, sys.argv = take_voice(sys.argv)
spec = importlib.util.spec_from_file_location('signal47_existing_voice_pipeline', BASE_PATH)
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)
base.HERE = meta_dir(VOICE)
base.PACK = HERE
base.HOME_DATA = CACHE / VOICE
base.__file__ = str(Path(__file__).resolve())
base.cues = manifest_cues
base.load_casting = lambda: {'profiles': {'dale': role_for(VOICE)}}
HOME_DATA = base.HOME_DATA
for name in ('RATE', 'PEAK_CAP_DBFS', 'TARGET_ACTIVE_RMS_DBFS', 'metrics', 'atomic_json', 'integrated_lufs'):
    globals()[name] = getattr(base, name)
cues = manifest_cues

if __name__ == '__main__':
    refuse_while_busy(sys.argv)
    base.main()
