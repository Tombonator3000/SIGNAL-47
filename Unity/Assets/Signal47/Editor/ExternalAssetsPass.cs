#if UNITY_EDITOR
using System.IO;
using UnityEditor;
using UnityEngine;
using Signal47.Audio;
using Signal47.Events;
using Signal47.Interaction;
using Signal47.Signals;
using Signal47.UI;
namespace Signal47.Editor
{
    public static class ExternalAssetsPass
    {
        const string Root="Assets/Signal47/Art/ThirdParty/";
        static string Folder(string id)=>Root+"PolyHaven/"+id+"/";
        static Texture2D Texture(string path,bool normal=false,bool linear=false,bool readable=false)
        {
            var importer=(TextureImporter)AssetImporter.GetAtPath(path);
            if(!importer)throw new FileNotFoundException(path);
            var type=normal?TextureImporterType.NormalMap:TextureImporterType.Default;
            bool changed=importer.textureType!=type||importer.sRGBTexture==linear||importer.isReadable!=readable;
            importer.textureType=type;importer.sRGBTexture=!linear;importer.isReadable=readable;importer.maxTextureSize=2048;
            if(changed)importer.SaveAndReimport();
            return AssetDatabase.LoadAssetAtPath<Texture2D>(path);
        }
        static Material Surface(string id,string prefix="")
        {
            string dir=Folder(id),name=id+prefix,path=dir+name+".mat";
            var mat=AssetDatabase.LoadAssetAtPath<Material>(path);
            if(!mat){mat=new Material(Shader.Find("Universal Render Pipeline/Lit"));AssetDatabase.CreateAsset(mat,path);}
            string diffuse=prefix==""?"Diffuse":"accessories_diff",rough=prefix==""?"Rough":"accessories_rough",metal=prefix==""?"Metal":"accessories_metal",normal=prefix==""?"nor_gl":"accessories_nor_gl";
            mat.SetTexture("_BaseMap",Texture(dir+diffuse+".jpg"));mat.SetColor("_BaseColor",Color.white);
            mat.SetTexture("_BumpMap",Texture(dir+normal+".png",true,true));mat.SetFloat("_BumpScale",.65f);mat.EnableKeyword("_NORMALMAP");
            string packedPath=dir+name+"_MetalSmooth.png";
            if(!File.Exists(packedPath)){
                var r=Texture(dir+rough+".jpg",false,true,true);var m=File.Exists(dir+metal+".jpg")?Texture(dir+metal+".jpg",false,true,true):null;
                var rp=r.GetPixels32();var mp=m?m.GetPixels32():null;var data=new Color32[rp.Length];
                for(int i=0;i<data.Length;i++)data[i]=new Color32(mp!=null?mp[i].r:(byte)0,0,0,(byte)(255-rp[i].r));
                var packed=new Texture2D(r.width,r.height,TextureFormat.RGBA32,false,true);packed.SetPixels32(data);packed.Apply();File.WriteAllBytes(packedPath,packed.EncodeToPNG());Object.DestroyImmediate(packed);AssetDatabase.ImportAsset(packedPath);
                Texture(dir+rough+".jpg",false,true);if(m)Texture(dir+metal+".jpg",false,true);
            }
            mat.SetTexture("_MetallicGlossMap",Texture(packedPath,false,true));mat.SetFloat("_Smoothness",.7f);mat.EnableKeyword("_METALLICSPECGLOSSMAP");EditorUtility.SetDirty(mat);return mat;
        }
        static Bounds Bounds(GameObject root)
        {
            var b=new Bounds();bool first=true;foreach(var r in root.GetComponentsInChildren<Renderer>()){if(!r.enabled)continue;if(first){b=r.bounds;first=false;}else b.Encapsulate(r.bounds);}return b;
        }
        static GameObject Model(string id,string name,Vector3 floor,float yaw,float width)
        {
            string path=Folder(id)+id+".fbx";var source=AssetDatabase.LoadAssetAtPath<GameObject>(path);if(!source)throw new FileNotFoundException(path);
            var root=(GameObject)PrefabUtility.InstantiatePrefab(source);root.name=name;root.transform.rotation=Quaternion.Euler(0,yaw,0);
            // The static FBX contains rig curves and loose wire segments without their Blender deformation.
            foreach(var renderer in root.GetComponentsInChildren<Renderer>())if(id=="desk_lamp_arm_01"&&(renderer.name.StartsWith("cur_")||renderer.name.EndsWith("_wire")))renderer.enabled=false;
            var bounds=Bounds(root);root.transform.localScale*=width/bounds.size.x;bounds=Bounds(root);root.transform.position+=floor-new Vector3(bounds.center.x,bounds.min.y,bounds.center.z);
            Material bulb=null;
            if(id=="desk_lamp_arm_01"){
                string bulbPath=Folder(id)+"LampBulb.mat";bulb=AssetDatabase.LoadAssetAtPath<Material>(bulbPath);
                if(!bulb){bulb=new Material(Shader.Find("Universal Render Pipeline/Lit"));AssetDatabase.CreateAsset(bulb,bulbPath);}
                bulb.SetColor("_BaseColor",new Color(1,.9f,.65f));bulb.SetColor("_EmissionColor",new Color(1,.65f,.3f)*1.2f);bulb.EnableKeyword("_EMISSION");EditorUtility.SetDirty(bulb);
            }
            var main=Surface(id);Material accessories=id=="vintage_radio_transceiver"?Surface(id,"_accessories"):main;
            foreach(var renderer in root.GetComponentsInChildren<Renderer>()){
                var old=renderer.sharedMaterials;var materials=new Material[old.Length];
                for(int i=0;i<old.Length;i++)materials[i]=bulb&&old[i]&&old[i].name.EndsWith("_light")?bulb:old[i]&&old[i].name.Contains("accessories")?accessories:main;
                renderer.sharedMaterials=materials;
            }
            return root;
        }
        static AudioClip Clip(string path,bool streaming=false)
        {
            string full=Root+path;var importer=(AudioImporter)AssetImporter.GetAtPath(full);if(!importer)throw new FileNotFoundException(full);
            var settings=importer.defaultSampleSettings;settings.loadType=streaming?AudioClipLoadType.Streaming:AudioClipLoadType.DecompressOnLoad;settings.compressionFormat=streaming?AudioCompressionFormat.Vorbis:AudioCompressionFormat.PCM;settings.preloadAudioData=!streaming;importer.defaultSampleSettings=settings;importer.SaveAndReimport();return AssetDatabase.LoadAssetAtPath<AudioClip>(full);
        }
        public static void Dress()
        {
            var floor=GameObject.Find("Floor").GetComponent<Renderer>();var material=Surface("old_linoleum_flooring_01");material.SetTextureScale("_BaseMap",new Vector2(9.35f,7.5f));material.SetTextureScale("_BumpMap",new Vector2(9.35f,7.5f));material.SetTextureScale("_MetallicGlossMap",new Vector2(9.35f,7.5f));floor.sharedMaterial=material;
            var desk=Model("metal_office_desk","ImportedPhoneDesk",new Vector3(5.1f,0,.5f),180,2.2f);var db=Bounds(desk);
            Object.DestroyImmediate(GameObject.Find("PhoneDesk"));
            var collider=desk.AddComponent<BoxCollider>();collider.center=desk.transform.InverseTransformPoint(db.center);collider.size=new Vector3(db.size.x/desk.transform.lossyScale.x,db.size.y/desk.transform.lossyScale.y,db.size.z/desk.transform.lossyScale.z);
            var phone=Object.FindFirstObjectByType<PhoneInteractable>();phone.transform.position=new Vector3(5.1f,db.max.y+.015f,.5f);
            var mug=Object.FindFirstObjectByType<MugBreakable>();mug.transform.position=new Vector3(4.35f,db.max.y+.004f,.30f);
            foreach(string name in new[]{"LampFoot","LampStem","LampShade"})Object.DestroyImmediate(GameObject.Find(name));
            var lamp=Model("desk_lamp_arm_01","ImportedDeskLamp",new Vector3(5.8f,db.max.y-.06f,.55f),180,.55f);
            var lampLight=GameObject.Find("DeskLamp").GetComponent<Light>();
            foreach(var r in lamp.GetComponentsInChildren<Renderer>())if(r.name=="geo_lamp-head")lampLight.transform.position=new Vector3(r.bounds.center.x,r.bounds.min.y-.015f,r.bounds.center.z);
            lampLight.type=LightType.Spot;lampLight.transform.rotation=Quaternion.Euler(90,0,0);lampLight.spotAngle=100;lampLight.innerSpotAngle=65;lampLight.range=2.5f;lampLight.intensity=.5f;lampLight.shadows=LightShadows.Soft;
            var radioDesk=Model("metal_office_desk","RadioWorkbench",new Vector3(-5.8f,0,-3.7f),90,1.1f);var rb=Bounds(radioDesk);
            var workCollider=radioDesk.AddComponent<BoxCollider>();workCollider.center=radioDesk.transform.InverseTransformPoint(rb.center);workCollider.size=radioDesk.transform.InverseTransformVector(rb.size);workCollider.size=new Vector3(Mathf.Abs(workCollider.size.x),Mathf.Abs(workCollider.size.y),Mathf.Abs(workCollider.size.z));
            Model("vintage_radio_transceiver","ImportedBackupRadio",new Vector3(-5.8f,rb.max.y+.004f,-3.7f),90,.72f);
            string skyPath=Folder("qwantani_night_puresky")+"NightSky.mat";var sky=AssetDatabase.LoadAssetAtPath<Material>(skyPath);if(!sky){sky=new Material(Shader.Find("Skybox/Panoramic"));AssetDatabase.CreateAsset(sky,skyPath);}sky.SetTexture("_MainTex",AssetDatabase.LoadAssetAtPath<Texture2D>(Folder("qwantani_night_puresky")+"Sky.hdr"));sky.SetFloat("_Exposure",.025f);sky.SetColor("_Tint",new Color(.32f,.40f,.5f));sky.SetFloat("_Rotation",115);EditorUtility.SetDirty(sky);RenderSettings.skybox=sky;
            Object.FindFirstObjectByType<Camera>().clearFlags=CameraClearFlags.Skybox;
            var director=Object.FindFirstObjectByType<PrologueDirector>();var palette=director.gameObject.AddComponent<SoundPalette>();
            palette.printer=Clip("PreparedAudio/Printer.wav");palette.phone=Clip("PreparedAudio/PhoneRing.wav");palette.smash=Clip("PreparedAudio/CeramicBreak.wav");palette.wind=Clip("PreparedAudio/DesertWind.wav");palette.click=Clip("Kenney/click_001.ogg");palette.switchClick=Clip("Kenney/switch_003.ogg");palette.titleMusic=Clip("PreparedAudio/TitleMusic.ogg",true);director.palette=palette;
            var font=AssetDatabase.LoadAssetAtPath<Font>(Root+"VT323/VT323-Regular.ttf");Object.FindFirstObjectByType<SignalConsole>().terminalFont=font;
            Object.FindFirstObjectByType<HUDController>().terminalFont=font;
        }
    }
}
#endif
