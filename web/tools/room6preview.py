#!/usr/bin/env python3
"""Selvstendig Room6-forhåndsvisning, kun genererte inngangsfiler under /tmp.

Kjør: python3 tools/room6preview.py --port 8482
Åpne URL-en manuelt. Scriptet starter Vite, men ingen nettleser eller spilltester.
Ingen endring av main.ts, vite.config.ts, byggets inngang eller Pages-innhold.
"""
from pathlib import Path
import argparse
import json
import os
import shutil
import subprocess
import tempfile

HTML = r'''<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>SIGNAL / 47: Room 6 preview</title>
<style>
*{box-sizing:border-box}html,body{margin:0;width:100%;height:100%;overflow:hidden;background:#081529;color:#f0e7d2;font:13px system-ui,sans-serif}canvas{display:block;width:100%;height:100%}header{position:absolute;top:0;left:0;right:0;padding:10px 12px;background:#101922de;display:flex;align-items:center;gap:10px;flex-wrap:wrap}header strong{letter-spacing:.08em;margin-right:5px}nav{display:flex;gap:5px;flex-wrap:wrap}button{color:#ecdfc6;border:1px solid #8b826f;background:#263640;border-radius:4px;min-height:32px;padding:4px 11px;cursor:pointer}button[aria-pressed=true]{background:#d4ba82;color:#182a32}button:focus-visible{outline:3px solid #ead18b;outline-offset:2px}aside{position:absolute;right:12px;bottom:12px;width:min(355px,calc(100vw - 24px));background:#111c26e8;padding:10px 12px;border:1px solid #4e6269;line-height:1.5}summary{cursor:pointer;padding-top:4px}#metrics{font-variant-numeric:tabular-nums}#checks{max-height:34vh;overflow:auto;margin-top:7px}#checks div{border-top:1px solid #31424a;padding:3px 0}.pass{color:#bbd2a5}.fail{color:#ff9f8c}#error{position:absolute;left:12px;bottom:12px;background:#631f20;white-space:pre-wrap;max-width:55vw;padding:12px;display:none}@media(max-height:480px){header{padding:6px 8px;gap:6px}button{min-height:29px;padding:3px 8px}aside{font-size:11px;padding:6px 9px;bottom:8px;right:8px}header strong{font-size:12px}}
</style></head><body><canvas id="view"></canvas>
<header><strong>ROOM 6 / PREVIEW</strong><nav aria-label="Camera views"><button data-view="Door">Door</button><button data-view="Table">Table</button><button data-view="Nora">Nora</button><button data-view="Window">Window</button></nav><nav aria-label="Quality"><button id="high">High</button><button id="low">Low</button></nav><button id="lamp">Lamp on</button><button id="recreate">Recreate</button></header>
<aside aria-live="polite"><div id="metrics">Starting renderer…</div><details><summary id="contract-title">Contract checks</summary><div id="checks"></div></details></aside><pre id="error"></pre><script type="module" src="/preview.ts"></script></body></html>'''

ENTRY = r'''import * as THREE from 'three';
import { Room6, motelFlood } from '/src/world/Room6.ts';
import { setQuality } from '/src/core/quality.ts';
import { Interaction } from '/src/core/Interaction.ts';

const errorBox=document.querySelector<HTMLPreElement>('#error')!;
const errors:string[]=[];
function report(e:unknown){errors.push(String(e));errorBox.style.display='block';errorBox.textContent=errors.join('\n');}
window.addEventListener('error',e=>report(e.message));
window.addEventListener('unhandledrejection',e=>report(e.reason));
const warn=console.warn.bind(console);console.warn=(...args)=>{warn(...args);if(args.join(' ').includes('flood set full'))report(args.join(' '));};
const origin=new THREE.Vector3(0,0,-8000);
const renderer=new THREE.WebGLRenderer({canvas:document.querySelector<HTMLCanvasElement>('#view')!,antialias:true});
renderer.outputColorSpace=THREE.SRGBColorSpace;
renderer.toneMapping=THREE.ACESFilmicToneMapping;renderer.toneMappingExposure=1.12;
const scene=new THREE.Scene();scene.background=new THREE.Color(0x081529);
const camera=new THREE.PerspectiveCamera(67,innerWidth/innerHeight,.035,65);
let room=new Room6(origin);scene.add(room.group);
let quality:'high'|'low'='high', view='Door',recreations=0;
const views:Record<string,{eye:number[],target:number[]}>= {
 Door:{eye:[-1.37,1.62,2.62],target:[.12,1.02,-.5]},
 Table:{eye:[-.66,1.62,.97],target:[.7,.9,1.55]},
 Nora:{eye:[-.62,1.55,.1],target:[.75,1.18,2.3]},
 Window:{eye:[-.2,1.62,-.6],target:[.78,1.4,2.96]},
};
function place(name:string){view=name;const v=views[name];camera.position.fromArray(v.eye).add(origin);camera.lookAt(new THREE.Vector3().fromArray(v.target).add(origin));document.querySelectorAll<HTMLButtonElement>('[data-view]').forEach(b=>b.setAttribute('aria-pressed',String(b.dataset.view===name)));}
function resize(){renderer.setPixelRatio(Math.min(devicePixelRatio,quality==='high'?1.6:1));renderer.setSize(innerWidth,innerHeight,false);camera.aspect=innerWidth/innerHeight;camera.updateProjectionMatrix();}
function apply(q:'high'|'low'){quality=q;setQuality(scene,q);document.querySelector('#high')!.setAttribute('aria-pressed',String(q==='high'));document.querySelector('#low')!.setAttribute('aria-pressed',String(q==='low'));resize();}
document.querySelectorAll<HTMLButtonElement>('[data-view]').forEach(b=>b.addEventListener('click',()=>place(b.dataset.view!)));
document.querySelector('#high')!.addEventListener('click',()=>apply('high'));document.querySelector('#low')!.addEventListener('click',()=>apply('low'));
document.querySelector('#lamp')!.addEventListener('click',()=>{room.setLamp(!room.lampOn);document.querySelector('#lamp')!.textContent='Lamp '+(room.lampOn?'on':'off');});
document.querySelector('#recreate')!.addEventListener('click',()=>{const wasOn=room.lampOn;room.dispose();room=new Room6(origin);scene.add(room.group);room.setLamp(wasOn);recreations++;apply(quality);place(view);});
window.addEventListener('resize',resize);place('Door');apply('high');

const required=['door','nora','table','fieldCard','letter','correction','photo','window','phone','tv','bed','lamp','bathroom'];
function clear(x:number,z:number){const r=.27;return room.zones.some(b=>b.enabled!==false&&x>=b.minX+r&&x<=b.maxX-r&&z>=b.minZ+r&&z<=b.maxZ-r)&&!room.colliders.some(b=>x+r>b.minX&&x-r<b.maxX&&z+r>b.minZ&&z-r<b.maxZ);}
function routeClear(){const points=[[-1.4,4.3],[-1.4,2.2],[-1.4,1.28],[-.42,1.28]];for(let s=1;s<points.length;s++){const a=points[s-1],b=points[s];for(let k=0;k<=80;k++){const t=k/80;if(!clear(origin.x+a[0]+(b[0]-a[0])*t,origin.z+a[1]+(b[1]-a[1])*t))return false;}}return true;}
function overlap(){const edges:Record<number,number[]>={};for(let i=0;i<room.zones.length;i++){edges[i]=[];for(let k=0;k<room.zones.length;k++){if(i===k)continue;const a=room.zones[i],b=room.zones[k];if(Math.min(a.maxX,b.maxX)-Math.max(a.minX,b.minX)>.6&&Math.min(a.maxZ,b.maxZ)-Math.max(a.minZ,b.minZ)>.6)edges[i].push(k);}}const seen=new Set<number>([0]);const todo=[0];while(todo.length)for(const k of edges[todo.pop()!])if(!seen.has(k)){seen.add(k);todo.push(k);}return seen.size===room.zones.length;}
function documentPick(id:string){
 const inter=new Interaction();
 // Table is deliberately registered first and inert. Interaction.pick returns
 // null immediately if it hits that surface before a document.
 for(const key of ['table',...required.filter(x=>x!=='table')])inter.add({id:key,object:room.proxies[key],label:()=>key==='table'?null:key,use:()=>{}});
 const c=new THREE.PerspectiveCamera(67,1,.035,20);const a=room.anchors.talk;
 c.position.set(a.x,origin.y+1.62,a.z);c.lookAt(room.proxies[id].getWorldPosition(new THREE.Vector3()));c.updateMatrixWorld(true);
 return clear(a.x,a.z)&&inter.pick(c)?.id===id;
}
function metrics(){
 scene.updateMatrixWorld(true);
 const geos=new Set<THREE.BufferGeometry>(),mats=new Set<THREE.Material>();let meshes=0,triangles=0;
 room.group.traverse(o=>{const m=o as THREE.Mesh;if(!m.isMesh)return;meshes++;geos.add(m.geometry);triangles+=(m.geometry.index?.count??m.geometry.attributes.position.count)/3;(Array.isArray(m.material)?m.material:[m.material]).forEach(x=>mats.add(x));});
 const near=(p:THREE.Vector3)=>Math.abs(p.x-origin.x)<10&&Math.abs(p.z-origin.z)<10&&Number.isFinite(p.y);
 const checks:[string,boolean][]=[
  ['13 exact proxy IDs',required.every(x=>room.proxies[x])&&Object.keys(room.proxies).length===13],
  ['Proxies use translated world placement',Object.values(room.proxies).every(p=>near(p.getWorldPosition(new THREE.Vector3())))],
  ['3 documents and lamp remain separate', ['fieldCard','letter','correction','lamp'].every(k=>room.objs[k]?.parent===room.interior&&room.objs[k].userData.noMerge===true)],
  ['Nora and movable head remain separate',room.objs.nora?.parent===room.interior&&room.objs.noraHead?.parent===room.objs.nora],
  ['World-space zones and colliders', [...room.zones,...room.colliders].every(b=>Math.abs(b.minZ-origin.z)<10&&Math.abs(b.maxZ-origin.z)<10&&Math.abs(b.minX-origin.x)<10&&Math.abs(b.maxX-origin.x)<10)],
  ['Zone connections overlap by > 0.6 m',overlap()],
  ['Arrival / talk / exit anchors clear',Object.values(room.anchors).every(a=>clear(a.x,a.z)&&Number.isFinite(a.yaw))],
  ['Door-to-table route clear (radius 0.27)',routeClear()],
  ['Interaction: fieldCard beats inert table',documentPick('fieldCard')],
  ['Interaction: letter beats inert table',documentPick('letter')],
  ['Interaction: correction beats inert table',documentPick('correction')],
  ['4 / 6 motel flood slots, world positions',motelFlood.n===6&&motelFlood.count===4&&motelFlood.pos.slice(0,4).every(p=>Math.abs(p.z-origin.z)<10)],
  ['Exactly one Room6 group after Recreate',scene.children.filter(o=>o.name==='Room6').length===1],
  ['Lamp flood matches its switch',motelFlood.pos[0].w===(room.lampOn?3.3:0)],
  ['Room has its own hemisphere and moon',room.lights.hemi.parent===room.lights.group&&room.lights.moon.parent===room.lights.group&&room.lights.group.parent===room.group],
  ['Draw calls within 120',renderer.info.render.calls<=120],
  ['No captured error / flood overflow',errors.length===0],
 ];
 document.querySelector('#metrics')!.textContent=`${view} · ${quality.toUpperCase()} · Lamp ${room.lampOn?'on':'off'} | Draw calls ${renderer.info.render.calls}/120 | Triangles ${renderer.info.render.triangles.toLocaleString()} | Scene ${meshes} meshes / ${geos.size} geometries / ${Math.round(triangles).toLocaleString()} triangles / ${mats.size} materials | GPU ${renderer.info.memory.geometries} geometries, ${renderer.info.memory.textures} textures | Recreate ${recreations}, flood ${motelFlood.count}/${motelFlood.n}`;
 document.querySelector('#contract-title')!.textContent=`Contract checks: ${checks.filter(x=>x[1]).length}/${checks.length} PASS`;
 document.querySelector('#checks')!.replaceChildren(...checks.map(([label,pass])=>{const el=document.createElement('div');el.className=pass?'pass':'fail';el.textContent=(pass?'PASS: ':'FAIL: ')+label;return el;}));
}
let last=performance.now(),nextInfo=0;
function frame(now:number){requestAnimationFrame(frame);const dt=Math.min((now-last)/1000,.05);last=now;room.update(dt,now/1000);renderer.render(scene,camera);if(now>=nextInfo){metrics();nextInfo=now+250;}}
requestAnimationFrame(frame);
window.addEventListener('pagehide',()=>{room.dispose();renderer.dispose();});
'''


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port',type=int,default=8482)
    parser.add_argument('--write-only',action='store_true',help='Skriv midlertidige filer uten å starte serveren')
    args=parser.parse_args()
    web=Path(__file__).resolve().parents[1]
    folder=Path(tempfile.mkdtemp(prefix='s47-room6-preview-'))
    (folder/'src').symlink_to(web/'src',target_is_directory=True)
    (folder/'node_modules').symlink_to(web/'node_modules',target_is_directory=True)
    (folder/'index.html').write_text(HTML)
    (folder/'preview.ts').write_text(ENTRY)
    config={'root':str(folder),'server':{'host':'127.0.0.1','port':args.port,'strictPort':True,'fs':{'allow':[str(web),str(folder)]}},'optimizeDeps':{'include':['three']}}
    (folder/'vite.config.mjs').write_text('export default '+json.dumps(config)+';\n')
    print(f'Room6 preview: http://127.0.0.1:{args.port}/',flush=True)
    print(f'Midlertidige filer: {folder}',flush=True)
    print('Ingen endringer av spillinngang, Vite-konfigurasjon eller Pages-bygg.',flush=True)
    if args.write_only:return
    node=os.environ.get('MAP_NODE') or shutil.which('node') or str(Path.home()/'.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node')
    try:
        subprocess.run([node,str(web/'node_modules/vite/bin/vite.js'),'--config',str(folder/'vite.config.mjs')],cwd=folder,check=True)
    except KeyboardInterrupt:pass


if __name__=='__main__':main()
