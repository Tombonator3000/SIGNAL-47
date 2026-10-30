"""Authored corridor wear: original deterministic maps and carefully placed patches.

No scene reset, Office mutation, external imagery or standalone heavy execution.
The caller runs generate_maps in Blender, then appends wear_parts to its layout.
"""
from pathlib import Path
import math

MATERIAL = "M_Saro_CorridorWear"


def material_rows():
    return [{"name": MATERIAL, "base_color_linear": [1, 1, 1, 1],
        "roughness": .93, "metallic": 0, "specular": .20,
        "alpha_mode": "dithered_masked", "opacity_from_base_color": True,
        "two_sided": False, "normal_strength": 0,
        "textures": {"base_color": "textures/corridor_wear_base.png",
                     "roughness": "textures/corridor_wear_rough.png"},
        "source_use": "Original seeded grime, hand scuffs, heel/rubber marks and paint chips; no baked light"}]


def generate_maps(root):
    import bpy
    import numpy as np
    size, cell = 1024, 256
    v, u = np.mgrid[0:cell, 0:cell]/(cell-1)*2-1
    rgba = np.ones((size, size, 4), dtype=np.float32)
    rgba[:, :, :3] = 0
    rgba[:, :, 3] = 0
    rough = np.ones((size, size, 4), dtype=np.float32)
    rough[:, :, :3] = .94

    def noise(rng, frequency):
        grid = rng.uniform(-1, 1, (frequency+1, frequency+1))
        yy = (v+1)*.5*(frequency-.001)
        xx = (u+1)*.5*(frequency-.001)
        iy, ix = yy.astype(int), xx.astype(int)
        fy, fx = yy-iy, xx-ix
        fy, fx = fy*fy*(3-2*fy), fx*fx*(3-2*fx)
        return ((grid[iy, ix]*(1-fx)+grid[iy, ix+1]*fx)*(1-fy)
                +(grid[iy+1, ix]*(1-fx)+grid[iy+1, ix+1]*fx)*fy)

    def stroke(x0, y0, x1, y1, width):
        dx, dy = x1-x0, y1-y0
        t = np.clip(((u-x0)*dx+(v-y0)*dy)/(dx*dx+dy*dy), 0, 1)
        distance = np.sqrt((u-x0-t*dx)**2+(v-y0-t*dy)**2)
        return np.exp(-(distance/width)**2)*(1-.25*t)

    border = np.clip((1-np.abs(u)**8)*5, 0, 1)*np.clip((1-np.abs(v)**8)*5, 0, 1)
    for row in range(4):
        for col in range(4):
            rng = np.random.default_rng(47033000+row*101+col*17)
            broad, broken = noise(rng, 7), noise(rng, 37)
            alpha = np.zeros_like(u)
            if row == 0:  # Sock-line dirt, interrupted at different heights.
                for i in range(11):
                    x, y = rng.uniform(-.9, .9), rng.uniform(-.80, -.23)
                    sx, sy = rng.uniform(.08, .42), rng.uniform(.08, .35)
                    alpha = np.maximum(alpha, np.exp(-((u-x)/sx)**2-((v-y)/sy)**2)*rng.uniform(.12, .52))
                alpha *= np.clip(.80+broad*.55+broken*.45, .08, 1.2)
                color = np.array((.125, .109, .080))
            elif row == 1:  # Unequal finger/hand drag; no full rectangular palm stamp.
                for i in range(rng.integers(4, 7)):
                    x, y = rng.uniform(-.55, .50), rng.uniform(-.5, .1)
                    alpha = np.maximum(alpha, stroke(x, y, x+rng.uniform(-.25, .25),
                        y+rng.uniform(.25, .75), rng.uniform(.025, .08))*rng.uniform(.22, .56))
                alpha += np.exp(-((u+.1)/.48)**2-((v+.20)/.36)**2)*.12
                alpha *= np.clip(.72+broad*.5+broken*.42, .04, 1.2)
                color = np.array((.105, .098, .078))
            elif row == 2:  # Short rubber/heel scuffs with a broken curved outer edge.
                for i in range(rng.integers(5, 10)):
                    x, y = rng.uniform(-.75, .5), rng.uniform(-.7, .7)
                    alpha = np.maximum(alpha, stroke(x, y, x+rng.uniform(.12, .55),
                        y+rng.uniform(-.18, .22), rng.uniform(.009, .027))*rng.uniform(.35, .85))
                radius = np.sqrt(((u-rng.uniform(-.25, .25))/.75)**2+((v+.12)/.49)**2)
                arc = np.exp(-((radius-.72)/.045)**2)*np.clip(-u+.25, 0, 1)*.48
                alpha = np.maximum(alpha, arc)
                alpha *= np.clip(.72+broad*.38+broken*.85, 0, 1.2)
                color = np.array((.065, .069, .062))
            else:  # Small ragged undercoat exposures; not an even noise layer.
                for i in range(rng.integers(7, 13)):
                    x, y = rng.uniform(-.75, .75), rng.uniform(-.65, .6)
                    sx, sy = rng.uniform(.045, .19), rng.uniform(.035, .13)
                    shape = 1-((u-x)/sx)**2-((v-y)/sy)**2+broken*.85
                    alpha = np.maximum(alpha, np.clip(shape*7, 0, 1))
                color = np.array((.49, .465, .375))
            alpha = np.clip(alpha*border, 0, 1)
            # Quiet colour variation stays inside each physical mark. No AO,
            # directional light, wall shading or whole-room dirt is baked here.
            linear = np.clip(color[None, None, :]*(1+broad[:, :, None]*.09), .001, 1)
            rgb = np.where(linear < .0031308, linear*12.92, 1.055*linear**(1/2.4)-.055)
            ys, xs = slice(row*cell, (row+1)*cell), slice(col*cell, (col+1)*cell)
            rgba[ys, xs, :3] = rgb
            rgba[ys, xs, 3] = alpha
            rough[ys, xs, :3] = np.clip(.91+broad[:, :, None]*.045+alpha[:, :, None]*.05, .80, 1)

    out = Path(root)/"textures"
    out.mkdir(exist_ok=True)
    for filename, pixels in (("corridor_wear_base.png", rgba), ("corridor_wear_rough.png", rough)):
        image = bpy.data.images.new(filename, size, size, alpha=True)
        # RGB is already encoded sRGB for base colour; alpha/roughness are raw.
        image.colorspace_settings.name = "Non-Color"
        image.pixels.foreach_set(pixels.ravel())
        image.filepath_raw = str(out/filename)
        image.file_format = "PNG"
        image.save()
        bpy.data.images.remove(image)


def wear_parts():
    parts = []

    def uv(cell):
        col, row = cell % 4, cell // 4
        pad = .004
        a, b, c, d = col/4+pad, (col+1)/4-pad, row/4+pad, (row+1)/4-pad
        return [(a, c), (b, c), (b, d), (a, d)]

    def wall(name, side, x, y, width, height, cell, angle=0, front=None):
        north = side == "north"
        if front is None:
            front = (4.8243 if y < 1.19 else 4.8362) if north else (6.5837 if y < 1.19 else 6.5778)
        points = []
        for a, b in ((-width/2, -height/2), (width/2, -height/2),
                     (width/2, height/2), (-width/2, height/2)):
            points.append((x+a*math.cos(angle)-b*math.sin(angle),
                           y+a*math.sin(angle)+b*math.cos(angle), front))
        tex = uv(cell)
        if not north:
            points, tex = list(reversed(points)), list(reversed(tex))
        parts.append({"name": "Wear_"+name, "points_three_m": points,
                      "uv": tex, "material": MATERIAL})

    def floor(name, x, z, width, depth, cell, angle, lift):
        points = []
        for a, b in ((-width/2, depth/2), (width/2, depth/2),
                     (width/2, -depth/2), (-width/2, -depth/2)):
            points.append((x+a*math.cos(angle)-b*math.sin(angle), .0092+lift,
                           z+a*math.sin(angle)+b*math.cos(angle)))
        parts.append({"name": "Wear_"+name, "points_three_m": points,
                      "uv": uv(cell), "material": MATERIAL})

    # The route is predominantly west/east, with local turn marks at actual doors.
    for i, (x, z, w, d, cell, angle) in enumerate((
        (-5.20, 5.71, 1.40, .43, 8, .04), (-3.94, 5.66, 1.10, .46, 9, -.07),
        (-2.66, 5.75, 1.34, .53, 10, -.03), (-1.35, 5.67, 1.06, .47, 11, .13),
        (-.12, 5.76, 1.22, .43, 9, -.11), (1.13, 5.71, .88, .46, 8, .06),
        (-2.4, 5.05, .63, .29, 11, .08), (-3.9, 6.31, .72, .30, 10, -.04),
        (-5.67, 5.69, .49, .65, 9, -.06))):
        floor("traffic_%02d" % i, x, z, w, d, cell, angle, (i % 3)*.00015)
    for i, (x, z, w, d, cell, angle) in enumerate((
        (-4.62, 5.70, 1.68, .66, 0, -.05),
        (-2.02, 5.77, 1.91, .69, 2, .08),
        (.34, 5.69, 1.39, .61, 3, -.09))):
        floor("matte_traffic_%02d" % i, x, z, w, d, cell, angle, .00055+i*.0001)

    # Dirty paint just above, rather than uniformly over, the black skirting.
    for i, (side, x, width, height, cell) in enumerate((
        ("north", -5.45, .91, .22, 0), ("north", -4.35, 1.03, .18, 1),
        ("north", -3.28, .48, .24, 2), ("north", -1.48, .48, .17, 3),
        ("north", -.42, .85, .20, 1), ("north", .76, .83, .19, 0),
        ("south", -5.35, 1.08, .25, 2), ("south", -4.72, .30, .18, 0),
        ("south", -2.92, .73, .21, 3), ("south", -1.70, .83, .23, 1))):
        wall("sockline_%02d" % i, side, x, .22, width, height, cell, (i % 3-1)*.012)

    # Hand/shoulder contact beside the two working doors. Printed matter and
    # the original switch plate are excluded; each mark is outside its portal.
    wall("control_hand_low", "north", -3.025, 1.035, .13, .28, 4, -.06)
    wall("control_switch_drag", "north", -3.157, 1.31, .045, .19, 5, .02)
    wall("records_hand_left", "south", -4.515, 1.355, .105, .30, 6, -.03)
    wall("records_hand_right", "south", -3.275, 1.075, .11, .25, 7, .06)
    wall("dado_rub_north", "north", -3.29, 1.198, .29, .023, 9, 0, 4.8292)
    wall("dado_rub_south", "south", -4.70, 1.198, .25, .024, 10, 0, 6.5788)

    # Uneven cream undercoat chips in the lower green paint, close to jambs.
    for i, (side, x, y, w, h, cell) in enumerate((
        ("north", -3.018, .77, .080, .14, 12), ("north", -3.12, 1.12, .16, .055, 13),
        ("north", -1.75, .34, .12, .075, 15), ("north", .28, .55, .11, .11, 14),
        ("south", -4.515, .79, .083, .14, 14), ("south", -3.275, .42, .09, .11, 13),
        ("south", -4.74, 1.135, .19, .060, 12), ("south", -1.46, .28, .12, .07, 15))):
        wall("paint_chips_%02d" % i, side, x, y, w, h, cell, (i % 3-1)*.09)
    return parts
