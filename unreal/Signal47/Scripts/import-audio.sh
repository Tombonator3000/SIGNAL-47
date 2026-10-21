#!/bin/bash
# Builds the game's sounds for Unreal from the web version's files and imports them:
#   /Game/Audio/Sfx       the free sounds in web/src/assets/audio (CC0 and CC BY, see web/THIRD_PARTY_NOTICES.md)
#   /Game/Audio/Music     the music in web/public/music (CC BY, credited in the game)
#   /Game/Audio/Radio     the radio programmes in web/public/radio
#   /Game/Audio/Synth     tones the web version makes with oscillators (beeps, carrier, motors), made here as WAV
#   /Game/Licensed/Sfx    the bought sounds from the private clone web/src/assets/licensed/sfx, if it is there
# Content/Audio and Content/Licensed are ignored by Git; run this after a fresh clone.
# MP3 is converted to WAV with ffmpeg first (Unreal imports WAV). Needs: ffmpeg, python3, the editor target built.
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
D="$(cd "$(dirname "$0")/.." && pwd)"
ROOT="$(cd "$D/../.." && pwd)"
STAGE=${S47_AUDIO_STAGE:-$HOME/scratch-bm/Signal47-audio}
rm -rf "${STAGE:?}/wav"
mkdir -p "$STAGE/wav"

conv() { # group srcdir
  local group=$1 dir=$2
  [ -d "$dir" ] || { echo "skip $group: no $dir"; return; }
  mkdir -p "$STAGE/wav/$group"
  for f in "$dir"/*.mp3 "$dir"/*.wav; do
    [ -f "$f" ] || continue
    local name; name=$(basename "${f%.*}")
    ffmpeg -loglevel error -y -i "$f" -ar 44100 -c:a pcm_s16le "$STAGE/wav/$group/$name.wav"
  done
  echo "$group: $(ls "$STAGE/wav/$group" | wc -l) files"
}
conv Sfx "$ROOT/web/src/assets/audio"
conv Music "$ROOT/web/public/music"
conv Radio "$ROOT/web/public/radio"
conv Licensed "$ROOT/web/src/assets/licensed/sfx"
python3 "$D/Scripts/make-synth.py" "$STAGE/wav/Synth"
echo "Synth: $(ls "$STAGE/wav/Synth" | wc -l) files"
S47_AUDIO_STAGE="$STAGE/wav" "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$D/Signal47.uproject" -run=pythonscript -script="$D/Scripts/import-audio.py" -unattended -nullrhi -nosplash 2>&1 | grep -E "S47:|Error:|Traceback" | grep -v "LogPython: Display: Python" | tail -20
