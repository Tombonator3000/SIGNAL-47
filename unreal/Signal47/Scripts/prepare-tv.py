#!/usr/bin/env python3
"""Pack local videos for the TVs (FS47TvProgramme): small JPEG frames at the sources' own rate,
and their sound beside them, cut to the frames so the two loop together.

  Content/WebData/tv/unarius.s47tv     'S47TV001', fps, 320, 240, frame count, the offsets, the JPEGs
  Content/Audio/Raw/tv/unarius.wav     mono PCM16 at 24 kHz (2000 samples to a frame at 12 fps); each
                                       video's sound padded or cut to exactly its frames' length

Both folders are ignored by Git and staged as loose files (DefaultGame.ini). The footage is local
only: its rights are not cleared, so the pack and the sound are never committed or shared.
No platform video decoder and no UObject per frame: the game decodes one JPEG when the frame changes.
Needs ffmpeg and ffprobe.
Usage: prepare-tv.py VIDEO [VIDEO ...] [--out PACK] [--audio WAV] [--fps 12] [--quality 10]"""
import argparse
import io
from pathlib import Path
import struct
import subprocess
import tempfile
import wave

HERE = Path(__file__).resolve().parent
CONTENT = HERE.parent / 'Content'
RATE = 24000

p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
p.add_argument('videos', nargs='+', type=Path)
p.add_argument('--out', type=Path, default=CONTENT / 'WebData' / 'tv' / 'unarius.s47tv')
p.add_argument('--audio', type=Path, default=CONTENT / 'Audio' / 'Raw' / 'tv' / 'unarius.wav')
p.add_argument('--fps', type=int, default=12, help='frames a second (the sources are 12)')
p.add_argument('--quality', type=int, default=10, help='JPEG -q:v, 2 (best) to 31')
a = p.parse_args()
assert 1 <= a.fps <= 30 and RATE % a.fps == 0
frames, pcm = [], bytearray()
with tempfile.TemporaryDirectory(prefix='s47-tv-') as tmp:
    for i, video in enumerate(a.videos):
        folder = Path(tmp) / str(i)
        folder.mkdir()
        subprocess.run(['ffmpeg', '-hide_banner', '-loglevel', 'error', '-i', str(video),
                        '-vf', f'fps={a.fps},scale=320:240:force_original_aspect_ratio=decrease,pad=320:240:(ow-iw)/2:(oh-ih)/2,setsar=1',
                        '-q:v', str(a.quality), str(folder / '%06d.jpg')], check=True)
        own = [f.read_bytes() for f in sorted(folder.glob('*.jpg'))]
        assert own, video
        frames.extend(own)
        # the sound for exactly these frames: mono, 24 kHz, silence after if it is short, cut if long
        samples = len(own) * (RATE // a.fps)
        raw = subprocess.run(['ffmpeg', '-hide_banner', '-loglevel', 'error', '-i', str(video), '-vn', '-ac', '1', '-ar', str(RATE),
                              '-af', 'apad', '-t', '%.6f' % (samples / RATE), '-c:a', 'pcm_s16le', '-f', 's16le', '-'],
                             check=True, capture_output=True).stdout
        raw = raw[:samples * 2] + bytes(max(0, samples * 2 - len(raw)))
        pcm += raw
        print(f'{video.name}: {len(own)} frames, {len(own) / a.fps:.2f} s, sound {len(raw) // 2 / RATE:.2f} s')
assert 0 < len(frames) <= 24000
out = io.BytesIO()
out.write(b'S47TV001')
out.write(struct.pack('<IIII', a.fps, 320, 240, len(frames)))
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
assert len(pcm) == len(frames) * (RATE // a.fps) * 2
a.audio.parent.mkdir(parents=True, exist_ok=True)
with wave.open(str(a.audio), 'wb') as w:
    w.setnchannels(1)
    w.setsampwidth(2)
    w.setframerate(RATE)
    w.writeframes(bytes(pcm))
print(f'{a.out}: {len(frames)} frames at {a.fps} fps, {len(frames) / a.fps:.2f} s, {offset / 1024 / 1024:.2f} MiB')
print(f'{a.audio}: {len(pcm) // 2 / RATE:.2f} s at {RATE} Hz, {a.audio.stat().st_size / 1024 / 1024:.2f} MiB')
