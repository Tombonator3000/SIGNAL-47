#if UNITY_EDITOR
using System.IO;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine.SceneManagement;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;
using Signal47.Core; using Signal47.Player; using Signal47.Interaction; using Signal47.Signals; using Signal47.Events; using Signal47.Environment; using Signal47.UI; using Signal47.Investigation;
namespace Signal47.Editor
{
    public static class Signal47SceneBuilder
    {
        const string ScenePath="Assets/Signal47/Scenes/Prototype/SARO_Prologue.unity";
        [InitializeOnLoadMethod] static void Auto(){EditorApplication.delayCall+=()=>{if(!Application.isBatchMode && !File.Exists(ScenePath)) BuildVerticalSlice();};}
        [MenuItem("SIGNAL 47/Build or Rebuild Vertical Slice")]
        public static void BuildVerticalSlice()
        {
            AssetDatabase.Refresh();EnsureURP();EnsureInputSystem();var scene=EditorSceneManager.NewScene(NewSceneSetup.EmptyScene,NewSceneMode.Single);
            var matFloor=Mat("MAT_Floor",new Color(.28f,.33f,.31f),.82f);var matWall=Mat("MAT_Wall",new Color(.56f,.58f,.53f),.85f);var matLower=Mat("MAT_LowerWall",new Color(.20f,.28f,.27f),.72f);var matBeige=Mat("MAT_BeigePlastic",new Color(.58f,.55f,.46f),.63f);var matMetal=Mat("MAT_PaintedMetal",new Color(.31f,.36f,.35f),.48f);var matDark=Mat("MAT_Dark",new Color(.055f,.065f,.06f),.6f);var matWood=Mat("MAT_Formica",new Color(.28f,.20f,.14f),.68f);var matGlass=Mat("MAT_Glass",new Color(.08f,.15f,.18f),.2f);var matGreen=Mat("MAT_CRTGreen",new Color(.01f,.13f,.045f),.35f,true,new Color(.06f,.9f,.2f)*1.5f);var matPaper=Mat("MAT_Paper",new Color(.78f,.74f,.60f),.9f);
            // room follows the 0.2 web prototype footprint and player start.
            Cube("Floor",new Vector3(0,-.08f,0),new Vector3(18.7f,.16f,15f),matFloor,true);
            Cube("Ceiling",new Vector3(0,3.45f,0),new Vector3(18.7f,.10f,15f),matWall,true);
            Cube("BackWall",new Vector3(0,1.72f,7.5f),new Vector3(18.7f,3.45f,.12f),matWall,true);Cube("LeftWall",new Vector3(-9.35f,1.72f,0),new Vector3(.12f,3.45f,15f),matLower,true);Cube("RightWall",new Vector3(9.35f,1.72f,0),new Vector3(.12f,3.45f,15f),matLower,true);
            // front window wall: lower sill, upper header, side mullions
            Cube("FrontSill",new Vector3(0,.52f,-7.5f),new Vector3(18.7f,1.05f,.12f),matLower,true);Cube("FrontHeader",new Vector3(0,3.15f,-7.5f),new Vector3(18.7f,.60f,.12f),matWall,true);
            for(int i=-4;i<=4;i++)Cube($"WindowMullion_{i}",new Vector3(i*2.05f,1.85f,-7.48f),new Vector3(.09f,2.1f,.10f),matMetal,true);
            // Open sightline through the glass; an invisible collision pane bounds the room.
            var pane=Cube("WindowGlass",new Vector3(0,1.85f,-7.54f),new Vector3(18.4f,2.0f,.03f),matGlass,true);pane.GetComponent<Renderer>().enabled=false;
            // control desk + objects
            var desk=Prefab("SM_ControlDesk_A",new Vector3(0,0,-2.25f),Quaternion.identity,new Vector3(1,1,1),matWood);AddBoxCollider(desk,new Vector3(6.9f,.85f,1.1f),new Vector3(0,.42f,0));
            var console=Prefab("SM_CRT_Terminal_A",new Vector3(0,.82f,-2.32f),Quaternion.Euler(0,180,0),Vector3.one,matBeige);AddBoxCollider(console,new Vector3(1.2f,1.1f,.85f),new Vector3(0,.45f,0));var sc=console.AddComponent<SignalConsole>();var screen=GameObject.CreatePrimitive(PrimitiveType.Quad);screen.name="PhysicalDisplay";screen.transform.SetParent(console.transform,false);screen.transform.localPosition=new Vector3(0,.60f,-.435f);screen.transform.localScale=new Vector3(.82f,.49f,1);Object.DestroyImmediate(screen.GetComponent<Collider>());var screenMat=AssetDatabase.LoadAssetAtPath<Material>("Assets/Signal47/Art/PrototypePort/PhysicalDisplay.mat");if(!screenMat){screenMat=new Material(Shader.Find("Universal Render Pipeline/Unlit"));screenMat.name="PhysicalDisplay";AssetDatabase.CreateAsset(screenMat,"Assets/Signal47/Art/PrototypePort/PhysicalDisplay.mat");}screen.GetComponent<Renderer>().sharedMaterial=screenMat;sc.physicalScreen=screen.GetComponent<Renderer>();
            var printer=Prefab("SM_DotMatrixPrinter_A",new Vector3(3.1f,.82f,-1.98f),Quaternion.Euler(0,180,0),Vector3.one,matBeige);AddBoxCollider(printer,new Vector3(1.2f,.6f,.85f),new Vector3(0,.28f,0));var pp=printer.AddComponent<PaperInteractable>();pp.kind=PaperInteractable.PaperKind.Printer;
            var receiver=Prefab("SM_ReceiverRack_A",new Vector3(-6.3f,0,1.60f),Quaternion.Euler(0,180,0),Vector3.one,matMetal);AddBoxCollider(receiver,new Vector3(1.7f,2.7f,.72f),new Vector3(0,1.35f,0));var rb=receiver.AddComponent<ReceiverBank>();rb.leds=FindRenderers(receiver,"LED_");
            var phoneDesk=Cube("PhoneDesk",new Vector3(5.1f,.61f,.5f),new Vector3(2.2f,1.22f,1.05f),matWood,true);var phone=Prefab("SM_DeskPhone_A",new Vector3(5.1f,1.30f,.5f),Quaternion.identity,Vector3.one,matBeige);AddBoxCollider(phone,new Vector3(.76f,.5f,.65f),new Vector3(0,.23f,0));phone.AddComponent<PhoneInteractable>();
            var clip=Cube("ShiftClipboard",new Vector3(-1.7f,1.15f,5.2f),new Vector3(.65f,.04f,.95f),matPaper,true);var cp=clip.AddComponent<PaperInteractable>();cp.kind=PaperInteractable.PaperKind.Clipboard;
            // mug intact + shards, exactly reused story position from web prototype
            var mugRoot=new GameObject("StoryMug");mugRoot.transform.position=new Vector3(4.35f,1.225f,.30f);var intact=Prefab("SM_Mug_Intact_A",Vector3.zero,Quaternion.identity,Vector3.one,matPaper,mugRoot.transform);var broken=Prefab("SM_Mug_Broken_A",Vector3.zero,Quaternion.identity,Vector3.one,matPaper,mugRoot.transform);broken.SetActive(false);var mb=mugRoot.AddComponent<MugBreakable>();mb.intact=intact;mb.broken=broken;AddBoxCollider(mugRoot,new Vector3(.32f,.32f,.32f),new Vector3(0,.16f,0));
            // set dressing, kept outside interaction paths
            for(int i=0;i<3;i++)Cube($"Filing_{i}",new Vector3(-8.45f+i*.78f,1.0f,-4.8f),new Vector3(.68f,2f,.78f),matMetal,true);
            for(int i=0;i<5;i++)Cube($"Binder_{i}",new Vector3(-8.5f+i*.18f,2.25f,-4.75f),new Vector3(.14f,.42f,.35f),i%2==0?matLower:matWall,false);
            Cube("WallChart_A",new Vector3(-4.3f,2.15f,7.42f),new Vector3(2.6f,1.25f,.035f),matPaper,false);Cube("SARO_Sign",new Vector3(4.8f,2.45f,7.42f),new Vector3(3.1f,.58f,.035f),matDark,false);
            // lights
            var moon=new GameObject("Moonlight");var dl=moon.AddComponent<Light>();dl.type=LightType.Directional;dl.color=new Color(.48f,.61f,.78f);dl.intensity=.55f;dl.shadows=LightShadows.Soft;dl.transform.rotation=Quaternion.Euler(35,-28,0);
            for(int i=-2;i<=2;i++){var lgo=Cube($"Fluorescent_{i}",new Vector3(i*3.1f,3.30f,-.3f),new Vector3(1.8f,.05f,.25f),matPaper,false);var l=lgo.AddComponent<Light>();l.type=LightType.Point;l.range=9f;l.intensity=4.5f;l.color=new Color(.73f,.84f,.74f);l.shadows=LightShadows.None;}
            var deskLamp=new GameObject("DeskLamp");deskLamp.transform.position=new Vector3(4.4f,2.05f,.2f);var pl=deskLamp.AddComponent<Light>();pl.type=LightType.Point;pl.range=3;pl.intensity=1.4f;pl.color=new Color(1f,.58f,.25f);pl.shadows=LightShadows.None;
            RenderSettings.ambientMode=UnityEngine.Rendering.AmbientMode.Flat;RenderSettings.ambientLight=new Color(.20f,.23f,.24f);RenderSettings.fog=true;RenderSettings.fogColor=new Color(.045f,.065f,.085f);RenderSettings.fogMode=FogMode.Exponential;RenderSettings.fogDensity=.0085f;
            // exterior desert + dishes using same web positions
            Cube("Desert",new Vector3(0,-.28f,-67),new Vector3(180,.35f,120),Mat("MAT_Desert",new Color(.19f,.22f,.20f),1),true);
            Cube("ServiceRoad",new Vector3(0,-.08f,-55),new Vector3(4.7f,.02f,92),Mat("MAT_Road",new Color(.25f,.25f,.22f),1),false);
            float[,] dp={{-8,-21,1},{3,-25,.95f},{14,-32,.95f},{-19,-35,.95f},{-5,-43,.9f},{9,-51,.9f},{25,-60,.86f},{-30,-58,.85f},{-15,-73,.85f},{0,-80,.85f},{35,-88,.8f}};var pivots=new List<Transform>();
            for(int i=0;i<dp.GetLength(0);i++){var root=new GameObject($"Dish_{i:00}");root.transform.position=new Vector3(dp[i,0],0,dp[i,1]);root.transform.localScale=Vector3.one*dp[i,2];Prefab("SM_RadioDish_Pedestal_A",Vector3.zero,Quaternion.identity,Vector3.one,matMetal,root.transform);var pivot=new GameObject("DishPivot");pivot.transform.SetParent(root.transform,false);pivot.transform.localPosition=new Vector3(0,2.3f,0);Prefab("SM_RadioDish_Bowl_A",Vector3.zero,Quaternion.Euler(38,(i%3-1)*10,0),Vector3.one,matMetal,pivot.transform);pivots.Add(pivot.transform);}
            var da=new GameObject("DishArray").AddComponent<DishArrayController>();da.dishPivots=pivots.ToArray();
            // distant mesa silhouettes
            for(int i=0;i<24;i++){float x=-75+i*6.5f;float h=3+Mathf.Abs(Mathf.Sin(i*.73f))*7;Cube($"Mesa_{i:00}",new Vector3(x,h*.5f,-108-i%3*4),new Vector3(8,h,6),matDark,false);}
            // player
            var pg=new GameObject("Player");pg.layer=LayerMask.NameToLayer("Ignore Raycast");pg.transform.position=new Vector3(0,.05f,6.7f);pg.transform.rotation=Quaternion.Euler(0,180,0);var cc=pg.AddComponent<CharacterController>();cc.height=1.8f;cc.radius=.34f;cc.center=new Vector3(0,.9f,0);var camGo=new GameObject("ViewCamera");camGo.transform.SetParent(pg.transform,false);camGo.transform.localPosition=new Vector3(0,1.63f,0);var cam=camGo.AddComponent<Camera>();cam.fieldOfView=60;cam.nearClipPlane=.05f;cam.farClipPlane=230;cam.clearFlags=CameraClearFlags.SolidColor;cam.backgroundColor=new Color(.015f,.028f,.045f);camGo.AddComponent<AudioListener>();var fps=pg.AddComponent<FirstPersonController>();fps.viewCamera=cam;var pi=pg.AddComponent<PlayerInteractor>();pi.viewCamera=cam;
            // runtime services
            var systems=new GameObject("_SYSTEMS");var notebook=systems.AddComponent<Notebook>();var hud=systems.AddComponent<HUDController>();hud.notebook=notebook;var director=systems.AddComponent<PrologueDirector>();director.mug=mb;director.dishes=da;
            director.printerSource=Audio(printer,1,.25f);director.phoneSource=Audio(phone,1,1);var eventPoint=new GameObject("StorySound");eventPoint.transform.position=new Vector3(0,2,-15);director.eventSource=Audio(eventPoint,0,1);director.ambienceSource=Audio(systems,0,.45f);director.phoneSource.spatialBlend=1;director.phoneSource.minDistance=1;director.phoneSource.maxDistance=12;director.printerSource.spatialBlend=1;director.printerSource.maxDistance=10;director.eventSource.spatialBlend=1;director.eventSource.maxDistance=40;
            var session=systems.AddComponent<GameSession>();session.player=fps;session.hud=hud;session.notebook=notebook;session.director=director;
            // signal profiles ported verbatim from web prototype thresholds
            sc.profiles=new[]{Profile("SIG_CALIBRATION","CALIBRATION","LOG CALIBRATION","TAPED REF CARD // CAL 1419.900 MHz · AZ 042° · GAIN 55 ±10 · BW 48 ±14",1419.900f,.025f,new Vector2(45,65),new Vector2(34,62),new Vector2(35,49),"Receiver calibration completed at 23:4x local."),Profile("SIG_INTERFERENCE","INTERFERENCE","NOTCH INTERFERENCE","LOCAL SPIKE // center carrier near 1420.110 MHz. Narrow BW below 22 kHz while retaining nominal gain.",1420.110f,.03f,new Vector2(40,72),new Vector2(4,22),new Vector2(0,180),"Local carrier interference rejected."),Profile("SIG_0047_UNKNOWN","ANOMALY","ISOLATE PATTERN","UNLOGGED RESIDUAL // weak peak near 1420.4 MHz. Increase gain, narrow bandwidth, then search azimuth manually.",1420.405f,.008f,new Vector2(78,100),new Vector2(4,14),new Vector2(79,87),"Unlogged narrowband carrier isolated near hydrogen line.")};
            ArtPass.Dress();StoryPass.Dress(printer,phone,director,matPaper);ExternalAssetsPass.Dress();ServiceYardPass.Dress(session);FieldCameraPass.Dress(session);
            Chapter09Pass.Dress(session);Visual10Pass.Dress(session);systems.AddComponent<Signal47.Chapter.ChapterSave>();
            EditorSceneManager.SaveScene(scene,ScenePath);EditorBuildSettings.scenes=new[]{new EditorBuildSettingsScene(ScenePath,true)};AssetDatabase.SaveAssets();Debug.Log("SIGNAL / 47 chapter generated: "+ScenePath);
        }

        static void EnsureURP()
        {
            const string pipelinePath="Assets/Signal47/Art/PrototypePort/Signal47_URP.asset";
            const string rendererPath="Assets/Signal47/Art/PrototypePort/Signal47_URP_Renderer.asset";
            var urp=AssetDatabase.LoadAssetAtPath<UniversalRenderPipelineAsset>(pipelinePath);
            if(!urp)
            {
                urp=ScriptableObject.CreateInstance<UniversalRenderPipelineAsset>();
                AssetDatabase.CreateAsset(urp,pipelinePath);
                var renderer=urp.LoadBuiltinRendererData();
                if(renderer&&!AssetDatabase.Contains(renderer)) AssetDatabase.CreateAsset(renderer,rendererPath);
                EditorUtility.SetDirty(urp);AssetDatabase.SaveAssets();
            }
            GraphicsSettings.defaultRenderPipeline=urp;QualitySettings.renderPipeline=urp;
        }
        static void EnsureInputSystem()
        {
            var assets=AssetDatabase.LoadAllAssetsAtPath("ProjectSettings/ProjectSettings.asset");
            if(assets.Length==0)return;var so=new SerializedObject(assets[0]);var p=so.FindProperty("activeInputHandler");
            if(p!=null){p.intValue=2;so.ApplyModifiedPropertiesWithoutUndo();}
        }
        internal static Material Mat(string name,Color color,float smooth,bool emissive=false,Color emission=default){string path=$"Assets/Signal47/Art/PrototypePort/{name}.mat";var m=AssetDatabase.LoadAssetAtPath<Material>(path);if(m)return m;var shader=Shader.Find("Universal Render Pipeline/Lit")??Shader.Find("Standard");m=new Material(shader){name=name};if(m.HasProperty("_BaseColor"))m.SetColor("_BaseColor",color);else m.color=color;if(m.HasProperty("_Smoothness"))m.SetFloat("_Smoothness",1-smooth);if(emissive&&m.HasProperty("_EmissionColor")){m.EnableKeyword("_EMISSION");m.SetColor("_EmissionColor",emission);}AssetDatabase.CreateAsset(m,path);return m;}
        internal static GameObject Cube(string n,Vector3 pos,Vector3 scale,Material m,bool collider){var g=GameObject.CreatePrimitive(PrimitiveType.Cube);g.name=n;g.transform.position=pos;g.transform.localScale=scale;g.GetComponent<Renderer>().sharedMaterial=m;if(!collider)Object.DestroyImmediate(g.GetComponent<Collider>());return g;}
        internal static GameObject Prefab(string name,Vector3 pos,Quaternion rot,Vector3 scale,Material mat,Transform parent=null){var src=(name=="SM_DeskPhone_A"?AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Signal47/Art/Authored/SM_StoryPhone.fbx"):null)??AssetDatabase.LoadAssetAtPath<GameObject>($"Assets/Signal47/Art/Refined/{name}.fbx")??AssetDatabase.LoadAssetAtPath<GameObject>($"Assets/Signal47/Art/PrototypePort/{name}.obj");GameObject g=src?(GameObject)PrefabUtility.InstantiatePrefab(src):Cube(name,pos,Vector3.one,mat,false);g.name=name;if(parent)g.transform.SetParent(parent,false);g.transform.localPosition=pos;g.transform.localRotation=rot;g.transform.localScale=scale;foreach(var r in g.GetComponentsInChildren<Renderer>())r.sharedMaterial=ArtPass.PropMaterial(r.name,mat);return g;}
        static void AddBoxCollider(GameObject g,Vector3 size,Vector3 center){var c=g.AddComponent<BoxCollider>();c.size=size;c.center=center;}
        static Renderer FindRenderer(GameObject g,string token){foreach(var r in g.GetComponentsInChildren<Renderer>())if(r.name.Contains(token))return r;return g.GetComponentInChildren<Renderer>();}
        static Renderer[] FindRenderers(GameObject g,string token){var l=new List<Renderer>();foreach(var r in g.GetComponentsInChildren<Renderer>())if(r.name.Contains(token))l.Add(r);return l.ToArray();}
        static AudioSource Audio(GameObject g,float spatial,float vol){var a=g.AddComponent<AudioSource>();a.playOnAwake=false;a.spatialBlend=spatial;a.volume=vol;return a;}
        static SignalProfile Profile(string asset,string stage,string action,string card,float f,float tol,Vector2 gain,Vector2 bw,Vector2 az,string note){string path=$"Assets/Signal47/ScriptableObjects/Signals/{asset}.asset";var p=AssetDatabase.LoadAssetAtPath<SignalProfile>(path);if(!p){p=ScriptableObject.CreateInstance<SignalProfile>();AssetDatabase.CreateAsset(p,path);}p.stageName=stage;p.actionLabel=action;p.referenceCard=card;p.targetFrequency=f;p.frequencyTolerance=tol;p.gainRange=gain;p.bandwidthRange=bw;p.azimuthRange=az;p.successNote=note;EditorUtility.SetDirty(p);return p;}
    }
}
#endif
