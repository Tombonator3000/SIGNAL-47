#if UNITY_EDITOR
using System;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;
using Signal47.Environment;
using static Signal47.Editor.Signal47SceneBuilder;

namespace Signal47.Editor
{
    /// <summary>
    /// Lightweight world-art pass for the first exterior language of SIGNAL / 47.
    /// It deliberately uses simple Unity primitives and the existing authored dishes so
    /// the project can move toward stylized realism without turning the prologue into a
    /// high-end hardware benchmark.
    /// </summary>
    public static class WorldAreaPass
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
            if(path!=ScenePath || GameObject.Find("WorldAreaArt"))return;
            Dress(UnityEngine.Object.FindFirstObjectByType<DishArrayController>());
        }

        static GameObject LocalBox(Transform parent,string name,Vector3 localPosition,Vector3 scale,Material material,bool collider=false,Quaternion rotation=default)
        {
            var go=Cube(name,Vector3.zero,scale,material,collider);
            go.transform.SetParent(parent,false);
            go.transform.localPosition=localPosition;
            go.transform.localRotation=rotation==default?Quaternion.identity:rotation;
            return go;
        }

        static void LocalLabel(Transform parent,string name,string text,Vector3 localPosition,float size,Color color,Quaternion rotation)
        {
            var go=new GameObject(name);
            go.transform.SetParent(parent,false);
            go.transform.localPosition=localPosition;
            go.transform.localRotation=rotation;
            var mesh=go.AddComponent<TextMesh>();
            mesh.text=text;
            mesh.fontSize=64;
            mesh.characterSize=size/8f;
            mesh.anchor=TextAnchor.MiddleCenter;
            mesh.alignment=TextAlignment.Center;
            mesh.color=color;
        }

        static Light LocalLight(Transform parent,string name,Vector3 localPosition,Color color,float intensity,float range,LightType type=LightType.Point,float spotAngle=70f)
        {
            var go=new GameObject(name);
            go.transform.SetParent(parent,false);
            go.transform.localPosition=localPosition;
            var light=go.AddComponent<Light>();
            light.type=type;
            light.color=color;
            light.intensity=intensity;
            light.range=range;
            light.shadows=LightShadows.None;
            if(type==LightType.Spot){light.spotAngle=spotAngle;light.innerSpotAngle=spotAngle*.55f;go.transform.localRotation=Quaternion.Euler(68,0,0);}
            return light;
        }

        static void RefineDishComposition(Transform world,Material warmGlow,Material redGlow,Material desertDark,Material scrub)
        {
            var arrayRoot=new GameObject("ArrayNightArt");
            arrayRoot.transform.SetParent(world,false);

            // Stronger, cleaner perspective rhythm from the control-room windows.
            // The underlying DishPivot transforms remain untouched, so story movement and tests still use the same objects.
            Vector3[] heroPositions={
                new Vector3(-10,0,-21),new Vector3(-2,0,-27),new Vector3(8,0,-34),new Vector3(18,0,-43),new Vector3(31,0,-55)
            };
            float[] heroScales={1.16f,1.09f,1.02f,.95f,.88f};
            for(int i=0;i<heroPositions.Length;i++)
            {
                var dish=GameObject.Find($"Dish_{i:00}");
                if(!dish)continue;
                dish.transform.position=heroPositions[i];
                dish.transform.localScale=Vector3.one*heroScales[i];
            }

            // Amber maintenance light is the visual counterpoint to the cold star field.
            // Only three shadowless real-time lights are added; the rest is emissive set dressing.
            Vector3[] floodPositions={new Vector3(-10,2.7f,-18),new Vector3(5,2.9f,-30),new Vector3(20,3.0f,-42)};
            for(int i=0;i<floodPositions.Length;i++)
            {
                LocalLight(arrayRoot.transform,$"ArrayFlood_{i}",floodPositions[i],new Color(1f,.31f,.055f),2.0f,20f,LightType.Point);
                LocalBox(arrayRoot.transform,$"ArrayFloodHousing_{i}",floodPositions[i]+new Vector3(0,-2.45f,0),new Vector3(.28f,.16f,.22f),warmGlow);
            }

            // Sparse red obstruction/service beacons; emissive only, no extra light sources.
            int[] beaconDish={0,2,4,6,9};
            for(int i=0;i<beaconDish.Length;i++)
            {
                var dish=GameObject.Find($"Dish_{beaconDish[i]:00}");
                if(!dish)continue;
                var beacon=LocalBox(dish.transform,"ArrayBeacon",new Vector3(0,6.0f,0),new Vector3(.09f,.09f,.09f),redGlow);
                beacon.transform.localScale/=Mathf.Max(.01f,dish.transform.localScale.x);
            }

            // Low-poly desert breakup: broad silhouettes and scrub clusters instead of dense texture detail.
            var scrubRoot=new GameObject("DesertScrubRoot");
            scrubRoot.transform.SetParent(arrayRoot.transform,false);
            var rng=new System.Random(47);
            for(int i=0;i<34;i++)
            {
                float x=-58f+(float)rng.NextDouble()*116f;
                float z=-14f-(float)rng.NextDouble()*92f;
                if(Mathf.Abs(x)<4.7f) x+=Mathf.Sign(x==0?1:x)*7f;
                float h=.10f+(float)rng.NextDouble()*.28f;
                float w=.20f+(float)rng.NextDouble()*.55f;
                LocalBox(scrubRoot.transform,$"Scrub_{i:00}",new Vector3(x,-.06f+h*.5f,z),new Vector3(w,h,w*.72f),scrub,false,Quaternion.Euler(0,(float)rng.NextDouble()*180f,0));
            }
            for(int i=0;i<10;i++)
            {
                float x=-50f+(float)rng.NextDouble()*100f;
                float z=-22f-(float)rng.NextDouble()*78f;
                LocalBox(scrubRoot.transform,$"Rock_{i:00}",new Vector3(x,-.02f,z),new Vector3(.45f+(float)rng.NextDouble()*.9f,.16f+(float)rng.NextDouble()*.32f,.5f+(float)rng.NextDouble()*.8f),desertDark,false,Quaternion.Euler((float)rng.NextDouble()*8f,(float)rng.NextDouble()*180f,(float)rng.NextDouble()*7f));
            }

            // Small service cabinets and cable-trench markers make the array read as infrastructure, not sculpture.
            for(int i=0;i<7;i++)
            {
                float z=-18-i*11.2f;
                LocalBox(arrayRoot.transform,$"ServiceCabinet_{i:00}",new Vector3(-4.6f,.44f,z),new Vector3(.58f,.88f,.34f),desertDark);
                LocalBox(arrayRoot.transform,$"ServiceMarker_{i:00}",new Vector3(4.0f,.09f,z-2.4f),new Vector3(.07f,.18f,.07f),warmGlow);
            }
        }

        static void BuildRoadsideMotelBlockout(Transform world,Material concrete,Material dark,Material cream,Material neonRed,Material neonGreen)
        {
            // This is a compressed spatial/art blockout for a later field location, not a claim that a motel sits beside SARO.
            // It stays behind the current prologue building and outside the playable room, so existing story flow is unchanged.
            var root=new GameObject("RoadsideMotel_Blockout");
            root.transform.SetParent(world,false);
            root.transform.position=new Vector3(112f,0f,52f);
            root.transform.rotation=Quaternion.Euler(0,-18f,0);

            LocalBox(root.transform,"MotelParking",new Vector3(0,-.12f,-6f),new Vector3(48,.14f,25),dark);
            LocalBox(root.transform,"MotelMainWing",new Vector3(1,1.45f,4.8f),new Vector3(42,2.9f,5.5f),cream);
            LocalBox(root.transform,"MotelOffice",new Vector3(-18.5f,1.65f,-.5f),new Vector3(8.5f,3.3f,6.8f),cream);
            LocalBox(root.transform,"MotelWalkway",new Vector3(1,.05f,1.7f),new Vector3(43,.1f,1.55f),concrete);
            LocalBox(root.transform,"OfficeCanopy",new Vector3(-18.5f,2.7f,-4.2f),new Vector3(9,.2f,1.3f),dark);

            for(int i=0;i<10;i++)
            {
                float x=-13.7f+i*3.05f;
                LocalBox(root.transform,$"RoomDoor_{i:00}",new Vector3(x,1.15f,1.98f),new Vector3(1.18f,2.25f,.09f),dark);
                LocalBox(root.transform,$"RoomWindow_{i:00}",new Vector3(x+1.05f,1.42f,1.93f),new Vector3(.66f,.8f,.08f),neonGreen);
                LocalBox(root.transform,$"RoomAC_{i:00}",new Vector3(x+.95f,.46f,1.88f),new Vector3(.7f,.36f,.22f),concrete);
            }

            var sign=new GameObject("MotelNeonSign");
            sign.transform.SetParent(root.transform,false);
            sign.transform.localPosition=new Vector3(-28f,0f,-6.5f);
            LocalBox(sign.transform,"SignPoleA",new Vector3(-.7f,2.7f,0),new Vector3(.13f,5.4f,.13f),dark);
            LocalBox(sign.transform,"SignPoleB",new Vector3(.7f,2.7f,0),new Vector3(.13f,5.4f,.13f),dark);
            LocalBox(sign.transform,"SignTop",new Vector3(0,5.25f,0),new Vector3(4.9f,1.08f,.22f),neonRed);
            LocalBox(sign.transform,"SignMiddle",new Vector3(0,4.05f,0),new Vector3(5.35f,1.05f,.22f),neonGreen);
            LocalBox(sign.transform,"SignVacancy",new Vector3(0,3.08f,0),new Vector3(3.1f,.45f,.22f),neonRed);
            LocalBox(sign.transform,"SignReader",new Vector3(0,2.15f,0),new Vector3(5.0f,1.15f,.2f),cream);
            LocalLabel(sign.transform,"SignLabelTop","SIERRA",new Vector3(0,5.22f,-.125f),.10f,new Color(1f,.82f,.74f),Quaternion.Euler(0,180,0));
            LocalLabel(sign.transform,"SignLabelMiddle","MOTOR COURT",new Vector3(0,4.02f,-.125f),.078f,new Color(.82f,1f,.87f),Quaternion.Euler(0,180,0));
            LocalLabel(sign.transform,"SignLabelVacancy","VACANCY",new Vector3(0,3.06f,-.125f),.048f,new Color(1f,.83f,.73f),Quaternion.Euler(0,180,0));
            LocalLabel(sign.transform,"SignReaderText","KITCHENETTES  •  COLOR TV\nWEEKLY RATES",new Vector3(0,2.15f,-.115f),.037f,new Color(.12f,.15f,.12f),Quaternion.Euler(0,180,0));
            LocalLight(root.transform,"MotelOfficeLight",new Vector3(-18.5f,2.2f,-3.2f),new Color(1f,.67f,.38f),.75f,8f);
        }

        public static void Dress(DishArrayController dishes)
        {
            var world=new GameObject("WorldAreaArt");
            var desert=Mat("DesertNight",new Color(.105f,.095f,.075f),.98f);
            var desertDark=Mat("DesertRock",new Color(.075f,.07f,.06f),.94f);
            var scrub=Mat("DesertScrub",new Color(.12f,.13f,.09f),.93f);
            var concrete=Mat("RoadConcrete",new Color(.19f,.18f,.15f),.90f);
            var road=Mat("AsphaltNight",new Color(.075f,.075f,.067f),.96f);
            var warmGlow=Mat("ArrayAmberGlow",new Color(.34f,.10f,.025f),.45f,true,new Color(1f,.24f,.025f)*2.1f);
            var redGlow=Mat("ArrayRedGlow",new Color(.25f,.015f,.012f),.35f,true,new Color(1f,.025f,.015f)*2.4f);
            var motelCream=Mat("MotelStucco",new Color(.50f,.46f,.37f),.92f);
            var motelDark=Mat("MotelAsphalt",new Color(.055f,.055f,.05f),.98f);
            var motelRed=Mat("MotelNeonRed",new Color(.32f,.025f,.035f),.35f,true,new Color(1f,.04f,.075f)*2.2f);
            var motelGreen=Mat("MotelNeonGreen",new Color(.025f,.27f,.13f),.35f,true,new Color(.04f,1f,.32f)*1.9f);

            var desertObject=GameObject.Find("Desert");if(desertObject)desertObject.GetComponent<Renderer>().sharedMaterial=desert;
            var roadObject=GameObject.Find("ServiceRoad");if(roadObject)roadObject.GetComponent<Renderer>().sharedMaterial=road;
            RenderSettings.fogColor=new Color(.022f,.026f,.032f);
            RenderSettings.fogDensity=.0046f;
            if(RenderSettings.skybox)
            {
                RenderSettings.skybox.SetFloat("_Exposure",.085f);
                RenderSettings.skybox.SetColor("_Tint",new Color(.40f,.47f,.58f));
                EditorUtility.SetDirty(RenderSettings.skybox);
            }

            RefineDishComposition(world.transform,warmGlow,redGlow,desertDark,scrub);
            BuildRoadsideMotelBlockout(world.transform,concrete,motelDark,motelCream,motelRed,motelGreen);

            // One compact root makes it easy to disable/rebuild this pass during comparison.
            if(dishes)EditorUtility.SetDirty(dishes);
        }
    }
}
#endif
