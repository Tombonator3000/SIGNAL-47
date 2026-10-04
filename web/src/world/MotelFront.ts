import * as THREE from 'three';
import { artTexture } from '../core/art';
import { eachVariant } from '../core/quality';
import { box, cyl, plane, rod, noMerge, mergeStatic, floodSet, floodlit, addFlood, setFlood } from './kit';
import type { Collider } from './ControlRoom';
import type { Zone } from '../player/Player';

// SARO coordinates, north -Z. All walkable surfaces are at y=0.
// The retained road and Sierra sign belong to Exterior, never this module.
// Office: x[-45.6,-41.2], z[22.2,27.6]. Door: east wall, z[23.4,24.8].
// Eight rooms run south; room 6's threshold is (-41.1,0,55).
// Only one live court owns this shader set; old instances cannot reset a new one.
export const courtFlood = floodSet(10, 'court', 0.1);
let owner: MotelFront | null = null;
type Anchor = { x: number; z: number; yaw: number };
export type MotelFrontAnchors = { entry: Anchor; officeInside: Anchor; room6Outside: Anchor; fromRoom6: Anchor };
const V = (x: number, y: number, z: number) => new THREE.Vector3(x, y, z);
const yawTo = (x: number, z: number, tx: number, tz: number) => Math.atan2(x - tx, z - tz);

// Metre-based projection keeps plaster/concrete continuous across modular pieces.
function metreUV(m: THREE.Mesh) {
  m.updateMatrix();
  const p = m.geometry.attributes.position, n = m.geometry.attributes.normal, uv = m.geometry.attributes.uv;
  const v = new THREE.Vector3(), nn = new THREE.Vector3(), nm = new THREE.Matrix3().getNormalMatrix(m.matrix);
  for (let i = 0; i < p.count; i++) {
    v.fromBufferAttribute(p, i).applyMatrix4(m.matrix); nn.fromBufferAttribute(n, i).applyMatrix3(nm);
    if (Math.abs(nn.y) > .5) uv.setXY(i, v.x, v.z);
    else if (Math.abs(nn.x) > .5) uv.setXY(i, -v.z, v.y);
    else uv.setXY(i, v.x, v.y);
  }
  return m;
}

export class MotelFront {
  readonly group = new THREE.Group();
  readonly interior = new THREE.Group();
  readonly proxies: Record<string, THREE.Object3D> = {};
  readonly objs: Record<string, THREE.Object3D> = {};
  readonly zones: Zone[] = [];
  readonly colliders: Collider[] = [];
  readonly flood = courtFlood;
  readonly anchors: MotelFrontAnchors = {
    entry: { x: -29, z: 6, yaw: yawTo(-29, 6, -40, 24) },
    officeInside: { x: -42.6, z: 24.1, yaw: yawTo(-42.6, 24.1, -44, 25.1) },
    room6Outside: { x: -39.5, z: 55, yaw: Math.PI / 2 },
    fromRoom6: { x: -39.5, z: 55, yaw: -Math.PI / 2 },
  };
  room6LightOn = true;
  officeLightOn = true;
  private readonly st = new THREE.Group();
  private readonly inSt = new THREE.Group();
  private readonly materials = new Set<THREE.Material>();
  private readonly textures = new Set<THREE.Texture>();
  private readonly hit = new THREE.MeshBasicMaterial({ visible: false });
  private readonly slots: Record<string, number> = {};
  private m!: ReturnType<MotelFront['makeMaterials']>;
  private darkWindow!: THREE.Texture;
  private litWindow!: THREE.Texture;
  private disposed = false;

  constructor() {
    this.group.name = 'MotelFront'; this.interior.name = 'MotelOfficeInterior';
    this.group.add(this.st, this.interior); this.interior.add(this.inSt);
    this.materials.add(this.hit);
    owner = this; courtFlood.count = 0;
    for (const p of courtFlood.pos) p.set(0, -999, 0, 0);
    this.m = this.makeMaterials();
    for (const [key, material] of Object.entries(this.m)) material.name = 'court/' + key;
    this.forecourt(); this.building(); this.office(); this.car(); this.pool();
    // Proxy groups are separate from merged visuals and remain usable when interior
    // visibility changes. Their labels/enabled state belong to Chapter4.
    this.zone('lot', -41, -27.7, 1.5, 70.8);
    this.zone('walk', -41.25, -38.25, 21.6, 70.8);
    this.zone('office', -45.6, -41.2, 22.2, 27.6);
    this.zone('officeDoor', -42.25, -40.05, 23.4, 24.8);
    // Sign posts are owned/drawn by Exterior, but need court collision bounds.
    for (const x of [-31.5, -29.3]) this.col(x-.24, x+.24, 29.76, 30.24);
    this.slots.office = this.light(-43.2, 2.35, 24.6, 6, 0xffcb8b);
    this.slots.officeSpill = this.light(-40.65, 2.25, 24.1, 4, 0xffc277);
    for (const [name,z] of [['r1',30],['r2',35],['r34',42.5],['r5',50],['room6',55],['r78',62.5]] as const)
      this.slots[name] = this.light(-40.7, 2.24, z, name==='room6'?4:5, 0xffbd72);
    this.slots.red = this.light(-30.9, 4.6, 30, 9, 0xff4839);
    this.slots.cyan = this.light(-30.0, 3.8, 30, 4, 0x51cad6);
    mergeStatic(this.st); mergeStatic(this.inSt);
    this.group.updateMatrixWorld(true);
  }

  private mat(p: THREE.MeshStandardMaterialParameters, falloff=.055) {
    const m = floodlit(new THREE.MeshStandardMaterial(p), falloff, courtFlood);
    this.materials.add(m); return m;
  }
  private tex(w: number, h: number, draw: (g: CanvasRenderingContext2D) => void, repeat?: [number,number]) {
    const c=document.createElement('canvas'); c.width=w; c.height=h; draw(c.getContext('2d')!);
    const t=new THREE.CanvasTexture(c); t.colorSpace=THREE.SRGBColorSpace; t.anisotropy=4;
    if(repeat){t.wrapS=t.wrapT=THREE.RepeatWrapping;t.repeat.set(...repeat);}
    this.textures.add(t);return t;
  }
  private makeMaterials() {
    const plaster=this.tex(128,128,g=>{
      g.fillStyle='#d0ad91';g.fillRect(0,0,128,128);
      let s=1947;for(let i=0;i<1900;i++){s=(s*1664525+1013904223)>>>0;const x=s%128;s=(s*1664525+1013904223)>>>0;g.fillStyle=i%2?'#d7b79d':'#c5a38a';g.fillRect(x,s%128,1,1);}
    },[.5,.5]);
    const door=this.tex(128,256,g=>{
      g.fillStyle='#78352d';g.fillRect(0,0,128,256);
      g.strokeStyle='#542923';g.lineWidth=2;for(const y of [20,128])g.strokeRect(14,y,100,94);
      g.fillStyle='#a27245';g.fillRect(3,229,122,24);g.fillStyle='#a27050';
      for(let i=0;i<28;i++)g.fillRect(15+(i*17)%19,132+(i*13)%39,1,4);
    });
    const win=(on:boolean)=>this.tex(128,128,g=>{
      g.fillStyle=on?'#ba9463':'#272c32';g.fillRect(0,0,128,128);
      for(let x=4;x<124;x+=7){g.fillStyle=on?(x%3?'#d3b17a':'#ad8259'):(x%3?'#34323a':'#242a30');g.fillRect(x,3,3,121);}
      g.fillStyle=on?'#876641':'#432d32';g.fillRect(62,0,4,128);
      g.strokeStyle='#77766c';g.lineWidth=5;g.strokeRect(2.5,2.5,123,123);
      g.lineWidth=3;g.beginPath();g.moveTo(64,0);g.lineTo(64,128);g.stroke();
    });
    this.litWindow=win(true);this.darkWindow=win(false);
    const atlas=this.tex(1024,512,g=>{
      g.fillStyle='#ded3b9';g.fillRect(0,0,1024,512);g.textAlign='center';g.textBaseline='middle';
      for(let n=1;n<=8;n++){g.fillStyle='#57372b';g.font='bold 72px serif';g.fillText(String(n),(n-.5)*128,64);}
      g.fillStyle='#243c38';g.font='bold 84px sans-serif';g.fillText('OFFICE',256,192);g.fillStyle='#b84331';g.font='bold 62px sans-serif';g.fillText('VACANCY',768,192);
      g.fillStyle='#354a48';g.font='bold 70px sans-serif';g.fillText('ICE',128,320);g.font='bold 34px sans-serif';g.fillText('POOL CLOSED',512,320);g.fillText('ROOM 6',848,305);g.font='48px sans-serif';g.fillText('↓',848,350);
      g.fillStyle='#3c453e';g.font='bold 31px serif';g.fillText('GUEST REGISTER',256,423);g.strokeStyle='#83765d';g.lineWidth=1;
      for(let y=447;y<507;y+=12){g.beginPath();g.moveTo(20,y);g.lineTo(492,y);g.stroke();}
      for(let x=130;x<500;x+=90){g.beginPath();g.moveTo(x,444);g.lineTo(x,506);g.stroke();}
    });
    return {
      plaster:this.mat({map:plaster,roughness:1}), concrete:this.mat({map:artTexture('concrete',[.5,.5]),color:0xc0af96,roughness:1}),
      asphalt:this.mat({map:artTexture('asphalt',[.2,.2]),color:0x817668,roughness:.95}),
      trim:this.mat({color:0x435b53,roughness:.8}), wood:this.mat({color:0x775039,roughness:.75}),
      door:this.mat({map:door,roughness:.8}), metal:this.mat({color:0x9c9988,roughness:.55,metalness:.5}),
      black:this.mat({color:0x202425,roughness:.7}), brass:this.mat({color:0xaa8954,roughness:.45,metalness:.55}),
      atlas:this.mat({map:atlas,roughness:.9}), paper:this.mat({map:artTexture('paper'),color:0xe6d5b0,roughness:1}),
      curtain:this.mat({map:this.darkWindow,roughness:.85}), room6Window:this.mat({map:this.litWindow,emissive:0xd99742,emissiveMap:this.litWindow,emissiveIntensity:.3,roughness:.85}),
      officeWindow:this.mat({map:this.litWindow,emissive:0xd99742,emissiveMap:this.litWindow,emissiveIntensity:.3,roughness:.85}),
      lamps:this.mat({color:0xe9c890,emissive:0xffc478,emissiveIntensity:.8}),
      room6Lamp:this.mat({color:0xe9c890,emissive:0xffc478,emissiveIntensity:.8}),
      officeLamp:this.mat({color:0xe9c890,emissive:0xffc478,emissiveIntensity:.8}),
      paint:this.mat({color:0xb3a27c,roughness:.7,metalness:.18}), leaf:this.mat({color:0x756e45,roughness:1}),
      pool:this.mat({color:0x315653,roughness:.3,metalness:.15}),
    };
  }
  private label(parent:THREE.Object3D,w:number,h:number,x:number,y:number,z:number,rect:[number,number,number,number],ry=Math.PI/2,rx=0){
    const p=plane(parent,w,h,this.m.atlas,x,y,z,ry,rx),uv=p.geometry.attributes.uv;
    for(let i=0;i<uv.count;i++)uv.setXY(i,(rect[0]+uv.getX(i)*rect[2])/1024,1-(rect[1]+(1-uv.getY(i))*rect[3])/512);
    return p;
  }
  private col(minX:number,maxX:number,minZ:number,maxZ:number){this.colliders.push({minX,maxX,minZ,maxZ});}
  private zone(id:string,minX:number,maxX:number,minZ:number,maxZ:number){this.zones.push({id,enabled:true,minX,maxX,minZ,maxZ});}
  private proxy(id:string,w:number,h:number,d:number,x:number,y:number,z:number,parent:THREE.Object3D=this.group){
    const p=box(parent,w,h,d,this.hit,x,y,z);p.name='motel:'+id;if(parent===this.group)this.proxies[id]=p;return p;
  }
  private model(id:string,parent:THREE.Object3D,x:number,y:number,z:number){const g=new THREE.Group();g.name='motel:'+id;g.position.set(x,y,z);noMerge(g);parent.add(g);this.objs[id]=g;return g;}
  private light(x:number,y:number,z:number,w:number,c:number){return addFlood(x,y,z,w,c,courtFlood);}
  private plant(x:number,z:number,inside=false){
    const p=inside?this.inSt:this.st,m=this.m;
    cyl(p,.21,.15,.4,m.wood,x,.2,z,8);cyl(p,.185,.185,.015,m.black,x,.397,z,8);
    for(let i=0;i<7;i++){const a=i*2.4;rod(p,V(x,.4,z),V(x+Math.cos(a)*.26,.75+(i%3)*.1,z+Math.sin(a)*.26),.011,m.leaf,4);}
    this.col(x-.27,x+.27,z-.27,z+.27);
  }

  private forecourt(){
    const s=this.st,m=this.m;
    // The driveway starts at the road crossing, not only beside the guest rooms.
    // Surfaces meet edge-to-edge at y=0. Zone overlap is for Player's radius,
    // never a second coplanar surface beneath the concrete walk or office floor.
    const slab=(x0:number,x1:number,z0:number,z1:number,mat:THREE.Material)=>
      metreUV(box(s,x1-x0,.62,z1-z0,mat,(x0+x1)/2,-.31,(z0+z1)/2));
    slab(-41,-27.7,1.5,21.6,m.asphalt);
    slab(-38.25,-27.7,21.6,70.8,m.asphalt);
    slab(-41.25,-38.25,21.6,22.2,m.concrete);
    slab(-41.2,-38.25,22.2,27.6,m.concrete); // office floor ends at x=-41.2
    slab(-41.25,-38.25,27.6,70.8,m.concrete);
    // The boundary curbs also abut the slab instead of covering its top face.
    box(s,13.3,.08,.16,m.concrete,-34.35,-.04,1.42);
    box(s,13.55,.08,.16,m.concrete,-34.475,-.04,70.88);
    for(let z=31;z<70;z+=5){box(s,5.6,.006,.075,m.paint,-35.2,.004,z);box(s,.17,.13,2.1,m.concrete,-38.3,.065,z+2.5);}
    // A low, continuous fascia and slim posts carry the covered walk.
    metreUV(box(s,3.15,.18,49,m.concrete,-39.75,2.84,46.1));
    box(s,.12,.24,49,m.trim,-38.18,2.75,46.1);
    for(const z of [22,28,33,38,43,48,53,58,63,69]){
      box(s,.115,2.74,.115,m.trim,-38.8,1.37,z);box(s,.23,.12,.23,m.concrete,-38.8,.06,z);
      this.col(-38.93,-38.67,z-.13,z+.13);
    }
    this.plant(-40.2,22.1);this.plant(-40.25,56.35);this.plant(-40.2,68.5);
    // Ice cabinet is separate from the office doorway and the continuous inner walk.
    metreUV(box(s,.7,1.38,.8,m.concrete,-40.65,.69,28.1));
    box(s,.015,.65,.62,m.trim,-40.291,.85,28.1);box(s,.06,.06,.15,m.metal,-40.24,.65,28.36);
    this.label(s,.46,.23,-40.28,1.14,28.1,[0,256,256,128]);
    this.col(-41.02,-40.27,27.67,28.53);this.proxy('iceMachine',.78,1.4,.88,-40.63,.7,28.1);
  }

  private building(){
    const s=this.st,m=this.m;
    // Rear mass and guest-room interiors stay closed background. The facade sits
    // in front, with an actual recess behind the hinged Room6 leaf.
    metreUV(box(s,4.2,2.8,48,m.plaster,-47.9,1.4,46));
    metreUV(box(s,3.5,2.8,42.1,m.plaster,-43.95,1.4,48.95));
    metreUV(box(s,9.15,.22,48.2,m.concrete,-45.5,2.9,46));
    box(s,9.2,.18,.16,m.trim,-45.5,3.02,21.95);box(s,9.2,.18,.16,m.trim,-45.5,3.02,70.05);
    this.col(-50,-45.7,22,70);this.col(-45.7,-41.05,27.8,70);
    // Office walls: front has a 1.4 m doorway; inner faces belong to interior.
    for(const [z0,z1] of [[22,23.4],[24.8,25.15],[27.25,27.8]])metreUV(box(s,.2,2.8,z1-z0,m.plaster,-41.1,1.4,(z0+z1)/2));
    metreUV(box(s,.2,.58,1.4,m.plaster,-41.1,2.51,24.1));
    metreUV(box(s,.2,.72,2.1,m.plaster,-41.1,.36,26.2));
    metreUV(box(s,.2,.7,2.1,m.plaster,-41.1,2.45,26.2));
    for(const [x0,x1,z0,z1] of [[-45.8,-41,22,22.2],[-45.8,-41,27.6,27.8],[-45.8,-45.6,22,27.8],[-41.2,-41,22,23.4],[-41.2,-41,24.8,27.8]]){
      this.col(x0,x1,z0,z1);if(x1-x0>1||z1-z0>3)metreUV(box(s,x1-x0,2.8,z1-z0,m.plaster,(x0+x1)/2,1.4,(z0+z1)/2));
    }
    this.label(s,1.15,.29,-40.982,2.49,24.1,[0,128,512,128]);
    this.label(s,.9,.225,-40.97,.49,26.2,[512,128,512,128]);
    plane(s,2.04,1.36,m.officeWindow,-40.989,1.39,26.2,Math.PI/2);
    box(s,.055,2.16,.055,m.trim,-41.01,1.08,23.38);box(s,.055,2.16,.055,m.trim,-41.01,1.08,24.82);
    // Office door stands open inward against its south return.
    box(this.inSt,1.15,2.1,.055,m.trim,-41.78,1.05,24.85);
    this.col(-42.36,-41.2,24.81,24.9);
    this.proxy('officeDoor',.13,2.2,1.39,-41.02,1.1,24.1);
    this.label(s,.6,.44,-40.985,1.68,27.51,[704,256,320,128]);
    const others=new THREE.Group();others.name='motel:otherDoors';this.group.add(others);this.proxies.otherDoors=others;
    for(let i=1;i<=8;i++){
      const z=25+i*5,is6=i===6;
      // Faces share one door texture; exact room numbers are small atlas cards.
      metreUV(box(s,.15,2.8,3.98,m.plaster,-41.12,1.4,z+2.49));
      metreUV(box(s,.15,.75,1.02,m.plaster,-41.12,2.425,z));
      for(const dz of [-.53,.53])box(s,.12,2.13,.07,m.trim,-41.015,1.065,z+dz);
      const leaf=is6?this.model('room6DoorLeaf',this.group,-41.07,0,z+.46):new THREE.Group();
      if(!is6){leaf.position.set(-41.07,0,z+.46);s.add(leaf);}
      box(leaf,.06,2.03,.92,m.wood,0,1.015,-.46);
      plane(leaf,.92,2.03,m.door,.032,1.015,-.46,Math.PI/2);
      this.label(leaf,.18,.18,.039,1.72,-.46,[(i-1)*128,0,128,128]);
      cyl(leaf,.014,.014,.028,m.brass,.057,1.39,-.46,8).rotation.z=Math.PI/2;
      box(leaf,.06,.045,.09,m.brass,.065,.96,-.78);
      if(is6){mergeStatic(leaf);this.proxy('room6Door',.14,2.05,.94,-41.015,1.03,z);}
      else this.proxy('otherDoors',.12,2.05,.94,-41.015,1.03,z,others);
      plane(s,1.4,1.2,is6?m.room6Window:m.curtain,-41.027,1.38,z+2.03,Math.PI/2);
      box(s,.22,.09,1.52,m.concrete,-40.98,.745,z+2.03);
      box(s,.15,.39,.66,m.metal,-40.99,.38,z+2.03);
      for(let sl=0;sl<4;sl++)box(s,.02,.015,.55,m.black,-40.905,.27+sl*.06,z+2.03);
      box(s,.25,.08,.29,m.black,-40.93,2.23,z);
      box(s,.2,.035,.22,is6?m.room6Lamp:m.lamps,-40.86,2.175,z);
    }
    // Close the northern and southern ends of the facade.
    metreUV(box(s,.15,2.8,1.7,m.plaster,-41.12,1.4,28.35));
    metreUV(box(s,.15,2.8,.8,m.plaster,-41.12,1.4,69.6));
  }

  private office(){
    const s=this.inSt,m=this.m;
    metreUV(plane(s,4.4,5.4,m.concrete,-43.4,0,24.9,0,-Math.PI/2));
    for(const z of [22.205,27.595]){box(s,4.4,.82,.018,m.trim,-43.4,.41,z);box(s,4.4,.035,.03,m.wood,-43.4,.84,z);}
    box(s,.02,.82,5.4,m.trim,-45.59,.41,24.9);box(s,.03,.035,5.4,m.wood,-45.58,.84,24.9);
    // Counter leaves a public strip along its east side and access to the phone
    // and key board to its north. All three paper hits sit above the counter top.
    box(s,1,.95,3.2,m.wood,-44.2,.475,25.25);box(s,1.16,.07,3.34,m.trim,-44.2,.985,25.25);
    for(let z=23.75;z<26.8;z+=.22)box(s,.015,.73,.025,m.brass,-43.687,.45,z);
    this.col(-44.79,-43.61,23.57,26.93);
    box(s,.1,.05,.1,m.brass,-43.82,1.045,26.62);rod(s,V(-43.82,1.07,26.62),V(-43.82,1.29,26.62),.004,m.metal,5);
    for(let n=0;n<4;n++)plane(s,.13,.19,m.paper,-43.82,1.08+n*.008,26.62,0,-Math.PI/2);
    this.label(s,.39,.3,-43.9,1.026,24.05,[0,384,512,128],0,-Math.PI/2);
    this.proxy('register',.43,.04,.34,-43.9,1.048,24.05);
    for(const [id,z,w,d] of [['envelope',25.1,.27,.19],['message',26.03,.24,.27]] as const){
      const p=this.model(id,this.interior,-43.9,1.027,z);
      plane(p,w,d,m.paper,0,0,0,0,-Math.PI/2);
      if(id==='envelope'){
        // A physical folded flap and a small wax seal; no authored story text.
        rod(p,V(-w/2,.003,-d/2),V(0,.003,.015),.002,m.wood,3);rod(p,V(w/2,.003,-d/2),V(0,.003,.015),.002,m.wood,3);
        cyl(p,.018,.018,.003,m.door,0,.006,.015,8);
      }
      mergeStatic(p);this.proxy(id,w+.05,.05,d+.05,-43.9,1.049,z);
    }
    // Oak key board faces east, with eight numbered hooks. Hook 6 is empty.
    box(s,.075,.63,1.55,m.wood,-45.5,1.67,23.23);
    for(let i=1;i<=8;i++){
      const row=i<=4?0:1, col=(i-1)%4, z=22.68+col*.36,y=1.83-row*.3;
      this.label(s,.12,.1,-45.458,y+.075,z,[(i-1)*128,0,128,128]);
      const hook=i===6?this.model('key6Hook',this.interior,-45.42,y-.04,z):new THREE.Group();
      if(i!==6){hook.position.set(-45.42,y-.04,z);s.add(hook);}
      rod(hook,V(-.026,0,0),V(.012,-.014,0),.009,m.brass,5);rod(hook,V(.012,-.014,0),V(.025,.009,0),.009,m.brass,5);
      if(i===6)mergeStatic(hook);else {box(hook,.012,.12,.055,m.brass,.018,-.085,0);box(hook,.01,.018,.023,m.black,.021,-.116,.035);}
    }
    this.proxy('keyBoard',.16,.67,1.58,-45.47,1.67,23.23);
    // The same oak field-telephone family as STATION 01, here on the north wall.
    const ph=this.model('officePhoneHandset',this.interior,-42.97,1.57,22.38);
    cyl(ph,.026,.02,.14,m.black,0,-.07,0,8);cyl(ph,.035,.035,.025,m.black,0,-.145,0,8);mergeStatic(ph);
    box(s,.32,.52,.16,m.wood,-43.17,1.46,22.29);box(s,.37,.04,.25,m.wood,-43.17,1.17,22.34);
    for(const dx of [-.085,.085]){
      const b=new THREE.Mesh(new THREE.SphereGeometry(.053,8,6,0,Math.PI*2,0,Math.PI/2),m.brass);b.position.set(-43.17+dx,1.64,22.405);b.rotation.x=Math.PI/2;s.add(b);
    }
    rod(s,V(-43.17,1.43,22.37),V(-43.17,1.43,22.52),.014,m.black,6);
    const mouth=new THREE.Mesh(new THREE.ConeGeometry(.049,.08,8,1,true),m.black);mouth.position.set(-43.17,1.43,22.54);mouth.rotation.x=-Math.PI/2;s.add(mouth);
    rod(s,V(-43.36,1.44,22.31),V(-43.43,1.44,22.38),.011,m.brass,5);rod(s,V(-43.43,1.44,22.38),V(-43.43,1.35,22.38),.011,m.brass,5);
    box(s,.07,.035,.04,m.black,-43.43,1.335,22.38);
    rod(s,V(-42.97,1.42,22.38),V(-43.05,1.08,22.33),.006,m.black,4);
    rod(s,V(-43.05,1.08,22.33),V(-43.19,1.19,22.36),.006,m.black,4);
    // Cable follows the wall/ceiling, then exits the west (back) wall.
    rod(s,V(-43.2,1.72,22.31),V(-43.2,2.58,22.25),.006,m.black,4);
    rod(s,V(-43.2,2.58,22.25),V(-45.75,2.58,22.25),.006,m.black,4);
    this.col(-43.5,-42.85,22.2,22.57);this.proxy('officePhone',.68,.72,.43,-43.17,1.43,22.39);
    // Desk light, filing shelves, tired plant and a vacant chair keep the room lived-in.
    cyl(s,.14,.2,.29,m.trim,-44.37,1.4,26.45,10);cyl(s,.13,.13,.02,m.officeLamp,-44.37,1.27,26.45,10);
    rod(s,V(-44.37,1.03,26.45),V(-44.37,1.27,26.45),.015,m.brass,6);cyl(s,.095,.095,.02,m.brass,-44.37,1.035,26.45,10);
    box(s,.45,1.5,1,m.wood,-45.33,.75,26.85);this.col(-45.57,-45.1,26.33,27.4);
    for(const y of [.2,.6,1])for(let j=0;j<4;j++)box(s,.28,.25,.14,j%2?m.trim:m.paper,-45.05,y+.13,26.55+j*.19);
    this.plant(-41.75,26.95,true);
    box(s,.52,.065,.5,m.trim,-42.45,.47,27.1);box(s,.52,.55,.065,m.trim,-42.45,.73,27.3);
    for(const x of [-42.66,-42.24])for(const z of [26.91,27.29])box(s,.035,.46,.035,m.wood,x,.23,z);
    this.col(-42.73,-42.17,26.8,27.39);
    box(s,1.1,.045,.26,m.officeLamp,-43.3,2.7,24.65);
  }

  private car(){
    const s=this.st,m=this.m,x=-35.3,z=34.6;
    box(s,1.92,.59,4.55,m.paint,x,.7,z);box(s,1.68,.55,2.35,m.black,x,1.26,z+.15);
    box(s,1.79,.07,2.48,m.paint,x,1.56,z+.15);
    for(const dx of [-.84,.84])for(const dz of [-1.03,1.3])box(s,.06,.57,.055,m.paint,x+dx,1.28,z+dz);
    for(const dx of [-.91,.91])for(const dz of [-1.42,1.42]){
      const wh=cyl(s,.34,.34,.22,m.black,x+dx,.35,z+dz,10);wh.rotation.z=Math.PI/2;
      const hub=cyl(s,.16,.16,.232,m.metal,x+dx,.35,z+dz,8);hub.rotation.z=Math.PI/2;
    }
    for(const dz of [-2.31,2.31])box(s,1.96,.13,.11,m.metal,x,.48,z+dz);
    for(const dx of [-.67,.67]){box(s,.38,.17,.02,m.concrete,x+dx,.79,z-2.29);box(s,.34,.12,.02,m.door,x+dx,.8,z+2.29);}
    box(s,.62,.07,.035,m.black,x,.92,z+2.292);
    this.col(x-1.09,x+1.09,z-2.39,z+2.39);this.proxy('car',2.2,1.65,4.8,x,.825,z);
  }
  private pool(){
    const s=this.st,m=this.m;
    metreUV(box(s,5.9,.14,12.2,m.concrete,-31.6,.07,55.5));
    box(s,4.65,.06,10.7,m.pool,-31.6,.145,55.5);
    for(const x of [-34.5,-28.7])for(let z=49.4;z<=61.7;z+=2.4)cyl(s,.038,.038,1.22,m.metal,x,.71,z,5);
    for(const z of [49.4,61.6])for(let x=-34.5;x<=-28.69;x+=1.45)cyl(s,.038,.038,1.22,m.metal,x,.71,z,5);
    // Sparse mesh fence as one batched material, no transparency over the desert.
    for(const x of [-34.5,-28.7])for(const y of [.34,.7,1.22])rod(s,V(x,y,49.4),V(x,y,61.6),.018,m.metal,4);
    for(const z of [49.4,61.6])for(const y of [.34,.7,1.22])rod(s,V(-34.5,y,z),V(-28.7,y,z),.018,m.metal,4);
    for(const x of [-34.5,-28.7])for(let z=49.4;z<61.5;z+=.42)rod(s,V(x,.25,z),V(x,1.21,z+.42),.007,m.metal,3);
    for(const z of [49.4,61.6])for(let x=-34.5;x<-29.0;x+=.42)rod(s,V(x,.25,z),V(x+.42,1.21,z),.007,m.metal,3);
    this.label(s,1.05,.26,-34.54,.95,55.5,[256,256,448,128],-Math.PI/2);
    this.col(-34.57,-28.63,49.33,61.67);this.proxy('pool',.12,1.3,12.34,-34.59,.7,55.5);
  }

  setRoom6Light(on:boolean){
    this.room6LightOn=on;if(owner===this)setFlood(this.slots.room6,on?4:0,courtFlood);
    eachVariant(this.m.room6Lamp,v=>(v as THREE.MeshStandardMaterial).emissiveIntensity=on?.8:0);
    eachVariant(this.m.room6Window,v=>{const m=v as THREE.MeshStandardMaterial;m.map=on?this.litWindow:this.darkWindow;m.emissiveIntensity=on?.3:0;});
  }
  setOfficeLight(on:boolean){
    this.officeLightOn=on;
    if(owner===this){setFlood(this.slots.office,on?6:0,courtFlood);setFlood(this.slots.officeSpill,on?4:0,courtFlood);}
    eachVariant(this.m.officeLamp,v=>(v as THREE.MeshStandardMaterial).emissiveIntensity=on?.8:0);
    eachVariant(this.m.officeWindow,v=>{const m=v as THREE.MeshStandardMaterial;m.map=on?this.litWindow:this.darkWindow;m.emissiveIntensity=on?.3:0;});
  }
  update(_dt:number,t:number){
    if(this.disposed||owner!==this)return;
    const k=.97+.03*Math.sin(t*5.3)*Math.sin(t*.73);
    setFlood(this.slots.red,9*k,courtFlood);setFlood(this.slots.cyan,4*(.985+.015*Math.sin(t*3.1)),courtFlood);
    setFlood(this.slots.r34,5*(.96+.04*Math.sin(t*8.3)),courtFlood);
  }
  dispose(){
    if(this.disposed)return;this.disposed=true;
    const geos=new Set<THREE.BufferGeometry>();this.group.traverse(o=>{const m=o as THREE.Mesh;if(m.isMesh)geos.add(m.geometry);});
    for(const g of geos)g.dispose();
    const mats=new Set<THREE.Material>();for(const m of this.materials)eachVariant(m,v=>mats.add(v));for(const m of mats)m.dispose();
    // artTexture samplers and kit.M belong to the shared startup cache, never us.
    for(const t of this.textures)t.dispose();
    if(owner===this){for(const p of courtFlood.pos)p.w=0;courtFlood.count=0;owner=null;}
    this.group.removeFromParent();this.group.clear();
  }
}
