#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""RX-SEARCH-SOUND-20261113 (Tom 13. nov.): Toms egen søkelyd under mottakerens ACQUIRE og TUNE.

«Signal Sound Effect er en søke-lydeffekt til signalmottak som blir kraftigere og kraftigere, som jeg har laget, som kan mikses
inn med eksisterende signallyder.»

Kjør (på denne maskinen):
  /home/tom/stable-diffusion-webui-forge/venv/bin/python3 -I Tools/rxacquire/make_search.py [--src MP3] [--content DIR] [--previews DIR]
Krever numpy og ffmpeg. Uten --content skrives filene til HOVEDKOPIENS Content (/home/tom/SIGNAL-47/unreal/Signal47/Content), i
Audio/Raw/media/rxsearch/search_0.wav … search_6.wav, der spillet leser dem ved første bruk (FS47RxSearch::Load, S47RxSearch.cpp).
Content/Audio er git-ignorert og stages med bygget som de andre råopptakene (DefaultGame.ini, Audio/Raw), slik Toms Intro Drive er
(Tools/music/build_music.py own). Ikke i Git: bare denne oppskriften. Mangler filene, spiller spillet uten søkelaget, uten feil.

Hva fila er (målt 13. nov. med ffmpeg og numpy; skriptet skriver målingene på nytt hver gang):
  114,45 s, 48 kHz, «stereo» som er ren mono (L=R, korrelasjon 1,000). Ingen stillhet å kutte: over -60 dB fra 0,01 s til 114,4 s.
  0–3 s   svak luft (RMS -44 til -38 dB, mest over 300 Hz), små anslag rundt 1 og 2 s.
  3–14 s  STIGNINGEN: en pulsende «sonar»-swell med ett slag omtrent hvert 1,07 s; slagene vokser fra ca. -32 til -10 dB (0,25 s
          RMS), og klangen går fra lys (mellomtone) til dyp og tung (nesten alt under 200 Hz).
  14–111 s en jevn kropp på ca. -16,5 dB RMS: samme slag, et mønster på 12 slag som gjentar seg hvert 13,03 s (omhyllingskurve
          gjentas, bølgeformen ikke sampleidentisk: tekstur med støy).
  111,5–114,4 s utklingning.
  Altså: ikke monoton over 114 s, men 14 s vekst og så en sløyfe. Derfor bånd-sløyfer, ikke spoling i én lang fil.

Hva som lages: 7 sløyfer, ett bånd for hvert nærhetstrinn (spillet krysstoner mellom to nabobånd, likeffekt):
  search_0   luften (0–3 s), det spilleren hører langt unna
  search_1…5 tre slag hver fra stigningen, lagt på slagenes egne daler (ingen klikk i skjøten), med nivåhellingen inne i
             utsnittet rettet ut slik at sløyfa ikke «sagtanner»; nivået stiger bånd for bånd
  search_6   kroppen: ett helt 12-slags mønster (13,03 s)
  Nivå: kroppen på -20 dBFS RMS i fila; de andre beholder 60 % av fila sin egen dB-avstand til kroppen (luften havner ca. 14 dB
  under). I spillet ganges laget med 0,32 på ACQUIRE (kroppen ca. -30 dBFS, godt under lock_confirm på 0,60 som måles til ca.
  -19 dBFS på sitt sterkeste) og 0,26 på TUNE. Mono, 48 kHz, 16-bit PCM, DC fjernet, topp under -1 dBFS.

--previews DIR: det spilleren skal høre, simulert med samme regel som spillet (konstantene speiler S47RxSearch.h):
  rxsearch_a_acquire_25s.wav  25 s ACQUIRE: langt unna → nær → lås, med sweep_tick/converge/line_lock/lock_ring/lock_confirm
                              på nivåene i SkySound
  rxsearch_b_tune_20s.wav     20 s TUNE: bom → på bæreren → setter seg under en stand-in bærer (støy og en svak tone, ikke
                              spillets egen mottakertekstur)
  rxsearch_c_layer.wav        de sju båndene etter hverandre, slik de er staget
"""
import argparse
import math
import os
import subprocess
import sys
import wave

import numpy as np

SR = 48000
HERE = os.path.dirname(os.path.abspath(__file__))
SRC = "/home/tom/Desktop/SIGNAL 47/Music/ Signal Sound Effect.mp3"  # (navnet begynner med et mellomrom)
MAIN_CONTENT = "/home/tom/SIGNAL-47/unreal/Signal47/Content"
SUB = os.path.join("Audio", "Raw", "media", "rxsearch")
ACQ_SOUNDS = os.path.normpath(os.path.join(HERE, "..", "..", "Content", "RxAcquire", "Audio"))

BANDS = 7
BODY_RMS_DB = -20.0
KEEP = 0.6          # andel av fila sin egen dB-avstand til kroppen som beholdes
SEAM = 0.12         # s krysstoning i skjøten

# ---- speiler S47RxSearch.h (endres de der, endres de her) ----
LEVEL_ACQUIRE = 0.32
LEVEL_TUNE = 0.26
TAU_UP, TAU_DOWN, MAX_SLEW = 0.25, 0.5, 1.2
GAIN_TAU = 0.08
FADE_TAU = 0.12
LOCK_FADE_TAU = 0.3
HOLD, SETTLED, SETTLE_TAU = 1.5, 0.35, 1.2


def decode(path):
    r = subprocess.run(["ffmpeg", "-v", "error", "-i", path, "-ac", "1", "-ar", str(SR), "-f", "f32le", "-"], capture_output=True)
    if r.returncode != 0:
        sys.exit("S47 rxsearch: ffmpeg kunne ikke lese %s: %s" % (path, r.stderr.decode("utf-8", "replace")[-300:]))
    return np.frombuffer(r.stdout, np.float32).astype(np.float64)


def db(v):
    return 20 * math.log10(max(float(v), 1e-12))


def rms(x):
    return float(np.sqrt(np.mean(np.square(x)))) if len(x) else 0.0


def envelope(x, win=0.02, hop=0.01):
    w, h = int(win * SR), int(hop * SR)
    e = np.sqrt(np.convolve(np.square(x), np.ones(w) / w, "same"))[::h]
    return e, h


def pulses(x):
    """Slagenes topper og dalene mellom dem (sekunder), fra omhyllingskurven."""
    e, h = envelope(x)
    rad = int(0.4 / 0.01)
    floor = 10 ** (-48 / 20)
    peaks = [i for i in range(rad, len(e) - rad) if e[i] >= e[i - rad:i + rad + 1].max() and e[i] > floor]
    valleys = [a + int(np.argmin(e[a:b])) for a, b in zip(peaks[:-1], peaks[1:])]
    return np.array(peaks) * h / SR, np.array(valleys) * h / SR


def analyse(x):
    print("S47 rxsearch: %.2f s; topp %.1f dBFS; over -60 dB fra %.2f til %.2f s" % (
        len(x) / SR, db(np.abs(x).max()), np.argmax(np.abs(x) > 1e-3) / SR, (len(x) - np.argmax(np.abs(x[::-1]) > 1e-3)) / SR))
    for a, b in [(0, 3), (3, 6), (6, 9), (9, 12), (12, 14.4), (14.4, 27.4), (27.4, 111), (111.5, 114.4)]:
        s = x[int(a * SR):int(b * SR)]
        sp = np.abs(np.fft.rfft(s)) ** 2
        f = np.fft.rfftfreq(len(s), 1 / SR)
        print("  %6.1f–%6.1f s  RMS %6.1f dB   under 200 Hz %5.1f %%" % (a, b, db(rms(s)), 100 * sp[f < 200].sum() / sp.sum()))


def tilt_flat(seg, peaks_in):
    """Retter ut nivåhellingen inne i et utsnitt (lineær i dB, målt på slagene) så sløyfa ikke vokser og faller hver runde."""
    if len(peaks_in) < 2:
        return seg
    lv = []
    for p in peaks_in:
        c = int(p * SR)
        lv.append(db(rms(seg[max(0, c - SR // 10):c + SR // 10])))
    t = np.array(peaks_in)
    k, _ = np.polyfit(t, np.array(lv), 1)
    mid = (len(seg) / SR) / 2
    tt = np.arange(len(seg)) / SR
    return seg * 10 ** (-k * (tt - mid) / 20)


def loop(x, a, b, peaks_in=None):
    """x[a:b] (sekunder) som sømløs sløyfe: det som følger etter b toner inn i starten (likeffekt over SEAM). Hellingen rettes på
    hele utsnittet med det som følger, før skjøten, så overgangen fra slutt til start er sammenhengende."""
    ia, ib, n = int(a * SR), int(b * SR), int(SEAM * SR)
    ext = x[ia:ib + n].copy()
    if peaks_in is not None:
        ext = tilt_flat(ext, peaks_in)
    seg = ext[:ib - ia].copy()
    after = ext[ib - ia:]
    n = min(n, len(after))
    ph = np.linspace(0, math.pi / 2, n)
    seg[:n] = seg[:n] * np.sin(ph) + after[:n] * np.cos(ph)
    return seg


def bands(x):
    peaks, valleys = pulses(x)
    body_at = 14.3  # kroppens første slag (målt: 14,37 s)
    rise = [v for v in valleys if 2.6 < v < body_at]
    body_start = min(valleys, key=lambda v: abs(v - (body_at - 0.25)) if v < body_at else 99)
    print("S47 rxsearch: %d slag; daler i stigningen: %s; kroppen fra %.2f s" % (len(peaks), ", ".join("%.2f" % v for v in rise), body_start))
    cuts = []
    air_end = rise[0]
    cuts.append((0.05, air_end))
    # fem utsnitt på tre slag, jevnt fordelt over stigningen (hvert annet dalpar), det siste rett før kroppen
    starts = np.linspace(0, len(rise) - 4, 5).round().astype(int)
    for s in starts:
        cuts.append((rise[s], rise[s + 3]))
    body_end = min(valleys, key=lambda v: abs(v - (body_start + 13.03)))
    cuts.append((body_start, body_end))
    out = []
    for k, (a, b) in enumerate(cuts):
        inside = [p - a for p in peaks if a < p < b]
        seg = loop(x, a, b, inside if 0 < k < BANDS - 1 else None)
        seg = seg - seg.mean()
        out.append((k, a, b, seg))
    nat = [db(rms(s)) for _, _, _, s in out]
    body = nat[-1]
    final = []
    for (k, a, b, seg), n in zip(out, nat):
        target = BODY_RMS_DB + KEEP * (n - body)
        g = 10 ** ((target - n) / 20)
        seg = seg * g
        pk = np.abs(seg).max()
        if pk > 10 ** (-1 / 20):
            seg *= 10 ** (-1 / 20) / pk
        final.append((k, a, b, seg))
    return final


def write(path, seg):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    pcm = np.clip(np.round(seg * 32767), -32768, 32767).astype("<i2")
    tmp = path + ".tmp-%d" % os.getpid()
    with wave.open(tmp, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(pcm.tobytes())
    os.replace(tmp, path)


def read(path):
    with wave.open(path, "rb") as w:
        assert w.getnchannels() == 1 and w.getsampwidth() == 2, path
        return np.frombuffer(w.readframes(w.getnframes()), "<i2").astype(np.float64) / 32768.0, w.getframerate()


# ---------- the rule, as the game has it (S47RxSearch.cpp) ----------

def weights(c):
    x = min(max(c, 0.0), 1.0) * (BANDS - 1)
    lo = min(int(math.floor(x)), BANDS - 2)
    f = x - lo
    w = [0.0] * BANDS
    w[lo] = math.cos(f * math.pi / 2)
    w[lo + 1] = math.sin(f * math.pi / 2)
    return w


def smooth(held, target, dt, calm=False):
    tau = (TAU_UP if target > held else TAU_DOWN) * (1.5 if calm else 1.0)
    step = (target - held) * (1 - math.exp(-dt / tau))
    lim = MAX_SLEW * dt
    return held + max(-lim, min(lim, step))


def tune_settle(tuned_for):
    return 1.0 if tuned_for < HOLD else SETTLED + (1 - SETTLED) * math.exp(-(tuned_for - HOLD) / SETTLE_TAU)


def render_layer(stems, frames, dt=1 / 60):
    """frames: liste av (closeness, level) per spillbilde. Hver båndsløyfe går hele tida; gain følger med SetTargetAtTime."""
    n = int(len(frames) * dt * SR)
    out = np.zeros(n)
    gains = np.zeros((BANDS, n))
    g = np.zeros(BANDS)
    per = int(dt * SR)
    for i, (c, lev, tau) in enumerate(frames):
        w = weights(c)
        tgt = np.array([lev * wk for wk in w])
        aa = 1 - math.exp(-1 / ((tau or GAIN_TAU) * SR))
        for j in range(per):
            g += (tgt - g) * aa
            gains[:, i * per + j] = g
    for k, s in enumerate(stems):
        reps = int(math.ceil(n / len(s))) + 1
        out += np.tile(s, reps)[:n] * gains[k]
    return out


def one_shot(out, sound, at, gain, rate=1.0):
    x, sr = sound
    if rate != 1.0 or sr != SR:
        idx = np.arange(0, len(x) - 1, rate * sr / SR)
        x = np.interp(idx, np.arange(len(x)), x)
    i = int(at * SR)
    m = min(len(x), len(out) - i)
    if m > 0:
        out[i:i + m] += x[:m] * gain


def previews(stems, outdir):
    os.makedirs(outdir, exist_ok=True)
    dt = 1 / 60
    snd = {n: read(os.path.join(ACQ_SOUNDS, n + ".wav")) for n in ("sweep_tick", "converge", "line_lock", "lock_ring", "lock_confirm")}
    # (a) ACQUIRE: 25 s. Siktet starter 38° unna (nærhet 0,16), vandrer, nærmer seg i rykk, låser fra 21 s, holdt 23 s.
    T = 25.0
    frames, held = [], 0.0
    sweep_times, ev = [], []
    rng = np.random.default_rng(47)
    for i in range(int(T / dt)):
        t = i * dt
        if t < 5:
            c = 0.16 + 0.04 * math.sin(t * 1.3)
        elif t < 20:
            c = 0.18 + 0.80 * ((t - 5) / 15) ** 1.2 + 0.03 * math.sin(t * 2.1)
        else:
            c = 1.0
        c = min(1.0, c)
        locked = t >= 23.0
        held = smooth(held, 1.0 if t >= 21 else c, dt)
        frames.append((held, 0.0 if locked else LEVEL_ACQUIRE, LOCK_FADE_TAU if locked else None))
    lay = render_layer(stems, frames, dt)
    mix = lay.copy()
    # SkySound: et søketikk for hvert steg siktet tar (her ca. 6 i sekundet mens det flyttes), converge nær krysset
    t = 0.3
    while t < 21:
        c = frames[int(t / dt)][0]
        one_shot(mix, snd["sweep_tick"], t, 0.30 + 0.20 * c, 0.80 + 0.55 * c * c)
        close = min(max((c - 0.55) / 0.45, 0), 1)
        if close > 0:
            one_shot(mix, snd["converge"], t, 0.16 + 0.34 * close, 0.75 + 0.85 * close * close)
        t += 0.17 + 0.05 * rng.random()
    for k, at in enumerate((12.0, 16.5, 19.6)):
        one_shot(mix, snd["line_lock"], at, 0.55, 1 + 0.122 * k)
    for k, at in enumerate((21.0, 21.7, 22.4)):
        one_shot(mix, snd["lock_ring"], at, 0.50, 1.122 ** (2 * k))
    one_shot(mix, snd["lock_confirm"], 23.0, 0.60)
    write(os.path.join(outdir, "rxsearch_a_acquire_25s.wav"), np.clip(mix, -1, 1))
    write(os.path.join(outdir, "rxsearch_a_layer_only_25s.wav"), np.clip(lay, -1, 1))
    # (b) TUNE: 20 s. Kvaliteten fra 0,05 opp i rykk til 1,0 ved 11 s (Pass), holdt; laget setter seg under bæreren.
    T = 20.0
    frames, held, tuned = [], 0.0, 0.0
    qs = []
    for i in range(int(T / dt)):
        t = i * dt
        q = 0.05 if t < 2 else min(1.0, 0.05 + 0.95 * ((t - 2) / 9) ** 1.6 + (0.06 * math.sin(t * 3) if t < 9 else 0))
        q = max(0.0, q)
        qs.append(q)
        tuned = tuned + dt if q >= 0.995 else 0.0
        held = smooth(held, q, dt)
        frames.append((held, LEVEL_TUNE * tune_settle(tuned), 0.4 if tuned >= HOLD else None))
    lay = render_layer(stems, frames, dt)
    # stand-in bærer: støy som faller og en 1 kHz-ish tone som stiger med kvaliteten (S47ReceiverTexture's Source/Static-nivåer)
    n = len(lay)
    noise = np.random.default_rng(1).standard_normal(n)
    b, a2 = 0.0, 0.0
    lp = np.empty(n)
    for i in range(n):  # enkel lavpass
        b += (noise[i] - b) * 0.18
        lp[i] = b
    lp /= rms(lp)
    q = np.repeat(np.array(qs), int(dt * SR))[:n]
    tt = np.arange(n) / SR
    tone = np.sin(2 * math.pi * 820 * tt) * (0.6 + 0.4 * np.sin(2 * math.pi * tt / 0.84) ** 2)
    level = 0.42 * 0.5
    bed = lp * (level * (0.60 - 0.52 * q)) * 0.2 + tone / rms(tone) * (level * 0.8 * q * q) * 0.2
    write(os.path.join(outdir, "rxsearch_b_tune_20s.wav"), np.clip(bed + lay, -1, 1))
    # (c) laget alene: hvert bånd 3 s (kroppen 6 s), med den samme krysstoningen mellom dem
    frames = []
    for k in range(BANDS):
        secs = 6.0 if k == BANDS - 1 else 3.0
        for i in range(int(secs / dt)):
            frames.append((k / (BANDS - 1), LEVEL_ACQUIRE, 0.25))
    write(os.path.join(outdir, "rxsearch_c_layer.wav"), np.clip(render_layer(stems, frames, dt), -1, 1))
    for f in sorted(os.listdir(outdir)):
        if f.startswith("rxsearch_"):
            x, _ = read(os.path.join(outdir, f))
            print("S47 rxsearch: forhåndslytt %-30s %5.1f s  RMS %6.1f dBFS  topp %6.1f dBFS" % (f, len(x) / SR, db(rms(x)), db(np.abs(x).max())))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--src", default=SRC)
    ap.add_argument("--content", default=MAIN_CONTENT)
    ap.add_argument("--previews")
    o = ap.parse_args()
    if not os.path.isfile(o.src):
        print("S47 rxsearch: ingen %s: søkelaget blir ute (spillet går uten det)" % o.src)
        return 0
    x = decode(o.src)
    analyse(x)
    made = bands(x)
    dst = os.path.join(o.content, SUB)
    bad = 0
    for k, a, b, seg in made:
        p = os.path.join(dst, "search_%d.wav" % k)
        write(p, seg)
        y, sr = read(p)
        jump = abs(y[0] - y[-1])
        ok = sr == SR and len(y) == len(seg) and np.abs(y).max() < 0.9 and abs(y.mean()) < 1e-3 and jump < 0.05
        bad += not ok
        print("S47 rxsearch: %s %-12s %5.2f–%6.2f s  %5.2f s  RMS %6.1f dBFS  topp %5.1f  skjøt %.4f" % (
            "OK  " if ok else "FEIL", "search_%d" % k, a, b, len(y) / SR, db(rms(y)), db(np.abs(y).max()), jump))
    if o.previews:
        previews([read(os.path.join(dst, "search_%d.wav" % k))[0] for k in range(BANDS)], o.previews)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
