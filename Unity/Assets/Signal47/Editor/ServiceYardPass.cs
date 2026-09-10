#if UNITY_EDITOR
using UnityEditor;
using UnityEngine;
using Signal47.Core;
using Signal47.Interaction;
using Signal47.Investigation;
using static Signal47.Editor.Signal47SceneBuilder;

namespace Signal47.Editor
{
    public static class ServiceYardPass
    {
        static GameObject Box(Transform root,string name,Vector3 position,Vector3 size,Material material,bool collision=false)
        {
            var go=Cube(name,position,size,material,collision);go.transform.SetParent(root,true);return go;
        }
        static void Label(Transform parent,string name,string words,Vector3 position,Vector2 size)
        {
            var go=new GameObject(name);go.transform.SetParent(parent,true);go.transform.position=position;go.transform.rotation=Quaternion.Euler(0,90,0);
            var text=go.AddComponent<TextMesh>();text.text=words;text.anchor=TextAnchor.MiddleCenter;text.alignment=TextAlignment.Center;text.fontSize=64;text.characterSize=.03f;text.color=new Color(.86f,.86f,.68f);
            var font=AssetDatabase.LoadAssetAtPath<Font>("Assets/Signal47/Art/ThirdParty/VT323/VT323-Regular.ttf");
            text.font=font;go.GetComponent<Renderer>().sharedMaterial=font.material;
            var bounds=go.GetComponent<Renderer>().localBounds.size;
            if(bounds.x>.001f&&bounds.y>.001f)text.characterSize*=Mathf.Min(size.x/bounds.x,size.y/bounds.y);
        }
        public static void Dress(GameSession session)
        {
            if(GameObject.Find("ServiceYard07"))return;
            var root=new GameObject("ServiceYard07").transform;
            var wall=Mat("MAT_LowerWall",new Color(.20f,.28f,.27f),.72f);
            var steel=Mat("ControlRoomSteel06",new Color(.17f,.20f,.18f),.48f);
            var dark=Mat("ControlRoomCharcoal06",new Color(.038f,.048f,.043f),.72f);
            var concrete=Mat("ServiceWalkConcrete07",new Color(.24f,.28f,.29f),.95f);
            var paint=Mat("ServicePaint07",new Color(.27f,.34f,.31f),.8f);
            var glow=Mat("ServiceLamp07",new Color(.9f,.62f,.28f),.7f,true,new Color(1f,.52f,.14f)*1.2f);
            var oldWall=GameObject.Find("RightWall");oldWall.GetComponent<Renderer>().enabled=false;oldWall.GetComponent<Collider>().enabled=false;
            Box(root,"EastWallSouth",new Vector3(9.35f,1.725f,-2.65f),new Vector3(.12f,3.45f,9.7f),wall,true);
            Box(root,"EastWallNorth",new Vector3(9.35f,1.725f,5.85f),new Vector3(.12f,3.45f,3.3f),wall,true);
            Box(root,"ServiceDoorLintel",new Vector3(9.35f,2.88f,3.2f),new Vector3(.12f,1.14f,2),wall,true);
            Box(root,"ServiceThreshold",new Vector3(9.4f,0,3.2f),new Vector3(.55f,.16f,1.95f),steel,true);
            Box(root,"EastServiceWalk",new Vector3(10.75f,0,-2.5f),new Vector3(2.5f,.16f,13.5f),concrete,true);
            Box(root,"MotorCabinetApron",new Vector3(10.75f,0,-12),new Vector3(2.5f,.16f,6),concrete,true);
            for(int i=0;i<6;i++)
                Box(root,"ServiceRailPost",new Vector3(12,.59f,3.7f-i*2.5f),new Vector3(.07f,1.02f,.07f),steel,true);
            Box(root,"ServiceHandrail",new Vector3(12,1.08f,-2.55f),new Vector3(.07f,.07f,12.5f),steel,true);
            Box(root,"ServiceEndRail",new Vector3(10.75f,.7f,4.25f),new Vector3(2.5f,.10f,.08f),steel,true);

            // Split existing trim at the doorway; retain the original objects disabled.
            foreach(var trim in Object.FindObjectsByType<Transform>(FindObjectsSortMode.None))
            {
                if((trim.name!="WallRail" && trim.name!="Skirting") || trim.position.x<9)continue;
                var renderer=trim.GetComponent<Renderer>();renderer.enabled=false;var collider=trim.GetComponent<Collider>();if(collider)collider.enabled=false;
                var size=trim.localScale;var y=trim.position.y;
                Box(root,trim.name+"South",new Vector3(trim.position.x,y,-2.65f),new Vector3(size.x,size.y,9.7f),renderer.sharedMaterial);
                Box(root,trim.name+"North",new Vector3(trim.position.x,y,5.85f),new Vector3(size.x,size.y,3.3f),renderer.sharedMaterial);
            }

            var investigation=session.gameObject.AddComponent<ServiceYardInvestigation>();session.yard=investigation;
            var door=new GameObject("ServiceDoor");door.transform.SetParent(root,true);door.transform.position=new Vector3(9.35f,0,4.15f);
            Box(door.transform,"ServiceDoorPanel",new Vector3(9.35f,1.12f,3.2f),new Vector3(.12f,2.2f,1.9f),paint,true);
            Box(door.transform,"ServiceDoorHandle",new Vector3(9.25f,1.0f,2.51f),new Vector3(.07f,.08f,.27f),steel);
            Box(door.transform,"ServiceDoorSign",new Vector3(9.278f,1.69f,3.2f),new Vector3(.02f,.33f,1.22f),dark);
            Label(door.transform,"ServiceDoorText","SERVICE YARD\nAUTHORIZED PERSONNEL",new Vector3(9.261f,1.69f,3.2f),new Vector2(1.05f,.24f));
            var access=door.AddComponent<ServiceYardDoor>();access.leaf=door.transform;access.investigation=investigation;investigation.door=access;

            float[] positions={2f,-4.5f,-11.7f};
            foreach(float z in positions)
            {
                Box(root,"ServiceLampPole",new Vector3(11.92f,1.7f,z),new Vector3(.075f,3.24f,.075f),steel);
                Box(root,"ServiceLampArm",new Vector3(11.52f,3.24f,z),new Vector3(.88f,.07f,.07f),steel);
                Box(root,"ServiceLampHousing",new Vector3(11.1f,3.19f,z),new Vector3(.37f,.14f,.28f),dark);
                Box(root,"ServiceLampDiffuser",new Vector3(11.1f,3.105f,z),new Vector3(.29f,.025f,.21f),glow);
                var lamp=new GameObject("ServicePathLight");lamp.transform.SetParent(root,true);lamp.transform.position=new Vector3(11.1f,3.07f,z);lamp.transform.rotation=Quaternion.Euler(90,0,0);
                var light=lamp.AddComponent<Light>();light.type=LightType.Spot;light.color=new Color(1f,.68f,.36f);light.intensity=4;light.range=7;light.spotAngle=110;light.innerSpotAngle=80;light.shadows=LightShadows.None;
            }

            var cabinet=new GameObject("MotorBusS03");cabinet.transform.SetParent(root,true);cabinet.transform.position=new Vector3(12.1f,0,-13.2f);
            Box(cabinet.transform,"MotorCabinetBody",new Vector3(12.1f,.89f,-13.2f),new Vector3(.6f,1.62f,.92f),paint,true);
            Box(cabinet.transform,"MotorCabinetPanel",new Vector3(11.786f,1.19f,-13.2f),new Vector3(.024f,.47f,.77f),dark);
            Label(cabinet.transform,"MotorCabinetText","S-03\nMOTOR BUS",new Vector3(11.766f,1.19f,-13.2f),new Vector2(.63f,.36f));
            Box(cabinet.transform,"MotorCabinetLatch",new Vector3(11.762f,.64f,-12.87f),new Vector3(.035f,.22f,.065f),steel);
            cabinet.AddComponent<ServiceCabinet>().investigation=investigation;
            Debug.Log("SERVICE_YARD_07_BUILT controlled doorway, walkable service path, S-03 cabinet");
        }
    }
}
#endif
