#!/usr/bin/env python3
"""Privat Kessler-forhåndsvisning i faktisk OldRoad og Sky ved 05:24.

python3 tools/landmarkpreview.py --port 8486
python3 tools/landmarkpreview.py --capture /tmp/kessler-preview

Lager en separat Vite-rot i /tmp uten endringer i spillinngang eller byggeoppsett.
To faste 1280x800-utsnitt og window.LANDMARK_PREVIEW.report dokumenterer modulens
geometri, kontrakter og ressurser. --capture starter én nettleser; koordiner den
med andre tunge tester. Forhåndsvisningen er ikke bevis på runtime-integrasjon.
"""
from pathlib import Path
import argparse
import asyncio
from datetime import datetime, timezone
import hashlib
import json
import os
import shutil
import subprocess
import tempfile

HTML = r'''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><link rel="icon" href="data:,"><title>SIGNAL / 47: Kessler preview</title><style>
*{box-sizing:border-box}html,body{margin:0;width:100%;height:100%;overflow:hidden;background:#080d15;color:#e5dbc6;font:13px system-ui,sans-serif}#stage{position:absolute;left:50%;top:50%;transform:translate(-50%,-50%);width:min(100vw,160vh);aspect-ratio:8/5}canvas{display:block;width:100%;height:100%}header{position:absolute;left:0;top:0;right:0;padding:8px 12px;background:#111b27ee;display:flex;gap:8px;align-items:center;flex-wrap:wrap}header strong{letter-spacing:.06em}button{border:1px solid #8c816e;background:#263640;color:#ecdfc6;border-radius:4px;min-height:30px;padding:4px 10px;cursor:pointer}button[aria-pressed=true]{background:#d4ba82;color:#182a32}button:focus-visible{outline:3px solid #ead18b;outline-offset:2px}aside{position:absolute;right:12px;bottom:12px;max-width:490px;background:#111b27ee;border:1px solid #4e6269;padding:10px 12px;line-height:1.45}#checks{max-height:33vh;overflow:auto}#checks div{border-top:1px solid #31424a;padding:3px 0}.pass{color:#bbd2a5}.fail{color:#ff9f8c}#error{position:absolute;left:12px;bottom:12px;display:none;background:#631f20;padding:12px;max-width:55vw;white-space:pre-wrap}body.capture .overlay{display:none}
</style></head><body><div id="stage"><canvas id="view" width="1280" height="800"></canvas></div><header class="overlay"><strong>KESSLER / PREVIEW · 05:24</strong><button data-view="Road">Road · mile 3.18</button><button data-view="Driveway">Driveway · north</button><button id="recreate">Recreate</button><button id="export">Export JSON</button></header><aside class="overlay"><div id="metrics">Loading actual OldRoad artwork…</div><p>Module preview only. Existing OldRoad fences remain unchanged; the returned fence gap awaits integration.</p><details><summary id="contract-title">Structural checks</summary><div id="checks"></div></details></aside><pre id="error" class="overlay"></pre><script type="module" src="/preview.ts"></script></body></html>'''

ENTRY = r'''import * as THREE from 'three';
import { OldRoad, oldFlood } from '/src/drive/OldRoad.ts';
import { buildLandmarks } from '/src/drive/oldRoadLandmarks.ts';
import { beside, heightAt, sAt, rnd, nearest, HALF } from '/src/drive/oldRoadLayout.ts';
import { floodlit } from '/src/world/kit.ts';
import { GlowPoints, glowScale } from '/src/world/glow.ts';
import { Sky } from '/src/world/Sky.ts';
import { loadArtFor, OLDROAD_ART } from '/src/core/art.ts';

const errors:string[]=[],warnings:string[]=[],source=__SOURCE_INFO__;
function error(e:unknown){errors.push(String(e));const box=document.querySelector<HTMLPreElement>('#error')!;box.style.display='block';box.textContent=errors.join('\n');}
window.addEventListener('error',e=>error(e.message));window.addEventListener('unhandledrejection',e=>error(e.reason));
const oldError=console.error.bind(console),oldWarn=console.warn.bind(console);
console.error=(...a)=>{oldError(...a);error(a.map(String).join(' '));};
console.warn=(...a)=>{oldWarn(...a);const m=a.map(String).join(' ');warnings.push(m);if(m.includes('flood set full'))error(m);};
await loadArtFor([...OLDROAD_ART,'desert','concrete','asphalt','sky']);
const renderer=new THREE.WebGLRenderer({canvas:document.querySelector<HTMLCanvasElement>('#view')!,antialias:true,preserveDrawingBuffer:true});
renderer.setPixelRatio(1);renderer.setSize(1280,800,false);
renderer.outputColorSpace=THREE.SRGBColorSpace;renderer.toneMapping=THREE.ACESFilmicToneMapping;renderer.toneMappingExposure=1.05;
const scene=new THREE.Scene();scene.fog=new THREE.FogExp2(0x0b0f19,.00055);
const camera=new THREE.PerspectiveCamera(70,1280/800,.03,12000);
// The constructor derives the area's true origin from the shared road layout. It is
// deliberately nonzero, so an accidental double origin in the new module is visible.
const road=new OldRoad(new THREE.Vector3(8000,0,0));scene.add(road.group);
const origin=road.group.position.clone();
const sky=new Sky();sky.setDawnLift(1);sky.setClock(5*3600+24*60);sky.setCometClock(5*3600+24*60);scene.add(sky.group);
const clockSeconds=5*3600+24*60,gate=beside(sAt(3.2),-17),approach=beside(sAt(3.18),1.6);
const std=(o:THREE.MeshStandardMaterialParameters,fall=.012)=>floodlit(new THREE.MeshStandardMaterial(o),fall,oldFlood);
const ctx={beside,heightAt,sAt,rnd,std};
let start=performance.now(),landmarks=buildLandmarks(ctx),buildMs=performance.now()-start;
const firstBuildMs=buildMs;road.group.add(landmarks.group);
glowScale.value=800/(2*Math.tan(camera.fov*Math.PI/360));
function makeGlows(){const glow=new GlowPoints();for(const p of landmarks.glints)glow.add(p.x,p.y,p.z,.35,0x9b7446);const points=glow.build();points.name='preview/Kessler-yard-lamp';road.group.add(points);return glow;}
let landmarkGlow=makeGlows();
let view='Road',recreations=0,frames=0;
const views:Record<string,{eye:number[],target:number[],note:string}>={
 Road:{eye:[approach.x,heightAt(approach.x,approach.z)+1.62,approach.z],target:[gate.x,heightAt(gate.x,gate.z)+3.1,gate.z],note:'Fixed road-local eye at mile 3.18, facing the hanging gate sign at mile 3.2.'},
 Driveway:{eye:[gate.x,heightAt(gate.x,gate.z-10)+1.62,gate.z-10],target:[gate.x,heightAt(gate.x,gate.z-300)+1.4,gate.z-300],note:'Fixed eye 10 m north of the gate, looking due north along the 300 m track.'},
};
const world=(p:number[])=>new THREE.Vector3().fromArray(p).add(origin);
function place(name:string){if(!views[name])throw new Error('Unknown preview view: '+name);view=name;camera.position.copy(world(views[name].eye));camera.lookAt(world(views[name].target));camera.updateMatrixWorld(true);document.querySelectorAll<HTMLButtonElement>('[data-view]').forEach(b=>b.setAttribute('aria-pressed',String(b.dataset.view===view)));render();}
function resources(root:THREE.Object3D){const gs=new Set<THREE.BufferGeometry>(),ms=new Set<THREE.Material>(),ts=new Set<THREE.Texture>();let meshes=0,triangles=0,drawCalls=0,lights=0,finite=true;
 root.traverse(o=>{const m=o as THREE.Mesh;const l=o as THREE.Light;if(l.isLight)lights++;if(!m.geometry||!m.material)return;if(m.isMesh)meshes++;gs.add(m.geometry);
  for(const a of Object.values(m.geometry.attributes))for(const n of a.array)if(!Number.isFinite(n))finite=false;
  const index=m.geometry.index;if(index)for(const n of index.array)if(!Number.isFinite(n)||n<0||n>=m.geometry.attributes.position.count)finite=false;
  const copies=(m as THREE.InstancedMesh).isInstancedMesh?(m as THREE.InstancedMesh).count:1;
  if(m.isMesh)triangles+=(index?.count??m.geometry.attributes.position.count)/3*copies;
  const materials=Array.isArray(m.material)?m.material:[m.material];materials.forEach(mat=>{ms.add(mat);for(const value of Object.values(mat))if(value instanceof THREE.Texture)ts.add(value);if(mat instanceof THREE.ShaderMaterial)for(const uniform of Object.values(mat.uniforms))if(uniform.value instanceof THREE.Texture)ts.add(uniform.value);});
  // Grouped geometry can use one material in several submitted calls.
  drawCalls+=Array.isArray(m.material)?m.geometry.groups.filter(g=>materials[g.materialIndex??0]?.visible).length:(m.material.visible?1:0);
 });return {gs,ms,ts,meshes,triangles,drawCalls,lights,finite};
}
let disposal={exercised:false,geometries:0,disposedGeometries:0,materials:0,disposedMaterials:0,textures:0,disposedTextures:0};
function disposeModule(){const r=resources(landmarks.group),glow=resources(landmarkGlow.points);glow.gs.forEach(g=>r.gs.add(g));glow.ms.forEach(m=>r.ms.add(m));glow.ts.forEach(t=>r.ts.add(t));disposal={exercised:true,geometries:r.gs.size,disposedGeometries:0,materials:r.ms.size,disposedMaterials:0,textures:r.ts.size,disposedTextures:0};
 r.gs.forEach(g=>g.addEventListener('dispose',()=>disposal.disposedGeometries++));r.ms.forEach(m=>m.addEventListener('dispose',()=>disposal.disposedMaterials++));r.ts.forEach(t=>t.addEventListener('dispose',()=>disposal.disposedTextures++));
 landmarks.group.removeFromParent();landmarkGlow.points.removeFromParent();r.gs.forEach(g=>g.dispose());r.ms.forEach(m=>m.dispose());r.ts.forEach(t=>{if(!t.name.startsWith('art/'))t.dispose();});
}
function recreate(){disposeModule();start=performance.now();landmarks=buildLandmarks(ctx);buildMs=performance.now()-start;road.group.add(landmarks.group);landmarkGlow=makeGlows();recreations++;place(view);return report;}
function obstacleResult(o:typeof landmarks.obstacles[number]){if(o.kind==='circle'){
  const finite=[o.x,o.z,o.r].every(Number.isFinite)&&o.r>0,n={...nearest(o.x,o.z)};
  return {kind:o.kind,finite,outsideAsphalt:finite&&(n.d===Infinity||n.d-o.r>HALF),distance:n.d===Infinity?null:n.d,radius:o.r};
 }if(o.kind==='box'){
  const finite=[o.minX,o.maxX,o.minZ,o.maxZ].every(Number.isFinite)&&o.minX<o.maxX&&o.minZ<o.maxZ;
  const x=(o.minX+o.maxX)/2,z=(o.minZ+o.maxZ)/2,r=Math.hypot(o.maxX-o.minX,o.maxZ-o.minZ)/2,n={...nearest(x,z)};
  // Circumscribed radius is a conservative whole-box clearance proof.
  return {kind:o.kind,finite,outsideAsphalt:finite&&(n.d===Infinity||n.d-r>HALF),distance:n.d===Infinity?null:n.d,radius:r};
 }return {kind:'unknown',finite:false,outsideAsphalt:false,distance:null,radius:null};}
let report:Record<string,unknown>={};
function metrics(){scene.updateMatrixWorld(true);const r=resources(landmarks.group),bounds=new THREE.Box3().setFromObject(landmarks.group),localBounds=bounds.clone().translate(origin.clone().negate()),obs=landmarks.obstacles.map(obstacleResult);
 const g=landmarks.group,atGate=g.position.distanceTo(new THREE.Vector3(gate.x,0,gate.z))<1e-5&&g.rotation.x===0&&g.rotation.y===0&&g.rotation.z===0&&g.scale.distanceTo(new THREE.Vector3(1,1,1))<1e-8;
 const gateNearest={...nearest(g.position.x,g.position.z)},gaps=landmarks.fenceGaps;
 const glints=landmarks.glints;
 const gapAtGate=gaps.length===1&&gaps.every(gap=>gap.side===-1&&Number.isFinite(gap.s0)&&Number.isFinite(gap.s1)&&gap.s0<sAt(3.2)&&gap.s1>sAt(3.2));
 // Bounding neighbourhood rejects world coordinates baked into otherwise local geometry.
 const localNeighbourhood=!localBounds.isEmpty()&&[localBounds.min.x,localBounds.max.x].every(x=>Math.abs(x-gate.x)<600)&&[localBounds.min.z,localBounds.max.z].every(z=>Math.abs(z-gate.z)<600);
 const checks:[string,boolean][]=[
  ['Module uses local coordinates under OldRoad origin',g.parent===road.group&&atGate&&localNeighbourhood],
  // beside uses interpolated tangents on 4 m samples; nearest projects the actual
  // piecewise segment, so its recovered s is approximate (0.121 m here).
  ['Gate group is exact beside(3.2 mi,-17); inverse projection within 0.25 m',atGate&&Math.abs(gateNearest.o+17)<.03&&Math.abs(gateNearest.s-sAt(3.2))<.25],
  ['Exactly one left fence gap contains mile 3.2',gapAtGate],
  ['Finite geometry attributes and indices',r.finite],
  ['Fewer than 15000 module triangles',r.triangles<15000],
  ['At most four module meshes',r.meshes<=4],
  ['At most four material submission calls',r.drawCalls<=4&&r.ms.size<=4],
  ['Module plus shared lamp needs at most four calls',r.drawCalls+1<=4],
  ['No new light nodes',r.lights===0],
  ['Exactly one finite yard-lamp glint',glints.length===1&&glints.every(glint=>[glint.x,glint.y,glint.z].every(Number.isFinite))],
  ['Finite collision obstacles wholly outside asphalt',obs.length>0&&obs.every(o=>o.finite&&o.outsideAsphalt)],
  ['Fixed 1280x800 drawing buffer',renderer.domElement.width===1280&&renderer.domElement.height===800],
  ['No captured runtime errors',errors.length===0],
 ];
 if(disposal.exercised)checks.push(['Retired module resources disposed once',disposal.geometries===disposal.disposedGeometries&&disposal.materials===disposal.disposedMaterials&&disposal.textures===disposal.disposedTextures]);
 report={scope:'Standalone module preview in actual OldRoad and Sky; not gameplay integration',source,view,viewport:[1280,800],clock:'05:24:00',clockSeconds,simulationTime:0,frames,origin:origin.toArray(),camera:{localEye:views[view].eye,worldEye:camera.position.toArray(),localTarget:views[view].target,direction:camera.getWorldDirection(new THREE.Vector3()).toArray(),fov:camera.fov,note:views[view].note},gateReference:{x:gate.x,z:gate.z,y:heightAt(gate.x,gate.z),s:sAt(3.2),offset:-17,nearest:gateNearest},module:{meshes:r.meshes,materials:r.ms.size,triangles:r.triangles,estimatedCalls:r.drawCalls,groupLocalPosition:g.position.toArray(),boundsLocal:{min:localBounds.min.toArray(),max:localBounds.max.toArray()}},sharedLamp:{calls:1,sizeMetres:.35,color:0x9b7446},sceneRender:{...renderer.info.render},gpu:{...renderer.info.memory},firstBuildMs,buildMs,recreations,obstacles:landmarks.obstacles,obstacleChecks:obs,fenceGaps:gaps,glints:glints.map(g=>g.toArray()),disposal,checks:checks.map(([name,pass])=>({name,pass})),errors:[...errors],warnings:[...warnings],integration:{fenceGapApplied:false,glintsConnectedToOldRoad:false,lamp:'Rendered in a separate preview-only GlowPoints consumer',gameplay:'UNVERIFIED',hardwareFps:'UNVERIFIED',visual:'Requires actual image review'}};
 const api=(window as any).LANDMARK_PREVIEW;api.report=report;
 document.querySelector('#metrics')!.textContent=`${view} · ${r.meshes}/4 meshes · ${r.drawCalls}/4 estimated module calls · ${r.triangles.toLocaleString()} triangles | Full scene: ${renderer.info.render.calls} calls | Build ${buildMs.toFixed(1)} ms | Recreate ${recreations}`;
 document.querySelector('#contract-title')!.textContent=`Structural checks: ${checks.filter(c=>c[1]).length}/${checks.length} PASS`;
 document.querySelector('#checks')!.replaceChildren(...checks.map(([label,pass])=>{const d=document.createElement('div');d.className=pass?'pass':'fail';d.textContent=(pass?'PASS: ':'FAIL: ')+label;return d;}));
}
function render(){sky.update(0,0,camera.position,1);scene.fog!.color.copy(sky.fogColor);road.update(0,0,camera.position,{dawn:sky.uniforms.uDawn.value,fog:sky.fogColor,sunDir:sky.uniforms.uSunDir.value,sun:sky.uniforms.uSun.value});landmarkGlow.update(0);renderer.render(scene,camera);frames++;metrics();}
(window as any).LANDMARK_PREVIEW={report,setView:place,recreate,captureUi:(hidden=true)=>{document.body.classList.toggle('capture',hidden);render();},render};
document.querySelectorAll<HTMLButtonElement>('[data-view]').forEach(b=>b.addEventListener('click',()=>place(b.dataset.view!)));
document.querySelector('#recreate')!.addEventListener('click',recreate);
document.querySelector('#export')!.addEventListener('click',()=>{const a=document.createElement('a'),url=URL.createObjectURL(new Blob([JSON.stringify(report,null,2)],{type:'application/json'}));a.href=url;a.download='landmark-preview.json';a.click();URL.revokeObjectURL(url);});
window.addEventListener('pagehide',()=>{disposeModule();road.dispose();const r=resources(sky.group);r.gs.forEach(g=>g.dispose());r.ms.forEach(m=>m.dispose());r.ts.forEach(t=>t.dispose());renderer.dispose();});
place('Road');
'''


SOURCE_FILES = ('src/drive/oldRoadLandmarks.ts', 'src/drive/OldRoad.ts',
                'src/drive/oldRoadLayout.ts', 'src/world/Sky.ts', 'tools/landmarkpreview.py')


def source_snapshot(web):
    def git(*args):
        return subprocess.check_output(['git', *args], cwd=web, text=True).strip()
    return {'head': git('rev-parse', 'HEAD'), 'branch': git('branch', '--show-current'),
            'fileSha256': {name: hashlib.sha256((web / name).read_bytes()).hexdigest() for name in SOURCE_FILES}}


def create_preview(web, port, source):
    folder = Path(tempfile.mkdtemp(prefix='s47-landmark-preview-'))
    (folder / 'src').symlink_to(web / 'src', target_is_directory=True)
    (folder / 'node_modules').symlink_to(web / 'node_modules', target_is_directory=True)
    (folder / 'index.html').write_text(HTML, encoding='utf-8')
    (folder / 'preview.ts').write_text(ENTRY.replace('__SOURCE_INFO__', json.dumps(source)), encoding='utf-8')
    config = {'root': str(folder), 'server': {'host': '127.0.0.1', 'port': port,
              'strictPort': True, 'fs': {'allow': [str(web), str(folder)]}},
              'optimizeDeps': {'include': ['three']}}
    (folder / 'vite.config.mjs').write_text('export default ' + json.dumps(config) + ';\n', encoding='utf-8')
    return folder


async def capture(url, output, server, web, source):
    from playwright.async_api import async_playwright
    from urllib.request import urlopen
    output.mkdir(parents=True, exist_ok=True)
    for _ in range(120):
        if server.poll() is not None:
            raise RuntimeError('Vite avsluttet før fangst; se vite.log')
        try:
            with urlopen(url, timeout=1) as response:
                if response.status == 200:
                    break
        except OSError:
            await asyncio.sleep(.25)
    else:
        raise RuntimeError('Vite svarte ikke etter 30 sekunder')
    raw_console, page_errors, http_errors, shots = [], [], [], []
    async with async_playwright() as p:
        browser = await p.chromium.launch(channel='chromium', args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'])
        try:
            page = await browser.new_page(viewport={'width': 1280, 'height': 800}, device_scale_factor=1)
            page.on('console', lambda m: raw_console.append({'type': m.type, 'text': m.text, 'location': m.location}) if m.type in ('error', 'warning') else None)
            page.on('pageerror', lambda e: page_errors.append(str(e)))
            page.on('response', lambda r: http_errors.append({'url': r.url, 'status': r.status}) if r.status >= 400 else None)
            await page.goto(url, wait_until='networkidle', timeout=120000)
            try:
                await page.wait_for_function('window.LANDMARK_PREVIEW?.report?.frames > 0', timeout=30000)
            except Exception as exc:
                (output / 'failure.json').write_text(json.dumps({'status': 'FAIL', 'error': str(exc),
                    'console': raw_console, 'pageErrors': page_errors, 'httpErrors': http_errors,
                    'visibleText': await page.locator('body').inner_text(), 'source': source}, indent=2) + '\n')
                raise
            await page.evaluate('LANDMARK_PREVIEW.captureUi(true)')
            for name in ('Road', 'Driveway'):
                await page.evaluate('(name)=>LANDMARK_PREVIEW.setView(name)', name)
                await page.evaluate('document.fonts.ready')
                path = output / (name.lower() + '.png')
                await page.screenshot(path=str(path))
                report = await page.evaluate('LANDMARK_PREVIEW.report')
                shots.append({'file': path.name, 'sha256': hashlib.sha256(path.read_bytes()).hexdigest(), 'report': report})
            await page.evaluate('LANDMARK_PREVIEW.recreate()')
            recreation = await page.evaluate('LANDMARK_PREVIEW.report')
        finally:
            await browser.close()
    source_after = source_snapshot(web)
    source_unchanged = source['fileSha256'] == source_after['fileSha256']
    passed = source_unchanged and not page_errors and not http_errors and not any(m['type'] == 'error' for m in raw_console)
    passed = passed and all(all(c['pass'] for c in s['report']['checks']) for s in shots) and all(c['pass'] for c in recreation['checks'])
    result = {'status': 'PASS' if passed else 'FAIL', 'capturedAtUtc': datetime.now(timezone.utc).isoformat(),
              'scope': 'Structural checks and two fixed standalone preview screenshots; visual/gameplay/hardware review is separate.',
              'url': url, 'source': source, 'sourceAfter': source_after, 'sourceFilesUnchanged': source_unchanged,
              'shots': shots, 'recreation': recreation, 'console': raw_console,
              'pageErrors': page_errors, 'httpErrors': http_errors}
    (output / 'report.json').write_text(json.dumps(result, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    print(f"{result['status']}: {output / 'report.json'}", flush=True)
    return 0 if passed else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=8486)
    parser.add_argument('--write-only', action='store_true')
    parser.add_argument('--capture', type=Path, help='Lagre to 1280x800 PNG og kontrollrapport; starter én nettleser')
    args = parser.parse_args()
    if args.write_only and args.capture:
        parser.error('--write-only og --capture kan ikke kombineres')
    if args.capture and args.capture.expanduser().resolve().exists() and any(args.capture.expanduser().resolve().iterdir()):
        parser.error('--capture krever en ny eller tom mappe; eksisterende evidens bevares')
    web = Path(__file__).resolve().parents[1]
    source = source_snapshot(web)
    folder = create_preview(web, args.port, source)
    url = f'http://127.0.0.1:{args.port}/'
    print(f'Landmark preview: {url}\nMidlertidige filer: {folder}', flush=True)
    if args.write_only:
        return 0
    node = os.environ.get('LANDMARK_NODE') or shutil.which('node') or str(Path.home() / '.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node')
    command = [node, str(web / 'node_modules/vite/bin/vite.js'), '--config', str(folder / 'vite.config.mjs')]
    if args.capture:
        output = args.capture.expanduser().resolve()
        output.mkdir(parents=True, exist_ok=True)
        with (output / 'vite.log').open('w', encoding='utf-8') as log:
            server = subprocess.Popen(command, cwd=folder, stdout=log, stderr=subprocess.STDOUT)
            try:
                return asyncio.run(capture(url, output, server, web, source))
            finally:
                server.terminate()
                try:
                    server.wait(timeout=8)
                except subprocess.TimeoutExpired:
                    server.kill()
                    server.wait()
    try:
        subprocess.run(command, cwd=folder, check=True)
    except KeyboardInterrupt:
        pass
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
