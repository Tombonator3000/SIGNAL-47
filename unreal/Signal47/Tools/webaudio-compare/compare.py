#!/usr/bin/env python3
"""The game's Web Audio (S47WebAudio) held against the browser's: the same graphs with known input
(page.html here, US47Audio::Compare in Source/Signal47/Private/S47AudioTour.cpp), rendered by
Chromium's OfflineAudioContext and by the game, compared sample by sample.

  1. the browser:  <python with playwright> compare.py chrome OUT_DIR
                   (Tom's venv has it: ~/.venvs/signal47/bin/python)
  2. the game:     UnrealEditor <Signal47.uproject> -game -S47SmokeTest -S47Seconds=2 -RenderOffscreen
                   -nosound -unattended -S47AudioCompare=OUT_DIR2
  3. compare:      python3 compare.py diff OUT_DIR OUT_DIR2

Each case is 2 s of stereo float at 48 kHz (<case>.f32, interleaved). The browser renders a few more
cases than the game (the raw waveforms, to measure its normalisation)."""
import math
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def chrome(out_dir):
    import asyncio
    import base64
    import json
    from playwright.async_api import async_playwright

    async def main():
        async with async_playwright() as p:
            b = await p.chromium.launch(args=['--autoplay-policy=no-user-gesture-required'])
            pg = await b.new_page()
            await pg.goto('file://' + os.path.join(HERE, 'page.html'))
            await pg.wait_for_function("document.getElementById('out').textContent !== 'running'", timeout=120000)
            t = await pg.eval_on_selector('#out', 'e => e.textContent')
            print('chromium', b.version)
            await b.close()
        if not t.startswith('RESULT'):
            raise SystemExit(t[:500])
        os.makedirs(out_dir, exist_ok=True)
        for name, (l, r) in json.loads(t[len('RESULT'):-len('END')]).items():
            L, R = base64.b64decode(l), base64.b64decode(r)
            n = len(L) // 4
            fl, fr = struct.unpack('<%df' % n, L), struct.unpack('<%df' % n, R)
            with open(os.path.join(out_dir, name + '.f32'), 'wb') as f:
                f.write(struct.pack('<%df' % (2 * n), *[v for pair in zip(fl, fr) for v in pair]))
    asyncio.run(main())


def load(p):
    data = open(p, 'rb').read()
    v = struct.unpack('<%df' % (len(data) // 4), data)
    return v[0::2], v[1::2]


def diff(a_dir, b_dir):
    W = 960
    for name in sorted(os.listdir(a_dir)):
        if not name.endswith('.f32') or not os.path.exists(os.path.join(b_dir, name)):
            continue
        (al, ar), (bl, br) = load(os.path.join(a_dir, name)), load(os.path.join(b_dir, name))
        n = min(len(al), len(bl))
        maxdiff = max(max(abs(al[i] - bl[i]) for i in range(n)), max(abs(ar[i] - br[i]) for i in range(n)))
        peak = max(max(abs(x) for x in al[:n]), max(abs(x) for x in ar[:n]))
        worst, errs = 0.0, []
        for w in range(0, n - W, W):
            for x, y in ((al, bl), (ar, br)):
                ra = math.sqrt(sum(v * v for v in x[w:w + W]) / W)
                rb = math.sqrt(sum(v * v for v in y[w:w + W]) / W)
                if max(ra, rb) > 1e-3:
                    d = abs(20 * math.log10(max(ra, 1e-9) / max(rb, 1e-9)))
                    errs.append(d)
                    worst = max(worst, d)
        print('%-11s peak %.3f  max sample difference %.2e (%.1f%% of peak)  20 ms rms: mean %.2f dB, worst %.2f dB'
              % (name[:-4], peak, maxdiff, 100 * maxdiff / max(peak, 1e-9), sum(errs) / max(1, len(errs)), worst))


if __name__ == '__main__':
    if len(sys.argv) >= 3 and sys.argv[1] == 'chrome':
        chrome(sys.argv[2])
    elif len(sys.argv) >= 4 and sys.argv[1] == 'diff':
        diff(sys.argv[2], sys.argv[3])
    else:
        print(__doc__)
