#!/usr/bin/env python3
"""Compile the production DSP kernel, run behavioral checks and render a listening file."""
import argparse
from pathlib import Path
import subprocess
import tempfile
import os
import shutil

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, help='Optional 72-second PCM16 stereo WAV')
args = parser.parse_args()
here = Path(__file__).resolve().parent
if args.output:
    args.output.parent.mkdir(parents=True, exist_ok=True)
compiler = os.environ.get('CXX') or shutil.which('c++') or shutil.which('clang++')
flags = []
if not compiler:
    sdk = Path.home() / '.local/opt/UnrealEngine-5.8.4/Engine/Extras/ThirdPartyNotUE/SDKs/HostLinux/Linux_x64'
    candidates = sorted(sdk.glob('*/x86_64-unknown-linux-gnu/bin/clang++'))
    if not candidates:
        parser.error('No C++ compiler found; set CXX or install a C++17 compiler')
    compiler = str(candidates[-1])
    flags = ['--sysroot=' + str(candidates[-1].parent.parent)]
with tempfile.TemporaryDirectory(prefix='s47-cosmic-') as tmp:
    binary = Path(tmp) / 'preview'
    subprocess.run([compiler, *flags, '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-I', str(here.parents[1] / 'Source/Signal47/Public'),
                    str(here / 'preview.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)] + ([str(args.output.resolve())] if args.output else []), check=True)
