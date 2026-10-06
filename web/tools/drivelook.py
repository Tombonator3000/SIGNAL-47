#!/usr/bin/env python3
"""Record uninterrupted real drives for visual review, with no route placement.

The Kessler trip adds explicitly placed stationary fixtures after its real drive.

python3 tools/drivelook.py OUTDIR [WxH]
S47_URL selects a build; S47_CHROMIUM optionally selects an installed browser.
Each trip uses a fresh context. Importing this module does not launch a browser.
Exit 0 means capture completed without blocking console errors, not a visual/gameplay
PASS. URL-confirmed missing favicon HTTP 404s remain in the recorded diagnostics.
"""
from __future__ import annotations

import argparse
import asyncio
from dataclasses import dataclass
from datetime import datetime, timezone
import hashlib
import html
import json
import math
import os
from pathlib import Path
import re
import signal
import subprocess
import time
from urllib.parse import urlsplit

WEB = Path(__file__).resolve().parents[1]
INTERVAL = 400.0
STEP = 0.5
FPS = 30


def viewport(value: str) -> tuple[int, int]:
    try:
        w, h = map(int, value.lower().split('x'))
        if not (320 <= w <= 4096 and 200 <= h <= 4096):
            raise ValueError
        return w, h
    except ValueError as e:
        raise argparse.ArgumentTypeError('Viewport must be WxH, 320..4096 by 200..4096.') from e


def parse_args(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('outdir', type=Path, help='New directory; existing paths are refused.')
    p.add_argument('size', nargs='?', type=viewport, default=(1280, 800), metavar='WxH')
    p.add_argument('--trip', choices=('all','saro_diner','diner_oldroad','kessler'), default='all',
                   help='Capture both original trips or one requested trip; kessler is a separate chapter-six review.')
    p.add_argument('--quality', choices=('low','high','ultra'), default='high',
                   help='Graphics preset for --trip kessler; original trips retain High/off.')
    p.add_argument('--timeout', type=float, default=2400, help='Whole-run wall seconds, default 2400.')
    a = p.parse_args(argv)
    if not math.isfinite(a.timeout) or a.timeout <= 0:
        p.error('--timeout must be finite and positive')
    return a


def capture_due(distance: float, next_at: float, interval: float = INTERVAL):
    """Return every crossed threshold and the next one, including large overshoots."""
    if not all(math.isfinite(v) for v in (distance, next_at, interval)) or interval <= 0 or next_at <= 0:
        raise ValueError('Invalid capture schedule')
    due = []
    while distance + 1e-9 >= next_at:
        due.append(next_at)
        next_at += interval
    return due, next_at


@dataclass
class StallMonitor:
    patience: float = 90.0
    improvement: float = 1.0
    best: float | None = None
    last_progress: float = 0.0

    def observe(self, elapsed: float, progress: float) -> bool:
        if not all(math.isfinite(v) for v in (elapsed, progress, self.patience, self.improvement)):
            raise ValueError('Non-finite stall input')
        if self.patience <= 0 or self.improvement <= 0 or elapsed < self.last_progress:
            raise ValueError('Invalid stall time/threshold')
        if self.best is None or progress >= self.best + self.improvement:
            self.best, self.last_progress = progress, elapsed
        return elapsed - self.last_progress >= self.patience


def path_increment(previous, current) -> float:
    """Horizontal metres on the shared road map; scene-origin handoffs add no distance."""
    if previous is None:
        return 0.0
    coords = [previous[0], previous[2], current[0], current[2]]
    if not all(math.isfinite(v) for v in coords):
        raise ValueError('Non-finite path coordinate')
    return math.hypot(current[0] - previous[0], current[2] - previous[2])


def code_fingerprint(resources: dict) -> str:
    return hashlib.sha256(json.dumps(resources, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def record_console(trip: dict, kind: str, text: str, location: dict, *, page_url: str):
    """Preserve every error/warning; exempt only a proven missing browser icon."""
    if kind not in ('error', 'warning'):
        return
    event = {'type': kind, 'text': text, 'location': location}
    trip['console_messages'].append(event)
    try:
        source = urlsplit(location.get('url', ''))
        expected = urlsplit(page_url)
        missing_icon = (kind == 'error' and source.scheme in ('http', 'https')
                        and bool(source.netloc) and source.path == '/favicon.ico'
                        and (source.scheme, source.netloc) == (expected.scheme, expected.netloc)
                        and re.fullmatch(r'Failed to load resource: the server responded with a status of 404 \([^)]*\)', text))
    except ValueError:
        missing_icon = False
    if missing_icon:
        event['non_blocking_reason'] = 'missing_favicon_http_404'
        trip['warnings'].append(f"Missing favicon (HTTP 404): {source.geturl()}")
    else:
        trip['errors' if kind == 'error' else 'warnings'].append(text)


def shot_metadata(state: dict, *, name: str, filename: str, reason: str, distance: float,
                  elapsed: float, thresholds: list, url: str, source_sha: str,
                  resources: dict, renderer: dict, size: tuple[int, int],
                  quality: str = 'high', picture: str = 'off') -> dict:
    """Assemble a JSON-safe record without browser/file effects or inventing missing fields."""
    result = {**state, 'name':name, 'file':filename, 'reason':reason,
              'interval_thresholds_m':thresholds, 'path_distance_m':distance,
              'simulation_elapsed_s':elapsed, 'url':url, 'source_git_head':source_sha,
              'build_fingerprint_sha256':code_fingerprint(resources), 'renderer':renderer,
              'preset':{'quality':quality,'picture':picture},
              'viewport':{'width':size[0],'height':size[1],'device_scale_factor':1}}
    json.dumps(result, allow_nan=False)
    return result


def source_info(kessler: bool = False) -> dict:
    def git(*args):
        return subprocess.check_output(['git', '-C', str(WEB), *args], text=True, timeout=10).strip()
    files = ['src/world/World.ts', 'src/drive/Drive.ts', 'src/drive/OldRoad.ts',
             'src/drive/legs.ts', 'src/story/Chapter6.ts', 'tools/drivelook.py']
    if kessler:
        files += ['src/drive/oldRoadLandmarks.ts', 'src/drive/oldRoadLayout.ts',
                  'src/drive/roadShape.ts', 'src/world/Sky.ts', 'src/world/glow.ts',
                  'src/core/quality.ts', 'src/core/ultra.ts', 'src/core/vhs.ts', 'src/main.ts']
    return {'git_head': git('rev-parse', 'HEAD'), 'git_status': git('status', '--short'),
            'files_sha256': {f: hashlib.sha256((WEB / f).read_bytes()).hexdigest() for f in files},
            'relationship_to_served_build': 'UNVERIFIED; local source is recorded separately from loaded build bytes.'}


def write_report(out: Path, report: dict):
    """Only called for the exclusively created output directory owned by this run."""
    (out / 'manifest.json').write_text(json.dumps(report, indent=2, allow_nan=False) + '\n', encoding='utf-8')
    cards = []
    for trip in report['trips']:
        cards.append(f'<h2>{html.escape(trip["id"])}: {html.escape(trip["status"])}</h2>')
        for s in trip['shots']:
            label = f'{s["name"]} | path {s["path_distance_m"]:.1f} m | actual mile {s["mile"]} | map XYZ {s["map_xyz"]} | clock {s["clock_seconds"]:.2f} | {s["area"]}/{s["leg"]}'
            cards.append(f'<figure><a href="{html.escape(s["file"], quote=True)}"><img loading="lazy" src="{html.escape(s["file"], quote=True)}"></a><figcaption>{html.escape(label)}</figcaption></figure>')
    body = '<!doctype html><meta charset="utf-8"><title>SIGNAL / 47 driving capture</title><style>body{background:#171b20;color:#eee;font:16px system-ui;margin:24px}figure{display:inline-block;width:min(620px,95vw);margin:12px}img{width:100%}figcaption{font-size:13px}a{color:#add}</style>'
    body += f'<h1>SIGNAL / 47: {html.escape(report["status"])}</h1><p>Visuell vurdering: UNVERIFIED. Captures are evidence for manual review; no automatic visual PASS. Software renderer does not verify hardware performance.</p><p><a href="manifest.json">Manifest</a></p>'
    body += '<p>' + html.escape('; '.join(report['failures'])) + '</p>' + ''.join(cards)
    (out / 'index.html').write_text(body + '\n', encoding='utf-8')


INIT = r"""(() => {
  // A fresh isolated context has no real saves. Clear only its own storage.
  localStorage.clear();
  HTMLElement.prototype.requestPointerLock = function() { return Promise.resolve(); };
})();"""

STATE = r"""(() => {
  const s=S47,w=s.world,L=w.leg&&w.legs[w.leg];
  // After parking the driver controller still supplies the truck's final pose.
  const D=L?.drive || (w.truckAt==='diner' ? w.drive : null);
  if(!D) throw Error('No current driver controller');
  const p=D.truck.group.position, origin=D.area.group?.position || p.clone().set(0,0,0);
  const road=w.road?.group.position || p.clone().set(8000,0,0);
  const map=w.area==='saro' ? [p.x+24,p.y,p.z-300] : [p.x-road.x,p.y-road.y,p.z-road.z];
  const old=w.area==='roswell' ? w.oldRoad.where(D.pos.x,D.pos.z) : null;
  const direction=s.camera.getWorldDirection(p.clone());
  return {world_xyz:p.toArray(),local_xyz:p.clone().sub(origin).toArray(),map_xyz:map,
    driver_xyz:D.pos.toArray(),camera_xyz:s.camera.position.toArray(),heading_rad:D.heading,
    view_direction:direction.toArray(),cab_yaw_rad:D.yaw,cab_pitch_rad:D.pitch,
    camera_role:w.driving?'driver_cab':w.area==='diner'?'player_after_parking':'arrival_transition',
    clock_seconds:s.game.clock,draw_calls:s.renderer.info.render.calls,area:w.area,leg:w.leg,
    driving:w.driving,truck_at:w.truckAt,mile:old?.mi??null,oldroad_s_m:old?.s??null,
    oldroad_distance_to_center_m:Number.isFinite(old?.d)?old.d:null,
    oldroad_distance_finite:old===null?null:Number.isFinite(old.d),
    road_local_z:map[2],speed_mps:D.speed,chapter6_stage:s.ch6.stage};
})()"""

LOOK = r"""angle => {
  const w=S47.world,D=w.legs[w.leg]?.drive,c=S47.camera;
  if(!D || !w.driving || !S47.hold || !S47.game.d.view.draw) throw Error('Missing held driving view API');
  const saved={position:c.position.toArray(),quaternion:c.quaternion.toArray(),
    driver:D.pos.toArray(),truck:D.truck.group.position.toArray(),heading:D.heading,clock:S47.game.clock};
  // Draw the current scene through the existing view API. tick(0) advances physics!
  c.rotateY(angle);c.updateMatrixWorld(true);S47.game.d.view.draw();
  return saved;
}"""

RESTORE_VIEW = r"""saved => {
  const w=S47.world,D=w.legs[w.leg]?.drive,c=S47.camera;
  if(!D) throw Error('Driver changed during a held side capture');
  c.position.fromArray(saved.position);c.quaternion.fromArray(saved.quaternion);
  c.updateMatrixWorld(true);S47.game.d.view.draw();
  return {unchanged:S47.game.clock===saved.clock && D.heading===saved.heading &&
    D.pos.toArray().every((v,i)=>v===saved.driver[i]) &&
    D.truck.group.position.toArray().every((v,i)=>v===saved.truck[i]),
    camera_restored:c.position.toArray().every((v,i)=>v===saved.position[i]) &&
    c.quaternion.toArray().every((v,i)=>v===saved.quaternion[i])};
}"""


KESSLER_MILE = 1609.34
KESSLER_START, KESSLER_END, KESSLER_GATE = 2.9, 3.5, 3.2

KESSLER_EXTRA = r"""(() => {
  const s=S47,D=s.world.oldDrive,g=s.world.oldRoad.group.getObjectByName('oldroad/Kessler');
  const materials=[];
  g?.traverse(m=>{if(m.isMesh) for(const a of (Array.isArray(m.material)?m.material:[m.material]))
    materials.push({mesh:m.name,material:a.name,uuid:a.uuid,type:a.type,map:a.map?.uuid??null});});
  const sky=Object.fromEntries(['uTime','uFlash','uDawn','uSun','uSunDir'].filter(k=>s.sky.uniforms[k])
    .map(k=>{const v=s.sky.uniforms[k].value;return [k,v?.toArray?v.toArray():v];}));
  return {camera_quaternion:s.camera.quaternion.toArray(),
    camera_projection:s.camera.projectionMatrix.toArray(),camera_world:s.camera.matrixWorld.toArray(),
    truck_quaternion:D.truck.group.quaternion.toArray(),
    quality:JSON.parse(localStorage.getItem('s47.quality')),
    picture:s.vhs.picture,ultra_enabled:s.vhs.ultra,held:s.hold,
    sky_dawn:s.sky.uniforms.uDawn.value,sky_sun:s.sky.uniforms.uSun.value,sky_uniforms:sky,
    post:{on:s.vhs.on,supported:s.vhs.supported,glitch:s.vhs.glitch,ultra:s.vhs.ultra},
    render_state:{pixel_ratio:s.renderer.getPixelRatio(),buffer_width:s.renderer.domElement.width,
      buffer_height:s.renderer.domElement.height,tone_mapping:s.renderer.toneMapping,
      exposure:s.renderer.toneMappingExposure,ultra_mapped:s.ultra.mapped},landmark_materials:materials};
})()"""

KESSLER_ROIS = r"""([width,height]) => {
  const c=S47.camera,g=S47.world.oldRoad.group.getObjectByName('oldroad/Kessler');
  if(!g) throw Error('Missing integrated oldroad/Kessler group');
  g.updateWorldMatrix(true,true);c.updateMatrixWorld(true);
  const bins={farm:[],gravel_track:[],gate_grid:[],name:[]};
  // The two meshes are merged. Partition their actual vertices in the gate's local
  // frame; no assumed screen rectangle, HUD or whole-scene comparison.
  g.traverse(m=>{
    if(!m.isMesh) return;
    const a=m.geometry.getAttribute('position');
    for(let i=0;i<a.count;i++) {
      const world=c.position.clone().fromBufferAttribute(a,i).applyMatrix4(m.matrixWorld);
      const p=g.worldToLocal(world.clone());
      const eye=world.clone().applyMatrix4(c.matrixWorldInverse);
      if(eye.z>=-c.near) continue;
      const q=world.clone().project(c);
      if(q.z < -1 || q.z > 1 || ![q.x,q.y].every(Number.isFinite)) continue;
      const xy=[(q.x+1)*width/2,(1-q.y)*height/2];
      if(m.name==='oldroad/Kessler-name') bins.name.push(xy);
      else if(p.z < -280 && Math.abs(p.x)<25) bins.farm.push(xy);
      else if(p.z>=-280 && p.z<-5 && Math.abs(p.x)<5) bins.gravel_track.push(xy);
      else if(Math.abs(p.z)<=1.35 && Math.abs(p.x)<3.1 &&
        world.y<S47.world.oldRoad.height(world.x,world.z)+0.3) bins.gate_grid.push(xy);
    }
  });
  return Object.fromEntries(Object.entries(bins).map(([name,points])=>{
    if(!points.length) return [name,{rect:null,projected_vertices:0,visible:false}];
    const xs=points.map(p=>p[0]),ys=points.map(p=>p[1]);
    const raw=[Math.floor(Math.min(...xs))-2,Math.floor(Math.min(...ys))-2,
      Math.ceil(Math.max(...xs))+2,Math.ceil(Math.max(...ys))+2];
    const rect=[Math.max(0,raw[0]),Math.max(0,raw[1]),Math.min(width,raw[2]),Math.min(height,raw[3])];
    const visible=rect[2]>rect[0] && rect[3]>rect[1];
    return [name,{rect:visible?rect:null,unclipped_rect:raw,projected_vertices:points.length,visible}];
  }));
}"""

KESSLER_FROZEN_DRAW = r"""({time,saved}) => {
  const s=S47,c=s.camera,D=s.world.oldDrive;
  if(!s.hold) throw Error('Fixture draw requires the held loop');
  const before={driver:D.pos.toArray(),truck:D.truck.group.position.toArray(),
    truck_q:D.truck.group.quaternion.toArray(),heading:D.heading,speed:D.speed,
    camera:c.position.toArray(),camera_q:c.quaternion.toArray(),clock:s.game.clock};
  if(saved) {
    D.pos.fromArray(saved.driver);D.heading=saved.heading;D.speed=saved.speed;
    D.truck.group.position.fromArray(saved.truck);D.truck.group.quaternion.fromArray(saved.truck_q);
    D.truck.group.updateMatrixWorld(true);
    c.position.fromArray(saved.camera);c.quaternion.fromArray(saved.camera_q);
  }
  c.updateMatrixWorld(true);
  // Normal draw still sets glowScale, culling and renderer counters. Override only
  // VHS render-time: wall-time screenshot latency is not the simulated 1/30 s.
  const render=s.vhs.render;
  s.vhs.render=function(scene,camera,_wallTime){return render.call(this,scene,camera,time);};
  try {s.game.d.view.draw();} finally {s.vhs.render=render;}
  return {before,saved:saved||before,render_time_s:time,clock:s.game.clock,
    frozen:saved ? JSON.stringify(D.pos.toArray())===JSON.stringify(saved.driver) &&
      JSON.stringify(D.truck.group.position.toArray())===JSON.stringify(saved.truck) &&
      JSON.stringify(D.truck.group.quaternion.toArray())===JSON.stringify(saved.truck_q) &&
      JSON.stringify(c.position.toArray())===JSON.stringify(saved.camera) &&
      JSON.stringify(c.quaternion.toArray())===JSON.stringify(saved.camera_q) &&
      D.heading===saved.heading && D.speed===saved.speed : true};
}"""


def pixel_change_metrics(first: Path, second: Path, regions: dict) -> dict:
    """Temporal RGB variation in projected component rectangles, never a visual PASS."""
    from PIL import Image, ImageChops
    with Image.open(first) as a, Image.open(second) as b:
        if a.size != b.size:
            raise ValueError('Flicker pair image sizes differ')
        diff = ImageChops.difference(a.convert('RGB'), b.convert('RGB'))
        result = {}
        for name, region in regions.items():
            rect = region['rect']
            if rect is None or rect[2]-rect[0]<8 or rect[3]-rect[1]<8:
                result[name] = {'status':'UNVERIFIED','reason':'Component outside viewport or below 8x8 pixels',**region}
                continue
            values = list(diff.crop(tuple(rect)).getdata())
            n = len(values)
            maxima = sorted(max(rgb) for rgb in values)
            result[name] = {**region,'pixel_count':n,
                'changed_pixels':{str(t):sum(v>t for v in maxima) for t in (0,2,4,8)},
                'changed_fraction':{str(t):sum(v>t for v in maxima)/n for t in (0,2,4,8)},
                'mean_absolute_channel_delta':sum(sum(rgb) for rgb in values)/(n*3),
                'max_channel_delta':maxima[-1],
                'percentiles_max_channel_delta':{str(p):maxima[math.ceil(p*n)-1] for p in (.95,.99)},
                'classification':'pixel_variation_only'}
        return result


async def run_kessler_trip(browser, args, report, url, out):
    """Drive uninterrupted first; separate, explicitly placed still fixtures follow."""
    trip = {'id':'kessler','status':'CAPTURING','visual_review':'UNVERIFIED','shots':[],
            'errors':[],'warnings':[],'console_messages':[],'failed_http':[],
            'loaded_code_sha256':{},'quality_requested':args.quality,'flicker_pairs':[],
            'route_placement_calls':0,'fixture_placement_calls':0,'side_capture_checks':[]}
    report['trips'].append(trip)
    context = await browser.new_context(viewport={'width':args.size[0],'height':args.size[1]},
                                        device_scale_factor=1,has_touch=False,is_mobile=False)
    page = await context.new_page();page.set_default_timeout(60000)
    tasks = [];previous = None;distance = 0.0;elapsed = 0.0
    async def response_body(response):
        if response.request.resource_type in ('document','script'):
            try: trip['loaded_code_sha256'][response.url] = hashlib.sha256(await response.body()).hexdigest()
            except Exception as e: trip['warnings'].append(f'Could not hash loaded code: {response.url}: {e}')
    def response_seen(response):
        if response.status>=400:
            trip['failed_http'].append({'url':response.url,'status':response.status,'resource_type':response.request.resource_type})
        tasks.append(asyncio.create_task(response_body(response)))
    page.on('response',response_seen)
    page.on('console',lambda m:record_console(trip,m.type,m.text,m.location,page_url=page.url))
    page.on('pageerror',lambda e:trip['errors'].append('PAGEERROR: '+str(e)))
    async def state():
        nonlocal previous,distance
        st=await page.evaluate(STATE);st.update(await page.evaluate(KESSLER_EXTRA))
        distance+=path_increment(previous,st['map_xyz']);previous=st['map_xyz']
        return st
    async def tick(seconds=STEP):
        nonlocal elapsed
        await page.evaluate('([s,f])=>S47.tick(s,f)',[seconds,FPS]);elapsed+=max(1,round(seconds*FPS))/FPS
        return await state()
    async def shot(name,reason,thresholds=None,fixture=None):
        if tasks: await asyncio.gather(*tasks)
        st=await state();filename=f'kessler_{len(trip["shots"]):03d}_{name}.png'
        await page.screenshot(path=str(out/filename),timeout=120000)
        st=shot_metadata(st,name=name,filename=filename,reason=reason,thresholds=thresholds or [],
            distance=distance,elapsed=elapsed,url=page.url,source_sha=report['source']['git_head'],
            resources=trip['loaded_code_sha256'],renderer=trip['renderer'],size=args.size,
            quality=st['quality'],picture=st['picture'])
        st['image_sha256']=hashlib.sha256((out/filename).read_bytes()).hexdigest()
        st['capture_phase']='stationary_fixture' if fixture else 'uninterrupted_route'
        if fixture: st['fixture']=fixture
        trip['shots'].append(st);write_report(out,report)
        print(f'kessler/{args.quality}: {name}, mile={st["mile"]:.5f}, picture={st["picture"]}',flush=True)
        return st
    async def left(name,reason):
        saved=await page.evaluate(LOOK,1.2)
        try: return await shot(name,reason)
        finally:
            proof=await page.evaluate(RESTORE_VIEW,saved)
            trip['side_capture_checks'].append({'name':name,'yaw_offset_rad':1.2,**proof})
            if not proof['unchanged'] or not proof['camera_restored']:
                raise RuntimeError('Kessler left capture failed pose/clock restoration')
    try:
        await page.add_init_script(INIT+f"\nlocalStorage.setItem('s47.quality', JSON.stringify({json.dumps(args.quality)}));")
        await page.goto(url,wait_until='load',timeout=120000)
        await page.wait_for_function('window.S47 && S47.tick && S47.world')
        await page.click('button[data-a=start]');await page.wait_for_function('S47.started()')
        await page.evaluate("S47.hold=true;S47.jump('chapter6')")
        for _ in range(240):
            await page.evaluate('S47.hold=true;S47.tick(0.25)')
            if await page.evaluate("S47.world.area==='roswell' && S47.world.driving && S47.world.leg==='roswell' && !S47.world.busy && S47.ch6.stage==='drive'"): break
            await page.wait_for_timeout(100)
        else: raise RuntimeError('Kessler setup did not reach the real chapter-six cab')
        await page.evaluate('S47.hold=true;S47.world.autopilot=20')
        st=await tick(1/FPS)
        if st['quality']!=args.quality or st['picture']!=('off' if args.quality=='low' else 'vhs'):
            raise RuntimeError(f'Requested preset not applied: {st["quality"]}/{st["picture"]}')
        if st['ultra_enabled']!=(args.quality=='ultra'):
            raise RuntimeError('Effective Ultra state differs from the requested preset')
        if args.quality=='ultra' and not st['post']['supported']:
            raise RuntimeError('Ultra postprocessing is unsupported by this browser context')
        trip['renderer']=await page.evaluate("""(() => {const g=S47.renderer.getContext(),e=g.getExtension('WEBGL_debug_renderer_info');return {version:g.getParameter(g.VERSION),vendor:g.getParameter(e?e.UNMASKED_VENDOR_WEBGL:g.VENDOR),renderer:g.getParameter(e?e.UNMASKED_RENDERER_WEBGL:g.RENDERER),pixel_ratio:S47.renderer.getPixelRatio(),hardware_performance:'UNVERIFIED'};})()""")
        trip['lot_start']=st;monitor=StallMonitor();started=False;next_at=40.0;seen=set()
        for _ in range(2600):
            if not st['held'] or not st['driving'] or st['chapter6_stage']!='drive':
                raise RuntimeError('Uninterrupted Kessler route lost the held chapter-six cab')
            mi=st['mile'];relative=(mi-KESSLER_START)*KESSLER_MILE if mi is not None else -1
            if relative>=0 and not started:
                started=True;trip['capture_start']=st
                await shot('start_mile2_9','First actual autopilot pose crossing mile 2.9; no placement')
            if started:
                due,next_at=capture_due(relative,next_at,40.0)
                if due: await shot(f'forward_{int(due[-1]):04d}m','40 m old-road distance threshold; actual pose, no interpolation',due)
                for target in (3.15,3.20,3.25):
                    if mi>=target and target not in seen:
                        seen.add(target);await left(f'mile{target:.2f}_left','Held current driver camera yaw +1.2; no simulation tick')
                if mi>=KESSLER_END:
                    await shot('end_mile3_5','First actual autopilot pose crossing mile 3.5; no placement')
                    trip['route_end']=st;break
            progress=mi*KESSLER_MILE if mi is not None else -math.hypot(st['map_xyz'][0]+9.05,st['road_local_z']-2005.95)
            if monitor.observe(elapsed,progress): raise RuntimeError('Kessler autopilot stalled before mile 3.5')
            # Same 30 Hz physics, fewer renders during the warm-up drive. Stop the
            # batching comfortably before the reviewed stretch; no route placement.
            st=await tick(4.0 if mi is not None and mi<2.8 else STEP)
        else: raise RuntimeError('Kessler route exhausted its bounded steps')
        if seen!={3.15,3.20,3.25}: raise RuntimeError('Kessler route missed requested left views')
        await page.evaluate('S47.world.autopilot=null')
        # These explicitly placed fixtures occur only AFTER the uninterrupted route.
        # drive.place resets speed to zero; no manual clock changes or route teleport.
        for before_gate in (400,200,80):
            mile=KESSLER_GATE-before_gate/KESSLER_MILE
            await page.evaluate("""mi=>{const w=S47.world,p=w.oldRoad.poseAt(mi);w.autopilot=null;w.testInput=null;w.oldDrive.place(p.pos,p.heading);}""",mile)
            trip['fixture_placement_calls']+=1;previous=None
            await tick(1/FPS);await page.evaluate(LOOK,1.2)
            default_picture=await page.evaluate('S47.vhs.picture')
            fixture={'placement':'explicit drive.place after completed route','requested_mile':mile,
                     'distance_before_gate_m':before_gate,'yaw_offset_rad':1.2}
            for mode in ('player_default','picture_off_repeatability'):
                if mode=='picture_off_repeatability' and default_picture=='off': continue
                if mode=='picture_off_repeatability': await page.evaluate("S47.setPicture('off')")
                t0=await page.evaluate('performance.now()/1000')
                for _warm in range(2):
                    await page.evaluate(KESSLER_FROZEN_DRAW,{'time':t0,'saved':None})
                first_draw=await page.evaluate(KESSLER_FROZEN_DRAW,{'time':t0,'saved':None})
                rois=await page.evaluate(KESSLER_ROIS,list(args.size))
                a=await shot(f'fixture_{before_gate}m_{mode}_a','Stationary component variation baseline; fixed VHS render-time',fixture={**fixture,'mode':mode,'render_time_s':t0})
                wall_start=time.monotonic();pre_step=await state()
                step=1/FPS if mode=='player_default' else 0
                if step: await tick(step)
                post_step=await state()
                proof=await page.evaluate(KESSLER_FROZEN_DRAW,{'time':t0+step,'saved':first_draw['saved']})
                reason='One actual 1/30 s step, original camera/truck restored; VHS time +1/30 s' if step else 'No simulation tick; identical pose and render-time, picture off repeatability'
                b=await shot(f'fixture_{before_gate}m_{mode}_b',reason,fixture={**fixture,'mode':mode,'render_time_s':t0+step})
                pair={**fixture,'mode':mode,'files':[a['file'],b['file']],
                      'simulation_step_s':step,'vhs_render_times_s':[t0,t0+step],
                      'wall_interval_s':time.monotonic()-wall_start,'before_step':pre_step,'after_step_before_restore':post_step,
                      'restoration':proof,'regions':pixel_change_metrics(out/a['file'],out/b['file'],rois),
                      'interpretation':'Temporal variation includes scene/sky/light changes and VHS; the no-tick off pair measures static repeatability. Zero variation cannot rule out movement-dependent z-fighting.'}
                trip['flicker_pairs'].append(pair);write_report(out,report)
                if not proof['frozen']: raise RuntimeError('Fixture camera/truck was not restored exactly')
            if default_picture!='off': await page.evaluate('p=>S47.setPicture(p)',default_picture)
        if trip['errors']: raise RuntimeError(f'{len(trip["errors"])} console/page errors; see manifest')
        expected=urlsplit(page.url)
        bad_http=[]
        for event in trip['failed_http']:
            source=urlsplit(event['url'])
            if not (event['status']==404 and source.path=='/favicon.ico' and
                    source.scheme in ('http','https') and
                    (source.scheme,source.netloc)==(expected.scheme,expected.netloc)):
                bad_http.append(event)
        if bad_http: raise RuntimeError(f'{len(bad_http)} non-favicon HTTP failures; see manifest')
        trip['status']='CAPTURED'
    except BaseException as e:
        trip['status']='FAIL';trip['failure']=f'{type(e).__name__}: {e}';raise
    finally:
        trip['path_distance_m']=distance;trip['simulation_elapsed_s']=elapsed
        write_report(out,report);await asyncio.wait_for(context.close(),timeout=10)


async def run_trip(browser, args, report, trip_id, url, out):
    if trip_id == 'kessler':
        return await run_kessler_trip(browser, args, report, url, out)
    trip = {'id': trip_id, 'status': 'CAPTURING', 'visual_review': 'UNVERIFIED',
            'shots': [], 'errors': [], 'warnings': [], 'console_messages': [],
            'failed_http': [], 'loaded_code_sha256': {}}
    report['trips'].append(trip)
    context = await browser.new_context(viewport={'width': args.size[0], 'height': args.size[1]}, device_scale_factor=1)
    page = await context.new_page()
    page.set_default_timeout(60000)
    tasks = []
    async def record_code(response):
        kind = response.request.resource_type
        if kind not in ('document', 'script'):
            return
        try:
            trip['loaded_code_sha256'][response.url] = hashlib.sha256(await response.body()).hexdigest()
        except Exception as e:
            trip['warnings'].append(f'Could not hash loaded {kind}: {response.url}: {e}')
    def on_response(response):
        if response.status >= 400:
            trip['failed_http'].append({'url':response.url,'status':response.status,
                                        'resource_type':response.request.resource_type})
        tasks.append(asyncio.create_task(record_code(response)))
    def on_console(message):
        record_console(trip, message.type, message.text, message.location, page_url=page.url)
    page.on('response', on_response)
    page.on('console', on_console)
    page.on('pageerror', lambda e: trip['errors'].append('PAGEERROR: ' + str(e)))
    previous = None
    distance = 0.0
    elapsed = 0.0
    async def state():
        nonlocal previous, distance
        st = await page.evaluate(STATE)
        distance += path_increment(previous, st['map_xyz'])
        previous = st['map_xyz']
        return st
    async def tick(seconds=STEP):
        nonlocal elapsed
        await page.evaluate('([s,f])=>S47.tick(s,f)', [seconds, FPS])
        elapsed += max(1, round(seconds * FPS)) / FPS
        return await state()
    async def shot(name, reason, thresholds=None):
        if tasks:
            await asyncio.gather(*tasks)
        st = await state()
        filename = f'{trip_id}_{len(trip["shots"]):03d}_{name}.png'
        await page.screenshot(path=str(out / filename), timeout=120000)
        st = shot_metadata(st, name=name, filename=filename, reason=reason,
                  thresholds=thresholds or [], distance=distance, elapsed=elapsed, url=page.url,
                  source_sha=report['source']['git_head'], resources=trip['loaded_code_sha256'],
                  renderer=trip['renderer'], size=args.size)
        st['image_sha256'] = hashlib.sha256((out / filename).read_bytes()).hexdigest()
        trip['shots'].append(st)
        write_report(out, report)
        print(f'{trip_id}: {name}, {distance:.0f} m, {st["area"]}, mile={st["mile"]}', flush=True)
    async def sides(name):
        for suffix, yaw in [('left', math.pi/2), ('right', -math.pi/2)]:
            saved = await page.evaluate(LOOK, yaw)
            try:
                await shot(name + '_' + suffix, 'held driver camera rotated 90 degrees; existing view.draw, no tick')
            finally:
                proof = await page.evaluate(RESTORE_VIEW, saved)
                trip.setdefault('side_capture_checks', []).append({'name':name+'_'+suffix, **proof})
                if not proof['unchanged'] or not proof['camera_restored']:
                    raise RuntimeError('Side capture changed truck/clock or failed to restore the driver camera')
    try:
        await page.add_init_script(INIT)
        await page.goto(url, wait_until='load', timeout=120000)
        await page.wait_for_function('window.S47 && S47.tick && S47.world')
        await page.click('button[data-a=start]')
        await page.wait_for_function('S47.started()')
        await page.evaluate("S47.hold=true; S47.setQuality('high'); S47.setPicture('off'); S47.tick(0.3)")
        if trip_id == 'saro_diner':
            await page.evaluate("S47.jump('chapter5'); S47.world.placeTruck('saro'); S47.world.driveToDiner(()=>{})")
        else:
            await page.evaluate("S47.jump('chapter6')")
        # Promise/asset loading needs real event-loop time, not just accelerated game time.
        for _ in range(240):
            await page.evaluate('S47.tick(0.25)')
            if await page.evaluate('S47.world.driving && S47.world.leg===S47.world.area && !S47.world.busy'): break
            await page.wait_for_timeout(100)
        else:
            raise RuntimeError('Setup did not reach a driving cab within its bounded wait')
        await page.evaluate('S47.world.autopilot=20')
        trip['renderer'] = await page.evaluate("""(() => { const g=S47.renderer.getContext(),e=g.getExtension('WEBGL_debug_renderer_info'); return {version:g.getParameter(g.VERSION),vendor:g.getParameter(e?e.UNMASKED_VENDOR_WEBGL:g.VENDOR),renderer:g.getParameter(e?e.UNMASKED_RENDERER_WEBGL:g.RENDERER),pixel_ratio:S47.renderer.getPixelRatio(),hardware_performance:'UNVERIFIED'}; })()""")
        st = await state()
        trip['start_clock_seconds'] = st['clock_seconds']
        await tick(1/FPS)  # draw the now-loaded driving scene after startTrip's promise
        await shot('start', 'first driving pose')
        if trip_id == 'diner_oldroad': await sides('diner_lot')
        next_at = INTERVAL
        monitor = StallMonitor()
        seen = set()
        for _ in range(2600):
            st = await tick()
            # park() clears driving/leg immediately; enter('diner') and getOut() run
            # after 0.4 s. w.drive is still the same road truck's final controller.
            if trip_id=='saro_diner' and not st['driving'] and st['truck_at']=='diner':
                for _settle in range(20):
                    if st['area']=='diner': break
                    st = await tick(0.1)
                if st['area']!='diner':
                    raise RuntimeError('Parked at diner but bounded arrival transition did not finish')
            complete = (not st['driving'] and st['area']=='diner' and st['truck_at']=='diner') if trip_id=='saro_diner' else st['mile'] is not None and st['mile'] > 8.1
            if complete:
                await page.evaluate('S47.world.autopilot=null')
                await shot('end', 'destination reached' if trip_id=='saro_diner' else 'past mile 8.1, before THE EVENT')
                trip['status'] = 'CAPTURED'
                break
            if not st['driving']:
                raise RuntimeError('Trip stopped before its requested endpoint')
            due, next_at = capture_due(distance, next_at)
            if due: await shot(f'forward_{int(due[-1]):05d}m', '400 m accumulated-path interval', due)
            z, mi = st['road_local_z'], st['mile']
            landmarks = []
            if trip_id=='saro_diner' and st['leg']=='road':
                if z >= 500: landmarks.append('survey_junction_fence_opening')
                if z >= 1945: landmarks.append('diner_approach_fence_opening')
                if z >= 1997: landmarks.append('diner_lot')
            if trip_id=='diner_oldroad' and mi is not None:
                if st['map_xyz'][0] >= -1 and mi < .02:
                    landmarks.append('highway_oldroad_crossroads')
                if mi >= .025: landmarks.append('oldroad_fence_opening')
                for mile in range(1,9):
                    if mi >= mile - .012: landmarks.append(f'milepost_{mile}')
            for landmark in landmarks:
                if landmark not in seen:
                    seen.add(landmark)
                    await shot(landmark+'_forward', 'landmark at actual recorded pose; no placement or threshold interpolation')
                    await sides(landmark)
            # Progress towards the goal catches both motionless trucks and circles/recovery loops.
            progress = mi * 1609.34 if mi is not None else -math.hypot(st['map_xyz'][0]+9.05, z-2005.95)
            if monitor.observe(elapsed, progress):
                raise RuntimeError(f'Stalled trip: no 1 m net goal progress for {monitor.patience:g} simulated seconds')
        else:
            raise RuntimeError('Trip exhausted 2600 bounded steps before its endpoint')
        if trip['errors']:
            raise RuntimeError(f'{len(trip["errors"])} console/page errors; see manifest')
    except BaseException as e:
        trip['status'] = 'FAIL'
        trip['failure'] = f'{type(e).__name__}: {e}'
        raise
    finally:
        trip['path_distance_m'] = distance
        trip['simulation_elapsed_s'] = elapsed
        write_report(out, report)
        await asyncio.wait_for(context.close(), timeout=10)


async def capture(args, report, out):
    from playwright.async_api import async_playwright
    url = os.environ.get('S47_URL') or (WEB / 'dist-single/index.html').as_uri()
    report['url'] = url
    opts = {'args':['--use-angle=swiftshader','--enable-unsafe-swiftshader','--ignore-gpu-blocklist','--autoplay-policy=no-user-gesture-required']}
    if os.environ.get('S47_CHROMIUM'): opts['executable_path'] = os.environ['S47_CHROMIUM']
    async with async_playwright() as p:
        browser = await p.chromium.launch(**opts)
        try:
            for trip_id in report['requested_trips']:
                try:
                    await run_trip(browser, args, report, trip_id, url, out)
                except Exception as e:
                    report['failures'].append(f'{trip_id}: {type(e).__name__}: {e}')
                    write_report(out, report)
        finally:
            await asyncio.wait_for(browser.close(), timeout=10)


def main(argv=None) -> int:
    args = parse_args(argv)
    out = args.outdir.expanduser().resolve()
    try:
        out.mkdir(parents=True, exist_ok=False)
    except OSError as e:
        print(f'FAIL output directory: {e}', flush=True)
        return 2
    report = {'schema_version':1,'status':'CAPTURING','visual_review':'UNVERIFIED',
              'created_utc':datetime.now(timezone.utc).isoformat(),'trips':[],'failures':[],
              'requested_trips':['saro_diner','diner_oldroad'] if args.trip=='all' else [args.trip],
              'url':os.environ.get('S47_URL') or (WEB / 'dist-single/index.html').as_uri(),
              'interval_m':INTERVAL,'step_s':STEP,'fps':FPS,'autopilot_mps':20,
              'wall_timeout_s':args.timeout,'limitations':['Autopilot capture, not manual controls or hardware fps verification.',
              'Only same-origin, URL-confirmed /favicon.ico HTTP 404 console messages are non-blocking; raw diagnostics are retained.',
              'Side shots use held camera rotation and view.draw without ticking; truck/clock preservation is checked.',
              'No automatic visual PASS; source/build byte equivalence is UNVERIFIED.']}
    if args.trip=='kessler':
        report['interval_m']=40.0
        report['quality_requested']=args.quality
        report['kessler']={'start_mile':KESSLER_START,'end_mile':KESSLER_END,'gate_mile':KESSLER_GATE,
            'left_view_miles':[3.15,3.20,3.25],'left_yaw_offset_rad':1.2,
            'warmup_tick_s':4.0,'warmup_until_mile':2.8,'capture_tick_s':STEP,
            'fixture_distances_before_gate_m':[400,200,80],
            'fixture_semantics':'Explicit drive.place at zero speed after the uninterrupted route; no clock assignment.',
            'post_picture':'Player default: Low off, High/Ultra VHS. Off repeatability pairs are diagnostics only.'}
        report['limitations'] += ['Stationary fixture placement is not route or manual-driving evidence.',
            'Component rectangles include background and subpixel projections may be UNVERIFIED.',
            'Temporal RGB variation is not an automatic z-fighting verdict; no-tick off pairs measure only repeatability.',
            'Phone-sized viewport uses a desktop pointer context so Ultra remains testable; no physical phone/fps claim.']
    started = time.monotonic()
    def terminated(_signum, _frame):
        raise KeyboardInterrupt('SIGTERM: capture interrupted; partial evidence retained')
    previous_sigterm = signal.signal(signal.SIGTERM, terminated)
    try:
        report['source'] = source_info(args.trip == 'kessler')
        write_report(out, report)
        asyncio.run(asyncio.wait_for(capture(args, report, out), timeout=args.timeout))
        expected = set(report['requested_trips'])
        actual = {t['id'] for t in report['trips']}
        if actual != expected or len(report['trips'])!=len(expected) or any(t['status'] != 'CAPTURED' for t in report['trips']):
            if not report['failures']: report['failures'].append('Requested trip captures did not complete')
    except (Exception, KeyboardInterrupt) as e:
        report['failures'].append(f'{type(e).__name__}: {e}')
    finally:
        signal.signal(signal.SIGTERM, previous_sigterm)
        report['wall_elapsed_s'] = time.monotonic()-started
        report['status'] = 'FAIL' if report['failures'] else 'CAPTURED'
    try:
        write_report(out, report)
    except OSError as e:
        print(f'FAIL writing partial report: {e}', flush=True)
        return 2
    print(f'{report["status"]}: {out / "index.html"}; visual review UNVERIFIED', flush=True)
    return 1 if report['failures'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
