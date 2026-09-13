#if UNITY_EDITOR
using System;
using System.IO;
using System.Linq;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEditor;
using UnityEditor.SceneManagement;
using Signal47.Core;
using Signal47.Station26;
using Object=UnityEngine.Object;
namespace Signal47.Editor
{
    // Additive authoring over the preserved baked SARO scene. No original objects are regenerated.
    public static class Station26Build
    {
        const string Art="Assets/Signal47/Art/Station26/";
        static readonly Vector3 Origin=new Vector3(120,0,100);
        static Transform world;
        static Font font;
        static Material stone,metal,paper,dark,warm,worldText;
        static StationController controller;
        [Serializable] sealed class MeshData {public float[] positions,normals,uv;public int[] triangles;}
        static Material Mat(string name,Color color)
        {
            string path=Art+name+".mat";var mat=AssetDatabase.LoadAssetAtPath<Material>(path);
            if(!mat){mat=new Material(Shader.Find("Universal Render Pipeline/Lit"));AssetDatabase.CreateAsset(mat,path);}
            mat.color=color;mat.SetFloat("_Smoothness",.17f);EditorUtility.SetDirty(mat);return mat;
        }
        static GameObject Box(string name,Vector3 pos,Vector3 size,Material material,bool collision=true)
        {
            var go=GameObject.CreatePrimitive(PrimitiveType.Cube);go.name="Station26."+name;go.transform.SetParent(world,false);
            go.transform.localPosition=pos;go.transform.localScale=size;go.GetComponent<Renderer>().sharedMaterial=material;
            if(!collision)Object.DestroyImmediate(go.GetComponent<Collider>());return go;
        }
        static Transform Pose(string name,Vector3 pos,float yaw=0)
        {
            var go=new GameObject("Station26."+name);go.transform.SetParent(world,false);go.transform.localPosition=pos;go.transform.localRotation=Quaternion.Euler(0,yaw,0);return go.transform;
        }
        static GameObject Label(string text,Vector3 pos,float size=.07f,float yaw=0)
        {
            var go=new GameObject("Station26.Label "+text);go.transform.SetParent(world,false);go.transform.localPosition=pos;go.transform.localRotation=Quaternion.Euler(0,yaw,0);
            var tm=go.AddComponent<TextMesh>();tm.text=text;tm.font=font;tm.fontSize=64;tm.characterSize=size;tm.anchor=TextAnchor.MiddleCenter;tm.alignment=TextAlignment.Center;tm.color=new Color(.88f,.87f,.73f);go.GetComponent<Renderer>().sharedMaterial=worldText;return go;
        }
        static void Action(GameObject go,string page)
        {
            var action=go.AddComponent<StationAction>();action.controller=controller;action.action=page;
        }
        static void Beam(string name,Vector3 a,Vector3 b,float width,Material material)
        {
            var go=Box(name,(a+b)*.5f,new Vector3(width,width,Vector3.Distance(a,b)),material,false);go.transform.localRotation=Quaternion.LookRotation(b-a);
        }
        static void CableSegment(string name,Vector3 a,Vector3 b,float radius,Material material)
        {
            var go=GameObject.CreatePrimitive(PrimitiveType.Cylinder);go.name="Station26."+name;go.transform.SetParent(world,false);
            go.transform.localPosition=(a+b)*.5f;go.transform.localRotation=Quaternion.FromToRotation(Vector3.up,b-a);go.transform.localScale=new Vector3(radius*2,Vector3.Distance(a,b)*.5f,radius*2);go.GetComponent<Renderer>().sharedMaterial=material;Object.DestroyImmediate(go.GetComponent<Collider>());
        }
        static Light Lamp(string name,Vector3 pos,Color color,float intensity,float range)
        {
            var go=new GameObject("Station26."+name);go.transform.SetParent(world,false);go.transform.localPosition=pos;
            var light=go.AddComponent<Light>();light.type=LightType.Point;light.color=color;light.intensity=intensity;light.range=range;light.shadows=LightShadows.Soft;return light;
        }
        static void Model(string file,Vector3 pos,float rotation)
        {
            var source=AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Signal47/Art/Archive17/"+file+".fbx");
            var go=(GameObject)PrefabUtility.InstantiatePrefab(source);go.name="Station26."+file;go.transform.SetParent(world,false);go.transform.localPosition=pos;go.transform.localRotation=Quaternion.Euler(0,rotation,0);
            foreach(var r in go.GetComponentsInChildren<Renderer>())r.sharedMaterials=r.sharedMaterials.Select(m=>m&&m.name.Contains("Paper")?paper:m&&m.name.Contains("Steel")?metal:stone).ToArray();
        }
        static void Building()
        {
            // Same clipped, textured visual surface from Hybrid23; conventional mesh, no splat dependency.
            var data=JsonUtility.FromJson<MeshData>(File.ReadAllText(Art+"station-mesh.json"));int n=data.positions.Length/3;
            var vertices=new Vector3[n];var normals=new Vector3[n];var uv=new Vector2[n];
            for(int i=0;i<n;i++){vertices[i]=new Vector3(data.positions[3*i],data.positions[3*i+1],data.positions[3*i+2]);normals[i]=new Vector3(data.normals[3*i],data.normals[3*i+1],data.normals[3*i+2]);uv[i]=new Vector2(data.uv[2*i],data.uv[2*i+1]);}
            var mesh=AssetDatabase.LoadAssetAtPath<Mesh>(Art+"StationShell.asset");if(!mesh){mesh=new Mesh();AssetDatabase.CreateAsset(mesh,Art+"StationShell.asset");}else mesh.Clear();
            mesh.indexFormat=IndexFormat.UInt32;mesh.vertices=vertices;mesh.normals=normals;mesh.uv=uv;mesh.triangles=data.triangles;mesh.RecalculateBounds();EditorUtility.SetDirty(mesh);
            var mat=Mat("Hut",Color.white);mat.mainTexture=AssetDatabase.LoadAssetAtPath<Texture2D>(Art+"station-texture.jpg");EditorUtility.SetDirty(mat);
            var shell=new GameObject("Station26.TexturedFieldHut");shell.transform.SetParent(world,false);shell.transform.localPosition=new Vector3(0,0,10);
            shell.AddComponent<MeshFilter>().sharedMesh=mesh;shell.AddComponent<MeshRenderer>().sharedMaterial=mat;
            // Interior and colliders follow the inspected Hybrid23 bounds: x +-3, z 0..5, front doorway x +-.8.
            Box("HutFloor",new Vector3(0,.045f,13),new Vector3(7.16f,.07f,5.16f),stone);
            Box("HutBack",new Vector3(0,1.35f,15.55f),new Vector3(7.16f,.06f,2.7f),stone).transform.localRotation=Quaternion.Euler(90,0,0);
            Box("HutWest",new Vector3(-3.55f,1.35f,13),new Vector3(.06f,2.7f,5.16f),stone);
            Box("HutEast",new Vector3(3.55f,1.35f,13),new Vector3(.06f,2.7f,5.16f),stone);
            Box("HutFrontWest",new Vector3(-2.15f,1.35f,10.45f),new Vector3(2.8f,2.7f,.06f),stone);
            Box("HutFrontEast",new Vector3(2.15f,1.35f,10.45f),new Vector3(2.8f,2.7f,.06f),stone);
            Box("HutLintel",new Vector3(0,2.5f,10.45f),new Vector3(1.5f,.4f,.06f),stone);
            Box("HutCeiling",new Vector3(0,2.63f,13),new Vector3(7.16f,.06f,5.16f),stone);
            Label("S T A T I O N  0 1",new Vector3(0,2.42f,9.88f),.058f);
            Model("A17_Desk",new Vector3(0,0,13.7f),180);
            Box("RecordsBenchCollider",new Vector3(0,.40f,13.7f),new Vector3(1.7f,.8f,.7f),stone).GetComponent<Renderer>().enabled=false;
            var log=Box("TimingRecord",new Vector3(-.35f,.84f,13.65f),new Vector3(.42f,.04f,.30f),paper);Action(log,"timing");
            var receiver=Box("Receiver",new Vector3(.5f,1.02f,13.72f),new Vector3(.48f,.30f,.23f),metal);Action(receiver,"timing");
            for(int i=0;i<6;i++)Box("ReceiverVent",new Vector3(.35f+i*.025f,1.03f,13.592f),new Vector3(.008f,.15f,.003f),dark,false);
            Label("FIELD TIMING / T. VEGA",new Vector3(0,1.7f,14.87f),.043f);
            Lamp("HutLamp",new Vector3(.7f,2.3f,12.5f),new Color(1,.73f,.42f),2.1f,7);
        }
        public static void Build()
        {
            EditorSceneManager.OpenScene("Assets/Signal47/Scenes/Prototype/SARO_Prologue.unity");
            var prior=GameObject.Find("Station26");if(prior)Object.DestroyImmediate(prior);
            var g=Object.FindFirstObjectByType<GameSession>();if(!g||!g.worldCase||LightmapSettings.lightmaps.Length!=2)throw new Exception("Preserved WorldCase22 and baked SARO required");
            var root=new GameObject("Station26");controller=root.AddComponent<StationController>();g.station=controller;
            var worldGo=new GameObject("Station26.Area");worldGo.transform.SetParent(root.transform,false);worldGo.transform.position=Origin;world=worldGo.transform;controller.stationRoot=worldGo;
            font=g.hud.terminalFont;controller.font=font;
            worldText=AssetDatabase.LoadAssetAtPath<Material>(Art+"WorldText.mat");
            if(!worldText){worldText=new Material(Shader.Find("Signal47/Archive World Text"));AssetDatabase.CreateAsset(worldText,Art+"WorldText.mat");}
            worldText.mainTexture=font.material.mainTexture;EditorUtility.SetDirty(worldText);
            stone=Mat("WeatheredStone",new Color(.32f,.28f,.22f));metal=Mat("SurveyGreen",new Color(.14f,.22f,.19f));paper=Mat("RecordPaper",new Color(.83f,.8f,.65f));dark=Mat("Rubber",new Color(.035f,.044f,.042f));warm=Mat("Copper",new Color(.55f,.29f,.11f));
            var groundMat=Mat("FieldGround",new Color(.29f,.25f,.18f));
            Box("Ground",new Vector3(0,-.22f,5),new Vector3(38,.4f,38),groundMat?groundMat:stone);
            Box("Approach",new Vector3(0,-.015f,2),new Vector3(2.2f,.05f,17),stone);
            Box("BoundaryWest",new Vector3(-16,1,5),new Vector3(.6f,2,36),stone);
            Box("BoundaryEast",new Vector3(16,1,5),new Vector3(.6f,2,36),stone);
            Box("BoundaryNorth",new Vector3(0,1,22),new Vector3(32,2,.6f),stone);
            Box("BoundarySouth",new Vector3(0,1,-13),new Vector3(32,2,.6f),stone);
            Building();
            controller.arrival=Pose("Arrival",new Vector3(0,.12f,-6));
            controller.saroReturn=Pose("SAROReturn",Vector3.zero);controller.saroReturn.position=new Vector3(1.2f,.1f,9.4f);controller.saroReturn.rotation=Quaternion.Euler(0,90,0);
            var travel=Box("ReturnFolio",new Vector3(0,.94f,-6.9f),new Vector3(.65f,.12f,.45f),paper);Action(travel,"return");
            Box("ReturnStand",new Vector3(0,.43f,-6.9f),new Vector3(.1f,.86f,.1f),metal);Label("SARO / RETURN",new Vector3(0,1.30f,-6.89f),.039f,180);
            var departure=Box("DepartureFolio",Vector3.zero,new Vector3(.45f,.045f,.32f),paper);departure.transform.SetParent(root.transform,true);departure.transform.position=new Vector3(2.65f,GameObject.Find("CH09.ArchiveBench.FormicaTop").GetComponent<Renderer>().bounds.max.y+.024f,9.9f);Action(departure,"travel-station");
            var departureSign=Label("FIELD TRAVEL",Vector3.zero,.027f);departureSign.transform.SetParent(root.transform,true);departureSign.transform.SetPositionAndRotation(new Vector3(2.35f,1.09f,9.9f),Quaternion.Euler(90,90,0));
            // Transit and all fixed-point geometry share this explicit local arrangement.
            var transit=Box("Transit",new Vector3(-5,1.18f,0),new Vector3(.44f,.20f,.36f),metal);Action(transit,"marker");
            for(int i=0;i<3;i++){float angle=i*Mathf.PI*2/3;Beam("TransitLeg",new Vector3(-5,1.08f,0),new Vector3(-5+Mathf.Cos(angle)*.45f,.02f,Mathf.Sin(angle)*.45f),.045f,metal);}
            Box("HistoricFooting",new Vector3(-5,.07f,6),new Vector3(.65f,.14f,.6f),stone);Label("A / ORIGINAL FOOTING",new Vector3(-5,.38f,5.7f),.027f);
            var moved=Box("MovedMark",new Vector3(-3.9f,.70f,6),new Vector3(.18f,1.4f,.18f),metal);Action(moved,"marker");
            Box("MarkerStripe",new Vector3(-3.9f,1.12f,5.9f),new Vector3(.30f,.055f,.012f),paper,false);
            Label("A",new Vector3(-3.9f,1.31f,5.89f),.043f);
            Box("FixedMarkB",new Vector3(-7.5f,.7f,6),new Vector3(.18f,1.4f,.18f),metal);Label("B / FIXED",new Vector3(-7.5f,1.2f,5.88f),.033f);
            controller.markerTarget=Pose("MarkerPhotoTarget",new Vector3(-4.6f,.7f,6));
            var lampBase=Box("LampControl",new Vector3(0,.80f,6),new Vector3(.5f,.3f,.35f),metal);Action(lampBase,"lamp");
            Box("LampPost",new Vector3(0,.75f,6),new Vector3(.08f,1.5f,.08f),metal);
            controller.testLamp=Lamp("NullTestLamp",new Vector3(0,1.65f,5.8f),new Color(1,.76f,.4f),2.4f,5);
            Label("ISOLATED LAMP / NO CLOSING LINK",new Vector3(0,1.05f,5.81f),.026f);
            Box("CableInspectionSlab",new Vector3(5,.25f,4),new Vector3(1.4f,.5f,1.05f),stone);
            var route=new[]{new Vector3(0,.06f,6),new Vector3(2,.06f,6),new Vector3(3,.06f,4),new Vector3(4.4f,.54f,4.18f),new Vector3(4.89f,.54f,4)};
            var returnRoute=new[]{new Vector3(5.11f,.54f,4),new Vector3(5.6f,.54f,4.18f),new Vector3(6.5f,.06f,4),new Vector3(6.5f,.06f,8),new Vector3(2,.06f,8),new Vector3(0,.06f,6)};
            for(int i=1;i<route.Length;i++)CableSegment("CableFeed",route[i-1],route[i],.043f,dark);
            for(int i=1;i<returnRoute.Length;i++)CableSegment("CableReturn",returnRoute[i-1],returnRoute[i],.043f,dark);
            CableSegment("CutFaceWest",new Vector3(4.885f,.54f,4.002f),new Vector3(4.9f,.54f,3.996f),.035f,warm);
            CableSegment("CutFaceEast",new Vector3(5.10f,.54f,3.996f),new Vector3(5.115f,.54f,4.002f),.035f,warm);
            var cable=Box("CableInspection",new Vector3(5,.65f,4.47f),new Vector3(.6f,.25f,.15f),metal);Action(cable,"cable");
            controller.cableTarget=Pose("CablePhotoTarget",new Vector3(5,.57f,4));
            Label("CABLE LOOP / KEEP OPEN",new Vector3(5,1.0f,4.38f),.034f);
            Lamp("FieldWorkLight",new Vector3(-4,3,2),new Color(.66f,.76f,1),1.8f,15);
            Lamp("CableWorkLight",new Vector3(6,2.3f,3),new Color(1,.79f,.52f),1.9f,7);
            // Reuse authored stone-colored low mesas as a bounded night silhouette, not new destinations.
            for(int i=0;i<12;i++){float angle=i*Mathf.PI/6;var rock=GameObject.CreatePrimitive(PrimitiveType.Sphere);rock.name="Station26.RockRim";rock.transform.SetParent(world,false);rock.transform.localPosition=new Vector3(Mathf.Cos(angle)*20,-1,5+Mathf.Sin(angle)*20);rock.transform.localScale=new Vector3(9,5+(i%3),7);rock.GetComponent<Renderer>().sharedMaterial=stone;Object.DestroyImmediate(rock.GetComponent<Collider>());}
            var moonGo=new GameObject("Station26.MoonFill");moonGo.transform.SetParent(world,false);moonGo.transform.localRotation=Quaternion.Euler(35,-40,0);
            var moon=moonGo.AddComponent<Light>();moon.type=LightType.Directional;moon.color=new Color(.52f,.64f,.83f);moon.intensity=.68f;moon.shadows=LightShadows.Soft;
            // Boundary colliders are hidden behind terrain rather than drawing a rectangular enclosure.
            foreach(var rr in world.GetComponentsInChildren<Renderer>())if(rr.name.Contains("Boundary"))rr.enabled=false;
            var tuft=AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Signal47/Art/Visual10/Models/V10_DryTuft_A.fbx");
            if(tuft)for(int i=0;i<48;i++){float x=-14+(i*7.13f)%28,z=-10+(i*3.81f)%30;if(Mathf.Abs(x)<8 && z>0 && z<17)continue;var go=(GameObject)PrefabUtility.InstantiatePrefab(tuft);go.name="Station26.DryTuft";go.transform.SetParent(world,false);go.transform.localPosition=new Vector3(x,0,z);go.transform.localRotation=Quaternion.Euler(0,i*43,0);foreach(var rr in go.GetComponentsInChildren<Renderer>())rr.sharedMaterial=stone;}
            // The same triangle-and-bar symbol seen in the source record is physically present on A.
            Vector3 top=new Vector3(-3.9f,1.45f,5.89f),left=new Vector3(-4.05f,1.20f,5.89f),right=new Vector3(-3.75f,1.20f,5.89f);
            Beam("MarkerTriangle",top,left,.018f,paper);Beam("MarkerTriangle",left,right,.018f,paper);Beam("MarkerTriangle",right,top,.018f,paper);
            Environment27Pass.Apply(world,font,worldText);
            worldGo.SetActive(false);EditorUtility.SetDirty(g);AssetDatabase.SaveAssets();EditorSceneManager.SaveScene(UnityEngine.SceneManagement.SceneManager.GetActiveScene());
            Directory.CreateDirectory("../Artifacts/Station26");File.WriteAllText("../Artifacts/Station26/scene-audit.json",JsonUtility.ToJson(new Audit{lightmaps=LightmapSettings.lightmaps.Length,stationColliders=root.GetComponentsInChildren<Collider>(true).Length,source="Preserved WorldCase22 + ordinary mesh from Hybrid23 + original Archive17 props",origin=Origin},true));
            Automation.BuildCurrentGauntletLinux();
        }
        [Serializable] sealed class Audit{public int lightmaps,stationColliders;public string source;public Vector3 origin;}
    }
}
#endif
