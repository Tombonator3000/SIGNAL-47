"""Build a daytime variant while leaving the known night recipe untouched."""
from pathlib import Path
import hashlib
import sys

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT.parent))
from sky_capture import CAPTURE_SHA256, HELPER


def day_code(original):
    assert hashlib.sha256(original.encode()).hexdigest() == CAPTURE_SHA256, 'Review changed night recipe before creating day variant'
    endings = [
        'return max(lerp(captureC,cirC,cirMean),float3(0,0,0));',
        'return lerp(col, cirC, cirA);',
        'return lerp(lerp(col,calibrated,saturate(ConceptBlend)),cirC,cirA);',
    ]
    code = original
    for old in endings:
        assert code.count(old) == 1, old
        expression = old[len('return '):-1]
        code = code.replace(old, 'return lerp(' + expression + ', pureRadiance, pureW);', 1)
    assert code.startswith(HELPER)
    return HELPER + (ROOT / 'day_sky.hlsl').read_text() + code[len(HELPER):]
