"""Private pump source inspection. Run only under the project's heavy-job lease.
blender -b -t 2 --python probe.py -- --input /private/pump.glb --out /private/probe
"""
from pathlib import Path
import argparse,json,sys,math

def main():
 import bpy
 from mathutils import Vector
 a=argparse.ArgumentParser(description=__doc__);a.add_argument('--input',type=Path,required=True);a.add_argument('--out',type=Path,required=True);a.add_argument('--resume',action='store_true');args=a.parse_args(sys.argv[sys.argv.index('--')+1:])
 src=args.input.resolve();out=args.out.resolve();repo=Path(__file__).resolve().parents[4]
 assert src.is_file() and not out.is_relative_to(repo),'Source missing or private output inside public repository'
 out.mkdir(parents=True,exist_ok=True)
 if args.resume and (out/'probe-original.blend').is_file():bpy.ops.wm.open_mainfile(filepath=str(out/'probe-original.blend'))
 else:bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.gltf(filepath=str(src))
 meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];points=[o.matrix_world@Vector(c) for o in meshes for c in o.bound_box]
 lo=Vector(tuple(min(p[k] for p in points) for k in range(3)));hi=Vector(tuple(max(p[k] for p in points) for k in range(3)));centre=(lo+hi)*.5
 rows=[]
 for o in meshes:
  o.data.calc_loop_triangles();rows.append({'name':o.name,'vertices':len(o.data.vertices),'triangles':len(o.data.loop_triangles),'dimensions_m':list(o.dimensions),'materials':[m.name if m else '' for m in o.data.materials]})
 data={'source':str(src),'bounds_blender_m':[list(lo),list(hi)],'dimensions_m':list(hi-lo),'meshes':rows,'status':'SOURCE_INSPECTED_NATIVE_UNVERIFIED'}
 (out/'probe.json').write_text(json.dumps(data,indent=2)+'\n');bpy.ops.wm.save_as_mainfile(filepath=str(out/'probe-original.blend'))
 scene=bpy.context.scene;scene.render.engine='BLENDER_WORKBENCH';scene.render.resolution_x=512;scene.render.resolution_y=512;scene.render.resolution_percentage=100
 scene.display.shading.light='STUDIO';scene.display.shading.color_type='TEXTURE';scene.display.shading.show_shadows=True;scene.display.shading.show_cavity=True;scene.display.shading.background_type='WORLD'
 if scene.world is None:scene.world=bpy.data.worlds.new('PumpProbeWorld')
 scene.world.color=(.12,.12,.12)
 camera=bpy.data.objects.new('PumpProbeCamera',bpy.data.cameras.new('PumpProbeCamera'));scene.collection.objects.link(camera);scene.camera=camera;camera.data.type='ORTHO';camera.data.ortho_scale=max(hi-lo)*1.25
 for label,direction in [('posY',(0,1,.12)),('negY',(0,-1,.12)),('posX',(1,0,.12)),('negX',(-1,0,.12))]:
  camera.location=centre+Vector(direction)*max(hi-lo)*3;camera.rotation_euler=(centre-camera.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(out/(label+'.png'));bpy.ops.render.render(write_still=True)
 print('S47_PUMP_PROBE_READY',json.dumps(data),flush=True)
if __name__=='__main__':main()
