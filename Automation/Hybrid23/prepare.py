#!/usr/bin/env python3
"""Prepare an isolated, pinned UnitySplats hybrid scene; never modifies main Unity/."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import shutil
import struct
import subprocess

UPSTREAM_REVISION = '6c0258189a2b124af1282fa9236fd9b6637f1a1a'
NAMES = ['x','y','z','nx','ny','nz','f_dc_0','f_dc_1','f_dc_2','opacity','scale_0','scale_1','scale_2','rot_0','rot_1','rot_2','rot_3']

def digest(p):
    with p.open('rb') as f:
        return hashlib.file_digest(f,'sha256').hexdigest()

def checkout(path, revision):
    head = subprocess.check_output(['git','-C',str(path),'rev-parse','HEAD'], text=True).strip()
    if head != revision: raise SystemExit(f'Unexpected revision for {path.name}: {head}')
    if subprocess.check_output(['git','-C',str(path),'status','--porcelain'],text=True).strip():
        raise SystemExit(f'Upstream checkout must be clean: {path.name}')
    return head

def fixture(path):
    rows=[]
    def splat(x,y,z,rgb,size):
        rows.append([x,y,z,0,0,0,*[(v-.5)/.28209479177387814 for v in rgb],3.5,*([math.log(size)]*3),1,0,0,0])
    # Authored directly in Unity RUF. Decorative rock/soil patch east of the playable hut.
    for i in range(31):
        for j in range(31):
            splat(3.7+i*.15,.04,1.3+j*.15,(.37,.25,.16),.10)
    for cx,cz,r in [(5.0,3.1,.7),(6.7,4.6,1.0),(7.1,2.5,.55)]:
        for lat in range(13):
            theta=(lat+.5)/13*math.pi/2
            for lon in range(40):
                phi=lon/40*math.tau
                splat(cx+r*math.sin(theta)*math.cos(phi),r*math.cos(theta),cz+r*math.sin(theta)*math.sin(phi),(.50,.31,.19),.085)
    header='ply\nformat binary_little_endian 1.0\ncomment SIGNAL47 original synthetic RUF fixture, not scan or generated world\nelement vertex '+str(len(rows))+'\n'+''.join('property float '+n+'\n' for n in NAMES)+'end_header\n'
    with path.open('wb') as f:
        f.write(header.encode('ascii'))
        for row in rows:f.write(struct.pack('<17f',*row))
    return len(rows)

def ply_count(path, max_splats, max_bytes):
    if path.stat().st_size>max_bytes:raise SystemExit('PLY exceeds configured input byte budget')
    with path.open('rb') as f: raw=f.read(65536)
    end=raw.find(b'end_header\n')
    if end<0:raise SystemExit('Expected PLY header within 64 KiB')
    header=raw[:end].decode('ascii')
    if not header.startswith('ply\n'):raise SystemExit('Expected PLY file')
    counts=[int(line.split()[2]) for line in header.splitlines() if line.startswith('element vertex ')]
    if len(counts)!=1 or not 0<counts[0]<=max_splats:raise SystemExit('PLY vertex count outside configured budget')
    return counts[0]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--package-repo',required=True,type=Path)
    p.add_argument('--webp-repo',required=True,type=Path)
    p.add_argument('--webp-revision',required=True)
    p.add_argument('--output',required=True,type=Path)
    p.add_argument('--ply',type=Path)
    p.add_argument('--mesh-baseline',type=Path,help='Directory with aligned station-mesh.json and station-texture.jpg for same-source A/B')
    p.add_argument('--data-origin')
    p.add_argument('--source-coordinates',choices=['RUF','RUB','RDF','LUF','LDB','RDB','LUB','LDF'])
    p.add_argument('--position',nargs=3,type=float,default=[0,0,0])
    p.add_argument('--rotation',nargs=3,type=float,default=[0,0,0])
    p.add_argument('--scale',type=float,default=1)
    p.add_argument('--max-splats',type=int,default=250000)
    p.add_argument('--max-input-mib',type=int,default=128)
    p.add_argument('--proxy-relighting',action='store_true',help='Aligned generated mesh baseline, or authored synthetic fixture, supplies a collider-free lighting proxy.')
    a=p.parse_args();repo=Path(__file__).resolve().parents[2];out=a.output.resolve()
    if out == repo/'Unity' or repo/'Unity' in out.parents:raise SystemExit('Output must be isolated from the main Unity project')
    package=a.package_repo.resolve();webp=a.webp_repo.resolve()
    revision=checkout(package,UPSTREAM_REVISION);webprev=checkout(webp,a.webp_revision)
    webppackage=webp/'unity_project/Assets/unity.webp'
    if not (webppackage/'package.json').is_file():raise SystemExit('Unity.WebP package directory missing')
    if json.loads((webppackage/'package.json').read_text())['version']!='0.3.22':raise SystemExit('Expected Unity.WebP 0.3.22')
    if a.ply and (not a.data_origin or not a.source_coordinates):raise SystemExit('External PLY needs explicit --data-origin and --source-coordinates')
    if a.ply and a.proxy_relighting and not a.mesh_baseline:raise SystemExit('External proxy relighting requires a validated aligned --mesh-baseline')
    if not a.scale>0 or not all(math.isfinite(v) for v in a.position+a.rotation+[a.scale]):raise SystemExit('Invalid source transform')
    if a.ply: count=ply_count(a.ply.resolve(),a.max_splats,a.max_input_mib*1024*1024)
    mesh_hashes={}
    if a.mesh_baseline:
        if not a.ply or a.source_coordinates!='RUF' or a.position!=[0,0,0] or a.rotation!=[0,0,0] or a.scale!=1:
            raise SystemExit('Generated mesh baseline requires matching external RUF PLY at the normalized identity transform')
        baseline=a.mesh_baseline.resolve()
        for name in ['station-mesh.json','station-texture.jpg']:
            src=baseline/name
            if not src.is_file() or src.resolve().parent!=baseline:raise SystemExit('Baseline assets must be regular files within the supplied directory')
            if src.stat().st_size>a.max_input_mib*1024*1024:raise SystemExit('Generated baseline asset exceeds configured byte budget')
            mesh_hashes[name]=digest(src)
        mesh=json.loads((baseline/'station-mesh.json').read_text())
        positions=mesh.get('positions',[]);normals=mesh.get('normals',[]);uv=mesh.get('uv',[]);triangles=mesh.get('triangles',[])
        if len(positions)<9 or len(positions)%3 or len(normals)!=len(positions) or len(uv)!=len(positions)//3*2 or len(triangles)<3 or len(triangles)%3:
            raise SystemExit('Generated baseline needs flat positions/normals/uv/triangles arrays')
        if len(positions)//3>500000 or len(triangles)>1500000:raise SystemExit('Generated baseline exceeds mesh element budget')
        if not all(isinstance(v,(int,float)) and math.isfinite(v) for v in positions+normals+uv):raise SystemExit('Nonfinite baseline vertex data')
        if not all(type(i) is int and 0<=i<len(positions)//3 for i in triangles):raise SystemExit('Invalid baseline triangle index')
        for axis,low,high in [(0,-4.1,4.1),(1,-.1,2.9),(2,-.1,6.1)]:
            if not all(low<=v<=high for v in positions[axis::3]):raise SystemExit('Baseline must fit normalized STATION01 bounds')
        if (baseline/'station-texture.jpg').read_bytes()[:2]!=b'\xff\xd8':raise SystemExit('Expected original JPEG baseline texture')
    for name in ['Hybrid23Build.cs','Hybrid23Runtime.cs']:
        if not Path(__file__).with_name(name).is_file():raise SystemExit(f'Missing shared source: {name}')
    out.mkdir(parents=True,exist_ok=False)
    shutil.copytree(repo/'Unity/ProjectSettings',out/'ProjectSettings')
    (out/'Packages').mkdir();manifest=json.loads((repo/'Unity/Packages/manifest.json').read_text())
    deps=manifest['dependencies'];deps['com.arloopa.unitysplats']='file:'+str(package);deps['com.netpyoung.webp']='file:'+str(webppackage)
    for name,version in [('mathematics','1.3.3'),('render-pipelines.universal','17.3.0'),('inputsystem','1.20.0'),('modules.xr','1.0.0'),('modules.vr','1.0.0'),('modules.unitywebrequest','1.0.0')]:deps['com.unity.'+name]=version
    (out/'Packages/manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    hashes={}
    for name in ['Assets/UniversalRenderer.asset','Assets/UniversalRenderPipelineGlobalSettings.asset','Assets/Signal47/Art/PrototypePort/Signal47_URP.asset']:
        for ext in ['', '.meta']:
            src=repo/'Unity'/(name+ext);dst=out/(name+ext);dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dst);hashes[name+ext]=digest(src)
    (out/'Assets/Editor').mkdir();(out/'Assets/Probe').mkdir();(out/'Assets/Resources').mkdir()
    source=out/'Assets/Probe/environment.ply'
    if a.ply:shutil.copy2(a.ply,source)
    else:count=fixture(source)
    config={'dataOrigin':a.data_origin or 'Original synthetic SIGNAL47 soil/rock fixture; not a scan or generated World Labs scene','sourceCoordinates':a.source_coordinates or 'RUF','declaredSplatCount':count,'position':dict(zip('xyz',a.position)),'rotation':dict(zip('xyz',a.rotation)),'scale':a.scale,'proxyRelighting':a.proxy_relighting,'synthetic':not bool(a.ply),'generatedMesh':bool(a.mesh_baseline),'meshSha256':mesh_hashes.get('station-mesh.json',''),'textureSha256':mesh_hashes.get('station-texture.jpg','')}
    if a.mesh_baseline:
        for name in mesh_hashes:shutil.copy2(baseline/name,out/'Assets/Probe'/name)
    (out/'Assets/Resources/Hybrid23Config.json').write_text(json.dumps(config,indent=2)+'\n')
    for name,target in [('Hybrid23Build.cs','Editor'),('Hybrid23Runtime.cs','Probe')]:
        src=Path(__file__).with_name(name)
        if not src.is_file():raise SystemExit(f'Missing shared runtime source: {src}')
        shutil.copy2(src,out/'Assets'/target/name)
    (out/'preparation.json').write_text(json.dumps({'upstream_revision':revision,'webp_revision':webprev,'webp_version':'0.3.22','source_assets_sha256':hashes,'environment_sha256':digest(source),'generated_mesh_sha256':mesh_hashes,'config':config},indent=2)+'\n')
    print(out)

if __name__=='__main__':main()
