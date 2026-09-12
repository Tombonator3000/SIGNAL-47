#if UNITY_EDITOR
using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEditor.Build.Reporting;
using UnityEngine;
using UnityEngine.Rendering;
using Signal47.Archive;

namespace Signal47.Editor
{
    public static class Archive16Build
    {
        [Serializable] sealed class BoundsRecord { public Vector3 center,size; public bool scalePass; }
        [Serializable] sealed class AssetSource { public string name,fbx; public float[] dimensions_blender_xyz; public int triangles,triangle_budget,render_meshes; }
        [Serializable] sealed class AssetSources { public AssetSource[] assets; }
        [Serializable] sealed class ImportedAsset { public string name; public Vector3 size; public int triangles,renderMeshes; public bool boundsPass,normalsAndUvPass; }
        [Serializable] sealed class ImportReport { public List<ImportedAsset> assets=new(); }
        static ImportReport importReport;
        static AssetSources sources;
        const string Furniture="Assets/Signal47/Art/Archive17/";
        const string Art="Assets/Signal47/Art/Archive16/";
        static Material labelMaterial;
        static Material M(string name)=>AssetDatabase.LoadAssetAtPath<Material>("Assets/Signal47/Art/Chapter09/CH09_"+name+".mat");
        static GameObject Box(string name,Vector3 at,Vector3 size,Material mat)
        {var g=GameObject.CreatePrimitive(PrimitiveType.Cube);g.name=name;g.transform.position=at;g.transform.localScale=size;g.GetComponent<Renderer>().sharedMaterial=mat;return g;}
        static Transform Pose(string name,Vector3 at,Vector3 target)
        {var t=new GameObject(name).transform;t.position=at;t.rotation=Quaternion.LookRotation(target-at);return t;}
        static GameObject Model(string path,string name,Vector3 bottom,float width,float yaw=0)
        {
            var source=AssetDatabase.LoadAssetAtPath<GameObject>(path);if(!source)throw new Exception("Missing "+path);
            var parent=new GameObject(name);var g=(GameObject)PrefabUtility.InstantiatePrefab(source);g.transform.SetParent(parent.transform,false);
            parent.transform.rotation=Quaternion.Euler(0,yaw,0);var rs=g.GetComponentsInChildren<Renderer>();
            var bounds=rs[0].bounds;foreach(var r in rs)bounds.Encapsulate(r.bounds);
            parent.transform.localScale=Vector3.one*(width/bounds.size.x);bounds=rs[0].bounds;foreach(var r in rs)bounds.Encapsulate(r.bounds);
            parent.transform.position=bottom-new Vector3(bounds.center.x,bounds.min.y,bounds.center.z);
            foreach(var r in rs)
            {
                var mats=r.sharedMaterials;
                for(int i=0;i<mats.Length;i++)
                {
                    var key=mats[i]?mats[i].name:"";
                    if(key.StartsWith("A17_",StringComparison.Ordinal))
                    {
                        string materialName=key.Substring(4);
                        mats[i]=M(materialName=="Card"?"Cork":materialName=="Tab"?"Red":materialName);
                        if(!mats[i])throw new Exception("Missing archive material: "+key);
                        continue;
                    }
                    mats[i]=key.Contains("Paper")||key.Contains("Label")?M("Paper"):key.Contains("Clip")?M("Steel"):key.Contains("Tab")?M("Red"):key.Contains("Card")?M("Cork"):M("Green");
                }
                r.sharedMaterials=mats;
            }
            if(path.StartsWith(Furniture,StringComparison.Ordinal))
            {
                bounds=rs[0].bounds;foreach(var r in rs)bounds.Encapsulate(r.bounds);
                var expected=sources.assets.First(a=>a.fbx==Path.GetFileName(path));
                var d=expected.dimensions_blender_xyz;
                var target=new Vector3(d[0],d[2],d[1])*(width/d[0]);
                if((bounds.size-target).magnitude>.002f)throw new Exception("Furniture scale/axis mismatch: "+name+" "+bounds.size+" expected "+target);
                int triangles=0;
                foreach(var mf in g.GetComponentsInChildren<MeshFilter>())
                {
                    var mesh=mf.sharedMesh;triangles+=mesh.triangles.Length/3;
                    if(mesh.normals.Length!=mesh.vertexCount||mesh.uv.Length!=mesh.vertexCount||
                       mesh.normals.Any(n=>float.IsNaN(n.sqrMagnitude)||Mathf.Abs(n.sqrMagnitude-1)>.03f)||
                       mesh.uv.Any(uv=>!float.IsFinite(uv.x)||!float.IsFinite(uv.y)))
                        throw new Exception("Invalid imported normals/UVs: "+mf.name);
                }
                if(triangles!=expected.triangles||triangles>expected.triangle_budget||rs.Length!=expected.render_meshes)
                    throw new Exception("Furniture triangle/mesh budget mismatch: "+name+" "+triangles);
                importReport.assets.Add(new ImportedAsset{name=name,size=bounds.size,triangles=triangles,renderMeshes=rs.Length,boundsPass=true,normalsAndUvPass=true});
            }
            return parent;
        }
        static void Label(string name,string words,Vector3 at,float size,Quaternion rotation,Font font,float maxWidth=0)
        {
            var g=new GameObject(name);g.transform.SetPositionAndRotation(at,rotation);var t=g.AddComponent<TextMesh>();t.text=words;t.font=font;t.fontSize=64;t.characterSize=size;t.anchor=TextAnchor.MiddleCenter;t.alignment=TextAlignment.Center;t.color=new Color(.12f,.16f,.10f);g.GetComponent<MeshRenderer>().sharedMaterial=labelMaterial;
            if(maxWidth>0){float width=g.GetComponent<MeshRenderer>().bounds.size.x;if(width>maxWidth)g.transform.localScale*=maxWidth/width;}
        }
        public static void Build()
        {
            AssetDatabase.Refresh();importReport=new ImportReport();
            sources=JsonUtility.FromJson<AssetSources>(File.ReadAllText(Furniture+"source-manifest.json"));
            var scene=EditorSceneManager.NewScene(NewSceneSetup.EmptyScene,NewSceneMode.Single);
            var font=AssetDatabase.LoadAssetAtPath<Font>("Assets/Signal47/Art/ThirdParty/VT323/VT323-Regular.ttf");
            labelMaterial=AssetDatabase.LoadAssetAtPath<Material>(Art+"ArchiveWorldText.mat");
            if(!labelMaterial){labelMaterial=new Material(Shader.Find("Signal47/Archive World Text"));AssetDatabase.CreateAsset(labelMaterial,Art+"ArchiveWorldText.mat");}
            labelMaterial.mainTexture=font.material.mainTexture;
            RenderSettings.ambientMode=AmbientMode.Flat;RenderSettings.ambientLight=new Color(.33f,.37f,.32f);RenderSettings.fog=false;RenderSettings.skybox=null;
            Box("Records floor",new Vector3(0,-.1f,0),new Vector3(6,.2f,6),M("Linoleum"));
            Box("Back plaster",new Vector3(0,1.6f,2.4f),new Vector3(6,3.2f,.2f),M("UpperWall"));
            Box("Back green dado",new Vector3(0,.6f,2.28f),new Vector3(6,1.2f,.06f),M("LowerWall"));
            Box("Left wall",new Vector3(-2.8f,1.5f,0),new Vector3(.2f,3,5),M("UpperWall"));
            Model(Furniture+"A17_Desk.fbx","Work table",new Vector3(0,.008f,0),2.7f);
            Model("Assets/Signal47/Art/Chapter09/CH09_LabStool.fbx","Lab stool",new Vector3(1.45f,0,-.2f),.5f);
            for(int i=0;i<3;i++)
            {
                float x=-1.95f+i*1.6f;
                var cabinet=Model(Furniture+"A17_Cabinet.fbx","Archive cabinet",new Vector3(x,0,1.85f),1.3f);
                // Place text from imported paper pockets, avoiding assumptions about FBX axes.
                var paper=cabinet.GetComponentsInChildren<MeshFilter>().First(f=>f.name=="Cabinet_Paper");
                var mesh=paper.sharedMesh;
                var points=mesh.vertices.Select(v=>paper.transform.TransformPoint(v)).ToArray();
                float front=points.Min(v=>v.z);
                for(int j=0;j<4;j++)
                    Label("Drawer index",$"{1947+i*10} / {j+1:00}",new Vector3(x,.336f+j*.46f,front-.001f),.012f,Quaternion.identity,font);
            }
            var folder=Model(Furniture+"A17_Folio.fbx","Protocol folio",new Vector3(0,.806f,-.16f),.46f);
            var rs=folder.GetComponentsInChildren<Renderer>();var b=rs[0].bounds;foreach(var r in rs)b.Encapsulate(r.bounds);
            if(b.size.y>.06f||b.size.z<.3f||b.size.x>.48f)throw new Exception("Folio Unity bounds invalid: "+b);
            var hit=new GameObject("Folio interaction").AddComponent<BoxCollider>();hit.transform.position=b.center;hit.size=b.size+new Vector3(.04f,.04f,.04f);
            var labelBounds=rs.First(r=>r.name.Contains("Label field")).bounds;
            Label("Folio caption","STATION 01\nFIELD RECORD / 1947",new Vector3(labelBounds.center.x,labelBounds.max.y+.001f,labelBounds.center.z),.014f,Quaternion.Euler(90,0,0),font,.245f);
            Model(Furniture+"A17_PaperStack.fbx","Document stack",new Vector3(.69f,.806f,.12f),.3475463f);
            Model(Furniture+"A17_Lamp.fbx","Task lamp",new Vector3(-.83f,.806f,.20f),.4623919f);
            var lamp=new GameObject("Warm task light").AddComponent<Light>();lamp.type=LightType.Point;lamp.transform.position=new Vector3(-.80f,1.235f,.095f);lamp.color=new Color(1,.79f,.5f);lamp.intensity=.35f;lamp.range=3;lamp.shadows=LightShadows.Soft;
            var main=new GameObject("Ceiling wash").AddComponent<Light>();main.type=LightType.Spot;main.spotAngle=125;main.transform.rotation=Quaternion.Euler(90,0,0);main.transform.position=new Vector3(.3f,2.7f,-.5f);main.color=new Color(.8f,.91f,.86f);main.intensity=1.2f;main.range=7;main.shadows=LightShadows.Soft;
            var cam=new GameObject("Study camera").AddComponent<Camera>();cam.gameObject.AddComponent<AudioListener>();cam.clearFlags=CameraClearFlags.SolidColor;cam.backgroundColor=new Color(.025f,.035f,.03f);cam.fieldOfView=52;cam.nearClipPlane=.05f;
            var controller=new GameObject("Archive Study").AddComponent<ArchiveStudy>();controller.labelMaterial=labelMaterial;controller.view=cam;controller.folio=hit;controller.font=font;
            controller.deskView=Pose("Desk view",new Vector3(0,1.65f,-1.45f),b.center);
            controller.roomView=Pose("Room view",new Vector3(2.15f,1.8f,-2.55f),new Vector3(-.2f,1,1));
            controller.original=AssetDatabase.LoadAssetAtPath<TextAsset>(Art+"original.txt");controller.amended=AssetDatabase.LoadAssetAtPath<TextAsset>(Art+"amended.txt");
            if(!controller.original||!controller.amended)throw new Exception("Missing source text");
            cam.transform.SetPositionAndRotation(controller.deskView.position,controller.deskView.rotation);
            string scenePath="Assets/Signal47/Scenes/Prototype/Archive16_Study.unity";EditorSceneManager.SaveScene(scene,scenePath);AssetDatabase.SaveAssets();
            Directory.CreateDirectory("../Artifacts/Archive16");File.WriteAllText("../Artifacts/Archive16/unity-folio-bounds.json",JsonUtility.ToJson(new BoundsRecord{center=b.center,size=b.size,scalePass=true},true));
            Directory.CreateDirectory("../Artifacts/Archive17");
            File.WriteAllText("../Artifacts/Archive17/unity-import.json",JsonUtility.ToJson(importReport,true));
            string output=Path.GetFullPath("../Artifacts/GauntletLinux/Signal47.x86_64");
            var report=BuildPipeline.BuildPlayer(new BuildPlayerOptions{scenes=new[]{scenePath},locationPathName=output,target=BuildTarget.StandaloneLinux64,options=BuildOptions.None});
            if(report.summary.result!=BuildResult.Succeeded)throw new Exception("Archive build failed: "+report.summary.result);
            File.Copy("../Docs/THIRD_PARTY_NOTICES.md",Path.Combine(Path.GetDirectoryName(output),"THIRD_PARTY_NOTICES.md"),true);File.Copy("Assets/Signal47/Art/ThirdParty/VT323/OFL.txt",Path.Combine(Path.GetDirectoryName(output),"VT323-OFL.txt"),true);
            Debug.Log("ARCHIVE16_BUILD_OK");
        }
    }
}
#endif
