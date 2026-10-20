#!/usr/bin/env python3
"""Resumable, serial VoiceStudio generation for the optional offline parody voice.

Run extract.mjs first. API access and cached WAVs are build-time only. No source
reference audio, service URL or local machine paths are put in the runtime catalog.
"""
from pathlib import Path
import argparse
import array
import datetime
import hashlib
import json
import math
import re
import subprocess
import urllib.request
import uuid

def atomic_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n')
    temporary.replace(path)


def multipart(fields):
    boundary = 'signal47-' + uuid.uuid4().hex
    chunks = []
    for key, value in fields.items():
        if isinstance(value, bool):
            value = str(value).lower()
        chunks += [f'--{boundary}\r\nContent-Disposition: form-data; name="{key}"\r\n\r\n'.encode(),
                   str(value).encode(), b'\r\n']
    chunks.append(f'--{boundary}--\r\n'.encode())
    return b''.join(chunks), 'multipart/form-data; boundary=' + boundary


def generate(endpoint, fields):
    body, content_type = multipart(fields)
    request = urllib.request.Request(endpoint + '/generate', data=body,
                                     headers={'Content-Type': content_type}, method='POST')
    done = None
    with urllib.request.urlopen(request, timeout=300) as response:
        if 'application/json' in response.headers.get('Content-Type', ''):
            done = json.load(response)
        else:
            for raw in response:
                raw = raw.strip()
                if raw.startswith(b'data:'):
                    raw = raw[5:].strip()
                if not raw.startswith(b'{'):
                    continue
                message = json.loads(raw)
                if message.get('type') == 'error':
                    raise RuntimeError(message)
                if message.get('type') == 'done':
                    done = message
    if not done or not done.get('audio_path'):
        raise RuntimeError(f'Generation did not return audio_path: {done}')
    if done.get('dropped_chunks', 0):
        raise RuntimeError(f'Generation dropped chunks: {done}')
    return done


def decoded(path):
    raw = subprocess.check_output(['ffmpeg', '-hide_banner', '-loglevel', 'error', '-i', str(path),
                                   '-f', 'f32le', '-ac', '1', '-ar', '24000', '-'])
    samples = array.array('f')
    samples.frombytes(raw)
    return samples


def metrics(path):
    samples = decoded(path)
    if not samples:
        raise ValueError(f'Empty audio: {path.name}')
    peak = max(abs(sample) for sample in samples)
    rms = math.sqrt(sum(sample * sample for sample in samples) / len(samples))
    blocks = [samples[i:i+480] for i in range(0, len(samples), 480)]
    non_silent = [block for block in blocks
                  if math.sqrt(sum(x*x for x in block) / len(block)) > 10 ** (-50/20)]
    count = sum(len(block) for block in non_silent)
    active_rms = math.sqrt(sum(x*x for block in non_silent for x in block) / count) if count else 0
    db = lambda x: 20 * math.log10(x) if x > 0 else None
    return {
        'duration_seconds': len(samples) / 24000, 'peak_dbfs': db(peak),
        'rms_dbfs': db(rms), 'nonsilent_rms_dbfs': db(active_rms),
        'nonsilent_seconds': count / 24000,
        'samples_at_or_above_full_scale': sum(abs(x) >= 1 for x in samples),
    }


def encode(wav, mp3):
    raw = metrics(wav)
    if raw['nonsilent_seconds'] < 0.03 or raw['peak_dbfs'] is None:
        raise ValueError(f'No meaningful speech energy: {wav.name}')
    # One constant gain preserves timing, inflection and dynamic shape. Check the
    # decoded MP3 too, since codec reconstruction can overshoot the input peak.
    gain_db = min(-20 - raw['nonsilent_rms_dbfs'], -2.2 - raw['peak_dbfs'])
    for attempt in range(3):
        subprocess.run(['ffmpeg', '-hide_banner', '-loglevel', 'error', '-y', '-i', str(wav),
                        '-af', f'volume={gain_db:.8f}dB', '-ac', '1', '-ar', '24000',
                        '-c:a', 'libmp3lame', '-b:a', '64k', '-map_metadata', '-1', str(mp3)], check=True)
        result = metrics(mp3)
        if result['peak_dbfs'] <= -2:
            break
        gain_db += -2.1 - result['peak_dbfs']
    else:
        raise ValueError(f'MP3 peak cap failed: {mp3.name}')
    probe = json.loads(subprocess.check_output(['ffprobe', '-v', 'error', '-show_entries',
                         'stream=codec_name,sample_rate,channels,bit_rate', '-of', 'json', str(mp3)]))
    stream = probe['streams'][0]
    if (stream['codec_name'], stream['sample_rate'], stream['channels']) != ('mp3', '24000', 1):
        raise ValueError(f'Wrong MP3 format: {stream}')
    result.update({'constant_gain_db': gain_db, 'bytes': mp3.stat().st_size,
                   'sha256': hashlib.sha256(mp3.read_bytes()).hexdigest(), 'ffprobe': stream})
    return result




def tokens(text):
    return re.findall(r"[\w]+(?:'[\w]+)?", text.lower())


def edit_distance(left, right):
    previous = list(range(len(right) + 1))
    for i, a in enumerate(left, 1):
        current = [i]
        for j, b in enumerate(right, 1):
            current.append(min(current[-1] + 1, previous[j] + 1, previous[j-1] + (a != b)))
        previous = current
    return previous[-1]


def transcribe(endpoint, file):
    boundary = 'signal47-asr-' + uuid.uuid4().hex
    body = (f'--{boundary}\r\nContent-Disposition: form-data; name="audio"; filename="sample.audio"\r\n'
            'Content-Type: application/octet-stream\r\n\r\n').encode() + file.read_bytes() + b'\r\n'
    for key, value in {'language': 'en', 'mode': 'accurate', 'refine': 'false'}.items():
        body += f'--{boundary}\r\nContent-Disposition: form-data; name="{key}"\r\n\r\n{value}\r\n'.encode()
    body += f'--{boundary}--\r\n'.encode()
    request = urllib.request.Request(endpoint + '/transcribe', data=body,
                                     headers={'Content-Type': 'multipart/form-data; boundary=' + boundary})
    return json.load(urllib.request.urlopen(request, timeout=120))
