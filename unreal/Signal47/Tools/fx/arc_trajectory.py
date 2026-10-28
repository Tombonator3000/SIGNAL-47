"""Original S47 ballistic spark expressions, in emitter-local centimetres/seconds.

The runtime keeps the component stationary at the bus insulator with identity world
rotation. User.GroundZ is the local gravel height (-777 cm), not absolute world Z.
Analytic impact times make the single bounce independent of frame rate; no scene
collision, depth readback, ray tracing, or GPU simulation is involved.
"""


def assignments(contract):
    g = -contract['gravity'][2]
    restitution = contract['restitution']
    tangent = contract['tangentialRetention']
    start = 'Particles.ArcStart'
    velocity = 'Particles.ArcInitialVelocity'
    hit = 'Particles.ArcHitTime'
    rebound = 'Particles.ArcReboundSpeed'
    age = 'Particles.Age'
    dt = f'({age} - {hit})'
    before_position = f'({start} + {velocity} * {age} + float3(0.0,0.0,{-0.5*g}) * {age} * {age})'
    after_position = (f'float3({start}.xy + {velocity}.xy * {hit} + {velocity}.xy * {tangent} * {dt}, '
                      f'User.GroundZ + {rebound} * {dt} - {0.5*g} * {dt} * {dt})')
    before_velocity = f'({velocity} + float3(0.0,0.0,{-g}) * {age})'
    after_velocity = f'float3({velocity}.xy * {tangent}, {rebound} - {g} * {dt})'
    return [
        ('ParticleSpawnScript', start, 'vec3', 'Particles.Position'),
        ('ParticleSpawnScript', velocity, 'vec3', 'Particles.Velocity'),
        ('ParticleSpawnScript', hit, 'float',
         f'(sqrt({velocity}.z * {velocity}.z + {2*g} * max(0.0, {start}.z - User.GroundZ)) + {velocity}.z) / {g}'),
        ('ParticleSpawnScript', rebound, 'float', f'({g} * {hit} - {velocity}.z) * {restitution}'),
        ('ParticleUpdateScript', 'Particles.Position', 'position', f'{age} < {hit} ? {before_position} : {after_position}'),
        ('ParticleUpdateScript', 'Particles.Velocity', 'vec3', f'{age} < {hit} ? {before_velocity} : {after_velocity}'),
    ]


def kill_expression(contract):
    g = -contract['gravity'][2]
    return f'Particles.Age >= Particles.ArcHitTime + 2.0 * Particles.ArcReboundSpeed / {g}'
