#!/usr/bin/env python3
"""Technical and ASR check of Dale's takes: EveningShift/voice/verify.py over manifest.json.

Same checks as the Evening Shift pack (48 kHz PCM16 mono, hash against generation.json,
peak <= -2 dBFS, active RMS within 3 dB of -20 dBFS, faded edges, speech not at the file
edge, faster-whisper ASR through VoiceStudio /transcribe, asr-review.json for accepted
differences). ASR uses the GPU: run it through q.py, never while Tom plays.

  python3 verify.py --voice dale_de_design --only FF_BARK_hello
  python3 verify.py                    # voice dale, every take
  python3 verify.py --no-asr           # format and level only (CPU)

Writes verification.json and takes.json in the voice's metadata folder (here for dale,
~/.cache/signal47-dale-voice/CANDIDATE/meta for auditions). takes.json files are relative
to Content/Audio, under DaleVoice/.
"""
from pathlib import Path
import importlib.util
import json
import sys

HERE = Path(__file__).resolve().parent
sys.dont_write_bytecode = True

_spec = importlib.util.spec_from_file_location('signal47_dale_generate', HERE / 'generate.py')
dale = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(dale)          # takes --voice out of sys.argv
_spec_v = importlib.util.spec_from_file_location('signal47_existing_voice_verify', HERE.parent / 'EveningShift/voice/verify.py')
base = importlib.util.module_from_spec(_spec_v)
_spec_v.loader.exec_module(base)
base.HERE = dale.base.HERE
base.HOME_DATA = dale.HOME_DATA
base.cues = dale.manifest_cues
transcribe, tokens, edit_distance = base.transcribe, base.tokens, base.edit_distance


def main():
    if '--no-asr' not in sys.argv:
        dale.refuse_while_busy([])  # ASR is GPU work
    code = base.main()
    # The base names its own bank folder; Dale's takes are their own bank.
    path = base.HERE / 'takes.json'
    if path.exists():
        takes = json.loads(path.read_text())
        for row in takes['takes'].values():
            row['file'] = 'DaleVoice/' + row['file'].split('/', 1)[-1]
        takes['voice'] = dale.VOICE
        base.atomic_json(path, takes)
    report_path = base.HERE / 'verification.json'
    if report_path.exists():
        report = json.loads(report_path.read_text())
        report.pop('alias', None)
        report['voice'] = dale.VOICE
        base.atomic_json(report_path, report)
    return code


if __name__ == '__main__':
    raise SystemExit(main())
