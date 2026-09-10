#!/usr/bin/env python3
"""Package evidence without retouching screenshots or relaxing performance thresholds."""
import argparse
import base64
import hashlib
import json
import math
from pathlib import Path


def performance_checks(report):
    def number(key):
        value = report.get(key)
        return isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value)
    checks = {
        'duration_120s': number('seconds') and report['seconds'] >= 120,
        'average_59fps': number('averageFps') and report['averageFps'] >= 59,
        'p95_17_2ms': number('p95ms') and report['p95ms'] <= 17.2,
        'p99_below_20ms': number('p99ms') and report['p99ms'] < 20,
        'no_stalls_over_50ms': number('stallsOver50ms') and report['stallsOver50ms'] == 0,
        'focused': report.get('focusedThroughout') is True,
        'release': report.get('development') is False,
        'preset': report.get('preset') == 'Ultra' and report.get('resolution') == '1280x800',
        'hardware_renderer': bool(report.get('gpu')) and not any(x in report.get('gpu', '').lower() for x in ('llvmpipe','softpipe','software')),
    }
    return checks


def package(root):
    from PIL import Image
    report = json.loads((root/'Performance/performance.json').read_text())
    capture = json.loads((root/'Snapshots/capture-manifest.json').read_text())
    journey = json.loads((root/'Performance/journey-result.json').read_text())
    views = capture['views']
    if len(views) != 6:
        raise ValueError('Six original capture views are required')
    records = []
    for view in views:
        path = root/'Snapshots'/view['file']
        if path.parent != root/'Snapshots' or not path.is_file():
            raise ValueError('Invalid/missing capture path')
        original = path.read_bytes()
        with Image.open(path) as im:
            if im.size != (1280,800):
                raise ValueError(f'Unexpected screenshot dimensions: {path} {im.size}')
            mobile = im.convert('RGB').resize((960,600), Image.Resampling.LANCZOS)
            mobile.save(path.with_name('mobile-'+path.stem+'.jpg'), quality=82)
            small = im.convert('RGB').resize((384,240), Image.Resampling.LANCZOS)
            preview = path.with_suffix('.review.webp')
            small.save(preview, 'WEBP', quality=42)
            # Fallback inspection transport when the private connector cannot mount binaries.
            preview.with_suffix('.b64.txt').write_text(base64.b64encode(preview.read_bytes()).decode()+'\n')
            records.append({'file':view['file'],'png_sha256':hashlib.sha256(original).hexdigest(),'review_sha256':hashlib.sha256(preview.read_bytes()).hexdigest(),'method':'Original PNG unchanged; mobile JPEG and small WebP are resized copies only'})
        if path.read_bytes() != original:
            raise RuntimeError('Original capture unexpectedly changed')
    checks = performance_checks(report)
    same_build = report.get('buildId') == capture.get('buildId') and report.get('buildId','unknown') != 'unknown'
    same_preset = report['preset'] == capture['quality'] and report['resolution'] == capture['resolution'] and capture.get('development') is False
    result = {'performance':'PASS' if all(checks.values()) else 'FAIL', 'checks':checks,
              'native_journey':journey.get('outcome','UNVERIFIED'), 'same_release_build_and_preset':same_build and same_preset,
              'gpu_timing':'MEASURED' if report.get('gpuTimingAvailable') else 'UNVERIFIED',
              'subjective_audio':'UNVERIFIED', 'visual_review':'UNVERIFIED until an agent or owner inspects the original/derived images',
              'source_build_id':report.get('buildId')}
    audio_path = root/'audio-audit.json'
    if audio_path.exists():
        audit=json.loads(audio_path.read_text())
        levels={level['state']:level for level in audit.get('levels',[])}
        required=('room','receiver','printer','ring','call','impact','title')
        audio_checks={state:state in levels and levels[state]['peak']>0.000001 and levels[state]['overFullScale']==0 for state in required}
        result['sampled_audio_checks']=audio_checks
        result['sampled_audio']='PASS' if audit.get('dspAdvanced') and all(audio_checks.values()) else 'FAIL'
    else:
        result['sampled_audio']='UNVERIFIED'
    (root/'quality-gates.json').write_text(json.dumps(result,indent=2)+'\n')
    (root/'Snapshots/file-hashes.json').write_text(json.dumps(records,indent=2)+'\n')
    print(json.dumps(result,indent=2))
    return result


if __name__ == '__main__':
    parser=argparse.ArgumentParser();parser.add_argument('root',type=Path);args=parser.parse_args()
    package(args.root)
