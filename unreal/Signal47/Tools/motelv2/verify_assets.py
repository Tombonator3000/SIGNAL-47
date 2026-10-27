"""Fresh-editor read-only validation of saved MotelV2 native assets.

Same S47_MOTEL_SOURCE/TRIMS/REPORT inputs as import_assets.py. No import, saves,
graph mutation or asset deletion. Every LOD uses its saved MeshDescription.
"""
import os
import sys
import traceback
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import native as N


if __name__ == '__main__':
    report = N.Report(os.environ['S47_MOTEL_REPORT'], 'saved-native-read-only')
    try:
        N.verify_all(Path(os.environ['S47_MOTEL_SOURCE']).resolve(),
                     Path(os.environ['S47_MOTEL_TRIMS']).resolve() if os.environ.get('S47_MOTEL_TRIMS') else None, report)
        report.data['status'] = 'PASS'
        report.flush()
        N.unreal.log('MOTEL_V2_SAVED_NATIVE_PASS')
    except Exception:
        report.data.update(status='FAIL', error=traceback.format_exc())
        report.flush()
        raise
