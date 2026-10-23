# Repaints of the free models' textures, done when they are prepared (gltf_prep.py), so the sources in
# ~/Assets/signal47-free stay as they came and the result is the same on every machine:
#   - the Fairheaven LT '80 Cop Cruiser as a 1986 New Mexico county sheriff's car: tan body, white
#     doors, a six-point star and SHERIFF on the front doors, no county named (fictional), the city
#     police lettering, unit number, slogan, flags and 911 decal taken off;
#   - brand logos painted out (unreal/MODELLER.md: Chevrolet, IBM, Sony, Cobra): the boxes below are
#     where they sit in each texture, filled with the colour round them.
# models.json names a model's repaint ("repaint": "<function>"); each function gets the model's source
# folder and returns {texture file name: repainted PIL image}. gltf_prep.py writes them beside the
# glTF (S47_repaint/) and points the prepared glTF at them.
# Needs Pillow (python3 -m pip install pillow) and a bold sans font (DejaVu or Liberation).
import os

FONTS = ['/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf', '/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf',
	'/usr/share/fonts/TTF/DejaVuSans-Bold.ttf', '/Library/Fonts/Arial Bold.ttf']


def font(size):
	from PIL import ImageFont
	for f in FONTS:
		if os.path.isfile(f):
			return ImageFont.truetype(f, size)
	return ImageFont.load_default()


def fill_box(im, box, colour=None, pad=4):
	"""Fills a box with the colour round it (the median of its border) or the one given."""
	from PIL import ImageDraw
	x0, y0 = max(0, box[0] - pad), max(0, box[1] - pad)
	x1, y1 = min(im.size[0] - 1, box[2] + pad), min(im.size[1] - 1, box[3] + pad)
	if colour is None:
		px = im.load()
		ring = [px[x, y0] for x in range(x0, x1)] + [px[x, y1] for x in range(x0, x1)] + [px[x0, y] for y in range(y0, y1)] + [px[x1, y] for y in range(y0, y1)]
		ring.sort(key=lambda c: sum(c[:3]))
		colour = ring[len(ring) // 2]
	ImageDraw.Draw(im).rectangle((x0, y0, x1, y1), fill=colour)


def star_points(cx, cy, r, points=6, inner=0.5, rot=-90):
	import math
	out = []
	for k in range(points * 2):
		a = math.radians(rot + k * 180.0 / points)
		rr = r if k % 2 == 0 else r * inner
		out.append((cx + rr * math.cos(a), cy + rr * math.sin(a)))
	return out


def sheriff_door(w, h):
	"""The door's decal (RGBA, upright): a gold six-point star with balls on its tips over SHERIFF."""
	from PIL import Image, ImageDraw
	im = Image.new('RGBA', (w, h), (0, 0, 0, 0))
	d = ImageDraw.Draw(im)
	brown, gold = (74, 52, 30, 255), (196, 152, 58, 255)
	r = h * 0.27
	cx, cy = w * 0.5, h * 0.3
	d.polygon(star_points(cx, cy, r + 4, inner=0.52), fill=brown)
	d.polygon(star_points(cx, cy, r, inner=0.52), fill=gold)
	for x, y in star_points(cx, cy, r, inner=1.0)[::2]:
		d.ellipse((x - r * 0.1, y - r * 0.1, x + r * 0.1, y + r * 0.1), fill=gold, outline=brown, width=2)
	d.ellipse((cx - r * 0.3, cy - r * 0.3, cx + r * 0.3, cy + r * 0.3), outline=brown, width=3)
	f = font(int(h * 0.24))
	tw = d.textlength('SHERIFF', font=f)
	d.text((cx - tw / 2, h * 0.64), 'SHERIFF', font=f, fill=brown)
	return im


def word(text, w, h, colour):
	from PIL import Image, ImageDraw
	im = Image.new('RGBA', (w, h), (0, 0, 0, 0))
	d = ImageDraw.Draw(im)
	f = font(int(h * 0.85))
	tw = d.textlength(text, font=f)
	if tw > w:
		f = font(int(h * 0.85 * w / tw))
		tw = d.textlength(text, font=f)
	d.text(((w - tw) / 2, 0), text, font=f, fill=colour)
	return im


def cop_sheriff(src):
	"""Fairheaven LT '80 Cop Cruiser: black and white city police car to a tan and white county sheriff's car."""
	from PIL import Image
	path = os.path.join(src, 'textures', 'Fairheaven_LT80_Bodymat_baseColor.jpeg')
	im = Image.open(path).convert('RGB')
	# the old lettering, unit number, slogan, flags and the 911 decals (in 2048 px; the texture's own size)
	k = im.size[0] / 2048.0
	B = lambda x0, y0, x1, y1: tuple(int(v * k) for v in (x0, y0, x1, y1))
	for b in [B(785, 700, 1010, 842), B(590, 812, 655, 858),            # driver's door: POLICE, number, slogan; flag
			B(805, 1805, 1030, 1950), B(600, 1788, 666, 1834),           # passenger's door
			B(462, 224, 652, 258), B(64, 1240, 100, 1410),               # trunk lid and roof: POLICE
			B(1712, 798, 1830, 856), B(1712, 1782, 1830, 1842)]:         # 911 decals
		fill_box(im, b)
	# black paint to tan, white stays white (the baked shading between them blends), the detail atlas
	# at the top right (grey bakes of the underbody) left alone
	tan = (178, 152, 106)
	atlas = B(1130, 0, 2048, 630)
	px = im.load()
	W, H = im.size
	for y in range(H):
		for x in range(W):
			if atlas[0] <= x and y <= atlas[3]:
				continue
			r, g, b = px[x, y]
			if max(r, g, b) - min(r, g, b) > 24:
				continue
			t = (r + g + b) / 765.0
			px[x, y] = (int(tan[0] + (255 - tan[0]) * t), int(tan[1] + (255 - tan[1]) * t), int(tan[2] + (255 - tan[2]) * t))
	# the doors' star and SHERIFF, turned as the texture has the old lettering on each side
	dw, dh = int(250 * k), int(230 * k)
	door = sheriff_door(dw, dh)
	im.paste(door.transpose(Image.FLIP_TOP_BOTTOM), (int(770 * k), int(650 * k)), door.transpose(Image.FLIP_TOP_BOTTOM))
	im.paste(door.transpose(Image.FLIP_LEFT_RIGHT), (int(790 * k), int(1735 * k)), door.transpose(Image.FLIP_LEFT_RIGHT))
	# SHERIFF on the trunk lid and the roof, white on tan as the old lettering was white on black
	w = word('SHERIFF', int(190 * k), int(26 * k), (255, 255, 255, 255)).transpose(Image.FLIP_TOP_BOTTOM)
	im.paste(w, (int(462 * k), int(226 * k)), w)
	v = word('SHERIFF', int(170 * k), int(28 * k), (255, 255, 255, 255)).transpose(Image.ROTATE_90).transpose(Image.FLIP_TOP_BOTTOM)
	im.paste(v, (int(68 * k), int(1240 * k)), v)
	return {'Fairheaven_LT80_Bodymat_baseColor.jpeg': im}


def boxes_out(src, todo):
	"""{texture path under the model: [(box, colour or None), ...]}: each box filled with the colour
	round it (or the one given); returns {file name: image}."""
	from PIL import Image
	out = {}
	for rel, boxes in todo.items():
		im = Image.open(os.path.join(src, rel))
		alpha = 'A' in im.getbands() or (im.mode == 'P' and 'transparency' in im.info)
		im = im.convert('RGBA' if alpha else 'RGB')
		for b, c, pad in boxes:
			fill_box(im, b, c, pad)
		out[os.path.basename(rel)] = im
	return out


def ibm_3178(src):
	# the two IBM badges on the front (the plate stays, its letters go)
	return boxes_out(src, {'textures/ibm_3178_baseColor.png': [((205, 279, 220, 286), None, 1), ((160, 344, 174, 351), None, 1)]})


def ibm_3278(src):
	return boxes_out(src, {'textures/ibm_3278_baseColor.png': [((495, 116, 500, 129), None, 1)]})


def ibm_selectric(src):
	# the black IBM badge on the red cover, painted over with the cover
	return boxes_out(src, {'textures/typewriter_a_baseColor.png': [((897, 553, 920, 576), None, 2)]})


def sony_tcd5(src):
	return boxes_out(src, {'textures/TC-D5_baseColor.png': [
		((1300, 1600, 1621, 1654), None, 6),            # SONY on the front
		((2225, 1135, 2240, 1185), None, 3), ((2225, 1430, 2240, 1485), None, 3),  # on the meters
		((3790, 345, 3915, 800), None, 4)]})            # TC-D5 PRO and the Dolby mark on the top


def sony_tc510(src):
	return boxes_out(src, {
		'textures/M_Sony_a_baseColor.png': [((3590, 573, 3757, 603), None, 5), ((1022, 1621, 1180, 1655), None, 3), ((1165, 1578, 1242, 1593), None, 3)],
		'textures/M_Sony_b_baseColor.png': [((214, 2936, 386, 2971), None, 4), ((710, 1018, 860, 1042), None, 4)]})


def cobra_139(src):
	return boxes_out(src, {'textures/M_Cobra_baseColor.png': [((1859, 94, 1935, 109), None, 3), ((1418, 696, 1440, 850), None, 3)]})


def cobra_base(src):
	# the Cobra Cam 89: the name on the front strip, COBRA on the microphone, Dynascan on the label
	return boxes_out(src, {
		'textures/Front_Name_baseColor.png': [((1088, 4, 1990, 124), None, 2)],
		'textures/Mic_Name_baseColor.png': [((40, 40, 984, 470), None, 0)],
		'textures/FCC-Label_baseColor.png': [((0, 172, 1024, 500), None, 0)]})


def payphone_plain(src):
	# the 1990s Sprint and Pacific Bell card (and its 50 cents) off the payphone, a plain 1986 card on
	from PIL import Image
	out = boxes_out(src, {'textures/Texture1_baseColor.png': [((60, 330, 985, 410), (236, 232, 226), 0), ((60, 418, 985, 765), (24, 84, 196), 0)]})
	im = out['Texture1_baseColor.png']
	for text, y, h in [('LOCAL CALLS 25c', 470, 120), ('DIAL 0 FOR OPERATOR', 640, 70)]:
		w = word(text, 820, h, (240, 240, 240, 255)).transpose(Image.ROTATE_180)
		im.paste(w, (100, y), w)
	return out


REPAINTS = {'payphone_plain': payphone_plain,'cop_sheriff': cop_sheriff, 'ibm_3178': ibm_3178, 'ibm_3278': ibm_3278, 'ibm_selectric': ibm_selectric,
	'sony_tcd5': sony_tcd5, 'sony_tc510': sony_tc510, 'cobra_139': cobra_139, 'cobra_base': cobra_base}


def apply(name, src, out_dir):
	"""Writes the repaint's textures into out_dir; returns {original file name: new file name}."""
	os.makedirs(out_dir, exist_ok=True)
	done = {}
	for f, im in REPAINTS[name](src).items():
		new = os.path.splitext(f)[0] + '_s47.png'
		im.save(os.path.join(out_dir, new))
		done[f] = new
	return done
