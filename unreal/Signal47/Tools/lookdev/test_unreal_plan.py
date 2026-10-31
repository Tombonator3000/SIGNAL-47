"""Small CPU-only contract checks; never imports Unreal or starts an editor."""
import copy
import tempfile
import unittest
from pathlib import Path

from unreal_setup import apply_plan, blender_to_unreal_meters, generate_plan


class UnrealPlanTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root / "base.png").write_bytes(b"texture placeholder for path validation")
        self.manifest = {"schema_version": 1, "name": "Test Room",
            "materials": [{"object": "Cup", "slot": 0, "kind": "surface",
                           "material_name": "Paint", "textures": {"base_color": "base.png"}}],
            "lighting": {"day": {"sun": {"azimuth_deg": 26, "elevation_deg": 30,
                                            "energy_w_m2": 7},
                                  "unreal": {"sun_lux": 50000, "sky_intensity": 0.8,
                                             "exposure_bias": 0}}}}

    def plan(self, manifest=None, profile="day"):
        return generate_plan(manifest or self.manifest, profile, self.root)

    def test_explicit_lux_is_not_blender_energy(self):
        result = self.plan()
        self.assertEqual(result["lighting"]["directionals"][0]["lux"], 50000)
        self.assertEqual(result["preview_requirements"], [])
        self.assertTrue(result["destination"].startswith("/Game/Signal47/Lookdev/"))

    def test_coordinate_units_and_handedness(self):
        self.assertEqual(blender_to_unreal_meters([1, 2, 3]), [200, 100, 300])
        self.assertEqual(blender_to_unreal_meters([0, 1, 0]), [100, 0, 0])

    def test_unreal_overrides_are_required_not_inferred(self):
        del self.manifest["lighting"]["day"]["unreal"]["sun_lux"]
        result = self.plan()
        self.assertIsNone(result["lighting"]["directionals"][0]["lux"])
        self.assertIn("lighting.day.unreal.sun_lux", result["preview_requirements"])

    def test_material_identity_prevents_shared_name_collision(self):
        self.manifest["materials"].append(dict(self.manifest["materials"][0], object="Desk"))
        materials = self.plan()["materials"]
        self.assertNotEqual(materials[0]["asset_path"], materials[1]["asset_path"])
        self.manifest["materials"].append(dict(self.manifest["materials"][0]))
        with self.assertRaisesRegex(ValueError, "Duplicate"):
            self.plan()

    def test_invalid_numbers_and_missing_files_fail(self):
        for key, value in (("roughness", float("nan")), ("metallic", True), ("slot", -1)):
            manifest = copy.deepcopy(self.manifest)
            manifest["materials"][0][key] = value
            with self.assertRaises(ValueError):
                self.plan(manifest)
        self.manifest["materials"][0]["textures"]["base_color"] = "missing.png"
        with self.assertRaisesRegex(ValueError, "Missing baked texture"):
            self.plan()

    def test_unknown_shader_channels_and_normals_fail(self):
        self.manifest["materials"][0]["textures"]["opacity"] = "base.png"
        with self.assertRaisesRegex(ValueError, "texture role"):
            self.plan()
        self.manifest["materials"][0]["textures"].pop("opacity")
        self.manifest["materials"][0]["normal_convention"] = "guess"
        with self.assertRaisesRegex(ValueError, "normal_convention"):
            self.plan()

    def test_unbaked_procedural_surface_blocks_apply_without_importing_unreal(self):
        self.manifest["materials"][0]["surface_values_are_fallbacks"] = True
        result = self.plan()
        self.assertTrue(any("bake the procedural" in row for row in result["apply_requirements"]))
        with self.assertRaisesRegex(RuntimeError, "Unbaked material data"):
            apply_plan(result)
        for role in ("roughness", "metallic", "normal"):
            self.manifest["materials"][0]["textures"][role] = "base.png"
        self.assertEqual(self.plan()["apply_requirements"], [])

    def test_glass_records_native_limitation(self):
        self.manifest["materials"][0].update(kind="glass", textures={}, glass={
            "mode": "solid", "ior": 1.52, "tint": [0.9, 1, 0.95],
            "roughness": 0.08, "thickness": 0.006, "absorption": 0.2})
        result = self.plan()
        self.assertEqual(result["materials"][0]["glass"]["mode"], "solid")
        self.assertTrue(any("approximation" in warning for warning in result["warnings"]))
        self.manifest["materials"][0]["glass"]["ior"] = -1
        with self.assertRaises(ValueError):
            self.plan()

    def test_preview_profile_and_practicals_validate(self):
        with self.assertRaises(ValueError):
            self.plan(profile="sunset")
        self.manifest["lighting"]["day"]["practicals"] = [
            {"name": "Lamp", "type": "SPOT", "location_m": [1, 2, 3],
             "target_m": [0, 0, 0]}]
        self.manifest["lighting"]["day"]["unreal"]["practical_lumens"] = {"Lamp": 700}
        result = self.plan()
        self.assertEqual(result["lighting"]["practicals"][0]["lumens"], 700)
        self.assertEqual(result["lighting"]["practicals"][0]["location_cm"], [200, 100, 300])


if __name__ == "__main__":
    unittest.main()
