#!/usr/bin/env python3
"""Derive the continuous low-volume generator loop from the retained licensed recording."""
import hashlib
import json
import struct
import subprocess
import wave
from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = root / 'Docs/Resources19/Audio/engine_noise.ogg'
output = root / 'Unity/Assets/Signal47/Art/Environment27/Audio/GeneratorLoop.wav'
assert hashlib.sha256(source.read_bytes()).hexdigest() == '6dad15c22608b68e61a141e4fe0fe92c96c9a8aca78c75ca1ff27f2c13048447'
raw = subprocess.check_output(['ffmpeg', '-v', 'error', '-i', str(source), '-f', 'f32le', '-ac', '1', '-ar', '22050', 'pipe:1'])
samples = list(struct.unpack('<' + 'f' * (len(raw) // 4), raw))
n = 8820  # 400 ms; blend the tail into the head, preserving the original middle.
crossfade = [samples[-n+i] * (1-i/(n-1)) + samples[i] * i/(n-1) for i in range(n)]
loop = samples[n:-n] + crossfade
mean = sum(loop) / len(loop)
loop = [v-mean for v in loop]
output.parent.mkdir(parents=True, exist_ok=True)
with wave.open(str(output), 'wb') as audio:
    audio.setnchannels(1)
    audio.setsampwidth(2)
    audio.setframerate(22050)
    audio.writeframes(struct.pack('<' + 'h' * len(loop), *(max(-32767, min(32767, round(v*32767))) for v in loop)))
print(json.dumps({'file': str(output), 'sha256': hashlib.sha256(output.read_bytes()).hexdigest(),
                  'duration_s': len(loop)/22050, 'peak': max(map(abs, loop)), 'seam_delta': abs(loop[-1]-loop[0])}))
