import * as THREE from 'three';
import { artImage, artTexture, PHOTO_1947 } from '../core/art';
import { eachVariant } from '../core/quality';
import { box, cyl, plane, rod, noMerge, mergeStatic, floodSet, floodlit, addFlood, setFlood } from './kit';
import type { Collider } from './ControlRoom';
import type { Zone } from '../player/Player';

// All Night: local metres, north -Z. World normally supplies (-8000,0,0).
// Load DINER_ART before construction. The five diner images are the only required art.
// The SARO truck is owned by World; this module supplies its parking anchor/collider.
export const dinerFlood = floodSet(10, 'diner', .14);
let floodOwner: Diner | null = null;
type Anchor = { x: number; z: number; yaw: number };
export type DinerAnchors = Record<'truckPark'|'arrive'|'outside'|'inside'|'stool'|'phone', Anchor>;
const V = (x:number,y:number,z:number)=>new THREE.Vector3(x,y,z);
const yawTo = (x:number,z:number,tx:number,tz:number)=>Math.atan2(x-tx,z-tz);

// Position-based UVs, in local metres. Repeat scales belong to the sampler.
function metreUV(m:THREE.Mesh){
  m.updateMatrix();const p=m.geometry.attributes.position,n=m.geometry.attributes.normal,uv=m.geometry.attributes.uv;
  const v=new THREE.Vector3(),q=new THREE.Vector3(),nm=new THREE.Matrix3().getNormalMatrix(m.matrix);
  for(let i=0;i<p.count;i++){v.fromBufferAttribute(p,i).applyMatrix4(m.matrix);q.fromBufferAttribute(n,i).applyMatrix3(nm);
    if(Math.abs(q.y)>.5)uv.setXY(i,v.x,v.z);else if(Math.abs(q.x)>.5)uv.setXY(i,-v.z,v.y);else uv.setXY(i,v.x,v.y);}
  return m;
}

export class Diner {
  readonly group=new THREE.Group();
  readonly interior=new THREE.Group();
  readonly proxies:Record<string,THREE.Object3D>={};
  readonly objs:Record<string,THREE.Object3D>={};
  readonly zones:Zone[]=[];
  readonly colliders:Collider[]=[];
  readonly anchors:DinerAnchors;
  readonly flood=dinerFlood;
  readonly lights:{hemi:THREE.HemisphereLight};
  signLit=true;
  dawn=0;
  private readonly origin:THREE.Vector3;
  private readonly shell=new THREE.Group();
  private readonly fixed=new THREE.Group();
  private readonly materials=new Set<THREE.Material>();
  private readonly textures=new Set<THREE.Texture>();
  private readonly hit=new THREE.MeshBasicMaterial({visible:false});
  private m!:ReturnType<Diner['makeMaterials']>;
  private readonly slots:Record<string,number>={};
  private disposed=false;

  /** onRoad: the diner stands in the road area (World.ts), whose highway and land go round it,
   *  so it leaves out its own strip of highway and ground. */
  constructor(origin:THREE.Vector3,private readonly onRoad=false){
    this.origin=origin.clone();this.group.name='Diner';this.group.position.copy(origin);
    this.interior.name='DinerInterior';this.group.add(this.shell,this.interior);this.interior.add(this.fixed);
    this.materials.add(this.hit);floodOwner=this;dinerFlood.count=0;
    for(const p of dinerFlood.pos)p.set(0,-999,0,0);
    this.m=this.makeMaterials();for(const [id,m] of Object.entries(this.m))m.name='diner/'+id;
    this.landscape();this.building();this.counter();this.booths();this.papersAndPhone();this.people();this.signAndRig();
    mergeStatic(this.shell);mergeStatic(this.fixed);
    const hemi=new THREE.HemisphereLight(0x6d87a4,0x55402f,.55);hemi.name='Diner ambient';this.group.add(hemi);this.lights={hemi};
    this.slots.red=this.light(13,2.2,-10,10,0xff5341);this.slots.cyan=this.light(12.8,1.8,-10,5,0x52ddd7);
    for(const z of [-5,0,5])this.light(-2.0,2.8,z,4.5,0xddeaff);
    this.slots.hatch=this.light(-5.25,2.1,-2.4,2.8,0xffd391);
    this.light(2.5,1.7,-4.8,4,0xffcf98);this.light(2.5,1.7,4.8,4,0xffcf98);
    this.slots.dawn=this.light(16,7,3,0,0xaac7df);
    this.slots.phone=this.light(.5,2.6,8.1,1.8,0xddddd2);
    this.zone('lot',2.8,16.7,-16,16);this.zone('walk',1.55,3.7,-9.4,9.4);
    this.zone('door',.7,3.3,-.6,.6);this.zone('inside',-4.05,1.8,-8.8,8.8);
    this.zone('phone',-1.6,1.8,7.4,8.8);
    this.col(7.85,10.15,3.2,8.8);
    const a=(x:number,z:number,yaw:number):Anchor=>({x:origin.x+x,z:origin.z+z,yaw});
    this.anchors={truckPark:a(9,6,0),arrive:a(7.25,4.9,yawTo(7.25,4.9,2,0)),outside:a(3.1,0,Math.PI/2),
      inside:a(.75,0,Math.PI/2),stool:a(-2.8,0,Math.PI/2),phone:a(.4,8.05,Math.PI)};
    this.setDawn(.25);this.group.updateMatrixWorld(true);
  }

  private mat(p:THREE.MeshStandardMaterialParameters,falloff=.14){const m=floodlit(new THREE.MeshStandardMaterial(p),falloff,dinerFlood);this.materials.add(m);return m;}
  private tex(w:number,h:number,draw:(g:CanvasRenderingContext2D)=>void,repeat?:[number,number]){
    const c=document.createElement('canvas');c.width=w;c.height=h;draw(c.getContext('2d')!);
    const t=new THREE.CanvasTexture(c);t.colorSpace=THREE.SRGBColorSpace;t.anisotropy=4;
    if(repeat){t.wrapS=t.wrapT=THREE.RepeatWrapping;t.repeat.set(...repeat);}this.textures.add(t);return t;
  }
  private makeMaterials(){
    // Procedural reserves for future dinerBooth and dinerWall ArtIds (round 8).
    // setSurfaceArt accepts externally owned samplers once Claude wires those IDs.
    const panel=this.tex(128,128,g=>{g.fillStyle='#967047';g.fillRect(0,0,128,128);for(let x=0;x<128;x+=16){g.fillStyle='#705439';g.fillRect(x,0,1,128);g.fillStyle='#b08957';g.fillRect(x+2,0,1,128);for(let n=0;n<7;n++){g.fillStyle='#8c653f';g.fillRect(x+4+n,((n*31+x*3)%128),1,32);}}},[.5,.5]);
    const vinyl=this.tex(128,128,g=>{g.fillStyle='#833c35';g.fillRect(0,0,128,128);g.strokeStyle='#632f2b';g.lineWidth=2;for(const y of [0,64,128]){g.beginPath();g.moveTo(0,y);g.lineTo(128,y);g.stroke();}for(const x of [0,64,128]){g.beginPath();g.moveTo(x,0);g.lineTo(x,128);g.stroke();}for(const x of [32,96])for(const y of [32,96]){g.fillStyle='#a04a3c';g.beginPath();g.arc(x,y,3,0,7);g.fill();}},[2,2]);
    const gravel=this.tex(128,128,g=>{g.fillStyle='#9a8d78';g.fillRect(0,0,128,128);let s=1947;for(let i=0;i<2600;i++){s=(s*1664525+1013904223)>>>0;const x=s%128;s=(s*1664525+1013904223)>>>0;g.fillStyle=i%3?'#a69982':'#7b7469';g.fillRect(x,s%128,1,1);}},[.25,.25]);
    const sign=this.tex(1024,512,g=>{
      g.drawImage(artImage('dinerSign'),0,0,1024,512);g.textAlign='center';g.fillStyle='#273e43';g.font='bold 50px sans-serif';g.fillText('MESA DINER',512,230,555);
      g.font='18px sans-serif';g.fillText('OPEN ALL NIGHT',512,258,400);
    });
    const menu=this.tex(1024,512,g=>{
      g.drawImage(artImage('dinerMenu'),0,0,1024,512);g.fillStyle='#e5dec5';g.textAlign='left';g.font='39px monospace';
      ['COFFEE .35','TWO EGGS ANY STYLE 1.95','HOTCAKES 1.75','GREEN CHILE STEW 2.95','PIE .95'].forEach((s,i)=>g.fillText(s,64,102+i*70,896));
    });
    const clipping=this.tex(1024,1536,g=>{
      g.fillStyle='#d3c8ad';g.fillRect(0,0,1024,1536);g.fillStyle='#302f2a';g.textAlign='center';g.font='bold 54px Georgia,serif';g.fillText('PECOS VALLEY SENTINEL',512,106,932);
      g.fillRect(48,137,928,3);g.font='bold 64px Georgia,serif';g.fillText('LIGHT HELD OVER MESA',512,228,928);g.fillText('FOR AN HOUR',512,304,928);
      g.font='31px Georgia,serif';['Ranchers on the old survey road watched a steady','glow; Army field office cites weather equipment'].forEach((s,i)=>g.fillText(s,512,365+i*39,928));
      {const [sx,sy,sw,sh]=PHOTO_1947.B;g.drawImage(artImage('photo1947'),sx,sy,sw,sh,72,464,880,660);}   // crop B of the 1947 master (round 10)
      g.font='italic 29px Georgia,serif';g.fillText('Seen from the Kessler ranch, 3 a.m.',512,1178,880);
      g.fillStyle='#7b7463';for(let col=0;col<3;col++)for(let i=0;i<12;i++)g.fillRect(72+col*306,1230+i*19,260-(i%4)*11,3);
      // The clipping has no printed date. The separate pencil note is on its frame.
    });
    const labels=this.tex(1024,512,g=>{
      g.fillStyle='#ddd4b8';g.fillRect(0,0,1024,512);g.fillStyle='#394847';g.textAlign='center';g.textBaseline='middle';
      g.font='bold 57px sans-serif';g.fillText('RESTROOM',256,70);g.fillText('RESTROOM',768,70);g.font='bold 60px sans-serif';g.fillText('TELEPHONE',256,204);
      g.font='italic 55px Georgia,serif';g.fillText("July '47",768,204);g.font='bold 58px sans-serif';g.fillText('OPEN',128,340);g.font='28px monospace';g.fillText('PUSH',384,340);
      g.font='20px monospace';g.fillText('COINS',650,329);g.fillText('RETURN',845,329);g.font='24px monospace';g.fillText('SELECTIONS',256,448);
    });
    return {
      panel:this.mat({map:panel,roughness:.85}),vinyl:this.mat({map:vinyl,roughness:.46}),
      floor:this.mat({map:artTexture('dinerFloor',[1/2.4,1/2.4]),roughness:.85}),counter:this.mat({map:artTexture('dinerCounter',[1/.6,1/.6]),roughness:.45}),
      gravel:this.mat({map:gravel,color:0xb8ab93,roughness:1},.02),asphalt:this.mat({color:0x35363a,roughness:.9},.02),
      cream:this.mat({color:0xd8c9ad,roughness:.9}),red:this.mat({color:0x8f3f35,roughness:.6}),mint:this.mat({color:0x7b9a89,roughness:.65}),
      steel:this.mat({color:0xa0a9aa,roughness:.32,metalness:.75}),black:this.mat({color:0x222628,roughness:.75}),wood:this.mat({color:0x5d422e,roughness:.9}),
      glass:this.mat({color:0x627584,transparent:true,opacity:.075,roughness:.12,metalness:.08,depthWrite:false,side:THREE.DoubleSide}),
      sign:this.mat({map:sign,transparent:true,alphaTest:.035,depthWrite:false,roughness:.6,emissive:0xffffff,emissiveMap:sign,emissiveIntensity:.65},.04),
      menu:this.mat({map:menu,roughness:1}),clipping:this.mat({map:clipping,roughness:1}),labels:this.mat({map:labels,roughness:1}),
      tube:this.mat({color:0xe6e5cd,emissive:0xddeaff,emissiveIntensity:.75,roughness:.7}),
      amber:this.mat({color:0xeac477,emissive:0xffb77a,emissiveIntensity:.45,roughness:.65}),
      skin:this.mat({color:0xbd9c80,roughness:1,flatShading:true}),hair:this.mat({color:0x52483d,roughness:1,flatShading:true}),
      uniform:this.mat({color:0x5d918c,roughness:1,flatShading:true}),jacket:this.mat({color:0x726449,roughness:1,flatShading:true}),
      trousers:this.mat({color:0x38434b,roughness:1,flatShading:true}),coffee:this.mat({color:0x2e190d,roughness:.22}),
      yellow:this.mat({color:0xc6a34f,roughness:1}),mesa:this.mat({color:0x26323e,roughness:1},.005),
    };
  }
  private light(x:number,y:number,z:number,w:number,c:number){return addFlood(x+this.origin.x,y+this.origin.y,z+this.origin.z,w,c,dinerFlood);}
  private col(x0:number,x1:number,z0:number,z1:number){this.colliders.push({minX:x0+this.origin.x,maxX:x1+this.origin.x,minZ:z0+this.origin.z,maxZ:z1+this.origin.z});}
  private zone(id:string,x0:number,x1:number,z0:number,z1:number){this.zones.push({id,enabled:true,minX:x0+this.origin.x,maxX:x1+this.origin.x,minZ:z0+this.origin.z,maxZ:z1+this.origin.z});}
  private model(id:string,parent:THREE.Object3D,x:number,y:number,z:number){const g=new THREE.Group();g.name='diner:'+id;g.position.set(x,y,z);noMerge(g);parent.add(g);this.objs[id]=g;return g;}
  private proxy(id:string,w:number,h:number,d:number,x:number,y:number,z:number){
    const g=new THREE.Group();g.name='diner:'+id;g.position.set(x,y,z);noMerge(g);
    const hit=box(g,w,h,d,this.hit);hit.layers.set(31);hit.name='diner-hit:'+id;
    // Interaction's ray uses layer0. A non-rendered layer0 group forwards directly
    // to its layer31 hit mesh; the normal recursive cast then skips that child.
    g.raycast=(ray,hits)=>hit.raycast(ray,hits);this.group.add(g);this.proxies[id]=g;return g;
  }
  private label(p:THREE.Object3D,w:number,h:number,x:number,y:number,z:number,r:[number,number,number,number],ry=0){
    const f=plane(p,w,h,this.m.labels,x,y,z,ry),uv=f.geometry.attributes.uv;
    for(let i=0;i<uv.count;i++)uv.setXY(i,(r[0]+uv.getX(i)*r[2])/1024,1-(r[1]+(1-uv.getY(i))*r[3])/512);return f;
  }
  private landscape(){
    const s=this.shell,m=this.m;
    const slab=(x0:number,x1:number,z0:number,z1:number,mat:THREE.Material)=>metreUV(box(s,x1-x0,.14,z1-z0,mat,(x0+x1)/2,-.07,(z0+z1)/2));
    // Floor plates meet at exact edges. Zones overlap, render meshes never do.
    slab(3.6,16,-16,16,m.gravel);slab(2,3.6,-16,-9.4,m.gravel);slab(2,3.6,9.4,16,m.gravel);
    slab(2,3.6,-9.4,9.4,m.cream);slab(1.8,2,-.6,.6,m.cream);
    if(!this.onRoad){slab(16,24,-180,180,m.asphalt);
      for(const x of [19.9,20.1])box(s,.08,.006,360,m.yellow,x,.004,0);
      for(const x of [16.3,23.7])box(s,.1,.006,360,m.cream,x,.004,0);}
    // Exponentially spaced rings keep cells short near the building.
    const geo=new THREE.RingGeometry(.4,360,64,72),p=geo.attributes.position,uv=geo.attributes.uv;
    for(let i=0;i<p.count;i++){const x=p.getX(i),y=p.getY(i),r=Math.hypot(x,y),k=.4*Math.pow(360/.4,(r-.4)/(360-.4))/r;p.setXY(i,x*k,y*k);}
    geo.rotateX(-Math.PI/2);
    for(let i=0;i<p.count;i++){const x=p.getX(i),z=p.getZ(i),r=Math.hypot(x,z);p.setY(i,-.18+(r>45?Math.sin(x*.031+z*.016)*Math.min(2.4,(r-45)*.015):0));uv.setXY(i,x,z);}
    geo.computeVertexNormals();if(!this.onRoad)this.shell.add(new THREE.Mesh(geo,m.gravel));
    for(const [x,z,w,d,h] of [[145,55,64,30,16],[75,190,54,34,20],[180,-85,60,36,13]]){
      // (beside the highway the road's own mesas stand on the horizon: World.ts, horizon.ts)
      if(this.onRoad)continue;
      const g=new THREE.CylinderGeometry(w*.4,w*.55,h,7,1);g.scale(1,1,d/w);const mesa=new THREE.Mesh(g,m.mesa);mesa.position.set(x,h/2-2,z);s.add(mesa);
    }
    // Low desert scrub, batched with the shell, outside the flat usable lot.
    for(let i=0;i<30;i++){const a=i*2.4,r=24+(i%7)*3,x=Math.cos(a)*r,z=Math.sin(a)*r;if(x>14&&x<26)continue;
      const b=new THREE.Mesh(new THREE.IcosahedronGeometry(.4+(i%3)*.16,0),m.wood);b.scale.y=.52;b.position.set(x,.02,z);s.add(b);}
  }
  private building(){
    const s=this.shell,f=this.fixed,m=this.m;
    const wall=(x0:number,x1:number,z0:number,z1:number,h=3.2,y=h/2)=>metreUV(box(s,x1-x0,h,z1-z0,m.panel,(x0+x1)/2,y,(z0+z1)/2));
    wall(-8,-7.8,-9,9);wall(-8,2,-9,-8.8);wall(-8,2,8.8,9);
    metreUV(box(s,10,.2,18,m.cream,-3,3.3,0));box(s,.15,.26,18.2,m.red,2.04,3.17,0);
    // A solid pier beside the doorway backs the framed clipping on the inside.
    wall(1.8,2,.6,2.15);this.col(1.8,2,.6,2.15);
    for(const [a,b] of [[-9,-.6],[2.15,9]]){
      wall(1.8,2,a,b,.85,.425);wall(1.8,2,a,b,.65,2.875);
      for(let z=a+.09;z<b;z+=2.05){box(s,.13,1.74,.065,m.steel,1.99,1.705,z);}
      plane(s,b-a,1.72,m.glass,1.985,1.71,(a+b)/2,Math.PI/2);
      for(const y of [.85,2.57])box(s,.2,.065,b-a,m.steel,1.99,y,(a+b)/2);
      this.col(1.8,2,a,b);
    }
    wall(1.8,2,-.6,.6,.9,2.75);this.col(-8,-7.8,-9,9);this.col(-8,2,-9,-8.8);this.col(-8,2,8.8,9);
    // Rear kitchen/service space is closed behind a partition and hatch.
    wall(-5.9,-5.7,-8.8,-3.6);wall(-5.9,-5.7,-1.2,8.8);wall(-5.9,-5.7,-3.6,-1.2,1.06,.53);wall(-5.9,-5.7,-3.6,-1.2,.9,2.75);
    plane(f,2.4,1.24,m.black,-5.925,1.68,-2.4,Math.PI/2);
    box(f,.5,.09,2.6,m.steel,-5.63,1.055,-2.4);
    plane(f,1.6,.8,m.menu,-5.68,2.69,-2.4,Math.PI/2);this.proxy('menu',.09,.8,1.6,-5.65,2.69,-2.4);
    this.col(-8,-5.7,-8.8,8.8);
    // A single checkerboard floor includes the niche, without overlapping planes.
    metreUV(plane(f,9.6,17.6,m.floor,-3,0,0,0,-Math.PI/2));
    const door=this.model('door',this.group,1.9,0,.58);
    for(const z of [-1.13,-.02])box(door,.07,2.24,.065,m.steel,0,1.12,z);
    for(const y of [.08,.78,2.2])box(door,.075,.085,1.14,m.steel,0,y,-.575);
    box(door,.045,.67,1.06,m.mint,0,.425,-.575);plane(door,1.04,1.34,m.glass,.004,1.47,-.575,Math.PI/2);
    box(door,.12,.04,.72,m.steel,.075,1.0,-.575);this.label(door,.22,.1,.049,1.13,-.78,[256,256,256,128],Math.PI/2);
    mergeStatic(door);this.proxy('door',.13,2.25,1.16,1.94,1.125,0);
    this.label(s,.5,.25,2.016,1.77,-1.12,[0,256,256,128],Math.PI/2);
    this.proxy('window',.1,1.55,1.75,1.965,1.67,-1.72);
    // Both restroom doors are closed background on the southern partition.
    for(const x of [-4.5,-2.8]){box(f,1.03,2.2,.055,m.mint,x,1.1,8.735);this.label(f,.81,.15,x,1.78,8.698,[0,0,512,128],Math.PI);box(f,.11,.04,.07,m.steel,x+.35,1,8.68);}
    for(const z of [-5,0,5]){box(f,.6,.06,2.8,m.steel,-2,3.08,z);box(f,.38,.025,2.5,m.tube,-2,3.038,z);}
    box(f,.34,.045,1.8,m.amber,-5.58,2.22,-2.4);
  }
  private counter(){
    const s=this.fixed,m=this.m;
    box(s,.58,.94,10.8,m.mint,-4.5,.47,0);metreUV(box(s,.6,.06,10.95,m.counter,-4.5,.97,0));
    box(s,.035,.065,10.97,m.steel,-4.181,.955,0);
    rod(s,V(-3.99,.19,-5.4),V(-3.99,.19,5.4),.023,m.steel,7);
    this.col(-4.81,-4.19,-5.5,5.5);this.proxy('counter',.6,.14,10.9,-4.5,.87,0);
    for(let i=0;i<7;i++){const z=-4.8+i*1.6;cyl(s,.255,.26,.12,m.vinyl,-3.4,.73,z,12);cyl(s,.045,.06,.6,m.steel,-3.4,.36,z,8);cyl(s,.23,.23,.06,m.steel,-3.4,.03,z,12);this.col(-3.68,-3.12,z-.28,z+.28);}
    // Coffee maker sits behind the reachable pot, leaving the counter hit below it.
    box(s,.36,.45,.48,m.steel,-4.64,1.23,-1.25);box(s,.17,.045,.48,m.black,-4.39,1.42,-1.25);box(s,.24,.045,.4,m.black,-4.4,1.026,-1.25);
    const pot=this.model('coffeePot',this.interior,-4.3,1.0,-1.15);
    cyl(pot,.11,.12,.2,m.coffee,0,.12,0,12);cyl(pot,.115,.115,.03,m.black,0,.235,0,12);
    rod(pot,V(.1,.19,-.045),V(.18,.18,-.045),.015,m.black,6);rod(pot,V(.18,.18,-.045),V(.18,.055,-.045),.015,m.black,6);rod(pot,V(.18,.055,-.045),V(.1,.06,-.045),.015,m.black,6);mergeStatic(pot);
    this.proxy('coffee',.4,.36,.5,-4.32,1.18,-1.15);
    const cup=this.model('cup',this.interior,-4.2,1.0,.6);cyl(cup,.062,.053,.12,m.cream,0,.07,0,10);cyl(cup,.047,.047,.005,m.coffee,0,.132,0,10);
    const handle=new THREE.Mesh(new THREE.TorusGeometry(.035,.008,5,10),m.cream);handle.position.set(.07,.075,0);cup.add(handle);mergeStatic(cup);
    for(const z of [-4.2,3.5,5]){box(s,.16,.19,.13,m.steel,-4.35,1.095,z);box(s,.15,.13,.018,m.cream,-4.25,1.12,z);cyl(s,.027,.03,.09,m.cream,-4.27,1.045,z+.16,7);}
  }
  private booths(){
    const s=this.fixed,m=this.m;
    for(const z of [-6.1,-3.05,3.05,6.1]){
      metreUV(box(s,1.65,.075,.76,m.counter,.7,.765,z));cyl(s,.055,.09,.71,m.steel,.7,.355,z,8);
      for(const d of [-.93,.93]){metreUV(box(s,1.85,.14,.49,m.vinyl,.7,.48,z+d));metreUV(box(s,1.85,.72,.13,m.vinyl,.7,.87,z+d+Math.sign(d)*.21));box(s,1.8,.35,.48,m.wood,.7,.175,z+d);}
      this.col(-.27,1.67,z-1.24,z+1.24);
      box(s,.13,.2,.13,m.steel,.96,.89,z);cyl(s,.035,.04,.12,m.cream,.63,.87,z,8);
    }
    this.proxy('booth',1.95,1.3,2.5,.7,.65,3.05);
    // A compact 1950s jukebox at the north end of the customer aisle.
    box(s,.9,1.62,.62,m.wood,-1.1,.81,-8.25);box(s,.79,1.41,.08,m.red,-1.1,.84,-7.92);
    box(s,.58,.37,.02,m.black,-1.1,1.26,-7.868);this.label(s,.58,.13,-1.1,1.32,-7.85,[0,384,512,128]);
    for(const x of [-1.46,-.74])box(s,.065,1.3,.03,m.amber,x,.84,-7.86);
    for(let x=-1.38;x<=-.81;x+=.08)box(s,.022,.56,.02,m.steel,x,.51,-7.86);
    for(let i=0;i<6;i++)box(s,.038,.032,.025,m.cream,-1.3+i*.078,1.05,-7.835);
    this.col(-1.61,-.59,-8.6,-7.87);this.proxy('jukebox',.95,1.65,.72,-1.1,.825,-8.21);
  }
  private papersAndPhone(){
    const s=this.fixed,m=this.m;
    const clipping=this.model('clipping',this.interior,1.77,1.65,1.35);
    box(clipping,.045,1.33,.93,m.wood,0,0,0);plane(clipping,.84,1.26,m.clipping,-.026,0,0,-Math.PI/2);
    this.label(clipping,.25,.085,-.027,-.606,-.2,[512,128,512,128],-Math.PI/2);mergeStatic(clipping);
    this.proxy('clipping',.12,1.35,.94,1.72,1.65,1.35);
    box(s,.64,.87,.22,m.steel,.4,1.44,8.66);box(s,.53,.7,.045,m.black,.4,1.43,8.529);
    this.label(s,.66,.125,.4,2.01,8.52,[0,128,512,128],Math.PI);
    this.label(s,.18,.08,.59,1.74,8.501,[512,256,256,128],Math.PI);
    box(s,.135,.018,.04,m.steel,.59,1.62,8.49);box(s,.15,.12,.05,m.steel,.58,1.12,8.487);
    for(let row=0;row<4;row++)for(let col=0;col<3;col++)box(s,.035,.03,.022,m.steel,.48+col*.05,1.48-row*.057,8.492);
    const handset=this.model('payphoneHandset',this.interior,.22,1.48,8.43);
    box(handset,.055,.26,.055,m.black,0,0,0);for(const y of [-.145,.145])box(handset,.095,.068,.075,m.black,0,y,-.018);mergeStatic(handset);
    const pts:THREE.Vector3[]=[];for(let i=0;i<=32;i++){const a=i*.8;pts.push(V(.22+.016*Math.sin(a),1.29-i*.011,8.43-.03*Math.cos(a)));}
    const cord=new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts),32,.006,4,false),m.black);s.add(cord);
    rod(s,V(.22,.938,8.43),V(.45,1.02,8.59),.007,m.black,5);
    this.col(.04,.77,8.47,8.8);this.proxy('payphone',.74,.94,.4,.4,1.45,8.59);
    // Two side cheeks give the phone a shallow acoustic niche, not another room.
    box(s,.055,1.23,.47,m.panel,-.15,1.55,8.58);box(s,.055,1.23,.47,m.panel,.95,1.55,8.58);
  }
  private people(){
    const m=this.m;
    const head=(parent:THREE.Group,id:string,y:number)=>{
      const h=this.model(id,parent,0,y,0);const f=new THREE.Mesh(new THREE.IcosahedronGeometry(.15,1),m.skin);f.scale.set(.87,1.15,.88);h.add(f);
      const hair=new THREE.Mesh(new THREE.IcosahedronGeometry(.154,1),m.hair);hair.scale.set(.98,.7,.9);hair.position.set(0,.065,.04);h.add(hair);return h;
    };
    const woman=this.model('waitress',this.interior,-5.13,0,0);woman.rotation.y=-Math.PI/2;
    const dress=new THREE.Mesh(new THREE.CylinderGeometry(.17,.26,.72,7),m.uniform);dress.position.y=.91;woman.add(dress);
    box(woman,.26,.55,.04,m.cream,0,.88,-.17);cyl(woman,.056,.062,.1,m.skin,0,1.35,0,7);
    for(const x of [-.1,.1]){rod(woman,V(x,.57,0),V(x,.12,0),.055,m.trousers,6);box(woman,.13,.08,.22,m.black,x,.06,-.045);}
    for(const side of [-1,1]){rod(woman,V(side*.18,1.2,0),V(side*.23,.98,-.05),.058,m.uniform,6);rod(woman,V(side*.23,.98,-.05),V(side*.19,1.0,-.24),.043,m.skin,6);}
    const wh=head(woman,'waitressHead',1.53);const bun=new THREE.Mesh(new THREE.IcosahedronGeometry(.08,0),m.hair);bun.position.set(0,.01,.14);wh.add(bun);mergeStatic(wh);mergeStatic(woman);
    this.proxy('waitress',.56,1.16,.62,-5.1,1.17,0);
    const driver=this.model('driver',this.interior,-3.4,0,1.6);driver.rotation.y=Math.PI/2;
    box(driver,.36,.47,.27,m.jacket,0,1.08,0);cyl(driver,.057,.06,.1,m.skin,0,1.36,-.01,7);
    for(const x of [-.11,.11]){rod(driver,V(x,.83,0),V(x,.75,-.34),.08,m.trousers,6);rod(driver,V(x,.75,-.34),V(x,.29,-.31),.06,m.trousers,6);box(driver,.14,.09,.25,m.black,x,.23,-.37);}
    for(const side of [-1,1]){rod(driver,V(side*.18,1.27,0),V(side*.22,1.03,-.16),.065,m.jacket,6);rod(driver,V(side*.22,1.03,-.16),V(side*.17,1.05,-.43),.046,m.skin,6);}
    const dh=head(driver,'driverHead',1.52);box(dh,.28,.095,.25,m.red,0,.125,0);box(dh,.29,.025,.13,m.red,0,.075,-.165);mergeStatic(dh);mergeStatic(driver);
    this.proxy('driver',.67,1.18,.63,-3.47,1.08,1.6);
  }
  private signAndRig(){
    const s=this.shell,m=this.m;
    // The alpha artwork includes both posts and feet: its foot baseline is y=0. Drawn at
    // 1.8 times the brief's 4 x 2 m (Claude, at integration) so it reads from the highway,
    // and a metre north so it clears the rig's cab.
    for(const z of [-11.97,-8.03])this.col(12.8,13.2,z-.2,z+.2);
    // Two front-facing copies, so lettering never mirrors from the back.
    plane(s,7.2,3.6,m.sign,13.025,1.56,-10,Math.PI/2);plane(s,7.2,3.6,m.sign,12.975,1.56,-10,-Math.PI/2);
    this.proxy('sign',.17,1.3,5.9,13,2.02,-10);
    const z=-14.15;
    box(s,8.0,2.65,2.35,m.cream,7.9,2.2,z);box(s,8.2,.26,2.4,m.black,7.9,.76,z);
    for(let x=4.1;x<11.9;x+=.55)for(const dz of [-1.185,1.185])box(s,.035,2.5,.025,m.steel,x,2.2,z+dz);
    box(s,2.7,1.35,2.3,m.mint,13.2,1.42,z);box(s,1.5,1.12,2.15,m.mint,12.65,2.56,z);
    box(s,.035,.65,1.93,m.black,13.416,2.63,z);box(s,.06,.7,1.6,m.steel,14.58,1.3,z);
    for(const dz of [-1.19,1.19]){box(s,1.26,.61,.02,m.black,12.7,2.65,z+dz*.89);cyl(s,.08,.08,2.8,m.steel,11.88,2.3,z+dz,8);}
    for(const x of [4.9,5.75,11.6,13.6])for(const dz of [-1.13,1.13]){const w=cyl(s,.43,.43,.3,m.black,x,.44,z+dz,10);w.rotation.x=Math.PI/2;const hub=cyl(s,.22,.22,.314,m.steel,x,.44,z+dz,8);hub.rotation.x=Math.PI/2;}
    this.col(3.7,14.85,-15.55,-12.75);this.proxy('rig',11.3,3.7,2.8,9.28,1.85,z);
  }

  /** Shared external samplers, never disposed or modified here. Metre UVs expect
   * booth repeat [2,2] (0.5 m), wall repeat [.5,.5] (2 m), both in sRGB.
   * Proposed future art.ts IDs: dinerBooth and dinerWall. */
  setSurfaceArt(booth:THREE.Texture,wall:THREE.Texture){
    if(this.disposed)return;
    for(const [m,t] of [[this.m.vinyl,booth],[this.m.panel,wall]] as const)eachVariant(m,v=>{(v as THREE.MeshStandardMaterial).map=t;v.needsUpdate=true;});
  }
  setSignLit(on:boolean){this.signLit=on;if(floodOwner===this){setFlood(this.slots.red,on?8:0,dinerFlood);setFlood(this.slots.cyan,on?4:0,dinerFlood);}eachVariant(this.m.sign,v=>{const m=v as THREE.MeshStandardMaterial;m.emissiveIntensity=on?.65:0;m.color.setScalar(on?1:.32);});}
  setDawn(k:number){
    this.dawn=THREE.MathUtils.clamp(Number.isFinite(k)?k:0,0,1);
    if(floodOwner===this)setFlood(this.slots.dawn,this.dawn*8,dinerFlood);
    this.lights.hemi.color.set(0x6d87a4).lerp(new THREE.Color(0xb0bbc2),this.dawn);this.lights.hemi.intensity=(.55+this.dawn*.3)*this.near;
    eachVariant(this.m.glass,v=>{const m=v as THREE.MeshStandardMaterial;m.color.set(0x627584).lerp(new THREE.Color(0xc6c6b6),this.dawn);m.emissive.set(0x8fabc2);m.emissiveIntensity=this.dawn*.06;});
  }
  /** How much of the diner's own sky light is on: all of it at the diner, none far up the
   *  road, whose night light takes over there (World.ts). */
  setNear(w:number){this.near=THREE.MathUtils.clamp(w,0,1);this.lights.hemi.intensity=(.55+this.dawn*.3)*this.near;}
  private near=1;
  update(_dt:number,t:number){if(this.disposed||floodOwner!==this)return;
    if(this.signLit){setFlood(this.slots.red,8*(.965+.035*Math.sin(t*7.3)),dinerFlood);setFlood(this.slots.cyan,4*(.985+.015*Math.sin(t*4.7)),dinerFlood);}
    const k=.98+.02*Math.sin(t*8.1)*Math.sin(t*.9);setFlood(3,4.5*k,dinerFlood);eachVariant(this.m.tube,v=>(v as THREE.MeshStandardMaterial).emissiveIntensity=.75*k);
  }
  dispose(){if(this.disposed)return;this.disposed=true;
    const geos=new Set<THREE.BufferGeometry>();this.group.traverse(o=>{const m=o as THREE.Mesh;if(m.isMesh)geos.add(m.geometry);});for(const g of geos)g.dispose();
    const mats=new Set<THREE.Material>();for(const m of this.materials)eachVariant(m,v=>mats.add(v));for(const m of mats)m.dispose();for(const t of this.textures)t.dispose();
    if(floodOwner===this){for(const p of dinerFlood.pos)p.w=0;dinerFlood.count=0;floodOwner=null;}
    this.group.removeFromParent();this.group.clear();
  }
}
