#!/bin/bash
# The TV sign-off's anthem (Tom 12 Nov: a real recording, not the synthesised tune): "The Star-Spangled Banner" performed by the
# United States Navy Band, a work of the U.S. federal government, public domain, from Wikimedia Commons:
# https://commons.wikimedia.org/wiki/File:%22The_Star-Spangled_Banner%22_performed_by_the_United_States_Navy_Band.mp3
# Made mono 22.05 kHz, band-limited like a station's worn tape, levelled; written to Content/Audio/Raw/tv/anthem.wav
# (Content/Audio is not in Git; this script makes it again). FS47TvProgramme::Anthem() reads it and falls back to the tune.
set -euo pipefail
cd "$(dirname "$0")/../.."
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT
curl -sSfL -A "SIGNAL47-tools/1.0" -o "$W/navy.mp3" "https://upload.wikimedia.org/wikipedia/commons/2/25/%22The_Star-Spangled_Banner%22_performed_by_the_United_States_Navy_Band.mp3"
mkdir -p Content/Audio/Raw/tv
ffmpeg -v error -y -i "$W/navy.mp3" -ac 1 -ar 22050 -af "highpass=f=140,lowpass=f=6500,loudnorm=I=-18:TP=-2" -c:a pcm_s16le Content/Audio/Raw/tv/anthem.wav
echo "anthem: $(ffprobe -v error -show_entries format=duration -of csv=p=0 Content/Audio/Raw/tv/anthem.wav) s"
