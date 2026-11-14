#!/usr/bin/env python3
"""SIDEBAND-STEMS-ADPCM (14 November 2026): IMA ADPCM in standard WAVE files (wFormatTag 0x11, as Microsoft's "IMA ADPCM" and
every audio tool reads it), in pure numpy. The game's decoder is S47Adpcm in S47Adpcm.cpp; decode() here is a bit-exact mirror of
it, and reference_decode() is the plain one-sample-at-a-time form of the same arithmetic the mirror is tested against.

The block layout (the WAVE IMA ADPCM spec, Microsoft Multimedia Standards Update 1994):
  per block, per channel a 4-byte header: the block's first sample (int16, little-endian), the step index (0..88), a zero byte;
  then 4-byte words, channel after channel, each word 8 samples of that channel, the low nibble of each byte first.
  samples per block = (block_align - 4 * channels) * 8 / (4 * channels) + 1.
Every block starts from its own header: decoding never carries state from one block to the next. A sample's value is a
function of its block alone, so a seek is "jump to the block, decode forward in it", and a deck that seeks gives the same
samples as one that played through (S47Media.cpp relies on this for grouping, looping and crossfades).

Here: BLOCK_BYTES_PER_CHANNEL 512 (1017 samples per block, 46 ms at 22.05 kHz), the same for one- and two-channel files, so
every stem of a piece has the same blocks; the fact chunk holds the true length in frames, the last block is padded with zeros.

Usage (tests and a cross-check against ffmpeg when it is installed):
  ima_adpcm.py selftest
"""
import os
import struct
import subprocess
import sys

import numpy as np

STEPS = np.array([
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411,
    1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493,
    10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767], dtype=np.int32)
INDEX = np.array([-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8], dtype=np.int32)
BLOCK_BYTES_PER_CHANNEL = 512
FORMAT_TAG = 0x11


def samples_per_block(block_align, channels):
    return (block_align - 4 * channels) * 8 // (4 * channels) + 1


def _diff(step, code):
    """|delta| for a nibble, exactly as S47Adpcm::Nibble: ((2 * (code & 7) + 1) * step) >> 3, the rounding ffmpeg's
    adpcm_ima_wav decoder uses (bit-exact with it: the selftest checks). The 1992 IMA text and Windows' ACM codec add shifted
    steps instead (step >> 3, + step >> 2, ...), which truncates three times: a file from here decodes there off by up to some tens of LSB (41 on the selftest chord)."""
    return ((2 * (code & 7) + 1) * step) >> 3


def _step_state(pred, idx, code):
    d = _diff(STEPS[idx], code)
    pred = np.clip(np.where(code & 8, pred - d, pred + d), -32768, 32767)
    idx = np.clip(idx + INDEX[code], 0, 88)
    return pred, idx


# ------------------------------------------------------------------ decode
def decode_blocks(blocks, channels):
    """blocks: (n, block_align) uint8. Returns (n, spb, channels) int16. Vectorised over the blocks (they are independent)."""
    n, align = blocks.shape
    spb = samples_per_block(align, channels)
    out = np.zeros((n, spb, channels), dtype=np.int16)
    words = (align - 4 * channels) // (4 * channels)
    for c in range(channels):
        h = blocks[:, 4 * c:4 * c + 4].astype(np.int32)
        pred = (h[:, 0] | (h[:, 1] << 8)).astype(np.int16).astype(np.int32)
        idx = h[:, 2]
        if (idx > 88).any():
            raise ValueError('a block header has a step index over 88')
        out[:, 0, c] = pred
        # the channel's nibbles in order: word k of channel c at 4*ch + (k*ch + c)*4
        offs = 4 * channels + (np.arange(words)[:, None] * channels + c) * 4 + np.arange(4)[None, :]
        b = blocks[:, offs.reshape(-1)].astype(np.int32)                 # (n, words*4)
        nib = np.stack([b & 15, b >> 4], axis=2).reshape(n, -1)            # low nibble first
        for k in range(spb - 1):
            pred, idx = _step_state(pred, idx, nib[:, k])
            out[:, k + 1, c] = pred
    return out


def decode(data, channels, block_align, frames):
    """The data chunk to (frames, channels) int16. A last block shorter than block_align (other tools write one) decodes as far
    as its words go."""
    data = np.frombuffer(data, dtype=np.uint8)
    full = len(data) // block_align
    parts = []
    if full:
        parts.append(decode_blocks(data[:full * block_align].reshape(full, block_align), channels).reshape(-1, channels))
    rest = len(data) - full * block_align
    if rest >= 4 * channels and (rest - 4 * channels) % (4 * channels) == 0:
        parts.append(decode_blocks(data[full * block_align:].reshape(1, rest), channels).reshape(-1, channels))
    pcm = np.concatenate(parts) if parts else np.zeros((0, channels), np.int16)
    if len(pcm) < frames:
        raise ValueError('the fact chunk says %d frames, the data holds %d' % (frames, len(pcm)))
    return pcm[:frames]


def reference_decode(data, channels, block_align, frames):
    """The same decoder one sample at a time in plain Python (the form S47Adpcm.cpp is written in): the mirror's reference."""
    spb = samples_per_block(block_align, channels)
    out = []
    for at in range(0, len(data) - block_align + 1, block_align):
        blk = data[at:at + block_align]
        rows = [[0] * channels for _ in range(spb)]
        for c in range(channels):
            pred = struct.unpack_from('<h', blk, 4 * c)[0]
            idx = blk[4 * c + 2]
            rows[0][c] = pred
            k = 1
            for w in range((block_align - 4 * channels) // (4 * channels)):
                for byte in blk[4 * channels + (w * channels + c) * 4:4 * channels + (w * channels + c) * 4 + 4]:
                    for code in (byte & 15, byte >> 4):
                        step = int(STEPS[idx])
                        d = ((2 * (code & 7) + 1) * step) >> 3
                        pred = pred - d if code & 8 else pred + d
                        pred = max(-32768, min(32767, pred))
                        idx = max(0, min(88, idx + int(INDEX[code])))
                        rows[k][c] = pred
                        k += 1
        out.extend(rows)
    return np.array(out[:frames], dtype=np.int16).reshape(-1, channels)


# ------------------------------------------------------------------ encode
def _first_index(x0, x1):
    """A starting step index from the block's first difference (the step closest to it)."""
    d = np.abs(x1.astype(np.int64) - x0)
    return np.clip(np.searchsorted(STEPS, d), 0, 88).astype(np.int32)


def _encode_pass(x, idx0, shaping, lookahead=False):
    """x: (n, spb) int32 samples of one channel, idx0: (n,) the header's step index. Each nibble is the one of the 16 whose
    reconstruction is nearest the target (the target is the sample, less the shaped error when shaping > 0)."""
    n, spb = x.shape
    pred = x[:, 0].astype(np.int32)
    idx = idx0.copy()
    codes = np.zeros((n, spb - 1), dtype=np.int32)
    err = np.zeros(n)
    allc = np.arange(16, dtype=np.int32)[None, :]
    rows = np.arange(n)
    for k in range(1, spb):
        target = x[:, k] - shaping * err
        d = _diff(STEPS[idx][:, None], allc)
        cand = np.clip(np.where(allc & 8, pred[:, None] - d, pred[:, None] + d), -32768, 32767)
        cost = (cand - target[:, None]).astype(np.float64) ** 2
        if lookahead and k + 1 < spb:
            # one sample ahead: each of the 16 codes with the best of the 16 after it (the step a code leaves matters next)
            idx_a = np.clip(idx[:, None] + INDEX[None, :], 0, 88)                                   # (n, 16)
            d2 = _diff(STEPS[idx_a][:, :, None], allc[:, None, :])                                  # (n, 16, 16)
            c2 = np.clip(np.where(allc[:, None, :] & 8, cand[:, :, None] - d2, cand[:, :, None] + d2), -32768, 32767)
            cost = cost + ((c2 - x[:, k + 1, None, None]).astype(np.float64) ** 2).min(axis=2)
        best = np.argmin(cost, axis=1).astype(np.int32)
        new = cand[rows, best]
        err = new - target
        pred = new
        idx = np.clip(idx + INDEX[best], 0, 88)
        codes[:, k - 1] = best
    return codes, idx


def encode(pcm, block_align=None, shaping=0.0, lookahead=True):
    """pcm: (frames, channels) int16. Returns (data bytes, block_align, samples per block). Two passes: the first starts every
    block at a step index guessed from its first difference; the second starts each block where the first pass left the block
    before it (what a one-pass sequential encoder carries over), so the step is already right at the block's start."""
    frames, ch = pcm.shape
    block_align = block_align or BLOCK_BYTES_PER_CHANNEL * ch
    spb = samples_per_block(block_align, ch)
    nb = -(-frames // spb)
    padded = np.zeros((nb * spb, ch), dtype=np.int32)
    padded[:frames] = pcm
    blocks = np.zeros((nb, block_align), dtype=np.uint8)
    words = (block_align - 4 * ch) // (4 * ch)
    for c in range(ch):
        x = padded[:, c].reshape(nb, spb)
        idx0 = _first_index(x[:, 0], x[:, 1])
        _, end = _encode_pass(x, idx0, shaping, lookahead)
        idx1 = np.concatenate([idx0[:1], end[:-1]])
        codes, _ = _encode_pass(x, idx1, shaping, lookahead)
        hdr = x[:, 0].astype(np.int16).view(np.uint16)
        blocks[:, 4 * c] = hdr & 255
        blocks[:, 4 * c + 1] = hdr >> 8
        blocks[:, 4 * c + 2] = idx1
        packed = (codes[:, 0::2] | (codes[:, 1::2] << 4)).astype(np.uint8)   # (nb, words*4): low nibble first
        for w in range(words):
            at = 4 * ch + (w * ch + c) * 4
            blocks[:, at:at + 4] = packed[:, w * 4:w * 4 + 4]
    return blocks.tobytes(), block_align, spb


# ------------------------------------------------------------------ files
def write_wav(path, data, channels, rate, block_align, spb, frames):
    fmt = struct.pack('<HHIIHHHH', FORMAT_TAG, channels, rate, int(round(rate * block_align / spb)), block_align, 4, 2, spb)
    fact = struct.pack('<I', frames)
    body = b'WAVE' + b'fmt ' + struct.pack('<I', len(fmt)) + fmt + b'fact' + struct.pack('<I', 4) + fact + b'data' + struct.pack('<I', len(data)) + data
    if len(data) & 1:
        body += b'\0'
    with open(path, 'wb') as f:
        f.write(b'RIFF' + struct.pack('<I', len(body)) + body)


def read_wav(path):
    """Either form the game reads. Returns dict(format='pcm16'|'ima_adpcm', channels, rate, frames, pcm (frames, ch) int16, ...)."""
    raw = open(path, 'rb').read()
    if raw[:4] != b'RIFF' or raw[8:12] != b'WAVE':
        raise ValueError('%s: not RIFF WAVE' % path)
    at, fmt, fact, data = 12, None, None, None
    while at + 8 <= len(raw):
        tag, n = raw[at:at + 4], struct.unpack_from('<I', raw, at + 4)[0]
        if n > len(raw) - at - 8:
            raise ValueError('%s: chunk %r runs past the end' % (path, tag))
        body = raw[at + 8:at + 8 + n]
        if tag == b'fmt ':
            fmt = body
        elif tag == b'fact':
            fact = struct.unpack_from('<I', body)[0]
        elif tag == b'data':
            data = body
        at += 8 + n + (n & 1)
    tagv, ch, rate, _, align, bits = struct.unpack_from('<HHIIHH', fmt)
    if tagv == 1 and bits == 16:
        pcm = np.frombuffer(data, dtype='<i2')[:len(data) // (2 * ch) * ch].reshape(-1, ch)
        return dict(format='pcm16', channels=ch, rate=rate, frames=len(pcm), pcm=pcm)
    if tagv == FORMAT_TAG and bits == 4:
        spb = struct.unpack_from('<H', fmt, 18)[0]
        if spb != samples_per_block(align, ch):
            raise ValueError('%s: samples per block %d does not match block align %d' % (path, spb, align))
        frames = fact if fact is not None else len(data) // align * spb
        return dict(format='ima_adpcm', channels=ch, rate=rate, frames=frames, block_align=align, spb=spb, data=data,
                    pcm=decode(data, ch, align, frames))
    raise ValueError('%s: format %#x, %d bits: neither PCM16 nor IMA ADPCM' % (path, tagv, bits))


# ------------------------------------------------------------------ tests
def selftest(tmp=None):
    rng = np.random.default_rng(47)
    ok = True

    def check(b, what):
        nonlocal ok
        print('%s ima_adpcm: %s' % ('PASS' if b else 'FAIL', what))
        ok &= bool(b)

    # test vectors: random bytes (any nibble, any header with a legal index) through the mirror and the reference
    for ch in (1, 2):
        align = BLOCK_BYTES_PER_CHANNEL * ch
        raw = rng.integers(0, 256, size=(5, align), dtype=np.uint8)
        for c in range(ch):
            raw[:, 4 * c + 2] = rng.integers(0, 89, size=5)
            raw[:, 4 * c + 3] = 0
        raw[0, 0:2] = (0xff, 0x7f)   # a header at the rail
        data = raw.tobytes()
        frames = 5 * samples_per_block(align, ch) - 100
        check(np.array_equal(decode(data, ch, align, frames), reference_decode(data, ch, align, frames)),
              'the vectorised mirror equals the one-sample reference on random blocks (%d channel%s, clipping included)' % (ch, 's' if ch > 1 else ''))
    # round trip of a synthetic signal: a chord with a slow swell, and its quiet tail
    t = np.arange(22050 * 3) / 22050.0
    sig = (0.3 * np.sin(2 * np.pi * 220 * t) + 0.2 * np.sin(2 * np.pi * 331 * t + 1) + 0.05 * rng.standard_normal(len(t))) * np.minimum(1, t)
    pcm = np.round(np.stack([sig, sig * 0.7], axis=1) * 32767).astype(np.int16)
    data, align, spb = encode(pcm)
    dec = decode(data, 2, align, len(pcm))
    e = dec.astype(np.float64) - pcm
    snr = 10 * np.log10((pcm.astype(np.float64) ** 2).sum() / (e ** 2).sum())
    check(snr > 20 and len(dec) == len(pcm), 'round trip of a chord with noise: SNR %.1f dB (> 20), length kept' % snr)
    check(np.array_equal(decode(data, 2, align, len(pcm))[spb * 3 + 17:spb * 3 + 400],
                         decode(data[3 * align:], 2, align, len(pcm) - 3 * spb)[17:400]),
          'a block decodes alone to what it is in the whole stream (seek = jump to the block)')
    zeros, za, _ = encode(np.zeros((5000, 1), np.int16))
    check(not decode(zeros, 1, za, 5000).any(), 'silence stays exactly silent')
    # the file form, and ffmpeg's reading of it when ffmpeg is here (a cross-check of the layout, not of the arithmetic)
    tmp = tmp or os.path.join(os.environ.get('TMPDIR', '/tmp'), 's47_ima_selftest.wav')
    write_wav(tmp, data, 2, 22050, align, spb, len(pcm))
    back = read_wav(tmp)
    check(back['format'] == 'ima_adpcm' and back['frames'] == len(pcm) and np.array_equal(back['pcm'], dec), 'written and read back as a WAVE file')
    try:
        r = subprocess.run(['ffmpeg', '-v', 'error', '-i', tmp, '-f', 's16le', '-acodec', 'pcm_s16le', '-'], capture_output=True)
        if r.returncode == 0:
            ff = np.frombuffer(r.stdout, dtype='<i2').reshape(-1, 2)   # (ffmpeg plays the padding too: it ignores fact)
            m = min(len(ff), len(dec))
            diff = int(np.abs(ff[:m].astype(np.int32) - dec[:m]).max()) if m else -1
            check(m == len(dec) and diff == 0, "ffmpeg's adpcm_ima_wav decoder reads the file bit-exactly: %d of %d frames, largest difference %d LSB" % (m, len(dec), diff))
        else:
            print('INFO ima_adpcm: ffmpeg could not read the file: %s' % r.stderr.decode('utf-8', 'replace')[-200:])
    except FileNotFoundError:
        print('INFO ima_adpcm: no ffmpeg here: the cross-check is skipped')
    os.remove(tmp)
    return ok


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'selftest':
        sys.exit(0 if selftest() else 1)
    print(__doc__)
