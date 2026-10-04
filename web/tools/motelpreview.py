#!/usr/bin/env python3
"""Privat MotelFront-forhåndsvisning, generert i /tmp uten spillinngang eller Pages.

python3 tools/motelpreview.py --port 8483
Åpne URL-en manuelt. Synlig UI velger fem hovedutsnitt, Phone, High/Low og Recreate.
Ingen nettleser startes av skriptet. window.MOTEL_PREVIEW.report gir de samme
avgrensede kontraktkontrollene som panelet; dette er ingen kapitteltest.
"""
from pathlib import Path
import argparse
import json
import os
import shutil
import subprocess
import tempfile

HTML = r'''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>SIGNAL / 47: MotelFront preview</title><style>
*{box-sizing:border-box}html,body{margin:0;width:100%;height:100%;overflow:hidden;background:#091322;color:#f0e7d2;font:13px system-ui,sans-serif}canvas{display:block;width:100%;height:100%}header{position:absolute;top:0;left:0;right:0;padding:8px 12px;background:#101922e8;display:flex;align-items:center;gap:7px;flex-wrap:wrap}header strong{letter-spacing:.06em;margin-right:5px}nav{display:flex;gap:4px;flex-wrap:wrap}button{color:#ecdfc6;border:1px solid #8b826f;background:#263640;border-radius:4px;min-height:30px;padding:3px 9px;cursor:pointer}button[aria-pressed=true]{background:#d4ba82;color:#182a32}button:focus-visible{outline:3px solid #ead18b;outline-offset:2px}aside{position:absolute;right:12px;bottom:12px;width:min(450px,calc(100vw - 24px));background:#111c26ec;padding:9px 12px;border:1px solid #4e6269;line-height:1.45}summary{cursor:pointer;padding-top:4px}#metrics{font-variant-numeric:tabular-nums}#checks{max-height:40vh;overflow:auto;margin-top:7px}#checks div{border-top:1px solid #31424a;padding:3px 0}.pass{color:#bbd2a5}.fail{color:#ff9f8c}#error{position:absolute;left:12px;bottom:12px;background:#631f20;white-space:pre-wrap;max-width:55vw;padding:12px;display:none}@media(max-height:480px){header{padding:5px 8px;gap:5px}button{min-height:26px;padding:2px 7px}aside{font-size:11px;padding:6px 9px;bottom:8px;right:8px;width:355px}header strong{font-size:11px}}
</style></head><body><canvas id="view"></canvas><header><strong>MOTEL / PREVIEW</strong><nav aria-label="Views"><button data-view="Road">Road</button><button data-view="Lot">Lot</button><button data-view="Office">Office</button><button data-view="Room6">Room 6</button><button data-view="Overview">Overview</button><button data-view="Phone">Phone</button></nav><nav aria-label="Quality"><button id="high">High</button><button id="low">Low</button></nav><button id="office-light">Office light on</button><button id="room-light">Room light on</button><button id="interior">Interior visible</button><button id="door">Door closed</button><button id="recreate">Recreate</button></header><aside aria-live="polite"><div id="metrics">Loading startup artwork…</div><details><summary id="contract-title">Contract checks</summary><div id="checks"></div></details></aside><pre id="error"></pre><script type="module" src="/preview.ts"></script></body></html>'''

ENTRY = r'''import * as THREE from 'three';
import { MotelFront, courtFlood } from '/src/world/MotelFront.ts';
import { loadArt, artTexture } from '/src/core/art.ts';
import { setQuality } from '/src/core/quality.ts';
import { Interaction } from '/src/core/Interaction.ts';
import { Player } from '/src/player/Player.ts';
import { box, cyl, plane, flood as siteFlood } from '/src/world/kit.ts';

const errors:string[]=[];
const errorBox=document.querySelector<HTMLPreElement>('#error')!;
function error(e:unknown){errors.push(String(e));errorBox.style.display='block';errorBox.textContent=errors.join('\n');}
window.addEventListener('error',e=>error(e.message));window.addEventListener('unhandledrejection',e=>error(e.reason));
const warn=console.warn.bind(console);console.warn=(...a)=>{warn(...a);if(a.join(' ').includes('flood set full'))error(a.join(' '));};
await loadArt();
const renderer=new THREE.WebGLRenderer({canvas:document.querySelector<HTMLCanvasElement>('#view')!,antialias:true});
renderer.outputColorSpace=THREE.SRGBColorSpace;renderer.toneMapping=THREE.ACESFilmicToneMapping;renderer.toneMappingExposure=1.05;
const scene=new THREE.Scene();scene.background=new THREE.Color(0x091322);scene.fog=new THREE.FogExp2(0x111923,.0025);
const camera=new THREE.PerspectiveCamera(67,innerWidth/innerHeight,.05,350);
// Same ambient/moon values as SARO. These are preview context, not MotelFront lights.
const hemi=new THREE.HemisphereLight(0x22304f,0x2a1b10,.9),moon=new THREE.DirectionalLight(0x8ea4d8,.45);
moon.position.set(-40,80,-60);scene.add(hemi,moon,moon.target);
const context=new THREE.Group();context.name='Retained Exterior reference';scene.add(context);
plane(context,230,230,new THREE.MeshStandardMaterial({map:artTexture('desert',[35,35]),color:0x827664,roughness:1}),-35,-.62,30,0,-Math.PI/2);
box(context,8,.06,210,new THREE.MeshStandardMaterial({map:artTexture('asphalt',[1,25]),color:0x514f4b,roughness:.9}),-24,-.61,42);
const poleMat=new THREE.MeshStandardMaterial({color:0x52565b,roughness:.8});
for(const x of [-31.5,-29.3])cyl(context,.22,.22,9,poleMat,x,3.9,30,8);
plane(context,5.2,7.8,new THREE.MeshBasicMaterial({map:artTexture('sierra'),transparent:true,alphaTest:.02,depthWrite:false,side:THREE.DoubleSide,toneMapped:false}),-30.4,5.8,30.2,Math.PI/2-.5);
// The stars are a fixed preview backdrop; no sky module or active SARO world is built.
const stars:number[]=[];let seed=47;for(let i=0;i<500;i++){seed=(seed*1664525+1013904223)>>>0;const a=seed/4294967296*Math.PI*2;seed=(seed*1664525+1013904223)>>>0;const h=.08+seed/4294967296*.9;stars.push(-35+Math.cos(a)*210*Math.sqrt(1-h*h),h*210,35+Math.sin(a)*210*Math.sqrt(1-h*h));}
const sg=new THREE.BufferGeometry();sg.setAttribute('position',new THREE.Float32BufferAttribute(stars,3));context.add(new THREE.Points(sg,new THREE.PointsMaterial({color:0xaaaec0,size:.22,sizeAttenuation:true})));

const siteBefore=siteFlood.count;
const shared=[artTexture('concrete',[.5,.5]),artTexture('asphalt',[.2,.2]),artTexture('paper')];let sharedDisposed=0;
for(const t of shared)t.addEventListener('dispose',()=>sharedDisposed++);
let before=performance.now(),motel=new MotelFront(),buildMs=performance.now()-before,firstBuildMs=buildMs;scene.add(motel.group);
let quality:'high'|'low'='high',view='Road',recreations=0,doorOpen=false;
const views:Record<string,{eye:number[],target:number[]}>= {
 Road:{eye:[-27,1.62,6],target:[-43,1.7,34]},
 Lot:{eye:[-34.8,1.62,22.1],target:[-42.2,1.3,24.8]},
 Office:{eye:[-41.95,1.62,24.0],target:[-44.4,1.45,24.4]},
 Phone:{eye:[motel.anchors.officeInside.x,1.62,motel.anchors.officeInside.z],target:[-43.17,1.43,22.39]},
 Room6:{eye:[-38.8,1.62,52.5],target:[-41,1.2,55.6]},
 Overview:{eye:[-10,18,13],target:[-42,1,45]},
};
function place(v:string){view=v;camera.position.fromArray(views[v].eye);camera.lookAt(new THREE.Vector3().fromArray(views[v].target));document.querySelectorAll<HTMLButtonElement>('[data-view]').forEach(b=>b.setAttribute('aria-pressed',String(b.dataset.view===v)));}
function resize(){renderer.setPixelRatio(Math.min(devicePixelRatio,quality==='high'?1.6:1));renderer.setSize(innerWidth,innerHeight,false);camera.aspect=innerWidth/innerHeight;camera.updateProjectionMatrix();}
function apply(q:'high'|'low'){quality=q;setQuality(scene,q);document.querySelector('#high')!.setAttribute('aria-pressed',String(q==='high'));document.querySelector('#low')!.setAttribute('aria-pressed',String(q==='low'));resize();}
const required=['officeDoor','register','keyBoard','officePhone','envelope','message','room6Door','otherDoors','iceMachine','car','pool'];
const objects=['room6DoorLeaf','officePhoneHandset','envelope','message','key6Hook'];
const isClear=(x:number,z:number)=>{
 const r=.27;return motel.zones.some(b=>b.enabled!==false&&x>=b.minX+r-1e-6&&x<=b.maxX-r+1e-6&&z>=b.minZ+r-1e-6&&z<=b.maxZ-r+1e-6)&&!motel.colliders.some(b=>x+r>b.minX&&x-r<b.maxX&&z+r>b.minZ&&z-r<b.maxZ);
};
function overlap(){const seen=new Set([0]),q=[0];while(q.length){const a=motel.zones[q.pop()!];motel.zones.forEach((b,i)=>{if(!seen.has(i)&&Math.min(a.maxX,b.maxX)-Math.max(a.minX,b.minX)>.6&&Math.min(a.maxZ,b.maxZ)-Math.max(a.minZ,b.minZ)>.6){seen.add(i);q.push(i);}});}return seen.size===4;}
// This uses actual Player movement/resolution, not only an abstract point path.
function walk(points:number[][]){
 const p=new Player(new THREE.PerspectiveCamera(),motel.colliders,motel.zones);p.place(points[0][0],points[0][1],0);
 for(const [x,z] of points.slice(1)){let n=0;while(Math.hypot(x-p.pos.x,z-p.pos.z)>.035&&n++<1600){const dx=x-p.pos.x,dz=z-p.pos.z,l=Math.hypot(dx,dz);p.update(Math.min(.04,l/1.9),dx/l,dz/l,false);if(!isClear(p.pos.x,p.pos.z))return false;}if(n>=1600)return false;}return true;
}
const paths={entryToOffice:[[-29,6],[-35,20],[-39.65,24.1],[-42.6,24.1]],officeToRoom6:[[-42.6,24.1],[-39.65,24.1],[-39.65,55],[-39.5,55]]};
const picks:Record<string,{eye:number[],aim?:number[]}>= {
 officeDoor:{eye:[-39.7,1.62,24.1]},register:{eye:[-42.8,1.62,24.05]},keyBoard:{eye:[-43.35,1.62,23.12]},
 officePhone:{eye:[-42.6,1.62,24.1]},envelope:{eye:[-42.8,1.62,25.1]},message:{eye:[-42.8,1.62,26.03]},
 room6Door:{eye:[-39.5,1.62,55]},otherDoors:{eye:[-39.5,1.62,30],aim:[-41.015,1.03,30]},
 iceMachine:{eye:[-39.55,1.62,28.1]},car:{eye:[-33.15,1.62,34.6]},pool:{eye:[-35.95,1.62,55.5]},
};
let contract:[string,boolean][]=[];let pickResults:Record<string,unknown>={};let disposal={exercised:false,geometries:0,disposedGeometries:0,materials:0,disposedMaterials:0,textures:0,disposedTextures:0};
function contracts(){
 scene.updateMatrixWorld(true);const inter=new Interaction();
 // Register broad surfaces first to catch nearest-hit interference with papers.
 for(const id of ['officeDoor','otherDoors','car','pool',...required.filter(i=>!['officeDoor','otherDoors','car','pool'].includes(i))])inter.add({id,object:motel.proxies[id],label:()=>id,use:()=>{}});
 pickResults={};for(const id of required){const d=picks[id],c=new THREE.PerspectiveCamera(67,1,.05,20);c.position.fromArray(d.eye);const target=d.aim?new THREE.Vector3().fromArray(d.aim):motel.proxies[id].getWorldPosition(new THREE.Vector3());c.lookAt(target);c.updateMatrixWorld(true);const picked=inter.pick(c)?.id??null;pickResults[id]={picked,expected:id,standingClear:isClear(d.eye[0],d.eye[2])};}
 let lights=0;motel.group.traverse(o=>{if((o as THREE.Light).isLight)lights++;});
 const switchMaterials=new Map<string,THREE.MeshStandardMaterial>();
 motel.group.traverse(o=>{const m=o as THREE.Mesh;if(!m.isMesh)return;for(const mat of Array.isArray(m.material)?m.material:[m.material])if(['court/officeLamp','court/officeWindow','court/room6Lamp','court/room6Window'].includes(mat.name))switchMaterials.set(mat.name,mat as THREE.MeshStandardMaterial);});
 const old6=motel.room6LightOn,oldOffice=motel.officeLightOn;motel.setRoom6Light(false);motel.setOfficeLight(false);
 const offMaterials=switchMaterials.size===4&&[...switchMaterials.values()].every(m=>m.emissiveIntensity===0);
 const darkMap=switchMaterials.get('court/room6Window')?.map;
 const off=courtFlood.pos[6].w===0&&courtFlood.pos[0].w===0&&courtFlood.pos[1].w===0;
 motel.setRoom6Light(true);motel.setOfficeLight(true);const on=courtFlood.pos[6].w===4&&courtFlood.pos[0].w===6&&courtFlood.pos[1].w===4;
 const onMaterials=[...switchMaterials.values()].every(m=>m.emissiveIntensity>0)&&switchMaterials.get('court/room6Window')?.map!==darkMap;
 motel.setRoom6Light(old6);motel.setOfficeLight(oldOffice);
 contract=[
  ['11 exact proxy IDs',required.every(k=>motel.proxies[k])&&Object.keys(motel.proxies).length===11],
  ['5 required independent objects',objects.every(k=>motel.objs[k]?.parent&&motel.objs[k].userData.noMerge===true)],
  ['4 required zones',motel.zones.map(z=>z.id).sort().join(',')==='lot,office,officeDoor,walk'],
  ['Zone overlaps > 0.6 m',overlap()],
  ['Road entry x[-30,-28], z[2,10] radius-clear',[-30,-29,-28].every(x=>[2,6,10].every(z=>isClear(x,z)))],
  ['All anchors radius-clear',Object.values(motel.anchors).every(a=>isClear(a.x,a.z)&&Number.isFinite(a.yaw))],
  ['Player route: entry to office',walk(paths.entryToOffice)],
  ['Player route: office to Room 6',walk(paths.officeToRoom6)],
  ['Player route: Room 6 back to entry',walk([...paths.officeToRoom6].reverse())&&walk([...paths.entryToOffice].reverse())],
  ...required.map(id=>{const p=pickResults[id] as {picked:string;standingClear:boolean};return ['Interaction.pick: '+id,p.picked===id&&p.standingClear] as [string,boolean];}),
  ['Court flood 10/10; no SARO slots allocated',courtFlood.n===10&&courtFlood.count===10&&siteFlood.count===siteBefore],
  ['Module has no real lights',lights===0],
  ['Independent office and Room 6 switches',off&&on],
  ['Active quality materials switch lamps and window maps',offMaterials&&onMaterials],
  ['Exactly one MotelFront group',scene.children.filter(o=>o.name==='MotelFront').length===1],
  ['Shared startup textures never disposed',sharedDisposed===0],
 ];
 if(disposal.exercised)contract.push(['Retired geometries/materials/textures disposed',disposal.geometries===disposal.disposedGeometries&&disposal.materials===disposal.disposedMaterials&&disposal.textures===disposal.disposedTextures]);
}
function recreate(){
 const a=motel.officeLightOn,b=motel.room6LightOn,visible=motel.interior.visible;
 const gs=new Set<THREE.BufferGeometry>(),ms=new Set<THREE.Material>(),ts=new Set<THREE.Texture>();
 motel.group.traverse(o=>{const m=o as THREE.Mesh;if(!m.isMesh)return;gs.add(m.geometry);for(const mat of Array.isArray(m.material)?m.material:[m.material]){ms.add(mat);const p=mat as THREE.MeshStandardMaterial;if(p.map?.isCanvasTexture)ts.add(p.map);if(p.emissiveMap?.isCanvasTexture)ts.add(p.emissiveMap);}});
 disposal={exercised:true,geometries:gs.size,disposedGeometries:0,materials:ms.size,disposedMaterials:0,textures:ts.size,disposedTextures:0};
 gs.forEach(g=>g.addEventListener('dispose',()=>disposal.disposedGeometries++));ms.forEach(m=>m.addEventListener('dispose',()=>disposal.disposedMaterials++));ts.forEach(t=>t.addEventListener('dispose',()=>disposal.disposedTextures++));
 motel.dispose();motel.dispose();before=performance.now();motel=new MotelFront();buildMs=performance.now()-before;scene.add(motel.group);
 motel.setOfficeLight(a);motel.setRoom6Light(b);motel.interior.visible=visible;motel.objs.room6DoorLeaf.rotation.y=doorOpen?Math.PI/2:0;
 recreations++;apply(quality);place(view);contracts();
}
document.querySelectorAll<HTMLButtonElement>('[data-view]').forEach(b=>b.addEventListener('click',()=>place(b.dataset.view!)));
document.querySelector('#high')!.addEventListener('click',()=>{apply('high');contracts();});document.querySelector('#low')!.addEventListener('click',()=>{apply('low');contracts();});
document.querySelector('#office-light')!.addEventListener('click',()=>{motel.setOfficeLight(!motel.officeLightOn);document.querySelector('#office-light')!.textContent='Office light '+(motel.officeLightOn?'on':'off');});
document.querySelector('#room-light')!.addEventListener('click',()=>{motel.setRoom6Light(!motel.room6LightOn);document.querySelector('#room-light')!.textContent='Room light '+(motel.room6LightOn?'on':'off');});
document.querySelector('#interior')!.addEventListener('click',()=>{motel.interior.visible=!motel.interior.visible;document.querySelector('#interior')!.textContent='Interior '+(motel.interior.visible?'visible':'hidden');});
document.querySelector('#door')!.addEventListener('click',()=>{doorOpen=!doorOpen;motel.objs.room6DoorLeaf.rotation.y=doorOpen?Math.PI/2:0;document.querySelector('#door')!.textContent='Door '+(doorOpen?'open':'closed');});
document.querySelector('#recreate')!.addEventListener('click',recreate);window.addEventListener('resize',resize);
place('Road');apply('high');contracts();
let last=performance.now(),nextInfo=0;
function metrics(moduleCalls:number,moduleTriangles:number){
 const geos=new Set<THREE.BufferGeometry>(),mats=new Set<THREE.Material>();let meshes=0,triangles=0;
 motel.group.traverse(o=>{const m=o as THREE.Mesh;if(!m.isMesh||m.material===undefined)return;meshes++;geos.add(m.geometry);triangles+=(m.geometry.index?.count??m.geometry.attributes.position.count)/3;(Array.isArray(m.material)?m.material:[m.material]).forEach(x=>mats.add(x));});
 const checks=[...contract,['Module draw calls <= 50',moduleCalls<=50],['Constructor under 150 ms',buildMs<150],['No captured errors / overflow',errors.length===0]] as [string,boolean][];
 const report={view,quality,viewport:[innerWidth,innerHeight],calls:moduleCalls,triangles:moduleTriangles,scene:{meshes,geometries:geos.size,triangles:Math.round(triangles),materials:mats.size},gpu:{...renderer.info.memory},firstBuildMs,buildMs,recreations,flood:{count:courtFlood.count,n:courtFlood.n},switches:{room6:motel.room6LightOn,office:motel.officeLightOn,interior:motel.interior.visible,doorOpen},checks:checks.map(([name,pass])=>({name,pass})),pickResults,disposal,errors};
 (window as any).MOTEL_PREVIEW={report};
 document.querySelector('#metrics')!.textContent=`${view} · ${quality.toUpperCase()} | Module ${moduleCalls}/50 draw calls, ${moduleTriangles.toLocaleString()} triangles | Constructor ${buildMs.toFixed(1)} ms (first ${firstBuildMs.toFixed(1)}) | ${geos.size} geometries / ${mats.size} materials | Recreate ${recreations}, flood ${courtFlood.count}/${courtFlood.n} | Context excluded from budget`;
 document.querySelector('#contract-title')!.textContent=`Contract checks: ${checks.filter(x=>x[1]).length}/${checks.length} PASS`;
 document.querySelector('#checks')!.replaceChildren(...checks.map(([label,pass])=>{const d=document.createElement('div');d.className=pass?'pass':'fail';d.textContent=(pass?'PASS: ':'FAIL: ')+label;return d;}));
}
function frame(now:number){requestAnimationFrame(frame);const dt=Math.min((now-last)/1000,.05);last=now;motel.update(dt,now/1000);
 if(now>=nextInfo){context.visible=false;renderer.render(scene,camera);const calls=renderer.info.render.calls,tris=renderer.info.render.triangles;context.visible=true;metrics(calls,tris);nextInfo=now+400;}
 renderer.render(scene,camera);
}
requestAnimationFrame(frame);window.addEventListener('pagehide',()=>{motel.dispose();renderer.dispose();});
'''


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port',type=int,default=8483)
    parser.add_argument('--write-only',action='store_true')
    args=parser.parse_args()
    web=Path(__file__).resolve().parents[1]
    folder=Path(tempfile.mkdtemp(prefix='s47-motel-preview-'))
    (folder/'src').symlink_to(web/'src',target_is_directory=True)
    (folder/'node_modules').symlink_to(web/'node_modules',target_is_directory=True)
    (folder/'index.html').write_text(HTML)
    (folder/'preview.ts').write_text(ENTRY)
    config={'root':str(folder),'server':{'host':'127.0.0.1','port':args.port,'strictPort':True,'fs':{'allow':[str(web),str(folder)]}},'optimizeDeps':{'include':['three']}}
    (folder/'vite.config.mjs').write_text('export default '+json.dumps(config)+';\n')
    print(f'MotelFront preview: http://127.0.0.1:{args.port}/',flush=True)
    print(f'Midlertidige filer: {folder}',flush=True)
    print('Fem hovedutsnitt og Phone, High/Low, lys, dør, Interior og Recreate. Kontrakter i DOM og MOTEL_PREVIEW.report.',flush=True)
    if args.write_only:return
    node=os.environ.get('MOTEL_NODE') or shutil.which('node') or str(Path.home()/'.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node')
    try:subprocess.run([node,str(web/'node_modules/vite/bin/vite.js'),'--config',str(folder/'vite.config.mjs')],cwd=folder,check=True)
    except KeyboardInterrupt:pass


if __name__=='__main__':main()
