#if UNITY_EDITOR
using System.IO;
using UnityEditor;
using UnityEngine;
namespace Signal47.Editor
{
    // Derive finishes without changing any original Visual10 material or texture.
    public static class Environment27Materials
    {
        const string Root="Assets/Signal47/Art/Environment27/Materials/";
        const string V10="Assets/Signal47/Art/Visual10/";
        public static Material Ground,Rock,Boulder,FarRock,Gravel,Plaster,Concrete,Metal,Steel,Wood,Rubber,Glass,Window,Paper,Emissive,Grass,Roof;
        static Texture2D Map(string path,bool normal=false,bool linear=false,bool readable=false)
        {
            var imp=(TextureImporter)AssetImporter.GetAtPath(path);if(!imp)throw new FileNotFoundException(path);
            var type=normal?TextureImporterType.NormalMap:TextureImporterType.Default;
            if(imp.textureType!=type||imp.sRGBTexture==linear||imp.isReadable!=readable||imp.anisoLevel!=4)
            {imp.textureType=type;imp.sRGBTexture=!linear;imp.isReadable=readable;imp.anisoLevel=4;imp.maxTextureSize=2048;imp.SaveAndReimport();}
            return AssetDatabase.LoadAssetAtPath<Texture2D>(path);
        }
        static Material Clone(string name,string source,Color tint,float normal=.4f)
        {
            var original=AssetDatabase.LoadAssetAtPath<Material>(V10+source+".mat");
            if(!original)throw new FileNotFoundException(source);
            var path=Root+name+".mat";var m=AssetDatabase.LoadAssetAtPath<Material>(path);
            if(!m){m=new Material(original);AssetDatabase.CreateAsset(m,path);}else m.CopyPropertiesFromMaterial(original);
            m.name=name;m.SetColor("_BaseColor",tint);m.SetTextureScale("_BaseMap",Vector2.one);m.SetFloat("_BumpScale",normal);EditorUtility.SetDirty(m);return m;
        }
        public static void Build()
        {
            Directory.CreateDirectory(Root);AssetDatabase.Refresh();
            Ground=Clone("E27_Ground","V10_Desert",new Color(.82f,.72f,.59f),.85f);
            Rock=Clone("E27_Sandstone","V10_Desert",new Color(.69f,.65f,.55f),1.15f);
            Boulder=Clone("E27_Boulder01","V10_Desert",new Color(.76f,.75f,.71f),.8f);
            const string boulder="Assets/Signal47/Art/Environment27/Boulder01/boulder_01_";
            Boulder.SetTexture("_BaseMap",Map(boulder+"diff_2k.jpg"));Boulder.SetTexture("_BumpMap",Map(boulder+"nor_gl_2k.jpg",true,true));
            var arm=Map(boulder+"arm_2k.jpg",false,true,true);var pixels=arm.GetPixels32();
            for(int i=0;i<pixels.Length;i++)pixels[i]=new Color32(pixels[i].b,pixels[i].r,0,(byte)((255-pixels[i].g)*.45f));
            var packed=new Texture2D(arm.width,arm.height,TextureFormat.RGBA32,false,true);packed.SetPixels32(pixels);packed.Apply();var data=packed.EncodeToPNG();Object.DestroyImmediate(packed);
            string packedPath=Root+"BoulderMetalSmoothAO.png";if(!File.Exists(packedPath)||!System.Linq.Enumerable.SequenceEqual(File.ReadAllBytes(packedPath),data))File.WriteAllBytes(packedPath,data);
            AssetDatabase.ImportAsset(packedPath);var maps=Map(packedPath,false,true);Boulder.SetTexture("_MetallicGlossMap",maps);Boulder.SetTexture("_OcclusionMap",maps);Boulder.EnableKeyword("_OCCLUSIONMAP");Boulder.SetFloat("_OcclusionStrength",.65f);Map(boulder+"arm_2k.jpg",false,true);
            FarRock=Clone("E27_DistantRock","V10_Desert",new Color(.34f,.39f,.45f),.25f);
            Gravel=Clone("E27_Gravel","V10_Desert",new Color(.59f,.52f,.41f),.7f);
            Plaster=Clone("E27_Plaster","V10_DishEnamel",new Color(.79f,.72f,.58f),.8f);
            Concrete=Clone("E27_Concrete","V10_FieldSteel",new Color(.61f,.56f,.46f),.75f);
            Concrete.SetTexture("_MetallicGlossMap",null);Concrete.DisableKeyword("_METALLICSPECGLOSSMAP");Concrete.SetFloat("_Metallic",0);Concrete.SetFloat("_Smoothness",.08f);
            Metal=Clone("E27_PaintedMetal","V10_CabinetEnamel",new Color(.78f,.77f,.63f),.45f);
            Steel=Clone("E27_Steel","V10_FieldSteel",new Color(.62f,.62f,.54f),.18f);
            Wood=Clone("E27_Wood","V10_DishEnamel",new Color(.35f,.25f,.14f),.7f);
            Rubber=Clone("E27_Rubber","V10_FieldSteel",new Color(.065f,.059f,.048f),.12f);Rubber.SetTexture("_MetallicGlossMap",null);Rubber.DisableKeyword("_METALLICSPECGLOSSMAP");Rubber.SetFloat("_Metallic",0);Rubber.SetFloat("_Smoothness",.13f);
            Glass=Clone("E27_Glass","V10_FieldSteel",new Color(.035f,.07f,.078f),.03f);Glass.SetFloat("_Smoothness",.85f);
            Window=Clone("E27_Window","V10_FieldSteel",new Color(.09f,.15f,.18f,.19f),.01f);
            Window.SetFloat("_Surface",1);Window.SetFloat("_Blend",0);Window.SetFloat("_SrcBlend",(float)UnityEngine.Rendering.BlendMode.SrcAlpha);Window.SetFloat("_DstBlend",(float)UnityEngine.Rendering.BlendMode.OneMinusSrcAlpha);
            Window.SetFloat("_ZWrite",0);Window.SetOverrideTag("RenderType","Transparent");Window.EnableKeyword("_SURFACE_TYPE_TRANSPARENT");Window.renderQueue=3000;
            Emissive=Clone("E27_Lamp","V10_FluorescentTube",new Color(1,.77f,.43f),0);Emissive.EnableKeyword("_EMISSION");Emissive.SetColor("_EmissionColor",new Color(1,.52f,.18f)*1.5f);
            Grass=Clone("E27_Grass","V10_DryGrass_Atlas",new Color(.88f,.80f,.63f),0);
            Grass.enableInstancing=true;
            Paper=Clone("E27_Paper","V10_DishEnamel",new Color(.82f,.78f,.64f),.04f);
            Roof=Clone("E27_Roof","V10_CabinetEnamel",new Color(.44f,.40f,.32f),.3f);
            foreach(var m in new[]{Ground,Rock,Boulder,FarRock,Gravel,Plaster,Concrete,Metal,Steel,Wood,Rubber,Glass,Window,Emissive,Grass,Paper,Roof})EditorUtility.SetDirty(m);
        }
        public static Material Named(string name)
        {
            if(name.Contains("Boulder01"))return Boulder;
            if(name.Contains("Sandstone"))return Rock;if(name.Contains("Steel"))return Steel;
            if(name.Contains("Rubber"))return Rubber;if(name.Contains("Wood"))return Wood;
            if(name.Contains("Glass"))return Glass;if(name.StartsWith("E27_Lamp")||name.Contains("Bulb")||name.Contains("Diffuser"))return Emissive;
            if(name.Contains("Paper"))return Paper;return Metal;
        }
    }
}
#endif
