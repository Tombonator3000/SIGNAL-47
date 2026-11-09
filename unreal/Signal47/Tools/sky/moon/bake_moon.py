"""PT-MOON-20261109: the moon's near side as a disc, from NASA's CGI Moon Kit (public domain).

Source: NASA's Scientific Visualization Studio, CGI Moon Kit (ID 4720), lroc_color_poles_2k.tif: LRO LROC colour, equirectangular,
0 degrees longitude in the middle. https://svs.gsfc.nasa.gov/4720 . NASA SVS: "All of our content is in the public domain (unless
otherwise noted)". Credit: NASA's Scientific Visualization Studio (Ernie Wright, USRA; Noah Petro, NASA/GSFC).

What it makes: T_S47_Moon.png, the near side seen from the Earth (orthographic, no libration), lunar north up and Mare Crisium on
the right, as a northern observer sees it. Albedo only: the phase, the terminator and the earthshine are drawn by M_S47_Moon
(Tools/sky/import_moon.py) from where the sun is. Outside the disc the rim's colour carries on, so that no mip darkens the limb.
Somewhat desaturated: the moon at night reads grey with a little warmth, not the LROC map's enhanced browns.

Plain Python and Pillow (no numpy):
  python3 bake_moon.py /path/to/lroc_color_poles_2k.tif [out.png] [size]
"""
import math
import sys
from pathlib import Path

from PIL import Image

SATURATION = .45     # of the map's own colour
BRIGHT_PERCENTILE = .995  # this share of the disc's pixels at or under white
SUPER = 3            # samples per pixel each way


def main():
    src = Path(sys.argv[1])
    out = Path(sys.argv[2]) if len(sys.argv) > 2 else Path(__file__).resolve().parent / 'T_S47_Moon.png'
    size = int(sys.argv[3]) if len(sys.argv) > 3 else 512
    im = Image.open(src).convert('RGB')
    W, H = im.size
    px = im.load()
    # sRGB to linear and back, for averaging and scaling in light, not in code values
    lin = [((i / 255) / 12.92) if i <= 10 else (((i / 255) + .055) / 1.055) ** 2.4 for i in range(256)]

    def to_srgb(v):
        v = max(0.0, min(1.0, v))
        return int(round(255 * (12.92 * v if v <= .0031308 else 1.055 * v ** (1 / 2.4) - .055)))

    def sample(lon, lat):
        # bilinear in the equirectangular map; longitude wraps, latitude clamps
        u = (lon / (2 * math.pi) + .5) * W - .5
        v = (.5 - lat / math.pi) * H - .5
        x0, y0 = math.floor(u), math.floor(v)
        fx, fy = u - x0, v - y0
        acc = [0.0, 0.0, 0.0]
        for dy, wy in ((0, 1 - fy), (1, fy)):
            yy = min(max(y0 + dy, 0), H - 1)
            for dx, wx in ((0, 1 - fx), (1, fx)):
                c = px[(x0 + dx) % W, yy]
                w = wx * wy
                acc[0] += lin[c[0]] * w; acc[1] += lin[c[1]] * w; acc[2] += lin[c[2]] * w
        return acc

    data = []
    lum = []
    for y in range(size):
        for x in range(size):
            acc = [0.0, 0.0, 0.0]
            for sy in range(SUPER):
                for sx in range(SUPER):
                    r = ((x + (sx + .5) / SUPER) / size) * 2 - 1      # right
                    u = 1 - ((y + (sy + .5) / SUPER) / size) * 2      # up
                    d = math.hypot(r, u)
                    if d > .999:   # past the limb: the limb's own colour, for the mips
                        r, u = r / d * .999, u / d * .999
                    z = math.sqrt(max(0.0, 1 - r * r - u * u))         # toward the Earth
                    c = sample(math.atan2(r, z), math.asin(u))
                    acc[0] += c[0]; acc[1] += c[1]; acc[2] += c[2]
            n = SUPER * SUPER
            c = [acc[0] / n, acc[1] / n, acc[2] / n]
            l = .2126 * c[0] + .7152 * c[1] + .0722 * c[2]
            c = [l + (k - l) * SATURATION for k in c]
            data.append(c)
            if math.hypot((x + .5) / size * 2 - 1, (y + .5) / size * 2 - 1) < .98:
                lum.append(l)
    lum.sort()
    white = lum[int(len(lum) * BRIGHT_PERCENTILE)]
    mean = sum(lum) / len(lum)
    img = Image.new('RGB', (size, size))
    img.putdata([tuple(to_srgb(k / white) for k in c) for c in data])
    img.save(out)
    print('T_S47_Moon: %dx%d, white at linear %.4f, disc mean %.3f of white, -> %s' % (size, size, white, mean / white, out))


if __name__ == '__main__':
    main()
