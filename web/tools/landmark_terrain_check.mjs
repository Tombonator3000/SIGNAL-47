// Reproduce: node tools/landmark_terrain_check.mjs [report.json]
// Native geometry only. Canvas drawing/art are stubbed; no browser or GPU is used.
import { readFile, writeFile, mkdir } from 'node:fs/promises';
import { createHash } from 'node:crypto';
import { fileURLToPath } from 'node:url';
import path from 'node:path';
import { build } from 'esbuild';

const web = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const output = path.resolve(process.argv[2] || path.join(web, 'production/kessler_20261006/terrain_review.json'));
const inputs = ['src/drive/oldRoadLandmarks.ts', 'src/drive/OldRoad.ts', 'src/drive/oldRoadLayout.ts',
  'src/drive/roadShape.ts', 'src/drive/roadTerrain.ts', 'src/world/kit.ts', 'tools/landmark_terrain_check.mjs'];
const hashes = async () => Object.fromEntries(await Promise.all(inputs.map(async p =>
  [p, createHash('sha256').update(await readFile(path.join(web, p))).digest('hex')])));
const before = await hashes();
const probe = String.raw`
import * as THREE from 'three';
import { OldRoad, oldFlood } from './src/drive/OldRoad.ts';
import { floodlit } from './src/world/kit.ts';
import { buildLandmarks } from './src/drive/oldRoadLandmarks.ts';
import { path, beside, sAt, heightAt, rnd, HALF } from './src/drive/oldRoadLayout.ts';
export default function run() {
  const canvas=()=>{const c={width:0,height:0},gradient={addColorStop(){}};
    const ctx=new Proxy({canvas:c,createLinearGradient:()=>gradient,createRadialGradient:()=>gradient,
      measureText:t=>({width:String(t).length*10}),getImageData:()=>({data:new Uint8ClampedArray(c.width*c.height*4)}),
      createImageData:(w,h)=>({data:new Uint8ClampedArray(w*h*4)})},{get:(t,k)=>k in t?t[k]:()=>{}});
    c.getContext=()=>ctx;return c;};
  globalThis.document={createElement:canvas};
  const road=new OldRoad(new THREE.Vector3()), ground=road.group.getObjectByName('desert');
  const plain=road.group.children.find(o=>o.isMesh&&o!==ground&&o.material===ground.material);
  if(!plain)throw Error('Actual OldRoad plain mesh not found');
  const pp=plain.geometry.attributes.position, pi=plain.geometry.index;
  let columns=1;while(columns<pp.count&&pp.getZ(columns)===pp.getZ(0))columns++;
  const rows=pp.count/columns;
  const cell=(x,n,coord)=>{let a=0,b=n-1;while(b-a>1){const m=(a+b)>>1;if(coord(m)<=x)a=m;else b=m;}return Math.min(n-2,a);};
  // Project into x/z, then interpolate y with barycentric weights of the ACTUAL
  // indexed triangles. This retains OldRoad's Float32 vertices and 160 m grid.
  const plainAt=(x,z)=>{const i=cell(x,columns,k=>pp.getX(k)),j=cell(z,rows,k=>pp.getZ(k*columns));
    const offset=(j*(columns-1)+i)*6,q=new THREE.Vector3(x,0,z);
    for(const k of [0,3]){const ids=[0,1,2].map(v=>pi.getX(offset+k+v));
      const vs=ids.map(v=>new THREE.Vector3(pp.getX(v),0,pp.getZ(v)));
      const b=THREE.Triangle.getBarycoord(q,...vs,new THREE.Vector3());
      if(b&&Math.min(b.x,b.y,b.z)>-1e-7)return b.x*pp.getY(ids[0])+b.y*pp.getY(ids[1])+b.z*pp.getY(ids[2]);}
    throw Error('Point not covered by actual plain triangles');};
  const result=buildLandmarks({beside,sAt,heightAt,rnd,std:(p,f=.012)=>floodlit(new THREE.MeshStandardMaterial(p),f,oldFlood)});
  const gate=beside(sAt(3.2),-17), mesh=result.group.getObjectByName('oldroad/Kessler-static');
  const pos=mesh.geometry.attributes.position,col=mesh.geometry.attributes.color,norm=mesh.geometry.attributes.normal;
  const verts=Array.from({length:pos.count},(_,i)=>({i,x:pos.getX(i)+gate.x,y:pos.getY(i),z:pos.getZ(i)+gate.z}));
  const colorIs=(i,hex)=>{const c=new THREE.Color(hex);return Math.max(Math.abs(col.getX(i)-c.r),Math.abs(col.getY(i)-c.g),Math.abs(col.getZ(i)-c.b))<1e-6;};
  const unique=vs=>[...new Map(vs.map(v=>[[v.x,v.y,v.z].join(','),v])).values()];
  const byColor=hex=>verts.filter(v=>colorIs(v.i,hex));
  const support=(name,points)=>{if(!points.length)throw Error('No support points: '+name);
    const gaps=points.map(v=>v.y-plainAt(v.x,v.z));return {name,points:points.length,minGap:Math.min(...gaps),maxGap:Math.max(...gaps),pass:Math.max(...gaps)<0};};
  const bottom=vs=>{const y=Math.min(...vs.map(v=>v.y));return unique(vs.filter(v=>v.y<y+1e-4));};
  const supports=[support('earth skirts and north cap',unique(byColor(0x83705b).filter(v=>v.y-heightAt(v.x,v.z)<-2)))];
  const wx=gate.x+13,wz=gate.z-301,wy=heightAt(wx,wz),hx=gate.x-15,hz=gate.z-306,hy=heightAt(hx,hz);
  for(const dx of [-2,2])for(const dz of [-2,2])supports.push(support('windmill foot '+dx+'/'+dz,
    unique(byColor(0x4f5352).filter(v=>Math.hypot(v.x-wx-dx,v.z-wz-dz)<.25&&v.y<wy-1.8))));
  supports.push(support('tank masonry bottom',bottom(byColor(0x646357))));
  supports.push(support('house foundation bottom',bottom(byColor(0x625d50))));
  for(const dx of [-3,3])supports.push(support('porch foot '+dx,unique(byColor(0x786950).filter(v=>
    Math.hypot(v.x-hx-dx,v.z-hz-6.4)<.2&&v.y<hy-1.8))));
  const earth=byColor(0x83705b),normalGroups=['left','right','north cap'].map(name=>{
    const vs=earth.filter(v=>name==='north cap'?v.z<gate.z-299.99:
      v.z>=gate.z-299.99&&(name==='left'?v.x<gate.x:v.x>=gate.x));
    return {name,vertices:vs.length,minNormalY:Math.min(...vs.map(v=>norm.getY(v.i))),pass:vs.length>0&&vs.every(v=>norm.getY(v.i)>0)};});
  const distance=(x,z,a,b)=>{const dx=b.x-a.x,dz=b.z-a.z,t=Math.max(0,Math.min(1,((x-a.x)*dx+(z-a.z)*dz)/(dx*dx+dz*dz)));return Math.hypot(x-a.x-t*dx,z-a.z-t*dz);};
  const pointRoad=(x,z)=>Math.min(...path.slice(1).map((b,i)=>distance(x,z,path[i],b)));
  const boxRoad=o=>{const corners=[[o.minX,o.minZ],[o.minX,o.maxZ],[o.maxX,o.minZ],[o.maxX,o.maxZ]];
    const pointBox=p=>Math.hypot(Math.max(o.minX-p.x,0,p.x-o.maxX),Math.max(o.minZ-p.z,0,p.z-o.maxZ));
    return Math.min(...path.slice(1).map((b,i)=>{const a=path[i];let lo=0,hi=1;
      for(const [a0,d,min,max] of [[a.x,b.x-a.x,o.minX,o.maxX],[a.z,b.z-a.z,o.minZ,o.maxZ]]){
        if(d===0){if(a0<min||a0>max){lo=2;break;}}else{const ts=[(min-a0)/d,(max-a0)/d].sort((x,y)=>x-y);lo=Math.max(lo,ts[0]);hi=Math.min(hi,ts[1]);}}
      return lo<=hi?0:Math.min(pointBox(a),pointBox(b),...corners.map(([x,z])=>distance(x,z,a,b)));}));};
  const clearances=result.obstacles.map(o=>({...o,asphaltClearance:(o.kind==='circle'?pointRoad(o.x,o.z)-o.r:boxRoad(o))-HALF}));
  let meshes=0,triangles=0,finite=true;result.group.traverse(o=>{if(!o.isMesh)return;meshes++;triangles+=(o.geometry.index?.count??o.geometry.attributes.position.count)/3;
    for(const a of Object.values(o.geometry.attributes))finite&&=Array.from(a.array).every(Number.isFinite);});
  const budget={meshes,triangles,finite,glints:result.glints.length,pass:meshes<=4&&triangles<15000&&finite&&result.glints.length===1};
  const chipVertices=byColor(0xa08b70).length+byColor(0x6c5e4c).length;
  const checks={support:supports.every(s=>s.pass),normals:normalGroups.every(n=>n.pass),asphalt:clearances.every(o=>o.asphaltClearance>0),budget:budget.pass};
  road.dispose();
  return {status:Object.values(checks).every(Boolean)?'PASS':'FAIL',checks,budget,gravelChipTriangles:chipVertices/3,supports,normalGroups,clearances,
    plain:{columns,rows,triangles:pi.count/3,method:'Barycentric y interpolation using the actual OldRoad plain indexed geometry; raster APIs only are stubbed.'},
    heightMismatchBeforeSupports:[280,300].map(d=>{const x=gate.x,z=gate.z-d;return {northFromGate:d,heightAt:heightAt(x,z),visiblePlain:plainAt(x,z),uncoveredTrackGap:heightAt(x,z)+.055-plainAt(x,z)};})};
}
`;
try {
  const bundle = await build({stdin:{contents:probe,resolveDir:web,loader:'ts'},bundle:true,platform:'node',format:'esm',write:false,logLevel:'silent',
    plugins:[{name:'geometry-only-art',setup(b){b.onLoad({filter:/[/\\]core[/\\]art\.ts$/},()=>({resolveDir:web,contents:
      "import * as THREE from 'three'; export const artTexture=()=>new THREE.Texture(); export const artLoaded=()=>false; export const artImage=()=>null; export const loadArtFor=async()=>{};"}));}}]});
  const {default:run}=await import('data:text/javascript;base64,'+Buffer.from(bundle.outputFiles[0].text).toString('base64'));
  const report=run(), after=await hashes();
  if(JSON.stringify(before)!==JSON.stringify(after))throw Error('Input changed while the probe ran; rerun against a stable snapshot');
  const evidence={...report,sourceSHA256:before,reproduce:'node tools/landmark_terrain_check.mjs',
    limits:['No browser, font, texture, shadow or performance validation.','Support samples use generated geometry vertices; plain interpolation uses the real OldRoad constructor geometry.','This is geometry evidence, not a gameplay test.']};
  await mkdir(path.dirname(output),{recursive:true});await writeFile(output,JSON.stringify(evidence,null,2)+'\n');
  console.log(JSON.stringify({status:report.status,triangles:report.budget.triangles,meshes:report.budget.meshes,output}));
  if(report.status!=='PASS')process.exitCode=1;
} catch(error) {console.error(error.message);process.exitCode=1;}
