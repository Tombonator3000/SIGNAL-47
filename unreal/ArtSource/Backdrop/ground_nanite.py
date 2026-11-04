"""Enable render-only Nanite relief on the exported desert ground meshes.

Keep the complete original fallback mesh: visibility/complex collision and
non-Nanite rendering retain the authored surface. No collision or source
geometry is displaced. Called by the ground and full web importers.
"""
import json
import math
from pathlib import Path

GROUND_MATERIALS = {5, 416, 574, 774}


def ground_geometry_paths(manifest):
    nodes = manifest['nodes']
    selected = [n for n in nodes if len(n.get('mats', [])) == 1
                and n['mats'][0] in GROUND_MATERIALS and n.get('geo', -1) >= 0]
    geos = {n['geo'] for n in selected}
    if not geos:
        raise RuntimeError('No exported desert ground geometry found')
    for n in nodes:
        if n.get('geo', -1) in geos and n not in selected:
            raise RuntimeError('Desert geometry shared with a non-ground node')
    # Nanite displacement magnitude is in local units. Reject a changed export
    # that would silently turn centimetres of relief into larger world offsets.
    for n in selected:
        current = n
        while current:
            m = current.get('m')
            if m and any(not math.isclose(sum(m[c * 4 + r] ** 2 for r in range(3)), 1., abs_tol=1e-5)
                         for c in range(3)):
                raise RuntimeError('Non-unit ground transform: node %s' % current['i'])
            parent = current.get('p', -1)
            current = nodes[parent] if parent >= 0 else None
    result = []
    for geo in sorted(geos):
        files = [f for f in manifest['geoFiles'] if f['from'] <= geo < f['to']]
        if len(files) != 1:
            raise RuntimeError('Ambiguous ground geometry file: %s' % geo)
        folder = Path(files[0]['file']).stem
        result.append('/Game/Web/Geo/%s/StaticMeshes/g%d.g%d' % (folder, geo, geo))
    return result


def configure_ground_nanite(manifest=None):
    import unreal
    if manifest is None:
        manifest = json.loads((Path(unreal.Paths.project_content_dir()) / 'WebData/manifest.json').read_text())
    paths = ground_geometry_paths(manifest)
    assets = [(path, unreal.load_asset(path)) for path in paths]
    if any(not isinstance(mesh, unreal.StaticMesh) for _, mesh in assets):
        raise RuntimeError('Ground Nanite requires all exported ground static meshes')
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    result = []
    for path, mesh in assets:
        settings = subsystem.get_nanite_settings(mesh) if subsystem else mesh.get_editor_property('nanite_settings')
        settings.set_editor_property('enabled', True)
        settings.set_editor_property('generate_fallback', unreal.NaniteGenerateFallback.ENABLED)
        settings.set_editor_property('fallback_target', unreal.NaniteFallbackTarget.PERCENT_TRIANGLES)
        settings.set_editor_property('fallback_percent_triangles', 1.)
        settings.set_editor_property('fallback_relative_error', 0.)
        if subsystem:
            subsystem.set_nanite_settings(mesh, settings, True)
        else:
            # Commandlets do not instantiate editor subsystems. Reflected assignment
            # issues PostEditChangeProperty and builds the asset without editor UI.
            mesh.set_editor_property('nanite_settings', settings)
        if not unreal.EditorAssetLibrary.save_loaded_asset(mesh):
            raise RuntimeError('Could not save Nanite ground: ' + path)
        actual = subsystem.get_nanite_settings(mesh) if subsystem else mesh.get_editor_property('nanite_settings')
        if not actual.enabled or actual.fallback_percent_triangles != 1.:
            raise RuntimeError('Ground Nanite settings did not persist: ' + path)
        result.append({'mesh': path, 'nanite': True, 'fallback_triangles': 1.,
                       'collision': 'original undisplaced mesh'})
    return result
