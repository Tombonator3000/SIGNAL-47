#if UNITY_EDITOR
using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using UnityEditor;
using UnityEngine;
using UnityEngine.Rendering;
using Object=UnityEngine.Object;
using M=Signal47.Editor.Environment27Materials;
namespace Signal47.Editor
{
    /// <summary>Reproducible environment authoring over the intact Station26 interactions.</summary>
    public static class Environment27Pass
    {
        const string Art="Assets/Signal47/Art/Environment27/";
        static Transform field,root;
        static Font font;
        static Material textMaterial;
        static System.Random rng;
        static readonly List<GameObject> merge=new List<GameObject>();
        static readonly List<Mesh> transientMeshes=new List<Mesh>();
        static readonly Dictionary<string,string> importAudit=new Dictionary<string,string>();
        static readonly Dictionary<int,Mesh> grassMeshes=new Dictionary<int,Mesh>();
        static readonly Dictionary<int,List<Matrix4x4>> grassMatrices=new Dictionary<int,List<Matrix4x4>>();
        static float Random(float lo,float hi)=>lo+(hi-lo)*(float)rng.NextDouble();
        static Transform Node(string name,Vector3 position)
        {
            var go=new GameObject("E27."+name);go.transform.SetParent(root,false);go.transform.localPosition=position;return go.transform;
        }
        static Transform Find(string name)=>field.GetComponentsInChildren<Transform>(true).First(t=>t.name=="Station26."+name);
        static void Hide(string name)
        {
            foreach(var t in field.GetComponentsInChildren<Transform>(true).Where(t=>t.name=="Station26."+name))
                foreach(var r in t.GetComponentsInChildren<Renderer>(true))r.enabled=false;
        }
        // World-sized face UVs keep plaster, gravel and hardware texels at physical scale.
        static GameObject Box(string name,Vector3 p,Vector3 size,Material mat,bool collider=false,Vector3 rotation=default)
        {
            var go=GameObject.CreatePrimitive(PrimitiveType.Cube);go.name="E27."+name;go.transform.SetParent(root,false);
            go.transform.localPosition=p;go.transform.localRotation=Quaternion.Euler(rotation);
            var original=go.GetComponent<MeshFilter>().sharedMesh;var mesh=Object.Instantiate(original);mesh.name=name;
            var verts=mesh.vertices;var normals=mesh.normals;var uv=new Vector2[verts.Length];
            for(int i=0;i<verts.Length;i++){verts[i]=Vector3.Scale(verts[i],size);var v=verts[i];var n=normals[i];uv[i]=Mathf.Abs(n.y)>.9f?new Vector2(v.x,v.z):Mathf.Abs(n.x)>.9f?new Vector2(v.z,v.y):new Vector2(v.x,v.y);}
            mesh.vertices=verts;mesh.uv=uv;mesh.RecalculateBounds();go.GetComponent<MeshFilter>().sharedMesh=mesh;transientMeshes.Add(mesh);
            go.GetComponent<Renderer>().sharedMaterial=mat;
            if(collider)go.GetComponent<BoxCollider>().size=size;else Object.DestroyImmediate(go.GetComponent<Collider>());
            merge.Add(go);return go;
        }
        static void Beam(string name,Vector3 a,Vector3 b,float width,Material mat)
        {
            var go=Box(name,(a+b)*.5f,new Vector3(width,width,(a-b).magnitude),mat);go.transform.localRotation=Quaternion.LookRotation(b-a);
        }
        static GameObject Model(string file,Vector3 pos,float yaw=0,float scale=1,string directory=null)
        {
            var path=(directory??Art+"Models/")+file+".fbx";var prefab=AssetDatabase.LoadAssetAtPath<GameObject>(path);
            if(!prefab)throw new FileNotFoundException(path);
            var placement=Node(file,pos);placement.localRotation=Quaternion.Euler(0,yaw,0);placement.localScale=Vector3.one*scale;
            var go=(GameObject)PrefabUtility.InstantiatePrefab(prefab);go.transform.SetParent(placement,false);
            foreach(var rr in go.GetComponentsInChildren<Renderer>()){rr.sharedMaterials=rr.sharedMaterials.Select(m=>M.Named(m?m.name:"")).ToArray();merge.Add(rr.gameObject);}
            if(!importAudit.ContainsKey(file))
            {
                var rs=go.GetComponentsInChildren<Renderer>();var bounds=rs[0].bounds;foreach(var rr in rs)bounds.Encapsulate(rr.bounds);
                importAudit[file]=bounds.ToString("F4");
                float height=bounds.size.y/scale;
                if(file=="E27_SurveyTransit"&&(height<1.35f||height>1.5f)||file=="E27_LampPole"&&(height<1.65f||height>1.8f)||file=="E27_AnalogReceiver"&&(height<.38f||height>.44f))
                    throw new InvalidDataException("Environment27 imported scale/axis failed: "+file+" "+bounds);
            }
            if(file.StartsWith("E27_Boulder")&&scale>.4f&&Mathf.Abs(pos.x)<14.5f&&pos.z> -9&&pos.z<21)
            {
                var rs=go.GetComponentsInChildren<Renderer>();var bounds=rs[0].bounds;foreach(var rr in rs)bounds.Encapsulate(rr.bounds);
                var contact=Node("Rock contact",root.InverseTransformPoint(bounds.center));contact.localScale=bounds.size*.74f;
                contact.gameObject.AddComponent<SphereCollider>().radius=.5f;
            }
            return go;
        }
        static void Plaque(string name,string text,Vector3 position,Vector2 size,float yaw=0,float letters=.022f)
        {
            Box(name+" backing",position,new Vector3(size.x,size.y,.035f),M.Metal,false,new Vector3(0,yaw,0));
            var tr=Node(name+" lettering",position+Quaternion.Euler(0,yaw,0)*new Vector3(0,0,-.021f));tr.localRotation=Quaternion.Euler(0,yaw,0);
            var tm=tr.gameObject.AddComponent<TextMesh>();tm.font=font;tm.fontSize=64;tm.characterSize=letters;tm.anchor=TextAnchor.MiddleCenter;tm.alignment=TextAlignment.Center;tm.color=new Color(.82f,.78f,.61f);tm.text=text;tr.GetComponent<Renderer>().sharedMaterial=textMaterial;
            var textSize=tr.GetComponent<Renderer>().localBounds.size;
            float fit=Mathf.Min(1,Mathf.Min((size.x-.06f)/Mathf.Max(.001f,textSize.x),(size.y-.025f)/Mathf.Max(.001f,textSize.y)));
            tr.localScale=Vector3.one*fit;
            foreach(float x in new[]{-.5f,.5f})foreach(float y in new[]{-.5f,.5f})
                Box("Plaque screw",position+Quaternion.Euler(0,yaw,0)*new Vector3(x*(size.x-.04f),y*(size.y-.04f),-.021f),new Vector3(.009f,.009f,.008f),M.Steel);
        }
        // Flat, worn investigation yard transitions into a sloping arroyo and scrub banks.
        public static float Height(float x,float z)
        {
            float edge=Mathf.Max(Mathf.Abs(x)-9f,Mathf.Max(-z-10,z-17));
            float blend=Mathf.SmoothStep(0,1,Mathf.Clamp01(edge/12));
            float noise=Mathf.PerlinNoise(x*.033f+4.3f,z*.033f+3.1f)*3.3f+Mathf.PerlinNoise(x*.13f+7,z*.13f+2)*.55f;
            float wash=-1.8f*Mathf.Exp(-Mathf.Pow((x-22-Mathf.Sin(z*.07f)*4)/4,2));
            return -.025f+blend*(noise+wash);
        }
        static Mesh StoreMesh(string name,Mesh source)
        {
            string path=Art+name+".asset";var old=AssetDatabase.LoadAssetAtPath<Mesh>(path);
            if(old){EditorUtility.CopySerialized(source,old);Object.DestroyImmediate(source);return old;}
            source.name=name;AssetDatabase.CreateAsset(source,path);return source;
        }
        static void Terrain()
        {
            const int n=145;const float step=1.2f;var verts=new Vector3[n*n];var uv=new Vector2[n*n];var triangles=new int[(n-1)*(n-1)*6];
            for(int j=0;j<n;j++)for(int i=0;i<n;i++){float x=(i-(n-1)*.5f)*step,z=(j-(n-1)*.5f)*step+5;verts[j*n+i]=new Vector3(x,Height(x,z),z);uv[j*n+i]=new Vector2(x,z)*.34f;}
            int k=0;for(int j=0;j<n-1;j++)for(int i=0;i<n-1;i++){int a=j*n+i;triangles[k++]=a;triangles[k++]=a+n;triangles[k++]=a+1;triangles[k++]=a+1;triangles[k++]=a+n;triangles[k++]=a+n+1;}
            var mesh=new Mesh();mesh.vertices=verts;mesh.uv=uv;mesh.triangles=triangles;mesh.RecalculateNormals();mesh.RecalculateTangents();mesh.RecalculateBounds();mesh=StoreMesh("Terrain",mesh);
            var go=Node("Terrain",Vector3.zero).gameObject;go.AddComponent<MeshFilter>().sharedMesh=mesh;go.AddComponent<MeshRenderer>().sharedMaterial=M.Ground;
            // Original level yard collider remains below the dressed surface. Border terrain is inaccessible.
            Ribbon("Service road",-50,-7,4.4f,M.Gravel);
            Ribbon("Wheel rut west",-50,-5,.23f,M.Concrete,-.85f);
            Ribbon("Wheel rut east",-50,-5,.23f,M.Concrete,.85f);
            PathRibbon("Footpath to cabin",new[]{new Vector2(0,-6),new Vector2(1,1),new Vector2(1.3f,7),new Vector2(0,9.8f)},.85f);
            PathRibbon("Transit access",new[]{new Vector2(0,-3),new Vector2(-4,-2),new Vector2(-5,0),new Vector2(-6,4.8f)},.58f);
            PathRibbon("Service access",new[]{new Vector2(1,1),new Vector2(5,2.4f),new Vector2(7.5f,6),new Vector2(8,11)},.70f);
            Box("Work apron",new Vector3(0,-.009f,11.8f),new Vector3(8.2f,.028f,6.8f),M.Gravel);
            // Low angular mesa silhouettes, several depth layers, independently varied shoulders.
            for(int i=0;i<8;i++)
            {
                float x=-78+i*22,z=Random(62,80);
                Mesa("Northern escarpment "+i,new Vector3(x,Height(x,z)-2,z),Random(17,28),Random(10,18),Random(8,14));
            }
            Mesa("West ridge",new Vector3(-62,0,6),23,38,11);
            Mesa("Southwest mesa",new Vector3(-58,0,-58),32,24,13);
            Mesa("East escarpment",new Vector3(66,0,20),20,36,15);
        }
        static void Ribbon(string name,float start,float end,float width,Material mat,float offset=0)
        {
            int n=80;var v=new Vector3[n*2];var uv=new Vector2[n*2];var t=new int[(n-1)*6];
            for(int i=0;i<n;i++){float z=Mathf.Lerp(start,end,i/(float)(n-1));float bend=z< -12?Mathf.Sin((z+12)*.045f)*6:0;
                for(int s=0;s<2;s++){float x=bend+offset+(s-.5f)*width;v[i*2+s]=new Vector3(x,Height(x,z)+.024f+(offset!=0?.004f:0),z);uv[i*2+s]=new Vector2(x,z)*.45f;}
                if(i<n-1){int j=i*6,a=i*2;t[j]=a;t[j+1]=a+2;t[j+2]=a+1;t[j+3]=a+1;t[j+4]=a+2;t[j+5]=a+3;}}
            var mesh=new Mesh();mesh.vertices=v;mesh.uv=uv;mesh.triangles=t;mesh.RecalculateNormals();mesh.RecalculateTangents();mesh=StoreMesh(name.Replace(" ",""),mesh);
            var go=Node(name,Vector3.zero).gameObject;go.AddComponent<MeshFilter>().sharedMesh=mesh;go.AddComponent<MeshRenderer>().sharedMaterial=mat;
        }
        static void PathRibbon(string name,Vector2[] nodes,float width)
        {
            var v=new List<Vector3>();var uv=new List<Vector2>();var t=new List<int>();
            for(int segment=0;segment<nodes.Length-1;segment++)for(int i=0;i<12;i++)
            {
                Vector2 p=Vector2.Lerp(nodes[segment],nodes[segment+1],i/11f),direction=(nodes[segment+1]-nodes[segment]).normalized;
                Vector2 side=new Vector2(direction.y,-direction.x);int start=v.Count;
                foreach(int sign in new[]{-1,1}){Vector2 q=p+side*sign*width*(.5f+Mathf.Sin((segment*12+i)*1.1f)*.05f);v.Add(new Vector3(q.x,.006f,q.y));uv.Add(q*.34f);}
                if(i<11)t.AddRange(new[]{start,start+2,start+1,start+1,start+2,start+3});
            }
            var mesh=new Mesh();mesh.SetVertices(v);mesh.SetUVs(0,uv);mesh.SetTriangles(t,0);mesh.RecalculateNormals();mesh.RecalculateTangents();mesh=StoreMesh(name.Replace(" ",""),mesh);
            var go=Node(name,Vector3.zero).gameObject;go.AddComponent<MeshFilter>().sharedMesh=mesh;go.AddComponent<MeshRenderer>().sharedMaterial=M.Gravel;
        }
        static void Mesa(string name,Vector3 p,float rx,float rz,float height)
        {
            const int sides=18;var v=new List<Vector3>();var uv=new List<Vector2>();var t=new List<int>();
            float[] radii={1.45f,1.10f,.86f,.68f,.63f};float[] ys={0,.3f,.48f,.73f,1};
            var jitter=Enumerable.Range(0,sides).Select(_=>Random(.84f,1.12f)).ToArray();
            for(int band=0;band<radii.Length;band++)for(int i=0;i<sides;i++)
            {float a=i*Mathf.PI*2/sides;v.Add(new Vector3(Mathf.Cos(a)*rx*radii[band]*jitter[i],ys[band]*height+((band==0)?0:Mathf.Sin(i*2.71f)*.35f),Mathf.Sin(a)*rz*radii[band]*jitter[i]));uv.Add(new Vector2(i*rx*.13f,ys[band]*height*.3f));}
            for(int b=0;b<radii.Length-1;b++)for(int i=0;i<sides;i++){int a=b*sides+i,c=b*sides+(i+1)%sides;t.AddRange(new[]{a,a+sides,c,c,a+sides,c+sides});}
            v.Add(new Vector3(0,height,0));uv.Add(Vector2.zero);for(int i=0;i<sides;i++)t.AddRange(new[]{(radii.Length-1)*sides+i,v.Count-1,(radii.Length-1)*sides+(i+1)%sides});
            var mesh=new Mesh();mesh.SetVertices(v);mesh.SetUVs(0,uv);mesh.SetTriangles(t,0);mesh.RecalculateNormals();mesh.RecalculateTangents();transientMeshes.Add(mesh);
            var go=Node(name,p).gameObject;go.AddComponent<MeshFilter>().sharedMesh=mesh;go.AddComponent<MeshRenderer>().sharedMaterial=M.FarRock;merge.Add(go);
        }
        static void Building()
        {
            Hide("TexturedFieldHut");foreach(var n in new[]{"HutBack","HutWest","HutEast","HutFrontWest","HutFrontEast","HutLintel","HutCeiling"})Hide(n);
            Find("HutFloor").GetComponent<Renderer>().sharedMaterial=M.Concrete;
            // Front window openings are explicit voids, with deep sills and metal mullions.
            Box("Back wall",new Vector3(0,1.37f,15.6f),new Vector3(7.45f,2.74f,.25f),M.Plaster);
            Box("West wall",new Vector3(-3.65f,1.37f,13),new Vector3(.25f,2.74f,5.4f),M.Plaster);
            Box("East wall",new Vector3(3.65f,1.37f,13),new Vector3(.25f,2.74f,5.4f),M.Plaster);
            foreach(float x in new[]{-2.2f,2.2f})
            {
                Box("Front wall lower",new Vector3(x,.47f,10.32f),new Vector3(2.9f,.94f,.25f),M.Plaster);
                Box("Front wall upper",new Vector3(x,2.46f,10.32f),new Vector3(2.9f,.56f,.25f),M.Plaster);
                foreach(float dx in new[]{-.99f,.99f})Box("Window pier",new Vector3(x+dx,1.57f,10.32f),new Vector3(.91f,1.24f,.25f),M.Plaster);
                foreach(float dx in new[]{-.53f,0,.53f})Box("Window mullion",new Vector3(x+dx,1.57f,10.14f),new Vector3(.045f,1.25f,.065f),M.Metal);
                foreach(float y in new[]{.97f,1.57f,2.18f})Box("Window rail",new Vector3(x,y,10.14f),new Vector3(1.1f,.05f,.065f),M.Metal);
                Box("Window sill",new Vector3(x,.95f,10.08f),new Vector3(1.3f,.09f,.42f),M.Concrete);
                Box("Old glass",new Vector3(x,1.57f,10.26f),new Vector3(1.07f,1.20f,.015f),M.Window);
            }
            Box("Door lintel",new Vector3(0,2.47f,10.32f),new Vector3(1.5f,.54f,.25f),M.Plaster);
            foreach(float x in new[]{-.77f,.77f})Box("Door jamb",new Vector3(x,1.1f,10.20f),new Vector3(.1f,2.2f,.28f),M.Metal);
            Box("Door header",new Vector3(0,2.2f,10.20f),new Vector3(1.65f,.09f,.28f),M.Metal);
            Box("Open steel door",new Vector3(.94f,1.10f,11.02f),new Vector3(.06f,2.17f,1.45f),M.Metal);
            Box("Door window",new Vector3(.90f,1.55f,10.90f),new Vector3(.02f,.58f,.6f),M.Window);
            Box("Door handle",new Vector3(.88f,.97f,11.52f),new Vector3(.06f,.04f,.16f),M.Steel);
            Box("Ceiling",new Vector3(0,2.77f,13),new Vector3(7.55f,.14f,5.6f),M.Wood);
            Box("Roof sheet",new Vector3(0,2.92f,12.93f),new Vector3(8,.13f,6),M.Roof);
            for(int i=0;i<50;i++)Box("Roof standing seam",new Vector3(-3.95f+i*.16f,3,12.93f),new Vector3(.026f,.055f,6),M.Roof);
            foreach(float z in new[]{9.92f,15.95f})Box("Weathered fascia",new Vector3(0,2.84f,z),new Vector3(8,.24f,.065f),M.Wood);
            Box("Foundation",new Vector3(0,-.01f,13),new Vector3(7.65f,.18f,5.8f),M.Concrete);
            // Broad shallow step stays below CharacterController's existing step offset.
            Box("Threshold step",new Vector3(0,.016f,9.96f),new Vector3(1.9f,.065f,.65f),M.Concrete);
            Box("Doormat",new Vector3(0,.078f,10.86f),new Vector3(1.06f,.012f,.55f),M.Rubber);
            foreach(float x in new[]{-3.3f,3.3f})
            {
                Box("Porch pad",new Vector3(x,.012f,8.75f),new Vector3(.42f,.07f,.42f),M.Concrete);
                Box("Porch upright",new Vector3(x,1.28f,8.75f),new Vector3(.1f,2.5f,.1f),M.Wood,true);
                Beam("Porch knee brace",new Vector3(x,2.05f,8.75f),new Vector3(x*.8f,2.52f,8.75f),.07f,M.Wood);
            }
            Box("Porch canopy",new Vector3(0,2.58f,9.36f),new Vector3(7.1f,.07f,1.5f),M.Roof);
            Box("Porch beam",new Vector3(0,2.48f,8.69f),new Vector3(7.1f,.18f,.13f),M.Wood);
            Plaque("Station name","S A R O  /  S T A T I O N  0 1",new Vector3(0,2.77f,9.87f),new Vector2(2.9f,.26f),0,.039f);
            Plaque("Service notice","FIELD TIMING ANNEX\nAUTHORIZED PERSONNEL",new Vector3(1.18f,1.51f,10.14f),new Vector2(.62f,.28f),0,.020f);
            // Drip course, plinth and exposed conduit make construction legible at the door.
            foreach(float x in new[]{-3.48f,3.48f})Beam("Conduit",new Vector3(x,.15f,10.1f),new Vector3(x,2.46f,10.1f),.025f,M.Steel);
            for(int i=0;i<9;i++)Box("Foundation joint",new Vector3(-3.4f+i*.85f,.11f,10.13f),new Vector3(.016f,.19f,.018f),M.Rubber);
            Interior();
        }
        static void Interior()
        {
            var desk=Find("A17_Desk");foreach(var r in desk.GetComponentsInChildren<Renderer>())r.sharedMaterials=r.sharedMaterials.Select(_=>M.Wood).ToArray();
            Hide("Receiver");Hide("ReceiverVent");Model("E27_AnalogReceiver",new Vector3(.5f,.86f,13.72f),0);
            Find("TimingRecord").GetComponent<Renderer>().sharedMaterial=M.Paper;
            // Visible clipboard edges, binding and a few lines on the existing actionable record.
            Box("Record clip",new Vector3(-.35f,.868f,13.77f),new Vector3(.16f,.013f,.027f),M.Steel);
            for(int i=0;i<8;i++)Box("Record ruled line",new Vector3(-.35f,.861f,13.58f+i*.021f),new Vector3(.30f,.0015f,.0015f),M.Wood);
            Plaque("Timing heading","FIELD TIMING  /  T. VEGA",new Vector3(0,1.86f,15.43f),new Vector2(1.7f,.2f),0,.028f);
            Model("A17_Cabinet",new Vector3(-2.9f,.09f,14.75f),90,1,"Assets/Signal47/Art/Archive17/");
            Model("W18_Chair",new Vector3(-1.25f,.09f,12.15f),120,1,"Assets/Signal47/Art/Workstation18/");
            Box("Records cabinet contact",new Vector3(-2.94f,1.07f,14.75f),new Vector3(.67f,1.96f,1.28f),M.Metal,true).GetComponent<Renderer>().enabled=false;
            Box("Chair contact",new Vector3(-1.25f,.58f,12.15f),new Vector3(.60f,.95f,.60f),M.Metal,true).GetComponent<Renderer>().enabled=false;
            for(int level=0;level<3;level++)
            {
                float y=.55f+level*.48f;Box("Stores shelf",new Vector3(3.18f,y,13.7f),new Vector3(.48f,.07f,2.4f),M.Wood);
                for(int i=0;i<4;i++){float z=12.85f+i*.52f;Box("Instrument case",new Vector3(3.12f,y+.15f,z),new Vector3(.38f,.24f,.43f),M.Metal);Box("Case latch",new Vector3(2.922f,y+.16f,z),new Vector3(.018f,.06f,.065f),M.Steel);}
            }
            foreach(float z in new[]{12.5f,14.85f})Box("Shelf upright",new Vector3(3.42f,1.16f,z),new Vector3(.055f,2.18f,.055f),M.Steel);
            Box("Notice board",new Vector3(-1.7f,1.6f,15.43f),new Vector3(1.22f,.95f,.04f),M.Wood);
            Box("Service plan paper",new Vector3(-1.7f,1.58f,15.398f),new Vector3(1.04f,.77f,.002f),M.Paper);
            Vector3 Plan(float x,float z)=>new Vector3(-1.7f+x*.029f,1.4f+z*.023f,15.393f);
            void PlanLine(float x,float z,float x2,float z2)=>Beam("Service plan ink",Plan(x,z),Plan(x2,z2),.0022f,M.Wood);
            PlanLine(-14,-7,14,-7);PlanLine(-14,-7,-14,20);PlanLine(14,-7,14,20);PlanLine(-14,20,14,20);
            PlanLine(-4,10,4,10);PlanLine(-4,10,-4,16);PlanLine(4,10,4,16);PlanLine(-4,16,4,16);
            PlanLine(-7,0,-7,7);PlanLine(-9,6,-4,6);PlanLine(0,6,6,6);PlanLine(6,6,6,3);PlanLine(6,3,3,3);
            Plaque("Plan title","STATION 01 / SITE SERVICES",new Vector3(-1.7f,1.96f,15.39f),new Vector2(1.02f,.1f),0,.018f);
            Model("A17_Lamp",new Vector3(-.68f,.87f,13.7f),180,1,"Assets/Signal47/Art/Archive17/");
            // Lamp source is physically tied to a visible desk fitting.
            var l=Find("HutLamp").GetComponent<Light>();l.transform.localPosition=new Vector3(-.62f,1.43f,13.45f);l.range=6;l.intensity=1.1f;l.color=new Color(1,.73f,.44f);
            LightAt("Interior ceiling bounce",new Vector3(0,2.35f,13.4f),new Color(1,.70f,.43f),1.4f,6,false);
            LightAt("Porch bulb",new Vector3(0,2.25f,9.80f),new Color(1,.66f,.30f),1.1f,7,true);
            Box("Porch bulb fitting",new Vector3(0,2.32f,9.86f),new Vector3(.22f,.10f,.20f),M.Metal);
            Box("Porch bulb diffuser",new Vector3(0,2.24f,9.85f),new Vector3(.13f,.055f,.14f),M.Emissive);
        }
        static Light LightAt(string name,Vector3 pos,Color color,float intensity,float range,bool shadow)
        {
            var go=Node(name,pos);var l=go.gameObject.AddComponent<Light>();l.type=LightType.Point;l.color=color;l.intensity=intensity;l.range=range;l.shadows=shadow?LightShadows.Soft:LightShadows.None;l.shadowBias=.035f;l.shadowNormalBias=.18f;return l;
        }
        static void Yard()
        {
            Hide("Transit");Hide("TransitLeg");Model("E27_SurveyTransit",new Vector3(-5,0,0));
            Hide("LampPost");Hide("LampControl");var testFixture=Model("E27_LampPole",new Vector3(0,0,6),0);
            foreach(var rr in testFixture.GetComponentsInChildren<Renderer>())if(rr.sharedMaterials.Contains(M.Emissive))
            {
                merge.Remove(rr.gameObject);
                rr.gameObject.AddComponent<Signal47.Audio.PracticalLampEmission>().source=Find("NullTestLamp").GetComponent<Light>();
            }
            Hide("CableInspection");Find("CableInspectionSlab").GetComponent<Renderer>().sharedMaterial=M.Concrete;
            foreach(var r in field.GetComponentsInChildren<Renderer>(true))
            {
                if(r.name.Contains("CableFeed")||r.name.Contains("CableReturn")||r.name.Contains("CutFace"))
                {
                    bool cut=r.name.Contains("CutFace");var tr=r.transform;
                    Vector3 half=tr.localRotation*Vector3.up*tr.localScale.y,a=tr.localPosition-half,b=tr.localPosition+half;
                    a.y=a.y<.1f?.018f:.533f;b.y=b.y<.1f?.018f:.533f;
                    tr.localPosition=(a+b)*.5f;tr.localRotation=Quaternion.FromToRotation(Vector3.up,b-a);
                    tr.localScale=new Vector3(cut?.027f:.032f,Vector3.Distance(a,b)*.5f,cut?.027f:.032f);
                    if(!cut)r.sharedMaterial=M.Rubber;
                }
            }
            // A bolted test bed reads as maintenance equipment; the opposed cable faces stay unobstructed.
            Box("Cable test panel",new Vector3(5,.506f,4),new Vector3(1.32f,.01f,.96f),M.Metal);
            foreach(float x in new[]{4.45f,5.55f})foreach(float z in new[]{3.61f,4.36f})Box("Test bed bolt",new Vector3(x,.528f,z),new Vector3(.035f,.027f,.035f),M.Steel);
            Plaque("Cable safety tag","TEST LOOP  /  KEEP OPEN",new Vector3(5,.40f,3.463f),new Vector2(.87f,.12f),0,.024f);
            Plaque("Lamp plate","LOCAL LAMP",new Vector3(-.16f,.50f,5.911f),new Vector2(.22f,.06f),0,.012f);
            Find("LampControl").localPosition=new Vector3(-.16f,.60f,5.96f);Find("LampControl").localScale=new Vector3(.24f,.34f,.22f);
            Find("NullTestLamp").localPosition=new Vector3(0,1.60f,5.82f);Find("NullTestLamp").GetComponent<Light>().intensity=1.0f;
            foreach(var n in new[]{"MovedMark","FixedMarkB"})Find(n).GetComponent<Renderer>().sharedMaterial=M.Metal;
            Find("HistoricFooting").GetComponent<Renderer>().sharedMaterial=M.Concrete;
            Plaque("Original survey point","A / 1947",new Vector3(-5,.105f,5.687f),new Vector2(.32f,.08f),0,.021f);
            Plaque("Fixed B","B",new Vector3(-7.5f,1.21f,5.893f),new Vector2(.24f,.28f),0,.065f);
            for(int i=0;i<4;i++)Box("Exposed foundation anchor",new Vector3(-5+(i%2-.5f)*.38f,.17f,6+(i/2-.5f)*.34f),new Vector3(.025f,.10f,.025f),M.Steel);
            Model("E27_DieselGenerator",new Vector3(9,.048f,12.2f),0);Box("Generator plinth",new Vector3(9,-.002f,12.2f),new Vector3(2,.1f,1.5f),M.Concrete);
            Box("Generator collision",new Vector3(9,.503f,12.2f),new Vector3(.88f,.91f,.62f),M.Metal,true).GetComponent<Renderer>().enabled=false;
            Model("E27_DistributionCabinet",new Vector3(4.7f,0,10.8f),0);
            Box("Power cabinet collision",new Vector3(4.7f,.76f,10.8f),new Vector3(.65f,1.52f,.42f),M.Metal,true).GetComponent<Renderer>().enabled=false;
            Beam("Supply riser",new Vector3(4.7f,.1f,10.65f),new Vector3(4.7f,2.7f,10.65f),.035f,M.Steel);
            Beam("Cabin service conduit",new Vector3(4.7f,2.7f,10.65f),new Vector3(3.5f,2.7f,10.65f),.035f,M.Steel);
            // The service gate closes behind the implicit journey; sign out at the nearby return folio.
            for(int side=-1;side<=1;side+=2)
            {
                for(int i=0;i<=9;i++)
                {
                    float z=-9+i*3.35f;Box("Perimeter fence post",new Vector3(side*14.8f,.65f,z),new Vector3(.055f,1.3f,.055f),M.Steel);
                    if(i<9)foreach(float y in new[]{.35f,.75f,1.14f})Beam("Fence wire",new Vector3(side*14.8f,y,z),new Vector3(side*14.8f,y,z+3.35f),.008f,M.Steel);
                }
                for(int i=0;i<4;i++){float x=side*(3.2f+i*2.9f);Box("Entry fence post",new Vector3(x,.65f,-9),new Vector3(.06f,1.3f,.06f),M.Steel);foreach(float y in new[]{.4f,.8f,1.15f})Beam("Entry fence wire",new Vector3(x,y,-9),new Vector3(x+side*2.9f,y,-9),.008f,M.Steel);}
            }
            foreach(float x in new[]{-1.5f,1.5f})Model("E27_ServiceGate",new Vector3(x,0,-9),0).transform.parent.localScale=new Vector3(1.9f,1,1);
            for(int i=0;i<10;i++){float x=-14.8f+i*2.96f;Box("Rear fence post",new Vector3(x,.65f,21.15f),new Vector3(.06f,1.3f,.06f),M.Steel);foreach(float y in new[]{.4f,.8f,1.15f})Beam("Rear fence wire",new Vector3(x,y,21.15f),new Vector3(x+2.96f,y,21.15f),.008f,M.Steel);}
            Find("BoundaryWest").localPosition=new Vector3(-14.8f,.65f,6);Find("BoundaryWest").localScale=new Vector3(.12f,1.3f,30.3f);
            Find("BoundaryEast").localPosition=new Vector3(14.8f,.65f,6);Find("BoundaryEast").localScale=new Vector3(.12f,1.3f,30.3f);
            Find("BoundaryNorth").localPosition=new Vector3(0,.65f,21.15f);Find("BoundaryNorth").localScale=new Vector3(29.6f,1.3f,.12f);
            Find("BoundarySouth").localPosition=new Vector3(0,.65f,-9);Find("BoundarySouth").localScale=new Vector3(29.6f,1.3f,.12f);
            Plaque("Entry sign","SARO  /  FIELD STATION 01\nNO THROUGH ROAD",new Vector3(4.4f,1.31f,-9.02f),new Vector2(1.12f,.38f),0,.03f);
            // Departure paperwork lives on a real small writing shelf, not a floating plinth.
            Hide("ReturnStand");Find("ReturnFolio").GetComponent<Renderer>().sharedMaterial=M.Paper;
            Box("Dispatch writing shelf",new Vector3(0,.85f,-6.9f),new Vector3(.9f,.065f,.56f),M.Wood);
            foreach(float x in new[]{-.34f,.34f})Box("Dispatch shelf leg",new Vector3(x,.43f,-6.9f),new Vector3(.055f,.86f,.45f),M.Metal);
            Plaque("Return folio tab","SARO  /  RETURN",new Vector3(0,.81f,-6.605f),new Vector2(.68f,.12f),180,.025f);
            // Existing local sources now sit in authored fittings and have restrained shadow cost.
            var work=Find("FieldWorkLight").GetComponent<Light>();work.intensity=1.1f;work.range=13;work.shadows=LightShadows.None;
            var cable=Find("CableWorkLight").GetComponent<Light>();cable.intensity=2.1f;cable.range=7;cable.shadows=LightShadows.None;
            Model("E27_LampPole",new Vector3(6,0,3),-35);cable.transform.localPosition=new Vector3(6.10f,1.6f,2.85f);
            var moon=Find("MoonFill").GetComponent<Light>();moon.color=new Color(.65f,.73f,.91f);moon.intensity=.85f;moon.shadowNormalBias=.20f;
        }
        static void Scatter()
        {
            for(int i=0;i<100;i++)
            {
                float a=Random(0,Mathf.PI*2),d=Random(16,40);float x=Mathf.Cos(a)*d,z=5+Mathf.Sin(a)*d;
                if(Mathf.Abs(x)<5&&z<0)continue;
                Model("E27_Boulder_LOD1",new Vector3(x,Height(x,z)-.06f,z),Random(0,360),Random(.6f,2.2f));
            }
            // Several deliberate near clusters frame the walk, with clear evidence sightlines.
            foreach(var p in new[]{new Vector3(-11,0,2),new Vector3(-10.5f,0,10),new Vector3(11,0,6),new Vector3(8,0,-4),new Vector3(-8,0,-4),new Vector3(-12,0,17),new Vector3(11,0,19)})
                for(int j=0;j<5;j++){float x=p.x+Random(-1.4f,1.4f),z=p.z+Random(-1.4f,1.4f);float s=Random(.4f,1.2f);Model("E27_Boulder_LOD0",new Vector3(x,Height(x,z)-.06f,z),Random(0,360),s);}
            for(int i=0;i<640;i++)
            {
                float x=Random(-35,35),z=Random(-25,40);
                if(Mathf.Abs(x)<9&&z>-10&&z<17)continue;
                if(Mathf.Abs(x-Mathf.Sin((z+12)*.045f)*6)<3&&z< -10)continue;
                Grass(new Vector3(x,Height(x,z)-.01f,z),i);
            }
            for(int i=0;i<140;i++)
            {
                float x=Random(-12,12),z=Random(-7,19);
                if(Mathf.Abs(x)<2.5f||Mathf.Abs(x)<4&&z>9||x>3&&x<7&&z>1&&z<10||x>-8.2f&&x< -3&&z>-.8f&&z<8)continue;
                Grass(new Vector3(x,Height(x,z)-.01f,z),i);
            }
            // Small stones on the compacted ground give scale without impeding the controller.
            for(int i=0;i<180;i++){float x=Random(-13,13),z=Random(-9,19);if(Mathf.Abs(x)<2||Mathf.Abs(x)<4&&z>9)continue;Model("E27_Boulder_LOD1",new Vector3(x,-.025f,z),Random(0,360),Random(.025f,.10f));}
        }
        static void Grass(Vector3 p,int index)
        {
            int kind=index%3;var prefab=AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Signal47/Art/Visual10/Models/V10_DryTuft_"+(char)('A'+kind)+".fbx");
            var placement=Node("Dry grass",p);placement.localRotation=Quaternion.Euler(0,Random(0,360),0);placement.localScale=Vector3.one*Random(.9f,1.9f);
            var go=(GameObject)PrefabUtility.InstantiatePrefab(prefab);go.transform.SetParent(placement,false);
            if(!grassMeshes.ContainsKey(kind))
            {
                var combine=go.GetComponentsInChildren<MeshFilter>().Select(f=>new CombineInstance{mesh=f.sharedMesh,transform=placement.worldToLocalMatrix*f.transform.localToWorldMatrix}).ToArray();
                var mesh=new Mesh();mesh.CombineMeshes(combine,true,true);grassMeshes[kind]=StoreMesh("GrassPrototype"+kind,mesh);grassMatrices[kind]=new List<Matrix4x4>();
            }
            grassMatrices[kind].Add(placement.localToWorldMatrix);Object.DestroyImmediate(placement.gameObject);
        }
        static void BatchGeometry()
        {
            // Local batches let Unity cull distant rocks and select nearby lights.
            // One material-wide mesh made the entire site receive every local light.
            var groups=new Dictionary<(Material material,int x,int z),List<CombineInstance>>();
            foreach(var go in merge.Distinct())
            {
                var renderer=go.GetComponent<MeshRenderer>();var filter=go.GetComponent<MeshFilter>();if(!renderer||!renderer.enabled||!filter)continue;
                for(int s=0;s<filter.sharedMesh.subMeshCount;s++)
                {
                    var mat=renderer.sharedMaterials[Mathf.Min(s,renderer.sharedMaterials.Length-1)];
                    Vector3 center=root.InverseTransformPoint(renderer.bounds.center);
                    var key=(mat,Mathf.FloorToInt(center.x/10),Mathf.FloorToInt(center.z/10));
                    if(!groups.ContainsKey(key))groups[key]=new List<CombineInstance>();
                    groups[key].Add(new CombineInstance{mesh=filter.sharedMesh,subMeshIndex=s,transform=root.worldToLocalMatrix*go.transform.localToWorldMatrix});
                }
                renderer.enabled=false;
            }
            foreach(var group in groups)
            {
                string name=group.Key.material.name+"_"+group.Key.x+"_"+group.Key.z;
                var mesh=new Mesh{indexFormat=IndexFormat.UInt32};mesh.CombineMeshes(group.Value.ToArray(),true,true);mesh.RecalculateBounds();mesh=StoreMesh("Batch_"+name,mesh);
                var go=Node("Geometry "+name,Vector3.zero).gameObject;go.AddComponent<MeshFilter>().sharedMesh=mesh;var r=go.AddComponent<MeshRenderer>();r.sharedMaterial=group.Key.material;
                if(group.Key.material==M.Grass||group.Key.material==M.Window)r.shadowCastingMode=ShadowCastingMode.Off;
            }
            var retained=new HashSet<string>(groups.Keys.Select(key=>Art+"Batch_"+key.material.name+"_"+key.x+"_"+key.z+".asset"));
            foreach(var path in Directory.GetFiles(Art,"Batch_*.asset"))if(!retained.Contains(path.Replace('\\','/')))AssetDatabase.DeleteAsset(path);
            // Remove only visual mesh components. Named placements/colliders remain reviewable.
            foreach(var go in merge.Distinct()){var r=go.GetComponent<MeshRenderer>();if(r&&!r.enabled){Object.DestroyImmediate(r);Object.DestroyImmediate(go.GetComponent<MeshFilter>());}}
            foreach(var m in transientMeshes)if(m&&!AssetDatabase.Contains(m))Object.DestroyImmediate(m);
        }
        public static void Apply(Transform area,Font terminalFont,Material worldText)
        {
            field=area;font=terminalFont;textMaterial=worldText;rng=new System.Random(2747);merge.Clear();transientMeshes.Clear();importAudit.Clear();grassMeshes.Clear();grassMatrices.Clear();
            M.Build();root=new GameObject("Environment27").transform;root.SetParent(field,false);
            field.gameObject.AddComponent<Signal47.Environment.FieldAtmosphere27>();
            foreach(var n in new[]{"Ground","Approach","RockRim","DryTuft"})Hide(n);
            // Physical labels replace all Station26 floating instructional text in this area.
            foreach(var tm in field.GetComponentsInChildren<TextMesh>(true))tm.GetComponent<Renderer>().enabled=false;
            Terrain();Building();Yard();Scatter();BatchGeometry();
            var vegetation=root.gameObject.AddComponent<Signal47.Environment.FieldVegetation27>();vegetation.material=M.Grass;
            // Keep all authored grass, partitioning only its draw bounds.
            vegetation.batches=grassMeshes.SelectMany(pair=>grassMatrices[pair.Key]
                .GroupBy(matrix=>(Mathf.FloorToInt(matrix.m03/10),Mathf.FloorToInt(matrix.m23/10)))
                .Select(group=>new Signal47.Environment.FieldVegetation27.Batch{mesh=pair.Value,matrices=group.ToArray()})).ToArray();
            AssetDatabase.DeleteAsset(Art+"Batch_E27_Grass.asset");
            var sound=root.gameObject.AddComponent<Signal47.Environment.FieldSound27>();
            AudioSource Loop(string name,Vector3 pos,string path,float min,float max)
            {
                var audio=Node(name,pos).gameObject.AddComponent<AudioSource>();audio.clip=AssetDatabase.LoadAssetAtPath<AudioClip>(path);
                if(!audio.clip)throw new FileNotFoundException(path);
                audio.loop=true;audio.playOnAwake=false;audio.volume=0;audio.spatialBlend=1;audio.rolloffMode=AudioRolloffMode.Logarithmic;audio.minDistance=min;audio.maxDistance=max;audio.dopplerLevel=0;return audio;
            }
            sound.wind=Loop("Desert wind",new Vector3(-7,1,5),"Assets/Signal47/Art/ThirdParty/PreparedAudio/DesertWind.wav",12,45);
            sound.generator=Loop("Generator sound",new Vector3(9,.5f,12.2f),Art+"Audio/GeneratorLoop.wav",1.6f,13);
            Directory.CreateDirectory("../Artifacts/Environment27");
            File.WriteAllLines("../Artifacts/Environment27/import-bounds.txt",importAudit.Select(p=>p.Key+" "+p.Value));
            var renderers=field.GetComponentsInChildren<MeshRenderer>(true).Where(r=>r.enabled).ToArray();
            File.WriteAllText("../Artifacts/Environment27/scene-audit.json",JsonUtility.ToJson(new Audit{renderers=renderers.Length,triangles=renderers.Sum(r=>r.GetComponent<MeshFilter>()?r.GetComponent<MeshFilter>().sharedMesh.triangles.Length/3:0),colliders=field.GetComponentsInChildren<Collider>(true).Length,lightmaps=LightmapSettings.lightmaps.Length},true));
        }
        [Serializable]sealed class Audit{public int renderers,triangles,colliders,lightmaps;}
    }
}
#endif
