#!/usr/bin/env python3
"""The night's far sounds for Freeform (SOUNDSCAPE-20261108): cut, level and stage the CC0 recordings in sources.json.

  python3 -I Tools/nightsounds/prepare.py --originals DIR   cut DIR/<id>.mp3 (the Freesound HQ previews) into clips/<clip>.ogg
  python3 -I Tools/nightsounds/prepare.py --stage [CONTENT] write CONTENT/Audio/Raw/night/<clip>.wav (default: this project's
                                                            Content), which the game reads at run time (S47Ambience NightClip)
  python3 -I Tools/nightsounds/prepare.py --check           the clips are all there and match sources.json

The originals are downloaded data: they are only ever decoded by ffmpeg, never run. Needs python3 and ffmpeg only.
Each clip: mono, 44.1 kHz, a high-pass under the rumble, its loud part levelled to -18 dBFS RMS with the peak under -1 dBFS,
short fades; a held sound (loop) has its last two seconds crossfaded into its first, so that it can loop without a seam.
"""
import argparse
import array
import json
import math
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
RATE = 44100
TARGET_RMS_DB = -18.0
PEAK_DB = -1.0
LOOP_FADE = 2.0


def decode(path, start, end, highpass):
    cmd = ['ffmpeg', '-v', 'error', '-ss', '%.3f' % start, '-to', '%.3f' % end, '-i', path,
           '-ac', '1', '-ar', str(RATE), '-af', 'highpass=f=%d' % highpass, '-f', 's16le', '-']
    raw = subprocess.run(cmd, capture_output=True, check=True).stdout
    pcm = array.array('h')
    pcm.frombytes(raw[: len(raw) // 2 * 2])
    return [v / 32768.0 for v in pcm]


def level(samples):
    # the loud part decides: the top 30 per cent of 50 ms windows
    win = int(RATE * 0.05)
    rms = sorted(math.sqrt(sum(v * v for v in samples[i:i + win]) / win) for i in range(0, max(1, len(samples) - win), win))
    loud = rms[int(len(rms) * 0.7):] or rms
    now = math.sqrt(sum(v * v for v in loud) / len(loud)) if loud else 0
    if now <= 1e-6:
        return samples
    gain = 10 ** (TARGET_RMS_DB / 20) / now
    peak = max(abs(v) for v in samples) * gain
    if peak > 10 ** (PEAK_DB / 20):
        gain *= 10 ** (PEAK_DB / 20) / peak
    return [v * gain for v in samples]


def fades(samples, fade_in, fade_out):
    n_in, n_out = int(RATE * fade_in), int(RATE * fade_out)
    for i in range(min(n_in, len(samples))):
        samples[i] *= i / n_in
    for i in range(min(n_out, len(samples))):
        samples[-1 - i] *= i / n_out
    return samples


def loop(samples):
    # the tail crossfaded over the head, equal power, then the tail dropped
    n = int(RATE * LOOP_FADE)
    if len(samples) < 3 * n:
        return samples
    head, body, tail = samples[:n], samples[n:-n], samples[-n:]
    mixed = [tail[i] * math.cos(0.5 * math.pi * i / n) + head[i] * math.sin(0.5 * math.pi * i / n) for i in range(n)]
    return mixed + body


def encode(samples, out_path, fmt):
    pcm = array.array('h', (max(-32767, min(32767, int(round(v * 32767)))) for v in samples))
    args = ['ffmpeg', '-v', 'error', '-y', '-f', 's16le', '-ar', str(RATE), '-ac', '1', '-i', '-']
    args += ['-c:a', 'libvorbis', '-q:a', '6', out_path] if fmt == 'ogg' else ['-c:a', 'pcm_s16le', out_path]
    subprocess.run(args, input=pcm.tobytes(), check=True)


def sources():
    return json.load(open(os.path.join(HERE, 'sources.json'), encoding='utf-8'))['clips']


def cut(originals):
    os.makedirs(os.path.join(HERE, 'clips'), exist_ok=True)
    for row in sources():
        src = os.path.join(originals, '%d.mp3' % row['id'])
        if not os.path.isfile(src):
            sys.exit('S47: night sounds: missing original %s' % src)
        samples = decode(src, row['from'], row['to'], row.get('highpass', 80))
        samples = level(samples)
        samples = loop(samples) if row.get('loop') else fades(samples, 0.03, 0.25)
        out = os.path.join(HERE, 'clips', row['clip'] + '.ogg')
        encode(samples, out, 'ogg')
        print('S47: night sound %s: %.1f s from %d (%s)' % (row['clip'], len(samples) / RATE, row['id'], row['author']))


def stage(content):
    dest = os.path.join(content, 'Audio', 'Raw', 'night')
    os.makedirs(dest, exist_ok=True)
    n = 0
    for row in sources():
        src = os.path.join(HERE, 'clips', row['clip'] + '.ogg')
        if not os.path.isfile(src):
            sys.exit('S47: night sounds: no clip %s (run --originals first)' % src)
        subprocess.run(['ffmpeg', '-v', 'error', '-y', '-i', src, '-ac', '1', '-ar', str(RATE), '-c:a', 'pcm_s16le',
                        os.path.join(dest, row['clip'] + '.wav')], check=True)
        n += 1
    print('S47: night sounds staged: %d in %s' % (n, dest))


def check():
    missing = [row['clip'] for row in sources() if not os.path.isfile(os.path.join(HERE, 'clips', row['clip'] + '.ogg'))]
    if missing:
        sys.exit('S47: night sounds missing: %s' % ', '.join(missing))
    print('S47: night sounds: all %d clips there' % len(sources()))


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--originals')
    p.add_argument('--stage', nargs='?', const=os.path.join(HERE, '..', '..', 'Content'))
    p.add_argument('--check', action='store_true')
    a = p.parse_args()
    if a.originals:
        cut(a.originals)
    if a.stage:
        stage(os.path.abspath(a.stage))
    if a.check or not (a.originals or a.stage):
        check()


if __name__ == '__main__':
    main()
