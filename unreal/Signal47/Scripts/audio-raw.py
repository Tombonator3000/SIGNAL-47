#!/usr/bin/env python3
"""The recordings core/Audio.ts decodes into its AudioContext, written as WAVE files for the
game's own Web Audio (S47WebAudio.h, US47Audio::DecodeAll), with the tables it looks them up by.

  Content/Audio/Raw/sfx/<file>.wav        every sound in web/src/assets/audio (Audio.ts SOURCES: phoneRing,
                                          thudSoft, the steps, the aliens ..., named as Audio.ts names them)
  Content/Audio/Raw/phone/<id>.wav        Kinski's telephone lines: the kinski catalog's clips whose text
                                          is one of PHONE_TEXTS (core/phoneScripts.ts), as Audio.ts picks them
  Content/Audio/Raw/character/<id>.wav    the character voices: bacall for N. Vega's call lines and CB words, as
                                          Audio.ts picks them, and her lines in room 6 (core/npcDialogue.ts, which
                                          the web version loads from the ensemble bank as it plays them), price for
                                          "Reference west ..."
  Content/Audio/Raw/index.json            sources (web name -> file), phone (text -> phone:<id>),
                                          character (voice -> text -> character:<id>); texts normalised
                                          as readAloud.ts normalizeReadingText
  Content/Audio/Raw/media/{music,radio}/*.wav  the long recordings, seekable PCM16 for S47Media
  Content/Audio/Raw/voice/<bank>/*.wav    the voice banks read a clip at a time: Kinski's and Serling's (with
                                          the opening's) for ReadAloud (core/readAloud.ts, core/narrationBanks.ts),
                                          and the four RadioVoice banks, decoded lazily by RadioVoice
  Content/Audio/Raw/voices.json           every voice bank's catalog (kinski, serling, serling-intro, artbell,
                                          artbell-town, radiocast, ensemble) with the asset each clip is
                                          imported as (/Game/Audio/Voice/<Bank>/v_<file>) and its raw file
                                          (raw: voice/<bank>/<file>.wav), for ReadAloud, RecordedDialogue and
                                          RadioVoice
  Content/Audio/Raw/tv/                  left as it is: the TVs' programme sound (Scripts/prepare-tv.py)
  Content/Licensed/Raw/<name>.wav, index.json   the bought sounds (lic:<name>), when the private
                                          folder web/src/assets/licensed/sfx is there

Content/Audio and Content/Licensed are ignored by Git. The WAVE files keep each recording's own
sample rate and channels (the context resamples, as decodeAudioData does). Needs ffmpeg.
Usage: audio-raw.py [--web WEB_SRC] [--content CONTENT_DIR]"""
import argparse
import importlib.util
import json
import os
import re
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

HERE = os.path.dirname(os.path.abspath(__file__))
D = os.path.dirname(HERE)
ROOT = os.path.dirname(os.path.dirname(D))
ap = argparse.ArgumentParser()
ap.add_argument('--web', default=os.path.join(ROOT, 'web', 'src'))
ap.add_argument('--content', default=os.path.join(D, 'Content'))
ap.add_argument('--recordings-only', action='store_true', help='refresh long media and the voice banks (ReadAloud\'s and RadioVoice\'s) without replacing existing sfx/phone/bought raw files')
ap.add_argument('--evening-only', action='store_true', help='validate and import only the EveningShift bank, preserving every other bank and raw file')
ap.add_argument('--evening-wav-dir', type=Path, help='approved original EveningShift WAVs; defaults to Content/Audio/EveningShift')
args = ap.parse_args()
WEB = args.web
AUDIO = os.path.join(WEB, 'assets', 'audio')
LICENSED = os.path.join(WEB, 'assets', 'licensed', 'sfx')
RAW = os.path.join(args.content, 'Audio', 'Raw')
LICRAW = os.path.join(args.content, 'Licensed', 'Raw')

# EveningShift is native narrative data, independent of the archived web banks.
# Preflight before any destructive refresh, and reuse the strict standalone catalog.
spec = importlib.util.spec_from_file_location('s47_evening_catalog', os.path.join(HERE, 'evening-voice-catalog.py'))
evening_catalog = importlib.util.module_from_spec(spec)
spec.loader.exec_module(evening_catalog)
evening_dir = args.evening_wav_dir or Path(args.content) / 'Audio/EveningShift'
evening_entry = None
if evening_dir.is_dir() or args.evening_only or args.evening_wav_dir:
    try:
        manifest_path = evening_catalog.DEFAULT_MANIFEST
        evening_entry = evening_catalog.catalog(evening_catalog.read_json(manifest_path),
            evening_catalog.read_json(manifest_path.with_name('subtitles.json')), evening_dir,
            evening_catalog.read_json(evening_catalog.DEFAULT_TAKES))
    except (ValueError, OSError, KeyError, TypeError, evening_catalog.wave.Error) as exc:
        sys.exit('S47: EveningShift validation failed: %s' % exc)
if args.evening_only:
    evening_catalog.import_bank(evening_entry, evening_dir, Path(RAW))
    print('S47: raw voice evening: 109 supplied PCM WAVs, 110 cues; other banks preserved')
    raise SystemExit(0)

# the voice banks and the folders their clips are imported to (import-audio.sh does the same)
BANKS = [('kinski', 'Kinski'), ('serling', 'Serling'), ('serling-intro', 'SerlingIntro'), ('artbell', 'ArtBell'),
         ('artbell-town', 'ArtBellTown'), ('radiocast', 'Radiocast'), ('ensemble', 'Ensemble')]
# the banks whose clips are also written as WAVE files, read one clip at a time: ReadAloud's (Kinski, and
# the narrator's Serling bank with the opening's) and RadioVoice's
RAW_BANKS = ('kinski', 'serling', 'serling-intro', 'artbell', 'artbell-town', 'radiocast', 'ensemble')


def normalize(text):
    # readAloud.ts normalizeReadingText: text.replace(/\s+/g, ' ').trim()
    return re.sub(r'\s+', ' ', text).strip()


def camel(base):
    # step_concrete0 -> stepConcrete0, thud_soft -> thudSoft, alien_voice -> alienVoice (Audio.ts's names)
    parts = base.split('_')
    return parts[0] + ''.join(p[:1].upper() + p[1:] for p in parts[1:])


def wav(src, dst):
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    r = subprocess.run(['ffmpeg', '-loglevel', 'error', '-y', '-i', src, '-c:a', 'pcm_s16le', '-f', 'wav', dst], capture_output=True, text=True)
    if r.returncode != 0:
        print('S47: ffmpeg failed on %s: %s' % (src, r.stderr.strip()), file=sys.stderr)
        return False
    return True


# ---------- core/phoneScripts.ts: the texts, read from its source (all literal strings) ----------
JS_STRING = re.compile(r"'((?:[^'\\]|\\.)*)'")


def unquote(s):
    return re.sub(r'\\(.)', r'\1', s)


def array_strings(src, name):
    m = re.search(r'export const %s\s*=\s*\[(.*?)\];' % re.escape(name), src, re.S)
    if not m:
        raise SystemExit('S47: %s not found in phoneScripts.ts' % name)
    return [unquote(x) for x in JS_STRING.findall(m.group(1))]


def call_lines(src, key):
    # PHONE_CALLS.<key>: [{ by, text, aside?, pause? }, ...] -> the spoken ones' texts
    m = re.search(r'\b%s:\s*\[(.*?)\n  \],' % re.escape(key), src, re.S)
    if not m:
        raise SystemExit('S47: PHONE_CALLS.%s not found in phoneScripts.ts' % key)
    out = []
    for obj in re.findall(r'\{(.*?)\}', m.group(1), re.S):
        t = re.search(r"text:\s*'((?:[^'\\]|\\.)*)'", obj)
        if t and not re.search(r'aside:\s*true', obj):
            out.append(unquote(t.group(1)))
    return out


if not args.recordings_only:
    scripts = open(os.path.join(WEB, 'core', 'phoneScripts.ts'), encoding='utf-8').read()
    PHONE_TEXTS = array_strings(scripts, 'PHONE_TEXTS')
    NORA_CB_TEXTS = array_strings(scripts, 'NORA_CB_TEXTS')
    channel_texts = set(normalize(t) for t in call_lines(scripts, 'nora') + call_lines(scripts, 'fieldNora')) | set(normalize(t) for t in NORA_CB_TEXTS)
    phone_texts = set(normalize(t) for t in PHONE_TEXTS)

    # (Raw/tv is the TVs' programme sound from prepare-tv.py, not from web/src: it stays)
    for d in (RAW, LICRAW):
        for name in os.listdir(d) if os.path.isdir(d) else []:
            path = os.path.join(d, name)
            if d == RAW and name == 'tv':
                continue
            if os.path.isdir(path) and not os.path.islink(path):
                shutil.rmtree(path)
            else:
                os.remove(path)
    os.makedirs(RAW, exist_ok=True)
    index = {'sources': {}, 'phone': {}, 'character': {}}

    # the free sounds: everything in assets/audio (Audio.ts's SOURCES and the drive's engine loops)
    n = 0
    for name in sorted(os.listdir(AUDIO)):
        base, ext = os.path.splitext(name)
        if ext.lower() not in ('.mp3', '.wav', '.ogg') or not os.path.isfile(os.path.join(AUDIO, name)):
            continue
        if wav(os.path.join(AUDIO, name), os.path.join(RAW, 'sfx', base + '.wav')):
            index['sources'][camel(base)] = 'sfx/%s.wav' % base
            n += 1
    print('S47: raw sfx: %d' % n)

    # Kinski's telephone lines: only the 24 of PHONE_TEXTS (Audio.ts phoneKeys; a later clip with the same text wins)
    catalog = json.load(open(os.path.join(AUDIO, 'kinski', 'catalog.json'), encoding='utf-8'))
    for clip in catalog['clips']:
        text = normalize(clip['text'])
        src = os.path.join(AUDIO, 'kinski', clip['file'])
        if text not in phone_texts or not os.path.isfile(src):
            continue
        key = 'phone:%s' % clip['id']
        if key not in index['sources']:
            if not wav(src, os.path.join(RAW, 'phone', clip['id'] + '.wav')):
                continue
            index['sources'][key] = 'phone/%s.wav' % clip['id']
        index['phone'][text] = key
    missing = sorted(phone_texts - set(index['phone']))
    print('S47: raw phone lines: %d of %d%s' % (len(index['phone']), len(phone_texts), (' (no recording: %s)' % missing) if missing else ''))

    # the character voices on the line (Audio.ts characterKeys)
    ensemble = json.load(open(os.path.join(AUDIO, 'ensemble', 'catalog.json'), encoding='utf-8'))
    for clip in ensemble['clips']:
        text = normalize(clip['text'])
        # (all of N. Vega's lines and the line's voice: the talk in room 6 plays hers through core/npcDialogue.ts)
        if clip['voice'] not in ('bacall', 'price'):
            continue
        src = os.path.join(AUDIO, 'ensemble', clip['file'])
        if not os.path.isfile(src) or not wav(src, os.path.join(RAW, 'character', clip['id'] + '.wav')):
            continue
        key = 'character:%s' % clip['id']
        index['sources'][key] = 'character/%s.wav' % clip['id']
        index['character'].setdefault(clip['voice'], {})[text] = key
    missing = sorted(channel_texts - set(index['character'].get('bacall', {})))
    print('S47: raw character lines: %d%s' % (sum(len(v) for v in index['character'].values()), (' (no recording on the line: %s)' % missing) if missing else ''))

    with open(os.path.join(RAW, 'index.json'), 'w', encoding='utf-8') as f:
        json.dump(index, f, ensure_ascii=False, indent=1, sort_keys=True)


# every voice bank's catalog, with the asset each clip is imported as
os.makedirs(RAW, exist_ok=True)
voices_path = os.path.join(RAW, 'voices.json')
voices = json.load(open(voices_path, encoding='utf-8')) if args.recordings_only and os.path.isfile(voices_path) else {}
for bank, folder in BANKS:
    path = os.path.join(AUDIO, bank, 'catalog.json')
    if not os.path.isfile(path):
        continue
    cat = json.load(open(path, encoding='utf-8'))
    for clip in cat.get('clips', []):
        clip['asset'] = 'v_' + os.path.splitext(clip['file'])[0]
        if bank in RAW_BANKS:
            clip['raw'] = 'voice/%s/%s.wav' % (bank, os.path.splitext(clip['file'])[0])
    voices[bank] = {'folder': '/Game/Audio/Voice/' + folder, 'catalog': cat}
with open(os.path.join(RAW, 'voices.json'), 'w', encoding='utf-8') as f:
    json.dump(voices, f, ensure_ascii=False, indent=1)
print('S47: voice banks indexed: %s' % ', '.join('%s %d' % (b, len(v['catalog'].get('clips', []))) for b, v in voices.items()))

if evening_entry is not None:
    evening_catalog.import_bank(evening_entry, evening_dir, Path(RAW), voices)
    print('S47: raw voice evening: 109 supplied PCM WAVs, 110 cues; measured hashes/timings verified')

# core/radioVoice.ts and core/readAloud.ts decode only the selected clips, so keep the banks outside
# index.sources. This lets their LRUs (24 buffers / 24 MiB, 4 buffers / 32 MiB) work without US47Audio
# preloading a whole bank (Kinski's and Serling's are some 1500 clips and an hour and a half each).
# (some 3000 files: the ffmpeg runs go side by side)
with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
    for bank in RAW_BANKS:
        cat = voices.get(bank, {}).get('catalog', {})
        jobs = []
        for clip in cat.get('clips', []):
            src = os.path.join(AUDIO, bank, clip['file'])
            if os.path.isfile(src):
                jobs.append(pool.submit(wav, src, os.path.join(RAW, clip['raw'])))
        n = sum(1 for j in jobs if j.result())
        print('S47: raw voice %s: %d' % (bank, n))

# core/score.ts and core/radio.ts use long HTMLAudioElements. Keep each recording's own
# sample rate/channels; S47Media reads PCM16 in bounded chunks and resamples in the graph.
media = {}
for group in ('music', 'radio'):
    srcdir = os.path.join(os.path.dirname(WEB), 'public', group)
    count = 0
    if os.path.isdir(srcdir):
        for name in sorted(os.listdir(srcdir)):
            base, ext = os.path.splitext(name)
            if ext.lower() not in ('.mp3', '.wav', '.ogg'):
                continue
            rel = 'media/%s/%s.wav' % (group, base)
            if wav(os.path.join(srcdir, name), os.path.join(RAW, rel)):
                media[group + '/' + name] = rel
                count += 1
    print('S47: raw media %s: %d' % (group, count))
with open(os.path.join(RAW, 'media.json'), 'w', encoding='utf-8') as f:
    json.dump(media, f, ensure_ascii=False, indent=1, sort_keys=True)

# the bought sounds, when the private clone is there
if not args.recordings_only and os.path.isdir(LICENSED):
    lic = {'sources': {}}
    for name in sorted(os.listdir(LICENSED)):
        base, ext = os.path.splitext(name)
        if ext.lower() not in ('.mp3', '.wav', '.ogg'):
            continue
        if wav(os.path.join(LICENSED, name), os.path.join(LICRAW, base + '.wav')):
            lic['sources']['lic:' + base] = base + '.wav'
    with open(os.path.join(LICRAW, 'index.json'), 'w', encoding='utf-8') as f:
        json.dump(lic, f, ensure_ascii=False, indent=1, sort_keys=True)
    print('S47: raw bought sounds: %d' % len(lic['sources']))
elif not args.recordings_only:
    print('S47: no %s: the bought sounds are left out (the game sounds as it does without them)' % LICENSED)
