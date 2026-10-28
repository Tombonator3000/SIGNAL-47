#!/usr/bin/env python3
"""Run the shared production coordinator from any working directory."""
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from Automation.production.cli import entrypoint

if __name__ == "__main__":
    entrypoint()
