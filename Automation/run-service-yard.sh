#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
SERVICE_YARD_REVIEW=1 REVIEW_PHASE=candidate bash Automation/run-pass06.sh
python3 Automation/service-yard-evidence.py
