#if UNITY_EDITOR
using System.Collections.Generic;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;
using Signal47.Signals;
using static Signal47.Editor.Signal47SceneBuilder;
namespace Signal47.Editor
{
    // Extends the existing pass 06. Original models, interaction transforms and colliders survive.
    public static class ControlRoomPass06
    {
        const string ScenePath="Assets/Signal47/Scenes/Prototype/SARO_Prologue.unity";
        [InitializeOnLoadMethod]
        static void Register(){EditorSceneManager.sceneSaving-=OnSceneSaving;EditorSceneManager.sceneSaving+=OnSceneSaving;}
        static void OnSceneSaving(Scene scene,string path){if(path==ScenePath&&!GameObject.Find("ControlRoomArt06"))Dress();}
        static GameObject LocalBox(Transform parent,string name,Vector3 p,Vector3 size,Material mat,Quaternion rotation=default)
        {
            var go=Cube(name,Vector3.zero,size,mat,false);go.transform.SetParent(parent,false);go.transform.localPosition=p;
            go.transform.localRotation=rotation==default?Quaternion.identity:rotation;
            GameObjectUtility.SetStaticEditorFlags(go,StaticEditorFlags.BatchingStatic);return go;
        }
        static Mesh StoreMesh(string name,Mesh generated)
        {
            string path="Assets/Signal47/Art/Refined/"+name+".asset";
            var old=AssetDatabase.LoadAssetAtPath<Mesh>(path);
            if(old){EditorUtility.CopySerialized(generated,old);Object.DestroyImmediate(generated);EditorUtility.SetDirty(old);return old;}
            generated.name=name;AssetDatabase.CreateAsset(generated,path);return generated;
        }
        static void MeshObject(Transform parent,string name,Mesh mesh,Material material,Vector3 position,Quaternion rotation=default)
        {
            var go=new GameObject(name);go.transform.SetParent(parent,false);go.transform.localPosition=position;
            go.transform.localRotation=rotation==default?Quaternion.identity:rotation;
            go.AddComponent<MeshFilter>().sharedMesh=mesh;go.AddComponent<MeshRenderer>().sharedMaterial=material;
            GameObjectUtility.SetStaticEditorFlags(go,StaticEditorFlags.BatchingStatic);
        }
        static Mesh KeyboardMesh()
        {
            var template=GameObject.CreatePrimitive(PrimitiveType.Cube);
            var cube=template.GetComponent<MeshFilter>().sharedMesh;
            var keys=new List<CombineInstance>();
            void Key(Vector3 p,Vector3 s){keys.Add(new CombineInstance{mesh=cube,transform=Matrix4x4.TRS(p,Quaternion.identity,s)});}
            for(int row=0;row<4;row++)for(int col=0;col<10;col++)
                Key(new Vector3(-.465f+col*.079f,.111f,-.11f+row*.078f),new Vector3(.068f,.025f,.058f));
            for(int row=0;row<3;row++)for(int col=0;col<3;col++)
                Key(new Vector3(.337f+col*.067f,.111f,-.11f+row*.078f),new Vector3(.052f,.025f,.058f));
            Key(new Vector3(-.14f,.111f,-.185f),new Vector3(.35f,.025f,.052f));
            Key(new Vector3(-.43f,.111f,-.185f),new Vector3(.105f,.025f,.052f));
            Key(new Vector3(.18f,.111f,-.185f),new Vector3(.08f,.025f,.052f));
            var result=new Mesh();result.CombineMeshes(keys.ToArray(),true,true);result.RecalculateBounds();
            Object.DestroyImmediate(template);return StoreMesh("KeyboardKeys06",result);
        }
        // Three rings make a shallow bevel that catches practical light. No per-frame geometry work.
        static Mesh Cushion(string name,Vector3 size,float corner)
        {
            float x=size.x*.5f,z=size.z*.5f,h=size.y*.5f;
            var vertices=new List<Vector3>();var triangles=new List<int>();
            Vector2[] outline={new Vector2(-x+corner,-z),new Vector2(x-corner,-z),new Vector2(x,-z+corner),new Vector2(x,z-corner),new Vector2(x-corner,z),new Vector2(-x+corner,z),new Vector2(-x,z-corner),new Vector2(-x,-z+corner)};
            for(int ring=0;ring<3;ring++)for(int i=0;i<8;i++)
            {
                float inset=ring==2?.9f:1f;var v=outline[i];vertices.Add(new Vector3(v.x*inset,ring==0?-h:ring==1?h*.6f:h,v.y*inset));
            }
            // Outward-facing side rings and explicit top/bottom centre caps.
            for(int ring=0;ring<2;ring++)for(int i=0;i<8;i++)
            {int a=ring*8+i,b=ring*8+(i+1)%8,c=a+8,d=b+8;triangles.AddRange(new[]{a,c,b,b,c,d});}
            vertices.Add(new Vector3(0,-h,0));vertices.Add(new Vector3(0,h,0));
            for(int i=0;i<8;i++){int n=(i+1)%8;triangles.AddRange(new[]{24,i,n,25,16+n,16+i});}
            var mesh=new Mesh();mesh.SetVertices(vertices);mesh.SetTriangles(triangles,0);mesh.RecalculateNormals();mesh.RecalculateBounds();return StoreMesh(name,mesh);
        }
        static void DressCRT(Transform crt,Material cream,Material dark,Material steel,Mesh keys)
        {
            var root=new GameObject("CRT_SilhouetteUpgrade");root.transform.SetParent(crt,false);
            LocalBox(root.transform,"CRT_Brow",new Vector3(0,.93f,-.43f),new Vector3(1.08f,.075f,.13f),dark);
            LocalBox(root.transform,"CRT_BezelTop",new Vector3(0,.88f,-.455f),new Vector3(.94f,.055f,.055f),cream);
            LocalBox(root.transform,"CRT_BezelBottom",new Vector3(0,.315f,-.455f),new Vector3(.94f,.065f,.055f),cream);
            for(int side=-1;side<=1;side+=2)LocalBox(root.transform,"CRT_BezelSide",new Vector3(side*.49f,.60f,-.455f),new Vector3(.06f,.61f,.055f),cream);
            for(int i=0;i<5;i++)LocalBox(root.transform,"CRT_Vent",new Vector3(-.30f+i*.15f,1.015f,-.04f),new Vector3(.085f,.012f,.32f),dark);
            for(int i=0;i<3;i++)LocalBox(root.transform,"CRT_Control",new Vector3(.30f+i*.095f,.26f,-.47f),new Vector3(.06f,.04f,.035f),i==2?steel:dark);
            foreach(var renderer in crt.GetComponentsInChildren<Renderer>())
                if(renderer.name=="Keycap"||renderer.name.Contains("KeyboardBase")||renderer.name.Contains("Key_"))renderer.enabled=false;
            // A supported pull-out tray: the keyboard no longer hangs unsupported off the front of the desk.
            LocalBox(root.transform,"KeyboardTray",new Vector3(0,.025f,-.64f),new Vector3(1.13f,.035f,.53f),steel);
            var keyboard=new GameObject("KeyboardAssembly");keyboard.transform.SetParent(root.transform,false);
            keyboard.transform.localPosition=new Vector3(0,.035f,-.62f);keyboard.transform.localRotation=Quaternion.Euler(-5,0,0);
            LocalBox(keyboard.transform,"KeyboardDeck",new Vector3(0,.067f,-.025f),new Vector3(1.09f,.064f,.46f),cream);
            LocalBox(keyboard.transform,"KeyboardRecess",new Vector3(0,.101f,-.025f),new Vector3(1.035f,.006f,.422f),dark);
            MeshObject(keyboard.transform,"KeyboardKeysCombined",keys,cream,Vector3.zero);
            if(Mathf.Abs(crt.position.x)>.5f)
            {
                var display=crt.Find("StatusDisplay");
                if(display){var driver=display.gameObject.AddComponent<AuxiliaryCRTDisplay>();driver.screen=display.GetComponent<Renderer>();driver.amber=crt.position.x>0;}
            }
        }
        static void DressChair(Transform seat,int index,Transform parent,Material dark,Material steel,Mesh cushion,Mesh back)
        {
            var root=new GameObject($"ChairUpgrade_{index:00}");root.transform.SetParent(parent,false);root.transform.position=seat.position;
            MeshObject(root.transform,"ChairSeatCushion",cushion,dark,new Vector3(0,.045f,0));
            MeshObject(root.transform,"ChairBackPad",back,dark,new Vector3(0,.46f,.32f),Quaternion.Euler(84,0,0));
            for(int side=-1;side<=1;side+=2)
            {
                LocalBox(root.transform,"ChairArmPost",new Vector3(side*.34f,.14f,.02f),new Vector3(.035f,.29f,.035f),steel);
                LocalBox(root.transform,"ChairArmPad",new Vector3(side*.34f,.29f,-.04f),new Vector3(.09f,.05f,.37f),dark);
            }
            for(int i=0;i<4;i++)
            {
                float angle=(45+90*i)*Mathf.Deg2Rad;Vector3 direction=new Vector3(Mathf.Sin(angle),0,Mathf.Cos(angle));
                LocalBox(root.transform,"ChairSpoke",direction*.2f+new Vector3(0,-.435f,0),new Vector3(.05f,.035f,.4f),steel,Quaternion.Euler(0,45+90*i,0));
                LocalBox(root.transform,"ChairCaster",direction*.37f+new Vector3(0,-.465f,0),new Vector3(.08f,.075f,.10f),dark);
            }
        }
        public static void Dress()
        {
            if(GameObject.Find("ControlRoomArt06"))return;
            var root=new GameObject("ControlRoomArt06");
            var cream=Mat("ControlRoomCream06",new Color(.58f,.57f,.47f),.68f);
            var dark=Mat("ControlRoomCharcoal06",new Color(.038f,.048f,.043f),.72f);
            var steel=Mat("ControlRoomSteel06",new Color(.17f,.20f,.18f),.48f);
            var keys=KeyboardMesh();var cushion=Cushion("ChairSeatPad06",new Vector3(.64f,.10f,.62f),.075f);
            var back=Cushion("ChairBackPad06",new Vector3(.62f,.10f,.67f),.07f);
            var transforms=Object.FindObjectsByType<Transform>(FindObjectsSortMode.None);int crts=0,chairs=0;
            foreach(var t in transforms)if(t.name=="SM_CRT_Terminal_A"){DressCRT(t,cream,dark,steel,keys);crts++;}
            foreach(var t in transforms)
            {
                if(t.name=="ChairSeat")DressChair(t,chairs++,root.transform,dark,steel,cushion,back);
                if(t.name=="StatusText"||t.name=="ChairBack"){var r=t.GetComponent<Renderer>();if(r)r.enabled=false;}
                if(t.name=="FixtureLight")
                {
                    var light=t.GetComponent<Light>();if(!light)continue;float x=Mathf.Abs(t.position.x);
                    light.intensity=x>5?.70f:x>2?1.10f:1.55f;light.range=x>5?6:7.5f;
                    light.shadows=LightShadows.None;light.color=new Color(.73f,.82f,.73f);
                }
            }
            RenderSettings.ambientLight=new Color(.18f,.215f,.21f);
            root.AddComponent<Signal47.Debugging.ControlRoomPassSmokeChecks>();
            Debug.Log($"CONTROL_ROOM_PASS_06_DRESSED crt={crts} chairs={chairs} auxiliary=2 keys=52_per_mesh");
        }
    }
}
#endif
