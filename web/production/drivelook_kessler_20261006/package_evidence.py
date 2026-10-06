#!/usr/bin/env python3
"""Package completed Kessler capture evidence; never modify original captures.

Usage: python production/drivelook_kessler_20261006/package_evidence.py ROOT [--require-all]
Creates derived contact sheets, galleries, packaging.json and ROOT/index.html.
All PASS labels concern capture integrity and metadata, never visual/gameplay quality.
"""
from __future__ import annotations
import argparse
from datetime import datetime, timezone
import hashlib
import html
import importlib.util
import json
import math
from pathlib import Path
import sys
import textwrap
from urllib.parse import unquote, urlsplit
from PIL import Image, ImageChops, ImageDraw, ImageFont

QUALITIES=('low','high','ultra')
SIZES=((844,390),(1280,800))
FONT=Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf')
BG='#151a20'; FG='#dde4e8'; MUTED='#9caab5'


def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def font(n): return ImageFont.truetype(str(FONT),n) if FONT.is_file() else ImageFont.load_default()
def dump(path,obj): path.write_text(json.dumps(obj,indent=2,allow_nan=False)+'\n',encoding='utf-8')
def esc(value): return html.escape(str(value),quote=True)
def clock(value): return f'{int(value//3600):02d}:{int(value%3600//60):02d}:{value%60:04.1f}'
def fmt_xyz(values): return ','.join(f'{v:.1f}' for v in values)


def confined(folder,name):
    path=(folder/name).resolve()
    if path.parent!=folder.resolve() or path.suffix!='.png': raise ValueError(f'Unsafe capture path: {name}')
    return path


def console_helper(web):
    spec=importlib.util.spec_from_file_location('kessler_capture_console',web/'tools/drivelook.py')
    module=importlib.util.module_from_spec(spec);sys.modules[spec.name]=module;spec.loader.exec_module(module)
    return module.record_console


def validate(folder,manifest,quality,size,build,record_console):
    checks=[]
    def check(ok,name,detail=None):
        checks.append({'status':'PASS' if ok else 'FAIL','check':name,'detail':detail})
    check(manifest.get('status')=='CAPTURED' and not manifest.get('failures'),'completed capture without failures')
    trips=manifest.get('trips',[])
    check(len(trips)==1 and trips[0].get('id')=='kessler','exactly one Kessler trip')
    if len(trips)!=1: return checks,{},None
    trip=trips[0];shots=trip.get('shots',[])
    check(trip.get('status')=='CAPTURED','trip completed')
    check(manifest.get('interval_m')==40 and manifest.get('fps')==30,'40 metre schedule and 30 Hz physics')
    check(trip.get('route_placement_calls')==0 and trip.get('fixture_placement_calls')==3,'no route placement; three explicitly placed fixtures')
    check(trip.get('quality_requested')==quality,'requested quality')
    route=[s for s in shots if s.get('capture_phase')=='uninterrupted_route']
    check(bool(route),'route evidence present')
    thresholds=[t for s in route for t in s.get('interval_thresholds_m',[])]
    check(thresholds==list(range(40,961,40)),'all thresholds 40..960 once in order',thresholds)
    starts=[s for s in route if s['name']=='start_mile2_9'];ends=[s for s in route if s['name']=='end_mile3_5']
    check(len(starts)==1 and 2.9<=starts[0]['mile']<=2.91,'actual start crossing mile 2.9')
    check(len(ends)==1 and 3.5<=ends[0]['mile']<=3.51,'actual end crossing mile 3.5')
    check(trip.get('lot_start',{}).get('mile',99)<.01,'real chapter-six diner-lot start')
    check(all(s.get('held') and s.get('driving') and s.get('area')=='roswell' and s.get('chapter6_stage')=='drive' for s in route),'held chapter-six cab throughout route')
    check(all(b['mile']>=a['mile'] and b['simulation_elapsed_s']>=a['simulation_elapsed_s'] for a,b in zip(route,route[1:])),'route mile/time progression never regresses')
    check(all(0<=(s['mile']-2.9)*1609.34-t<=14 for s in route for t in s.get('interval_thresholds_m',[])),'threshold captures use actual nearby poses (maximum 14m overshoot)')
    sides=trip.get('side_capture_checks',[])
    names=[f'mile{mi:.2f}_left' for mi in (3.15,3.20,3.25)]
    check([s.get('name') for s in sides]==names and all(s.get('unchanged') and s.get('camera_restored') and s.get('yaw_offset_rad')==1.2 for s in sides),'three requested yaw +1.2 shots preserve truck/clock/camera')
    left=[s for s in route if s['name'] in names]
    check([s['name'] for s in left]==names and all(0<=s['mile']-mi<=.01 for s,mi in zip(left,(3.15,3.20,3.25))),'left captures at requested mile crossings')
    default='off' if quality=='low' else 'vhs'
    check(all(s.get('preset',{}).get('quality')==quality and s.get('quality')==quality and s.get('ultra_enabled')==(quality=='ultra') for s in shots),'effective quality and Ultra state on every image')
    check(all(s.get('preset',{}).get('picture')==default for s in route),'route uses player-default picture mode')
    check(all(s.get('post',{}).get('on')==(s.get('picture')=='vhs') and s.get('post',{}).get('ultra')==(quality=='ultra') for s in shots),'effective postprocessing follows each recorded picture/quality')
    check(all(s.get('render_state',{}).get('buffer_width')==size[0] and s.get('render_state',{}).get('buffer_height')==size[1] and s.get('render_state',{}).get('pixel_ratio')==1 for s in shots),'actual render buffers and DPR match requested screenshot size')
    if quality=='ultra': check(all(s.get('post',{}).get('supported') for s in shots),'Ultra supported in capture context')
    originals={folder/'manifest.json':sha(folder/'manifest.json')};hashes=[]
    for shot in shots:
        path=confined(folder,shot['file']);digest=sha(path)
        with Image.open(path) as image:
            dimensions=image.size;kind=image.format;image.verify()
        originals[path]=digest;hashes.append({'file':shot['file'],'sha256':digest,'size':list(dimensions)})
        check(digest==shot.get('image_sha256') and dimensions==size and kind=='PNG','lossless PNG hash/dimensions: '+shot['file'])
    check(len({s['file'] for s in shots})==len(shots),'unique image filenames')
    replay={'console_messages':[],'errors':[],'warnings':[]}
    for event in trip.get('console_messages',[]):
        record_console(replay,event['type'],event['text'],event.get('location',{}),page_url=manifest['url'])
    check(not trip.get('errors') and not replay['errors'],'strict console policy and no page errors',{'errors':trip.get('errors',[]),'replayed_errors':replay['errors'],'warnings':replay['warnings']})
    page=urlsplit(manifest['url']);bad=[]
    for event in trip.get('failed_http',[]):
        source=urlsplit(event['url'])
        if not (event['status']==404 and source.scheme in ('http','https') and source.path=='/favicon.ico' and (source.scheme,source.netloc)==(page.scheme,page.netloc)):
            bad.append(event)
    check(not bad,'no non-favicon HTTP failures',bad)
    capture_build=build['capture_build'];expected=capture_build['artifact_files_sha256'];base=urlsplit(capture_build['served_from'])
    resources=trip.get('loaded_code_sha256',{});resource_checks=[]
    for url,digest in resources.items():
        u=urlsplit(url);relative=unquote(u.path[len(base.path):]) if u.path.startswith(base.path) else None
        if relative=='': relative='index.html'
        ok=(u.scheme,u.netloc)==(base.scheme,base.netloc) and relative in expected and expected[relative]==digest
        resource_checks.append({'url':url,'artifact_file':relative,'captured_sha256':digest,'artifact_sha256':expected.get(relative),'matches':ok})
    check(bool(resource_checks) and all(r['matches'] for r in resource_checks),'all loaded document/script hashes match immutable Pages artifact',resource_checks)
    check(any(r['artifact_file'] and r['artifact_file'].startswith('assets/OldRoad-') for r in resource_checks),'integrated OldRoad chunk was loaded')
    check(all(s.get('source_git_head')==capture_build['main'] for s in shots),'image source baseline is recorded main',capture_build['main'])
    pairs=trip.get('flicker_pairs',[]);modes=['player_default'] if quality=='low' else ['player_default','picture_off_repeatability']
    expected_pairs={(distance,mode) for distance in (400,200,80) for mode in modes}
    check({(p['distance_before_gate_m'],p['mode']) for p in pairs}==expected_pairs and len(pairs)==len(expected_pairs),'all requested stationary fixture pairs captured')
    by_file={s['file']:s for s in shots}
    for pair in pairs:
        label=f'{pair["distance_before_gate_m"]}m/{pair["mode"]}';a,b=[by_file[name] for name in pair['files']]
        check(all(s['capture_phase']=='stationary_fixture' for s in (a,b)),'explicit fixture provenance '+label)
        check(pair.get('restoration',{}).get('frozen') and all(a.get(key)==b.get(key) for key in ('driver_xyz','world_xyz','heading_rad','camera_xyz','camera_quaternion','camera_projection','camera_world','truck_quaternion')),'pair pose and camera matrices restored exactly '+label)
        check(all(s.get('speed_mps')==0 and s.get('held') for s in (a,b)),'fixture truck remains stationary and RAF held '+label)
        check(a.get('landmark_materials')==b.get('landmark_materials') and a.get('render_state')==b.get('render_state'),'pair materials and render settings remain stable '+label)
        step=1/30 if pair['mode']=='player_default' else 0
        times=pair['vhs_render_times_s']
        check(pair['simulation_step_s']==step and math.isclose(times[1]-times[0],step,abs_tol=1e-8),'separate exact simulation/render timing '+label)
        check(all(s['picture']==(default if pair['mode']=='player_default' else 'off') for s in (a,b)),'pair picture mode '+label)
        with Image.open(confined(folder,a['file'])) as ai,Image.open(confined(folder,b['file'])) as bi:
            diff=ImageChops.difference(ai.convert('RGB'),bi.convert('RGB'))
            for name,region in pair['regions'].items():
                rect=region.get('rect')
                if region.get('status')=='UNVERIFIED': continue
                if rect is None: check(False,'valid measurable ROI '+label+'/'+name);continue
                valid=len(rect)==4 and all(isinstance(v,int) for v in rect) and 0<=rect[0]<rect[2]<=size[0] and 0<=rect[1]<rect[3]<=size[1] and rect[2]-rect[0]>=8 and rect[3]-rect[1]>=8
                check(valid,'measured component rectangle is valid '+label+'/'+name)
                if not valid: continue
                crop=diff.crop(tuple(rect));values=list(crop.get_flattened_data() if hasattr(crop,'get_flattened_data') else crop.getdata());maxima=sorted(max(rgb) for rgb in values);n=len(values)
                counts={str(t):sum(v>t for v in maxima) for t in (0,2,4,8)}
                fractions={str(t):counts[str(t)]/n for t in (0,2,4,8)}
                ok=n==region.get('pixel_count') and counts==region.get('changed_pixels') and all(math.isclose(value,region['changed_fraction'][key],abs_tol=1e-12) for key,value in fractions.items())
                check(ok,'recomputed component pixel thresholds '+label+'/'+name)
                mean=sum(sum(rgb) for rgb in values)/(n*3);percentiles={str(p):maxima[math.ceil(p*n)-1] for p in (.95,.99)}
                check(math.isclose(mean,region.get('mean_absolute_channel_delta',-1),abs_tol=1e-12) and maxima[-1]==region.get('max_channel_delta') and percentiles==region.get('percentiles_max_channel_delta'),'recomputed mean/max/percentiles '+label+'/'+name)
    return checks,originals,{'trip':trip,'route':route,'left':left,'image_hashes':hashes,'resource_hashes':resource_checks}


def fit(image,width,height):
    image=image.copy();image.thumbnail((width,height),Image.Resampling.LANCZOS)
    panel=Image.new('RGB',(width,height),BG);panel.paste(image,((width-image.width)//2,(height-image.height)//2));return panel


def image_copy(path):
    with Image.open(path) as image: return image.convert('RGB')


def caption(draw,xy,text,width,lines=4):
    for i,line in enumerate(textwrap.wrap(text,width=width)[:lines]): draw.text((xy[0],xy[1]+i*17),line,fill=FG,font=font(12))


def label(shot):
    direction='venstre +1,2 rad' if shot['name'].endswith('_left') else 'framover'
    if shot.get('capture_phase')=='stationary_fixture': direction='PLASSERT FIXTUR '+shot['fixture']['mode']
    return f'{shot["name"]} | mi {shot["mile"]:.4f} | {direction} | {clock(shot["clock_seconds"])} | map XYZ [{fmt_xyz(shot["map_xyz"])}] | {shot["quality"]}/{shot["picture"]}'


def contact(folder,shots,target,title,columns=3):
    width=360;height=225 if shots and shots[0]['viewport']['height']==800 else 166
    cellw=width+24;cellh=height+80;rows=max(1,math.ceil(len(shots)/columns))
    sheet=Image.new('RGB',(columns*cellw,rows*cellh+68),BG);draw=ImageDraw.Draw(sheet)
    draw.text((12,10),title,fill=FG,font=font(18));draw.text((12,36),'Deriverte miniatyrer. Originale tapsfrie PNG-er er bevart. Ingen automatisk visuell PASS.',fill=MUTED,font=font(12))
    for i,shot in enumerate(shots):
        x=(i%columns)*cellw+12;y=(i//columns)*cellh+60
        sheet.paste(fit(image_copy(confined(folder,shot['file'])),width,height),(x,y));caption(draw,(x,y+height+6),label(shot),48)
    sheet.save(target,format='PNG');return {'file':target.name,'width':sheet.width,'height':sheet.height,'kind':'contact_sheet','originals':[s['file'] for s in shots]}


def fixture_plate(folder,pair,target_dir):
    stem=f'fixture-{pair["distance_before_gate_m"]}m-{pair["mode"]}'
    a,b=[image_copy(confined(folder,name)) for name in pair['files']]
    diff=ImageChops.difference(a,b);raw=target_dir/(stem+'-raw-difference.png');diff.save(raw,format='PNG')
    amp=diff.point(lambda value:min(255,value*8))
    sheet=Image.new('RGB',(1152,720),BG);draw=ImageDraw.Draw(sheet)
    draw.text((12,10),f'PLASSERT FI XTUR: {pair["distance_before_gate_m"]} m før porten | {pair["mode"]}'.replace('FI XTUR','FIXTUR'),fill=FG,font=font(18))
    draw.text((12,34),'A / B / RGB-differanse x8. Komponentutsnitt under. Variasjon er ikke en z-fighting-dom.',fill=MUTED,font=font(13))
    draw.text((12,51),f'Simsteg {pair["simulation_step_s"]:.5f} s; eksplisitt render-tid {pair["vhs_render_times_s"][0]:.5f} / {pair["vhs_render_times_s"][1]:.5f}; veggintervall {pair["wall_interval_s"]:.3f} s.',fill=MUTED,font=font(11))
    for i,(name,image) in enumerate((('A',a),('B',b),('DIFF x8',amp))):
        x=i*384+12;sheet.paste(fit(image,360,225),(x,70));draw.text((x,301),name,fill=FG,font=font(15))
    for i,name in enumerate(('farm','gravel_track','gate_grid','name')):
        region=pair['regions'].get(name,{});x=i*288+12;rect=region.get('rect')
        draw.text((x,332),name,fill=FG,font=font(16))
        if rect:
            crop=amp.crop(tuple(rect));scale=min(264/crop.width,228/crop.height);crop=crop.resize((max(1,round(crop.width*scale)),max(1,round(crop.height*scale))),Image.Resampling.NEAREST)
            sheet.paste(crop,(x+(264-crop.width)//2,362+(228-crop.height)//2))
        if region.get('status')=='UNVERIFIED' or not rect:
            text='UNVERIFIED: utenfor bildet eller under 8x8 piksler.'
        else:
            fractions=region['changed_fraction'];text=f'N={region["pixel_count"]}; D>0: {fractions["0"]:.2%}; >2: {fractions["2"]:.2%}; >4: {fractions["4"]:.2%}; >8: {fractions["8"]:.2%}; max={region["max_channel_delta"]}'
        caption(draw,(x,604),text,36,5)
    plate=target_dir/(stem+'-plate.png');sheet.save(plate,format='PNG')
    return {'file':plate.name,'raw_difference':raw.name,'kind':'fixture_pair_component_differences','width':sheet.width,'height':sheet.height,'originals':pair['files']}


def gallery(folder,derived,data,config,checks):
    assets=[];front=[s for s in data['route'] if not s['name'].endswith('_left')]
    for offset in range(0,len(front),12): assets.append(contact(folder,front[offset:offset+12],derived/f'route-contact-{offset//12+1:02d}.png',config+f': ekte kjøring, framover {offset+1} til {min(offset+12,len(front))}'))
    assets.append(contact(folder,data['left'],derived/'left-contact.png',config+': tre venstreblikk under ekte kjøring'))
    fixtures=[s for s in data['trip']['shots'] if s['capture_phase']=='stationary_fixture']
    for offset in range(0,len(fixtures),12): assets.append(contact(folder,fixtures[offset:offset+12],derived/f'fixture-contact-{offset//12+1:02d}.png',config+': PLASSERTE FI XTURER, etter turen'.replace('FI XTURER','FIXTURER')))
    for pair in data['trip']['flicker_pairs']: assets.append(fixture_plate(folder,pair,derived))
    body=f'<h1>{esc(config)}</h1><p>Strukturell kontroll: {sum(c["status"]=="PASS" for c in checks)}/{len(checks)}. Visuell vurdering gjøres separat. Stillstandsfixturer er plassert etter den uavbrutte turen.</p><p><a href="../../index.html">Samlet oversikt</a> | <a href="../../{esc(config)}/manifest.json">Originalmanifest</a> | <a href="../../{esc(config)}/index.html">Originalt galleri</a> | <a href="packaging.json">Pakkekontroll</a></p>'
    for asset in assets:
        body+=f'<figure><a href="{esc(asset["file"])}"><img src="{esc(asset["file"])}" loading="lazy"></a><figcaption>{esc(asset["kind"])} | {asset["width"]}×{asset["height"]}'
        if 'raw_difference' in asset: body+=f' | <a href="{esc(asset["raw_difference"])}">Rå RGB-differanse, uten forsterkning</a>'
        body+='</figcaption></figure>'
    body+='<h2>Originalbilder fra ekte kjøring</h2>'
    for shot in data['route']:
        link=f'../../{config}/{shot["file"]}'
        body+=f'<figure><a href="{esc(link)}"><img src="{esc(link)}" loading="lazy"></a><figcaption>{esc(label(shot))}</figcaption></figure>'
    body+='<h2>Plasserte stillstandsfixturer</h2>'
    for shot in fixtures:
        link=f'../../{config}/{shot["file"]}'
        body+=f'<figure><a href="{esc(link)}"><img src="{esc(link)}" loading="lazy"></a><figcaption>{esc(label(shot))}</figcaption></figure>'
    (derived/'gallery.html').write_text(page(config,body),encoding='utf-8')
    return assets


def page(title,body):
    return '<!doctype html><html lang="nb"><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>'+esc(title)+'</title><style>body{background:'+BG+';color:'+FG+';font:16px system-ui;margin:24px auto;padding:0 20px;max-width:1400px}a{color:#a9cfe9}figure{margin:18px 0}img{max-width:100%;height:auto}figcaption{font-size:13px;color:'+MUTED+'}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:20px}.card{padding:14px;background:#222b33;border-radius:8px}.pending{color:#9ba6ad}code{overflow-wrap:anywhere}</style>'+body+'</html>'


def overview(root,entries):
    complete=[e for e in entries if e.get('integrity_status')=='PASS'];body=f'<h1>Kessler-strekningen, kapittel 6</h1><p>Dokumentert kjøring fra mile 2,9 til 3,5. {len(complete)} av 6 konfigurasjoner er ferdig fanget og pakket. Ingen automatisk visuell eller gameplay PASS.</p><p>'
    for name in ('review.md','review.json','build.json','packaging.json'):
        body+=f'<a href="{name}">{name}</a> ' if (root/name).is_file() else f'<span class="pending">{name}: kommer</span> '
    body+='</p><h2>Seks konfigurasjoner</h2><div class="grid">'
    for entry in entries:
        config=entry['config'];body+=f'<div class="card"><h3>{esc(config)}</h3><p>{esc(entry["status"])}</p>'
        if entry.get('integrity_status')=='PASS': body+=f'<p><a href="derived/{config}/gallery.html">Kontaktark, fixturpar og originalbilder</a> | <a href="{config}/manifest.json">Originalmanifest</a></p>'
        elif (root/config/'manifest.json').exists(): body+=f'<p><a href="{config}/manifest.json">Pågående/rått manifest</a></p>'
        body+='</div>'
    body+='</div><h2>Mile 3,20, venstre +1,2 rad</h2><p>Originale bilder. Low/High/Ultra per størrelse; forskjellig fremdrift og tid oppgis i bildeteksten.</p><div class="grid">'
    for entry in complete:
        shot=entry['left_3_20'];link=f'{entry["config"]}/{shot["file"]}'
        body+=f'<figure><a href="{esc(link)}"><img src="{esc(link)}" loading="lazy"></a><figcaption>{esc(entry["config"]+" | "+label(shot))}</figcaption></figure>'
    body+='</div><p>Skille i bevisene: ruten er kjørt med spillets autopilot uten plassering. Stillstandsfixturer er satt opp etter turen. RGB-variasjon inkluderer bildeeffekter og sceneendringer; null-differanse utelukker ikke bevegelsesavhengig z-fighting. Ekte telefon, manuell kjøring, lyd og maskinvare-fps er ikke verifisert.</p>'
    (root/'index.html').write_text(page('Kessler: kjøring og visuell vurdering',body),encoding='utf-8')


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('root',type=Path);p.add_argument('--require-all',action='store_true');args=p.parse_args()
    root=args.root.resolve();build=json.loads((root/'build.json').read_text());record_console=console_helper(root.parent.parent)
    derived_root=root/'derived';derived_root.mkdir(exist_ok=True)
    entries=[];failures=[]
    for w,h in SIZES:
        for quality in QUALITIES:
            config=f'{quality}_{w}x{h}';folder=root/config;path=folder/'manifest.json';entry={'config':config,'status':'PENDING'};entries.append(entry)
            if not path.exists(): continue
            try: manifest=json.loads(path.read_text())
            except json.JSONDecodeError: entry['status']='CAPTURING';continue
            if manifest.get('status')!='CAPTURED': entry['status']=manifest.get('status','PENDING');continue
            try:
                checks,originals,data=validate(folder,manifest,quality,(w,h),build,record_console)
                errors=[c for c in checks if c['status']=='FAIL'];entry['integrity_status']='FAIL' if errors else 'PASS'
                entry['status']='CAPTURED: metadata/integritet '+entry['integrity_status']
                entry['checks_passed']=sum(c['status']=='PASS' for c in checks);entry['checks_total']=len(checks)
                derived=derived_root/config;derived.mkdir(exist_ok=True)
                assets=gallery(folder,derived,data,config,checks) if not errors else []
                unchanged=all(sha(path)==digest for path,digest in originals.items())
                if not unchanged: raise RuntimeError('Original evidence changed during packaging')
                result={'config':config,'integrity_status':entry['integrity_status'],'visual_status':'UNVERIFIED; manual review is owned separately',
                        'manifest_sha256':originals[folder/'manifest.json'],'checks':checks,'originals_unchanged':unchanged,'derived_assets':assets}
                if data: result.update({'image_hashes':data['image_hashes'],'resource_hashes':data['resource_hashes']})
                dump(derived/'packaging.json',result)
                if errors: failures.append({'config':config,'failed_checks':errors})
                else: entry['left_3_20']=next(s for s in data['left'] if s['name']=='mile3.20_left')
                print(f'{config}: {entry["status"]}, {entry["checks_passed"]}/{entry["checks_total"]}',flush=True)
            except Exception as e:
                entry['status']='PACKAGING FAIL';entry['integrity_status']='FAIL';entry['failure']=f'{type(e).__name__}: {e}';failures.append({'config':config,'error':entry['failure']});print(config+': '+entry['failure'],flush=True)
    ready=sum(e.get('integrity_status')=='PASS' for e in entries)
    report={'checked_utc':datetime.now(timezone.utc).isoformat(),'status':'FAIL' if failures else 'PACKAGED' if ready==6 else 'PARTIAL',
            'completed_configs':ready,'expected_configs':6,'entries':entries,'failures':failures,
            'visual_status':'UNVERIFIED; this packer verifies evidence integrity only','build_sha256':sha(root/'build.json')}
    dump(root/'packaging.json',report);overview(root,entries)
    print(f'{report["status"]}: {ready}/6; {root / "index.html"}',flush=True)
    return 1 if failures or (args.require_all and ready!=6) else 0

if __name__=='__main__': raise SystemExit(main())
