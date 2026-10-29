"""Bind stemmebevis til dagens manus og faktisk opptaksfil, uten lydbehandling."""
import json


def spoken_overrides(folder):
    path = folder / 'speech-overrides.json'
    return json.loads(path.read_text())['cues'] if path.exists() else {}


def identity_problems(cue, item, label):
    return [f'{label} {key} differs from current cue' for key in ('text', 'actor', 'voice_profile')
            if item.get(key) != cue[key]]


def generation_problems(cue, item, digest, overrides):
    problems = identity_problems(cue, item, 'generation')
    if item.get('line_id') != cue['line_id'] or item.get('channel') != cue['channel']:
        problems.append('generation cue/channel identity differs')
    spoken = overrides.get(cue['line_id'], {}).get('spoken', cue['text'])
    if not isinstance(spoken, str) or not spoken.strip() or item.get('request', {}).get('text') != spoken:
        problems.append('generation request text differs from current spoken text')
    if item.get('pronunciation_override', False) is not (spoken != cue['text']):
        problems.append('generation pronunciation override differs from current policy')
    if item.get('audio', {}).get('sha256') != digest:
        problems.append('generation audio hash differs')
    return problems


def verification_problems(cue, item, digest):
    problems = identity_problems(cue, item, 'verification')
    if item.get('sha256') != digest:
        problems.append('verification audio hash differs')
    if item.get('technical_problems') != []:
        problems.append('verification technical results missing or failed')
    asr = item.get('asr')
    if asr and asr.get('expected_text', item.get('text')) != cue['text']:
        problems.append('verification ASR expected text differs')
    if asr and asr.get('audio_sha256', digest) != digest:
        problems.append('verification ASR audio hash differs')
    return problems


def cached_asr(cue, previous, digest):
    """Gammel ASR gjelder bare samme fil, ordlyd og rolle, også med --no-asr."""
    if verification_problems(cue, previous, digest):
        return None
    asr = previous.get('asr')
    if not asr or asr.get('expected_text', previous['text']) != cue['text']:
        return None
    if asr.get('audio_sha256', digest) != digest:
        return None
    return asr
