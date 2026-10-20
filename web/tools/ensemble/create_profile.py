#!/usr/bin/env python3
"""Create an explicitly sourced clone; never invent profile IDs or consent."""
from pathlib import Path
import argparse
import hashlib
import json
import urllib.request
import uuid
from audio_common import atomic_json, metrics

ROOT = Path(__file__).resolve().parents[2]
NAMES = {'stanton':'Harry Dean Stanton', 'bacall':'Lauren Bacall', 'stack':'Robert Stack',
         'lynch':'David Lynch', 'price':'Vincent Price', 'serling':'Rod Serling'}

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--voice', required=True, choices=NAMES)
    p.add_argument('--ref', required=True, type=Path)
    p.add_argument('--asr', required=True, type=Path)
    p.add_argument('--source', required=True, type=Path)
    p.add_argument('--endpoint', default='http://127.0.0.1:3900')
    args = p.parse_args()
    ref = args.ref.read_bytes(); ref_hash = hashlib.sha256(ref).hexdigest()
    asr = json.loads(args.asr.read_text()); text = asr.get('text', '').strip()
    if not text: raise ValueError('Reference ASR has no words')
    if len(text.split()) > 25: raise ValueError('Select a complete reference phrase of at most 25 words')
    source = json.loads(args.source.read_text())
    if isinstance(source, list): source = next(x for x in source if x.get('voice') == args.voice)
    source = {key: source[key] for key in ['voice', 'source_page', 'source_url', 'primary_context', 'original_file',
        'artifact', 'rights', 'original_bytes', 'original_sha256', 'publisher', 'date',
        'selected_reference_file', 'selected_interval_seconds', 'selection_note', 'speaker_attribution'] if key in source}
    config_path = ROOT / 'production/ensemble/profiles.json'
    config = json.loads(config_path.read_text()) if config_path.exists() else {'version':1,'roles':{}}
    if args.voice in config['roles']: raise ValueError('Validated profile already exists for ' + args.voice)
    fields = {'name': NAMES[args.voice] + ' / synthetic parody / SIGNAL 47', 'kind':'clone',
              'ref_text':text, 'language':'en', 'seed':42,
              'personality':'Synthetic parody performance from an explicitly attributed public reference. No endorsement or verified-own-voice assertion.'}
    boundary = 'ensemble-' + uuid.uuid4().hex; body = b''
    for key,value in fields.items():
        body += f'--{boundary}\r\nContent-Disposition: form-data; name="{key}"\r\n\r\n{value}\r\n'.encode()
    body += f'--{boundary}\r\nContent-Disposition: form-data; name="ref_audio"; filename="reference.wav"\r\nContent-Type: audio/wav\r\n\r\n'.encode() + ref + b'\r\n'
    body += f'--{boundary}--\r\n'.encode()
    profile = json.load(urllib.request.urlopen(urllib.request.Request(args.endpoint+'/profiles', data=body,
        headers={'Content-Type':'multipart/form-data; boundary='+boundary}), timeout=120))
    saved = urllib.request.urlopen(args.endpoint+'/profiles/'+profile['id']+'/audio').read()
    if hashlib.sha256(saved).hexdigest() != ref_hash: raise ValueError('Saved reference differs')
    config['roles'][args.voice] = {'actor':NAMES[args.voice], 'profile_id':profile['id'],
        'profile_name':profile['name'], 'kind':'clone', 'profile_reference_sha256':ref_hash,
        'conditioning_text':text, 'reference_pcm':metrics(args.ref), 'source':source,
        'reference_transcription':'Existing faster-whisper-small; not human-listened.',
        'human_listened':False, 'speaker_similarity':'UNVERIFIED'}
    atomic_json(config_path, config)
    print(json.dumps({'voice':args.voice,'actual_profile_id':profile['id'],'reference_sha256':ref_hash,
                      'text':text}),flush=True)

if __name__ == '__main__': main()
