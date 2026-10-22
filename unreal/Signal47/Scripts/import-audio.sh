#!/bin/bash
# Builds the game's sounds for Unreal from the web version's files and imports them:
#   Content/Audio/Raw     the recordings core/Audio.ts decodes (the free sounds, Kinski's 24 telephone lines,
#                         the character lines on the telephone) as WAVE files with index.json, and voices.json
#                         (every voice bank's catalog), for the game's own Web Audio (Scripts/audio-raw.py)
#   Content/Licensed/Raw  the same for the bought sounds, if the private clone is there
#   /Game/Audio/Sfx       the free sounds in web/src/assets/audio (CC0 and CC BY, see web/THIRD_PARTY_NOTICES.md)
#   /Game/Audio/Music     the music in web/public/music (CC BY, credited in the game)
#   /Game/Audio/Radio     the radio programmes in web/public/radio
#   /Game/Audio/Voice/<Bank>  the voice banks (kinski, serling, serling-intro, artbell, artbell-town, radiocast,
#                         ensemble) as v_<file>, looked up through Content/Audio/Raw/voices.json
#                         (S47_AUDIO_VOICES=0 leaves them out: some 3200 files)
#   /Game/Audio/Synth     tones the prologue still plays by file name (beep_1200 ...), made here as WAV
#   /Game/Licensed/Sfx    the bought sounds from the private clone web/src/assets/licensed/sfx, if it is there
# Content/Audio and Content/Licensed are ignored by Git; run this after a fresh clone. The raw step needs
# only python3 and ffmpeg; the asset import needs the editor target built and no editor of this project open.
# S47_PROJECT=<dir of Signal47.uproject> imports into another checkout (its web/ is the source).
set -euo pipefail
UE_ROOT=${UE_ROOT:-$HOME/.local/opt/UnrealEngine-5.8.4}
SCRIPTS="$(cd "$(dirname "$0")" && pwd)"
D="$(cd "${S47_PROJECT:-$SCRIPTS/..}" && pwd)"
ROOT="$(cd "$D/../.." && pwd)"
STAGE=${S47_AUDIO_STAGE:-$HOME/scratch-bm/Signal47-audio}
python3 "$SCRIPTS/audio-raw.py" --web "$ROOT/web/src" --content "$D/Content"
rm -rf "${STAGE:?}/wav"
mkdir -p "$STAGE/wav"

conv() { # group srcdir [rate [prefix]]: rate '' keeps each file's own
  local group=$1 dir=$2 rate=${3-44100} prefix=${4-}
  [ -d "$dir" ] || { echo "skip $group: no $dir"; return; }
  mkdir -p "$STAGE/wav/$group"
  for f in "$dir"/*.mp3 "$dir"/*.wav; do
    [ -f "$f" ] || continue
    local name; name=$(basename "${f%.*}")
    if [ -n "$rate" ]; then
      ffmpeg -loglevel error -y -i "$f" -ar "$rate" -c:a pcm_s16le "$STAGE/wav/$group/$prefix$name.wav"
    else
      ffmpeg -loglevel error -y -i "$f" -c:a pcm_s16le "$STAGE/wav/$group/$prefix$name.wav"
    fi
  done
  echo "$group: $(ls "$STAGE/wav/$group" | wc -l) files"
}
conv Sfx "$ROOT/web/src/assets/audio"
conv Music "$ROOT/web/public/music"
conv Radio "$ROOT/web/public/radio"
conv Licensed "$ROOT/web/src/assets/licensed/sfx"
if [ "${S47_AUDIO_VOICES:-1}" != 0 ]; then
  for bank in kinski:Kinski serling:Serling serling-intro:SerlingIntro artbell:ArtBell artbell-town:ArtBellTown radiocast:Radiocast ensemble:Ensemble; do
    conv "Voice/${bank#*:}" "$ROOT/web/src/assets/audio/${bank%%:*}" '' v_
  done
fi
python3 "$SCRIPTS/make-synth.py" "$STAGE/wav/Synth"
echo "Synth: $(ls "$STAGE/wav/Synth" | wc -l) files"
S47_AUDIO_STAGE="$STAGE/wav" "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$D/Signal47.uproject" -run=pythonscript -script="$SCRIPTS/import-audio.py" -unattended -nullrhi -nosplash 2>&1 | grep -E "S47:|Error:|Traceback" | grep -v "LogPython: Display: Python" | grep -v "Toolset\|toolset\|ToolsetDefinition" | tail -40
