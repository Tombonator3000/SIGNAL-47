#if UNITY_EDITOR
using System.IO;
using UnityEditor;
using UnityEngine;
using Signal47.Core;
using Signal47.Investigation;
using Signal47.Interaction;
using static Signal47.Editor.Signal47SceneBuilder;
namespace Signal47.Editor
{
    public static class FieldCameraPass
    {
        const string Folder="Assets/Signal47/Art/ThirdParty/PolyHaven/Camera_01/";
        static Texture2D Map(string file,bool normal=false,bool linear=false)
        {
            string path=Folder+file;var importer=(TextureImporter)AssetImporter.GetAtPath(path);
            var type=normal?TextureImporterType.NormalMap:TextureImporterType.Default;
            if(importer.textureType!=type || importer.sRGBTexture==linear || importer.maxTextureSize!=1024)
            {importer.textureType=type;importer.sRGBTexture=!linear;importer.maxTextureSize=1024;importer.SaveAndReimport();}
            return AssetDatabase.LoadAssetAtPath<Texture2D>(path);
        }
        static Material Material(string part)
        {
            string path=Folder+"Camera08_"+part+".mat";var m=AssetDatabase.LoadAssetAtPath<Material>(path);
            if(!m){m=new Material(Shader.Find("Universal Render Pipeline/Lit"));AssetDatabase.CreateAsset(m,path);}
            if(part=="lens"){m.SetColor("_BaseColor",new Color(.07f,.13f,.16f));m.SetFloat("_Metallic",.7f);m.SetFloat("_Smoothness",.87f);}
            else
            {
                m.SetTexture("_BaseMap",Map(part+"_diff.jpg"));m.SetColor("_BaseColor",Color.white);
                m.SetTexture("_BumpMap",Map(part+"_nor_gl.png",true,true));m.EnableKeyword("_NORMALMAP");m.SetFloat("_BumpScale",.7f);
                m.SetTexture("_MetallicGlossMap",Map(part+"_MetalSmooth.png",false,true));m.EnableKeyword("_METALLICSPECGLOSSMAP");m.SetFloat("_Smoothness",.65f);
            }
            EditorUtility.SetDirty(m);return m;
        }
        public static void Dress(GameSession session)
        {
            if(GameObject.Find("FieldKit08"))return;
            session.fieldCamera=session.gameObject.AddComponent<FieldCamera>();
            session.fieldCamera.shutter=AssetDatabase.LoadAssetAtPath<AudioClip>("Assets/Signal47/Art/ThirdParty/PreparedAudio/CameraShutter.wav");
            var root=new GameObject("FieldKit08");
            var paint=Mat("FieldKit08Paint",new Color(.16f,.22f,.19f),.8f);
            var dark=Mat("FieldKit08Rubber",new Color(.025f,.031f,.027f),.8f);
            var shelf=Cube("FieldKitPedestal",new Vector3(8.25f,.44f,5.0f),new Vector3(.75f,.88f,.55f),paint,true);shelf.transform.SetParent(root.transform,true);
            var mat=Cube("CameraRest",new Vector3(8.25f,.895f,5.0f),new Vector3(.70f,.025f,.50f),dark,false);mat.transform.SetParent(root.transform,true);
            var model=AssetDatabase.LoadAssetAtPath<GameObject>(Folder+"FieldCamera08.fbx");if(!model)throw new FileNotFoundException("FieldCamera08.fbx");
            var camera=new GameObject("FieldCameraPickup08");var imported=(GameObject)PrefabUtility.InstantiatePrefab(model);imported.transform.SetParent(camera.transform,false);camera.transform.SetParent(root.transform,true);camera.transform.rotation=Quaternion.Euler(0,175,0);camera.transform.localScale=Vector3.one*1.35f;
            var bounds=new Bounds();bool first=true;
            foreach(var r in camera.GetComponentsInChildren<Renderer>())
            {
                var materials=r.sharedMaterials;for(int i=0;i<materials.Length;i++)materials[i]=Material(materials[i].name.Replace("Camera_01_",""));r.sharedMaterials=materials;
                if(first){bounds=r.bounds;first=false;}else bounds.Encapsulate(r.bounds);
            }
            camera.transform.position+=new Vector3(8.25f,.913f,4.95f)-new Vector3(bounds.center.x,bounds.min.y,bounds.center.z);
            var box=camera.AddComponent<BoxCollider>();bounds=camera.GetComponentInChildren<Renderer>().bounds;box.center=camera.transform.InverseTransformPoint(bounds.center);box.size=new Vector3(.19f,.11f,.11f);
            camera.AddComponent<CameraPickup>();
            var label=new GameObject("FieldKitInstructions");label.transform.SetParent(root.transform,true);label.transform.position=new Vector3(8.25f,.64f,4.717f);label.transform.rotation=Quaternion.identity;
            var text=label.AddComponent<TextMesh>();text.text="FIELD CAMERA\nC / VIEWFINDER\nSPACE / SHUTTER";text.font=session.hud.terminalFont;text.fontSize=64;text.anchor=TextAnchor.MiddleCenter;text.alignment=TextAlignment.Center;text.characterSize=.012f;text.color=new Color(.85f,.84f,.65f);label.GetComponent<Renderer>().sharedMaterial=text.font.material;var labelSize=label.GetComponent<Renderer>().localBounds.size;text.characterSize*=Mathf.Min(.57f/labelSize.x,.22f/labelSize.y);
            var lamp=new GameObject("FieldKitLight08");lamp.transform.SetParent(root.transform,true);lamp.transform.position=new Vector3(8.1f,2.35f,4.55f);lamp.transform.rotation=Quaternion.Euler(75,0,0);
            var light=lamp.AddComponent<Light>();light.type=LightType.Spot;light.color=new Color(1,.74f,.43f);light.intensity=7;light.range=2.6f;light.spotAngle=65;light.innerSpotAngle=45;light.shadows=LightShadows.None;
            Debug.Log("FIELD_CAMERA_BOUNDS "+bounds);
            Debug.Log("FIELD_CAMERA_08_BUILT CC0 model, 1K PBR materials, field kit, photo investigation");
        }
    }
}
#endif
