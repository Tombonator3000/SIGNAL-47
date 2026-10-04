import * as THREE from 'three';
import { box, cyl, plane, rod, faced, mergeStatic, noMerge, floodSet, floodlit, addFlood, setFlood } from './kit';
import { eachVariant } from '../core/quality';
import type { Collider } from './ControlRoom';
import type { Zone } from '../player/Player';

// Chapter-four integration contract: todo.md at 1a08434. Local metres, north -Z.
// Interior x [-2.25, 2.25], z [-3, 3], floor y=0. South door x [-1.98, -.82].
// The covered walk is also at y=0. Zones overlap by .95 m and .70 m so Player's
// .27 m radius can cross the doorway. Only one live Room6 uses this flood set.
export const motelFlood = floodSet(6, 'motel', 0.2);
let floodOwner: Room6 | null = null;
type Anchor = { x: number; z: number; yaw: number };
export type Room6Anchors = { arrive: Anchor; talk: Anchor; exit: Anchor };
const yawTo = (x: number, z: number, tx: number, tz: number) => Math.atan2(x - tx, z - tz);

export class Room6 {
  readonly group = new THREE.Group();
  readonly interior = new THREE.Group();
  readonly proxies: Record<string, THREE.Object3D> = {};
  readonly objs: Record<string, THREE.Object3D> = {};
  readonly zones: Zone[] = [];
  readonly colliders: Collider[] = [];
  readonly anchors: Room6Anchors;
  readonly lights: { group: THREE.Group; hemi: THREE.HemisphereLight; moon: THREE.DirectionalLight };
  readonly flood = motelFlood;
  lampOn = true;

  private readonly o: THREE.Vector3;
  private readonly shell = new THREE.Group();
  private readonly fixed = new THREE.Group();
  private readonly materials = new Set<THREE.Material>();
  private readonly textures = new Set<THREE.Texture>();
  private readonly hitMaterial = new THREE.MeshBasicMaterial({ visible: false });
  private lampMaterial!: THREE.MeshStandardMaterial;
  private lampSlot = -1;
  private neonSlot = -1;
  private disposed = false;
  // the television: snow on every channel, lighting the room blue (Tom, 4 October)
  tvOn = false;
  private tvSlot = -1;
  private tvMaterial!: THREE.MeshStandardMaterial;
  private tvTex!: THREE.CanvasTexture;
  private tvAcc = 0;
  private tvGlow = 0;

  constructor(origin: THREE.Vector3) {
    this.o = origin.clone();
    this.group.name = 'Room6';
    this.group.position.copy(origin);
    this.interior.name = 'Room6Interior';
    this.group.add(this.shell, this.interior);
    this.interior.add(this.fixed);
    this.materials.add(this.hitMaterial);
    floodOwner = this;
    motelFlood.count = 0;
    for (const p of motelFlood.pos) p.set(0, -999, 0, 0);
    const m = this.makeMaterials();
    this.buildShell(m);
    this.buildFurniture(m);
    this.buildTable(m);
    this.buildNora(m);
    mergeStatic(this.shell);
    mergeStatic(this.fixed);

    const lg = new THREE.Group();
    const hemi = new THREE.HemisphereLight(0xa8bcd9, 0x5f4130, 0.55);
    const moon = new THREE.DirectionalLight(0x809ccc, 0.35);
    moon.position.set(-3, 5, 6);
    moon.target.position.set(0, 0, 0);
    lg.add(hemi, moon, moon.target);
    this.group.add(lg);
    this.lights = { group: lg, hemi, moon };
    this.lampSlot = this.light(1.18, 1.25, 1.15, 3.3, 0xffc078);
    this.light(-0.26, 1.18, -2.5, 2.3, 0xffc082);
    this.neonSlot = this.light(1.8, 1.5, 4.5, 0.9, 0xe7775b);
    this.light(0.85, 1.65, 3.2, 0.8, 0x709dcd);
    this.tvSlot = this.light(-1.4, 1.15, -0.55, 0, 0x7fa4ff);

    this.zone('room6', -2.25, 2.25, -3, 3);
    this.zone('room6Door', -1.98, -0.82, 2.05, 3.95);
    this.zone('room6Walk', -2.6, 2.6, 3.25, 5.05);
    const anchor = (x: number, z: number, yaw: number): Anchor => ({ x: origin.x + x, z: origin.z + z, yaw });
    this.anchors = {
      arrive: anchor(-1.4, 4.3, 0),
      talk: anchor(-0.42, 1.28, yawTo(-0.42, 1.28, 0.75, 2.3)),
      exit: anchor(-1.4, 4.6, Math.PI),
    };
    this.group.updateMatrixWorld(true);
  }

  private mat(p: THREE.MeshStandardMaterialParameters) {
    const m = floodlit(new THREE.MeshStandardMaterial(p), 0.55, motelFlood);
    this.materials.add(m); return m;
  }
  private basic(p: THREE.MeshBasicMaterialParameters) {
    const m = new THREE.MeshBasicMaterial(p); this.materials.add(m); return m;
  }
  private texture(w: number, h: number, draw: (g: CanvasRenderingContext2D) => void, repeat?: [number, number]) {
    const c = document.createElement('canvas'); c.width = w; c.height = h;
    draw(c.getContext('2d')!);
    const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; t.anisotropy = 2;
    if (repeat) { t.wrapS = t.wrapT = THREE.RepeatWrapping; t.repeat.set(...repeat); }
    this.textures.add(t); return t;
  }
  private makeMaterials() {
    const weave = this.texture(128, 128, g => {
      g.fillStyle = '#78634e'; g.fillRect(0, 0, 128, 128);
      g.fillStyle = '#836d53';
      for (let y = 0; y < 128; y += 4) for (let x = 0; x < 128; x += 4) g.fillRect(x + (y % 8 ? 1 : 0), y, 2, 1);
      g.strokeStyle = '#685947'; g.lineWidth = 2;
      for (let x = 0; x < 128; x += 32) { g.beginPath(); g.moveTo(x, 0); g.lineTo(x, 128); g.stroke(); }
    }, [5, 7]);
    const cloth = this.texture(256, 256, g => {
      g.fillStyle = '#b57643'; g.fillRect(0, 0, 256, 256);
      for (let y = 0; y < 256; y += 64) for (let x = 0; x < 256; x += 64) {
        g.fillStyle = ((x + y) / 64) % 2 ? '#744533' : '#986240';
        g.fillRect(x, y, 64, 15); g.fillRect(x + 24, y + 15, 16, 34); g.fillRect(x + 12, y + 26, 40, 12);
        g.fillStyle = '#4e625b'; g.fillRect(x, y + 51, 64, 6);
        g.fillStyle = '#d2b27c'; g.fillRect(x + 27, y + 23, 10, 14);
      }
    }, [2, 2]);
    const plaster = this.texture(128, 128, g => {
      g.fillStyle = '#c3b397'; g.fillRect(0, 0, 128, 128);
      let seed = 67;
      for (let n = 0; n < 1700; n++) { seed = (seed * 1664525 + 1013904223) >>> 0; const x = seed % 128; seed = (seed * 1664525 + 1013904223) >>> 0; g.fillStyle = n % 2 ? '#c9bba4' : '#b9ac95'; g.fillRect(x, seed % 128, 1, 1); }
    }, [2, 2]);
    return {
      wall: this.mat({ map: plaster, color: 0xd3bea1, roughness: 1 }),
      outer: this.mat({ color: 0xa68d75, roughness: 1 }),
      carpet: this.mat({ map: weave, color: 0xbba286, roughness: 1 }),
      wood: this.mat({ color: 0x73492d, roughness: 0.65 }),
      darkWood: this.mat({ color: 0x442c21, roughness: 0.75 }),
      teal: this.mat({ color: 0x43665e, roughness: 0.7 }),
      cloth: this.mat({ map: cloth, color: 0xceb795, roughness: 1 }),
      cream: this.mat({ color: 0xd5c5a8, roughness: 0.85 }),
      metal: this.mat({ color: 0x8c7b5b, roughness: 0.5, metalness: 0.35 }),
      black: this.mat({ color: 0x181b1c, roughness: 0.75 }),
      shade: this.mat({ color: 0xc7a870, emissive: 0xffbb68, emissiveIntensity: 0.55, roughness: 1 }),
      skin: this.mat({ color: 0xb69175, roughness: 1, flatShading: true }),
      hair: this.mat({ color: 0x52443c, roughness: 1, flatShading: true }),
      shirt: this.mat({ color: 0x647775, roughness: 1, flatShading: true }),
      trousers: this.mat({ color: 0x494a49, roughness: 1, flatShading: true }),
    };
  }
  private light(x: number, y: number, z: number, intensity: number, color: number) {
    return addFlood(x + this.o.x, y + this.o.y, z + this.o.z, intensity, color, motelFlood);
  }
  private zone(id: string, x0: number, x1: number, z0: number, z1: number) {
    this.zones.push({ id, enabled: true, minX: x0 + this.o.x, maxX: x1 + this.o.x, minZ: z0 + this.o.z, maxZ: z1 + this.o.z });
  }
  private col(x0: number, x1: number, z0: number, z1: number) {
    this.colliders.push({ minX: x0 + this.o.x, maxX: x1 + this.o.x, minZ: z0 + this.o.z, maxZ: z1 + this.o.z });
  }
  private proxy(id: string, w: number, h: number, d: number, x: number, y: number, z: number) {
    const p = box(this.group, w, h, d, this.hitMaterial, x, y, z);
    p.name = 'room6:' + id; this.proxies[id] = p; return p;
  }
  private model(id: string, x: number, y: number, z: number) {
    const g = new THREE.Group(); g.name = 'room6:' + id; g.position.set(x, y, z);
    noMerge(g); this.interior.add(g); this.objs[id] = g; return g;
  }
  private buildShell(m: ReturnType<Room6['makeMaterials']>) {
    const wall = (w: number, h: number, d: number, x: number, y: number, z: number, side: '+x' | '-x' | '+z' | '-z' | '-y') => {
      const f = faced(this.shell, w, h, d, m.outer, m.wall, x, y, z, side); this.fixed.add(f.face);
    };
    wall(0.16, 2.6, 6.16, -2.33, 1.3, 0, '+x'); wall(0.16, 2.6, 6.16, 2.33, 1.3, 0, '-x');
    wall(4.82, 2.6, 0.16, 0, 1.3, -3.08, '+z');
    for (const [a, b] of [[-2.25, -1.98], [-0.82, -0.2], [1.75, 2.25]]) wall(b-a, 2.6, 0.16, (a+b)/2, 1.3, 3.08, '-z');
    wall(1.16, 0.45, 0.16, -1.4, 2.375, 3.08, '-z');
    wall(1.95, 0.9, 0.16, 0.775, 0.45, 3.08, '-z'); wall(1.95, 0.5, 0.16, 0.775, 2.35, 3.08, '-z');
    wall(4.82, 0.18, 6.32, 0, 2.69, 0, '-y');
    plane(this.fixed, 4.5, 6, m.carpet, 0, 0, 0, 0, -Math.PI/2);
    box(this.shell, 5.4, 0.14, 2.2, m.outer, 0, -0.07, 4.1);
    box(this.shell, 5.4, 0.14, 2.3, m.teal, 0, 2.72, 4.05);
    for (const x of [-2.55, 2.55]) box(this.shell, 0.12, 2.65, 0.12, m.teal, x, 1.325, 5.05);
    this.col(-2.62, -2.48, 4.98, 5.12); this.col(2.48, 2.62, 4.98, 5.12);
    for (const x of [-2.2, 2.2]) box(this.fixed, 0.06, 0.12, 6, m.teal, x, 0.06, 0);
    box(this.fixed, 4.5, 0.12, 0.06, m.teal, 0, 0.06, -2.97);
    for (const x of [-1.98, -0.82]) box(this.shell, 0.06, 2.2, 0.2, m.teal, x, 1.1, 3.02);
    box(this.shell, 1.22, 0.07, 0.2, m.teal, -1.4, 2.17, 3.02);
    const door = this.model('door', -1.95, 0, 3.02); door.rotation.y = Math.PI/2;
    box(door, 1.1, 2.1, 0.06, m.teal, 0.55, 1.05, 0);
    box(door, 0.06, 0.12, 0.04, m.metal, 0.94, 1.02, -0.05);
    mergeStatic(door); this.col(-2.0, -1.89, 1.9, 3.05);
    this.proxy('door', 1.13, 2.1, 0.08, -1.4, 1.05, 3.04);
    const num = this.texture(64, 96, g => { g.fillStyle='#283e3c'; g.fillRect(0,0,64,96); g.fillStyle='#e5d7b8'; g.font='bold 68px Georgia'; g.textAlign='center'; g.fillText('6',32,73); });
    plane(this.shell, 0.18, 0.27, this.mat({ map: num, roughness: 0.8 }), -0.61, 1.75, 3.17);

    const night = this.texture(512, 256, g => {
      const sky=g.createLinearGradient(0,0,0,256); sky.addColorStop(0,'#071529'); sky.addColorStop(1,'#35536b'); g.fillStyle=sky; g.fillRect(0,0,512,256);
      g.fillStyle='#718594'; for(let i=0;i<31;i++)g.fillRect((i*137)%512,(i*37)%133,1,1);
      g.fillStyle='#192e41';g.beginPath();g.moveTo(0,205);g.lineTo(65,170);g.lineTo(95,181);g.lineTo(142,142);g.lineTo(195,144);g.lineTo(215,179);g.lineTo(345,165);g.lineTo(412,196);g.lineTo(512,180);g.lineTo(512,256);g.lineTo(0,256);g.fill();
      g.fillStyle='#382f36';g.fillRect(0,221,512,35);g.fillStyle='#bd7160';g.fillRect(371,156,7,51);
    });
    plane(this.fixed, 1.95, 1.2, this.basic({map:night,toneMapped:false}), .775, 1.5, 3.04, Math.PI);
    for(const x of [-.2,.775,1.75])box(this.fixed,.04,1.25,.09,m.teal,x,1.5,2.97);
    for(const y of [.88,2.12])box(this.fixed,2.06,.06,.14,m.teal,.775,y,2.96);
    for(const x of [-.21,1.65]) for(let k=0;k<4;k++)box(this.fixed,.12,1.49,.08,m.cloth,x+k*.055,1.46,2.8+(k%2)*.06);
    box(this.fixed,2.25,.18,.23,m.teal,.775,2.3,2.87);
    this.proxy('window',1.8,1.12,.12,.775,1.5,2.96);
    box(this.fixed,1.05,.42,.12,m.cream,.75,.33,2.85);
    for(let i=0;i<9;i++)box(this.fixed,.88,.018,.018,m.black,.75,.2+i*.032,2.781);
    // Bathroom remains closed and has no walkable zone behind it.
    box(this.fixed,.92,2.14,.05,m.teal,-.97,1.07,-2.96);
    for(const x of [-1.45,-.49])box(this.fixed,.05,2.2,.1,m.darkWood,x,1.1,-2.92);
    box(this.fixed,.98,.05,.1,m.darkWood,-.97,2.18,-2.92);
    box(this.fixed,.04,.12,.07,m.metal,-.6,1.02,-2.88);
    this.proxy('bathroom',.92,2.1,.12,-.97,1.05,-2.94);
  }

  private lamp(parent: THREE.Object3D, x: number, y: number, z: number, m: ReturnType<Room6['makeMaterials']>, controlled=false) {
    cyl(parent,.12,.15,.045,m.darkWood,x,y+.023,z,8);
    cyl(parent,.08,.12,.18,m.metal,x,y+.14,z,6);
    cyl(parent,.035,.035,.19,m.darkWood,x,y+.29,z,6);
    const shade=controlled?this.mat({color:0xc7a870,emissive:0xffbb68,emissiveIntensity:.8,roughness:1}):m.shade;
    cyl(parent,.13,.23,.3,shade,x,y+.47,z,8);
    if(controlled){shade.name='room6:tableLampShade';this.lampMaterial=shade;}
  }
  private buildFurniture(m: ReturnType<Room6['makeMaterials']>) {
    const bed=this.model('bed',1.12,0,-1.56);
    box(bed,1.88,.24,2.26,m.darkWood,0,.19,0);
    box(bed,1.82,.23,2.22,m.cream,0,.425,0);
    box(bed,1.87,.09,1.84,m.cloth,0,.58,.18);
    box(bed,.045,.28,1.84,m.cloth,-.947,.415,.18);box(bed,.045,.28,1.84,m.cloth,.947,.415,.18);
    box(bed,1.87,.3,.04,m.cloth,0,.43,1.13);
    box(bed,2.0,1.02,.12,m.wood,0,.69,-1.34);
    for(const x of [-.44,.44]) {const pillow=box(bed,.75,.14,.43,m.cream,x,.61,-.78);pillow.rotation.x=-.13;}
    mergeStatic(bed);this.col(.15,2.12,-2.98,-.38);this.proxy('bed',1.92,.85,2.36,1.12,.43,-1.56);
    const night=this.model('nightstand',-.26,0,-2.5);
    box(night,.55,.52,.6,m.wood,0,.28,0);box(night,.6,.05,.66,m.darkWood,0,.56,0);
    box(night,.12,.025,.04,m.metal,0,.32,.31);this.lamp(night,0,.585,0,m);mergeStatic(night);
    this.col(-.57,.04,-2.85,-2.14);
    const phone=this.model('phone',-.27,.6,-2.22);
    box(phone,.24,.07,.18,m.cream);box(phone,.045,.06,.21,m.black,-.085,.055,0);
    for(let r=0;r<4;r++)for(let c=0;c<3;c++)box(phone,.023,.005,.019,m.black,.005+c*.031,.039,-.056+r*.029);
    mergeStatic(phone);this.proxy('phone',.32,.22,.28,-.27,.66,-2.22);
    const dresser=this.model('dresser',-1.88,0,-.8);
    box(dresser,.64,.79,1.45,m.wood,0,.435,0);box(dresser,.72,.055,1.51,m.darkWood,0,.85,0);
    for(let i=0;i<3;i++){box(dresser,.035,.19,1.33,m.wood,.339,.22+i*.215,0);box(dresser,.055,.03,.2,m.metal,.37,.23+i*.215,0);}
    mergeStatic(dresser);this.col(-2.25,-1.48,-1.58,-.02);
    const tv=this.model('tv',-1.88,.88,-.55);tv.rotation.y=Math.PI/2;
    box(tv,.75,.54,.47,m.wood,0,.28,0);box(tv,.67,.45,.035,m.black,0,.28,.25);
    this.tvTex=this.texture(96,72,g=>{g.fillStyle='#000';g.fillRect(0,0,96,72);});
    const screen=this.mat({color:0x243336,roughness:.22,metalness:.1,emissive:0xffffff,emissiveMap:this.tvTex,emissiveIntensity:0});
    this.tvMaterial=screen;
    const crt=box(tv,.51,.36,.04,screen,-.055,.29,.279);crt.scale.x=.98;
    for(const y of [.21,.38])cyl(tv,.035,.035,.023,m.metal,.285,y,.284,8).rotation.x=Math.PI/2;
    for(let i=0;i<7;i++)box(tv,.09,.006,.012,m.darkWood,.278,.105+i*.012,.283);
    rod(tv,new THREE.Vector3(-.16,.56,0),new THREE.Vector3(-.37,.87,-.07),.006,m.metal,4);
    rod(tv,new THREE.Vector3(.16,.56,0),new THREE.Vector3(.35,.89,.02),.006,m.metal,4);
    mergeStatic(tv);this.proxy('tv',.55,.73,.82,-1.87,1.21,-.55);
    const photo=this.model('photo',-1.72,.89,-1.36);photo.rotation.y=Math.PI/2;
    const photoTex=this.texture(128,160,g=>{g.fillStyle='#b9ac8f';g.fillRect(0,0,128,160);g.fillStyle='#716b5e';g.fillRect(11,13,106,125);g.fillStyle='#b0a289';g.beginPath();g.moveTo(11,138);g.lineTo(37,80);g.lineTo(65,93);g.lineTo(91,60);g.lineTo(117,100);g.lineTo(117,138);g.fill();});
    box(photo,.23,.3,.035,m.darkWood,0,.15,0);plane(photo,.195,.258,this.mat({map:photoTex,roughness:.9}),0,.15,.021);mergeStatic(photo);
    this.proxy('photo',.15,.35,.29,-1.7,1.06,-1.36);
  }

  private chair(x: number,z: number,ry: number,m: ReturnType<Room6['makeMaterials']>) {
    const c=new THREE.Group();c.position.set(x,0,z);c.rotation.y=ry;
    box(c,.48,.065,.49,m.wood,0,.47,0);
    for(const xx of [-.195,.195])for(const zz of [-.195,.195])box(c,.052,.45,.052,m.darkWood,xx,.225,zz);
    for(const xx of [-.195,.195])box(c,.05,.55,.05,m.wood,xx,.75,.205);
    box(c,.46,.24,.045,m.wood,0,.91,.205);
    this.fixed.add(c);
    this.col(x-.29,x+.29,z-.29,z+.29);
  }
  private buildTable(m: ReturnType<Room6['makeMaterials']>) {
    const table=this.model('table',.7,0,1.45);
    box(table,1.35,.07,.95,m.wood,0,.775,0);
    for(const x of [-.55,.55])for(const z of [-.35,.35])box(table,.065,.73,.065,m.darkWood,x,.365,z);
    mergeStatic(table);this.col(.025,1.375,.975,1.925);this.proxy('table',1.35,.04,.95,.7,.775,1.45);
    this.chair(.75,2.3,0,m);this.chair(.75,.56,Math.PI,m);
    const lamp=this.model('lamp',1.18,.81,1.15);this.lamp(lamp,0,0,0,m,true);mergeStatic(lamp);
    this.proxy('lamp',.43,.63,.43,1.18,1.13,1.15);
    // Independent documents can be revealed/hidden by chapter four without
    // rebuilding the merged furniture. Writing is neutral; plot text belongs to UI.
    const card=(id:string,title:string,x:number,z:number,w:number,d:number,ry=0)=>{
      const tex=this.texture(256,192,g=>{g.fillStyle='#dfd2b4';g.fillRect(0,0,256,192);g.strokeStyle='#ad9b7a';g.strokeRect(9,9,238,174);g.fillStyle='#42443c';g.font='bold 17px monospace';g.fillText(title,18,36);g.fillStyle='#ad9b7a';for(let i=0;i<5;i++)g.fillRect(18,62+i*20,160+(i%3)*18,2);});
      const doc=this.model(id,x,.815,z);doc.rotation.y=ry;
      box(doc,w,.008,d,m.cream);plane(doc,w,d,this.mat({map:tex,roughness:1}),0,.006,0,0,-Math.PI/2);mergeStatic(doc);
      this.proxy(id,w+.025,.055,d+.025,x,.835,z);
    };
    card('fieldCard','FIELD CARD',.33,1.13,.28,.2,.08);
    card('letter','LETTER',.76,1.21,.31,.25,-.08);
    card('correction','CORRECTION',1.13,1.53,.28,.21,.12);
    const shoebox=this.model('shoebox',.93,.81,1.72);
    box(shoebox,.39,.14,.25,m.cream,0,.07,0);box(shoebox,.4,.015,.26,m.wood,.05,.15,.04);
    for(let i=0;i<4;i++)box(shoebox,.29,.008,.19,m.cream,-.018,.153+i*.009,-.025);
    mergeStatic(shoebox);
    // Tomás' field radio from 1947, lying on the table by the shoebox (Chapter4.ts, core/walkie.ts)
    const wk=this.model('walkie',.27,.81,1.62);wk.rotation.y=.25;
    const olive=this.mat({color:0x4b5135,roughness:.8});
    box(wk,.095,.075,.3,olive,0,.0375,0);box(wk,.07,.012,.09,m.black,0,.08,-.09);
    for(let i=0;i<4;i++)box(wk,.05,.004,.006,m.metal,0,.077,.03+i*.018);
    cyl(wk,.012,.012,.02,m.metal,.03,.085,-.12,8);
    rod(wk,new THREE.Vector3(-.03,.07,-.15),new THREE.Vector3(-.035,.075,.17),.004,m.metal,4);
    mergeStatic(wk);
    this.proxy('walkie',.18,.13,.38,.27,.86,1.62);
    cyl(this.fixed,.069,.08,.025,m.metal,.59,.83,1.59,12);
    cyl(this.fixed,.052,.052,.006,m.black,.59,.844,1.59,12);
  }

  private buildNora(m: ReturnType<Room6['makeMaterials']>) {
    const n=this.model('nora',.75,0,2.3);
    // Seated, facing north into the room. No facial features or dialogue animation.
    const torso=cyl(n,.19,.22,.44,m.shirt,0,.83,0,7);torso.scale.z=.68;
    box(n,.37,.15,.3,m.trousers,0,.535,-.05);
    for(const x of [-.11,.11]) {
      rod(n,new THREE.Vector3(x,.55,-.04),new THREE.Vector3(x,.49,-.33),.082,m.trousers,6);
      rod(n,new THREE.Vector3(x,.49,-.33),new THREE.Vector3(x,.12,-.3),.066,m.trousers,6);
      box(n,.135,.09,.25,m.darkWood,x,.07,-.37);
    }
    for(const side of [-1,1]) {
      rod(n,new THREE.Vector3(side*.18,.99,0),new THREE.Vector3(side*.23,.76,-.17),.065,m.shirt,6);
      rod(n,new THREE.Vector3(side*.23,.76,-.17),new THREE.Vector3(side*.17,.84,-.38),.047,m.skin,6);
      box(n,.072,.046,.11,m.skin,side*.17,.842,-.41);
    }
    cyl(n,.062,.063,.105,m.skin,0,1.095,0,7);
    const head=new THREE.Group();head.position.set(0,1.245,-.015);head.name='noraHead';noMerge(head);n.add(head);
    const face=new THREE.Mesh(new THREE.IcosahedronGeometry(.158,1),m.skin);face.scale.set(.85,1.17,.9);head.add(face);
    const hair=new THREE.Mesh(new THREE.IcosahedronGeometry(.163,1),m.hair);hair.position.set(0,.055,.055);hair.scale.set(.94,.84,.83);head.add(hair);
    mergeStatic(head);this.objs.noraHead=head;
    mergeStatic(n);this.proxy('nora',.61,1.44,.66,.75,.73,2.27);
  }

  setLamp(on: boolean) {
    this.lampOn=on;
    if (floodOwner===this) setFlood(this.lampSlot,on?3.3:0,motelFlood);
    eachVariant(this.lampMaterial,v=>{
      const m=v as THREE.MeshStandardMaterial;
      m.emissiveIntensity=on ? .8 : 0;
      m.color.set(on?0xc7a870:0x897652);
    });
  }
  /** Switch the television on (snow, a flickering blue light on the room) or off. */
  setTv(on: boolean) {
    this.tvOn=on;
    eachVariant(this.tvMaterial,v=>{
      const m=v as THREE.MeshStandardMaterial;
      m.emissiveIntensity=on ? 1.15 : 0;
      m.color.set(on?0x6d7c84:0x243336);
    });
    if(!on&&floodOwner===this)setFlood(this.tvSlot,0,motelFlood);
  }
  update(dt: number,t: number) {
    if(this.disposed)return;
    if(floodOwner===this)setFlood(this.neonSlot,.9*(.96+.04*Math.sin(t*.7)),motelFlood);
    if(!this.tvOn)return;
    // a new field of snow 15 times a second, a little rolling band in it, and the room's
    // light jumping with the brightness of the picture
    this.tvAcc+=dt;
    if(this.tvAcc>=1/15){
      this.tvAcc=0;
      const c=this.tvTex.image as HTMLCanvasElement,g=c.getContext('2d')!,img=g.createImageData(c.width,c.height),d=img.data;
      const band=(t*9)%c.height;
      for(let y=0;y<c.height;y++){
        const lift=Math.abs(y-band)<5?40:0;
        for(let x=0;x<c.width;x++){const v=Math.min(255,Math.random()*215+lift),i=(y*c.width+x)*4;d[i]=v*.92;d[i+1]=v*.96;d[i+2]=v;d[i+3]=255;}
      }
      g.putImageData(img,0,0);this.tvTex.needsUpdate=true;
      this.tvGlow+=((2.6+Math.random()*1.8)-this.tvGlow)*0.6;
    }
    if(floodOwner===this)setFlood(this.tvSlot,this.tvGlow,motelFlood);
  }
  dispose() {
    if(this.disposed)return;
    this.disposed=true;
    const geometries=new Set<THREE.BufferGeometry>();
    this.group.traverse(o=>{const mesh=o as THREE.Mesh;if(mesh.isMesh)geometries.add(mesh.geometry);});
    for(const g of geometries)g.dispose();
    const mats=new Set<THREE.Material>();
    for(const m of this.materials)eachVariant(m,v=>mats.add(v));
    for(const m of mats)m.dispose();
    for(const t of this.textures)t.dispose();
    if(floodOwner===this){for(const p of motelFlood.pos)p.w=0;motelFlood.count=0;floodOwner=null;}
    this.group.removeFromParent();this.group.clear();
  }
}
