"""Technical size/format export only; all scene editing happened in image_gen.

First run: python export_rasters.py /absolute/path/ai-jobs.json
Later reproducible export: python export_rasters.py
The selected generator originals and complete prompts are retained alongside this.
"""
from pathlib import Path
import hashlib
import json
import shutil
import sys
from PIL import Image, ImageCms, ImageOps

HERE = Path(__file__).resolve().parent
ART = HERE.parent.parent
PROFILE = HERE / 'srgb.icc'
if not PROFILE.exists():
    PROFILE.write_bytes(ImageCms.ImageCmsProfile(ImageCms.createProfile('sRGB')).tobytes())
ICC = PROFILE.read_bytes()

PEOPLE = {
    'nora': 'Nora Vega, 57, varm lampe og dempet motellneon',
    'waitress': 'Servitrisen på Mesa Diner, cirka 45',
    'driver': 'Sjåføren i dineren, cirka 55',
    'ward': 'Dr. Evelyn Ward, cirka 50, falmet ID-portrett',
    'dale': 'Dale, cirka 35, flanellskjorte og falmet ID-portrett',
    'tomas_1947': 'Tomás Vega, cirka 22 i 1947, sepia og hvit fotokant',
    'deputy': 'Nestlederen, cirka 40, blank sheriffstjerne',
    'host': 'Earl, cirka 60, nattradioen',
    'attendant': 'Betjenten, cirka 30, direkte blikk og blankt navneskilt',
    'radio_woman': 'Kvinnen med transistorradioen, cirka 70',
    'souvenir_man': 'Suvenirmannen, cirka 50, westernskjorte og bolo',
}
WINDOWS = {
    'gas': 'Bensinstasjonens kontor og betjenten',
    'sheriff': 'Tomt sheriffkontor med kart og to rader knappenåler',
    'studio': 'Radiostudioet med Earl',
    'studio_empty': 'Samme radiostudio etter klokka fire, Earl borte',
    'laundromat': 'Seks vaskemaskiner, én i gang, ingen folk',
    'tv': 'Sovende person bakfra i tv-lyset',
    'kitchen': 'Stillestående skygge bak kjøkkengardinen',
    'souvenir': 'Suvenirutstilling med pappfigur og falske vrakdeler',
}
KIT = {
    'notebook': 'Brun notatbok med strikk og blyant',
    'jacket': 'Arbeidsjakka bakfra med bredt refleksbånd',
    'camera': 'Sammenleggbart feltkamera for planfilm',
    'flashlight': 'Lang lommelykt med riflet grep',
    'keys': 'To bilnøkler og blank lærlapp',
    'papers': 'Brettede ark og fanfoldpapir med binders',
    'nickel_1947': 'Jefferson-femcent med korrekt 1947',
}
SPECS = {f'people/portrait_{k}.jpg': ([512,640],v) for k,v in PEOPLE.items()}
SPECS.update({f'town/win_{k}.jpg': ([1024,512],v) for k,v in WINDOWS.items()})
SPECS.update({f'kit/kit_{k}.png': ([256,256],v) for k,v in KIT.items()})
SPECS.update({
    'town/cutout_alien.png': ([512,1024], 'Flat håndmalt pappfigur med blank prislapp'),
    'town/paper_sentinel_blank.jpg': ([768,1024], 'Avisunderlag, blankt navnefelt og overskrift, golfbilde'),
    'abduct/frame_above.jpg': ([1024,768], 'Reyes på ryggen, hard blitz, ansikt skjult'),
    'abduct/frame_light.jpg': ([1024,768], 'Hvitt lys ovenfra, gress og bein uten synlig kilde'),
    'abduct/frame_truck.jpg': ([1024,768], 'Hvit servicepickup, åpen dør og tente frontlys'),
    'abduct/frame_figures.jpg': ([1024,768], 'Tre små ansiktsløse silhuetter på mesaryggen'),
    'board/tex_cork.jpg': ([1024,1024], 'Varm kork med små stifthull'),
    'album/tex_album_page.jpg': ([1024,1024], 'Mørkt falmet albumpapir med fin struktur'),
    'motel/tv_lot_above.jpg': ([640,480], 'Motellparkering ovenfra med hvit servicepickup'),
    'brand/og_cover.jpg': ([1200,630], 'Nattlig array under Melkeveien, rolig venstre tredjedel'),
})


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    records = []
    selected_path = HERE / 'raster-specs.json'
    if len(sys.argv) > 1:
        attempts = json.loads(Path(sys.argv[1]).read_text())
        for a in attempts:
            original = Path(a['source_path'])
            a['generator_sha256'] = sha(original)
            a['generator_size'] = list(Image.open(original).size)
            if a.get('rejected'):
                continue
            dest = HERE / 'raster' / (a['file'].replace('/', '__').rsplit('.',1)[0]+'.png')
            dest.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(original, dest)
            a['source'] = str(dest.relative_to(HERE))
            a['method'] = 'image_gen.imagegen'
            a['size'], a['description'] = SPECS[a['file']]
            if a['file'] in ('board/tex_cork.jpg','album/tex_album_page.jpg'):
                a['tile_axes'] = 'xy'
            records.append(a)
        (HERE / 'generation-attempts.json').write_text(json.dumps(attempts,ensure_ascii=False,indent=2)+'\n')
        selected_path.write_text(json.dumps(records,ensure_ascii=False,indent=2)+'\n')
    else:
        records = json.loads(selected_path.read_text())
    assert set(SPECS)=={r['file'] for r in records}, 'Missing or duplicate generated assets'
    for r in records:
        original = HERE / r['source']
        assert sha(original)==r['generator_sha256']
        with Image.open(original) as im:
            mode='RGBA' if r['file'].endswith('.png') else 'RGB'
            im=im.convert(mode)
            im=ImageOps.fit(im, r['size'], method=Image.Resampling.LANCZOS)
            target=ART / r['file']; target.parent.mkdir(parents=True,exist_ok=True)
            if mode=='RGB': im.save(target, quality=85,subsampling=0,optimize=True,icc_profile=ICC)
            else: im.save(target,optimize=True,icc_profile=ICC)
    print(f'Exported {len(records)} generator assets, size/format only.')


if __name__=='__main__':main()
