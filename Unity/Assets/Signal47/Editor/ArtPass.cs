#if UNITY_EDITOR
using System.IO;
using UnityEditor;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;
using static Signal47.Editor.Signal47SceneBuilder;
namespace Signal47.Editor
{
    public static class ArtPass
    {
        static Material cream,steel,dark,paper,glow;
        static Material M(string n,Color c,float rough=.7f)=>Mat(n,c,rough);
        public static Material PropMaterial(string n,Material fallback)
        {
            if(n.Contains("PhoneCream")||n.Contains("PhoneHandsetCream"))return M("PhoneCream",new Color(.62f,.59f,.49f),.55f);
            if(n.Contains("PhoneLegend"))return M("PhoneLegend",new Color(.86f,.85f,.71f),.8f);
            if(n.Contains("PhoneDark"))return M("Charcoal",new Color(.06f,.075f,.07f));
            if(n.Contains("Screen"))return M("DisplayBorder",new Color(.025f,.035f,.031f));
            if(n.Contains("Key_")||n.Contains("Button_")||n.Contains("Knob")||n.Contains("Handset")||n.Contains("Cable")||n.Contains("Perforation"))return M("Charcoal",new Color(.06f,.075f,.07f));
            if(n.Contains("Paper")||n.Contains("Meter"))return M("OldPaper",new Color(.84f,.81f,.65f));
            if(n.Contains("LED"))return Mat("ReceiverLEDOff",new Color(.04f,.15f,.08f),.3f,true,new Color(.015f,.05f,.02f));
            if(n.Contains("Module"))return M("RackPanel",new Color(.49f,.52f,.47f),.45f);
            if(n.Contains("Leg")||n.Contains("FeedSupport")||n.Contains("Azimuth"))return M("SteelFrame",new Color(.12f,.17f,.16f),.35f);
            if(n.Contains("DishBowl")){var m=M("DishEnamel",new Color(.57f,.65f,.68f),.8f);m.SetFloat("_Cull",0);return m;}
            return fallback;
        }
        static GameObject Box(string n,Vector3 p,Vector3 s,Material m,bool coll=false)=>Cube(n,p,s,m,coll);
        static void Label(string name,string text,Vector3 p,float size,Color color,Quaternion rotation)
        {
            var go=new GameObject(name);go.transform.position=p;go.transform.rotation=rotation;
            var t=go.AddComponent<TextMesh>();t.text=text;t.fontSize=64;t.characterSize=size;t.anchor=TextAnchor.MiddleCenter;t.alignment=TextAlignment.Center;t.color=color;
            var font=AssetDatabase.LoadAssetAtPath<Font>("Assets/Signal47/Art/ThirdParty/VT323/VT323-Regular.ttf");
            if(font){t.font=font;go.GetComponent<Renderer>().sharedMaterial=font.material;}
            Vector2 box=name=="StatusText"?new Vector2(.70f,.37f):name=="ConsoleLabel"?new Vector2(.86f,.06f):name=="RXLabel"?new Vector2(1.4f,.13f):name=="SAROLabel"?new Vector2(2.8f,.43f):new Vector2(.52f,.22f);
            var bounds=go.GetComponent<Renderer>().localBounds.size;
            if(bounds.x>.001f&&bounds.y>.001f)t.characterSize*=Mathf.Min(box.x/bounds.x,box.y/bounds.y);
            Debug.Log($"LABEL_FIT {name} size={t.characterSize} bounds={go.GetComponent<Renderer>().localBounds.size}");
        }
        static void FloorMaterial()
        {
            string path="Assets/Signal47/Art/Refined/Linoleum.png";
            if(!File.Exists(path)){
                var t=new Texture2D(256,256);var rng=new System.Random(47);var pixels=new Color[256*256];
                for(int y=0;y<256;y++)for(int x=0;x<256;x++){
                    float grain=(float)rng.NextDouble()*.065f;bool seam=x<2||y<2;float v=seam?.23f:.57f+grain;
                    pixels[y*256+x]=new Color(v*.92f,v,v*.96f);
                }
                t.SetPixels(pixels);t.Apply();File.WriteAllBytes(path,t.EncodeToPNG());Object.DestroyImmediate(t);AssetDatabase.ImportAsset(path);
            }
            var floor=GameObject.Find("Floor").GetComponent<Renderer>();var mat=M("TiledLinoleum",Color.white,.82f);mat.SetTexture("_BaseMap",AssetDatabase.LoadAssetAtPath<Texture2D>(path));mat.SetTextureScale("_BaseMap",new Vector2(23,19));floor.sharedMaterial=mat;
        }
        public static void Dress()
        {
            cream=M("InstitutionalCream",new Color(.65f,.65f,.53f));steel=M("SteelFrame",new Color(.12f,.17f,.16f),.35f);dark=M("Charcoal",new Color(.06f,.075f,.07f));paper=M("OldPaper",new Color(.84f,.81f,.65f));glow=Mat("TubeDiffuser",new Color(.62f,.73f,.65f),.9f,true,new Color(.58f,.72f,.63f)*1.3f);
            FloorMaterial();
            // Suspended ceiling grid and lower wall rails ground the large room in human scale.
            for(float x=-9;x<=9;x+=1.5f)Box("CeilingT",new Vector3(x,3.38f,0),new Vector3(.018f,.015f,15),steel);
            for(float z=-7.5f;z<=7.5f;z+=1.5f)Box("CeilingT",new Vector3(0,3.38f,z),new Vector3(18.7f,.015f,.018f),steel);
            for(int side=-1;side<=1;side+=2){Box("WallRail",new Vector3(side*9.27f,1.12f,0),new Vector3(.07f,.075f,15),cream);Box("Skirting",new Vector3(side*9.27f,.075f,0),new Vector3(.08f,.15f,15),dark);}
            for(int i=-2;i<=2;i++){
                var old=GameObject.Find("Fluorescent_"+i);old.GetComponent<Renderer>().sharedMaterial=steel;
                Box("Diffuser",new Vector3(i*3.1f,3.265f,-.3f),new Vector3(1.62f,.024f,.16f),glow);
                old.GetComponent<Light>().enabled=false;var fixtureLight=new GameObject("FixtureLight").AddComponent<Light>();fixtureLight.transform.position=new Vector3(i*3.1f,2.75f,-.3f);fixtureLight.type=LightType.Point;fixtureLight.range=9;fixtureLight.intensity=2.0f;fixtureLight.color=new Color(.73f,.84f,.74f);
            }
            var desk=GameObject.Find("SM_ControlDesk_A");
            Box("DesktopInlay",new Vector3(0,.824f,-2.25f),new Vector3(6.75f,.018f,1.09f),cream);
            for(int i=-1;i<=1;i+=2){
                var monitor=Prefab("SM_CRT_Terminal_A",new Vector3(i*1.65f,.84f,-2.32f),Quaternion.Euler(0,180,0),Vector3.one,cream);
                var display=GameObject.CreatePrimitive(PrimitiveType.Quad);display.name="StatusDisplay";Object.DestroyImmediate(display.GetComponent<Collider>());display.transform.SetParent(monitor.transform,false);display.transform.localPosition=new Vector3(0,.60f,-.435f);display.transform.localScale=new Vector3(.82f,.49f,1);var sm=AssetDatabase.LoadAssetAtPath<Material>("Assets/Signal47/Art/Refined/StandbyScreen.mat");if(!sm){sm=new Material(Shader.Find("Universal Render Pipeline/Unlit"));sm.SetColor("_BaseColor",new Color(.008f,.022f,.014f));AssetDatabase.CreateAsset(sm,"Assets/Signal47/Art/Refined/StandbyScreen.mat");}display.GetComponent<Renderer>().sharedMaterial=sm;
                Label("StatusText",i<0?"ARRAY 03\nTRACK NOMINAL\n\n042 / ORION":"SARO NETWORK\n\nNO MESSAGES\n23:41 LOCAL",new Vector3(i*1.65f,1.44f,-1.875f),.037f,new Color(.48f,.88f,.62f),Quaternion.Euler(0,180,0));
            }
            foreach(var monitor in Object.FindObjectsByType<Transform>(FindObjectsSortMode.None)){
                if(monitor.name!="SM_CRT_Terminal_A")continue;
                foreach(var part in monitor.GetComponentsInChildren<Renderer>()){
                    if(part.name.Contains("KeyboardBase"))part.transform.localPosition+=new Vector3(0,0,-.45f);
                    if(part.name.Contains("Key_"))part.enabled=false;
                }
                for(int row=0;row<4;row++)for(int col=0;col<10;col++){
                    var key=Box("Keycap",Vector3.zero,new Vector3(.075f,.025f,.06f),cream);
                    key.transform.SetParent(monitor,false);key.transform.localPosition=new Vector3(-.45f+col*.1f,.145f,-.87f+row*.085f);
                }
            }
            // Label equipment at the object instead of adding quest arrows.
            Label("RXLabel","RECEIVER BANK 03",new Vector3(-6.3f,2.48f,2.02f),.065f,new Color(.88f,.91f,.75f),Quaternion.Euler(0,180,0));
            Label("SAROLabel","S I E R R A   A R R A Y\nRADIO OBSERVATORY",new Vector3(4.8f,2.45f,7.39f),.08f,new Color(.73f,.81f,.72f),Quaternion.identity);
            Label("ConsoleLabel","RX 03    /    SIGNAL ANALYSIS",new Vector3(0,1.76f,-1.805f),.025f,new Color(.8f,.8f,.65f),Quaternion.Euler(0,180,0));
            // Practical chairs, drawer pedestals, binders and night-shift documents.
            for(int i=-1;i<=1;i++){
                float x=i==0?.8f:i*1.8f;
                Box("ChairSeat",new Vector3(x,.52f,-.6f),new Vector3(.62f,.13f,.62f),dark,true);
                Box("ChairBack",new Vector3(x,.97f,-.25f),new Vector3(.62f,.72f,.11f),dark,true);
                Box("ChairPost",new Vector3(x,.27f,-.6f),new Vector3(.075f,.4f,.075f),steel);
                Box("ChairBase",new Vector3(x,.065f,-.6f),new Vector3(.64f,.065f,.075f),steel);
                Box("ChairBase",new Vector3(x,.065f,-.6f),new Vector3(.075f,.065f,.64f),steel);
            }
            for(int i=0;i<3;i++)for(int j=0;j<4;j++){
                Vector3 p=new Vector3(-8.45f+i*.78f,.28f+j*.47f,-4.395f);
                Box("Drawer",p,new Vector3(.60f,.43f,.018f),cream);
                Box("DrawerHandle",p+new Vector3(0,.08f,.04f),new Vector3(.21f,.032f,.065f),steel);
                Box("FileLabel",p+new Vector3(0,-.06f,.012f),new Vector3(.17f,.07f,.005f),paper);
            }
            // Put the clipboard on a real work surface.
            Box("ShiftLogDesk",new Vector3(-1.7f,.78f,5.2f),new Vector3(1.5f,.12f,1.3f),cream,true);
            for(int i=-1;i<=1;i+=2)Box("ShiftLogLeg",new Vector3(-1.7f+i*.6f,.38f,5.2f),new Vector3(.07f,.76f,1.05f),steel,true);
            GameObject.Find("ShiftClipboard").transform.position=new Vector3(-1.7f,.88f,5.2f);
            Label("ShiftLogLabel","NIGHT SHIFT\nOPERATIONS LOG",new Vector3(-1.7f,.908f,5.2f),.053f,new Color(.17f,.2f,.16f),Quaternion.Euler(90,0,0));
            for(int i=0;i<5;i++)Box("TractorPaper",new Vector3(2.2f,.85f+i*.005f,-2f),new Vector3(.45f,.005f,.52f),paper);
            // A hooded desk lamp instead of an unexplained orange point light.
            Box("LampFoot",new Vector3(5.85f,1.24f,.5f),new Vector3(.28f,.05f,.24f),dark);
            Box("LampStem",new Vector3(5.85f,1.55f,.5f),new Vector3(.04f,.6f,.04f),steel);
            Box("LampShade",new Vector3(5.71f,1.89f,.5f),new Vector3(.4f,.08f,.24f),dark);
            GameObject.Find("DeskLamp").transform.position=new Vector3(5.65f,1.79f,.5f);
            // Soft room fill; the exterior retains a cooler value range.
            RenderSettings.ambientLight=new Color(.27f,.31f,.33f);
            GameObject.Find("Moonlight").GetComponent<Light>().intensity=.8f;
            for(int i=0;i<18;i++){
                var p=new Vector3((i%2==0?-1:1)*2.8f,.12f,-10-i*4);
                Box("RoadReflector",p,new Vector3(.08f,.14f,.08f),glow);
            }
            EditorUtility.SetDirty(GameObject.Find("Floor").GetComponent<Renderer>().sharedMaterial);
        }
    }
}
#endif
