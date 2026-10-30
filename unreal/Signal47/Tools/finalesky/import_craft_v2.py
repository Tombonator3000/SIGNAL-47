"""Replace only /Game/FinaleSky/SM_S47_FinalCraft from the separate v2 source.

Root runs Unreal through unreal_guard with S47_CRAFT_V2_SOURCE set to the new
output directory or manifest.json. Existing four material instances are read only.
Use the legacy FBX importer, as in this project's proven native recipe.
"""
import importlib.util
import json
import os
from pathlib import Path

import unreal

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parents[1]
DEST,NAME = '/Game/FinaleSky','SM_S47_FinalCraft'
REVISION = 'S47-FINALE-SKY-2'
ROLES = ('Hull','Ribs','Recess','Light')
SLOTS = ['M_S47_Final'+role for role in ROLES]


def log(message):
    unreal.log('S47 CRAFT V2 '+message)
    print('S47 CRAFT V2 '+message,flush=True)


class Checks:
    """The existing FBX helper requires only check(); no report/audit files."""
    def check(self,name,condition,detail=None):
        if not condition:
            raise AssertionError((name,detail))


def read_source(path):
    path = path/'manifest.json' if path.is_dir() else path
    data = json.loads(path.read_text())
    assert data.get('schema') == 1 and data.get('revision') == REVISION
    assert data.get('authorship') == 'Original SIGNAL / 47 craft geometry v2'
    assert data.get('mesh') == NAME and data.get('destination') == DEST
    assert data.get('material_slots') == SLOTS
    assert [row['lod'] for row in data['lods']] == [0,1,2]
    for row in data['lods']:
        source = (path.parent/row['fbx']).resolve()
        assert source.is_relative_to(path.parent) and source.is_file() and source.suffix.lower() == '.fbx'
        row['_source'] = source
    return data


def main():
    manifest = read_source(Path(os.environ['S47_CRAFT_V2_SOURCE']).expanduser().resolve())
    editor = unreal.EditorAssetLibrary
    subsystem = (unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
                 or unreal.new_object(unreal.StaticMeshEditorSubsystem))
    materials = {}
    for role in ROLES:
        material = editor.load_asset(DEST+'/MI_S47_Final'+role)
        assert isinstance(material,unreal.MaterialInstanceConstant), 'Existing final material required: '+role
        materials['M_S47_Final'+role] = material

    # Reuse only the explicit mesh-import function; it replaces this exact owned
    # mesh and creates neither materials nor textures. No v1 gate is involved.
    spec = importlib.util.spec_from_file_location('s47_craft_v2_fbx',PROJECT/'Tools/motelv2/native.py')
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    helper.DEST = DEST
    mesh = helper.import_mesh(manifest['lods'][0]['_source'],DEST,NAME,Checks())
    for lod in (1,2):
        assert subsystem.import_lod(mesh,lod,str(manifest['lods'][lod]['_source'])) == lod
    assert subsystem.get_lod_count(mesh) == 3
    slots = list(mesh.get_editor_property('static_materials'))
    actual_names = [str(slot.get_editor_property('imported_material_slot_name')) for slot in slots]
    assert len(actual_names) == 4 and set(actual_names) == set(SLOTS), actual_names
    for index,name in enumerate(actual_names):
        mesh.set_material(index,materials[name])
    mesh.set_editor_property('allow_cpu_access',True)
    nanite = mesh.get_editor_property('nanite_settings')
    nanite.set_editor_property('enabled',False)
    mesh.set_editor_property('nanite_settings',nanite)
    for lod in range(3):
        build = subsystem.get_lod_build_settings(mesh,lod)
        for key,value in (('use_full_precision_u_vs',True),('remove_degenerates',False),
                          ('recompute_normals',False),('recompute_tangents',True),('use_mikk_t_space',True)):
            build.set_editor_property(key,value)
        subsystem.set_lod_build_settings(mesh,lod,build)
    assert subsystem.set_lod_screen_sizes(mesh,manifest['lod_screen_sizes'])
    assert subsystem.remove_collisions(mesh)
    for lod in range(3):
        sections = mesh.get_num_sections(lod)
        assert sections > 0
        for section in range(sections):
            subsystem.enable_section_collision(mesh,False,lod,section)
        assert not any(subsystem.is_section_collision_enabled(mesh,lod,i) for i in range(sections))
    assert subsystem.get_simple_collision_count(mesh) == 0
    assert mesh.get_path_name() == DEST+'/'+NAME+'.'+NAME
    assert editor.save_loaded_asset(mesh,False)
    counts = []
    for lod,row in enumerate(manifest['lods']):
        count = mesh.get_static_mesh_description(lod).get_triangle_count()
        assert count == row['triangles'], (lod,count,row['triangles'])
        counts.append(count)
    log('IMPORT COMPLETE saved=%s lod_triangles=%s slots=%s collision=off; runtime view pending' %
        (mesh.get_path_name(),counts,actual_names))


if __name__ == '__main__':
    main()
