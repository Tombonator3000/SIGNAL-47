"""Portable metadata and explicit triangles shared by authoring and verification."""
import copy
import hashlib
import json
import os
from pathlib import Path

from design import CONTRACT_PATH, PACKAGE, PROJECT, TOOLS, sha
from check_source import triangles
from materials import material_rows


def write_json(path, value):
    Path(path).write_text(json.dumps(value, indent=2) + '\n')


def portable_materials(output):
    rows = copy.deepcopy(material_rows())
    for row in rows:
        row['textures'] = {role: os.path.relpath(path, output) for role, path in row.get('textures', {}).items()}
    return rows


def texture_sources(output):
    return {os.path.relpath(path, output): {'sha256': sha(path), 'bytes': Path(path).stat().st_size}
            for path in sorted({p for r in material_rows() for p in r.get('textures', {}).values()})}


def source_inputs():
    files = [CONTRACT_PATH, *sorted(TOOLS.glob('*.py')),
             TOOLS.parent / 'townbuildings' / 'geometry.py',
             TOOLS.parent / 'townbuildings' / 'materials.py',
             TOOLS.parent / 'substation' / 'meshkit.py']
    files += [Path(p) for p in sorted({p for r in material_rows() for p in r.get('textures', {}).values()})]
    return {str(path.relative_to(PROJECT.parent)): sha(path) for path in files}


def indexed_triangles(shape):
    """Exact position welding; UVs remain per corner, smoothing stays per face.

    This uses the original meshkit face direction, with no remeshing, decimation
    or DCC n-gon diagonal choice. Separate overlapping solids are not booleaned.
    """
    positions, index = [], {}
    faces, corner_uvs, materials, smooth, tags = [], [], [], [], []
    for ids, uv, mat, shading, tag in triangles(shape):
        face = []
        for i in ids:
            point = tuple(shape.v[i])
            if point not in index:
                index[point] = len(positions)
                positions.append(point)
            face.append(index[point])
        faces.append(face)
        corner_uvs.append(uv)
        materials.append(mat)
        smooth.append(shading)
        tags.append(tag)
    return positions, faces, corner_uvs, materials, smooth, tags


def topology_digest(mesh):
    """Actual saved-mesh state; API imports use the geometric checks as well."""
    payload = {
        'positions': [list(v.co) for v in mesh.vertices],
        'faces': [list(p.vertices) for p in mesh.polygons],
        'material_indices': [p.material_index for p in mesh.polygons],
        'smooth': [p.use_smooth for p in mesh.polygons],
        'uv': [list(u.uv) for u in mesh.uv_layers[0].data],
        'materials': [m.name for m in mesh.materials],
    }
    return hashlib.sha256(json.dumps(payload, separators=(',', ':')).encode()).hexdigest()
