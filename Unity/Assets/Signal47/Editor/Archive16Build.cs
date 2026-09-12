#if UNITY_EDITOR
using System;
using System.IO;
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
                    mats[i]=key.Contains("Paper")||key.Contains("Label")?M("Paper"):key.Contains("Clip")?M("Steel"):key.Contains("Tab")?M("Red"):key.Contains("Card")?M("Cork"):M("Green");
                }
                r.sharedMaterials=mats;
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
            AssetDatabase.Refresh();var scene=EditorSceneManager.NewScene(NewSceneSetup.EmptyScene,NewSceneMode.Single);
            var font=AssetDatabase.LoadAssetAtPath<Font>("Assets/Signal47/Art/ThirdParty/VT323/VT323-Regular.ttf");
            labelMaterial=AssetDatabase.LoadAssetAtPath<Material>(Art+"ArchiveWorldText.mat");
            if(!labelMaterial){labelMaterial=new Material(Shader.Find("Signal47/Archive World Text"));AssetDatabase.CreateAsset(labelMaterial,Art+"ArchiveWorldText.mat");}
            labelMaterial.mainTexture=font.material.mainTexture;
            RenderSettings.ambientMode=AmbientMode.Flat;RenderSettings.ambientLight=new Color(.33f,.37f,.32f);RenderSettings.fog=false;RenderSettings.skybox=null;
            Box("Records floor",new Vector3(0,-.1f,0),new Vector3(6,.2f,6),M("Linoleum"));
            Box("Back plaster",new Vector3(0,1.6f,2.4f),new Vector3(6,3.2f,.2f),M("UpperWall"));
            Box("Back green dado",new Vector3(0,.6f,2.28f),new Vector3(6,1.2f,.06f),M("LowerWall"));
            Box("Left wall",new Vector3(-2.8f,1.5f,0),new Vector3(.2f,3,5),M("UpperWall"));
            Box("Work table",new Vector3(0,.76f,0),new Vector3(2.7f,.09f,1.35f),M("Green"));
            foreach(float x in new[]{-1.13f,1.13f})foreach(float z in new[]{-.48f,.48f})Box("Table leg",new Vector3(x,.36f,z),new Vector3(.06f,.72f,.06f),M("Steel"));
            Model("Assets/Signal47/Art/Chapter09/CH09_LabStool.fbx","Lab stool",new Vector3(1.45f,0,-.2f),.5f);
            for(int i=0;i<3;i++)
            {
                float x=-1.95f+i*1.6f;Box("Archive cabinet",new Vector3(x,.98f,1.89f),new Vector3(1.3f,1.96f,.56f),M("Green"));
                for(int j=0;j<4;j++)
                {float y=.25f+j*.46f;Box("Drawer front",new Vector3(x,y,1.59f),new Vector3(1.19f,.4f,.04f),M("LowerWall"));Box("Label holder",new Vector3(x,y+.05f,1.557f),new Vector3(.29f,.09f,.022f),M("Paper"));Box("Drawer handle",new Vector3(x,y-.085f,1.52f),new Vector3(.21f,.025f,.05f),M("Steel"));Label("Drawer index",$"{1947+i*10} / {j+1:00}",new Vector3(x,y+.05f,1.539f),.012f,Quaternion.identity,font);}
            }
            var folder=Model(Art+"A16_Folio.fbx","Protocol folio",new Vector3(0,.81f,-.16f),.46f);
            var rs=folder.GetComponentsInChildren<Renderer>();var b=rs[0].bounds;foreach(var r in rs)b.Encapsulate(r.bounds);
            if(b.size.y>.06f||b.size.z<.3f||b.size.x>.48f)throw new Exception("Folio Unity bounds invalid: "+b);
            var hit=new GameObject("Folio interaction").AddComponent<BoxCollider>();hit.transform.position=b.center;hit.size=b.size+new Vector3(.04f,.04f,.04f);
            var labelBounds=rs.First(r=>r.name.Contains("Label field")).bounds;
            Label("Folio caption","STATION 01\nFIELD RECORD / 1947",new Vector3(labelBounds.center.x,labelBounds.max.y+.001f,labelBounds.center.z),.014f,Quaternion.Euler(90,0,0),font,.245f);
            Box("Document stack",new Vector3(.69f,.83f,.12f),new Vector3(.34f,.045f,.25f),M("Paper"));
            Box("Task lamp base",new Vector3(-.83f,.835f,.2f),new Vector3(.3f,.07f,.2f),M("Steel"));
            Box("Task lamp stem",new Vector3(-.83f,1.04f,.25f),new Vector3(.025f,.42f,.025f),M("Steel"));
            Box("Task lamp shade",new Vector3(-.69f,1.27f,.22f),new Vector3(.43f,.1f,.22f),M("Green"));
            Box("Task lamp diffuser",new Vector3(-.69f,1.214f,.22f),new Vector3(.34f,.012f,.16f),M("WarmDiffuser"));
            var lamp=new GameObject("Warm task light").AddComponent<Light>();lamp.type=LightType.Point;lamp.transform.position=new Vector3(-.65f,1.16f,.16f);lamp.color=new Color(1,.79f,.5f);lamp.intensity=.35f;lamp.range=3;lamp.shadows=LightShadows.Soft;
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
            string output=Path.GetFullPath("../Artifacts/GauntletLinux/Signal47.x86_64");
            var report=BuildPipeline.BuildPlayer(new BuildPlayerOptions{scenes=new[]{scenePath},locationPathName=output,target=BuildTarget.StandaloneLinux64,options=BuildOptions.None});
            if(report.summary.result!=BuildResult.Succeeded)throw new Exception("Archive build failed: "+report.summary.result);
            File.Copy("../Docs/THIRD_PARTY_NOTICES.md",Path.Combine(Path.GetDirectoryName(output),"THIRD_PARTY_NOTICES.md"),true);File.Copy("Assets/Signal47/Art/ThirdParty/VT323/OFL.txt",Path.Combine(Path.GetDirectoryName(output),"VT323-OFL.txt"),true);
            Debug.Log("ARCHIVE16_BUILD_OK");
        }
    }
}
#endif
