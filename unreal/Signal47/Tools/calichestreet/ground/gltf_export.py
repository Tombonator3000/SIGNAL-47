"""Preserve the existing RGBA Color layer through Blender 5.2 glTF export.

The stock exporter explicitly skips vertex alpha used without vertex RGB
(primitive_extract.py: alpha without color). Temporarily multiply Base Color
by the existing white vertex RGB so the supported RGBA path is used. The
original node links are restored even on failure. FBX/source data is untouched.
"""
import bpy


def export_glb(path):
    restore=[]
    try:
        for material in bpy.data.materials:
            if not material.use_nodes:continue
            tree=material.node_tree;bs=tree.nodes.get('Principled BSDF')
            if not bs or not bs.inputs['Alpha'].is_linked:continue
            vertex=bs.inputs['Alpha'].links[0].from_node
            if vertex.type!='VERTEX_COLOR' or vertex.layer_name!='Color':continue
            base=bs.inputs['Base Color']
            if not base.is_linked:raise AssertionError(material.name+' missing original base-color texture')
            original=base.links[0].from_socket
            # glTF's get_multiply_factors accepts the modern MIX/RGBA node;
            # it does not recognise the legacy MixRGB node here.
            multiply=tree.nodes.new('ShaderNodeMix');multiply.data_type='RGBA';multiply.blend_type='MULTIPLY'
            multiply.inputs[0].default_value=1.0
            restore.append((tree,base,original,multiply))
            tree.links.new(original,multiply.inputs[6])
            tree.links.new(vertex.outputs['Color'],multiply.inputs[7])
            tree.links.new(multiply.outputs[2],base)
        bpy.ops.export_scene.gltf(filepath=str(path),use_selection=True,
                                  export_format='GLB',export_vertex_color='ACTIVE',
                                  export_all_vertex_colors=False)
    finally:
        for tree,base,original,multiply in reversed(restore):
            tree.links.new(original,base)
            tree.nodes.remove(multiply)
