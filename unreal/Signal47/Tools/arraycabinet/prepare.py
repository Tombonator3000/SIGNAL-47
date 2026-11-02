"""Extract only S-03 from the original merged field. Pure Python; no engine/DCC."""
import argparse, copy, hashlib, importlib.util, json, struct
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('trim_web', ROOT/'Tools/freemodels/trim_web.py')
h = importlib.util.module_from_spec(spec); spec.loader.exec_module(h)
PIVOT = (4.647, -.01, -39.72)
DEST = '/Game/ArrayCabinet'
def sha(b): return hashlib.sha256(b).hexdigest()
def selected(p): return 4.5 <= p[0] <= 5.6 and -.7 <= p[1] <= .7 and -39.9 <= p[2] <= -38.9
def prepare(export, out):
    manifest = json.loads((export/'manifest.json').read_text())
    source = export/'geo/geo_004.glb'; j, original = h._load(source)
    b = bytearray(original); meshes = []; rows = {}; parts = {'SM_S03CabinetBody': [], 'SM_S03CabinetDoor': []}
    partmats = {k: [] for k in parts}; nodes = {n.get('geo'): n for n in manifest['nodes'] if 'geo' in n}
    materialmap={}
    def source_material(mid):
        if mid not in materialmap:
            j.setdefault('materials',[]).append({'name':f'SourceMaterial_{mid}','pbrMetallicRoughness':{'baseColorFactor':[.5,.5,.5,1]}})
            materialmap[mid]=len(j['materials'])-1
        return materialmap[mid]
    def accessor(vals, kind, component=5126):
        width = len(vals[0]); fmt = 'f' if component == 5126 else 'I'
        while len(b)%4: b.append(0)
        start = len(b); b.extend(struct.pack('<'+fmt*width*len(vals), *(x for v in vals for x in v)))
        j['bufferViews'].append({'buffer':0,'byteOffset':start,'byteLength':len(b)-start})
        a = {'bufferView':len(j['bufferViews'])-1,'componentType':component,'count':len(vals),'type':kind}
        if kind == 'VEC3': a.update(min=[min(v[c]for v in vals)for c in range(3)],max=[max(v[c]for v in vals)for c in range(3)])
        j['accessors'].append(a); return len(j['accessors'])-1
    def primitive(attrs, triangles, material):
        flat = [i for t in triangles for i in t]; unique = list(dict.fromkeys(flat)); remap = {v:i for i,v in enumerate(unique)}
        aa = {k:accessor([v[i]for i in unique], 'VEC'+str(len(v[0])))for k,v in attrs.items()}
        return {'attributes':aa,'indices':accessor([(remap[i],)for i in flat],'SCALAR',5125),'material':material}
    for mesh in j['meshes'][:]:
        name=mesh.get('name',''); geo=int(name[1:]) if name.startswith('g')and name[1:].isdigit() else -1
        if geo not in (533,534,535,536): continue
        newprims=[]; removed=0; total=0; slots=[]
        for slot,p in enumerate(mesh['primitives']):
            attrs={k:h._values(j,original,v)for k,v in p['attributes'].items()}
            ids=[v[0]for v in h._values(j,original,p['indices'])]; tris=[ids[i:i+3]for i in range(0,len(ids),3)]
            cut=[t for t in tris if all(selected(attrs['POSITION'][i])for i in t)]
            keep=[t for t in tris if not all(selected(attrs['POSITION'][i])for i in t)]
            total+=len(tris); removed+=len(cut)
            if keep:
                q=copy.deepcopy(p);q['indices']=accessor([(i,)for t in keep for i in t],'SCALAR',5125);newprims.append(q);slots.append(slot)
            if not cut: continue
            part='SM_S03CabinetDoor' if geo==535 else 'SM_S03CabinetBody'
            if geo==534:
                # Remove just the solid cabinet front, leaving cap and all other original surfaces.
                cut=[t for t in cut if not all(abs(attrs['POSITION'][i][0]-4.65)<1e-4 for i in t)]
            if part.endswith('Door'):
                attrs['POSITION']=[tuple(v[c]-PIVOT[c]for c in range(3))for v in attrs['POSITION']]
            parts[part].append(primitive(attrs,cut,source_material(nodes[geo]['mats'][slot])));partmats[part].append(nodes[geo]['mats'][slot])
            if geo==534:
                # Thin inward-facing liner from the original steel faces, preserving UVs.
                inner=copy.deepcopy(attrs);inner['POSITION']=[tuple(v[c]-.012*attrs['NORMAL'][i][c]for c in range(3))for i,v in enumerate(attrs['POSITION'])]
                inner['NORMAL']=[tuple(-x for x in v)for v in attrs['NORMAL']]
                parts[part].append(primitive(inner,[t[::-1]for t in cut],source_material(nodes[geo]['mats'][slot])));partmats[part].append(nodes[geo]['mats'][slot])
                # Back of the door uses original front steel, offset behind the atlas face.
                front=[t for t in tris if all(selected(attrs['POSITION'][i])and abs(attrs['POSITION'][i][0]-4.65)<1e-4 for i in t)]
                back=copy.deepcopy(attrs);back['POSITION']=[tuple(v[c]-PIVOT[c]+(.012 if c==0 else 0)for c in range(3))for v in attrs['POSITION']]
                back['NORMAL']=[tuple(-x for x in v)for v in attrs['NORMAL']]
                parts['SM_S03CabinetDoor'].append(primitive(back,[t[::-1]for t in front],source_material(nodes[geo]['mats'][slot])));partmats['SM_S03CabinetDoor'].append(nodes[geo]['mats'][slot])
        assert removed, name
        meshes.append({'name':name,'primitives':newprims})
        rows[name]={'node':nodes[geo]['i'],'parent':nodes[geo]['p'],'geo':geo,'tris':total,'removed':removed,'resultTris':total-removed,'sourceMaterialIds':nodes[geo]['mats'],'originalMaterialIndices':slots,'asset':f'{DEST}/StaticMeshes/{name}.{name}'}
    assert {k:v['removed']for k,v in rows.items()}=={'g533':14,'g534':24,'g535':2,'g536':48}, 'Unexpected source geometry; inspect before adapting'
    # Four narrow steel edge faces close the original front/back into a real leaf.
    edgeattrs={'POSITION':[],'NORMAL':[],'TEXCOORD_0':[]}; edgetris=[]
    for pts,normal in [([(0,-.49,0),(.015,-.49,0),(.015,.49,0),(0,.49,0)],(0,0,-1)),
                       ([(0,-.49,.64),(0,.49,.64),(.015,.49,.64),(.015,-.49,.64)],(0,0,1)),
                       ([(0,.49,0),(.015,.49,0),(.015,.49,.64),(0,.49,.64)],(0,1,0)),
                       ([(0,-.49,0),(0,-.49,.64),(.015,-.49,.64),(.015,-.49,0)],(0,-1,0))]:
        start=len(edgeattrs['POSITION']); edgeattrs['POSITION'].extend(pts);edgeattrs['NORMAL'].extend([normal]*4);edgeattrs['TEXCOORD_0'].extend([(0,0),(1,0),(1,1),(0,1)])
        edgetris.extend([(start,start+2,start+1),(start,start+3,start+2)])
    steelmaterial=parts['SM_S03CabinetDoor'][0]['material']
    parts['SM_S03CabinetDoor'].append(primitive(edgeattrs,edgetris,steelmaterial));partmats['SM_S03CabinetDoor'].append(354)
    for name,pp in parts.items():
        # One primitive per material avoids importer-dependent duplicate slot merging.
        combined=[]; mids=[]
        for mat in dict.fromkeys(p['material']for p in pp):
            aa={};tt=[];mid=None
            for p,sourceid in zip(pp,partmats[name]):
                if p['material']!=mat:continue
                mid=sourceid; attrs={k:h._values(j,b,a)for k,a in p['attributes'].items()}
                offset=len(aa.get('POSITION',[]))
                for k,v in attrs.items():aa.setdefault(k,[]).extend(v)
                ids=[v[0]+offset for v in h._values(j,b,p['indices'])];tt.extend([ids[i:i+3]for i in range(0,len(ids),3)])
            combined.append(primitive(aa,tt,mat));mids.append(mid)
        parts[name]=combined;partmats[name]=mids
        meshes.append({'name':name,'primitives':combined})
    j['meshes']=meshes;j['nodes']=[{'name':m['name'],'mesh':i}for i,m in enumerate(meshes)];j['scenes']=[{'nodes':list(range(len(meshes)))}];j['scene']=0
    assert b[:len(original)]==original
    j['buffers'][0]['byteLength']=len(b);data=json.dumps(j,separators=(',',':')).encode();data+=b' '*(-len(data)%4);b+=b'\0'*(-len(b)%4)
    raw=struct.pack('<III',0x46546c67,2,28+len(data)+len(b))+struct.pack('<II',len(data),0x4e4f534a)+data+struct.pack('<II',len(b),0x004e4942)+b
    out.mkdir(parents=True,exist_ok=True);(out/'s03_cabinet.glb').write_bytes(raw)
    report={'version':1,'sourceSha256':sha(source.read_bytes()),'manifestSha256':sha((export/'manifest.json').read_bytes()),'fileSha256':sha(raw),'file':'s03_cabinet.glb','meshes':rows,'parts':{name:{'asset':f'{DEST}/StaticMeshes/{name}.{name}','sourceMaterialIds':partmats[name],'triangles':sum(j['accessors'][p['indices']]['count']//3 for p in pp)}for name,pp in parts.items()},'hingeWorld':PIVOT,'openRotationY':-1.4,'frontWorld':[4.647,-.01,-39.4],'scope':'Prepared original geometry; native import and visual runtime check still required'}
    (out/'array-cabinet.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('export',type=Path);p.add_argument('out',type=Path);a=p.parse_args();prepare(a.export,a.out)
