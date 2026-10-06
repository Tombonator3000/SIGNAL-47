#!/usr/bin/env python3
"""Record uninterrupted real drives for visual review, with no route placement.

The Kessler trip adds explicitly placed stationary fixtures after its real drive.
The dinerlight trip records chapter-five arrival and chapter-six departure in
separate contexts, plus explicitly labelled Ultra on-foot lighting diagnostics.

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
    p.add_argument('--trip', choices=('all','saro_diner','diner_oldroad','kessler','dinerlight'), default='all',
                   help='Original trips, Kessler, or separate diner arrival/departure lighting reviews.')
    p.add_argument('--quality', choices=('low','high','ultra'), default='high',
                   help='Preset for kessler/dinerlight; original trips retain High/off.')
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


def code_hash_errors(trip: dict, *, require_closed: bool = False) -> list[str]:
    """Reject incomplete code evidence, including warnings in historical captures."""
    errors = [w for w in trip.get('warnings', []) if w.startswith('Could not hash loaded')]
    policy = trip.get('code_hash_policy')
    if 'code_hash_policy' in trip and policy != 'observed_responses_v1':
        errors.append(f'Unknown code hash policy: {policy}')
    observed = trip.get('observed_code_responses')
    if policy == 'observed_responses_v1' and not observed:
        errors.append('No observed document/script responses')
    if policy == 'observed_responses_v1' and require_closed and trip.get('code_observation_closed') is not True:
        errors.append('Code response observation did not finish with the closed context')
    if observed is not None:
        if not isinstance(observed, list) or any(not isinstance(r, dict) for r in observed):
            return errors + ['Malformed observed code response list']
        resources = trip.get('loaded_code_sha256', {})
        for response in observed:
            digest = response.get('sha256', '')
            if (response.get('hash_state') != 'hashed' or response.get('error')
                    or not isinstance(digest, str) or not re.fullmatch(r'[0-9a-f]{64}', digest)
                    or response.get('resource_type') not in ('document', 'script')
                    or resources.get(response.get('url')) != digest):
                errors.append(f'Incomplete or inconsistent code hash: {response.get("url")}')
        if {r.get('url') for r in observed} != set(resources):
            errors.append('Observed responses and code hash registry differ')
    return errors


def observe_response(trip: dict, response, tasks: list):
    """Record code responses synchronously, before their asynchronous body read."""
    if trip.get('code_observation_sealed'): return
    kind = response.request.resource_type
    if response.status >= 400:
        trip['failed_http'].append({'url': response.url, 'status': response.status,
                                    'resource_type': kind})
    if kind not in ('document', 'script'):
        return
    observed = {'url': response.url, 'resource_type': kind, 'status': response.status,
                'hash_state': 'pending'}
    trip['observed_code_responses'].append(observed)

    async def read_body():
        try:
            digest = hashlib.sha256(await response.body()).hexdigest()
            observed['sha256'] = digest
            observed['hash_state'] = 'hashed'
            trip['loaded_code_sha256'][response.url] = digest
        except asyncio.CancelledError:
            observed['hash_state'] = 'cancelled'
            observed['error'] = 'Code body read cancelled'
            trip['errors'].append(f'Could not hash loaded {kind}: {response.url}: cancelled')
            raise
        except Exception as e:
            observed['hash_state'] = 'failed'
            observed['error'] = f'{type(e).__name__}: {e}'
            trip['errors'].append(f'Could not hash loaded {kind}: {response.url}: {e}')
    tasks.append(asyncio.create_task(read_body()))


async def wait_code_hashes(trip: dict, tasks: list):
    # A response callback can append more work while a body read is awaited.
    cursor = 0
    try:
        while cursor < len(tasks):
            batch = tasks[cursor:]
            cursor += len(batch)
            await asyncio.gather(*batch)
    except BaseException:
        await cancel_code_hashes(trip, tasks)
        raise
    errors = code_hash_errors(trip)
    if errors:
        raise RuntimeError('; '.join(errors))


async def cancel_code_hashes(trip: dict, tasks: list):
    trip['code_observation_sealed'] = True
    for task in tasks:
        if not task.done(): task.cancel()
    if tasks: await asyncio.gather(*tasks, return_exceptions=True)
    for response in trip['observed_code_responses']:
        if response['hash_state'] == 'pending':
            response['hash_state'] = 'cancelled'
            response['error'] = 'Capture ended before body read completed'


def check_capture_diagnostics(trip: dict, page_url: str, *, strict_http: bool = False):
    if trip['errors']:
        raise RuntimeError(f'{len(trip["errors"])} console/page/code errors; see manifest')
    if strict_http:
        expected = urlsplit(page_url)
        bad = []
        for event in trip['failed_http']:
            source = urlsplit(event['url'])
            if not (event['status'] == 404 and source.path == '/favicon.ico'
                    and source.scheme in ('http', 'https')
                    and (source.scheme, source.netloc) == (expected.scheme, expected.netloc)):
                bad.append(event)
        if bad: raise RuntimeError(f'{len(bad)} non-favicon HTTP failures; see manifest')


def observe_page(page, trip: dict, tasks: list):
    handlers = {'response': lambda response: observe_response(trip, response, tasks),
                'console': lambda m: record_console(trip, m.type, m.text, m.location, page_url=page.url),
                'pageerror': lambda e: trip['errors'].append('PAGEERROR: ' + str(e))}
    for kind, callback in handlers.items(): page.on(kind, callback)
    return handlers


def stop_observing(page, trip: dict, handlers: dict):
    for kind, callback in handlers.items(): page.remove_listener(kind, callback)
    trip['code_observation_sealed'] = True


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


def source_info(kessler: bool = False, dinerlight: bool = False) -> dict:
    def git(*args):
        return subprocess.check_output(['git', '-C', str(WEB), *args], text=True, timeout=10).strip()
    files = ['src/world/World.ts', 'src/drive/Drive.ts', 'src/drive/OldRoad.ts',
             'src/drive/legs.ts', 'src/story/Chapter6.ts', 'tools/drivelook.py']
    if kessler:
        files += ['src/drive/oldRoadLandmarks.ts', 'src/drive/oldRoadLayout.ts',
                  'src/drive/roadShape.ts', 'src/world/Sky.ts', 'src/world/glow.ts',
                  'src/core/quality.ts', 'src/core/ultra.ts', 'src/core/vhs.ts', 'src/main.ts']
    if dinerlight:
        files += ['src/story/Chapter5.ts', 'src/world/Diner.ts', 'src/drive/Truck.ts',
                  'src/drive/RoadArea.ts', 'src/drive/roadShape.ts', 'src/world/kit.ts',
                  'src/world/Sky.ts', 'src/world/glow.ts',
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
            'loaded_code_sha256':{},'observed_code_responses':[],
            'code_hash_policy':'observed_responses_v1','quality_requested':args.quality,'flicker_pairs':[],
            'route_placement_calls':0,'fixture_placement_calls':0,'side_capture_checks':[]}
    report['trips'].append(trip)
    context = await browser.new_context(viewport={'width':args.size[0],'height':args.size[1]},
                                        device_scale_factor=1,has_touch=False,is_mobile=False)
    page = await context.new_page();page.set_default_timeout(60000)
    tasks = [];previous = None;distance = 0.0;elapsed = 0.0
    handlers=observe_page(page,trip,tasks)
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
        await wait_code_hashes(trip,tasks)
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
        await wait_code_hashes(trip,tasks)
        check_capture_diagnostics(trip,page.url,strict_http=True)
        trip['status']='CAPTURED'
    except BaseException as e:
        trip['status']='FAIL';trip['failure']=f'{type(e).__name__}: {e}';raise
    finally:
        try:
            try:
                await asyncio.wait_for(context.close(),timeout=10)
                trip['code_observation_closed']=True
            finally:
                stop_observing(page,trip,handlers)
            await asyncio.wait_for(wait_code_hashes(trip,tasks),timeout=10)
            if trip['status']=='CAPTURED': check_capture_diagnostics(trip,page.url,strict_http=True)
        except BaseException as e:
            already_failed=trip['status']=='FAIL'
            trip['status']='FAIL';trip.setdefault('failure',f'{type(e).__name__}: {e}')
            trip['cleanup_failure']=f'{type(e).__name__}: {e}'
            await cancel_code_hashes(trip,tasks)
            if not already_failed: raise
        finally:
            trip['path_distance_m']=distance;trip['simulation_elapsed_s']=elapsed
            write_report(out,report)


def dinerlight_route_remaining(points: list, current_xyz: list) -> dict:
    """Horizontal distance along the actual loaded autopilot polyline, without moving it."""
    if len(points) < 2 or any(len(p) != 3 for p in points) or len(current_xyz) != 3:
        raise ValueError('Missing diner autopilot route geometry')
    if not all(math.isfinite(v) for p in points + [current_xyz] for v in p):
        raise ValueError('Non-finite diner route geometry')
    lengths = [math.hypot(b[0]-a[0], b[2]-a[2]) for a,b in zip(points,points[1:])]
    total = sum(lengths)
    if total <= 0: raise ValueError('Degenerate diner autopilot route')
    best = None; acc = 0.0
    for a,b,length in zip(points,points[1:],lengths):
        if length:
            dx,dz = b[0]-a[0],b[2]-a[2]
            u = min(1.0,max(0.0,((current_xyz[0]-a[0])*dx+(current_xyz[2]-a[2])*dz)/(length*length)))
            lateral = math.hypot(current_xyz[0]-a[0]-u*dx,current_xyz[2]-a[2]-u*dz)
            if best is None or lateral < best[0]: best = (lateral,acc+u*length)
        acc += length
    return {'remaining_m':max(0.0,total-best[1]),'lateral_m':best[0],
            'nearest_s_m':best[1],'total_m':total,'goal_xyz':points[-1]}


def dinerlight_quality_errors(state: dict, requested: str) -> list[str]:
    errors = []
    if requested not in ('low','high','ultra'): return ['Unknown requested quality']
    if state.get('quality') != requested: errors.append('Requested quality differs from observed quality')
    if state.get('picture') != ('off' if requested == 'low' else 'vhs'):
        errors.append('Observed picture is not the player default for this quality')
    if state.get('ultra_enabled') is not (requested == 'ultra') or state.get('ultra_on') is not (requested == 'ultra'):
        errors.append('Effective Ultra state differs from requested preset')
    if requested == 'ultra' and state.get('post',{}).get('supported') is not True:
        errors.append('Ultra postprocessing is unsupported')
    render = state.get('render_state',{})
    if render.get('pixel_ratio') != 1: errors.append('Observed renderer DPR is not 1')
    if state.get('held') is not True: errors.append('Realtime loop is not held')
    return errors


def dinerlight_tick_seconds(state: dict, phase: str) -> float:
    if phase == 'dinerlight_arrival':
        remaining = state.get('route_remaining_m')
        if remaining is not None and remaining > 450: return 4.0
        if remaining is not None and remaining <= 20: return 1/FPS
    return STEP


def dinerlight_render_clock_errors(state: dict) -> list[str]:
    """Compare observed HUD/Sky with the clock and current Sky overrides, never repair them."""
    errors = []
    clock, inputs, sky = state.get('clock_seconds'), state.get('sky_clock_inputs',{}), state.get('sky_uniforms',{})
    if not isinstance(clock,(int,float)) or not math.isfinite(clock): return ['Missing render clock']
    seconds = clock % 86400
    expected_hud = f'{int(seconds//3600):02d}:{int(seconds//60)%60:02d}'
    if state.get('hud_clock') != expected_hud: errors.append('Observed HUD clock differs from game clock')
    lift, elevation = inputs.get('dawn_lift'), inputs.get('sun_elev_deg')
    if not isinstance(lift,(int,float)) or not math.isfinite(lift) or (
            elevation is not None and (not isinstance(elevation,(int,float)) or not math.isfinite(elevation))):
        return errors+['Missing/non-finite Sky clock inputs']
    clamp = lambda x: min(1.,max(0.,x))
    dawn = 0. if seconds>43200 else clamp((seconds-16800)/3000)**1.6*lift
    sun = 0. if seconds>43200 else 1. if elevation is not None else clamp((seconds-19560)/240)*lift
    el = math.radians(elevation if elevation is not None else .33+(seconds-19800)/60*.18)
    az = math.radians(79)
    expected = {'uDawn':dawn,'uSun':sun,'uSunDir':[math.sin(az)*math.cos(el),math.sin(el),-math.cos(az)*math.cos(el)]}
    for key,value in expected.items():
        actual = sky.get(key)
        observed, wanted = (actual,value) if isinstance(value,list) else ([actual],[value])
        if not isinstance(observed,(list,tuple)) or len(observed)!=len(wanted) or any(
                not isinstance(a,(int,float)) or not math.isfinite(a) or abs(a-b)>1e-6
                for a,b in zip(observed,wanted)):
            errors.append(f'Observed Sky {key} differs from game clock/overrides')
    return errors


def dinerlight_setup_errors(state: dict) -> list[str]:
    errors=[]
    if state.get('autopilot_mps') is not None or state.get('test_input_active') is not False or state.get('old_control_active') is not False:
        errors.append('Setup has active autopilot or input override')
    if state.get('driving') is not True or state.get('busy') is not False:
        errors.append('Setup is not a ready actual chapter cab')
    speed=state.get('speed_mps')
    if not isinstance(speed,(int,float)) or not math.isfinite(speed) or abs(speed)>1e-8:
        errors.append('Setup truck is not at rest')
    return errors


def dinerlight_camera_player_distance(state: dict) -> float:
    """Observe camera/player XZ separation; Player.place does not update the camera."""
    camera, player = state.get('camera_xyz'), state.get('player_xyz')
    if any(not isinstance(p,(list,tuple)) or len(p)!=3 or
           any(not isinstance(v,(int,float)) or not math.isfinite(v) for v in p)
           for p in (camera,player)):
        raise ValueError('Missing/non-finite diner camera/player pose')
    return math.hypot(camera[0]-player[0],camera[2]-player[2])


def dinerlight_light_findings(ledger: dict) -> list[dict]:
    """Mechanical candidates only; missing/nonfinite instrumentation is blocking."""
    if not all(k in ledger for k in ('sets','spots','player_xyz')) or len(ledger['player_xyz']) != 3:
        raise ValueError('Missing light ledger instrumentation')
    def finite(values):
        if not all(isinstance(v,(int,float)) and math.isfinite(v) for v in values):
            raise ValueError('Non-finite light ledger instrumentation')
    finite(ledger['player_xyz'])
    # Old captures lack both fields and retain their original player-based
    # classification. New observations use the same selector as runtime Ultra.
    modern_reference = 'driving' in ledger or 'camera_xyz' in ledger
    reference_role = 'player'
    if modern_reference:
        if type(ledger.get('driving')) is not bool or not isinstance(ledger.get('camera_xyz'), (list, tuple)) or len(ledger['camera_xyz']) != 3:
            raise ValueError('Missing camera/driving light ledger instrumentation')
        finite(ledger['camera_xyz'])
        reference_role = 'camera' if ledger['driving'] else 'player'
    reference_key = reference_role + '_xyz'
    reference = ledger[reference_key]
    slots = {}; scales = {}
    for flood in ledger['sets']:
        if not all(k in flood for k in ('key','count','capacity','scale','skip','headlight_slots','slots')):
            raise ValueError('Incomplete flood-set ledger')
        finite([flood['count'],flood['capacity'],flood['scale']])
        finite(flood['skip']+flood['headlight_slots'])
        if not 0 <= flood['count'] <= flood['capacity'] or len(flood['slots']) != flood['count']:
            raise ValueError('Inconsistent flood-set count')
        for slot in flood['slots']:
            if len(slot.get('position',[])) != 3 or len(slot.get('colour_rgb',[])) != 3:
                raise ValueError('Missing flood position/colour')
            finite(slot['position']+slot['colour_rgb']+[slot['strength_w'],slot['index']])
            slots[(flood['key'],slot['index'])] = slot
        scales[flood['key']] = flood['scale']
    findings = []
    for spot in ledger['spots']:
        if not all(k in spot for k in ('uuid','position','target','colour_rgb','intensity','distance',
                                       'visible','effective_visible','cast_shadow','matched_slots')):
            raise ValueError('Incomplete SpotLight ledger')
        if any(len(spot[k]) != 3 for k in ('position','target','colour_rgb')):
            raise ValueError('Missing SpotLight position/colour')
        finite(spot['position']+spot['target']+spot['colour_rgb']+[spot['intensity'],spot['distance']])
        finite([spot[k] for k in ('angle','penumbra','decay') if k in spot])
        if 'shadow' in spot: finite(spot['shadow']['map_size'])
        if not spot['effective_visible'] or spot['intensity'] <= 0 or max(spot['colour_rgb']) <= 0:
            continue
        matches = []
        for match in spot['matched_slots']:
            key = (match['key'],match['index'])
            if key not in slots: raise ValueError('Unknown matched flood slot')
            slot = slots[key]
            separation = math.dist(spot['position'],slot['position'])
            if separation > 0.05: raise ValueError('Invalid SpotLight/flood position match')
            # Shared neon/headlight aliases occupy the same position but may have
            # scaled colour. Compare hue before proposing a source association.
            a,b = max(spot['colour_rgb']),max(slot['colour_rgb'])
            if b <= 0 or max(abs(x/a-y/b) for x,y in zip(spot['colour_rgb'],slot['colour_rgb'])) > .02:
                continue
            matches.append((key,slot,separation))
        unskipped_source = any(not slot['skipped'] for _,slot,_ in matches)
        for key,slot,separation in matches:
            evidence = {'spot_uuid':spot['uuid'],'set_key':key[0],'slot_index':key[1],
                        'position_separation_m':separation,'spot_intensity':spot['intensity'],
                        'fake_strength_w':slot['strength_w'],'skipped':slot['skipped'],
                        'fake_set_scale':scales[key[0]],
                        'classification':'mechanical_candidate_not_visual_verdict'}
            if slot['strength_w'] > 0 and max(slot['colour_rgb']) > 0 and scales[key[0]] > 0:
                findings.append({'kind':'positive_fake_with_matching_spot',**evidence})
            if slot['skipped'] and not unskipped_source:
                findings.append({'kind':'active_spot_on_skipped_slot',**evidence})
        distance = math.hypot(spot['position'][0]-reference[0],
                              spot['position'][2]-reference[2])
        if distance > 45:
            finding = {'kind':'active_spot_far_from_observed_' + reference_role,'spot_uuid':spot['uuid'],
                       'horizontal_' + reference_role + '_distance_m':distance,'threshold_m':45,
                       'classification':'mechanical_candidate_not_visual_verdict'}
            if modern_reference:
                finding.update(distance_reference=reference_key, reference_xyz=list(reference))
            findings.append(finding)
        if ledger.get('on_foot_diner') and not any(key[0]=='diner' and not slot['skipped'] for key,slot,_ in matches):
            findings.append({'kind':'active_spot_without_diner_source','spot_uuid':spot['uuid'],
                             'matched_set_keys':sorted({key[0] for key,_,_ in matches}),
                             'classification':'mechanical_candidate_not_visual_verdict'})
    return findings


DINERLIGHT_EXTRA = r"""(() => {
  const s=S47,w=s.world,c=s.camera,L=w.leg&&w.legs[w.leg];
  const D=L?.drive || (w.truckAt==='diner'?w.drive:null);
  if(!D || !w.diner?.flood || !w.road?.headlights) throw Error('Missing diner lighting API');
  const departure=s.game.phase==='ch6';
  const routePoints=departure?w.oldRoad?.routes?.park:w.mods?.legs?.roadRoute?.('diner');
  if(!Array.isArray(routePoints)||routePoints.length<2) throw Error('Missing actual loaded dinerlight autopilot route');
  const sourceSets=new Map();
  const add=(set,role,heads=[])=>{if(!set)return;let item=sourceSets.get(set);
    if(!item){item={set,roles:[],heads:new Set()};sourceSets.set(set,item);}
    item.roles.push(role);heads.forEach(i=>item.heads.add(i));};
  add(w.diner.flood,'diner');
  add(w.road.headlights.set,'road_body',w.road.headlights.slots);
  add(w.oldRoad?.headlights?.set,'oldroad_body',w.oldRoad?.headlights?.slots||[]);
  add(D.area.headlights?.set,'current_truck_body',D.area.headlights?.slots||[]);
  const sets=[...sourceSets.values()].map(({set,roles,heads})=>({key:set.key,roles,
    capacity:set.n,count:set.count,scale:set.scale.value,skip:[...set.skip],headlight_slots:[...heads],
    slots:set.pos.slice(0,set.count).map((p,index)=>({index,position:[p.x,p.y,p.z],
      strength_w:p.w,colour_rgb:set.col[index].toArray(),skipped:set.skip.has(index)}))}));
  const effective=o=>{for(let p=o;p;p=p.parent)if(!p.visible)return false;return true;};
  s.scene.updateMatrixWorld(true);
  const spots=[];
  s.scene.traverse(o=>{if(!o.isSpotLight)return;const position=o.getWorldPosition(c.position.clone()).toArray();
    const matches=[];for(const set of sets)for(const p of set.slots)
      if(Math.hypot(...position.map((v,i)=>v-p.position[i]))<=.05)
        matches.push({key:set.key,index:p.index});
    spots.push({uuid:o.uuid,name:o.name,position,target:o.target.getWorldPosition(c.position.clone()).toArray(),
      colour_rgb:o.color.toArray(),intensity:o.intensity,distance:o.distance,visible:o.visible,
      effective_visible:effective(o),cast_shadow:o.castShadow,angle:o.angle,penumbra:o.penumbra,
      decay:o.decay,shadow:{map_size:o.shadow.mapSize.toArray(),needs_update:o.shadow.needsUpdate,
        auto_update:o.shadow.autoUpdate,map_present:!!o.shadow.map},matched_slots:matches});});
  const heads=D.area.headlights;
  const headlights_active=!!heads&&heads.slots.some(i=>heads.set.pos[i].w>0&&Math.max(...heads.set.col[i].toArray())>0);
  const sky=Object.fromEntries(['uTime','uFlash','uDawn','uSun','uSunDir'].filter(k=>s.sky.uniforms[k])
    .map(k=>{const v=s.sky.uniforms[k].value;return [k,v?.toArray?v.toArray():v];}));
  return {chapter_phase:s.game.phase,chapter5_stage:s.ch5.s.stage,chapter5_arrived:s.ch5.s.arrived,
    chapter6_stage:s.ch6.stage,cinematic:s.game.cinematic,held:s.hold,busy:w.busy,
    quality:JSON.parse(localStorage.getItem('s47.quality')),picture:s.vhs.picture,
    ultra_enabled:s.vhs.ultra,ultra_on:s.ultra.on,
    post:{on:s.vhs.on,supported:s.vhs.supported,glitch:s.vhs.glitch,ultra:s.vhs.ultra},
    camera_quaternion:c.quaternion.toArray(),camera_projection:c.projectionMatrix.toArray(),
    camera_world:c.matrixWorld.toArray(),truck_quaternion:D.truck.group.quaternion.toArray(),
    player_xyz:s.player.pos.toArray(),headlights_active,sky_uniforms:sky,diner_dawn:w.diner.dawn,
    hud_clock:document.querySelector('.clockline')?.textContent,
    sky_clock_inputs:{dawn_lift:s.game.ch6.skyLift ?? (w.area==='roswell'?1:.7),sun_elev_deg:s.game.ch6.sunElev ?? null},
    autopilot_mps:w.autopilot,test_input_active:w.testInput!=null,old_control_active:w.oldControl!=null,
    truck_view_state:{driving:D.truck.driving,group_visible:D.truck.group.visible,
      body_visible:D.truck.body.visible,cab_visible:D.truck.cab.visible,shell_visible:D.truck.shell.visible},
    diner_origin:w.diner.group.position.toArray(),road_origin:w.road.group.position.toArray(),
    route_points:routePoints.map(p=>p.toArray()),
    route_source_api:departure?"world.oldRoad.routes.park":"Read-only loaded World debug mods.legs.roadRoute('diner')",
    body_light_ledger:{sets,spots,player_xyz:s.player.pos.toArray(),
      driving:w.driving,camera_xyz:c.position.toArray(),
      matching_geometry:'World positions within 0.05 m, normalised RGB hue within 0.02; raw geometric aliases retained',
      stale_candidate_geometry:'Active effective-visible SpotLight over 45 horizontal metres from camera while driving or player otherwise, or no unskipped diner source within 0.05 m and normalised RGB hue 0.02 during actual diner foot diagnostic'},
    render_state:{pixel_ratio:s.renderer.getPixelRatio(),buffer_width:s.renderer.domElement.width,
      buffer_height:s.renderer.domElement.height,tone_mapping:s.renderer.toneMapping,
      exposure:s.renderer.toneMappingExposure,ultra_mapped:s.ultra.mapped}};
})()"""


DINERLIGHT_DRAW = r"""time => {
  const s=S47;if(!s.hold)throw Error('Diner draw requires held loop');
  const render=s.vhs.render;
  s.vhs.render=function(scene,camera,_wall){return render.call(this,scene,camera,time);};
  try{s.game.d.view.draw();}finally{s.vhs.render=render;}
  return {render_time_s:time};
}"""


DINERLIGHT_LOOK = r"""({proxy,cab,time}) => {
  const s=S47,w=s.world,D=(w.leg&&w.legs[w.leg]?.drive)||(w.truckAt==='diner'?w.drive:null),c=s.camera;
  if(!s.hold||!D||!w.diner?.proxies[proxy])throw Error('Missing held diner look API');
  const snapshot=()=>({camera:c.position.toArray(),camera_q:c.quaternion.toArray(),
    driver:D.pos.toArray(),truck:D.truck.group.position.toArray(),truck_q:D.truck.group.quaternion.toArray(),
    heading:D.heading,speed:D.speed,clock:s.game.clock,player:s.player.pos.toArray(),
    truck_driving:D.truck.driving,cab_visible:D.truck.cab.visible,shell_visible:D.truck.shell.visible,
    headlights:D.area.headlights?.slots.map(i=>({index:i,position:D.area.headlights.set.pos[i].toArray(),
      colour:D.area.headlights.set.col[i].toArray()}))||[]});
  const before=snapshot();
  try {
    if(cab){
      c.position.fromArray(cab.camera_xyz);c.quaternion.fromArray(cab.camera_quaternion);
      D.truck.cab.visible=true;D.truck.shell.visible=false;
    }
    const target=w.diner.proxies[proxy].getWorldPosition(c.position.clone());
    c.lookAt(target);c.updateMatrixWorld(true);
    const render=s.vhs.render;
    s.vhs.render=function(scene,camera,_wall){return render.call(this,scene,camera,time);};
    try{s.game.d.view.draw();}finally{s.vhs.render=render;}
    return {before,after:snapshot(),render_time_s:time,target_xyz:target.toArray(),proxy,cab_pose:!!cab};
  } catch(error) {
    c.position.fromArray(before.camera);c.quaternion.fromArray(before.camera_q);
    D.truck.cab.visible=before.cab_visible;D.truck.shell.visible=before.shell_visible;
    c.updateMatrixWorld(true);throw error;
  }
}"""


DINERLIGHT_RESTORE = r"""saved => {
  const s=S47,w=s.world,D=(w.leg&&w.legs[w.leg]?.drive)||(w.truckAt==='diner'?w.drive:null),c=s.camera;
  if(!s.hold||!D)throw Error('Missing held diner restoration API');
  const b=saved.before;
  const unchanged=s.game.clock===b.clock&&D.heading===b.heading&&D.speed===b.speed&&
    D.truck.driving===b.truck_driving&&
    JSON.stringify(D.area.headlights?.slots.map(i=>({index:i,position:D.area.headlights.set.pos[i].toArray(),
      colour:D.area.headlights.set.col[i].toArray()}))||[])===JSON.stringify(b.headlights)&&
    JSON.stringify(s.player.pos.toArray())===JSON.stringify(b.player)&&
    JSON.stringify(D.pos.toArray())===JSON.stringify(b.driver)&&
    JSON.stringify(D.truck.group.position.toArray())===JSON.stringify(b.truck)&&
    JSON.stringify(D.truck.group.quaternion.toArray())===JSON.stringify(b.truck_q);
  c.position.fromArray(b.camera);c.quaternion.fromArray(b.camera_q);
  D.truck.cab.visible=b.cab_visible;D.truck.shell.visible=b.shell_visible;c.updateMatrixWorld(true);
  const render=s.vhs.render;
  s.vhs.render=function(scene,camera,_wall){return render.call(this,scene,camera,saved.render_time_s);};
  try{s.game.d.view.draw();}finally{s.vhs.render=render;}
  return {unchanged,visibility_restored:D.truck.cab.visible===b.cab_visible&&D.truck.shell.visible===b.shell_visible,
    camera_restored:JSON.stringify(c.position.toArray())===JSON.stringify(b.camera)&&
    JSON.stringify(c.quaternion.toArray())===JSON.stringify(b.camera_q),render_time_s:saved.render_time_s,
    after:{camera:c.position.toArray(),camera_q:c.quaternion.toArray(),player:s.player.pos.toArray(),
      driver:D.pos.toArray(),truck:D.truck.group.position.toArray(),truck_q:D.truck.group.quaternion.toArray(),
      clock:s.game.clock,cab_visible:D.truck.cab.visible,shell_visible:D.truck.shell.visible,truck_driving:D.truck.driving}};
}"""


async def run_dinerlight_trip(browser, args, report, trip_id, url, out):
    """Actual chapter drives first; parked-camera and Ultra foot views are labelled diagnostics."""
    arrival = trip_id == 'dinerlight_arrival'
    trip = {'id':trip_id,'status':'CAPTURING','visual_review':'UNVERIFIED','shots':[],
            'errors':[],'warnings':[],'console_messages':[],'failed_http':[],
            'loaded_code_sha256':{},'observed_code_responses':[],
            'code_hash_policy':'observed_responses_v1','quality_requested':args.quality,
            'route_placement_calls':0,'clock_assignment_calls':0,'setup_actions':[],
            'side_capture_checks':[],'light_findings':[],'onfoot_trace':[]}
    report['trips'].append(trip)
    context = await browser.new_context(viewport={'width':args.size[0],'height':args.size[1]},
                                        device_scale_factor=1,has_touch=False,is_mobile=False)
    page = await context.new_page(); page.set_default_timeout(60000)
    tasks = []; handlers = observe_page(page,trip,tasks)
    previous = None; distance = 0.0; elapsed = 0.0; phase = 'route_setup'
    route_start_distance = 0.0

    async def state():
        nonlocal previous,distance
        st = await page.evaluate(STATE); st.update(await page.evaluate(DINERLIGHT_EXTRA))
        errors = dinerlight_quality_errors(st,args.quality)
        if errors: raise RuntimeError('; '.join(errors))
        points = st.pop('route_points')
        source_api = st.pop('route_source_api')
        if 'route_geometry' not in trip:
            trip['route_geometry'] = {'points':points,
                'source_api':source_api,
                'coordinate_frame':'world XYZ; distances use horizontal XZ',
                'relationship_to_route':'Same loaded polyline used by the runtime autopilot'}
        elif trip['route_geometry']['points'] != points:
            raise RuntimeError('Loaded diner autopilot route changed during capture')
        # SARO is translated onto the shared road map by STATE, before its actual
        # controller handover. This is an observation transform, never a placement.
        road_point = [st['map_xyz'][i]+st['road_origin'][i] for i in range(3)]
        geometry = dinerlight_route_remaining(points,road_point)
        st['route_remaining_m'] = geometry['remaining_m']
        st['route_lateral_m'] = geometry['lateral_m']
        st['route_goal_xyz'] = geometry['goal_xyz']
        if phase in ('arrival_drive','departure_drive'):
            distance += path_increment(previous,st['map_xyz']); previous = st['map_xyz']
        st['route_distance_m'] = distance-route_start_distance
        st['capture_phase'] = phase
        st['camera_player_horizontal_m'] = dinerlight_camera_player_distance(st)
        st['body_light_ledger']['on_foot_diner'] = phase == 'ultra_onfoot_diagnostic' and st['area']=='diner' and not st['driving']
        st['light_findings'] = dinerlight_light_findings(st['body_light_ledger'])
        return st

    async def tick(seconds=STEP):
        nonlocal elapsed
        await page.evaluate('([seconds,fps])=>{S47.hold=true;S47.tick(seconds,fps);}',[seconds,FPS])
        elapsed += max(1,round(seconds*FPS))/FPS
        return await state()

    async def shot(name,reason,thresholds=None,*,view=None,render_time=None):
        await wait_code_hashes(trip,tasks)
        if render_time is None: render_time = await page.evaluate('performance.now()/1000')
        await page.evaluate(DINERLIGHT_DRAW,render_time)
        st = await state(); filename = f'{trip_id}_{len(trip["shots"]):03d}_{name}.png'
        clock_errors = dinerlight_render_clock_errors(st)
        if clock_errors: raise RuntimeError('; '.join(clock_errors))
        await page.screenshot(path=str(out/filename),timeout=120000)
        st = shot_metadata(st,name=name,filename=filename,reason=reason,
            thresholds=thresholds or [],distance=distance,elapsed=elapsed,url=page.url,
            source_sha=report['source']['git_head'],resources=trip['loaded_code_sha256'],
            renderer=trip['renderer'],size=args.size,quality=st['quality'],picture=st['picture'])
        st['render_time_s'] = render_time; st['view_setup'] = view or {'kind':'actual_current_camera'}
        st['image_sha256'] = hashlib.sha256((out/filename).read_bytes()).hexdigest()
        trip['shots'].append(st)
        trip['light_findings'] += [{'file':filename,'capture_phase':phase,**finding} for finding in st['light_findings']]
        write_report(out,report)
        print(f'{trip_id}/{args.quality}: {name}, path={st["route_distance_m"]:.2f}, remaining={st["route_remaining_m"]:.2f}, clock={st["clock_seconds"]:.2f}',flush=True)
        return st

    async def look(name,proxy,*,cab=None):
        time_s = await page.evaluate('performance.now()/1000')
        saved = await page.evaluate(DINERLIGHT_LOOK,{'proxy':proxy,'cab':cab,'time':time_s})
        try:
            return await shot(name,'Held camera looks at actual diner proxy; no simulation tick',
                view={'kind':'diagnostic_parked_cab_from_observed_arrival' if cab else 'held_current_pose',
                      'target_proxy':proxy,'target_xyz':saved['target_xyz'],
                      'observed_cab_pose':cab,'before':saved['before'],'after':saved['after']},render_time=time_s)
        finally:
            proof = await page.evaluate(DINERLIGHT_RESTORE,saved)
            trip['side_capture_checks'].append({'name':name,**proof})
            if not proof['unchanged'] or not proof['camera_restored'] or not proof['visibility_restored']:
                raise RuntimeError('Diner look changed player/truck/clock or failed camera restoration')

    try:
        await page.add_init_script(INIT+f"\nlocalStorage.setItem('s47.quality',JSON.stringify({json.dumps(args.quality)}));")
        await page.goto(url,wait_until='load',timeout=120000)
        await page.wait_for_function('window.S47 && S47.tick && S47.world')
        await page.click('button[data-a=start]'); await page.wait_for_function('S47.started()')
        chapter = 'chapter5' if arrival else 'chapter6'
        await page.evaluate('chapter=>{S47.hold=true;S47.jump(chapter);}',chapter)
        trip['setup_actions'].append({'api':'S47.jump','chapter':chapter,'semantics':'Chapter setup, not gameplay evidence'})
        if arrival:
            await page.evaluate("""() => {
              if(S47.game.phase!=='ch5'||!S47.ch5.active||S47.ch5.s.stage!=='to-truck'||
                 S47.ch3.truckOverride?.label()!=='Drive to Mesa Diner')throw Error('Chapter-five truck action unavailable');
              S47.ch3.truckOverride.use();
            }""")
            trip['setup_actions'].append({'api':'S47.ch3.truckOverride.use','semantics':'Normal Chapter5 truck action with arrivedDiner callback; no walk-to-truck claim'})
        ready = ("S47.game.phase==='ch5' && S47.ch5.s.stage==='to-truck' && S47.world.area==='saro' && S47.world.leg==='saro'" if arrival else
                 "S47.game.phase==='ch6' && S47.ch6.stage==='drive' && S47.world.area==='roswell' && S47.world.leg==='roswell'")
        for _ in range(240):
            await page.evaluate('S47.hold=true')
            if await page.evaluate(f'S47.world.driving && !S47.world.busy && ({ready})'): break
            await page.evaluate('S47.tick(0.25,30)')
            await page.wait_for_timeout(100)
        else: raise RuntimeError('Dinerlight setup did not reach the requested real chapter cab')
        # Async chapter onStart can change the clock after the preceding frame.
        # Let the ordinary game update synchronise HUD/Sky before starting motion.
        setup_before = await state()
        setup_errors = dinerlight_setup_errors(setup_before)
        if setup_errors: raise RuntimeError('; '.join(setup_errors))
        setup_after = await tick(1/FPS)
        pose_unchanged = all(setup_before[k]==setup_after[k] for k in ('driver_xyz','heading_rad','chapter_phase','area','leg'))
        trip['setup_sync']={'before':setup_before,'after':setup_after,'normal_frames':1,
            'simulation_advance_s':1/FPS,'driver_pose_unchanged':pose_unchanged,
            'clock_assignment_calls':0,'ui_assignment_calls':0,'sky_assignment_calls':0,'placement_calls':0}
        setup_errors = dinerlight_setup_errors(setup_after)+dinerlight_render_clock_errors(setup_after)
        if not pose_unchanged: setup_errors.append('Setup sync moved truck or changed chapter/area/leg')
        if setup_errors: raise RuntimeError('; '.join(setup_errors))
        await page.evaluate('S47.hold=true;S47.world.autopilot=20')
        trip['renderer'] = await page.evaluate("""(() => {const g=S47.renderer.getContext(),e=g.getExtension('WEBGL_debug_renderer_info');return {version:g.getParameter(g.VERSION),vendor:g.getParameter(e?e.UNMASKED_VENDOR_WEBGL:g.VENDOR),renderer:g.getParameter(e?e.UNMASKED_RENDERER_WEBGL:g.RENDERER),pixel_ratio:S47.renderer.getPixelRatio(),hardware_performance:'UNVERIFIED'};})()""")
        phase = 'arrival_drive' if arrival else 'departure_drive'
        st = await state(); trip['route_start'] = st; previous = st['map_xyz']; route_start_distance = distance
        await shot('route_start','Actual cab start after chapter setup; uninterrupted route follows')
        monitor = StallMonitor(); started = not arrival; next_at = 40.0; prepark = False; park_cab = None
        for _ in range(2600):
            if arrival:
                if st['chapter_phase']!='ch5' or st['chapter5_stage'] not in ('to-truck','diner'):
                    raise RuntimeError('Arrival left Chapter5 before natural parking')
                if not st['driving']:
                    if st['truck_at']!='diner' or not started or not prepark:
                        raise RuntimeError('Arrival stopped before the requested approach/pre-park evidence')
                    # At 1/30 s this is the actual park() frame, before the 0.4 s getOut.
                    park_cab = {'camera_xyz':st['camera_xyz'],'camera_quaternion':st['camera_quaternion'],
                                'world_xyz':st['world_xyz'],'clock_seconds':st['clock_seconds']}
                    trip['park_cab_pose'] = park_cab; trip['route_end'] = st
                    break
                if st['chapter5_stage']!='to-truck' or st['leg'] not in ('saro','road') or st['area']!=st['leg']:
                    raise RuntimeError('Arrival phase advanced prematurely while still driving')
                if st['leg']=='road' and st['route_remaining_m']<=300 and not started:
                    started=True;trip['approach_start']=st
                    await shot('approach_start_300m','First actual pose with <=300 m remaining along loaded diner autopilot polyline',[300])
                if started:
                    approach = max(0.0,300-st['route_remaining_m'])
                    due,next_at = capture_due(approach,next_at,40)
                    if due: await shot(f'forward_{int(due[-1]):03d}m','40 m remaining-polyline threshold; actual pose, no interpolation',due)
                    if not prepark and st['route_remaining_m']<=6 and abs(st['speed_mps'])<.35:
                        if not st['headlights_active']: raise RuntimeError('Pre-park frame has no active headlights')
                        prepark=True;trip['prepark_observed_pose']=st
                        await shot('prepark_forward_lights_on','Actual near-stopped cab before natural park; headlights observed on')
                        await look('prepark_facade_lights_on','window')
                        await look('prepark_sign_lights_on','sign')
                progress = -st['route_remaining_m']
            else:
                if st['chapter_phase']!='ch6' or st['chapter6_stage']!='drive' or not st['driving'] or st['leg']!='roswell' or st['area']!='roswell':
                    raise RuntimeError('Departure left the actual Chapter6 cab before 300 m')
                due,next_at = capture_due(st['route_distance_m'],next_at,40)
                if due: await shot(f'forward_{int(due[-1]):03d}m','40 m actual accumulated-path threshold from diner lot; no placement',due)
                if st['route_distance_m']>=300:
                    await shot('departure_end_300m','First actual pose reaching 300 m accumulated horizontal path from the lot',[300])
                    trip['route_end']=st;break
                progress = st['route_distance_m']
            if monitor.observe(elapsed,progress): raise RuntimeError('Dinerlight autopilot stalled before its endpoint')
            st = await tick(dinerlight_tick_seconds(st,trip_id))
        else: raise RuntimeError('Dinerlight route exhausted its bounded steps')
        await page.evaluate('S47.world.autopilot=null')
        trip['route_path_distance_m'] = distance-route_start_distance
        if arrival:
            phase='natural_park_transition'
            for _ in range(30):
                st=await tick(1/FPS)
                if st['area']=='diner' and not st['driving'] and st['truck_at']=='diner' and st['chapter5_arrived'] and st['chapter5_stage']=='diner' and not st['cinematic']: break
            else: raise RuntimeError('Natural diner parking/getOut/Chapter5 callback did not finish')
            # getOut runs after Player.update in this frame. Keep the raw callback
            # state, then let ordinary frames sync its new pose into the camera.
            trip['park_callback_state']=st
            sync_before=st;sync_frames=0
            while st['camera_player_horizontal_m']>.1 and sync_frames<30:
                st=await tick(1/FPS);sync_frames+=1
                if st['area']!='diner' or st['driving'] or st['truck_at']!='diner' or not st['chapter5_arrived'] or st['chapter5_stage']!='diner' or st['cinematic']:
                    raise RuntimeError('Natural diner phase changed while waiting for player camera')
            trip['park_camera_sync']={'before':sync_before,'after':st,
                'normal_tick_frames':sync_frames,'simulation_advance_s':sync_frames/FPS,
                'horizontal_tolerance_m':.1,'camera_assignment_calls':0,'player_assignment_calls':0}
            if st['camera_player_horizontal_m']>.1:
                raise RuntimeError('Natural parked player camera did not sync within 30 normal frames')
            if st['headlights_active']: raise RuntimeError('Natural parked truck headlights were not off')
            phase='natural_parked_onfoot'
            await shot('arrival_end_parked','Natural parked truck, headlights off, normal Chapter5 callback followed by observed player camera sync')
            phase='parked_cab_camera_diagnostic'
            await look('parked_facade_lights_off','window',cab=park_cab)
            await look('parked_sign_lights_off','sign',cab=park_cab)
        if args.quality=='ultra':
            phase='ultra_onfoot_diagnostic'
            if not arrival:
                before_setup=await state()
                await page.evaluate("S47.world.stopDriving();S47.world.enter('diner');S47.world.placeAtDiner('arrive');S47.hold=true")
                trip['setup_actions'].append({'api':"world.stopDriving(); world.enter('diner'); world.placeAtDiner('arrive')",
                    'semantics':'Explicit diagnostic after completed route: stopDriving resets trucks to trip homes; enter diner and place only player at arrive anchor',
                    'before':before_setup,'after':await state()})
            else:
                await page.evaluate("S47.world.enter('diner');S47.hold=true")
                trip['setup_actions'].append({'api':"world.enter('diner')",'semantics':'Explicit diagnostic using actual natural getOut player pose'})
            immediate=await state();trip['onfoot_trace'].append({'step_index':0,'simulation_advance_s':0,'state':immediate})
            trip['light_findings'] += [{'trace_step_index':0,'capture_phase':phase,**f} for f in immediate['light_findings']]
            for step_index in range(1,16):
                st=await tick(1/FPS)
                if st['area']!='diner' or st['driving']: raise RuntimeError('On-foot diagnostic did not remain at the diner')
                trip['onfoot_trace'].append({'step_index':step_index,'simulation_advance_s':step_index/FPS,'state':st})
                trip['light_findings'] += [{'trace_step_index':step_index,'capture_phase':phase,**f} for f in st['light_findings']]
            if st['camera_player_horizontal_m']>.1:
                raise RuntimeError('Ultra on-foot camera did not sync before diagnostic images')
            await shot('ultra_onfoot_after_0_5s','Explicit on-foot diagnostic after 15 normal 1/30 s frames; pool selection interval elapsed, see raw ledger trace')
            await look('ultra_onfoot_facade','window')
            await look('ultra_onfoot_sign','sign')
        await wait_code_hashes(trip,tasks)
        check_capture_diagnostics(trip,page.url,strict_http=True)
        trip['status']='CAPTURED'
    except BaseException as e:
        trip['status']='FAIL';trip['failure']=f'{type(e).__name__}: {e}';raise
    finally:
        try:
            try:
                await asyncio.wait_for(context.close(),timeout=10);trip['code_observation_closed']=True
            finally: stop_observing(page,trip,handlers)
            await asyncio.wait_for(wait_code_hashes(trip,tasks),timeout=10)
            if trip['status']=='CAPTURED': check_capture_diagnostics(trip,page.url,strict_http=True)
        except BaseException as e:
            already_failed=trip['status']=='FAIL';trip['status']='FAIL'
            trip.setdefault('failure',f'{type(e).__name__}: {e}');trip['cleanup_failure']=f'{type(e).__name__}: {e}'
            await cancel_code_hashes(trip,tasks)
            if not already_failed: raise
        finally:
            trip['path_distance_m']=distance;trip['simulation_elapsed_s']=elapsed;write_report(out,report)


async def run_trip(browser, args, report, trip_id, url, out):
    if trip_id in ('dinerlight_arrival','dinerlight_departure'):
        return await run_dinerlight_trip(browser,args,report,trip_id,url,out)
    if trip_id == 'kessler':
        return await run_kessler_trip(browser, args, report, url, out)
    trip = {'id': trip_id, 'status': 'CAPTURING', 'visual_review': 'UNVERIFIED',
            'shots': [], 'errors': [], 'warnings': [], 'console_messages': [],
            'failed_http': [], 'loaded_code_sha256': {}, 'observed_code_responses': [],
            'code_hash_policy': 'observed_responses_v1'}
    report['trips'].append(trip)
    context = await browser.new_context(viewport={'width': args.size[0], 'height': args.size[1]}, device_scale_factor=1)
    page = await context.new_page()
    page.set_default_timeout(60000)
    tasks = []
    handlers = observe_page(page, trip, tasks)
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
        await wait_code_hashes(trip, tasks)
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
        await wait_code_hashes(trip, tasks)
        check_capture_diagnostics(trip, page.url)
    except BaseException as e:
        trip['status'] = 'FAIL'
        trip['failure'] = f'{type(e).__name__}: {e}'
        raise
    finally:
        try:
            try:
                await asyncio.wait_for(context.close(), timeout=10)
                trip['code_observation_closed'] = True
            finally:
                stop_observing(page, trip, handlers)
            await asyncio.wait_for(wait_code_hashes(trip, tasks), timeout=10)
            if trip['status'] == 'CAPTURED': check_capture_diagnostics(trip, page.url)
        except BaseException as e:
            already_failed = trip['status'] == 'FAIL'
            trip['status'] = 'FAIL'
            trip.setdefault('failure', f'{type(e).__name__}: {e}')
            trip['cleanup_failure'] = f'{type(e).__name__}: {e}'
            await cancel_code_hashes(trip, tasks)
            if not already_failed: raise
        finally:
            trip['path_distance_m'] = distance
            trip['simulation_elapsed_s'] = elapsed
            write_report(out, report)


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
              'requested_trips':(['saro_diner','diner_oldroad'] if args.trip=='all' else
                  ['dinerlight_arrival','dinerlight_departure'] if args.trip=='dinerlight' else [args.trip]),
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
    if args.trip=='dinerlight':
        report['interval_m']=40.0;report['quality_requested']=args.quality
        report['dinerlight']={'arrival_remaining_m':300,'departure_accumulated_path_m':300,
            'arrival_warmup_tick_s':4,'arrival_warmup_only_above_remaining_m':450,
            'capture_tick_s':STEP,'arrival_final_20m_tick_s':1/FPS,
            'onfoot_pool_trace_steps':15,'onfoot_pool_trace_step_s':1/FPS,
            'preset':'Fresh requested quality, player-default picture: Low off, High/Ultra VHS',
            'contexts':'Separate fresh Chapter5 arrival and Chapter6 departure contexts'}
        report['limitations'] += ['Chapter jumps and direct chapter truck action are setup, not complete gameplay evidence.',
            'Arrival remaining distance follows the loaded autopilot polyline; departure path is accumulated from actual sampled poses, including the lot.',
            'Parked cab views use the camera observed on the actual park frame after natural getOut, with reversible cab/shell visibility overrides; headlights remain off.',
            'Ultra on-foot setup after completed departure resets trucks to trip homes and places only the player at a documented diner anchor.',
            'Position/hue-matched light candidates are mechanical evidence, not an authoritative Ultra source ledger or automatic visual PASS.',
            '844x390 uses a desktop pointer context and DPR1; physical phone controls/fps are UNVERIFIED.',
            'Captures must be run only after the requested diner-lighting Pages deployment is available.']
    started = time.monotonic()
    def terminated(_signum, _frame):
        raise KeyboardInterrupt('SIGTERM: capture interrupted; partial evidence retained')
    previous_sigterm = signal.signal(signal.SIGTERM, terminated)
    try:
        report['source'] = source_info(args.trip == 'kessler',dinerlight=args.trip == 'dinerlight')
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
