#!/usr/bin/env python3
"""MUSIC-20261113 (Tom, 13 November 2026): the score's and the radio's pieces that are not in web/public.

  build_music.py free  [--inbox DIR]    the free downloads (CC BY 4.0 and public domain): trimmed, levelled, the radio's narrowed to AM
  build_music.py own   [--own FILE]     Tom's own "Signal 47 - Track 1 - Intro Drive" (id signal47_intro_drive)
  build_music.py mixes [--bundle DIR]   our own arrangements of the bought loops: four from the "1984 Retro Music Production Bundle"
                                        and "ABYSS", and five quiet ones that lay the 1980s loops over the dark ones
  build_music.py quiet [--bundle DIR]   only those five
  build_music.py stems [--bundle DIR]   SIDEBAND-20261113: the nine own mixes again as stems (channels), two to four per piece,
                                        to DIR/Audio/Raw/media/music/stems/<piece>__<stem>.wav with stems.json beside them. The
                                        stems sum to the mix before the bus; they carry the bus's linear parts and the mix's own
                                        gain curves, and the game runs the one tape curve over their sum (FS47ScoreStems). Every
                                        stem is an arrangement of two loops or more: no bought loop is staged alone. Checked here:
                                        the sum against the bus, the rebuilt mix against the staged file, the game's sum against it.
                                        Staged at half the mixes' rate (22.05 kHz, 16 bit; see STEM_RATE below for why and how), a
                                        stem whose sides are 30 dB or more under its middle as one channel; checked as staged and
                                        as the game plays them.
  build_music.py stems --adpcm          SIDEBAND-STEMS-ADPCM: the staged PCM stems again as IMA ADPCM (4 bits, 4:1) to
                                        DIR/Audio/Raw/media/music/stems_adpcm/ with a stems.json of the same rows (stems_adpcm.py,
                                        ima_adpcm.py; numpy only). The PCM stems are not rendered again and not touched.
  build_music.py stems --adpcm-check    decode that set back (the mirror of the game's decoder) and measure it against the PCM
                                        set, stem by stem and piece by piece: --report FILE, --ab DIR (A/B of the worst cases)
  build_music.py all                    free, own and mixes
  build_music.py index                  only media.json's rows for what is staged
  --content DIR    the project's Content (default: this project's); the files go to DIR/Audio/Raw/media/{music,radio}/<name>.wav,
                   where FS47MediaSource::Open finds music/<name>.mp3 and radio/<name>.mp3 (S47Media.cpp)
  --previews DIR   also write listening copies there (mp3, at -16 LUFS: the staged files are quieter, like the rest of the score)

Nothing here is in Git but this recipe: Content/Audio is ignored, the bought loops and Tom's own track never leave this machine,
and the game runs without any of it (S47Score skips a piece whose file is missing; the radio keeps its first record).
The raw loops are never staged, only the finished mixes. Function Loops' terms for the bundle (use and editing in a game, no
redistribution of the sounds as sound files: Docs/Research/Assets/LYD_ULTIMATE_SFX_2026-11-10.md) are Tom's and Claude's
earlier summary, not a licence file: Tom confirms them against the vendor's EULA before a public release.

Levels follow what is already there, measured with ffmpeg ebur128 on 13 November 2026: the score's 17 pieces are -19.6 to
-25.4 LUFS (median -21.5), the radio's four records -21.9 to -22.9 (mono, 22.05 kHz, 90 to 5500 Hz: web/THIRD_PARTY_NOTICES.md).
The downloads are data: only ffmpeg ever decodes them. free/own/index need python3 and ffmpeg; mixes also numpy and scipy
(on this machine: /home/tom/stable-diffusion-webui-forge/venv/bin/python3 -I Tools/music/build_music.py mixes).
"""
import argparse
import glob
import json
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PROJECT = os.path.dirname(os.path.dirname(HERE))
MUSIC_LUFS = -21.5
RADIO_LUFS = -22.7
PREVIEW_LUFS = -16.0
INBOX = os.path.expanduser('~/Assets/signal47-free/_inbox/music-20261113')
OWN = os.path.expanduser('~/Desktop/SIGNAL 47/Music/Signal 47 - Track 1-Intro Drive.mp3')
BUNDLE = os.path.expanduser('~/Desktop/SIGNAL 47/bt25_ultimatesfxforgames_softwarebundle')

# The free pieces. Licences read on the pages below on 13 November 2026 (Docs/Research/FREE_ASSET_CATALOG.md).
#   out: media/<out>.wav; fade: seconds of fade-out at the end.
# Left out on purpose: loc-jukebox-655645 "De gospel train am comin'" (Harry C. Browne and the Knickerbocker Quartet, Columbia
# A2255, 1916): a minstrel-dialect number by a blackface performer. Not for this game.
FREE = [
    {'src': 'fma_look-back-steelguitar-organ-f-35.mp3', 'out': 'music/look_back_steelguitar', 'fade': 1.0,
     'title': 'Look Back Steelguitar Organ (F 35)', 'by': 'Lobo Loco', 'licence': 'CC BY 4.0',
     'url': 'https://freemusicarchive.org/music/Lobo_Loco/completely-free-2/look-back-steelguitar-organ-f-35/'},
    # 80 seconds, cut as a loop: it plays once (the score never repeats a piece back to back), with a longer fade at its end
    {'src': 'fma_stumping-slow-deltablues-guitar-steelguitar-f-30.mp3', 'out': 'music/stumping_slow_deltablues', 'fade': 2.5,
     'title': 'Stumping Slow Deltablues - Guitar Steelguitar (F 30)', 'by': 'Lobo Loco', 'licence': 'CC BY 4.0',
     'url': 'https://freemusicarchive.org/music/Lobo_Loco/completely-free-2/stumping-slow-deltablues-guitar-steelguitar-f-30/'},
    {'src': 'fma_03_-_Uuriter_-_Standby_Mode.mp3', 'out': 'music/standby_mode', 'fade': 2.0,
     'title': 'Standby Mode', 'by': 'Uuriter', 'licence': 'CC BY 4.0',
     'url': 'https://freemusicarchive.org/music/Uuriter/Analog_Fields_1913/03_-_Uuriter_-_Standby_Mode/'},
    {'src': 'fma_08_-_Uuriter_-_Routine_Drone.mp3', 'out': 'music/routine_drone', 'fade': 2.0,
     'title': 'Routine Drone', 'by': 'Uuriter', 'licence': 'CC BY 4.0',
     'url': 'https://freemusicarchive.org/music/Uuriter/Analog_Fields_1913/08_-_Uuriter_-_Routine_Drone/'},
    # Library of Congress National Jukebox, by way of the Internet Archive (Public Domain Mark): acoustic 78s, for the AM dial
    {'src': 'loc-jukebox-711901-hawaiian-hotel.mp3', 'out': 'radio/hawaiian_hotel', 'fade': 1.0, 'old': True,
     'title': 'Hawaiian Hotel', 'by': 'Frank Ferera and Anthony Franchini (1920)', 'licence': 'public domain',
     'url': 'https://www.loc.gov/item/jukebox-711901/'},
    {'src': 'loc-jukebox-122577-roll-on-silver-moon.mp3', 'out': 'radio/roll_on_silver_moon', 'fade': 1.0, 'old': True,
     'title': 'Roll On, Silver Moon', 'by': 'yodel song, Victor (1906)', 'licence': 'public domain',
     'url': 'https://www.loc.gov/item/jukebox-122577/'},
    {'src': 'loc-jukebox-132141-till-the-sands-of-the-desert-grow-cold.mp3', 'out': 'radio/till_the_sands', 'fade': 1.0, 'old': True,
     'title': 'Till the Sands of the Desert Grow Cold', 'by': 'Victor Orchestra (1912)', 'licence': 'public domain',
     'url': 'https://www.loc.gov/item/jukebox-132141/'},
]
# Tom's own track: the score's copy, and one narrowed to AM for a set that plays it as the radio does
OWN_OUT = [('music/signal47_intro_drive', False), ('radio/signal47_intro_drive', True)]
# SIDEBAND-20261113: which layers (in the order they are added below) make which stem. Every stem is two loops or more.
STEMS = {
    's47_night_freight': [('bed', (0, 1, 4)), ('low', (2, 10)), ('arps', (3, 8, 9)), ('drums', (5, 6, 7, 11))],
    's47_cold_dish': [('ground', (0, 1)), ('pads', (2, 3)), ('high', (4, 5))],
    's47_carrier_lock': [('bed', (1, 6, 7)), ('pulse', (2, 3, 8)), ('arp', (0, 4, 5, 9))],
    's47_county_road': [('bed', (0, 1, 11)), ('low', (3, 6)), ('arps', (2, 7, 9)), ('drums', (4, 5, 8, 10))],
    's47_third_shift': [('ground', (0, 1)), ('inside', (2, 3, 4))],
    's47_dry_lake': [('ground', (0, 1, 3)), ('lights', (2, 4)), ('low', (5, 6))],
    's47_vacancy': [('ground', (0, 1)), ('chords', (2, 3, 4))],
    's47_grey_hour': [('ground', (0, 1, 2)), ('pads', (3, 4)), ('top', (5, 6))],
    's47_standing_wave': [('ground', (0, 1)), ('low', (2, 3)), ('far', (4, 5))],
}
# SIDEBAND-20261113 (Tom: can the stems take less space?). The stems are staged at half the mixes' rate. Measured on the nine
# pieces as the game sums them: what lies above 11.025 kHz is 47 to 70 dB under the whole (-71 to -94 dBFS): the bus's 9.5 kHz
# low-pass is baked into every stem (linear() in master). 24 kHz was tried and gains nothing (what lies between 11 and 12 kHz is
# 50 dB under the whole in the brightest piece, and its images are no lower). The engine plays a file by linear interpolation to
# its own 48 kHz (FS47MediaSource::Process), which takes 1.5 dB at 5 kHz and 3.9 dB at 8 kHz from a 22.05 kHz file (0.4 and 1.0 dB
# from a 44.1 kHz one) and leaves images above 12 kHz. So: the game runs a fourth-order low-pass at STEM_LOWPASS_HZ over the stems'
# sum (S47ScoreStems.cpp, two BiquadFilterNodes) against the images, and an equaliser baked into the stems here gives back what
# the interpolation and that low-pass take, magnitude and phase, down to what a 44.1 kHz file keeps. Measured as the game plays
# them: level per band within 0.1 dB of the 44.1 kHz stems up to 10 kHz, nothing added above 12 kHz.
STEM_RATE = 22050
STEM_LOWPASS_HZ = 11000.0
STEM_LOWPASS_Q = (0.5411961, 1.3065630)   # a fourth-order Butterworth as two sections (S47ScoreStems.cpp has the same two, in dB)
STEM_MONO_SIDE_DB = -30.0                 # a stem whose sides are this far under its middle over the whole file is staged as one channel
MIXES = ['music/s47_night_freight', 'music/s47_cold_dish', 'music/s47_carrier_lock', 'music/s47_county_road', 'radio/s47_night_freight',
         'music/s47_third_shift', 'music/s47_dry_lake', 'music/s47_vacancy', 'music/s47_grey_hour', 'music/s47_standing_wave']

TRIM = ('silenceremove=start_periods=1:start_duration=0:start_silence=0.15:start_threshold=-50dB:detection=peak,areverse,'
        'silenceremove=start_periods=1:start_duration=0:start_silence=0.15:start_threshold=-50dB:detection=peak,areverse')
# The AM dial: mono, 90 to 5500 Hz as the first four records; an acoustic 78 has nothing under 150 Hz but rumble and
# nothing over 5 kHz but shellac, and a station's own compressor evens it out
AM = 'pan=mono|c0=0.5*c0+0.5*c1,highpass=f=90,lowpass=f=5500'
# (the declicker and the limiter take the shellac's ticks down: measured 2 to 5 dB off the peaks, the loudness unchanged)
AM_OLD = ('pan=mono|c0=0.5*c0+0.5*c1,adeclick=threshold=4,highpass=f=150,highpass=f=150,lowpass=f=4800,lowpass=f=4800,'
          'acompressor=threshold=-26dB:ratio=3:attack=20:release=400:makeup=1,alimiter=limit=0.3:attack=2:release=60:level=false')


def run(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, **kw)


def loudness(path, pre=None):
    """Integrated loudness (LUFS), true peak (dBTP) and length (s) of a file, after an optional filter chain."""
    chain = (pre + ',' if pre else '') + 'ebur128=peak=true'
    r = run(['ffmpeg', '-hide_banner', '-nostats', '-i', path, '-map', '0:a:0', '-af', chain, '-f', 'null', '-'])
    text = r.stderr.decode('utf-8', 'replace')
    tail = text[text.rfind('Summary:'):]
    i = re.search(r'I:\s+(-?[\d.]+) LUFS', tail)
    p = re.search(r'Peak:\s+(-?[\d.]+) dBFS', tail)
    t = re.findall(r'time=(\d+):(\d+):([\d.]+)', text)
    if r.returncode != 0 or not i or not p:
        sys.exit('S47 music: ffmpeg could not measure %s: %s' % (path, text[-300:]))
    secs = (int(t[-1][0]) * 3600 + int(t[-1][1]) * 60 + float(t[-1][2])) if t else 0.0
    return float(i.group(1)), float(p.group(1)), secs


def duration(path):
    r = run(['ffprobe', '-v', 'error', '-show_entries', 'format=duration', '-of', 'csv=p=0', path])
    return float(r.stdout.decode().strip())


def stage(src, dst, target, pre=None, fade=1.0, rate=44100, channels=2):
    """src, through pre, without the silence at its ends, at target LUFS with short fades, as PCM16 WAVE at dst."""
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='s47music') as tmp:
        mid = os.path.join(tmp, 'mid.wav')
        chain = (pre + ',' if pre else '') + TRIM
        r = run(['ffmpeg', '-v', 'error', '-y', '-i', src, '-map', '0:a:0', '-af', chain, '-ar', str(rate), '-ac', str(channels),
                 '-c:a', 'pcm_f32le', mid])
        if r.returncode != 0:
            sys.exit('S47 music: ffmpeg failed on %s: %s' % (src, r.stderr.decode('utf-8', 'replace')[-300:]))
        lufs, peak, _ = loudness(mid)
        secs = duration(mid)
        gain = target - lufs
        if peak + gain > -1.0:  # never louder than a true peak of -1 dBTP
            gain = -1.0 - peak
        out = 'volume=%.2fdB,afade=t=in:st=0:d=0.03,afade=t=out:st=%.3f:d=%.3f,aresample=osf=s16:dither_method=triangular' % (
            gain, max(0.0, secs - fade), fade)
        tmpdst = dst + '.tmp-%d.wav' % os.getpid()
        r = run(['ffmpeg', '-v', 'error', '-y', '-i', mid, '-af', out, '-c:a', 'pcm_s16le', '-f', 'wav', tmpdst])
        if r.returncode != 0:
            sys.exit('S47 music: ffmpeg failed writing %s: %s' % (dst, r.stderr.decode('utf-8', 'replace')[-300:]))
        os.replace(tmpdst, dst)
    lufs, peak, secs = loudness(dst)
    print('S47 music: %-44s %6.1f s  %6.1f LUFS  %5.1f dBTP' % (os.path.relpath(dst, os.path.dirname(os.path.dirname(dst))), duration(dst), lufs, peak))
    return dst


def preview(src, dst, start=None, secs=None):
    """A listening copy: mp3 at PREVIEW_LUFS; a clip when start and secs are given, and never over 6 MB."""
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    total = duration(src)
    cut = []
    if start is not None:
        start = max(0.0, min(start, total - secs))
        cut = ['-ss', '%.2f' % start, '-t', '%.2f' % secs]
    length = secs if start is not None else total
    lufs, peak, _ = loudness(src)
    gain = min(PREVIEW_LUFS - lufs, -1.0 - peak)
    rate = 192 if length * 192000 / 8 < 5.8e6 else 160 if length * 160000 / 8 < 5.8e6 else 128
    fades = 'volume=%.2fdB' % gain
    if start is not None:
        fades += ',afade=t=in:st=0:d=0.3,afade=t=out:st=%.2f:d=0.6' % (secs - 0.6)
    r = run(['ffmpeg', '-v', 'error', '-y'] + cut + ['-i', src, '-af', fades, '-c:a', 'libmp3lame', '-b:a', '%dk' % rate, dst])
    if r.returncode != 0:
        sys.exit('S47 music: preview failed for %s: %s' % (src, r.stderr.decode('utf-8', 'replace')[-300:]))


def index(media):
    """media.json: the web names of everything in media/{music,radio} (audio-raw.py writes the same rows)."""
    path = os.path.join(os.path.dirname(media), 'media.json')
    rows = {}
    if os.path.isfile(path):
        rows = json.load(open(path, encoding='utf-8'))
    for group in ('music', 'radio'):
        for wav in sorted(glob.glob(os.path.join(media, group, '*.wav'))):
            base = os.path.splitext(os.path.basename(wav))[0]
            rows['%s/%s.mp3' % (group, base)] = 'media/%s/%s.wav' % (group, base)
    tmp = path + '.tmp-%d' % os.getpid()
    with open(tmp, 'w', encoding='utf-8') as f:
        json.dump(rows, f, ensure_ascii=False, indent=1, sort_keys=True)
    os.replace(tmp, path)
    print('S47 music: media.json has %d rows' % len(rows))


def free(media, inbox, previews):
    if not os.path.isdir(inbox):
        print('S47 music: no %s: the free pieces are left out (the game plays without them)' % inbox)
        return
    for row in FREE:
        src = os.path.join(inbox, row['src'])
        if not os.path.isfile(src):
            print('S47 music: %s is not in the inbox: left out' % row['src'])
            continue
        radio = row['out'].startswith('radio/')
        dst = os.path.join(media, row['out'] + '.wav')
        if radio:
            stage(src, dst, RADIO_LUFS, AM_OLD if row.get('old') else AM, row['fade'], 22050, 1)
        else:
            stage(src, dst, MUSIC_LUFS, None, row['fade'])
        if previews:
            name = ('radio_am_' if radio else 'new_') + os.path.basename(row['out']) + '_30s.mp3'
            preview(dst, os.path.join(previews, name), 40, 30)


def own(media, src, previews):
    if not os.path.isfile(src):
        print('S47 music: no %s: Tom\'s own track is left out (the game plays without it)' % src)
        return
    for out, radio in OWN_OUT:
        dst = os.path.join(media, out + '.wav')
        if radio:
            stage(src, dst, RADIO_LUFS, AM, 1.0, 22050, 1)
        else:
            stage(src, dst, MUSIC_LUFS, None, 1.0)
    if previews:
        preview(os.path.join(media, OWN_OUT[0][0] + '.wav'), os.path.join(previews, 'new_signal47_intro_drive_20s.mp3'), 30, 20)


# ---------------------------------------------------------------- the mixes
# Each piece is one kit's loops, phase-locked to one bar grid (a loop always plays the bar the piece is on, whenever it comes
# in), so the kit's own chord cycle holds. Layers come and go on bar lines; levels are set from each loop's measured loudest
# half, not by ear. The bus: 30 Hz high-pass, a little narrower, 9.5 kHz low-pass, soft saturation, slow wow, 2:1 compression
# of the loudest few dB, a ceiling, then MUSIC_LUFS. Tom has heard the first four (13 November: "kule, og kan brukes utenfor radioen også").
SR = 44100


def mixes(media, bundle, previews, first=True, quiet=True, stems=False):
    retro = os.path.join(bundle, '1984 Retro Music Production Bundle')
    abyss = os.path.join(bundle, 'ABYSS - Dark Cinematic', 'Loops')
    scifi = os.path.join(bundle, 'Ambience & SFX', 'Ambience SciFi')
    if not os.path.isdir(retro) or not os.path.isdir(abyss) or not os.path.isdir(scifi):
        print('S47 music: no bought bundle at %s: the mixes are left out (the game plays without them)' % bundle)
        return
    try:
        import numpy as np
        from scipy.signal import butter, fftconvolve, freqz, lfilter, resample_poly, sosfilt, welch
    except ImportError:
        print('S47 music: numpy and scipy are needed for the mixes: left out (see this file\'s head)')
        return

    def load(path, semi=0):
        hits = glob.glob(glob.escape(path)) or glob.glob(path)
        if len(hits) != 1:
            sys.exit('S47 music: loop not found (or not unique): %s' % path)
        # semi: semitones by tape speed (pitch and tempo together, as on a deck: no stretching, so nothing to smear)
        speed = ['-af', 'aresample=%d,asetrate=%d,aresample=%d' % (SR, round(SR * 2 ** (semi / 12.0)), SR)] if semi else []
        raw = run(['ffmpeg', '-v', 'error', '-i', hits[0]] + speed + ['-ac', '2', '-ar', str(SR), '-f', 'f32le', '-']).stdout
        x = np.frombuffer(raw, dtype=np.float32).astype(np.float64).reshape(-1, 2)
        if len(x) < SR // 4:
            sys.exit('S47 music: loop is empty: %s' % path)
        return x

    def loud_rms(x):
        # the level of the loudest half of its 200 ms windows: a loop that rests half its bars is not turned up for it
        w = SR // 5
        n = len(x) // w
        p = (x[:n * w] ** 2).reshape(n, w, 2).mean(axis=(1, 2))
        p = np.sort(p)[n // 2:]
        return 10 * np.log10(p.mean() + 1e-12)

    def sos(kind, hz, order=2):
        return butter(order, hz, kind, fs=SR, output='sos')

    def ramp(n, up=True):
        t = np.sin(np.linspace(0, np.pi / 2, max(2, n))) ** 2
        return t if up else t[::-1]

    def split_of(name, n):
        # stems mode: one bus per stem, and which stem each layer goes to
        rows = STEMS[name] if stems else []
        where = {layer: k for k, (_, layers) in enumerate(rows) for layer in layers}
        return [np.zeros((n, 2)) for _ in rows], where

    class Piece:
        def __init__(self, bpm, bars, name):
            self.spb = SR * 240.0 / bpm
            self.bars = bars
            self.bpm = bpm
            self.n = int(round(bars * self.spb))
            self.bus = np.zeros((self.n, 2))
            self.used = []
            self.name = name
            self.parts, self.where = split_of(name, self.n)

        def named(self):
            if not stems:
                return None
            if sorted(self.where) != list(range(len(self.used))):
                sys.exit('S47 music: %s: STEMS does not name every layer once' % self.name)
            return dict(bpm=self.bpm, parts=[(STEMS[self.name][k][0], x, [self.used[i] for i in STEMS[self.name][k][1]]) for k, x in enumerate(self.parts)])

        def at(self, bar):
            return int(round(bar * self.spb))

        def add(self, path, level, spans, lp=None, hp=None, perc=False, once=False, open_from=None, close_to=None, width=1.0):
            """spans: (from bar, to bar, fade-in bars, fade-out bars). open_from/close_to: (Hz, bars) low-pass that opens over the
            first span's first bars or closes over the last span's last bars."""
            x = load(path)
            layer = len(self.used)
            self.used.append(os.path.relpath(path, bundle))
            x = x * 10 ** ((level - loud_rms(x)) / 20)
            if width != 1.0:
                mid, side = x.mean(axis=1), (x[:, 0] - x[:, 1]) / 2
                x = np.stack([mid + side * width, mid - side * width], axis=1)
            edge = int(0.002 * SR)
            x[-edge:] *= ramp(edge, False)[:, None]  # no click where a loop wraps
            if not perc:
                x[:edge // 2] *= ramp(edge // 2)[:, None]
            for k, (b0, b1, fin, fout) in enumerate(spans):
                s, e = self.at(b0), min(self.n, self.at(b1))
                if once:
                    e = min(e, s + len(x))
                    seg = x[:e - s].copy()
                else:
                    seg = x[np.arange(s, e) % len(x)]
                if hp:
                    seg = sosfilt(sos('highpass', hp), seg, axis=0)
                if lp:
                    seg = sosfilt(sos('lowpass', lp), seg, axis=0)
                if open_from and k == 0:
                    n = min(len(seg), int(open_from[1] * self.spb))
                    a = ramp(n)[:, None]
                    seg[:n] = sosfilt(sos('lowpass', open_from[0]), seg[:n], axis=0) * (1 - a) + seg[:n] * a
                if close_to and k == len(spans) - 1:
                    n = min(len(seg), int(close_to[1] * self.spb))
                    a = ramp(n, False)[:, None]
                    dull = sosfilt(sos('lowpass', close_to[0]), seg, axis=0)[-n:]
                    seg[-n:] = dull * (1 - a) + seg[-n:] * a
                n = min(len(seg), max(int(0.005 * SR), int(fin * self.spb)))
                seg[:n] *= ramp(n)[:, None]
                n = min(len(seg), max(int(0.005 * SR), int(fout * self.spb)))
                seg[-n:] *= ramp(n, False)[:, None]
                self.bus[s:e] += seg
                if self.parts:
                    self.parts[self.where[layer]][s:e] += seg

    def measure(x, rate=SR):
        raw = (np.clip(x, -1, 1)).astype('<f4').tobytes()
        r = run(['ffmpeg', '-hide_banner', '-nostats', '-f', 'f32le', '-ar', str(rate), '-ac', '2', '-i', '-', '-af', 'ebur128=peak=true', '-f', 'null', '-'], input=raw)
        tail = r.stderr.decode('utf-8', 'replace')
        tail = tail[tail.rfind('Summary:'):]
        return float(re.search(r'I:\s+(-?[\d.]+) LUFS', tail).group(1)), float(re.search(r'Peak:\s+(-?[\d.]+) dBFS', tail).group(1))

    def master(x, name, wow=0.0006, ceiling=-1.0, split=None):
        raw = x

        def linear(v):
            # the bus's linear parts, the same for the mix and for every stem
            v = sosfilt(sos('highpass', 30), v, axis=0)
            vm, vs = v.mean(axis=1), (v[:, 0] - v[:, 1]) / 2
            v = np.stack([vm + vs * 0.85, vm - vs * 0.85], axis=1)
            return sosfilt(sos('lowpass', 9500), v, axis=0)

        x = linear(x)
        # work at a known level: the loudest half at -18 dBFS
        scale = 10 ** ((-18 - loud_rms(x)) / 20)
        x *= scale
        x = np.tanh(x * 1.6) / 1.6  # tape-like: the peaks round off, the body is left alone
        # wow: a slow drift in pitch of about 0.06 %, as a deck's capstan gives
        t = np.arange(len(x), dtype=np.float64)
        delay = wow * SR / (2 * np.pi * 0.45) * (1 + np.sin(2 * np.pi * 0.45 * t / SR)) + 0.00008 * SR / (2 * np.pi * 5.7) * (1 + np.sin(2 * np.pi * 5.7 * t / SR))
        src = np.clip(t - delay, 0, len(x) - 1)
        x = np.stack([np.interp(src, t, x[:, 0]), np.interp(src, t, x[:, 1])], axis=1)
        # bus compression at control rate (10 ms): 2:1 over -20 dBFS RMS, 30 ms attack, 300 ms release, at most 4 dB
        hop = SR // 100
        n = len(x) // hop
        level = 10 * np.log10((x[:n * hop] ** 2).reshape(n, hop, 2).mean(axis=(1, 2)) + 1e-12)
        want = np.clip((level + 20) / 2, 0, 4) * -1.0
        got = np.zeros(n)
        g = 0.0
        ka, kr = 1 - np.exp(-0.010 / 0.030), 1 - np.exp(-0.010 / 0.300)
        for i in range(n):
            g += (want[i] - g) * (ka if want[i] < g else kr)
            got[i] = g
        gain = np.interp(np.arange(len(x)), np.arange(n) * hop + hop / 2, got)
        comp = 10 ** (gain / 20)
        x *= comp[:, None]
        # ceiling: 5 ms look-ahead, nothing over -4 dBFS at this working level
        peak = np.abs(x).max(axis=1)
        look = int(0.005 * SR)
        n2 = len(peak) // look
        blk = peak[:n2 * look].reshape(n2, look).max(axis=1)
        blk = np.maximum(blk, np.roll(blk, -1))
        need = np.minimum(1.0, 10 ** (-4 / 20) / np.maximum(blk, 1e-9))
        for i in range(1, n2):  # 80 ms release
            need[i] = min(need[i], need[i - 1] + (1 - need[i - 1]) * 0.06) if need[i] > need[i - 1] else need[i]
        ceil = np.interp(np.arange(len(x)), np.arange(n2) * look, need)
        x *= ceil[:, None]
        lufs, _ = measure(x)
        final = 10 ** ((MUSIC_LUFS - lufs) / 20)
        x *= final
        lufs, peak = measure(x)
        if peak > ceiling:
            sys.exit('S47 music: %s peaks at %.1f dBTP' % (name, peak))
        dst = os.path.join(media, 'music', name + '.wav')
        if split is not None:
            write_stems(x, raw, name, split, linear, scale, src, t, comp * ceil, final, dst)
            return dst
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        tmpdst = dst + '.tmp-%d.wav' % os.getpid()
        r = run(['ffmpeg', '-v', 'error', '-y', '-f', 'f32le', '-ar', str(SR), '-ac', '2', '-i', '-', '-af', 'aresample=osf=s16:dither_method=triangular',
                 '-c:a', 'pcm_s16le', '-f', 'wav', tmpdst], input=x.astype('<f4').tobytes())
        if r.returncode != 0:
            sys.exit('S47 music: ffmpeg failed writing %s' % dst)
        os.replace(tmpdst, dst)
        print('S47 music: %-44s %6.1f s  %6.1f LUFS  %5.1f dBTP (mix)' % ('music/' + name + '.wav', len(x) / SR, lufs, peak))
        if previews:
            preview(dst, os.path.join(previews, 'mix_' + name + '.mp3'))
        return dst

    def under(a, ref):
        return 10 * np.log10((a ** 2).mean() / ((ref ** 2).mean() + 1e-30) + 1e-30)

    def stem_lowpass():
        # the game's two BiquadFilterNodes ('lowpass', Q in dB there) at its 48 kHz: (b, a) each
        rows = []
        for q in STEM_LOWPASS_Q:
            w0 = 2 * np.pi * STEM_LOWPASS_HZ / 48000.0
            al, cs = np.sin(w0) / (2 * q), np.cos(w0)
            rows.append((np.array([(1 - cs) / 2, 1 - cs, (1 - cs) / 2]) / (1 + al), np.array([1 + al, -2 * cs, 1 - al]) / (1 + al)))
        return rows

    def stem_equaliser(taps=255, hold=10500.0, nfft=4096):
        """What the stems are given before they are staged, at STEM_RATE: the inverse of the engine's linear interpolation from that
        rate (sinc squared) and of the game's low-pass (magnitude and phase), down to what interpolation from 44.1 kHz leaves, so
        that a stem sounds in the game as its 44.1 kHz form did. The magnitude is held from `hold` up."""
        f = np.arange(nfft // 2 + 1) * STEM_RATE / nfft
        fc = np.minimum(f, hold)
        mag = np.sinc(fc / float(SR)) ** 2 / np.sinc(fc / float(STEM_RATE)) ** 2
        low, low_held = np.ones(len(f), dtype=complex), np.ones(len(f), dtype=complex)
        for b, a in stem_lowpass():
            low = low * freqz(b, a, worN=np.minimum(f, STEM_RATE / 2.0 - 1), fs=48000.0)[1]
            low_held = low_held * freqz(b, a, worN=fc, fs=48000.0)[1]
        want = mag / np.abs(low_held) * np.exp(-1j * np.unwrap(np.angle(low)))
        want = want * np.exp(-2j * np.pi * f / STEM_RATE * ((taps - 1) / 2))
        return np.fft.irfft(want, nfft)[:taps] * np.kaiser(taps, 6.0)

    def equalise(y, h):
        k = (len(h) - 1) // 2
        return np.stack([np.convolve(y[:, c], h, mode='full')[k:k + len(y)] for c in range(y.shape[1])], axis=1)

    def engine_play(x, rate, n48):
        # FS47MediaSource::Process: the position moves rate / 48000 a frame, linear interpolation between two frames
        pos = np.arange(n48) * (rate / 48000.0)
        pos = pos[pos < len(x) - 1]
        i = pos.astype(np.int64)
        f = (pos - i)[:, None]
        y = x[i] * (1 - f) + x[i + 1] * f
        return np.concatenate([y, np.zeros((n48 - len(y), x.shape[1]))])

    def band_levels(x48):
        f, P = welch(x48, 48000, nperseg=8192, axis=0)
        P = P.sum(axis=1)
        return [10 * np.log10(P[(f >= a) & (f < b)].sum() * (f[1] - f[0]) + 1e-30) for a, b in ((2000, 6000), (6000, 10000), (12000, 24000))]

    def write_wav16(path, y, rate):
        import wave
        q = np.clip(np.round(y * 32768.0), -32768, 32767).astype('<i2')
        with wave.open(path, 'wb') as w:
            w.setnchannels(y.shape[1])
            w.setsampwidth(2)
            w.setframerate(rate)
            w.writeframes(q.tobytes())
        return q.astype(np.float64) / 32768.0   # as the game reads it back

    def write_stems(mix, raw, name, split, linear, scale, src, t, curve, final, staged):
        """SIDEBAND-20261113. Each stem gets the bus's linear parts, the working level, the wow and the mix's own gain curves
        (compression, ceiling, level), but not the tape curve: that cannot be shared out between stems, so the game runs it once
        over their sum, at the working level (stems.json's "level" is the gain between that level and the staged one)."""
        parts = split['parts']
        total = sum(x for _, x, _ in parts)
        before = under(total - raw, raw)
        if before > -200:
            sys.exit('S47 music: %s: the stems do not sum to the mix before the bus (%.0f dB)' % (name, before))
        out, files = [], []
        for label, x, loops in parts:
            if len(loops) < 2:
                sys.exit('S47 music: %s: stem %s is one loop alone' % (name, label))
            v = linear(x) * scale
            v = np.stack([np.interp(src, t, v[:, 0]), np.interp(src, t, v[:, 1])], axis=1)
            out.append((label, v * curve[:, None] * final, loops))
        # what the game makes of them: the sum, to the working level, through the tape curve, back
        game = np.tanh(sum(v for _, v, _ in out) / final * 1.6) / 1.6 * final
        after = under(game - mix, mix)
        lufs_game, peak_game = measure(game)
        lufs_mix, _ = measure(mix)
        rebuilt = None
        if os.path.isfile(staged):
            was = np.frombuffer(run(['ffmpeg', '-v', 'error', '-i', staged, '-ac', '2', '-ar', str(SR), '-f', 'f32le', '-']).stdout, dtype=np.float32).astype(np.float64).reshape(-1, 2)
            m = min(len(was), len(mix))
            rebuilt = under(mix[:m] - was[:m], was[:m])
        if after > -30 or abs(lufs_game - lufs_mix) > 0.5 or peak_game > -0.5:
            sys.exit('S47 music: %s: the stems through the tape curve are %.1f dB from the mix (%.1f against %.1f LUFS, peak %.1f)' % (name, after, lufs_game, lufs_mix, peak_game))
        folder = os.path.join(media, 'music', 'stems')
        os.makedirs(folder, exist_ok=True)
        # the staged form: half the rate, the equaliser, one channel where the sides are far enough under the middle, 16 bit
        h = stem_equaliser()
        n48 = int(len(mix) * 48000 // SR)
        played, lin_got, lin_ref, size, frames, ends, pending = np.zeros((n48, 2)), 0.0, 0.0, 0, 0, 0.0, []
        for label, v, loops in out:
            mid, side = v.mean(axis=1), (v[:, 0] - v[:, 1]) / 2
            side_db = float(under(side, mid))
            likeness = float((v[:, 0] * v[:, 1]).sum() / (np.sqrt((v[:, 0] ** 2).sum() * (v[:, 1] ** 2).sum()) + 1e-30))
            mono = side_db <= STEM_MONO_SIDE_DB
            whole = equalise(resample_poly(v, 1, 2, axis=0), h)
            y = equalise(resample_poly(mid[:, None], 1, 2, axis=0), h) if mono else whole
            if np.abs(y).max() >= 1.0:
                sys.exit('S47 music: %s: stem %s would clip as staged' % (name, label))
            dst = os.path.join(folder, '%s__%s.wav' % (name, label))
            z = write_wav16(dst + '.new', y, STEM_RATE)   # (beside the old one until this piece has passed its checks)
            pending.append(dst)
            both = np.repeat(z, 2, axis=1) if mono else z   # (a one-channel deck goes to both speakers as it is: no pan law)
            lin_got, lin_ref = lin_got + both, lin_ref + whole
            played += engine_play(both, STEM_RATE, n48)
            edge = int(0.005 * STEM_RATE)
            ends = max(ends, float(np.abs(z[:edge]).max()), float(np.abs(z[-edge:]).max()))
            size += os.path.getsize(dst + '.new')
            frames = len(z)
            files.append(dict(stem=label, file='%s__%s.wav' % (name, label), loops=len(loops), lufs=round(measure(v)[0], 1), channels=1 if mono else 2,
                              mono=bool(mono), side_under_mid_db=round(side_db, 1), likeness=round(likeness, 3),
                              why='sides %.1f dB under the middle: %s' % (-side_db, 'one channel' if mono else 'kept in two (needs %.0f)' % -STEM_MONO_SIDE_DB)))
        # as staged: the staged stems against the full stems through the same resampling and equaliser (what rounding and folding cost)
        staged_db = float(under(lin_got - lin_ref, lin_ref))
        # as the game plays them: each deck interpolated to 48 kHz, summed, the low-pass, the tape curve; against the mix file played the same way
        for b, a in stem_lowpass():
            played = lfilter(b, a, played, axis=0)
        heard = np.tanh(played / final * 1.6) / 1.6 * final
        mix48 = engine_play(mix, SR, n48)
        heard_db = float(under(heard - mix48, mix48))
        heard_lu = measure(heard, 48000)[0] - measure(mix48, 48000)[0]
        hb, mb = band_levels(heard), band_levels(mix48)
        seconds_off = abs(frames / float(STEM_RATE) - len(mix) / float(SR))
        # (a band is held to the mix's level unless both are under -80 dBFS there: then it is only the 16 bits' own floor)
        level_ok = all(abs(x - y) <= 0.5 or max(x, y) < -80 for x, y in zip(hb[:2], mb[:2])) and (hb[2] - mb[2] <= 6 or hb[2] < -80)
        if staged_db > -60 or heard_db > -30 or abs(heard_lu) > 0.5 or not level_ok or seconds_off > 1.0 / STEM_RATE or ends > 0.002:
            for dst in pending:
                os.remove(dst + '.new')
            sys.exit('S47 music: %s: the staged stems fail their checks: as staged %.1f dB, as played %.1f dB and %+.2f LU against the mix, bands %s, '
                     'length off by %.6f s, ends %.4f' % (name, staged_db, heard_db, heard_lu, ['%+.1f' % (x - y) for x, y in zip(hb, mb)], seconds_off, ends))
        for dst in pending:
            os.replace(dst + '.new', dst)
        index_path = os.path.join(folder, 'stems.json')
        rows = json.load(open(index_path, encoding='utf-8')) if os.path.isfile(index_path) else {}
        rows.setdefault('pieces', {})[name] = dict(level=float(final), bpm=split['bpm'], frames=int(frames), rate=STEM_RATE, lowpass_hz=STEM_LOWPASS_HZ, stems=files,
                                                   seconds=round(frames / float(STEM_RATE), 4), bytes=int(size),
                                                   sum_before_bus_db=round(float(before), 0), game_sum_against_mix_db=round(float(after), 1),
                                                   rebuilt_mix_against_staged_db=None if rebuilt is None else round(float(rebuilt), 1),
                                                   staged_against_full_stems_db=round(staged_db, 1), played_against_mix_db=round(heard_db, 1),
                                                   played_against_mix_lu=round(float(heard_lu), 2),
                                                   played_bands_against_mix_db=dict(zip(('2_6k', '6_10k', 'over_12k'), [round(float(x - y), 1) for x, y in zip(hb, mb)])),
                                                   ends_peak=round(ends, 5))
        rows['format'] = dict(rate=STEM_RATE, bits=16, mono_when_sides_under_middle_db=-STEM_MONO_SIDE_DB, lowpass_hz=STEM_LOWPASS_HZ,
                              note='half the mixes\' rate; an equaliser baked in gives back what the game\'s interpolation and its low-pass take (build_music.py)')
        rows['made_by'] = 'Tools/music/build_music.py stems'
        tmp = index_path + '.tmp-%d' % os.getpid()
        with open(tmp, 'w', encoding='utf-8') as f:
            json.dump(rows, f, indent=1, sort_keys=True)
        os.replace(tmp, index_path)
        print('S47 music: %-22s %d stems (%s); sum before the bus %.0f dB, the game\'s sum against the mix %.1f dB (%.1f against %.1f LUFS)%s' % (
            name, len(out), ', '.join('%s %d' % (label, len(loops)) for label, _, loops in out), before, after, lufs_game, lufs_mix,
            '' if rebuilt is None else ', this build against the staged mix %.1f dB' % rebuilt), flush=True)
        print('S47 music:   staged at %d Hz, %.1f MB (%s); as staged %.1f dB from the full stems; as the game plays them %.1f dB and %+.2f LU from the mix, '
              'bands 2-6k %+.1f, 6-10k %+.1f, over 12k %+.1f dB; ends %.5f' % (
                  STEM_RATE, size / 1e6, ', '.join('%s %s %.1f' % (f['stem'], 'mono' if f['mono'] else 'stereo', f['side_under_mid_db']) for f in files),
                  staged_db, heard_db, heard_lu, hb[0] - mb[0], hb[1] - mb[1], hb[2] - mb[2], ends), flush=True)

    used = {}
    if first:
        # (a) "Night Freight": the late truck radio. Neon Highway kit 5, 80 BPM, F#-E-B-D, 72 bars = 3:36.
        # intro 0-8 (pad, air) / A 8-24 (bass, arp; shaker and high pad from 16) / B 24-40 (kick; lead and snare from 32) /
        # break 40-48 (pads and the high arp) / C 48-64 (all; lead from 56) / outro 64-72
        k5 = os.path.join(retro, 'Function Loops - Neon Highway', 'FL_NHW_KIT05_80BPM_F#-E-B-D', 'FL_NHW_KIT05_80BPM_LOOPS', 'FL_NHW_KIT05_80_')
        p = Piece(80, 72, 's47_night_freight')
        p.add(k5 + 'F#-E-B-D_LOW NOTE PAD.wav', -23, [(0, 72, 4, 7)])
        p.add(k5 + 'ATMO SFX.wav', -33, [(0, 16, 2, 2), (40, 48, 1, 1)])
        p.add(k5 + 'F#-E-B-D_BASS.wav', -21, [(8, 40, 0.1, 0.25), (48, 64, 0.1, 1)], open_from=(260, 6))
        p.add(k5 + 'F#-E-B-D_ARP.wav', -27, [(8, 72, 4, 7)], open_from=(900, 8))
        p.add(k5 + 'F#-E-B-D_HIGH PAD.wav', -28, [(16, 40, 2, 1), (48, 64, 1, 2)])
        p.add(k5 + 'SHAKER.wav', -39, [(16, 40, 1, 0.5), (48, 64, 0.5, 1)], perc=True)
        p.add(k5 + 'KICK.wav', -25, [(24, 40, 0.02, 0.1), (48, 64, 0.02, 0.5)], lp=240, perc=True)
        p.add(k5 + 'SNARE.wav', -32, [(32, 40, 0.02, 0.1), (56, 64, 0.02, 0.5)], lp=3000, perc=True)
        p.add(k5 + 'F#-E-B-D_LEAD.wav', -29, [(32, 40, 0.5, 0.5), (56, 64, 0.5, 1)], lp=4500)
        p.add(k5 + 'F#_HIGH ARP.wav', -33, [(40, 56, 1, 2)])
        p.add(k5 + 'F#-E-B-D_MID BASS.wav', -30, [(48, 64, 1, 1)])
        p.add(k5 + 'CRASH.wav', -36, [(24, 28, 0, 1), (48, 52, 0, 1)], perc=True, once=True)
        freight = master(p.bus, 's47_night_freight', split=p.named())
        used['s47_night_freight'] = p.used

        # (b) "Cold Dish": the observatory's night bed, no drums. ABYSS, 100 BPM, D# minor (D#m, D#m, B, G#m every 8 bars), 88 bars = 3:31.
        # pedal 0-8 / sub 8-16 / first pad 16-32 / second pad over it 32-48 / thin and high 48-56 / darker pair 56-72 / down 72-88
        pad = os.path.join(abyss, 'Pad Loops', 'FL_ABB_Pad_100_D#_Obsidian_')
        p = Piece(100, 88, 's47_cold_dish')
        p.add(pad + '21_8bars.wav', -23, [(0, 88, 4, 8)])
        p.add(os.path.join(abyss, 'Bass Loops', 'FL_ABB_Bass_100_D#_Void_08_4bars.wav'), -25, [(8, 48, 4, 4), (56, 80, 4, 6)], lp=140)
        p.add(pad + '01_8bars.wav', -23, [(16, 48, 4, 3), (56, 82, 3, 8)])
        p.add(pad + '18_8bars.wav', -26, [(32, 48, 4, 3)])
        p.add(pad + '19_8bars.wav', -27, [(46, 58, 2, 3), (72, 82, 3, 6)])
        p.add(pad + '02_8bars.wav', -25, [(56, 72, 3, 3)])
        master(p.bus, 's47_cold_dish', wow=0.0004, split=p.named())
        used['s47_cold_dish'] = p.used

        # (c) "Carrier Lock": tension, a pulse over a C pedal. Retrowave, 100 BPM, C minor, 72 bars = 2:53.
        # arp alone 0-8 / pulse 8-16 / heartbeat kick 16-24 / B 24-40 (bass and pad move C, C, Eb, F) / break 40-48 /
        # pulse again 48-64 / out 64-72
        rw = os.path.join(retro, 'Function Loops - Retrowave', 'Loops')
        p = Piece(100, 72, 's47_carrier_lock')
        p.add(os.path.join(rw, 'Synth Loops', 'Arps', 'FL_SKY_100_Synth_Arp_Analog_Cm.wav'), -26, [(0, 72, 3, 6)], open_from=(500, 8), close_to=(600, 6))
        p.add(os.path.join(rw, 'Synth Loops', 'Pads', 'FL_SKY_100_Synth_Pad_Velvet_Cmin.wav'), -25, [(0, 24, 4, 1), (40, 48, 0.5, 1), (64, 72, 1, 6)])
        p.add(os.path.join(rw, 'Bass Loops', 'FL_SKY_100_Bass_Synth_Pulse_Cm_Bas.wav'), -21, [(8, 24, 0.5, 0.1), (48, 64, 0.5, 1)], open_from=(220, 4))
        p.add(os.path.join(rw, 'Drum Loops', 'Kicks', 'FL_SKY_100_Drum_Kick_Feedback.wav'), -26, [(16, 40, 0.02, 0.1), (48, 64, 0.02, 1)], lp=160, perc=True)
        p.add(os.path.join(rw, 'Synth Loops', 'Arps', 'FL_SKY_100_Synth_Arp_Electric_Cm.wav'), -31, [(16, 24, 2, 0.5), (56, 64, 2, 1)], lp=5000)
        p.add(os.path.join(rw, 'Synth Loops', 'Leads', 'FL_SKY_100_Synth_Lead_Flashes_Cmin.wav'), -30, [(16, 24, 0.5, 0.5), (56, 64, 0.5, 1)], lp=5000)
        p.add(os.path.join(rw, 'Bass Loops', 'FL_SKY_100_Bass_Synth_Deep_Cm.wav'), -21, [(24, 40, 0.1, 0.25)])
        p.add(os.path.join(rw, 'Synth Loops', 'Pads', 'FL_SKY_100_Synth_Pad_Starlit_Cmin.wav'), -25, [(24, 40, 1, 1)])
        p.add(os.path.join(rw, 'Drum Loops', 'Hats', 'FL_SKY_100_Drum_Hat_Textured.wav'), -40, [(24, 40, 1, 1)], perc=True)
        p.add(os.path.join(rw, 'Synth Loops', 'Leads', 'FL_SKY_100_Synth_Lead_Slowdive_Cmin.wav'), -29, [(40, 48, 0.5, 1)], lp=5000)
        master(p.bus, 's47_carrier_lock', split=p.named())
        used['s47_carrier_lock'] = p.used

        # (d) "County Road": the drive. Neon Highway kit 4, 124 BPM, A-C-D, 96 bars = 3:06.
        # pad 0-4 / arp 4-16 / bass 16-24 / kick and hat 24-40 (mid bass, pluck from 32) / lead and snare 40-56 /
        # break 56-64 / all 64-88 (lead and snare from 72) / out 88-96
        k4 = os.path.join(retro, 'Function Loops - Neon Highway', 'FL_NHW_KIT04_124BPM_A-C-D', 'FL_NHW_KIT04_124BPM_LOOPS', 'FL_NHW_KIT04_124_')
        p = Piece(124, 96, 's47_county_road')
        p.add(k4 + 'A-C-D_PAD.wav', -21, [(0, 96, 2, 7)])
        p.add(k4 + 'ATMO SFX.wav', -30, [(0, 16, 1, 2), (56, 64, 1, 1)])
        p.add(k4 + 'A_ARP.wav', -26, [(4, 96, 4, 7)], open_from=(800, 8))
        p.add(k4 + 'A-C-D_BASS.wav', -20, [(16, 56, 0.1, 0.25), (64, 88, 0.1, 1)], open_from=(260, 6))
        p.add(k4 + 'KICK.wav', -23, [(24, 56, 0.02, 0.1), (64, 88, 0.02, 0.5)], lp=320, perc=True)
        p.add(k4 + 'CLSHAT.wav', -37, [(24, 56, 1, 0.25), (64, 88, 0.5, 1)], perc=True)
        p.add(k4 + 'A-C-D_MID BASS.wav', -27, [(32, 56, 1, 0.25), (64, 88, 1, 1)])
        p.add(k4 + 'A_HIGH PLUCK.wav', -31, [(32, 40, 0.5, 0.5), (56, 64, 0.5, 0.5), (80, 88, 0.5, 1)])
        p.add(k4 + 'SNARE.wav', -30, [(40, 56, 0.02, 0.1), (72, 88, 0.02, 0.5)], lp=3200, perc=True)
        p.add(k4 + 'A-C-D_LEAD.wav', -28, [(40, 56, 0.5, 0.5), (72, 88, 0.5, 1)], lp=4500)
        p.add(k4 + 'CRASH.wav', -36, [(24, 28, 0, 1), (64, 68, 0, 1)], perc=True, once=True)
        p.add(k4 + 'FLYBY.wav', -33, [(56, 64, 0, 1)], once=True)
        master(p.bus, 's47_county_road', split=p.named())
        used['s47_county_road'] = p.used

        # the radio's copy of (a): the far station's late record
        if not stems:
            stage(freight, os.path.join(media, 'radio', 's47_night_freight.wav'), RADIO_LUFS, AM, 1.0, 22050, 1)
        if previews and not stems:
            preview(os.path.join(media, 'radio', 's47_night_freight.wav'), os.path.join(previews, 'radio_am_s47_night_freight_30s.mp3'), 80, 30)

    # ---------------------------------------------------------------- the quiet ones
    # Tom, 13 November 2026: "Prøv å mikse sammen de andre musikk-loopene med 1980s-loopene, noe ambient, skummelt men rolig."
    # Five beds. Each is a sustained ground from the bundle's dark kits (ABYSS, and the tonal beds of "Ambience SciFi") with one
    # or two of the 1984 bundle's analog synth loops (Retrowave, Neon Highway, Darkwave) far inside it: low-passed, a little
    # of the shared room on them, never a drum kit and never a lead. Two to four things sound at a time; they come and go over
    # 10 to 30 seconds; every piece starts from nothing and ends in nothing. The other music kits in the bundle ("Modern Music
    # Kits": trap, phonk, breakcore, indie, lofi, pop) are the wrong decade and the wrong weather, and are not used.
    # The loops were chosen by measurement (Tools/music/measure_music.py and the catalogue's features): the pitch classes of
    # every bar, how still a loop is, its centroid and how much of it is pulse. A loop in another key is brought over by tape
    # speed, one or two semitones at most (semi=): pitch and tempo together, as on a deck, so nothing is stretched. A loop with a
    # pulse keeps its exact length (lock=); a sustained one wraps over a crossfade of its own tail (xf=).
    # Nobody has listened to these either: they are checked by measurement, and the previews are for Tom's ear.
    def seam(x, xf):
        """One period of a sustained loop whose end runs into its beginning over xf seconds."""
        n = min(int(xf * SR), len(x) // 3)
        if n < 64:
            return x
        size = len(x) - n
        head, tail = x[:n], x[size:]
        alike = float((head * tail).sum() / (np.sqrt((head ** 2).sum() * (tail ** 2).sum()) + 1e-12))
        up = ramp(n) if alike > 0.5 else np.sqrt(ramp(n))  # the same sound: amplitudes add up; another: powers do
        down = up[::-1]
        per = x[:size].copy()
        per[:n] = head * up[:, None] + tail * down[:, None]
        return per

    def room(send, secs=4.5):
        """The one room all five share: a 4.5 s tail of filtered noise (the same every build), dark, and different in each ear."""
        n = int(secs * SR)
        t = np.arange(n) / SR
        ir = np.random.default_rng(47).standard_normal((n, 2)) * np.exp(-6.9 * t / (secs * 0.8))[:, None]
        ir = sosfilt(sos('lowpass', 3000), sosfilt(sos('highpass', 220), ir, axis=0), axis=0)
        ir[:int(0.03 * SR)] *= ramp(int(0.03 * SR))[:, None]
        ir /= np.sqrt((ir ** 2).sum(axis=0))
        return np.stack([fftconvolve(send, ir[:, c])[:len(send)] for c in (0, 1)], axis=1)

    class Bed:
        def __init__(self, secs, name):
            self.n = int(round(secs * SR))
            self.bus = np.zeros((self.n, 2))
            self.send = np.zeros(self.n)
            self.used = []
            self.stems = []
            self.name = name
            self.parts, self.where = split_of(name, self.n)
            self.sends = [np.zeros(self.n) for _ in self.parts]

        def named(self, wet=0.5):
            # each stem with its own share of the room, and the piece's own edges (all linear: they sum to mix())
            if not stems:
                return None
            if sorted(self.where) != list(range(len(self.used))):
                sys.exit('S47 music: %s: STEMS does not name every layer once' % self.name)
            edge = int(1.0 * SR)
            rows = []
            for k, x in enumerate(self.parts):
                x = x + wet * room(self.sends[k])
                x[:edge] *= ramp(edge)[:, None]
                x[-edge:] *= ramp(edge, False)[:, None]
                rows.append((STEMS[self.name][k][0], x, [self.used[i] for i in STEMS[self.name][k][1]]))
            return dict(bpm=0, parts=rows)

        def add(self, path, level, spans, semi=0, lock=False, xf=2.0, hp=None, lp=None, move=None, far=0.0, width=1.0, sway=None):
            """level: dB of its loudest half, after its filters at their most open. spans: (from s, to s, fade-in s, fade-out s).
            lp: Hz, or (dark Hz, open Hz) with move: [(s, 0..1), ...], how open it is when. far: 0..1, how much of it is only
            the room. sway: (dB, period s, phase 0..1), a slow breath in its level."""
            x = load(path, semi)
            name = os.path.relpath(path, bundle) + (' (%+d semitones)' % semi if semi else '')
            layer = len(self.used)
            self.used.append(name)
            # safe in mono: no layer is wider than a correlation of 0.3 between its two channels (two of the beds come wider than 0)
            mid, side = x.mean(axis=1), (x[:, 0] - x[:, 1]) / 2
            width *= min(1.0, np.sqrt(0.54 * (mid ** 2).mean() / ((side ** 2).mean() + 1e-20)))
            if width != 1.0:
                x = np.stack([mid + side * width, mid - side * width], axis=1)
            if lock:
                edge = int(0.002 * SR)
                x[-edge:] *= ramp(edge, False)[:, None]
                x[:edge] *= ramp(edge)[:, None]
            else:
                x = seam(x, xf)
            # no click where it wraps: the join's sharpest corner against the loop's own sharpest thousandth
            m = np.concatenate([x[-64:], x[:64]]).mean(axis=1)
            corner = np.abs(np.diff(m, 2))[56:70].max() / (np.percentile(np.abs(np.diff(x.mean(axis=1), 2)), 99.9) + 1e-9)
            if corner > 3:
                sys.exit('S47 music: %s would click where it wraps (%.1f)' % (name, corner))
            dark, bright = (lp if isinstance(lp, tuple) else (None, lp))

            def shaped(seg, cut):
                if hp:
                    seg = sosfilt(sos('highpass', hp), seg, axis=0)
                return sosfilt(sos('lowpass', cut), seg, axis=0) if cut else seg

            twice = shaped(np.concatenate([x, x]), bright)
            gain = 10 ** ((level - loud_rms(twice[len(x):])) / 20)
            stem = np.zeros(self.n)
            for t0, t1, fin, fout in spans:
                s, e = int(t0 * SR), min(self.n, int(t1 * SR))
                seg = x[(np.arange(s, e) if lock else np.arange(e - s)) % len(x)]
                if dark:
                    a = np.interp(np.arange(s, e) / SR, [m[0] for m in move], [m[1] for m in move])[:, None]
                    seg = shaped(seg, dark) * (1 - a) + shaped(seg, bright) * a
                else:
                    seg = shaped(seg, bright)
                seg = seg * gain
                if sway:
                    seg *= (10 ** (sway[0] * np.sin(2 * np.pi * (np.arange(s, e) / SR / sway[1] + sway[2])) / 20))[:, None]
                n = min(len(seg), int(fin * SR))
                seg[:n] *= ramp(n)[:, None]
                n = min(len(seg), int(fout * SR))
                seg[-n:] *= ramp(n, False)[:, None]
                self.bus[s:e] += seg * (1 - 0.6 * far)
                self.send[s:e] += seg.mean(axis=1) * far
                if self.parts:
                    self.parts[self.where[layer]][s:e] += seg * (1 - 0.6 * far)
                    self.sends[self.where[layer]][s:e] += seg.mean(axis=1) * far
                stem[s:e] += (seg ** 2).mean(axis=1)
            self.stems.append((name, stem))

        def mix(self, wet=0.5):
            x = self.bus + wet * room(self.send)
            edge = int(1.0 * SR)
            x[:edge] *= ramp(edge)[:, None]
            x[-edge:] *= ramp(edge, False)[:, None]
            return x

        def table(self, name, step=30.0):
            # which layer carries how much of each half minute (dB under the sum of the layers): written, since nobody hears it
            w = int(step * SR)
            total = sum(s for _, s in self.stems)
            print('S47 music: %s, layers in dB under their sum, per %d s' % (name, step))
            for label, s in self.stems:
                row = []
                for i in range(0, self.n, w):
                    part, whole = s[i:i + w].mean(), total[i:i + w].mean()
                    row.append('  . ' if part < whole * 1e-4 else '%4.0f' % (10 * np.log10(part / (whole + 1e-20) + 1e-20)))
                print('S47 music:   %-58s%s' % (os.path.basename(label)[:58], ''.join(row)))

    def quiet_piece(name, bed, wow=0.0010):
        bed.table(name)
        used[name] = bed.used
        # (the same bus as the four above, so the nine are one score; a true peak of -7 dBTP at the most, like the calm ones there)
        return master(bed.mix(), name, wow=wow, ceiling=-7.0, split=bed.named())

    if quiet:
        sci = os.path.join(scifi, 'FL_GFX_Ambience_SciFi_')
        opad = os.path.join(abyss, 'Pad Loops', 'FL_ABB_Pad_100_')
        orhy = os.path.join(abyss, 'Rhythmic Loops', 'FL_ABB_Rhythm_100_')
        obas = os.path.join(abyss, 'Bass Loops', 'FL_ABB_Bass_100_')
        rw = os.path.join(retro, 'Function Loops - Retrowave', 'Loops', 'Synth Loops')
        rpad = os.path.join(rw, 'Pads', 'FL_SKY_100_Synth_Pad_')
        rarp = os.path.join(rw, 'Arps', 'FL_SKY_100_Synth_Arp_')
        nhw2 = os.path.join(retro, 'Function Loops - Neon Highway', 'FL_NHW_KIT02_90BPM_D-G-Bb-D#', 'FL_NHW_KIT02_90BPM_LOOPS', 'FL_NHW_KIT02_90_')
        dwpad = os.path.join(retro, 'Function Loops - Darkwave - Postpunk', 'FL_DW_Loops', 'FL_DW_Synth_Loops', 'FL_DW_150_Synth_Pad_')
        PAD = 0.08  # a pad with chords in it: a seam short enough to keep its bars, long enough not to tick

        # (e) "Third Shift": inside SARO at night. C, unresolved: a chord of fourths (G C F Bb D) that never lands. 100 BPM, 4:29.
        # the bed alone 0-19 / ABYSS' C minor pad 19-125 and 197-254 / the Retrowave fifth, pulsing like a machine two rooms
        # away, 48-134 and 202-257 / the Retrowave pad's four chords 110-192 / a low C with a B against it, far off, 154-202
        b = Bed(268.8, 's47_third_shift')
        b.add(sci + 'Retro_Smoothness_Cm.wav', -24, [(0, 268.8, 14, 20)], lp=(700, 4000), move=[(0, .15), (60, .6), (130, .9), (200, .5), (268, .1)], sway=(1.5, 47, 0))
        b.add(opad + 'Gm_Obsidian_41_8bars.wav', -25, [(19.2, 124.8, 14, 16), (196.8, 254.4, 16, 20)], xf=PAD, lp=3000)
        b.add(rarp + 'Electric_Cm.wav', -27, [(48, 134.4, 16, 16), (201.6, 256.8, 14, 18)], lock=True, lp=(500, 1500),
              move=[(48, .1), (90, .9), (134, .2), (201, .1), (230, .6), (257, .1)], far=.5)
        b.add(rpad + 'Starlit_Cmin.wav', -25, [(110.4, 192, 16, 18)], xf=PAD, lp=2600, far=.3)
        b.add(rarp + 'Midnight_Cm.wav', -30, [(153.6, 201.6, 12, 14)], lock=True, lp=900, far=.7)
        quiet_piece('s47_third_shift', b)

        # (f) "Dry Lake": out on the land at night. D, open: D G A and no third. Free time, 5:00.
        # the bed 0-300 / ABYSS' drone, a semitone down, 10-150 / the Neon Highway arpeggio (90 BPM) as lights far out, 40-112
        # and 228-284 / the pedal 100-190 / the Retrowave arpeggio opening and closing 136-214, alone with the bed near 195 /
        # the fifth and a slow sub 200-282
        b = Bed(300, 's47_dry_lake')
        b.add(sci + 'Gloomy_Silence_Dm.wav', -24, [(0, 300, 14, 20)], lp=(600, 3500), move=[(0, .2), (80, .7), (160, .4), (230, .9), (300, .1)], sway=(1.5, 53, .25))
        b.add(orhy + 'D#_Neural_04_8bars.wav', -27, [(10, 150, 16, 20)], semi=-1, sway=(2, 37, 0))
        b.add(nhw2 + 'D_ARP.wav', -27, [(40, 112, 18, 18), (228, 284, 16, 20)], lock=True, lp=1600, far=.7)
        b.add(opad + 'D#_Obsidian_21_8bars.wav', -26, [(100, 190, 18, 20)], semi=-1)
        b.add(rarp + 'Velvet_Dm.wav', -27, [(136, 214, 18, 18)], lock=True, lp=(500, 1400), move=[(136, .1), (175, 1), (214, .1)], far=.5)
        b.add(orhy + 'D#_Neural_07_8bars.wav', -27, [(200, 282, 18, 22)], semi=-1)
        b.add(obas + 'D#_Void_08_4bars.wav', -29, [(196, 272, 14, 20)], semi=-1, xf=PAD, lp=120)
        quiet_piece('s47_dry_lake', b)

        # (g) "Vacancy": the motel room. A minor over C, the warmest of the five: a major seventh that stays. 100 BPM, 4:10.
        # the bed 0-250 / the Retrowave pad (Am, C, F, F) 14-110 and 197-240 / ABYSS' still chord, a semitone up, 72-156 /
        # three long notes going down (C, A, F), far, 126-204 / the dark Retrowave chords 149-228
        b = Bed(249.6, 's47_vacancy')
        b.add(sci + 'Broken_Core_Am.wav', -24, [(0, 249.6, 12, 18)], lp=(800, 3500), move=[(0, .3), (70, .8), (150, .5), (210, .8), (249, .2)], sway=(1.2, 41, 0))
        b.add(rpad + 'Midnight_Amin.wav', -25, [(14.4, 110.4, 16, 18), (196.8, 240, 14, 18)], xf=PAD, lp=(900, 2600),
              move=[(14, .2), (60, 1), (110, .3), (196, .2), (220, .7), (240, .2)])
        b.add(opad + 'G#m_Obsidian_36_8bars.wav', -27, [(72, 156, 16, 18)], semi=1)
        b.add(opad + 'G#m_Obsidian_37_8bars.wav', -29, [(126, 204, 16, 18)], semi=1, xf=PAD, lp=2200, far=.6)
        b.add(os.path.join(rw, 'Chords', 'FL_SKY_100_Synth_Chord_Smooth_Amin.wav'), -26, [(148.8, 228, 16, 18)], xf=PAD, lp=1800)
        quiet_piece('s47_vacancy', b)

        # (h) "Grey Hour": before dawn. E minor; the low end leaves and the top opens, and nothing resolves (the bed itself brushes a
        # G sharp every half minute and lets it go). Free time, 4:24.
        # the bed 0-264, opening / a sub 0-105 / the pedal, a semitone up, 18-135 / the Darkwave pad opening 62-196 /
        # ABYSS' thin pad (Em, C, Am) 122-240 / the Retrowave arpeggio, far, 166-244 / a high shimmer, two semitones down, 204-254
        b = Bed(264, 's47_grey_hour')
        b.add(sci + 'Soft_Space_Em.wav', -24, [(0, 264, 12, 20)], lp=(700, 4500), move=[(0, .1), (100, .5), (200, 1), (264, .4)], sway=(1.2, 49, .5))
        b.add(obas + 'D#_Void_08_4bars.wav', -28, [(0, 105, 10, 30)], semi=1, xf=PAD, lp=130)
        b.add(opad + 'D#_Obsidian_21_8bars.wav', -26, [(18, 135, 16, 26)], semi=1)
        b.add(dwpad + 'Feedback_Em.wav', -26, [(62, 196, 20, 22)], xf=PAD, lp=(800, 3600), move=[(62, .05), (130, .6), (175, 1), (196, .5)], far=.3)
        b.add(opad + 'D#_Obsidian_19_8bars.wav', -26, [(122, 240, 18, 22)], semi=1, xf=PAD)
        b.add(rarp + 'Sunrise_Em.wav', -30, [(166, 244, 18, 20)], lock=True, lp=2400, far=.7)
        b.add(orhy + 'F#_Neural_21_4bars.wav', -35, [(204, 254, 16, 18)], semi=-2, lp=5000, far=.4)
        quiet_piece('s47_grey_hour', b)

        # (i) "Standing Wave": nearly still, for the archive and wherever the game waits. D#, one note and its fifth. Free time, 3:42.
        # the bed 0-222 / ABYSS' drone 6-214 and its pedal 30-196, each breathing at its own rate / a sub 20-204 /
        # the Retrowave pulse, a semitone up and nearly closed, 52-176 / a high drone, far, 88-204
        b = Bed(222, 's47_standing_wave')
        b.add(sci + 'Ellegant_Space_D#m.wav', -24, [(0, 222, 14, 20)], lp=(700, 3200), move=[(0, .3), (110, .7), (222, .3)], sway=(1.2, 61, 0))
        b.add(orhy + 'D#_Neural_04_8bars.wav', -26, [(6, 214, 16, 20)], sway=(2, 43, .5))
        b.add(opad + 'D#_Obsidian_21_8bars.wav', -27, [(30, 196, 18, 22)], sway=(2, 59, .2))
        b.add(obas + 'D#_Void_08_4bars.wav', -30, [(20, 204, 14, 20)], xf=PAD, lp=120)
        b.add(rarp + 'Pulse_Dm.wav', -29, [(52, 176, 20, 22)], semi=1, lock=True, lp=(450, 1000), move=[(52, .1), (114, .9), (176, .1)], far=.6)
        b.add(orhy + 'D#_Neural_06_8bars.wav', -33, [(88, 204, 20, 22)], lp=3500, far=.4)
        quiet_piece('s47_standing_wave', b, wow=0.0006)

    for name, loops in used.items():
        print('S47 music: %s: %s' % (name, '; '.join(os.path.basename(v) for v in loops)))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('what', choices=('free', 'own', 'mixes', 'quiet', 'stems', 'all', 'index'))
    ap.add_argument('--content', default=os.path.join(PROJECT, 'Content'))
    ap.add_argument('--inbox', default=INBOX)
    ap.add_argument('--own', default=OWN)
    ap.add_argument('--bundle', default=BUNDLE)
    ap.add_argument('--previews')
    ap.add_argument('--adpcm', action='store_true', help='stems: transcode the staged PCM stems to IMA ADPCM (media/music/stems_adpcm)')
    ap.add_argument('--adpcm-check', action='store_true', help='stems: measure the ADPCM set against the PCM set')
    ap.add_argument('--report', help='--adpcm-check: write the table here (Markdown)')
    ap.add_argument('--ab', help='--adpcm-check: write A/B WAVs of the worst cases here')
    ap.add_argument('--jobs', type=int, default=8, help='--adpcm/--adpcm-check: processes')
    args = ap.parse_args()
    media = os.path.join(args.content, 'Audio', 'Raw', 'media')
    if not os.path.isdir(os.path.dirname(media)):
        sys.exit('S47 music: no %s (run Scripts/import-audio.sh first)' % os.path.dirname(media))
    if args.what in ('free', 'all'):
        free(media, args.inbox, args.previews)
    if args.what in ('own', 'all'):
        own(media, args.own, args.previews)
    if args.what in ('mixes', 'quiet', 'all'):
        mixes(media, args.bundle, args.previews, first=args.what != 'quiet')
    if args.what == 'stems' and (args.adpcm or args.adpcm_check):
        # (the ADPCM set is made from the staged PCM set: nothing is rendered again, media.json is not touched)
        sys.path.insert(0, HERE)
        import stems_adpcm
        if args.adpcm:
            stems_adpcm.transcode(media, jobs=args.jobs)
        if args.adpcm_check:
            stems_adpcm.check(media, report=args.report, ab_dir=args.ab, jobs=args.jobs)
        return
    if args.what == 'stems':
        mixes(media, args.bundle, None, stems=True)
    index(media)


if __name__ == '__main__':
    main()
