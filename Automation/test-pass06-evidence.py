#!/usr/bin/env python3
"""Tests of evidence validation, NOT substitutes for a Unity playthrough."""
import contextlib
import importlib.util
import io
import json
import math
from pathlib import Path
import re
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('evidence',ROOT/'Automation/pass06-evidence.py')
evidence=importlib.util.module_from_spec(spec);spec.loader.exec_module(evidence)


def good():
    return dict(seconds=121,averageFps=59.98,p95ms=16.9,p99ms=17.5,maxMs=24,stallsOver50ms=0,focusedThroughout=True,development=False,preset='Ultra',resolution='1280x800',gpu='Mesa Intel(R) Graphics (ARL)',buildId='test-only-stamp')


class EvidenceTests(unittest.TestCase):
    def test_valid(self):
        self.assertTrue(all(evidence.performance_checks(good()).values()))
    def test_reported_world_pass_does_not_pass_on_average_alone(self):
        r=good();r.update(averageFps=59.985235176,p95ms=19.466661,p99ms=20.001248,maxMs=50.107452,stallsOver50ms=1)
        checks=evidence.performance_checks(r)
        self.assertTrue(checks['average_59fps']);self.assertFalse(checks['p95_17_2ms']);self.assertFalse(checks['p99_below_20ms']);self.assertFalse(checks['no_stalls_over_50ms'])
    def test_reported_actions_baseline_also_fails(self):
        r=good();r.update(p95ms=19.5076,p99ms=20.0622)
        self.assertFalse(all(evidence.performance_checks(r).values()))
    def test_threshold_boundaries(self):
        r=good();r['p95ms']=17.2;self.assertTrue(evidence.performance_checks(r)['p95_17_2ms'])
        r['p99ms']=20;self.assertFalse(evidence.performance_checks(r)['p99_below_20ms'])
        r['maxMs']=50.01;self.assertFalse(evidence.performance_checks(r)['no_stalls_over_50ms'])
    def test_invalid_numeric_data(self):
        for value in [math.nan, math.inf,-1,0,None,'16.9',True]:
            r=good();r['p95ms']=value;self.assertFalse(evidence.performance_checks(r)['p95_17_2ms'])
    def test_software_renderer_not_hardware_proof(self):
        for value in ['llvmpipe (LLVM 20)','softpipe','software rasterizer','',None,[]]:
            r=good();r['gpu']=value;self.assertFalse(evidence.performance_checks(r)['hardware_renderer'])
    def test_missing_data_not_pass(self):
        self.assertFalse(all(evidence.performance_checks({}).values()))
    def test_originals_unchanged_and_mismatched_stamp_rejected(self):
        from PIL import Image
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);(root/'Snapshots').mkdir();(root/'Performance').mkdir()
            # Synthetic unit-test fixtures. Never presented as screenshots.
            originals={}
            for n in range(6):
                p=root/'Snapshots'/f'{n}.png';Image.new('RGB',(1280,800)).save(p);originals[p]=p.read_bytes()
            (root/'Performance/performance.json').write_text(json.dumps(good()))
            (root/'Performance/journey-result.json').write_text(json.dumps({'outcome':'PASS'}))
            (root/'Snapshots/capture-manifest.json').write_text(json.dumps({'views':[{'file':p.name} for p in originals],'buildId':'different-test-stamp','quality':'Ultra','resolution':'1280x800','development':False}))
            with contextlib.redirect_stdout(io.StringIO()):r=evidence.package(root)
            self.assertFalse(r['same_release_build_and_preset'])
            for p,data in originals.items():self.assertEqual(p.read_bytes(),data)
    def test_all_bitmap_glyphs_have_35_binary_cells(self):
        text=(ROOT/'Unity/Assets/Signal47/Runtime/Signals/AuxiliaryCRTDisplay.cs').read_text()
        glyphs=re.findall(r"\['(.)'\]=\"([01]+)\"",text)
        self.assertEqual(len(glyphs),40);self.assertTrue(all(len(bits)==35 for _,bits in glyphs))
    def test_new_cosmetics_do_not_add_colliders(self):
        text=(ROOT/'Unity/Assets/Signal47/Editor/ControlRoomPass06.cs').read_text()
        self.assertIn('Cube(name,Vector3.zero,size,mat,false)',text)
        self.assertNotIn('AddComponent<BoxCollider>',text)
        self.assertNotIn('AddComponent<MeshCollider>',text)


if __name__=='__main__':unittest.main(verbosity=2)
