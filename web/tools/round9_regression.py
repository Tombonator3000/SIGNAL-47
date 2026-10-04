#!/usr/bin/env python3
"""Målrettede regresjoner for P2: dekodet JPEG, profilbånd og repetisjon."""
import tempfile
import unittest
from pathlib import Path
from PIL import Image, ImageFilter
from round9_check import check_pair, roughness_byte_bounds
from round9_maps import SIZE, PROFILES, encode_roughness, roughness_limits


class RoughnessRegression(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.root=Path(self.temp.name)
        self.normal=self.root/'flat_n.png'
        Image.new('RGB',(SIZE,SIZE),(128,128,255)).save(self.normal)
        self.roughness=self.root/'rough_r.jpg'

    def tearDown(self):
        self.temp.cleanup()

    def test_rejects_encoded_profile_overshoot(self):
        # The old report marked 236/255 PASS for a .90 maximum.
        Image.new('L',(SIZE,SIZE),236).save(self.roughness,quality=90,optimize=True)
        result=check_pair(self.normal,self.roughness,(.45,.90))
        self.assertEqual(result['status'],'FAIL')
        self.assertEqual(result['roughness_profile_status'],'FAIL')

    def test_rejects_encoded_border_mismatch(self):
        image=Image.new('L',(SIZE,SIZE),128)
        image.paste(180,(0,0,8,SIZE))
        image.save(self.roughness,quality=90,optimize=True)
        result=check_pair(self.normal,self.roughness,(.45,.90))
        self.assertEqual(result['status'],'FAIL')
        border=next(c for c in result['checks'] if c['check']=='roughness decoded opposite border pixels identical')
        self.assertGreater(border['detail']['mismatched_pixels'],0)

    def test_encoder_closes_high_contrast_periodic_map(self):
        # Thin, high-contrast joints stress JPEG ringing and block boundaries.
        guide=Image.new('L',(SIZE,SIZE))
        guide.putdata([230 if x%47<2 or y%51<2 else 128 for y in range(SIZE) for x in range(SIZE)])
        guide=guide.filter(ImageFilter.GaussianBlur(1))
        field=[v/255 for v in guide.get_flattened_data()]
        payload,report=encode_roughness(field,PROFILES['hextile'])
        self.roughness.write_bytes(payload)
        result=check_pair(self.normal,self.roughness,roughness_limits(PROFILES['hextile']))
        self.assertEqual(result['status'],'PASS')
        self.assertEqual(result['roughness_profile_status'],'PASS')
        self.assertEqual(report['decoded_edges']['max_abs_byte_delta'],0)

    def test_encoder_fails_closed_on_unrepresentable_sharp_field(self):
        field=[.90 if x%47<2 or y%51<2 else .45 for y in range(SIZE) for x in range(SIZE)]
        with self.assertRaisesRegex(ValueError,'did not converge'):
            encode_roughness(field,PROFILES['hextile'])

    def test_half_byte_constant_representation(self):
        self.assertEqual(roughness_byte_bounds((.90,.90)),[229,230])
        for value in (229,230):
            Image.new('L',(SIZE,SIZE),value).save(self.roughness,quality=90,optimize=True)
            self.assertEqual(check_pair(self.normal,self.roughness,(.90,.90))['status'],'PASS')


if __name__=='__main__':
    unittest.main(verbosity=2)
