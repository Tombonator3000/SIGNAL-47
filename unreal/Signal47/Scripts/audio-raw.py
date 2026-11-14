#!/usr/bin/env python3
"""The recordings core/Audio.ts decodes into its AudioContext, written as WAVE files for the
game's own Web Audio (S47WebAudio.h, US47Audio::DecodeAll), with the tables it looks them up by.

  Content/Audio/Raw/sfx/<file>.wav        every sound in web/src/assets/audio (Audio.ts SOURCES: phoneRing,
                                          thudSoft, the steps, the aliens ..., named as Audio.ts names them)
  Content/Audio/Raw/character/<id>.wav    the character voices on the telephone and the CB (Audio.ts characterKeys):
                                          none since VOICE-OUT-3 (below); kept for a speaker's own future recordings
  Content/Audio/Raw/index.json            sources (web name -> file), character (voice -> text ->
                                          character:<id>, empty for now); texts normalised
                                          as readAloud.ts normalizeReadingText
  Content/Audio/Raw/media/{music,radio}/*.wav  the long recordings, seekable PCM16 for S47Media (and, kept as they are, the
                                          pieces Tools/music/build_music.py stages there: MUSIC-20261113)
  Content/Audio/Raw/voice/<bank>/*.wav    the voice banks read a clip at a time: Serling's (with the opening's)
                                          for the narrator (core/readAloud.ts, core/narrationBanks.ts),
                                          and the four RadioVoice banks, decoded lazily by RadioVoice
  Content/Audio/Raw/voices.json           every voice bank's catalog (serling, serling-intro, artbell,
                                          artbell-town, radiocast, ensemble without its retired voices) with the asset each clip is
                                          imported as (/Game/Audio/Voice/<Bank>/v_<file>) and its raw file
                                          (raw: voice/<bank>/<file>.wav), for ReadAloud, RecordedDialogue and
                                          RadioVoice
  Content/Audio/Raw/tv/                  left as it is: the TVs' programme sound (Scripts/prepare-tv.py)
  Content/Licensed/Raw/<name>.wav, index.json   the bought sounds (lic:<name>), when the private
                                          folder web/src/assets/licensed/sfx is there

KINSKI-READ-OUT (Tom 9 November 2026): the web version's kinski bank (a clone of a real person's voice) is
not imported any more: no Raw/voice/kinski, no Raw/phone, no kinski entry in voices.json (also dropped from
an existing one with --recordings-only). Its files were moved to ~/.cache/signal47-kinski-retired.

VOICE-OUT-3 (Tom 9 November 2026, "Fjern bare bacall, price, lynch enn saa lenge"): the ensemble's voices
bacall (N. Vega), price (THE LINE) and lynch (the souvenir man) are clones of real people's voices and are
not imported any more: no Raw/character lines, none of their clips in voices.json's ensemble or in
Raw/voice/ensemble (also dropped from an existing voices.json and index.json with --recordings-only). Their
lines are text. The ensemble keeps stanton and stack. Their files were moved to ~/.cache/signal47-voices-retired.

Content/Audio and Content/Licensed are ignored by Git. The WAVE files keep each recording's own
sample rate and channels (the context resamples, as decodeAudioData does). Needs ffmpeg.
Usage: audio-raw.py [--web WEB_SRC] [--content CONTENT_DIR]"""
import argparse
import importlib.util
import json
import os
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
dale_dir = Path(args.content) / 'Audio/DaleVoice'
evening_entry = None
if evening_dir.is_dir() or args.evening_only or args.evening_wav_dir:
    try:
        manifest_path = evening_catalog.DEFAULT_MANIFEST
        # Dale's cues are his own voice (DALE-STORY-SWAP): Content/Audio/DaleVoice and DaleVoice/takes.json
        evening_entry = evening_catalog.catalog(evening_catalog.read_json(manifest_path),
            evening_catalog.read_json(manifest_path.with_name('subtitles.json')), evening_dir,
            evening_catalog.read_json(evening_catalog.DEFAULT_TAKES), dale_dir,
            evening_catalog.read_json(evening_catalog.DEFAULT_DALE_TAKES))
    except (ValueError, OSError, KeyError, TypeError, evening_catalog.wave.Error) as exc:
        sys.exit('S47: EveningShift validation failed: %s' % exc)
def evening_counts():
    v = evening_entry['evening']['validation']
    return '%d supplied PCM WAVs (%d Dale), %d cues' % (v['unique_recording_count'], v['dale_recording_count'], v['cue_count'])


if args.evening_only:
    evening_catalog.import_bank(evening_entry, evening_dir, Path(RAW), dale_dir=dale_dir)
    print('S47: raw voice evening: %s; other banks preserved' % evening_counts())
    raise SystemExit(0)

# the voice banks and the folders their clips are imported to (import-audio.sh does the same)
BANKS = [('serling', 'Serling'), ('serling-intro', 'SerlingIntro'), ('artbell', 'ArtBell'),
         ('artbell-town', 'ArtBellTown'), ('radiocast', 'Radiocast'), ('ensemble', 'Ensemble')]
# the banks whose clips are also written as WAVE files, read one clip at a time: the narrator's (Serling's,
# with the opening's) and RadioVoice's
# KINSKI-READ-OUT and VOICE-OUT-3: never imported, as a bank or as a voice inside one (the ensemble's bacall,
# price and lynch), and removed from an existing voices.json and index.json
RETIRED_BANKS = ('kinski', 'bacall', 'price', 'lynch')
RAW_BANKS = ('serling', 'serling-intro', 'artbell', 'artbell-town', 'radiocast', 'ensemble')


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


def retired(name):
    return (name or '').lower() in RETIRED_BANKS


def drop_retired(voices):
    # no retired bank, and no clip of a retired voice in any bank (VOICE-OUT-3)
    for bank in [b for b in voices if retired(b)]:
        voices.pop(bank)
    for entry in voices.values():
        cat = entry.get('catalog') if isinstance(entry, dict) else None
        if isinstance(cat, dict) and isinstance(cat.get('clips'), list):
            cat['clips'] = [c for c in cat['clips'] if not retired(c.get('voice'))]
    return voices


def write_json(path, data, **kw):
    # atomically: a reader (the game, another script) never sees half a table
    tmp = path + '.tmp-%d' % os.getpid()
    with open(tmp, 'w', encoding='utf-8') as f:
        json.dump(data, f, ensure_ascii=False, indent=1, **kw)
        f.flush()
        os.fsync(f.fileno())
    os.replace(tmp, path)


if not args.recordings_only:
    # (Raw/tv is the TVs' programme sound from prepare-tv.py, and Raw/night Freeform's recorded night from
    # Tools/nightsounds, not from web/src: they stay. So does Raw/media, MUSIC-20261113: beside web/public's recordings,
    # written again below, it holds the pieces Tools/music/build_music.py stages from sources that are not in Git)
    for d in (RAW, LICRAW):
        for name in os.listdir(d) if os.path.isdir(d) else []:
            path = os.path.join(d, name)
            if d == RAW and name in ('tv', 'night', 'media'):
                continue
            if os.path.isdir(path) and not os.path.islink(path):
                shutil.rmtree(path)
            else:
                os.remove(path)
    os.makedirs(RAW, exist_ok=True)
    index = {'sources': {}, 'character': {}}

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

    # (KINSKI-READ-OUT: no telephone lines from the retired kinski bank; the 24 PHONE_TEXTS are captions
    # until they are recorded in the speakers' own voices)

    # (VOICE-OUT-3: no character voices on the line. The web version's were the ensemble's bacall for
    # N. Vega and price for THE LINE, both retired; her calls, CB words and room 6 are captions until the
    # speakers have recordings of their own, which index['character'] will hold)
    print('S47: raw character lines: 0 (bacall and price retired: VOICE-OUT-3)')

    write_json(os.path.join(RAW, 'index.json'), index, sort_keys=True)
else:
    # an existing index keeps its sources, without a retired voice's character lines and their files' entries
    index_path = os.path.join(RAW, 'index.json')
    if os.path.isfile(index_path):
        index = json.load(open(index_path, encoding='utf-8'))
        character = index.get('character', {})
        gone = set()
        for voice in [v for v in character if retired(v)]:
            gone |= set(character.pop(voice).values())
        if gone:
            for key in [k for k, v in index.get('sources', {}).items() if k in gone]:
                index['sources'].pop(key)
            write_json(index_path, index, sort_keys=True)
            print('S47: index.json: %d retired character lines dropped' % len(gone))


# every voice bank's catalog, with the asset each clip is imported as
os.makedirs(RAW, exist_ok=True)
voices_path = os.path.join(RAW, 'voices.json')
voices = drop_retired(json.load(open(voices_path, encoding='utf-8')) if args.recordings_only and os.path.isfile(voices_path) else {})
for bank, folder in BANKS:
    path = os.path.join(AUDIO, bank, 'catalog.json')
    if not os.path.isfile(path):
        continue
    cat = json.load(open(path, encoding='utf-8'))
    cat['clips'] = [c for c in cat.get('clips', []) if not retired(c.get('voice'))]  # VOICE-OUT-3
    for clip in cat['clips']:
        clip['asset'] = 'v_' + os.path.splitext(clip['file'])[0]
        if bank in RAW_BANKS:
            clip['raw'] = 'voice/%s/%s.wav' % (bank, os.path.splitext(clip['file'])[0])
    voices[bank] = {'folder': '/Game/Audio/Voice/' + folder, 'catalog': cat}
write_json(voices_path, drop_retired(voices))
print('S47: voice banks indexed: %s' % ', '.join('%s %d' % (b, len(v['catalog'].get('clips', []))) for b, v in voices.items()))

if evening_entry is not None:
    evening_catalog.import_bank(evening_entry, evening_dir, Path(RAW), voices, dale_dir=dale_dir)
    print('S47: raw voice evening: %s; measured hashes/timings verified' % evening_counts())

# DALE-STORY-SWAP-20261109: HorrorStory's bank "horror" (SC01 dispatch and Dale's own takes from DaleVoice,
# Narrative/HorrorStory/voice/catalog.py, which keeps every other bank as it is). Without this, a full refresh
# rebuilt voices.json without it and Story's horror lines lost their voice.
try:
    spec = importlib.util.spec_from_file_location('s47_horror_catalog', os.path.join(ROOT, 'unreal', 'Narrative', 'HorrorStory', 'voice', 'catalog.py'))
    horror_catalog = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(horror_catalog)
    horror_entry = horror_catalog.catalog(horror_catalog.generate.HOME_DATA)
    print('S47: raw voice horror: %d takes' % horror_catalog.import_raw(horror_entry, horror_catalog.generate.HOME_DATA, Path(RAW)))
    voices = json.load(open(os.path.join(RAW, 'voices.json'), encoding='utf-8'))
except (ValueError, OSError, KeyError, TypeError) as exc:
    print('S47: raw voice horror left out: %s' % exc, file=sys.stderr)

# DALE-WIRE-20261109: Dale's own Freeform takes as the "dale" bank (Narrative/DaleVoice/catalog.py, which keeps every other bank
# as it is). Until there are takes it is an empty bank, and the game keeps his lines as text with the walkie's own chatter.
try:
    spec = importlib.util.spec_from_file_location('s47_dale_catalog', os.path.join(ROOT, 'unreal', 'Narrative', 'DaleVoice', 'catalog.py'))
    dale_catalog = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(dale_catalog)
    print('S47: raw voice dale: %d takes' % dale_catalog.import_raw(dale_catalog.build(), Path(RAW)))
except (ValueError, OSError, KeyError, TypeError) as exc:
    print('S47: raw voice dale left out: %s' % exc, file=sys.stderr)

# core/radioVoice.ts and core/readAloud.ts decode only the selected clips, so keep the banks outside
# index.sources. This lets their LRUs (24 buffers / 24 MiB, 4 buffers / 32 MiB) work without US47Audio
# preloading a whole bank (Serling's is some 1500 clips and an hour and a half).
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
    # MUSIC-20261113: the staged pieces beside them (Tools/music/build_music.py) keep their rows
    staged = os.path.join(RAW, 'media', group)
    for name in sorted(os.listdir(staged)) if os.path.isdir(staged) else []:
        base, ext = os.path.splitext(name)
        if ext.lower() == '.wav' and not any(k.startswith(group + '/' + base + '.') for k in media):
            media['%s/%s.mp3' % (group, base)] = 'media/%s/%s.wav' % (group, base)
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
