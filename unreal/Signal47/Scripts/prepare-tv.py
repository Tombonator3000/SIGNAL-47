#!/usr/bin/env python3
"""Pack local videos as small JPEG frames for TVs without a platform video decoder."""
import argparse
import io
from pathlib import Path
import struct
import subprocess
import tempfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('videos', nargs='+', type=Path)
p.add_argument('--out', required=True, type=Path)
a = p.parse_args()
frames = []
with tempfile.TemporaryDirectory(prefix='s47-tv-') as tmp:
    for i, video in enumerate(a.videos):
        folder = Path(tmp) / str(i)
        folder.mkdir()
        subprocess.run(['ffmpeg', '-hide_banner', '-loglevel', 'error', '-i', str(video),
                        '-vf', 'fps=8,scale=320:240:force_original_aspect_ratio=decrease,pad=320:240:(ow-iw)/2:(oh-ih)/2,setsar=1',
                        '-q:v', '10', str(folder / '%06d.jpg')], check=True)
        frames.extend(f.read_bytes() for f in sorted(folder.glob('*.jpg')))
assert 0 < len(frames) <= 24000
out = io.BytesIO()
out.write(b'S47TV001')
out.write(struct.pack('<IIII', 8, 320, 240, len(frames)))
offset = 24 + 4 * (len(frames) + 1)
for frame in frames:
    out.write(struct.pack('<I', offset))
    offset += len(frame)
out.write(struct.pack('<I', offset))
for frame in frames:
    out.write(frame)
assert out.tell() == offset < 32 * 1024 * 1024
a.out.parent.mkdir(parents=True, exist_ok=True)
a.out.write_bytes(out.getvalue())
print(f'{a.out}: {len(frames)} frames, {len(frames)/8:.1f} s, {offset/1024/1024:.2f} MiB')
