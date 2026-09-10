#!/usr/bin/env bash
set -euo pipefail
export FIELD_CAMERA_REVIEW=1 REVIEW_PHASE=candidate
bash "$(dirname "$0")/run-pass06.sh"
python3 "$(dirname "$0")/field-camera-evidence.py"
