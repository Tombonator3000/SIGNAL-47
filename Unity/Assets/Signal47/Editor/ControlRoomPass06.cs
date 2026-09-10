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
            for(int row=0;row<4;row++)
                LocalBox(root.transform,$"KeyboardRow_{row}",new Vector3(-.08f,.135f,-.92f+row*.105f),new Vector3(.76f,.025f,.065f),cream,Quaternion.Euler(-5,0,0));
            LocalBox(root.transform,"KeyboardFunctionBank",new Vector3(.43f,.135f,-.765f),new Vector3(.18f,.025f,.27f),steel,Quaternion.Euler(-5,0,0));
            LocalBox(root.transform,"KeyboardSpacebar",new Vector3(-.08f,.145f,-.535f),new Vector3(.42f,.026f,.055f),cream,Quaternion.Euler(-5,0,0));

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

            LocalBox(root.transform,"ChairSeatCushion",new Vector3(0,.075f,0),new Vector3(.68f,.12f,.66f),dark);
            LocalBox(root.transform,"ChairBackPad",new Vector3(0,.50f,.36f),new Vector3(.68f,.70f,.13f),dark,Quaternion.Euler(-6,0,0));
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

            RenderSettings.ambientLight=new Color(.205f,.235f,.225f);

            root.AddComponent<Signal47.Debugging.ControlRoomPassSmokeChecks>();
            Debug.Log($"CONTROL_ROOM_PASS_06_DRESSED crt={crtCount} chairs={chairCount}");
        }
    }
}
#endif
