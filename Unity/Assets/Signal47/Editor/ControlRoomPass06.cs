#if UNITY_EDITOR
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;
using static Signal47.Editor.Signal47SceneBuilder;

namespace Signal47.Editor
{
    /// <summary>
    /// Bounded control-room silhouette pass. Adds only non-colliding cosmetic geometry
    /// around the existing CRTs/keyboards/chairs so gameplay and interaction transforms stay intact.
    /// </summary>
    public static class ControlRoomPass06
    {
        const string ScenePath="Assets/Signal47/Scenes/Prototype/SARO_Prologue.unity";

        [InitializeOnLoadMethod]
        static void Register()
        {
            EditorSceneManager.sceneSaving-=OnSceneSaving;
            EditorSceneManager.sceneSaving+=OnSceneSaving;
        }

        static void OnSceneSaving(Scene scene,string path)
        {
            if(path!=ScenePath || GameObject.Find("ControlRoomArt06"))return;
            Dress();
        }

        static GameObject LocalBox(Transform parent,string name,Vector3 localPosition,Vector3 scale,Material material,Quaternion rotation=default)
        {
            var go=Cube(name,Vector3.zero,scale,material,false);
            go.transform.SetParent(parent,false);
            go.transform.localPosition=localPosition;
            go.transform.localRotation=rotation==default?Quaternion.identity:rotation;
            GameObjectUtility.SetStaticEditorFlags(go,StaticEditorFlags.BatchingStatic);
            return go;
        }

        static void DressCRT(Transform crt,Material cream,Material dark,Material steel)
        {
            var root=new GameObject("CRT_SilhouetteUpgrade");
            root.transform.SetParent(crt,false);

            LocalBox(root.transform,"CRT_Brow",new Vector3(0,.93f,-.43f),new Vector3(1.08f,.075f,.13f),dark);
            LocalBox(root.transform,"CRT_BezelTop",new Vector3(0,.88f,-.455f),new Vector3(.94f,.055f,.055f),cream);
            LocalBox(root.transform,"CRT_BezelBottom",new Vector3(0,.315f,-.455f),new Vector3(.94f,.065f,.055f),cream);
            LocalBox(root.transform,"CRT_BezelLeft",new Vector3(-.49f,.60f,-.455f),new Vector3(.06f,.61f,.055f),cream);
            LocalBox(root.transform,"CRT_BezelRight",new Vector3(.49f,.60f,-.455f),new Vector3(.06f,.61f,.055f),cream);

            for(int i=0;i<5;i++)
                LocalBox(root.transform,$"CRT_Vent_{i}",new Vector3(-.30f+i*.15f,1.015f,-.04f),new Vector3(.085f,.012f,.32f),dark);

            for(int i=0;i<3;i++)
                LocalBox(root.transform,$"CRT_Control_{i}",new Vector3(.30f+i*.095f,.26f,-.47f),new Vector3(.06f,.04f,.035f),i==2?steel:dark);

            LocalBox(root.transform,"KeyboardDeck",new Vector3(0,.095f,-.79f),new Vector3(1.12f,.055f,.52f),dark,Quaternion.Euler(-5,0,0));
            for(int row=0;row<4;row++)for(int col=0;col<11;col++)
                LocalBox(root.transform,$"KeyboardKey_{row}_{col}",new Vector3(-.46f+col*.073f+row*.007f,.145f,-.90f+row*.081f),new Vector3(.060f,.032f,.062f),cream);
            for(int row=0;row<4;row++)for(int col=0;col<2;col++)
                LocalBox(root.transform,$"KeyboardFunction_{row}_{col}",new Vector3(.40f+col*.075f,.145f,-.90f+row*.081f),new Vector3(.058f,.032f,.062f),steel);
            LocalBox(root.transform,"KeyboardSpacebar",new Vector3(-.08f,.145f,-.99f),new Vector3(.42f,.032f,.055f),cream);

            foreach(var child in crt.GetComponentsInChildren<Transform>(true))
                if(child.name=="Keycap")
                {
                    var r=child.GetComponent<Renderer>();
                    if(r)r.enabled=false;
                }
        }

        static void DressChair(Transform seat,int index,Material dark,Material steel)
        {
            var root=new GameObject($"ChairUpgrade_{index:00}");
            root.transform.position=seat.position;

            var fabric=Mat("ChairFabric06",new Color(.10f,.13f,.135f),.92f);
            LocalBox(root.transform,"ChairSeatCushion",new Vector3(0,.075f,0),new Vector3(.64f,.12f,.62f),fabric);
            LocalBox(root.transform,"ChairBackPad",new Vector3(0,.50f,.36f),new Vector3(.62f,.66f,.11f),fabric,Quaternion.Euler(-6,0,0));
            LocalBox(root.transform,"ChairBackSeam",new Vector3(0,.32f,.285f),new Vector3(.57f,.012f,.012f),dark);
            for(int side=-1;side<=1;side+=2)
            {
                LocalBox(root.transform,$"ChairArmPost_{side}",new Vector3(side*.38f,.30f,.02f),new Vector3(.045f,.42f,.045f),steel);
                LocalBox(root.transform,$"ChairArmPad_{side}",new Vector3(side*.38f,.50f,-.03f),new Vector3(.11f,.065f,.43f),dark);
            }
            for(int spoke=0;spoke<4;spoke++)
            {
                float yaw=45f+spoke*45f;
                LocalBox(root.transform,$"ChairSpoke_{spoke}",new Vector3(0,-.43f,0),new Vector3(.72f,.045f,.055f),steel,Quaternion.Euler(0,yaw,0));
            }
            Vector3[] caster={
                new Vector3(-.32f,-.46f,-.32f),
                new Vector3(.32f,-.46f,-.32f),
                new Vector3(-.32f,-.46f,.32f),
                new Vector3(.32f,-.46f,.32f)
            };
            for(int i=0;i<caster.Length;i++)LocalBox(root.transform,$"ChairCaster_{i}",caster[i],new Vector3(.10f,.075f,.08f),dark);
        }

        public static void Dress()
        {
            var root=new GameObject("ControlRoomArt06");
            var cream=Mat("ControlRoomCream06",new Color(.58f,.57f,.47f),.68f);
            var dark=Mat("ControlRoomCharcoal06",new Color(.038f,.048f,.043f),.72f);
            var steel=Mat("ControlRoomSteel06",new Color(.17f,.20f,.18f),.48f);

            int crtCount=0;
            foreach(var t in Object.FindObjectsByType<Transform>(FindObjectsSortMode.None))
                if(t.name=="SM_CRT_Terminal_A") { DressCRT(t,cream,dark,steel); crtCount++; }

            int chairCount=0;
            foreach(var t in Object.FindObjectsByType<Transform>(FindObjectsSortMode.None))
                if(t.name=="ChairSeat") DressChair(t,chairCount++,dark,steel);

            DressLightingAndScreens(root.transform);

            root.AddComponent<Signal47.Debugging.ControlRoomPassSmokeChecks>();
            Debug.Log($"CONTROL_ROOM_PASS_06_DRESSED crt={crtCount} chairs={chairCount}");
        }

        static void DressLightingAndScreens(Transform root)
        {
            RenderSettings.ambientLight=new Color(.16f,.19f,.20f);
            foreach(var light in Object.FindObjectsByType<Light>(FindObjectsSortMode.None))
                if(light.name=="FixtureLight")
                {
                    light.intensity=Mathf.Abs(light.transform.position.x)<1f?.45f:.20f;
                    light.range=6f;
                }
            var diffuser=Mat("TubeDiffuser06",new Color(.32f,.40f,.37f),.9f,true,new Color(.25f,.38f,.31f));
            foreach(var renderer in Object.FindObjectsByType<MeshRenderer>(FindObjectsSortMode.None))
                if(renderer.name=="Diffuser")renderer.sharedMaterial=diffuser;

            for(int i=-1;i<=1;i+=2)
            {
                var go=new GameObject(i<0?"AmberWorkLight06":"CoolWorkLight06");go.transform.SetParent(root,false);
                go.transform.position=new Vector3(i*1.7f,2.55f,-1.55f);
                go.transform.rotation=Quaternion.Euler(90,0,0);
                var light=go.AddComponent<Light>();light.type=LightType.Spot;light.range=3.8f;
                light.intensity=2.2f;light.spotAngle=88;light.innerSpotAngle=55;light.shadows=LightShadows.None;
                light.color=i<0?new Color(1f,.63f,.29f):new Color(.48f,.72f,1f);
                LocalBox(root,"WorkLightHousing",go.transform.position+Vector3.up*.10f,new Vector3(.48f,.08f,.22f),Mat("ControlRoomCharcoal06",Color.gray,.7f));
                LocalBox(root,"WorkLightSuspension",new Vector3(i*1.7f,3.03f,-1.55f),new Vector3(.025f,.73f,.025f),Mat("ControlRoomSteel06",Color.gray,.7f));
            }
            foreach(var label in Object.FindObjectsByType<TextMesh>(FindObjectsSortMode.None))
                if(label.name=="StatusText")
                {
                    bool amber=label.transform.position.x<0;
                    label.text=amber?"ARRAY 03\nTRACK NOMINAL\n\n042 / ORION":"SARO NETWORK\n\nSTANDBY\n23:41 LOCAL";
                    label.color=amber?new Color(1f,.70f,.28f):new Color(.36f,.60f,.72f);
                }
            foreach(var renderer in Object.FindObjectsByType<MeshRenderer>(FindObjectsSortMode.None))
                if(renderer.name=="StatusDisplay")
                {
                    bool amber=renderer.transform.position.x<0;
                    string path=$"Assets/Signal47/Art/PrototypePort/{(amber?"AmberDisplay06":"StandbyDisplay06")}.mat";
                    var mat=AssetDatabase.LoadAssetAtPath<Material>(path);
                    if(!mat){mat=new Material(Shader.Find("Universal Render Pipeline/Unlit"));AssetDatabase.CreateAsset(mat,path);}
                    mat.SetColor("_BaseColor",amber?new Color(.035f,.020f,.007f):new Color(.006f,.012f,.020f));
                    EditorUtility.SetDirty(mat);renderer.sharedMaterial=mat;
                }
        }
    }
}
#endif
