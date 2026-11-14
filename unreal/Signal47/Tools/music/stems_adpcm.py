#!/usr/bin/env python3
"""SIDEBAND-STEMS-ADPCM (14 November 2026): the staged stems (media/music/stems, 16-bit PCM at 22.05 kHz) as IMA ADPCM, 4 bits a
sample (ima_adpcm.py), to a folder of their own beside them (media/music/stems_adpcm) with a stems.json of the same rows; and the
measurement of what the 4 bits cost, stem by stem and piece by piece. Called from build_music.py:

  build_music.py stems --adpcm         transcode the staged PCM stems (they are not rendered again, and are left as they are)
  build_music.py stems --adpcm-check   decode the ADPCM set back with the mirror of the game's decoder and measure it against
                                       the PCM set: --report FILE (Markdown table), --ab DIR (A/B WAVs of the three worst cases)

The game prefers the PCM set while both are staged (S47Stems::bShipAdpcm, -S47StemsAdpcm), and plays the ADPCM set when it is
the only one: shipping it is staging only stems_adpcm.

Measured, all levels RMS against digital full scale (a full-scale square wave is 0 dBFS), A-weighted where it says (A):
  snr          the stem's power over the error's, over the whole file
  quiet        the quietest 2 s window (0.5 s hop) whose own level is over -90 dBFS(A): the stem's level there, the error's level
               there (the noise floor the 4 bits leave), and their difference
  worst block  the ADPCM block (46 ms) with the most error, in dBFS, and the lowest SNR of a block whose signal is over -60 dBFS
  white        the error's spectral flatness in the quiet window (1: white), and how its spectrum follows the stem's there
               (correlation of the two spectra in dB, 50 Hz to 10 kHz: near 1 means the noise has the music's colour, not hiss)
  pumps        across the file, the slope of the error's level against the stem's, block by block (1: the noise rises and falls
               with the music, 0: a steady floor)
  sum          the piece's stems added and through the game's tape curve at the piece's level (what the player hears with every
               stem in), ADPCM against PCM, with the same measures
"""
import json
import os
import sys
from multiprocessing import Pool

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import ima_adpcm as ima  # noqa: E402

RATE = 22050
FULL = 32768.0


def _aweight(n, rate=RATE):
    f = np.fft.rfftfreq(n, 1.0 / rate)
    f2 = f ** 2
    ra = 12194.0 ** 2 * f2 ** 2 / ((f2 + 20.6 ** 2) * np.sqrt((f2 + 107.7 ** 2) * (f2 + 737.9 ** 2)) * (f2 + 12194.0 ** 2))
    return ra * 10 ** (2.0 / 20)


def a_weighted(x):
    """Zero-phase A-weighting of (n, ch) by its whole-file spectrum."""
    X = np.fft.rfft(x, axis=0)
    return np.fft.irfft(X * _aweight(len(x))[:, None], n=len(x), axis=0)


def db(p):
    return 10 * np.log10(p + 1e-30)


def window_power(x, n, hop):
    """Mean power (over channels) of windows n long every hop."""
    p = (x ** 2).mean(axis=1)
    c = np.concatenate([[0.0], np.cumsum(p)])
    starts = np.arange(0, len(x) - n + 1, hop)
    return starts, (c[starts + n] - c[starts]) / n


def measure(x, y, spb):
    """x the PCM (reference), y the decoded, both (n, ch) float at full scale 1. The measures of the module's docstring."""
    e = y - x
    out = dict(snr_db=float(db((x ** 2).sum()) - db((e ** 2).sum())))
    xa, ea = a_weighted(x), a_weighted(e)
    n, hop = 2 * RATE, RATE // 2
    starts, px = window_power(xa, n, hop)
    _, pe = window_power(ea, n, hop)
    ok = db(px) > -90
    if ok.any():
        i = int(np.argmin(np.where(ok, px, np.inf)))
    else:
        i = int(np.argmin(px))
    s = int(starts[i])
    out.update(quiet_at_s=s / RATE, quiet_level_dbfs_a=float(db(px[i])), quiet_noise_dbfs_a=float(db(pe[i])), quiet_snr_a_db=float(db(px[i]) - db(pe[i])),
               loudest_noise_dbfs_a=float(db(pe.max())))
    # where it is heard (over -60 dBFS(A)): the window with the least SNR(A)
    heard = db(px) > -60
    if heard.any():
        j = int(np.argmin(np.where(heard, db(px) - db(pe), np.inf)))
        out.update(heard_min_snr_a_db=float(db(px[j]) - db(pe[j])), heard_min_at_s=float(starts[j] / RATE), heard_min_level_dbfs_a=float(db(px[j])), heard_min_noise_dbfs_a=float(db(pe[j])))
    else:
        out.update(heard_min_snr_a_db=None, heard_min_at_s=None, heard_min_level_dbfs_a=None, heard_min_noise_dbfs_a=None)
    # blocks
    nb = len(x) // spb
    bx = (x[:nb * spb] ** 2).reshape(nb, spb, -1).mean(axis=(1, 2))
    be = (e[:nb * spb] ** 2).reshape(nb, spb, -1).mean(axis=(1, 2))
    loud = db(bx) > -60
    out.update(worst_block_dbfs=float(db(be.max())), worst_block_at_s=float(np.argmax(be) * spb / RATE),
               worst_block_snr_db=float((db(bx) - db(be))[loud].min()) if loud.any() else None)
    # the error's spectrum in the quiet window
    from scipy.signal import welch
    f, Px = welch(x[s:s + n], RATE, nperseg=2048, axis=0)
    _, Pe = welch(e[s:s + n], RATE, nperseg=2048, axis=0)
    Px, Pe = Px.sum(axis=1), Pe.sum(axis=1)
    band = (f >= 50) & (f <= 10000)
    pe_b = Pe[band] + 1e-30
    out['quiet_error_flatness'] = float(np.exp(np.log(pe_b).mean()) / pe_b.mean())
    out['quiet_error_follows_signal'] = float(np.corrcoef(db(Px[band]), db(pe_b))[0, 1])
    # pumping: error level against signal level, block by block, where the stem sounds
    sel = db(bx) > -80
    out['pumps_slope'] = float(np.polyfit(db(bx[sel]), db(be[sel]), 1)[0]) if sel.sum() > 10 else None
    return out


def _encode_one(job):
    src, dst, shaping = job
    w = ima.read_wav(src)
    if w['format'] != 'pcm16' or w['rate'] != RATE:
        raise SystemExit('S47 adpcm: %s is not 16-bit PCM at %d Hz' % (src, RATE))
    data, align, spb = ima.encode(w['pcm'], shaping=shaping)
    ima.write_wav(dst + '.new', data, w['channels'], w['rate'], align, spb, w['frames'])
    back = ima.read_wav(dst + '.new')
    if back['frames'] != w['frames'] or back['channels'] != w['channels']:
        os.remove(dst + '.new')
        raise SystemExit('S47 adpcm: %s does not read back at its length' % dst)
    return dict(src=src, dst=dst, frames=w['frames'], channels=w['channels'], block_align=align, samples_per_block=spb,
                bytes=os.path.getsize(dst + '.new'), pcm_bytes=os.path.getsize(src))


def transcode(media, shaping=0.0, jobs=8):
    src_dir, dst_dir = os.path.join(media, 'music', 'stems'), os.path.join(media, 'music', 'stems_adpcm')
    index = json.load(open(os.path.join(src_dir, 'stems.json'), encoding='utf-8'))
    os.makedirs(dst_dir, exist_ok=True)
    todo = [(os.path.join(src_dir, f['file']), os.path.join(dst_dir, f['file']), shaping) for p in index['pieces'].values() for f in p['stems']]
    with Pool(jobs) as pool:
        done = pool.map(_encode_one, todo)
    by = {d['dst']: d for d in done}
    rows = dict(index)
    rows['pieces'] = {}
    total, total_pcm = 0, 0
    for name, p in index['pieces'].items():
        got = [by[os.path.join(dst_dir, f['file'])] for f in p['stems']]
        if len({g['frames'] for g in got}) != 1 or got[0]['frames'] != p['frames'] or len({g['samples_per_block'] for g in got}) != 1:
            for g in got:
                os.remove(g['dst'] + '.new')
            raise SystemExit('S47 adpcm: %s: the stems are not one length and one block size' % name)
        row = dict(p)
        row['bytes'] = sum(g['bytes'] for g in got)
        row['pcm_bytes'] = sum(g['pcm_bytes'] for g in got)
        row['adpcm'] = dict(block_align={f['file']: g['block_align'] for f, g in zip(p['stems'], got)}, samples_per_block=got[0]['samples_per_block'],
                            frames=got[0]['frames'], blocks=-(-got[0]['frames'] // got[0]['samples_per_block']))
        rows['pieces'][name] = row
        total += row['bytes']
        total_pcm += row['pcm_bytes']
    for g in done:
        os.replace(g['dst'] + '.new', g['dst'])
    rows['format'] = dict(index.get('format', {}), codec='ima_adpcm', wFormatTag=ima.FORMAT_TAG, bits=4, block_bytes_per_channel=ima.BLOCK_BYTES_PER_CHANNEL,
                          samples_per_block=ima.samples_per_block(ima.BLOCK_BYTES_PER_CHANNEL, 1), noise_shaping=shaping,
                          note='IMA ADPCM of the PCM set in ../stems (Tools/music/stems_adpcm.py); every block starts from its own header; '
                               'fact holds the frames, the last block is padded with zeros')
    rows['made_by'] = 'Tools/music/build_music.py stems --adpcm'
    path = os.path.join(dst_dir, 'stems.json')
    with open(path + '.tmp', 'w', encoding='utf-8') as f:
        json.dump(rows, f, indent=1, sort_keys=True)
    os.replace(path + '.tmp', path)
    print('S47 adpcm: %d stems of %d pieces to %s: %.1f MB (the PCM set %.1f MB, %.2f:1)' % (len(done), len(rows['pieces']), dst_dir, total / 1e6, total_pcm / 1e6, total_pcm / total))
    return rows


def _measure_piece(job):
    media, name, p = job
    src_dir, dst_dir = os.path.join(media, 'music', 'stems'), os.path.join(media, 'music', 'stems_adpcm')
    rows, xs, ys = [], [], []
    for f in p['stems']:
        a = ima.read_wav(os.path.join(src_dir, f['file']))
        b = ima.read_wav(os.path.join(dst_dir, f['file']))
        if b['format'] != 'ima_adpcm' or b['frames'] != a['frames']:
            raise SystemExit('S47 adpcm: %s: the ADPCM file is missing its form or its length' % f['file'])
        x, y = a['pcm'] / FULL, b['pcm'] / FULL
        m = measure(x, y, b['spb'])
        m.update(piece=name, stem=f['stem'], bytes=os.path.getsize(os.path.join(dst_dir, f['file'])), pcm_bytes=os.path.getsize(os.path.join(src_dir, f['file'])))
        rows.append(m)
        # (one-channel stems go to both speakers as they are: S47ScoreStems)
        xs.append(np.repeat(x, 2, axis=1) if x.shape[1] == 1 else x)
        ys.append(np.repeat(y, 2, axis=1) if y.shape[1] == 1 else y)
        spb = b['spb']
    lv = p['level']
    tape = lambda s: np.tanh(s / lv * 1.6) / 1.6 * lv  # noqa: E731
    sx, sy = tape(sum(xs)), tape(sum(ys))
    m = measure(sx, sy, spb)
    m.update(piece=name, stem='SUM', bytes=sum(r['bytes'] for r in rows), pcm_bytes=sum(r['pcm_bytes'] for r in rows))
    rows.append(m)
    return rows


def _write_ab(media, index, case, rank, ab_dir, at='quiet_at_s', tag='Q'):
    from scipy.io import wavfile
    src_dir, dst_dir = os.path.join(media, 'music', 'stems'), os.path.join(media, 'music', 'stems_adpcm')
    p = index['pieces'][case['piece']]
    if case['stem'] == 'SUM':
        lv = p['level']
        tape = lambda s: np.tanh(s / lv * 1.6) / 1.6 * lv  # noqa: E731
        two = lambda z: np.repeat(z, 2, axis=1) if z.shape[1] == 1 else z  # noqa: E731
        x = tape(sum(two(ima.read_wav(os.path.join(src_dir, f['file']))['pcm'] / FULL) for f in p['stems']))
        y = tape(sum(two(ima.read_wav(os.path.join(dst_dir, f['file']))['pcm'] / FULL) for f in p['stems']))
        label = 'all_stems'
    else:
        f = next(f for f in p['stems'] if f['stem'] == case['stem'])
        x = ima.read_wav(os.path.join(src_dir, f['file']))['pcm'] / FULL
        y = ima.read_wav(os.path.join(dst_dir, f['file']))['pcm'] / FULL
        label = case['stem']
    mid = int((case[at] + 1.0) * RATE)
    a, b = max(0, mid - 6 * RATE), min(len(x), max(0, mid - 6 * RATE) + 12 * RATE)
    x, y = x[a:b], y[a:b]
    gain = float(np.sqrt((x ** 2).sum() / max((y ** 2).sum(), 1e-30)))
    base = os.path.join(ab_dir, '%s%d_%s__%s_at_%ds' % (tag, rank, case['piece'], label, a // RATE))
    paths = (base + '_A_pcm.wav', base + '_B_adpcm.wav', base + '_C_difference_plus30dB.wav')
    diff = (y - x) * 10 ** (30 / 20)
    for path, z in zip(paths, (x, y, diff)):
        wavfile.write(path, RATE, z.astype(np.float32))
    return dict(files=[os.path.basename(q) for q in paths], level_match_db=20 * np.log10(gain), diff_peak_dbfs=float(20 * np.log10(np.abs(diff).max() + 1e-30)), start_s=a / RATE)


def check(media, report=None, ab_dir=None, jobs=6):
    dst_dir = os.path.join(media, 'music', 'stems_adpcm')
    index = json.load(open(os.path.join(dst_dir, 'stems.json'), encoding='utf-8'))
    with Pool(jobs) as pool:
        rows = [r for part in pool.map(_measure_piece, [(media, n, p) for n, p in index['pieces'].items()]) for r in part]
    worst = sorted(rows, key=lambda r: r['quiet_snr_a_db'])[:3]
    noisy = sorted([r for r in rows if r['heard_min_snr_a_db'] is not None], key=lambda r: r['heard_min_snr_a_db'])[:3]
    ab, abn = [], []
    if ab_dir:
        os.makedirs(ab_dir, exist_ok=True)
        ab = [_write_ab(media, index, c, i + 1, ab_dir) for i, c in enumerate(worst)]
        abn = [_write_ab(media, index, c, i + 1, ab_dir, at='heard_min_at_s', tag='N') for i, c in enumerate(noisy)]
    total, total_pcm = sum(r['bytes'] for r in rows if r['stem'] == 'SUM'), sum(r['pcm_bytes'] for r in rows if r['stem'] == 'SUM')
    lines = ['# SIDEBAND-STEMS-ADPCM: what 4 bits cost', '',
             'IMA ADPCM (WAVE 0x11, %d bytes a block per channel, %d samples a block), noise shaping %s. PCM set %.1f MB, ADPCM set %.1f MB (%.2f:1).' % (
                 ima.BLOCK_BYTES_PER_CHANNEL, ima.samples_per_block(ima.BLOCK_BYTES_PER_CHANNEL, 1), index['format'].get('noise_shaping'), total_pcm / 1e6, total / 1e6, total_pcm / total), '',
             'Levels in dBFS RMS (full-scale square 0 dBFS), (A) A-weighted, at the staged level (the game plays the score about 20 dB '
             'under full scale: these are the files, not the speaker). SUM: the stems added and through the tape curve at the piece\'s '
             'level, what the player hears with every stem in. Quiet: the quietest 2 s window over -90 dBFS(A). Flat: spectral flatness '
             'of the error there (1 white). Follows: correlation of the error\'s spectrum with the stem\'s there. Pumps: slope of block '
             'error level against block signal level (1 the noise moves with the music, 0 a steady floor).', '',
             '| piece | stem | SNR dB | quiet at s | level (A) | noise (A) | quiet SNR(A) | heard: least SNR(A) at s | loudest noise (A) | worst block dBFS | worst block SNR | flat | follows | pumps | MB |',
             '|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|']
    for r in rows:
        lines.append('| %s | %s | %.1f | %.1f | %.1f | %.1f | %.1f | %s | %.1f | %.1f | %s | %.2f | %.2f | %s | %.1f |' % (
            r['piece'].replace('s47_', ''), '**SUM**' if r['stem'] == 'SUM' else r['stem'], r['snr_db'], r['quiet_at_s'], r['quiet_level_dbfs_a'], r['quiet_noise_dbfs_a'],
            r['quiet_snr_a_db'], '%.1f at %.0f' % (r['heard_min_snr_a_db'], r['heard_min_at_s']) if r['heard_min_snr_a_db'] is not None else '-', r['loudest_noise_dbfs_a'], r['worst_block_dbfs'], '%.1f' % r['worst_block_snr_db'] if r['worst_block_snr_db'] is not None else '-',
            r['quiet_error_flatness'], r['quiet_error_follows_signal'], '%.2f' % r['pumps_slope'] if r['pumps_slope'] is not None else '-', r['bytes'] / 1e6))
    sums = [r for r in rows if r['stem'] == 'SUM']
    stems = [r for r in rows if r['stem'] != 'SUM']
    lines += ['', '## In short', '',
              '- stems: SNR %.1f to %.1f dB; quiet-window SNR(A) %.1f to %.1f dB; noise floor in the quiet window %.1f to %.1f dBFS(A)' % (
                  min(r['snr_db'] for r in stems), max(r['snr_db'] for r in stems), min(r['quiet_snr_a_db'] for r in stems), max(r['quiet_snr_a_db'] for r in stems),
                  min(r['quiet_noise_dbfs_a'] for r in stems), max(r['quiet_noise_dbfs_a'] for r in stems)),
              '- sums (heard): SNR %.1f to %.1f dB; quiet-window SNR(A) %.1f to %.1f dB; noise floor in the quiet window %.1f to %.1f dBFS(A)' % (
                  min(r['snr_db'] for r in sums), max(r['snr_db'] for r in sums), min(r['quiet_snr_a_db'] for r in sums), max(r['quiet_snr_a_db'] for r in sums),
                  min(r['quiet_noise_dbfs_a'] for r in sums), max(r['quiet_noise_dbfs_a'] for r in sums)),
              '', '## The three worst cases (lowest quiet-window SNR(A)), A/B files', '']
    for i, c in enumerate(worst):
        lines.append('%d. %s %s: quiet window at %.1f s, level %.1f dBFS(A), noise %.1f dBFS(A), SNR(A) %.1f dB' % (
            i + 1, c['piece'], c['stem'], c['quiet_at_s'], c['quiet_level_dbfs_a'], c['quiet_noise_dbfs_a'], c['quiet_snr_a_db']))
        if ab:
            lines.append('   files (12 s from %.1f s; A and B at the same gain, level match %+.3f dB; C peaks at %.1f dBFS): %s' % (
                ab[i]['start_s'], ab[i]['level_match_db'], ab[i]['diff_peak_dbfs'], ', '.join('`%s`' % f for f in ab[i]['files'])))
    lines += ['', '## The three noisiest where it is heard (least 2 s SNR(A) over -60 dBFS(A)), A/B files', '']
    for i, c in enumerate(noisy):
        lines.append('%d. %s %s: at %.1f s, level %.1f dBFS(A), noise %.1f dBFS(A), SNR(A) %.1f dB' % (
            i + 1, c['piece'], c['stem'], c['heard_min_at_s'], c['heard_min_level_dbfs_a'], c['heard_min_noise_dbfs_a'], c['heard_min_snr_a_db']))
        if abn:
            lines.append('   files (12 s from %.1f s; level match %+.3f dB; C peaks at %.1f dBFS): %s' % (
                abn[i]['start_s'], abn[i]['level_match_db'], abn[i]['diff_peak_dbfs'], ', '.join('`%s`' % f for f in abn[i]['files'])))
    text = '\n'.join(lines) + '\n'
    if report:
        os.makedirs(os.path.dirname(report), exist_ok=True)
        open(report, 'w', encoding='utf-8').write(text)
    print(text)
    return rows


if __name__ == '__main__':
    print(__doc__)
