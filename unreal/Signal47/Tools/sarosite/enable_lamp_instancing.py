"""Enable only the ISM shader usage of two existing, original lamp-support materials.

No graph, texture, material parameter or mesh is rebuilt. The ordinary importers
retain the same flags on later regeneration. Run in the Unreal Python commandlet.
"""
import json
import os
from pathlib import Path
import unreal

paths = (
    '/Game/SaroSite/Materials/M_SaroSite_Concrete',
    '/Game/SubstationDetail/Materials/M_SubstationDetail_GalvanizedSteel',
)
report_path = Path(os.environ['S47_LAMP_MATERIAL_REPORT'])
report = {'status': 'RUNNING', 'materials': []}
try:
    for path in paths:
        mat = unreal.EditorAssetLibrary.load_asset(path)
        assert isinstance(mat, unreal.Material), path
        count = unreal.MaterialEditingLibrary.get_num_material_expressions(mat)
        before = bool(mat.get_editor_property('used_with_instanced_static_meshes'))
        mat.set_editor_property('used_with_instanced_static_meshes', True)
        errors = list(unreal.MaterialEditingLibrary.recompile_material(mat))
        assert not errors, (path, errors)
        assert unreal.MaterialEditingLibrary.get_num_material_expressions(mat) == count
        assert mat.get_editor_property('used_with_instanced_static_meshes')
        assert unreal.EditorAssetLibrary.save_loaded_asset(mat), path
        report['materials'].append({'path': path, 'before': before, 'after': True, 'graph_nodes_unchanged': count})
    report['status'] = 'PASS'
    unreal.log('S47 SARO lamp material instancing PASS')
except Exception as error:
    report['status'] = 'FAIL'
    report['error'] = str(error)
    raise
finally:
    report_path.write_text(json.dumps(report, indent=2) + '\n')
