"""Original cloud-tank gas source. Root runs Blender through unreal_guard.

No process is launched by this file. --output must be a fresh directory.
The editable scene and OpenVDB cache remain useful even if atlas packing fails.
"""
import argparse
import json
import math
from pathlib import Path
import re
import sys
import threading
import time


HERE = Path(__file__).resolve().parent


def log(message):
    print('S47 CLOUD ' + message, flush=True)


def arguments():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--resolution', type=int, choices=(64, 128, 192, 256), default=128)
    parser.add_argument('--frames', type=int, default=48)
    parser.add_argument('--picks', type=int, nargs='+')
    parser.add_argument('--atlas-size', type=int, choices=(64, 128, 192, 256), default=128)
    parser.add_argument('--bit-depth', type=int, choices=(8,16), help='Default: 16 for a 256 atlas, otherwise 8')
    parser.add_argument('--vorticity', type=float, default=.18)
    parser.add_argument('--buoyancy', type=float, default=-.14, help='Density buoyancy; negative values sink')
    parser.add_argument('--vortex-strength', type=float, default=0, help='Positive strength enables three local lip rotors')
    parser.add_argument('--noise-scale', type=int, choices=(0,2), default=0, help='0 disables noise; 2 bakes wavelet noise at twice the base grid resolution')
    parser.add_argument('--noise-strength', type=float, default=.35)
    parser.add_argument('--noise-pos-scale', type=float, default=2)
    parser.add_argument('--noise-time-anim', type=float, default=.1)
    parser.add_argument('--no-pack', action='store_true')
    parser.add_argument('--setup-only', action='store_true')
    argv = sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else sys.argv[1:]
    args = parser.parse_args(argv)
    if not 12 <= args.frames <= 120:
        parser.error('--frames must be between 12 and 120')
    if not all(math.isfinite(value) for value in (args.vorticity,args.buoyancy,args.vortex_strength,
                                                args.noise_strength,args.noise_pos_scale,args.noise_time_anim)):
        parser.error('Fluid parameters must be finite')
    if args.vorticity < 0 or args.vortex_strength < 0 or not -5 <= args.buoyancy <= 5:
        parser.error('Vorticity/vortex strength must be nonnegative; buoyancy must be inside [-5,5]')
    if not 0 <= args.noise_strength <= 10 or not .0001 <= args.noise_pos_scale <= 10 or not .0001 <= args.noise_time_anim <= 10:
        parser.error('Noise strength must be inside [0,10]; position scale/time inside [.0001,10]')
    if args.noise_scale and args.resolution*args.noise_scale > 256:
        parser.error('This bounded candidate supports at most 256 cells on the longest noise-grid axis')
    if args.bit_depth is None:
        args.bit_depth = 16 if args.atlas_size == 256 else 8
    args.picks = args.picks or [round(args.frames*.60), round(args.frames*.80), args.frames]
    args.picks = sorted(set(args.picks))
    if not 1 <= len(args.picks) <= 3 or any(not 1 <= f <= args.frames for f in args.picks):
        parser.error('--picks must select one to three frames inside the bake range')
    return args


def setup(args, out):
    import bpy
    if not bpy.app.background:
        raise RuntimeError('Run in background Blender, through the root guard')
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 1
    scene.frame_start, scene.frame_end = 1, args.frames
    scene.render.fps = 24
    scene.gravity = (0, 0, -9.81)
    scene.use_gravity = True
    flows = bpy.data.collections.new('EDITABLE_SMOKE_EMITTERS')
    scene.collection.children.link(flows)

    bpy.ops.mesh.primitive_cube_add(size=2, location=(0, 0, 0))
    domain = bpy.context.object
    domain.name = 'CloudTank_GasDomain'
    domain.dimensions = (10, 14, 8)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    modifier = domain.modifiers.new('Original_Mantaflow_Density', 'FLUID')
    modifier.fluid_type = 'DOMAIN'
    bpy.context.view_layer.update()
    settings = modifier.domain_settings
    settings.domain_type = 'GAS'
    settings.resolution_max = args.resolution
    settings.cache_type = 'MODULAR'
    settings.cache_directory = '//cache'
    settings.cache_frame_start, settings.cache_frame_end = 1, args.frames
    settings.cache_data_format = 'OPENVDB'
    settings.cache_resumable = True
    settings.openvdb_cache_compress_type = 'BLOSC'
    settings.use_adaptive_domain = False  # Stable spatial coordinates across chosen frames.
    settings.use_adaptive_timesteps = True
    settings.timesteps_min, settings.timesteps_max = 1, 4
    settings.cfl_condition = 2
    settings.time_scale = 1
    settings.alpha = args.buoyancy
    settings.beta = 0
    settings.vorticity = args.vorticity
    settings.use_noise = args.noise_scale > 0
    if settings.use_noise:
        # RNA names follow Blender 5.2's installed fluid UI. Cache must be
        # resumable and noise enabled during the base bake to retain its inputs.
        settings.noise_scale = args.noise_scale
        settings.noise_strength = args.noise_strength
        settings.noise_pos_scale = args.noise_pos_scale
        settings.noise_time_anim = args.noise_time_anim
        settings.cache_noise_format = 'OPENVDB'
    settings.use_dissolve_smoke = False
    settings.clipping = .00001
    settings.fluid_group = flows
    for side in ('front', 'back', 'left', 'right', 'top', 'bottom'):
        setattr(settings, 'use_collision_border_' + side, False)
    domain.display_type = 'WIRE'
    domain['source_authorship'] = 'Original SIGNAL / 47 cloud-tank gas simulation'
    domain['source_axes'] = 'Blender metres: X forward, Y across bank, Z up; leading lip travels toward -X'

    material = bpy.data.materials.new('CloudTank_SourcePreview')
    material.use_nodes = True
    material.node_tree.nodes.clear()
    volume = material.node_tree.nodes.new('ShaderNodeVolumePrincipled')
    volume.inputs['Density'].default_value = 3
    volume.inputs['Color'].default_value = (.55, .59, .65, 1)
    output = material.node_tree.nodes.new('ShaderNodeOutputMaterial')
    material.node_tree.links.new(volume.outputs['Volume'], output.inputs['Volume'])
    domain.data.materials.append(material)

    # A broad hanging sheet with unequal heads. Horizontal shear and weak negative
    # buoyancy roll its leading edge down and forward after the inflow ends.
    emit_until = max(10, min(22, round(args.frames*.40)))
    specs = [
        ('Broad_rear_sheet', (1.70, -.10, 2.55), (1.28, 4.75, .36), (-1.20, .03, -.12), .72),
        ('Heavy_left_overhang', (.62, -4.60, 2.05), (1.05, 1.05, .64), (-1.65, .08, -.18), .94),
        ('Left_upper_fold', (1.12, -3.15, 2.75), (.92, 1.26, .54), (-1.30, -.10, -.10), .86),
        ('Low_leading_cap', (.25, -1.43, 1.90), (1.05, 1.18, .66), (-1.90, .10, -.30), 1.0),
        ('Centre_hanging_sheet', (1.03, .15, 2.45), (.85, 1.02, .43), (-1.38, -.10, -.14), .78),
        ('High_right_head', (1.28, 1.82, 2.92), (.82, 1.06, .52), (-1.25, .06, -.10), .88),
        ('Right_heavy_fold', (.58, 3.52, 2.24), (1.13, 1.30, .65), (-1.72, -.09, -.24), .96),
        ('Trailing_right_wisp', (1.65, 5.03, 2.63), (.75, .79, .39), (-1.16, -.12, -.10), .68),
    ]
    for name, at, scale, velocity, density in specs:
        bpy.ops.mesh.primitive_uv_sphere_add(segments=20, ring_count=12, radius=1, location=at)
        emitter = bpy.context.object
        emitter.name = name
        emitter.scale = scale
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        for collection in list(emitter.users_collection):
            collection.objects.unlink(emitter)
        flows.objects.link(emitter)
        emitter.display_type = 'WIRE'
        emitter.hide_render = True
        flow_modifier = emitter.modifiers.new('Smoke_only_no_fire', 'FLUID')
        flow_modifier.fluid_type = 'FLOW'
        bpy.context.view_layer.update()
        flow = flow_modifier.flow_settings
        flow.flow_type, flow.flow_behavior, flow.flow_source = 'SMOKE', 'INFLOW', 'MESH'
        flow.temperature, flow.density = 0, density
        flow.use_absolute = True
        flow.surface_distance, flow.volume_density = 1.5, 1
        flow.use_initial_velocity = True
        flow.velocity_coord = velocity
        flow.velocity_factor = 0
        flow.use_inflow = True
        flow.keyframe_insert('use_inflow', frame=1)
        flow.keyframe_insert('use_inflow', frame=emit_until)
        flow.use_inflow = False
        flow.keyframe_insert('use_inflow', frame=emit_until+1)
        emitter['emission_ends_after_frame'] = emit_until

    rotors = []
    if args.vortex_strength > 0:
        rotor_collection = bpy.data.collections.new('EDITABLE_LOCAL_LIP_ROTORS')
        scene.collection.children.link(rotor_collection)
        # Local +Z rotated +90 degrees about X becomes world -Y. Positive
        # rotation therefore sends the top toward -X and the front downward.
        # Spherical distance limits keep all three curls local to the lip.
        for name,at,factor,radius in (
                ('Lip_roll_left',(-1.10,-3.60,1.20),.85,2.20),
                ('Lip_roll_centre',(-1.75,-.55,.85),1.00,2.35),
                ('Lip_roll_right',(-1.05,3.45,1.35),.75,2.05)):
            bpy.ops.object.effector_add(type='VORTEX',location=at,rotation=(math.pi/2,0,0))
            rotor = bpy.context.object
            rotor.name = name
            for collection in list(rotor.users_collection):
                collection.objects.unlink(rotor)
            rotor_collection.objects.link(rotor)
            rotor.empty_display_size = .65
            field = rotor.field
            field.shape = 'POINT'
            field.strength = args.vortex_strength*factor
            field.inflow = 0
            field.flow = 0
            field.apply_to_location = True
            field.apply_to_rotation = False
            field.falloff_type = 'SPHERE'
            field.falloff_power = 2
            field.use_min_distance,field.distance_min = True,.45
            field.use_max_distance,field.distance_max = True,radius
            rotors.append(dict(name=name,position=list(rotor.location),
                               rotation_x_degrees=90,world_axis=[0,-1,0],
                               strength=field.strength,maximum_distance_m=field.distance_max,
                               minimum_distance_m=field.distance_min,falloff_power=field.falloff_power))

    bpy.ops.object.select_all(action='DESELECT')
    domain.select_set(True)
    bpy.context.view_layer.objects.active = domain
    scene.frame_set(1)
    plan = dict(schema=1, authorship='Original SIGNAL / 47', blender=bpy.app.version_string,
                simulation_resolution_max=args.resolution, frame_range=[1, args.frames], fps=24,
                picked_frames=args.picks, atlas_size=args.atlas_size, bit_depth=args.bit_depth,
                domain_dimensions_m=[10,14,8],
                buoyancy_density=settings.alpha, buoyancy_heat=settings.beta, vorticity=settings.vorticity,
                vortex_strength=args.vortex_strength,local_rotors=rotors,
                cache_format=settings.cache_data_format, adaptive_domain=False, noise=settings.use_noise,
                noise_scale=args.noise_scale,noise_strength=args.noise_strength,
                noise_pos_scale=args.noise_pos_scale,noise_time_anim=args.noise_time_anim,
                noise_method='Mantaflow wavelet' if args.noise_scale else None,
                packed_cache_kind='noise' if args.noise_scale else 'data',
                density_axes='X forward, Y across bank, Z up; leading lip toward -X',
                emitters=[dict(name=n, position=p, scale=s, velocity=v, density=d) for n,p,s,v,d in specs])
    (out/'source.json').write_text(json.dumps(plan, indent=2)+'\n')
    bpy.ops.wm.save_as_mainfile(filepath=str(out/'CloudTank.blend'))
    return bpy, domain


def monitor(cache, frames, started, stop, kind='data'):
    while not stop.wait(8):
        files = list(cache.rglob('fluid_'+kind+'_*.vdb')) if cache.exists() else []
        numbers = [int(m.group(1)) for p in files if (m := re.search(r'(\d+)\.vdb$', p.name))]
        log('BAKING kind=%s elapsed=%.1fs cache_frames=%d/%d latest=%s' %
            (kind,time.monotonic()-started,len(numbers),frames,max(numbers) if numbers else 'pending'))


def bake_stage(bpy, domain, args, out, kind):
    bpy.ops.object.select_all(action='DESELECT')
    domain.select_set(True)
    bpy.context.view_layer.objects.active = domain
    bpy.context.scene.frame_set(1)
    started,stop = time.monotonic(),threading.Event()
    worker = threading.Thread(target=monitor,args=(out/'cache',args.frames,started,stop,kind),daemon=True)
    worker.start()
    try:
        log('BAKE %s START; OpenVDB remains under %s' % (kind.upper(),out/'cache'/kind))
        result = bpy.ops.fluid.bake_noise() if kind == 'noise' else bpy.ops.fluid.bake_data()
        if 'FINISHED' not in result:
            raise RuntimeError('Mantaflow %s bake did not finish: %s' % (kind,result))
    finally:
        stop.set()
        worker.join(timeout=1)
    # Preserve the base result before beginning the optional high-resolution stage.
    bpy.ops.wm.save_as_mainfile(filepath=str(out/'CloudTank.blend'))
    log('BAKE %s COMPLETE elapsed=%.1fs' % (kind.upper(),time.monotonic()-started))


def main():
    args = arguments()
    out = args.output.expanduser().resolve()
    if out.exists():
        raise FileExistsError('Fresh output directory required: ' + str(out))
    out.mkdir(parents=True, exist_ok=False)
    log('SETUP output=%s resolution=%d frames=%d picks=%s atlas=%d bit_depth=%d vorticity=%.3f buoyancy=%.3f vortex=%.3f noise_scale=%d noise_strength=%.3f' %
        (out,args.resolution,args.frames,args.picks,args.atlas_size,args.bit_depth,args.vorticity,args.buoyancy,args.vortex_strength,args.noise_scale,args.noise_strength))
    bpy, domain = setup(args, out)
    log('SOURCE SAVED ' + str(out/'CloudTank.blend'))
    if args.setup_only:
        log('SETUP COMPLETE; simulation has not run')
        return
    bake_stage(bpy,domain,args,out,'data')
    if args.noise_scale:
        bake_stage(bpy,domain,args,out,'noise')
    if not args.no_pack:
        sys.path.insert(0, str(HERE))
        from pack_clouds import pack_cache
        pack_cache(out/'cache', out/'atlases', args.picks, args.atlas_size, args.bit_depth,
                   cache_kind='noise' if args.noise_scale else 'data')
    log('COMPLETE source=%s cache=%s' % (out/'CloudTank.blend', out/'cache'))


if __name__ == '__main__':
    main()
