#!/usr/bin/env python3
"""MUSIC-20261113: what can be said about a finished piece without hearing it.

  measure_music.py [-q] FILE...     (-q: the summary only, without the 15-second sections)

Per file: length, integrated loudness and loudness range (ffmpeg ebur128), true peak, the loudness of its mono sum (3 LU
under the stereo figure is one sound in both ears, 6 LU is two unrelated ones, more is something out of phase), DC, how quiet
its two ends are, and per 15 seconds: level, the correlation between the channels, spectral centroid, how much lies under
120 Hz, and its pitch classes between 55 and 2000 Hz. SEMITONE marks a section where two neighbouring pitch classes both
carry 12 % or more: a cluster, which is fine when it is meant. The click figure is the sharpest corner of each 50 ms against
that 50 ms' own median: a splice stands alone in the tens to hundreds, a gritty drone raises many blocks at once.
Needs python3, numpy and ffmpeg (on this machine: /home/tom/stable-diffusion-webui-forge/venv/bin/python3 -I).
"""
import os, re, subprocess, sys
import numpy as np
SR = 44100
NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']


def ebu(path, pre=None):
    chain = (pre + ',' if pre else '') + 'ebur128=peak=true'
    r = subprocess.run(['ffmpeg', '-hide_banner', '-nostats', '-i', path, '-af', chain, '-f', 'null', '-'], capture_output=True)
    t = r.stderr.decode('utf-8', 'replace')
    t = t[t.rfind('Summary:'):]
    g = lambda pat: float(re.search(pat, t).group(1))
    return g(r'I:\s+(-?[\d.]+) LUFS'), g(r'LRA:\s+(-?[\d.]+) LU'), g(r'Peak:\s+(-?[\d.]+) dBFS')


def load(path):
    raw = subprocess.run(['ffmpeg', '-v', 'error', '-i', path, '-ac', '2', '-ar', str(SR), '-f', 'f32le', '-'], capture_output=True).stdout
    return np.frombuffer(raw, dtype=np.float32).astype(np.float64).reshape(-1, 2)


def chroma(x, lo=55, hi=2000, n=16384):
    if len(x) < n:
        return np.zeros(12)
    hop = n // 2
    m = 1 + (len(x) - n) // hop
    idx = np.arange(n)[None, :] + hop * np.arange(m)[:, None]
    S = (np.abs(np.fft.rfft(x[idx] * np.hanning(n), axis=1)) ** 2).sum(axis=0)
    f = np.fft.rfftfreq(n, 1 / SR)
    sel = (f >= lo) & (f <= hi)
    midi = 69 + 12 * np.log2(f[sel] / 440.0)
    pc = np.mod(np.round(midi), 12).astype(int)
    w = np.cos(np.pi * (midi - np.round(midi))) ** 2
    c = np.array([(S[sel][pc == k] * w[pc == k]).sum() for k in range(12)])
    return c / (c.sum() + 1e-30)


def report(path, win=15.0, quiet=False):
    I, lra, tp = ebu(path)
    Im, _, _ = ebu(path, 'pan=mono|c0=0.5*c0+0.5*c1')
    x = load(path)
    secs = len(x) / SR
    mono = x.mean(axis=1)
    dc = x.mean(axis=0)
    out = ['%s  %d:%04.1f  I %.1f LUFS  LRA %.1f  TP %.1f dBTP  mono %.1f LUFS (%.1f)  DC %.5f/%.5f' % (
        os.path.basename(path), secs // 60, secs % 60, I, lra, tp, Im, Im - I, dc[0], dc[1])]
    w = int(win * SR)
    nseg = int(np.ceil(len(x) / w))
    lv, cor, rows, clash = [], [], [], 0
    f = np.fft.rfftfreq(16384, 1 / SR)
    for i in range(nseg):
        s = x[i * w:(i + 1) * w]
        r = 10 * np.log10((s ** 2).mean() + 1e-12)
        lv.append(r)
        c = np.corrcoef(s[:, 0], s[:, 1])[0, 1] if s.std() > 1e-6 else 1.0
        cor.append(c)
        ch = chroma(s.mean(axis=1))
        top = [k for k in np.argsort(ch)[::-1][:5] if ch[k] >= 0.08]
        bad = [(NAMES[k], NAMES[(k + 1) % 12]) for k in range(12) if ch[k] >= 0.12 and ch[(k + 1) % 12] >= 0.12]
        clash += bool(bad)
        m = s.mean(axis=1)
        S = np.abs(np.fft.rfft(m[:len(m) // 16384 * 16384].reshape(-1, 16384) * np.hanning(16384), axis=1)) ** 2 if len(m) >= 16384 else np.ones((1, 8193))
        P = S.sum(axis=0)
        cen = (f * P).sum() / (P.sum() + 1e-30)
        low = 100 * P[f < 120].sum() / (P.sum() + 1e-30)
        rows.append('  %3d-%3d s  %6.1f dB  corr %+.2f  cen %4d Hz  sub %2d%%  %s%s' % (i * win, min(secs, (i + 1) * win), r, c, cen, low,
                    ' '.join('%s%d' % (NAMES[k], round(100 * ch[k])) for k in top), ('   SEMITONE ' + ','.join(a + '+' + b for a, b in bad)) if bad else ''))
    # clicks: the second difference against its own surroundings (50 ms)
    d2 = np.abs(np.diff(mono, 2))
    blk = SR // 20
    nb = len(d2) // blk
    B = d2[:nb * blk].reshape(nb, blk)
    med = np.median(B, axis=1) + 1e-7
    ratio = B.max(axis=1) / med
    worst = int(np.argmax(ratio))
    # ends
    head = 10 * np.log10((x[:SR // 2] ** 2).mean() + 1e-12)
    tail = 10 * np.log10((x[-SR // 2:] ** 2).mean() + 1e-12)
    out.append('  ends: first 0.5 s %.1f dB, last 0.5 s %.1f dB; sections %.1f..%.1f dB; corr min %.2f; semitone windows %d of %d; click ratio max %.0f at %.1f s (p99 %.0f); sample peak %.1f dBFS' % (
        head, tail, min(lv), max(lv), min(cor), clash, nseg, ratio.max(), worst * blk / SR, np.percentile(ratio, 99), 20 * np.log10(np.abs(x).max() + 1e-12)))
    whole = chroma(mono)
    out.append('  whole: ' + ' '.join('%s%d' % (NAMES[k], round(100 * whole[k])) for k in np.argsort(whole)[::-1][:7]))
    if not quiet:
        out += rows
    print('\n'.join(out))


if __name__ == '__main__':
    q = '-q' in sys.argv
    for p in sys.argv[1:]:
        if p != '-q':
            report(p, quiet=q)
