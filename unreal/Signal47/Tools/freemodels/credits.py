# The free models' credits, from both CREDITS.txt in ~/Assets/signal47-free (CC BY 4.0 asks for title,
# author, licence and link), written twice:
#   Source/Signal47/Private/S47FreeCredits.inl   the rows SS47Ending::CreditsRows rolls at the end
#   CREDITS.txt                                  everything credited, with links; package-linux.sh puts
#                                                it beside the game
# Run by import_free.sh; the two files are committed (the roll must not depend on ignored content).
# Usage: python3 -I credits.py
import os
import sys
import json

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import gltf_prep  # noqa: E402  (read_credits only; -I leaves the script's folder out of the path)

PROJECT = os.path.normpath(os.path.join(HERE, '..', '..'))
LIMINAL = 'Vintage Liminal Props Pack (Superhive), Standard License'


def cpp(s):
	out = []
	for ch in s:
		if ch in '"\\':
			out.append('\\' + ch)
		elif ord(ch) > 126:
			out.append('\\u%04x' % ord(ch))
		else:
			out.append(ch)
	return 'TEXT("%s")' % ''.join(out)


def main():
	c = gltf_prep.read_credits()
	by = sorted((v for v in c.values() if v['source'] == 'sketchfab'), key=lambda v: (v['author'].lower(), v['title'].lower()))
	cc0 = sorted((v for v in c.values() if v['source'] == 'polyhaven'), key=lambda v: v['title'].lower())
	with open(os.path.join(PROJECT, 'Tools', 'sky', 'puresky', 'source.json'), encoding='utf-8') as source:
		sky = json.load(source)
	sky_credit = '"%s" by %s, CC0, %s' % (sky['title'], ' / '.join(sky['authors']), sky['source'])
	# Public provenance contains names/links only; licensed model/texture bytes stay private.
	with open(os.path.join(PROJECT, 'Tools', 'routenature', 'provenance.json'), encoding='utf-8') as source:
		route_packs = json.load(source)['source_packs']
	route_credits = ['"%s" by %s%s' % (pack['title'], pack['publisher'],
		' (including Quixel Megascans)' if key == 'oldwest' else '')
		for key, pack in route_packs.items()]
	# Keep the four existing model-credit rows: Ending preserves its conditional
	# SFX row and original roll order. New licensed packs join the licensed row;
	# sky photography joins the CC0 row, with its own explicit label.
	# One model per line ("Title" by Author); links remain in CREDITS.txt.
	by_lines = ['"%s" by %s' % (v['title'], v['author']) for v in by]
	rows = [
		('3D models', LIMINAL + '\nAdditional licensed environment assets (Fab):\n' + '\n'.join(route_credits)),
		('3D models, CC BY 4.0 (sketchfab.com)', '\n'.join(by_lines)),
		('', 'Links to every model: CREDITS.txt beside the game.'),
		('3D models, CC0', 'Poly Haven (polyhaven.com): ' + ', '.join(v['title'] for v in cc0) + '.' + '\nSky photography, CC0:\n' + sky_credit),
	]
	inl = ['// Made by Tools/freemodels/credits.py from ~/Assets/signal47-free/*/CREDITS.txt: do not edit.',
		'// The bought and the free 3D models, for SS47Ending::CreditsRows (Unreal only: the web version',
		'// has its own low-poly world and does not use them).']
	for k, v in rows:
		inl.append('Rows.Emplace(%s, %s);' % (cpp(k), cpp(v).replace('\n', '\\n')))
	open(os.path.join(PROJECT, 'Source', 'Signal47', 'Private', 'S47FreeCredits.inl'), 'w', encoding='utf-8').write('\n'.join(inl) + '\n')

	txt = ['SIGNAL / 47: credits for the 3D models used in the Unreal version',
		'',
		'Bought: ' + LIMINAL + '.',
		'',
		'Licensed under Creative Commons Attribution 4.0 (CC BY 4.0, https://creativecommons.org/licenses/by/4.0/).',
		'Some models were rescaled, re-centred, combined or cut apart, and brand names and logos may be removed.',
		'']
	for v in by:
		txt.append('"%s" by %s, %s, %s' % (v['title'], v['author'], v['licence'], v['link']))
	txt += ['', 'Poly Haven, CC0 (public domain, https://creativecommons.org/publicdomain/zero/1.0/); credited by choice:', '']
	for v in cc0:
		txt.append('"%s" by %s, %s, %s' % (v['title'], v['author'], v['licence'], v['link']))
	txt += ['', 'Daytime sky photography, CC0:', sky_credit,
		'', 'Additional licensed environment assets (Fab):']
	txt += [credit + ', ' + pack['url'] for credit, pack in zip(route_credits, route_packs.values())]
	txt += ['', 'MetaHuman character and hair assets: Epic Games, https://www.metahuman.com/',
		'Original waitress uniform and native integration: SIGNAL / 47.',
		'', 'Music, sound and fonts are credited in the game\'s end credits.', '']
	open(os.path.join(PROJECT, 'CREDITS.txt'), 'w', encoding='utf-8').write('\n'.join(txt))
	print('S47FREE credits: %d CC BY, %d CC0' % (len(by), len(cc0)))


if __name__ == '__main__':
	main()
