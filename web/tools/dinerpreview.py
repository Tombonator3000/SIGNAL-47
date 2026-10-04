#!/usr/bin/env python3
"""Privat Diner-forhåndsvisning, uten spillinngang eller endret Vite-konfigurasjon.

python3 tools/dinerpreview.py --port 8485
Genererer en separat Vite-rot i /tmp. Starter ingen nettleser. DOM og
window.DINER_PREVIEW.report viser målrettede kontrakter, ruter og målinger.
Runde 8-bilder lastes bare her hvis filene finnes; --fallback bruker reserveflatene.
"""
from pathlib import Path
import argparse
import json
import os
import shutil
import subprocess
import tempfile

HTML = r'''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>SIGNAL / 47: Diner preview</title><style>
*{box-sizing:border-box}html,body{margin:0;width:100%;height:100%;overflow:hidden;background:#1c2734;color:#eee6d1;font:13px system-ui,sans-serif}canvas{display:block;width:100%;height:100%}header{position:absolute;top:0;left:0;right:0;padding:8px 12px;background:#111d28ed;display:flex;align-items:center;gap:6px;flex-wrap:wrap}header strong{letter-spacing:.06em;margin-right:4px}nav{display:flex;gap:4px;flex-wrap:wrap}button{color:#ecdfc6;border:1px solid #8b826f;background:#263640;border-radius:4px;min-height:30px;padding:3px 8px;cursor:pointer}button[aria-pressed=true]{background:#d4ba82;color:#182a32}button:focus-visible{outline:3px solid #ead18b;outline-offset:2px}label{display:flex;align-items:center;gap:5px}input{width:84px}aside{position:absolute;right:12px;bottom:12px;width:min(480px,calc(100vw - 24px));background:#111c26ed;padding:9px 12px;border:1px solid #4e6269;line-height:1.45}summary{cursor:pointer;padding-top:4px}#metrics{font-variant-numeric:tabular-nums}#checks{max-height:36vh;overflow:auto;margin-top:7px}#checks div{border-top:1px solid #31424a;padding:3px 0}.pass{color:#bbd2a5}.fail{color:#ff9f8c}#error{position:absolute;left:12px;bottom:12px;background:#631f20;white-space:pre-wrap;max-width:55vw;padding:12px;display:none}@media(max-height:480px){header{padding:4px 8px;gap:4px}button{min-height:25px;padding:2px 6px}aside{font-size:10px;padding:5px 8px;bottom:8px;right:8px;width:355px}header strong{font-size:11px}}
</style></head><body><canvas id="view"></canvas><header><strong>DINER / PREVIEW</strong><nav aria-label="Views"><button data-view="Arrival">Arrival</button><button data-view="Outside">Outside</button><button data-view="Counter">Counter</button><button data-view="Booth">Booth</button><button data-view="Phone">Phone</button><button data-view="Clipping">Clipping</button><button data-view="Menu">Menu</button><button data-view="Sign">Sign</button><button data-view="Worst">Worst</button></nav><nav aria-label="Quality"><button id="high">High</button><button id="low">Low</button></nav><button id="sign">Sign on</button><button id="door">Door open</button><label>Dawn <input id="dawn" type="range" min="0" max="1" step=".05" value=".25"></label><button id="recreate">Recreate</button><button id="export">Export JSON</button></header><aside aria-live="polite"><div id="metrics">Loading DINER_ART…</div><details><summary id="contract-title">Contract checks</summary><div id="checks"></div></details></aside><pre id="error"></pre><script type="module" src="/preview.ts"></script></body></html>'''

ENTRY = r'''import * as THREE from 'three';
import { Diner, dinerFlood } from '/src/world/Diner.ts';
import { loadArtFor, DINER_ART, artTexture } from '/src/core/art.ts';
import { setQuality, eachVariant } from '/src/core/quality.ts';
import { Interaction } from '/src/core/Interaction.ts';
import { Player } from '/src/player/Player.ts';
import { flood as siteFlood, annexFlood } from '/src/world/kit.ts';

const errors:string[]=[];
function error(e:unknown){errors.push(String(e));const box=document.querySelector<HTMLPreElement>('#error')!;box.style.display='block';box.textContent=errors.join('\n');}
window.addEventListener('error',e=>error(e.message));window.addEventListener('unhandledrejection',e=>error(e.reason));
const warn=console.warn.bind(console);console.warn=(...a)=>{warn(...a);if(a.join(' ').includes('flood set full'))error(a.join(' '));};
await loadArtFor(DINER_ART);
const surfaceUrls:string[]=__SURFACE_URLS__,surfaceArt:THREE.Texture[]=[];
for(const [i,url] of surfaceUrls.entries()){const t=await new THREE.TextureLoader().loadAsync(url);t.colorSpace=THREE.SRGBColorSpace;t.wrapS=t.wrapT=THREE.RepeatWrapping;t.repeat.setScalar(i===0?2:.5);t.anisotropy=4;surfaceArt.push(t);}
const shared=[...DINER_ART.map(id=>artTexture(id)),artTexture('dinerFloor',[1/2.4,1/2.4]),artTexture('dinerCounter',[1/.6,1/.6]),...surfaceArt];let sharedDisposed=0;
shared.forEach(t=>t.addEventListener('dispose',()=>sharedDisposed++));
const origin=new THREE.Vector3(-8000,0,0),siteBefore=siteFlood.count,annexBefore=annexFlood.count;
const renderer=new THREE.WebGLRenderer({canvas:document.querySelector<HTMLCanvasElement>('#view')!,antialias:true});
renderer.outputColorSpace=THREE.SRGBColorSpace;renderer.toneMapping=THREE.ACESFilmicToneMapping;renderer.toneMappingExposure=1.1;
const scene=new THREE.Scene();scene.background=new THREE.Color(0x293544);scene.fog=new THREE.FogExp2(0x293544,.0022);
const camera=new THREE.PerspectiveCamera(67,innerWidth/innerHeight,.05,500);
let start=performance.now(),diner=new Diner(origin),buildMs=performance.now()-start,firstBuildMs=buildMs;
scene.add(diner.group);if(surfaceArt.length===2)diner.setSurfaceArt(surfaceArt[0],surfaceArt[1]);
let quality:'high'|'low'='high',view='Arrival',recreations=0,doorOpen=true;diner.objs.door.rotation.y=Math.PI/2;
const views:Record<string,{eye:number[],target:number[],legal?:boolean}>= {
 Arrival:{eye:[7.25,1.62,4.9],target:[-2,1.4,-1]},Outside:{eye:[3.1,1.62,0],target:[-3.5,1.3,0]},
 Counter:{eye:[-2.8,1.62,0],target:[-4.5,1.3,.6]},Booth:{eye:[-1.0,1.62,3.05],target:[.7,.95,3.05]},
 Phone:{eye:[.4,1.62,8.05],target:[.4,1.48,8.55]},Clipping:{eye:[.75,1.62,1.35],target:[1.77,1.65,1.35]},
 Menu:{eye:[-3.72,1.62,-2.4],target:[-5.68,2.69,-2.4]},Sign:{eye:[15.5,1.62,-10.6],target:[13,1,-11]},
 Worst:{eye:[-5.25,1.85,6.8],target:[2,1.35,-1],legal:false},
};
function world(a:number[]){return new THREE.Vector3().fromArray(a).add(origin);}
function place(v:string){view=v;camera.position.copy(world(views[v].eye));camera.lookAt(world(views[v].target));document.querySelectorAll<HTMLButtonElement>('[data-view]').forEach(b=>b.setAttribute('aria-pressed',String(b.dataset.view===v)));nextInfo=0;}
function resize(){renderer.setPixelRatio(Math.min(devicePixelRatio,quality==='high'?1.6:1));renderer.setSize(innerWidth,innerHeight,false);camera.aspect=innerWidth/innerHeight;camera.updateProjectionMatrix();nextInfo=0;}
function apply(q:'high'|'low'){quality=q;setQuality(scene,q);document.querySelector('#high')!.setAttribute('aria-pressed',String(q==='high'));document.querySelector('#low')!.setAttribute('aria-pressed',String(q==='low'));resize();}
const required=['door','counter','coffee','waitress','driver','menu','clipping','payphone','jukebox','window','booth','sign','rig'];
const objects=['door','coffeePot','cup','clipping','payphoneHandset','waitressHead','driverHead'];
const isClear=(x:number,z:number)=>{const r=.27;return diner.zones.some(b=>b.enabled!==false&&x>=b.minX+r-1e-6&&x<=b.maxX-r+1e-6&&z>=b.minZ+r-1e-6&&z<=b.maxZ-r+1e-6)&&!diner.colliders.some(b=>x+r>b.minX&&x-r<b.maxX&&z+r>b.minZ&&z-r<b.maxZ);};
function overlap(){const seen=new Set([0]),q=[0];while(q.length){const a=diner.zones[q.pop()!];diner.zones.forEach((b,i)=>{if(!seen.has(i)&&Math.min(a.maxX,b.maxX)-Math.max(a.minX,b.minX)>.6&&Math.min(a.maxZ,b.maxZ)-Math.max(a.minZ,b.minZ)>.6){seen.add(i);q.push(i);}});}return seen.size===5;}
const paths:Record<string,number[][]>={arriveOutsideInsideStool:[[7.25,4.9],[5,3],[3.1,0],[.75,0],[-2.8,0]],insidePhone:[[.75,0],[-1,0],[-1,8.05],[.4,8.05]],stoolMenu:[[-2.8,0],[-2.8,-2.4],[-3.72,-2.4]]};
function walk(points:number[][]){const p=new Player(new THREE.PerspectiveCamera(),diner.colliders,diner.zones);p.place(origin.x+points[0][0],origin.z+points[0][1],0);let steps=0;
 for(const [lx,lz] of points.slice(1)){const x=lx+origin.x,z=lz+origin.z;let n=0;while(Math.hypot(x-p.pos.x,z-p.pos.z)>.035&&n++<1600){const dx=x-p.pos.x,dz=z-p.pos.z,l=Math.hypot(dx,dz);p.update(Math.min(.04,l/1.9),dx/l,dz/l,false);steps++;if(!isClear(p.pos.x,p.pos.z))return {pass:false,steps,stuck:[p.pos.x-origin.x,p.pos.z-origin.z]};}if(n>=1600)return {pass:false,steps,stuck:[p.pos.x-origin.x,p.pos.z-origin.z]};}return {pass:true,steps};}
const probes:Record<string,{eye:number[],aim?:number[]}>= {
 door:{eye:[3.1,1.62,0]},counter:{eye:[-2.8,1.62,0]},coffee:{eye:[-2.8,1.62,0]},waitress:{eye:[-2.8,1.62,0],aim:[-5.1,1.45,0]},driver:{eye:[-2.8,1.62,0]},
 menu:{eye:[-3.72,1.62,-2.4]},clipping:{eye:[.75,1.62,0]},payphone:{eye:[.4,1.62,8.05]},jukebox:{eye:[-1.1,1.62,-6.8]},window:{eye:[.5,1.62,-1.15]},
 booth:{eye:[-1,1.62,3.05]},sign:{eye:[13.55,1.62,-11]},rig:{eye:[9,1.62,-11.3],aim:[9,1.8,-14.15]},
};
let contract:[string,boolean][]=[];let pickResults:Record<string,any>={},routeResults:Record<string,any>={};
let disposal={exercised:false,geometries:0,disposedGeometries:0,materials:0,disposedMaterials:0,textures:0,disposedTextures:0};
function materials(){const ms=new Set<THREE.Material>();diner.group.traverse(o=>{const m=o as THREE.Mesh;if(m.isMesh)(Array.isArray(m.material)?m.material:[m.material]).forEach(v=>eachVariant(v,t=>ms.add(t)));});return ms;}
function contracts(){scene.updateMatrixWorld(true);const inter=new Interaction();for(const id of ['counter',...required.filter(x=>x!=='counter')])inter.add({id,object:diner.proxies[id],label:()=>id,use:()=>{}});
 pickResults={};for(const id of required){const d=probes[id],c=new THREE.PerspectiveCamera(67,1,.05,20);c.position.copy(world(d.eye));c.lookAt(d.aim?world(d.aim):diner.proxies[id].getWorldPosition(new THREE.Vector3()));c.updateMatrixWorld(true);const picked=inter.pick(c)?.id??null;pickResults[id]={picked,expected:id,standingClear:isClear(c.position.x,c.position.z),worldEye:c.position.toArray(),range:2.4};}
 // The broad counter is inert here. A nearer counter would incorrectly block coffee.
 inter.get('counter')!.label=()=>null;const cc=new THREE.PerspectiveCamera(67,1,.05,20);cc.position.copy(world(probes.coffee.eye));cc.lookAt(diner.proxies.coffee.getWorldPosition(new THREE.Vector3()));cc.updateMatrixWorld(true);const coffeeInert=inter.pick(cc)?.id==='coffee';
 // Always open the actual leaf while simulating entry, then restore the UI state.
 const oldDoor=diner.objs.door.rotation.y;diner.objs.door.rotation.y=Math.PI/2;diner.group.updateMatrixWorld(true);
 routeResults={};for(const [key,path] of Object.entries(paths)){routeResults[key]=walk(path);routeResults[key+'Reverse']=walk([...path].reverse());}diner.objs.door.rotation.y=oldDoor;
 let lights=0,hemis=0,hidden=0;diner.group.traverse(o=>{if((o as THREE.Light).isLight){lights++;if((o as THREE.HemisphereLight).isHemisphereLight)hemis++;}if(o.name.startsWith('diner-hit:')&&o.layers.mask===(1<<31)>>>0)hidden++;});
 const ms=materials(),signM=[...ms].filter(m=>m.name==='diner/sign') as THREE.MeshStandardMaterial[],glassM=[...ms].filter(m=>m.name==='diner/glass') as THREE.MeshStandardMaterial[];
 const wasLit=diner.signLit,k=diner.dawn;diner.setSignLit(false);const off=dinerFlood.pos[0].w===0&&dinerFlood.pos[1].w===0&&signM.every(m=>m.emissiveIntensity===0&&Math.abs(m.color.r-.32)<.001);
 diner.setSignLit(true);const on=dinerFlood.pos[0].w===8&&dinerFlood.pos[1].w===4&&signM.every(m=>m.emissiveIntensity===.65&&m.color.r===1);diner.setSignLit(wasLit);
 diner.setDawn(-4);const low=diner.dawn===0&&dinerFlood.pos[8].w===0;diner.setDawn(4);const high=diner.dawn===1&&dinerFlood.pos[8].w===8&&glassM.every(m=>m.emissiveIntensity===.06);diner.setDawn(k);
 const offsets=diner.zones.every(b=>b.minX<origin.x+30&&b.maxX>origin.x-30)&&diner.colliders.every(b=>b.minX<origin.x+30&&b.maxX>origin.x-30)&&dinerFlood.pos.every(p=>p.x<origin.x+30&&p.x>origin.x-30);
 contract=[['13 exact proxy IDs',required.every(k=>diner.proxies[k])&&Object.keys(diner.proxies).length===13],['13 hit meshes hidden on layer 31',hidden===13],['7 independent contract objects',objects.every(k=>diner.objs[k]?.parent&&diner.objs[k].userData.noMerge===true)],['5 exact zones',diner.zones.map(z=>z.id).sort().join(',')==='door,inside,lot,phone,walk'],['Zone overlaps > 0.6 m',overlap()],['Origin applied to zones, colliders and lights',offsets],['Walk anchors radius-clear',Object.entries(diner.anchors).filter(([k])=>k!=='truckPark').every(([,a])=>isClear(a.x,a.z)&&Number.isFinite(a.yaw))],['Truck placement anchor faces north; separate parking collider',diner.anchors.truckPark.yaw===0&&!isClear(diner.anchors.truckPark.x,diner.anchors.truckPark.z)],...Object.entries(routeResults).map(([key,result])=>['Player route '+key,result.pass] as [string,boolean]),...required.map(id=>['Interaction.pick '+id,pickResults[id].picked===id&&pickResults[id].standingClear] as [string,boolean]),['Inert counter does not mask coffee',coffeeInert],['Diner flood 10/10; other sets unchanged',dinerFlood.n===10&&dinerFlood.count===10&&siteFlood.count===siteBefore&&annexFlood.count===annexBefore],['Exactly one real HemisphereLight',lights===1&&hemis===1],['Sign off/on affects all quality variants',off&&on],['Dawn clamps and updates all quality variants',low&&high],['Exactly one Diner group',scene.children.filter(o=>o.name==='Diner').length===1],['Shared artwork never disposed',sharedDisposed===0]];
 if(surfaceArt.length===2)contract.push(['Round 8 shared sampler assignment', [...ms].filter(m=>m.name==='diner/vinyl'||m.name==='diner/panel').every(m=>(m as THREE.MeshStandardMaterial).map===surfaceArt[m.name==='diner/vinyl'?0:1])]);
 if(disposal.exercised)contract.push(['Retired geometry/material/canvas resources disposed once',disposal.geometries===disposal.disposedGeometries&&disposal.materials===disposal.disposedMaterials&&disposal.textures===disposal.disposedTextures]);
}
function recreate(){const lit=diner.signLit,dawn=diner.dawn;const gs=new Set<THREE.BufferGeometry>(),ms=materials(),ts=new Set<THREE.Texture>();
 diner.group.traverse(o=>{const m=o as THREE.Mesh;if(m.isMesh)gs.add(m.geometry);});for(const m of ms){const p=m as THREE.MeshStandardMaterial;if(p.map?.isCanvasTexture)ts.add(p.map);if(p.emissiveMap?.isCanvasTexture)ts.add(p.emissiveMap);}
 // Replaced fallback canvases are still owned, tracked, and disposed by Diner.
 disposal={exercised:true,geometries:gs.size,disposedGeometries:0,materials:ms.size,disposedMaterials:0,textures:ts.size,disposedTextures:0};gs.forEach(g=>g.addEventListener('dispose',()=>disposal.disposedGeometries++));ms.forEach(m=>m.addEventListener('dispose',()=>disposal.disposedMaterials++));ts.forEach(t=>t.addEventListener('dispose',()=>disposal.disposedTextures++));
 diner.dispose();diner.dispose();start=performance.now();diner=new Diner(origin);buildMs=performance.now()-start;scene.add(diner.group);if(surfaceArt.length===2)diner.setSurfaceArt(surfaceArt[0],surfaceArt[1]);diner.setSignLit(lit);diner.setDawn(dawn);diner.objs.door.rotation.y=doorOpen?Math.PI/2:0;recreations++;apply(quality);place(view);contracts();
}
let nextInfo=0,last=performance.now();
document.querySelectorAll<HTMLButtonElement>('[data-view]').forEach(b=>b.addEventListener('click',()=>place(b.dataset.view!)));
document.querySelector('#high')!.addEventListener('click',()=>{apply('high');contracts();});document.querySelector('#low')!.addEventListener('click',()=>{apply('low');contracts();});
document.querySelector('#sign')!.addEventListener('click',()=>{diner.setSignLit(!diner.signLit);document.querySelector('#sign')!.textContent='Sign '+(diner.signLit?'on':'off');nextInfo=0;});
document.querySelector('#door')!.addEventListener('click',()=>{doorOpen=!doorOpen;diner.objs.door.rotation.y=doorOpen?Math.PI/2:0;document.querySelector('#door')!.textContent='Door '+(doorOpen?'open':'closed');nextInfo=0;});
document.querySelector<HTMLInputElement>('#dawn')!.addEventListener('input',e=>{diner.setDawn(Number((e.target as HTMLInputElement).value));nextInfo=0;});
document.querySelector('#recreate')!.addEventListener('click',recreate);document.querySelector('#export')!.addEventListener('click',()=>{const a=document.createElement('a');a.href=URL.createObjectURL(new Blob([JSON.stringify((window as any).DINER_PREVIEW.report,null,2)],{type:'application/json'}));a.download='diner-preview.json';a.click();URL.revokeObjectURL(a.href);});window.addEventListener('resize',resize);
place('Arrival');apply('high');contracts();
function metrics(){const geos=new Set<THREE.BufferGeometry>(),ms=new Set<THREE.Material>();let meshes=0,totalTriangles=0;diner.group.traverse(o=>{const m=o as THREE.Mesh;if(!m.isMesh)return;meshes++;geos.add(m.geometry);totalTriangles+=(m.geometry.index?.count??m.geometry.attributes.position.count)/3;(Array.isArray(m.material)?m.material:[m.material]).forEach(x=>ms.add(x));});
 const calls=renderer.info.render.calls,triangles=renderer.info.render.triangles,checks=[...contract,['Draw calls <= 120',calls<=120],['No captured runtime errors',errors.length===0]] as [string,boolean][];
 const report={view,quality,viewport:[innerWidth,innerHeight],calls,triangles,scene:{meshes,geometries:geos.size,totalTriangles,materials:ms.size},gpu:{...renderer.info.memory},firstBuildMs,buildMs,recreations,origin:origin.toArray(),surfaceArt:surfaceArt.length===2?'round8':'procedural fallback',flood:{count:dinerFlood.count,n:dinerFlood.n},switches:{sign:diner.signLit,dawn:diner.dawn,doorOpen},viewStandingClear:views[view].legal!==false&&isClear(camera.position.x,camera.position.z),viewNote:view==='Worst'?'Diagnostic camera behind counter, outside customer zone':'Walking-height view',checks:checks.map(([name,pass])=>({name,pass})),pickResults,routeResults,disposal,errors};
 (window as any).DINER_PREVIEW={report};document.querySelector('#metrics')!.textContent=`${view} · ${quality.toUpperCase()} | ${calls}/120 calls, ${triangles.toLocaleString()} triangles | ${geos.size} geometries / ${ms.size} materials | Build ${buildMs.toFixed(1)} ms (first ${firstBuildMs.toFixed(1)}) | Recreate ${recreations}, flood ${dinerFlood.count}/${dinerFlood.n} | ${report.surfaceArt}${view==='Worst'?' | Diagnostic staff-side camera':''}`;
 document.querySelector('#contract-title')!.textContent=`Contract checks: ${checks.filter(x=>x[1]).length}/${checks.length} PASS`;
 document.querySelector('#checks')!.replaceChildren(...checks.map(([label,pass])=>{const d=document.createElement('div');d.className=pass?'pass':'fail';d.textContent=(pass?'PASS: ':'FAIL: ')+label;return d;}));
}
function frame(now:number){requestAnimationFrame(frame);const dt=Math.min((now-last)/1000,.05);last=now;diner.update(dt,now/1000);renderer.render(scene,camera);if(now>=nextInfo){metrics();nextInfo=now+350;}}
requestAnimationFrame(frame);window.addEventListener('pagehide',()=>{diner.dispose();surfaceArt.forEach(t=>t.dispose());renderer.dispose();});
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=8485)
    parser.add_argument('--write-only', action='store_true')
    parser.add_argument('--fallback', action='store_true')
    args = parser.parse_args()
    web = Path(__file__).resolve().parents[1]
    folder = Path(tempfile.mkdtemp(prefix='s47-diner-preview-'))
    (folder/'src').symlink_to(web/'src', target_is_directory=True)
    (folder/'node_modules').symlink_to(web/'node_modules', target_is_directory=True)
    files = ['tex_booth_vinyl.jpg', 'tex_wall_panel.jpg']
    surfaces = ['/src/assets/art/diner/'+name for name in files] if not args.fallback and all((web/'src/assets/art/diner'/name).is_file() for name in files) else []
    (folder/'index.html').write_text(HTML)
    (folder/'preview.ts').write_text(ENTRY.replace('__SURFACE_URLS__', json.dumps(surfaces)))
    config = {'root':str(folder), 'server':{'host':'127.0.0.1', 'port':args.port, 'strictPort':True, 'fs':{'allow':[str(web),str(folder)]}}, 'optimizeDeps':{'include':['three']}}
    (folder/'vite.config.mjs').write_text('export default '+json.dumps(config)+';\n')
    print(f'Diner preview: http://127.0.0.1:{args.port}/', flush=True)
    print(f'Midlertidige filer: {folder}', flush=True)
    print('Faste utsnitt, High/Low, Sign, Dawn, Door, Recreate og JSON. DINER_PREVIEW.report viser ekte Interaction/Player-kontroller.', flush=True)
    if args.write_only:
        return
    node = os.environ.get('DINER_NODE') or shutil.which('node') or str(Path.home()/'.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node')
    try:
        subprocess.run([node,str(web/'node_modules/vite/bin/vite.js'),'--config',str(folder/'vite.config.mjs')],cwd=folder,check=True)
    except KeyboardInterrupt:
        pass


if __name__ == '__main__':
    main()
