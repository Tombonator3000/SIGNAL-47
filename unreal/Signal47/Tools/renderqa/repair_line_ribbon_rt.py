"""Saved mesh-property repair/audit for exclusively exported line ribbons.

Default audit is read-only. S47_RIBBON_RT_MODE=apply sets support_ray_tracing=False
only on exact /Game/Web/Geo assets with no solid or geoB consumer in the manifest.
No model, material, source geometry, collision, LOD or raster-shadow change.
S47_RIBBON_RT_REPORT must name a new JSON output. Root coordinates guarded UE runs.
"""
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys
import unreal

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'webexport'))
from ribbon_geometry import ribbon_asset_paths

mode = os.environ.get('S47_RIBBON_RT_MODE', 'audit')
assert mode in ('audit', 'apply'), mode
out = Path(os.environ['S47_RIBBON_RT_REPORT'])
assert not out.exists(), 'Refusing evidence overwrite: ' + str(out)
out.parent.mkdir(parents=True, exist_ok=True)
content = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()))
manifest_file = content/'WebData/manifest.json'
manifest = json.loads(manifest_file.read_text())
paths, rejected = ribbon_asset_paths(manifest)
assert paths, 'No proven exclusively line-ribbon geometry; refusing vacuous PASS'
E = unreal.EditorAssetLibrary
report = {'status': 'RUNNING', 'mode': mode, 'manifest_sha256': hashlib.sha256(manifest_file.read_bytes()).hexdigest(),
          'scope': 'Only exporter wire=true LineSegments geometry with no solid/geoB consumer',
          'rejected_shared_or_unproven_geometry': rejected, 'meshes': [], 'errors': [],
          'limits': 'Saved flags only. Fresh cook and actual runtime RT-residency measurement required.'}
try:
    # Resolve all assets and protect all required shape data before any mutation.
    tasks = []
    for path, detail in paths.items():
        mesh = E.load_asset(path)
        assert isinstance(mesh, unreal.StaticMesh), path
        assert not mesh.get_editor_property('nanite_settings').enabled, 'Unexpected Nanite ribbon: ' + path
        tasks.append((path, detail, mesh))
    for path, detail, mesh in tasks:
        asset_file = content/path.split('.',1)[0].removeprefix('/Game/')
        asset_file = asset_file.with_suffix('.uasset')
        before = bool(mesh.get_editor_property('support_ray_tracing'))
        materials_before = [slot.material_interface.get_path_name() if slot.material_interface else None for slot in mesh.get_editor_property('static_materials')]
        triangles_before = mesh.get_num_triangles(0)
        row = dict(detail, asset=path, support_ray_tracing_before=before, triangles=triangles_before, materials=materials_before)
        report['meshes'].append(row)
        if mode == 'apply' and before:
            backup = out.parent/'line-ribbon-assets-before'/asset_file.relative_to(content)
            assert not backup.exists(), 'Refusing backup overwrite: ' + str(backup)
            backup.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(asset_file, backup)
            row['backup'] = str(backup)
            row['asset_before_sha256'] = hashlib.sha256(backup.read_bytes()).hexdigest()
            mesh.set_editor_property('support_ray_tracing', False)
            assert E.save_loaded_asset(mesh), 'Could not save ' + path
        row['support_ray_tracing_after'] = bool(mesh.get_editor_property('support_ray_tracing'))
        assert not row['support_ray_tracing_after'], 'RT still enabled: ' + path
        assert mesh.get_num_triangles(0) == triangles_before
        assert [slot.material_interface.get_path_name() if slot.material_interface else None for slot in mesh.get_editor_property('static_materials')] == materials_before
    report['status'] = 'PASS_SAVED_FLAGS_ONLY'
except BaseException as error:
    report['status'] = 'FAIL'
    report['errors'].append(repr(error))
    raise
finally:
    out.write_text(json.dumps(report, indent=2) + '\n')
