/* SIGNAL / 47 — Graphics pass 01. Original procedural art, no external assets.
 * WebGL1-compatible. Static geometry is batched; small moving groups remain separate.
 * Lighting uses analytic furniture shadows + soft contact darkening, not a shadow map.
 */
(function () {
'use strict';
window.Signal47Graphics = function (canvas) {
const gl = canvas.getContext('webgl', { antialias:true, alpha:false, powerPreference:'high-performance' });
if (!gl) throw new Error('WebGL is not available. Open the game in a browser with hardware acceleration enabled.');
let seed=4701986; const rand=()=>{seed=(Math.imul(seed,1664525)+1013904223)>>>0;return seed/4294967296;};
const clamp=(v,a,b)=>Math.max(a,Math.min(b,v));
const vertex=`
attribute vec3 aPos; attribute vec3 aNormal; attribute vec2 aUV;
attribute vec3 aColor; attribute vec2 aSurface;
uniform mat4 uViewProjection; uniform mat4 uModel;
varying vec3 vPos; varying vec3 vNormal; varying vec2 vUV; varying vec3 vColor; varying vec2 vSurface;
void main(){vec4 p=uModel*vec4(aPos,1.0); vPos=p.xyz;vNormal=normalize(mat3(uModel)*aNormal);vUV=aUV;vColor=aColor;vSurface=aSurface;gl_Position=uViewProjection*p;}`;
const fragment=`
precision mediump float;
varying vec3 vPos;varying vec3 vNormal;varying vec2 vUV;varying vec3 vColor;varying vec2 vSurface;
uniform sampler2D uAtlas; uniform vec3 uEye; uniform float uPower;uniform float uTime;uniform float uExposure;uniform float uFlash;
float insideBox(vec3 p,vec3 l,vec3 lo,vec3 hi){
 vec3 d=l-p;d=sign(d+vec3(.00001))*max(abs(d),vec3(.0001));
 vec3 a=(lo-p)/d,b=(hi-p)/d;vec3 mn=min(a,b),mx=max(a,b);
 float t0=max(max(mn.x,mn.y),mn.z),t1=min(min(mx.x,mx.y),mx.z);
 return step(max(t0,.003),min(t1,.985));
}
float shadow(vec3 p,vec3 l){
 float s=insideBox(p,l,vec3(-2.82,.8,-3.13),vec3(3.78,.99,-1.39));
 s=max(s,insideBox(p,l,vec3(3.41,.8,-.39),vec3(6.19,1.,1.39)));
 s=max(s,insideBox(p,l,vec3(-7.08,.03,.36),vec3(-5.52,2.54,1.53)));
 return 1.0-.77*s;
}
vec3 pointLight(vec3 p,vec3 n,vec3 l,vec3 col,float falloff,float spec,vec3 eye){
 vec3 d=l-p;float d2=dot(d,d);vec3 ld=normalize(d);
 float diffuse=max(dot(n,ld),0.0);float h=pow(max(dot(n,normalize(ld+normalize(eye-p))),0.0),32.0);
 float sh=shadow(p+n*.012,l);return col*(diffuse+spec*h*.38)*sh/(1.0+falloff*d2);
}
void main(){
 vec4 tex=texture2D(uAtlas,vUV);if(tex.a<.08)discard;
 vec3 n=normalize(vNormal);vec3 base=pow(max(tex.rgb*vColor,vec3(.001)),vec3(2.2));
 float indoors=smoothstep(-8.3,-7.6,vPos.z);
 vec3 light=vec3(.045,.061,.09)+max(n.y,0.0)*vec3(.03,.037,.04);
 float metal=vSurface.y;
 light+=max(dot(n,normalize(vec3(-.4,.65,-.9))),0.0)*vec3(.19,.3,.44)*(1.0+uFlash*2.);
 light+=pointLight(vPos,n,vec3(-4.4,3.66,0.),vec3(.75,.90,.79),.10,metal,uEye)*indoors;
 light+=pointLight(vPos,n,vec3(2.6,3.65,-2.2),vec3(.72,.86,.66),.105,metal,uEye)*indoors;
 light+=pointLight(vPos,n,vec3(5.6,3.67,2.6),vec3(.31,.40,.42),.14,metal,uEye)*indoors;
 light+=pointLight(vPos,n,vec3(0.,3.7,5.5),vec3(.53,.59,.46),.16,metal,uEye)*indoors;
 light+=pointLight(vPos,n,vec3(4.58,1.63,.36),vec3(2.9,1.24,.34),1.6,metal,uEye)*indoors;
 light+=pointLight(vPos,n,vec3(.0,1.5,-1.54),vec3(.16,1.0,.37)*uPower,1.2,metal,uEye)*indoors;
 if(vPos.y<.09&&indoors>.5){
  vec2 d=max(abs(vPos.xz-vec2(.45,-2.27))-vec2(3.25,.85),vec2(0.));
  float ao=1.-.48*exp(-dot(d,d)*3.3);
  d=max(abs(vPos.xz-vec2(4.8,.5))-vec2(1.4,.85),vec2(0.));ao*=1.-.42*exp(-dot(d,d)*3.8);
  light*=ao;
 }
 vec3 c=base*light; c=mix(c,base*2.3,vSurface.x);
 c*=uExposure;c=c/(vec3(.82)+c);c=pow(max(c,vec3(0.)),vec3(1.0/2.2));
 float fog=smoothstep(12.,115.,length(vPos.xz-uEye.xz))*(1.0-indoors);
 c=mix(c,vec3(.075,.112,.15)+vec3(.11,.14,.18)*uFlash,fog*.87);
 gl_FragColor=vec4(c,tex.a);
}`;
function shader(type,source){const s=gl.createShader(type);gl.shaderSource(s,source);gl.compileShader(s);if(!gl.getShaderParameter(s,gl.COMPILE_STATUS))throw Error(gl.getShaderInfoLog(s));return s;}
const program=gl.createProgram();gl.attachShader(program,shader(gl.VERTEX_SHADER,vertex));gl.attachShader(program,shader(gl.FRAGMENT_SHADER,fragment));gl.linkProgram(program);if(!gl.getProgramParameter(program,gl.LINK_STATUS))throw Error(gl.getProgramInfoLog(program));gl.useProgram(program);
const attrs={};for(const k of ['aPos','aNormal','aUV','aColor','aSurface'])attrs[k]=gl.getAttribLocation(program,k);
const uni={};for(const k of ['uViewProjection','uModel','uAtlas','uEye','uPower','uTime','uExposure','uFlash'])uni[k]=gl.getUniformLocation(program,k);
gl.enable(gl.DEPTH_TEST);gl.disable(gl.CULL_FACE);gl.depthFunc(gl.LEQUAL);

// Texture atlas: material surfaces and period-specific printed graphics.
const atlas=document.createElement('canvas');atlas.width=atlas.height=2048;const ctx=atlas.getContext('2d');
const tileNames=['white','floor','wall','ceiling','wood','steel','rack','keyboard','chart','map','log','paper','sign','vent','dial','note'];
const tileIds=Object.fromEntries(tileNames.map((n,i)=>[n,i]));
function paint(name,fn){let i=tileIds[name];ctx.save();ctx.translate((i%4)*512,Math.floor(i/4)*512);ctx.beginPath();ctx.rect(0,0,512,512);ctx.clip();fn(ctx);ctx.restore();}
function fill(c,col){c.fillStyle=col;c.fillRect(0,0,512,512);}
function grain(c,n,strength=12){for(let i=0;i<n;i++){let v=rand()>.5?255:0;c.fillStyle=`rgba(${v},${v},${v},${rand()*strength/100})`;c.fillRect(rand()*512,rand()*512,1+rand()*3,1+rand()*2);}}
function text(c,t,x,y,size=16,color='#222a28',align='left'){c.font=`${size}px "Courier New", monospace`;c.fillStyle=color;c.textAlign=align;c.fillText(t,x,y);}
function line(c,a,b,col,w=1){c.strokeStyle=col;c.lineWidth=w;c.beginPath();c.moveTo(...a);c.lineTo(...b);c.stroke();}
paint('white',c=>fill(c,'#ffffff'));
paint('floor',c=>{fill(c,'#697774');for(let y=0;y<2;y++)for(let x=0;x<2;x++){c.fillStyle=['#7e8880','#75837d','#899088','#7a857e'][(x+y*3)%4];c.fillRect(x*256+1,y*256+1,254,254);}grain(c,20000,5);c.strokeStyle='#4d5955';c.lineWidth=.8;for(let i=0;i<38;i++){c.beginPath();let x=rand()*512,y=rand()*512;c.ellipse(x,y,4+rand()*18,1+rand()*3,rand()*6,0,Math.PI*.6);c.stroke();}});
paint('wall',c=>{fill(c,'#cac5af');grain(c,22000,10);for(let i=0;i<50;i++){c.fillStyle='rgba(61,58,43,.025)';c.fillRect(rand()*512,rand()*512,10+rand()*95,10+rand()*100);}});
paint('ceiling',c=>{fill(c,'#abae9f');for(let x=0;x<4;x++)for(let y=0;y<4;y++){c.fillStyle='#c5c5b5';c.fillRect(x*128+1,y*128+1,126,126);}grain(c,12000,7);});
paint('wood',c=>{fill(c,'#776048');for(let i=0;i<1000;i++){let y=rand()*512;c.strokeStyle=`rgba(${rand()>.5?'25,19,13':'224,203,160'},${.025+rand()*.065})`;c.lineWidth=.3+rand();c.beginPath();c.moveTo(0,y);c.bezierCurveTo(170,y+rand()*10-5,340,y+rand()*9,512,y+rand()*10-5);c.stroke();}grain(c,4000,6);});
paint('steel',c=>{fill(c,'#89908a');grain(c,8000,12);for(let i=0;i<480;i++){let y=rand()*512;line(c,[0,y],[512,y],'rgba(40,50,47,.035)');}});
paint('rack',c=>{fill(c,'#b8b7a5');grain(c,6000,5);text(c,'SARO / RECEIVER BANK 03',27,37,21);for(let i=0;i<4;i++){let y=58+i*111;c.fillStyle='#232c28';c.fillRect(26,y,460,98);c.fillStyle='#d8d1ad';c.fillRect(40,y+15,149,51);for(let j=0;j<13;j++)line(c,[48+j*10,y+24],[48+j*10,y+31+(j%3===0?8:2)],'#454d38');line(c,[111,y+58],[147,y+29],'#343b2d',2);text(c,['IF LEVEL / dB','TRACKING ERROR','SYSTEM VOLTAGE','CARRIER STATUS'][i],207,y+25,15,'#bac5b0');text(c,['03 · H LINE','LOCAL / REMOTE','120 VAC  60 Hz','LOCK / ACQUIRE'][i],207,y+48,13,'#858e80');for(let j=0;j<15;j++)line(c,[212+j*17,y+64],[212+j*17,y+83],'#0e1410',3);}text(c,'PROPERTY OF SIERRA ARRAY RADIO OBSERVATORY',23,509,10);});
paint('keyboard',c=>{fill(c,'#b6b2a1');c.fillStyle='#56584f';c.fillRect(18,38,476,427);const chars=['1234567890-=','QWERTYUIOP[]','ASDFGHJKL;\'','ZXCVBNM,./'];for(let y=0;y<4;y++)for(let x=0;x<12;x++){c.fillStyle=y===0?'#a7a595':'#cecab9';c.fillRect(29+x*38,51+y*76,32,65);text(c,chars[y][x]||'',34+x*38,77+y*76,20);}c.fillStyle='#d7d0bc';c.fillRect(116,367,245,61);text(c,'SARO  //  DATA ENTRY',35,495,17);});
paint('chart',c=>{fill(c,'#152b33');for(let x=0;x<512;x+=32)line(c,[x,45],[x,487],'#284148');for(let y=45;y<490;y+=32)line(c,[0,y],[512,y],'#284148');text(c,'NORTHERN SKY / AUTUMN 1986',18,30,21,'#cacba9');for(let i=0;i<155;i++){c.fillStyle='#d8dbc1';c.beginPath();c.arc(15+rand()*480,50+rand()*430,.7+rand()*2,0,7);c.fill();}let pts=[[150,97],[263,135],[250,203],[216,267],[257,278],[294,286],[161,394],[324,415]];for(const [a,b] of [[0,1],[1,2],[2,3],[3,4],[4,5],[3,6],[5,7]])line(c,pts[a],pts[b],'#869997');for(const p of pts){c.fillStyle='#eee1b4';c.beginPath();c.arc(...p,4,0,7);c.fill();}text(c,'ORION',322,291,18,'#dec98e');text(c,'05h 17m       -05 DEG 23 MIN',22,486,15,'#babfa9');});
paint('map',c=>{fill(c,'#c7c3a5');for(let j=0;j<22;j++){c.strokeStyle='rgba(95,97,63,.2)';c.beginPath();for(let x=0;x<512;x+=8){let y=j*25+Math.sin(x*.03+j)*14;if(!x)c.moveTo(x,y);else c.lineTo(x,y);}c.stroke();}text(c,'SIERRA ARRAY / SITE SURVEY',19,34,21);let a=[257,260];for(const b of [[70,78],[443,100],[251,455]]){line(c,a,b,'#565a45',3);for(let t=.18;t<1;t+=.18){let x=a[0]+(b[0]-a[0])*t,y=a[1]+(b[1]-a[1])*t;c.fillStyle='#284b4b';c.fillRect(x-4,y-4,8,8);}}text(c,'N',477,78,22);line(c,[481,133],[481,88],'#333e35',2);text(c,'SERVICE ROAD 7  /  RESTRICTED',32,484,17);});
paint('log',c=>{fill(c,'#ddd4b5');grain(c,4000,6);text(c,'SARO // NIGHT SHIFT LOG',26,46,24);line(c,[26,64],[485,64],'#39453c',3);['SHIFT 23:30 - 07:30','LOCAL 23:41','[ ] POWER RECEIVER BANK 3','[ ] CALIBRATE ORION TRACK','[ ] NOTCH LOCAL INTERFERENCE','','ARRAY: nominal','WEATHER: distant lightning','','Night\'s yours.','Don\'t break anything. - R.'].forEach((t,i)=>text(c,t,29,105+i*32,i>8?21:19));});
paint('paper',c=>{fill(c,'#e0dac5');for(let y=0;y<512;y+=32){c.fillStyle=y%64===0?'#ced6bc':'#e1e2cc';c.fillRect(19,y,474,32);}for(let y=10;y<512;y+=24){c.fillStyle='#535d50';c.beginPath();c.arc(9,y,3,0,7);c.arc(503,y,3,0,7);c.fill();}['SARO // DIRECTION SOLVE','SOURCE: UNKNOWN','FREQ: 1420.405 MHz','RA: 05h 17m 32s','DEC: -05 23 14','S/N: 4.71','','DISTANCE: -39 LY','','*** RANGE SIGN ERROR ***','*** RE-RUN REQUIRED ***'].forEach((t,i)=>text(c,t,32,51+i*35,21));});
paint('sign',c=>{fill(c,'#254743');text(c,'S A R O',256,183,102,'#e0ddc0','center');line(c,[45,224],[467,224],'#c6c4a9',3);text(c,'SIERRA ARRAY',256,296,43,'#e0ddc0','center');text(c,'RADIO OBSERVATORY',256,345,32,'#e0ddc0','center');text(c,'NIGHT OPERATIONS / 03',256,425,23,'#b4c4b2','center');});
paint('vent',c=>{fill(c,'#b3b4a8');for(let y=28;y<490;y+=24){c.fillStyle='#2f3a37';c.fillRect(25,y,462,12);c.fillStyle='#dae0cc';c.fillRect(25,y+12,462,2);}for(let x of [11,501])for(let y of [10,502]){c.fillStyle='#4d5b56';c.fillRect(x-3,y-3,6,6);}});
paint('dial',c=>{fill(c,'#d4d0b9');for(let i=0;i<12;i++){let a=i/12*Math.PI*2,x=256+Math.sin(a)*210,y=256-Math.cos(a)*210;line(c,[256+Math.sin(a)*181,256-Math.cos(a)*181],[x,y],'#293630',6);}text(c,'SARO',256,179,30,'#46534b','center');text(c,'LOCAL TIME',256,358,19,'#596359','center');line(c,[256,256],[215,172],'#22362e',12);line(c,[256,256],[96,176],'#22362e',7);line(c,[256,278],[327,111],'#954b39',2);});
paint('note',c=>{fill(c,'#dbcf95');text(c,'RECEIVER NOTES',29,63,29);['REFERENCE TONE','1419.900 MHz','AZIMUTH 042 DEG','GAIN 55 +/-10','BW 48 +/-14','','DO NOT TRANSMIT','WITHOUT AUTHORIZATION'].forEach((s,i)=>text(c,s,29,120+i*45,26));});
function texture(source){let t=gl.createTexture();gl.bindTexture(gl.TEXTURE_2D,t);gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL,false);gl.texImage2D(gl.TEXTURE_2D,0,gl.RGBA,gl.RGBA,gl.UNSIGNED_BYTE,source);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MIN_FILTER,gl.LINEAR);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MAG_FILTER,gl.LINEAR);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_S,gl.CLAMP_TO_EDGE);gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_WRAP_T,gl.CLAMP_TO_EDGE);return t;}
const atlasTexture=texture(atlas);
const screen=document.createElement('canvas');screen.width=512;screen.height=256;const sx=screen.getContext('2d');const screenTexture=texture(screen);

// Small mesh builder. All static props go into one GPU buffer.
const identity=()=>new Float32Array([1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]);
function multiply(a,b){const m=new Float32Array(16);for(let c=0;c<4;c++)for(let r=0;r<4;r++){let v=0;for(let k=0;k<4;k++)v+=a[k*4+r]*b[c*4+k];m[c*4+r]=v;}return m;}
function matrix(p=[0,0,0],r=[0,0,0],s=1){let [x,y,z]=r;let cx=Math.cos(x),sx=Math.sin(x),cy=Math.cos(y),sy=Math.sin(y),cz=Math.cos(z),sz=Math.sin(z);let mx=identity(),my=identity(),mz=identity();mx[5]=cx;mx[6]=sx;mx[9]=-sx;mx[10]=cx;my[0]=cy;my[2]=-sy;my[8]=sy;my[10]=cy;mz[0]=cz;mz[1]=sz;mz[4]=-sz;mz[5]=cz;let m=multiply(my,multiply(mx,mz));for(let i=0;i<12;i++)m[i]*=s;m[12]=p[0];m[13]=p[1];m[14]=p[2];return m;}
const transform=(m,v,w=1)=>[m[0]*v[0]+m[4]*v[1]+m[8]*v[2]+m[12]*w,m[1]*v[0]+m[5]*v[1]+m[9]*v[2]+m[13]*w,m[2]*v[0]+m[6]*v[1]+m[10]*v[2]+m[14]*w];
const norm=v=>{let l=Math.hypot(...v)||1;return v.map(x=>x/l);};
const cross=(a,b)=>[a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]];
const sub=(a,b)=>a.map((x,i)=>x-b[i]);
const rgb=h=>[(h>>16&255)/255,(h>>8&255)/255,(h&255)/255];
function material(color=0xffffff,tile='white',emission=0,metal=.12){return {color:rgb(color),tile,emission,metal};}
const mats={wall:material(0xffffff,'wall'),lower:material(0x768c83,'wall'),floor:material(0xffffff,'floor'),ceiling:material(0xffffff,'ceiling'),steel:material(0xb4b9ae,'steel',0,.8),dark:material(0x252f2d),rim:material(0x536765,'steel',0,.7),beige:material(0xc9c1a8,'wall'),wood:material(0xffffff,'wood'),paper:material(0xdfd8c1),white:material(0xe6dfc7),rubber:material(0x182321),green:material(0x7ccb9d,'white',.9),amber:material(0xfac77b,'white',.8),lamp:material(0xe3ecd0,'white',1),screen:material(0xffffff,'full',1)};
function meshBuilder(){return {data:[]};}
function vertexPush(b,p,n,uv,mat){let u=uv[0],v=uv[1];if(mat.tile!=='full'){const i=tileIds[mat.tile];u=((i%4)*512+2+u*508)/2048;v=(Math.floor(i/4)*512+2+v*508)/2048;}b.data.push(...p,...n,u,v,...mat.color,mat.emission,mat.metal);}
function quad(b,pts,mat,n=null){n=n||norm(cross(sub(pts[1],pts[0]),sub(pts[2],pts[0])));const uvs=[[0,1],[1,1],[1,0],[0,0]];for(const i of [0,1,2,0,2,3])vertexPush(b,pts[i],n,uvs[i],mat);}
function box(b,p,size,mat,r=[0,0,0]){let m=matrix(p,r),[x,y,z]=size.map(v=>v/2);const f=[[[0,0,1],[[-x,-y,z],[x,-y,z],[x,y,z],[-x,y,z]]],[[0,0,-1],[[x,-y,-z],[-x,-y,-z],[-x,y,-z],[x,y,-z]]],[[1,0,0],[[x,-y,z],[x,-y,-z],[x,y,-z],[x,y,z]]],[[-1,0,0],[[-x,-y,-z],[-x,-y,z],[-x,y,z],[-x,y,-z]]],[[0,1,0],[[-x,y,z],[x,y,z],[x,y,-z],[-x,y,-z]]],[[0,-1,0],[[-x,-y,-z],[x,-y,-z],[x,-y,z],[-x,-y,z]]]];for(const [n,pts] of f)quad(b,pts.map(v=>transform(m,v)),mat,transform(m,n,0));}
function plane(b,p,size,mat,r=[0,0,0]){let m=matrix(p,r),[w,h]=size;quad(b,[[-w/2,-h/2,0],[w/2,-h/2,0],[w/2,h/2,0],[-w/2,h/2,0]].map(v=>transform(m,v)),mat);}
function rod(b,a,c,r,mat,segments=7){let d=norm(sub(c,a)),e=norm(cross(d,Math.abs(d[1])<.9?[0,1,0]:[1,0,0])),f=cross(d,e);for(let i=0;i<segments;i++){let aa=i/segments*Math.PI*2,ab=(i+1)/segments*Math.PI*2;let off=t=>e.map((v,j)=>r*(v*Math.cos(t)+f[j]*Math.sin(t))),u=off(aa),v=off(ab);quad(b,[a.map((x,j)=>x+u[j]),a.map((x,j)=>x+v[j]),c.map((x,j)=>x+v[j]),c.map((x,j)=>x+u[j])],mat);}}
function lathe(b,p,profile,mat,segments=32,r=[0,0,0]){const m=matrix(p,r);for(let j=0;j<profile.length-1;j++)for(let i=0;i<segments;i++){let a=i/segments*Math.PI*2,c=(i+1)/segments*Math.PI*2;let pt=(k,t)=>[Math.cos(t)*profile[k][0],profile[k][1],Math.sin(t)*profile[k][0]];quad(b,[pt(j,a),pt(j,c),pt(j+1,c),pt(j+1,a)].map(v=>transform(m,v)),mat);}}
function torus(b,p,major,minor,mat,r=[0,0,0],sweep=Math.PI*2){const m=matrix(p,r);for(let i=0;i<32;i++)for(let j=0;j<6;j++){let a=i/32*sweep,c=(i+1)/32*sweep,aa=j/6*Math.PI*2,cc=(j+1)/6*Math.PI*2;let pt=(t,u)=>transform(m,[(major+minor*Math.cos(u))*Math.cos(t),(major+minor*Math.cos(u))*Math.sin(t),minor*Math.sin(u)]);quad(b,[pt(a,aa),pt(c,aa),pt(c,cc),pt(a,cc)],mat);}}
function upload(b){if(b.data.some(v=>!Number.isFinite(v)))throw Error('Non-finite geometry');const buffer=gl.createBuffer();gl.bindBuffer(gl.ARRAY_BUFFER,buffer);const data=new Float32Array(b.data);gl.bufferData(gl.ARRAY_BUFFER,data,gl.STATIC_DRAW);return {buffer,count:data.length/13,data};}
const world=meshBuilder();
// Architecture: wall paint break, structural mullions, sill and a real ceiling.
box(world,[0,-.10,0],[19.8,.18,15.8],mats.floor);box(world,[0,4.09,0],[19.8,.14,15.8],mats.ceiling);
for(let ix=0;ix<10;ix++)for(let iz=0;iz<8;iz++){let x=-9.9+(ix+.5)*1.98,z=-7.9+(iz+.5)*1.975;plane(world,[x,0,z],[1.98,1.975],mats.floor,[-Math.PI/2,0,0]);plane(world,[x,4.0,z],[1.98,1.975],mats.ceiling,[Math.PI/2,0,0]);}
for(let side of [-1,1]){box(world,[side*9.87,2.04,0],[.2,4.08,15.8],mats.wall);box(world,[side*9.74,.65,0],[.035,1.3,15.8],mats.lower);box(world,[side*9.70,1.3,0],[.05,.045,15.8],mats.rim);box(world,[side*9.7,.06,0],[.08,.12,15.8],mats.rubber);}
box(world,[0,2.02,7.87],[19.8,4.04,.2],mats.wall);box(world,[0,.64,7.72],[19.8,1.28,.04],mats.lower);
box(world,[0,.48,-7.88],[19.8,.96,.22],mats.lower);box(world,[0,3.69,-7.88],[19.8,.64,.24],mats.wall);box(world,[0,.97,-7.74],[19.8,.10,.45],mats.steel);
for(let x of [-9.4,-6.2,-3.1,0,3.1,6.2,9.4]){box(world,[x,2.16,-7.88],[.12,2.34,.16],mats.rim);box(world,[x,2.16,-7.79],[.025,2.34,.04],mats.steel);}
for(let side of [-1,1]){box(world,[side*2.9,2.0,5.6],[.18,4,4.5],mats.wall);box(world,[side*2.79,.64,5.6],[.04,1.28,4.5],mats.lower);box(world,[side*2.77,1.3,5.6],[.06,.045,4.5],mats.rim);}
box(world,[0,3.74,3.31],[5.9,.5,.18],mats.lower);box(world,[0,3.9,-.7],[19.6,.14,.12],mats.steel);
// Acoustic ceiling grid, conduits and light housings.
for(let p of [[0,3.9,5.5],[-4.4,3.9,0],[2.6,3.9,-2.2],[5.6,3.9,2.6]]){box(world,p,[2.22,.14,.45],mats.dark);for(let z of [-.12,.12])rod(world,[p[0]-1.01,p[1]-.09,p[2]+z],[p[0]+1.01,p[1]-.09,p[2]+z],.035,mats.lamp,8);for(let x of [-1.06,1.06])box(world,[p[0]+x,p[1]-.10,p[2]],[.065,.1,.37],mats.steel);}
rod(world,[-9.6,3.55,-6.2],[-9.6,3.55,6.8],.035,mats.rim);rod(world,[-9.6,3.55,.9],[-9.6,.2,.9],.035,mats.rim);
// Doors, signs, charts and clock.
box(world,[0,1.35,7.72],[1.7,2.7,.12],mats.wood);box(world,[.68,1.15,7.6],[.08,.06,.15],mats.steel);
plane(world,[0,3.09,7.60],[.76,.76],material(0xffffff,'dial'),[0,Math.PI,0]);
plane(world,[0,3.35,3.17],[2.45,.4],material(0xffffff,'sign'),[0,0,0]);
box(world,[-1.72,1.34,5.14],[.69,.88,.07],mats.wood);plane(world,[-1.72,1.34,5.185],[.60,.78],material(0xffffff,'log'));box(world,[-1.72,1.76,5.21],[.27,.10,.018],mats.steel);
for(const [x,z,tile] of [[-8.55,-5.92,'chart'],[7.85,-5.9,'map']]){box(world,[x,2.25,z],[1.5,1.4,.11],mats.dark);plane(world,[x,2.25,z+.064],[1.35,1.24],material(0xffffff,tile));rod(world,[x-.65,.1,z],[x-.65,2.7,z],.024,mats.steel);rod(world,[x+.65,.1,z],[x+.65,2.7,z],.024,mats.steel);}
plane(world,[-9.745,2.6,-2.6],[1.8,1.4],material(0xffffff,'vent'),[0,Math.PI/2,0]);
// Receiver rack. Readable meters, recessed face plates, physical knobs and handles.
box(world,[-6.3,1.3,.93],[1.62,2.6,1.2],mats.dark);box(world,[-6.3,1.3,1.57],[1.45,2.5,.07],mats.steel);plane(world,[-6.3,1.3,1.612],[1.31,2.38],material(0xffffff,'rack'));
for(let y of [.39,.93,1.46,2.01]){for(let x of [-6.01,-5.7]){rod(world,[x,y,1.64],[x,y,1.72],.058,mats.rubber,12);rod(world,[x,y,1.723],[x,y+.032,1.723],.006,mats.white);}rod(world,[-6.91,y-.1,1.67],[-6.91,y+.12,1.67],.017,mats.dark);}
plane(world,[-6.3,2.82,.92],[1.3,.34],material(0xffffff,'sign'));box(world,[-6.3,2.81,.87],[1.4,.39,.08],mats.lower);
// Main bench: actual legs, drawers, controls and veneer edge, not a solid cube.
const benchStart=world.data.length;
box(world,[.45,1.32,-2.27],[6.6,.20,1.8],mats.wood);box(world,[.45,1.445,-2.27],[6.65,.06,1.82],material(0xa9af9c,'wall'));box(world,[.45,1.28,-1.35],[6.7,.085,.07],mats.rim);
for(let x of [-2.55,1.7,3.50])for(let z of [-2.98,-1.54])box(world,[x,.62,z],[.075,1.24,.075],mats.steel);
box(world,[-1.95,.69,-2.32],[1.18,1.13,1.38],mats.lower);for(let y of [.32,.69,1.06]){box(world,[-1.95,y,-1.615],[1.08,.32,.038],mats.steel);rod(world,[-2.12,y+.07,-1.56],[-1.78,y+.07,-1.56],.014,mats.dark);}
function terminal(x,z,scale=1,displayTile='full'){box(world,[x,1.6,z],[.97*scale,.19,.65],mats.beige);box(world,[x,1.96,z-.15],[1.1*scale,.75,.71],mats.beige);box(world,[x,1.97,z+.224],[1.03*scale,.64,.07],material(0xd9d1b8,'wall'));box(world,[x,2.0,z+.27],[.84*scale,.47,.08],mats.dark);box(world,[x,1.64,z+.28],[.1,.03,.02],mats.green);box(world,[x,1.51,z+.67],[1.25*scale,.09,.48],mats.beige);plane(world,[x,1.558,z+.67],[1.18*scale,.42],material(0xffffff,'keyboard'),[-Math.PI/2,0,0]);for(let i=0;i<5;i++)box(world,[x+.56*scale,1.77+i*.066,z-.10],[.01,.022,.43],mats.dark);}
terminal(0,-2.65);terminal(-1.5,-2.71,.93);terminal(1.43,-2.67,.88);
plane(world,[-1.5,2,-2.39],[.74,.40],material(0x91be9c,'map',.72));plane(world,[1.43,2,-2.35],[.70,.40],material(0x94c8af,'chart',.7));
plane(world,[-.80,1.49,-1.8],[.35,.44],material(0xffffff,'note'),[-Math.PI/2,0,.05]);
// Printer and fanfold tray.
box(world,[3.06,1.63,-2.39],[1.12,.38,.85],mats.beige);box(world,[3.06,1.80,-2.45],[1.02,.09,.32],mats.dark);rod(world,[2.58,1.85,-2.45],[3.54,1.85,-2.45],.06,mats.rubber,12);box(world,[3.06,1.45,-1.80],[1.2,.04,.52],mats.steel);box(world,[3.54,1.79,-2.08],[.035,.025,.06],mats.green);
for(let i=benchStart+1;i<world.data.length;i+=13)world.data[i]=world.data[i]<1.22?world.data[i]*(.77/1.22):world.data[i]-.45;
// Phone desk, old office supplies, task lamp.
const phoneDeskStart=world.data.length;
box(world,[4.8,1.34,.5],[2.8,.17,1.8],mats.wood);for(let x of [3.54,6.06])for(let z of [-.24,1.24])box(world,[x,.65,z],[.095,1.3,.095],mats.steel);
box(world,[5.08,1.54,.48],[.76,.23,.50],mats.beige,[.10,0,0]);for(let y=0;y<4;y++)for(let x=0;x<3;x++)box(world,[4.95+x*.075,1.685,.35+y*.066],[.058,.025,.05],mats.dark);box(world,[4.79,1.73,.53],[.14,.08,.27],mats.beige);box(world,[5.38,1.73,.53],[.14,.08,.27],mats.beige);
for(let i=0;i<38;i++){let t=i/37*Math.PI*2*7;const a=[5.5+Math.cos(t)*.037,1.52-i*.012,.67+Math.sin(t)*.037];let tn=(i+1)/37*Math.PI*2*7;const c=[5.5+Math.cos(tn)*.037,1.52-(i+1)*.012,.67+Math.sin(tn)*.037];rod(world,a,c,.014,mats.rubber,5);}
rod(world,[5.5,1.07,.67],[5.93,.10,.91],.016,mats.rubber,6);
lathe(world,[4.54,1.43,.37],[[0,0],[.20,0],[.20,.035],[.08,.055],[0,.055]],mats.dark,24);rod(world,[4.54,1.47,.37],[4.75,1.91,.4],.035,mats.steel);rod(world,[4.75,1.91,.4],[4.54,2.12,.36],.03,mats.steel);lathe(world,[4.54,2.04,.36],[[.20,0],[.19,.13],[.07,.22],[0,.22]],material(0x718d77,'steel'),24);lathe(world,[4.54,2.033,.36],[[0,0],[.175,0]],mats.amber,24);
for(let i=0;i<6;i++)box(world,[5.70,1.44+i*.006,.95],[.5,.005,.58],mats.paper,[0,(i-2)*.02,0]);plane(world,[5.7,1.483,.95],[.46,.54],material(0xffffff,'log'),[-Math.PI/2,0,0]);
for(let i=phoneDeskStart+1;i<world.data.length;i+=13)world.data[i]=world.data[i]<1.22?world.data[i]*(.8/1.22):world.data[i]-.42;
// Filing cabinets and floor-level cables; keep existing paths clear.
for(const [x,z] of [[-8.55,-4.6],[7.9,-4.2]]){box(world,[x,1.17,z],[1.45,2.34,1.1],mats.lower);for(let i=0;i<4;i++){let y=.32+i*.55;box(world,[x,y,z+.57],[1.32,.48,.04],mats.steel);rod(world,[x-.18,y+.10,z+.625],[x+.18,y+.10,z+.625],.018,mats.dark);box(world,[x,y-.03,z+.601],[.36,.09,.009],mats.paper);}for(let i=0;i<7;i++)box(world,[x-.45+i*.125,2.57,z],[.10,.45,.44],material([0x344f4b,0x6b6952,0x805348][i%3]),[0,0,.04]);}
for(let i=0;i<3;i++){rod(world,[-6.9,.045+i*.008,.1],[-6.8,.045+i*.008,-3.7],.022,mats.rubber);rod(world,[-6.8,.045+i*.008,-3.7],[-1.2,.045+i*.008,-3.7],.022,mats.rubber);}
// Office chair, rotated just off the central walking path.
box(world,[-2.8,.64,.0],[.78,.12,.70],material(0x405b54,'wall'));box(world,[-2.8,1.05,.35],[.76,.73,.12],material(0x405b54,'wall'),[-.14,0,0]);rod(world,[-2.8,.14,0],[-2.8,.57,0],.065,mats.steel);for(let i=0;i<5;i++){let a=i/5*Math.PI*2,x=-2.8+Math.cos(a)*.43,z=Math.sin(a)*.43;rod(world,[-2.8,.16,0],[x,.10,z],.03,mats.dark);box(world,[x,.067,z],[.12,.12,.08],mats.rubber);}
// Exterior ground, service road, layered rugged mesa silhouettes.
box(world,[0,-.28,-67],[240,.35,120],material(0x63756f,'floor'));box(world,[0,-.087,-55],[4.7,.012,92],material(0x77786a,'steel'));
for(let row=0;row<3;row++){let z=-70-row*29;let prev=[-135,0,z];for(let x=-135;x<=135;x+=5){let h=3+rand()*3+Math.pow(Math.sin(x*.02+row),4)*(5+row*2);let cur=[x,h,z];quad(world,[prev,cur,[x,-1,z],[prev[0],-1,z]],material([0x283e46,0x293c48,0x273641][row]));prev=cur;}}
for(let i=0;i<75;i++){let x=(rand()-.5)*150,z=-10-rand()*90;if(Math.abs(x)<4)continue;let s=.08+rand()*.21;box(world,[x,-.02,z],[s*1.7,s,s],material(0x657169),[rand(),rand(),rand()]);}
for(let i=0;i<15;i++){let z=-10-i*5.8;box(world,[-2.5,.21,z],[.04,.60,.04],mats.steel);box(world,[-2.5,.48,z],[.10,.065,.035],mats.amber);}
// Stars rendered as small world-space quads and restrained moon.
for(let i=0;i<430;i++){let x=(rand()-.5)*260,y=12+rand()*83,z=-100-rand()*70,s=.035+Math.pow(rand(),5)*.13;plane(world,[x,y,z],[s,s],material(0xbacdc9,'white',.9));}
lathe(world,[-37,37,-120],[[0,0],[1.4,0]],material(0xb4c7c6,'white',.7),36,[Math.PI/2,0,0]);
const staticMesh=upload(world);
// Parabolic reflector (y = r²/12) with feed supports, rim and ribs.
const dish=meshBuilder(),dishMat=material(0xb4c4bf,'steel',0,.75);
const profile=[];for(let i=0;i<=14;i++){let r=i/14*4.2;profile.push([r,r*r/12]);}lathe(dish,[0,0,0],profile,dishMat,56);
torus(dish,[0,4.2*4.2/12,0],4.2,.055,mats.steel,[Math.PI/2,0,0]);
for(let i=0;i<12;i++){let a=i/12*Math.PI*2;for(let j=0;j<6;j++){let r=j/6*4.2,rr=(j+1)/6*4.2;rod(dish,[Math.cos(a)*r,r*r/12-.12,Math.sin(a)*r],[Math.cos(a)*rr,rr*rr/12-.12,Math.sin(a)*rr],.041,mats.rim,5);}}
for(let i=0;i<3;i++){let a=i/3*Math.PI*2;rod(dish,[Math.cos(a)*3.8,1.2,Math.sin(a)*3.8],[0,3.35,0],.055,mats.steel,7);}lathe(dish,[0,3.30,0],[[.13,0],[.13,.46],[.05,.53]],mats.dark,16);box(dish,[0,-.37,0],[.58,.61,.55],mats.steel);
const dishMesh=upload(dish);const pedestal=meshBuilder();box(pedestal,[0,-.08,0],[2.5,.24,2.5],material(0x89948e,'wall'));box(pedestal,[0,.84,0],[.83,1.65,.85],mats.lower);for(let x of [-.66,.66]){box(pedestal,[x,2.20,0],[.24,2.12,.5],mats.steel);rod(pedestal,[x,1.24,.0],[x,2.0,.80],.11,mats.steel);}rod(pedestal,[-.82,3.16,0],[.82,3.16,0],.16,mats.dark);const pedestalMesh=upload(pedestal);
const dishes=[[-8,-21,1],[3,-25,.95],[14,-32,.95],[-19,-35,.95],[-5,-43,.9],[9,-51,.9],[25,-60,.86],[-30,-58,.85],[-15,-73,.85],[0,-80,.85],[35,-88,.8]];
// Animated groups: actual screen, status indicators, fanfold printout, telephone handset, mug & fragments.
const screenB=meshBuilder();plane(screenB,[0,1.55,-2.321],[.78,.425],mats.screen);const screenMesh=upload(screenB);
const lights=meshBuilder();for(let y of [.39,.93,1.46,2.01])box(lights,[-6.72,y,1.633],[.053,.029,.007],mats.green);const ledMesh=upload(lights);
const printB=meshBuilder();plane(printB,[3.06,1.10,-1.77],[.75,.74],material(0xffffff,'paper'),[-Math.PI/2+.19,0,0]);const printMesh=upload(printB);
const handset=meshBuilder();box(handset,[0,0,0],[.55,.07,.13],mats.beige);for(let x of [-.30,.30]){box(handset,[x,-.035,0],[.18,.14,.18],mats.beige);box(handset,[x,-.114,0],[.12,.022,.12],mats.dark);}const handsetMesh=upload(handset);
const mug=meshBuilder();lathe(mug,[0,0,0],[[0,-.19],[.13,-.19],[.155,.17],[.135,.19],[.118,.165],[.10,-.145],[0,-.145]],material(0xe4ddc7,'white',0,.65),28);lathe(mug,[0,.105,0],[[0,0],[.117,0]],material(0x33281f),28);torus(mug,[.172,-.006,0],.095,.023,mats.white,[0,0,0]);const mugMesh=upload(mug);
const shardB=meshBuilder();for(let i=0;i<10;i++){let x=(rand()-.5)*.68,z=(rand()-.5)*.64;box(shardB,[4.03+x,.025+rand()*.028,-.01+z],[.08+rand()*.12,.021,.06+rand()*.06],mats.white,[rand()*.6,rand()*6,rand()*.6]);}const shardMesh=upload(shardB);
let lastScreen=-1,drawCalls=0,triangles=0;
function updateScreen(now,state){if(now-lastScreen<100)return;lastScreen=now;const power=state.receiver;fill(sx,'#04130d');if(power){text(sx,'SARO / RX CONTROL 03',20,28,19,'#91d2a0');text(sx,state.signalAcquired?'SIGNAL ACQUIRED':state.consolePhase.toUpperCase(),20,52,14,'#91d2a0');for(let x=20;x<500;x+=40)line(sx,[x,70],[x,214],'#123624');for(let y=74;y<220;y+=28)line(sx,[20,y],[494,y],'#123624');sx.beginPath();sx.lineWidth=1.7;sx.strokeStyle='#95ffb2';for(let x=22;x<492;x++){let peak=state.consolePhase==='calibration'?210:state.consolePhase==='interference'?286:356;let y=186-65*Math.exp(-Math.pow((x-peak)/12,2))+Math.sin(x*.38+now*.004)*2;if(x===22)sx.moveTo(x,y);else sx.lineTo(x,y);}sx.stroke();text(sx,state.printout?'DISTANCE SOLVE: -39 LY':'1420 BAND / STANDBY',22,238,16,'#95d3a7');for(let y=0;y<256;y+=3){sx.fillStyle='rgba(0,0,0,.16)';sx.fillRect(0,y,512,1);}}else{text(sx,'RECEIVER OFFLINE',256,126,24,'#284a36','center');text(sx,'BANK 3 / NO POWER',256,152,14,'#234231','center');}gl.bindTexture(gl.TEXTURE_2D,screenTexture);gl.texSubImage2D(gl.TEXTURE_2D,0,0,0,gl.RGBA,gl.UNSIGNED_BYTE,screen);}
function bind(mesh,modelMatrix,tex=atlasTexture){gl.bindBuffer(gl.ARRAY_BUFFER,mesh.buffer);const stride=13*4;let offset=0;for(const [a,n] of [['aPos',3],['aNormal',3],['aUV',2],['aColor',3],['aSurface',2]]){gl.enableVertexAttribArray(attrs[a]);gl.vertexAttribPointer(attrs[a],n,gl.FLOAT,false,stride,offset*4);offset+=n;}gl.bindTexture(gl.TEXTURE_2D,tex);gl.uniformMatrix4fv(uni.uModel,false,modelMatrix);gl.drawArrays(gl.TRIANGLES,0,mesh.count);drawCalls++;triangles+=mesh.count/3;}
let exposure=1.05,reduced=matchMedia('(prefers-reduced-motion: reduce)').matches;const I=identity();
function render(now,player,state,animation){
 const dpr=Math.min(reduced?1:1.5,devicePixelRatio||1);const w=Math.max(1,Math.round(innerWidth*dpr)),h=Math.max(1,Math.round(innerHeight*dpr));if(canvas.width!==w||canvas.height!==h){canvas.width=w;canvas.height=h;}gl.viewport(0,0,w,h);
 const flash=reduced?0:Math.pow(Math.max(0,Math.sin(now*.00015+1.3)),120)*.24;
 gl.clearColor(.030+flash*.1,.053+flash*.13,.081+flash*.16,1);gl.clear(gl.COLOR_BUFFER_BIT|gl.DEPTH_BUFFER_BIT);gl.useProgram(program);
 let f=1/Math.tan(Math.PI/6),aspect=w/h,near=.065,far=230;const projection=new Float32Array([f/aspect,0,0,0,0,f,0,0,0,0,(far+near)/(near-far),-1,0,0,2*far*near/(near-far),0]);
 const rotX=matrix([0,0,0],[-player.pitch,0,0]),rotY=matrix([0,0,0],[0,-player.yaw,0]),translate=matrix([-player.x,-player.y,-player.z]);const view=multiply(rotX,multiply(rotY,translate));
 gl.uniformMatrix4fv(uni.uViewProjection,false,multiply(projection,view));gl.uniform3f(uni.uEye,player.x,player.y,player.z);gl.uniform1f(uni.uPower,state.receiver?1:0);gl.uniform1f(uni.uTime,now*.001);gl.uniform1f(uni.uExposure,exposure);gl.uniform1f(uni.uFlash,flash);gl.uniform1i(uni.uAtlas,0);
 updateScreen(now,state);drawCalls=0;triangles=0;bind(staticMesh,I);
 for(let i=0;i<dishes.length;i++){let [x,z,s]=dishes[i];bind(pedestalMesh,matrix([x,0,z],[0,0,0],s));const turn=animation.dishFinal*.46;bind(dishMesh,matrix([x,3.16*s,z],[.66,(i%3-.7)*.18+turn,0],s));}
 bind(screenMesh,I,screenTexture);if(state.receiver)bind(ledMesh,I);if(state.printout)bind(printMesh,I);
 bind(handsetMesh,matrix(state.phoneAnswered?[5.34,1.32,.78]:[5.08,1.37,.53],state.phoneAnswered?[.25,.2,-.1]:[0,0,state.phoneRinging?Math.sin(now*.06)*.02:0]));
 if(state.mugBroken)bind(shardMesh,I);else {let t=animation.mugDropStart?clamp((now-animation.mugDropStart)/700,0,1):0;bind(mugMesh,matrix([4.02,1.215-.99*t*t,-.18+.35*t],[t*2.2,t*.8,t*2.8]));}
}
return {render,setExposure:v=>exposure=clamp(v,.65,1.75),setReduced:v=>reduced=!!v,get stats(){return{drawCalls,triangles,staticVertices:staticMesh.count,textures:2,version:'0.2.0'};},exportGeometry(){return {stride:13,data:Array.from(staticMesh.data),vertex,fragment};},atlas};
};
})();
