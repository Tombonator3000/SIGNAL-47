"""Identify only exporter-generated LineSegments ribbons, never solid/shared geometry.

export.js makes a fresh crossed-quad BufferGeometry for every Three line node,
sets wire=true, disables raster shadows/collision at runtime, and keeps the source
node type. Material type alone is insufficient: road lines use MeshStandardMaterial.
The resulting 1.2 cm ribbons are a raster line representation, not cable volumes.
"""
from pathlib import PurePosixPath


def line_ribbon_geometry(manifest):
    consumers = {}
    for node in manifest['nodes']:
        for field in ('geo', 'geoB'):
            geo = node.get(field, -1)
            if isinstance(geo, int) and geo >= 0:
                consumers.setdefault(geo, []).append((node, field))
    targets, rejected = {}, {}
    for geo, uses in sorted(consumers.items()):
        if not any(node.get('wire') for node, field in uses):
            continue
        reasons = []
        for node, field in uses:
            if field != 'geo' or node.get('wire') is not True or node.get('type') != 'LineSegments':
                reasons.append({'node': node['i'], 'field': field, 'type': node.get('type'), 'wire': node.get('wire', False)})
        if reasons:
            rejected[geo] = reasons
            continue
        targets[geo] = {'nodes': [node['i'] for node, field in uses],
                        'materials': sorted({material for node, field in uses for material in node.get('mats', [])}),
                        'source_type': 'LineSegments', 'wire': True}
    return targets, rejected


def ribbon_asset_paths(manifest):
    targets, rejected = line_ribbon_geometry(manifest)
    paths = {}
    for geo, detail in targets.items():
        files = [row for row in manifest['geoFiles'] if row['from'] <= geo < row['to']]
        if len(files) != 1:
            raise ValueError(f'Geometry {geo} does not resolve to one export file')
        folder = PurePosixPath(files[0]['file']).stem
        if not folder.startswith('geo_') or '/' in folder:
            raise ValueError('Unexpected export geometry folder: ' + folder)
        paths[f'/Game/Web/Geo/{folder}/StaticMeshes/g{geo}.g{geo}'] = dict(detail, geometry=geo)
    return paths, rejected
