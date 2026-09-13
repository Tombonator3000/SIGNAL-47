using System;
using System.IO;
using System.Security.Cryptography;
using Gsplat;
using Gsplat.Editor;
using UnityEditor;
using UnityEditor.Build.Reporting;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;

// Isolated STATION 01 mechanics/rendering experiment. No SIGNAL47 gameplay assembly.
public static class Hybrid23Build
{
    [Serializable] public sealed class Config
    {
        public string dataOrigin, sourceCoordinates;
        public int declaredSplatCount;
        public Vector3 position, rotation;
        public float scale = 1;
        public bool proxyRelighting, synthetic, generatedMesh;
        public string meshSha256, textureSha256;
    }
    [Serializable] sealed class Result
    {
        public string result, dataOrigin, sourceCoordinates, importedCompression, settingsResourcePath;
        public int errors, splatCount;
        public bool proxyRelighting, generatedMesh;
    }
    [Serializable] sealed class MeshData
    {
        public float[] positions, normals, uv;
        public int[] triangles;
    }
    static void EnsureRuntimeSettings()
    {
        // Upstream's delayed editor bootstrap may not run before -executeMethod Build.
        // Runtime Resources.Load has no fallback creator, so make the dependency explicit.
        var settings=GsplatSettings.Instance;
        if(!settings)throw new Exception("UnitySplats settings creation failed");
        if(!settings.ComputeShader || settings.Materials==null || settings.Materials.Length!=Enum.GetValues(typeof(CompressionMode)).Length || settings.SplatInstanceSize==0 || settings.UploadBatchSize==0 || settings.MaxRenderOrder==0)
            throw new Exception("UnitySplats settings have missing compute/material configuration");
        foreach(var material in settings.Materials)
            if(!material || !material.DefaultMaterial || !material.DefaultMaterial.shader || !material.CalcDepthShader || !material.InitOrderShader)
                throw new Exception("UnitySplats runtime material or compute shader dependency is missing");
        if(!settings.GlobalMaterial || !settings.GlobalMaterial.Valid() || !settings.GlobalMaterial.DefaultMaterial.shader)
            throw new Exception("UnitySplats global rendering dependencies are missing");
        EditorUtility.SetDirty(settings);AssetDatabase.SaveAssets();
        const string path="Assets/Gsplat/Settings/Resources/GsplatSettings.asset";
        if(AssetDatabase.GetAssetPath(settings)!=path || !File.Exists(path) || !Resources.Load<GsplatSettings>("GsplatSettings"))
            throw new Exception("UnitySplats settings are not a persisted runtime Resources asset");
        // Shader isSupported / compute execution cannot be judged in a -nographics editor.
        Debug.Log("Hybrid23 runtime Resources settings and material/compute references persisted: "+path);
    }
    static string DigestFile(string path)
    {
        using (var stream=File.OpenRead(path)) using (var sha=SHA256.Create())
            return BitConverter.ToString(sha.ComputeHash(stream)).Replace("-", "").ToLowerInvariant();
    }
    static GameObject GeneratedMesh(Config config)
    {
        const string meshPath="Assets/Probe/station-mesh.json", texturePath="Assets/Probe/station-texture.jpg";
        if (DigestFile(meshPath)!=config.meshSha256 || DigestFile(texturePath)!=config.textureSha256)
            throw new Exception("Generated mesh/texture identity differs from preparation");
        var data=JsonUtility.FromJson<MeshData>(File.ReadAllText(meshPath));
        if (data==null || data.positions==null || data.normals==null || data.uv==null || data.triangles==null || data.positions.Length%3!=0 || data.positions.Length<9 || data.normals.Length!=data.positions.Length || data.uv.Length!=data.positions.Length/3*2 || data.triangles.Length%3!=0 || data.triangles.Length<3 || data.positions.Length>1500000 || data.triangles.Length>1500000)
            throw new Exception("Invalid generated mesh arrays");
        int count=data.positions.Length/3;
        var vertices=new Vector3[count]; var normals=new Vector3[count]; var uv=new Vector2[count];
        for(int i=0;i<count;i++)
        {
            vertices[i]=new Vector3(data.positions[i*3],data.positions[i*3+1],data.positions[i*3+2]);
            normals[i]=new Vector3(data.normals[i*3],data.normals[i*3+1],data.normals[i*3+2]);
            uv[i]=new Vector2(data.uv[i*2],data.uv[i*2+1]);
        }
        foreach(int index in data.triangles)if(index<0||index>=count)throw new Exception("Invalid generated mesh triangle");
        var mesh=AssetDatabase.LoadAssetAtPath<Mesh>("Assets/Probe/GeneratedStation.asset");
        if(!mesh){mesh=new Mesh();AssetDatabase.CreateAsset(mesh,"Assets/Probe/GeneratedStation.asset");}else mesh.Clear();
        mesh.name="Generated STATION01 visual surface";mesh.indexFormat=IndexFormat.UInt32;mesh.vertices=vertices;mesh.normals=normals;mesh.uv=uv;mesh.triangles=data.triangles;mesh.RecalculateBounds();EditorUtility.SetDirty(mesh);
        var textureImporter=AssetImporter.GetAtPath(texturePath) as TextureImporter;
        if(!textureImporter)throw new Exception("Generated texture importer missing");
        textureImporter.textureType=TextureImporterType.Default;textureImporter.sRGBTexture=true;textureImporter.maxTextureSize=4096;textureImporter.textureCompression=TextureImporterCompression.Uncompressed;textureImporter.mipmapEnabled=true;textureImporter.SaveAndReimport();
        var material=Material("GeneratedStationLit",Color.white);material.mainTexture=AssetDatabase.LoadAssetAtPath<Texture2D>(texturePath);EditorUtility.SetDirty(material);
        var go=new GameObject("Generated source textured mesh baseline / same source as splats");go.AddComponent<MeshFilter>().sharedMesh=mesh;go.AddComponent<MeshRenderer>().sharedMaterial=material;
        return go;
    }
    static Material Material(string name, Color color, bool unlit = false)
    {
        var path = "Assets/Probe/" + name + ".mat";
        var material = AssetDatabase.LoadAssetAtPath<Material>(path);
        var shader = Shader.Find(unlit ? "Universal Render Pipeline/Unlit" : "Universal Render Pipeline/Lit");
        if (!shader) throw new Exception("Required URP shader missing");
        if (!material) { material = new Material(shader); AssetDatabase.CreateAsset(material, path); }
        material.shader = shader; material.color = color;
        if (material.HasProperty("_Smoothness")) material.SetFloat("_Smoothness", .15f);
        EditorUtility.SetDirty(material); return material;
    }
    static GameObject Box(string name, Vector3 position, Vector3 scale, Material material, Transform parent = null)
    {
        var go = GameObject.CreatePrimitive(PrimitiveType.Cube); go.name = name;
        if (parent) go.transform.SetParent(parent, false);
        go.transform.localPosition = position; go.transform.localScale = scale;
        go.GetComponent<Renderer>().sharedMaterial = material; return go;
    }
    static Transform Pose(string name, Vector3 position, Vector3 lookAt)
    {
        var t = new GameObject(name).transform; t.position = position;
        t.rotation = Quaternion.LookRotation(lookAt - position, Vector3.up); return t;
    }
    public static void Build()
    {
        var configPath = "Assets/Resources/Hybrid23Config.json";
        if (!File.Exists(configPath)) throw new Exception("Prepare the isolated project first");
        var config = JsonUtility.FromJson<Config>(File.ReadAllText(configPath));
        if (config == null || config.declaredSplatCount < 1 || config.scale <= 0 || string.IsNullOrWhiteSpace(config.dataOrigin)) throw new Exception("Invalid preparation config");
        EnsureRuntimeSettings();
        var importer = AssetImporter.GetAtPath("Assets/Probe/environment.ply") as GsplatImporter;
        if (!importer) throw new Exception("UnitySplats importer is unavailable");
        importer.SourceCoordinates = (SourceCoordinates)Enum.Parse(typeof(SourceCoordinates), config.sourceCoordinates);
        importer.Compression = CompressionMode.Uncompressed;
        EditorUtility.SetDirty(importer);
        AssetDatabase.WriteImportSettingsIfDirty("Assets/Probe/environment.ply");
        importer.SaveAndReimport();
        importer=AssetImporter.GetAtPath("Assets/Probe/environment.ply") as GsplatImporter;
        if(!importer || importer.Compression!=CompressionMode.Uncompressed || importer.SourceCoordinates!=(SourceCoordinates)Enum.Parse(typeof(SourceCoordinates),config.sourceCoordinates))
            throw new Exception("Explicit compression/coordinate import settings were not persisted");
        var asset = AssetDatabase.LoadAssetAtPath<GsplatAsset>("Assets/Probe/environment.ply");
        if (!asset || asset.SplatCount != config.declaredSplatCount) throw new Exception("Imported splat count differs from preparation manifest");
        if(asset.Compression!=CompressionMode.Uncompressed)throw new Exception("Imported asset does not use the requested Uncompressed representation");
        Debug.Log("Hybrid23 actual imported representation: "+asset.GetType().Name+" / "+asset.Compression+" / "+asset.SplatCount+" splats / "+importer.SourceCoordinates);

        var pipeline = AssetDatabase.LoadAssetAtPath<UniversalRenderPipelineAsset>("Assets/Signal47/Art/PrototypePort/Signal47_URP.asset");
        if (!pipeline) throw new Exception("Copied pipeline missing");
        pipeline.msaaSampleCount = 1; pipeline.supportsHDR = true;
        GraphicsSettings.defaultRenderPipeline = pipeline; QualitySettings.renderPipeline = pipeline;
        var renderer = AssetDatabase.LoadAssetAtPath<UniversalRendererData>("Assets/UniversalRenderer.asset");
        var type = typeof(GsplatRenderer).Assembly.GetType("Gsplat.GsplatURPFeature", true);
        foreach (var old in renderer.rendererFeatures.ToArray())
            if (old != null && old.GetType() == type) { renderer.rendererFeatures.Remove(old); UnityEngine.Object.DestroyImmediate(old, true); }
        var feature = (ScriptableRendererFeature)ScriptableObject.CreateInstance(type);
        feature.name = "Hybrid23 isolated UnitySplats"; AssetDatabase.AddObjectToAsset(feature, renderer); renderer.rendererFeatures.Add(feature);
        EditorUtility.SetDirty(renderer); EditorUtility.SetDirty(pipeline);
        PlayerSettings.SetUseDefaultGraphicsAPIs(BuildTarget.StandaloneLinux64, false);
        PlayerSettings.SetGraphicsAPIs(BuildTarget.StandaloneLinux64, new[] { GraphicsDeviceType.Vulkan });
        PlayerSettings.productName = "SIGNAL47 Hybrid23 Isolated Station";
        PlayerSettings.companyName = "SIGNAL47Tests"; PlayerSettings.runInBackground = true;
        PlayerSettings.fullScreenMode = FullScreenMode.Windowed; PlayerSettings.defaultScreenWidth = 1280; PlayerSettings.defaultScreenHeight = 800;
        var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);
        RenderSettings.ambientMode = AmbientMode.Flat; RenderSettings.ambientLight = new Color(.24f,.28f,.34f);
        var stone = Material("Stone", new Color(.45f,.39f,.29f));
        var wallMat = Material("FieldBuilding", new Color(.66f,.65f,.54f));
        var green = Material("Equipment", new Color(.19f,.29f,.22f));
        var slab = Material("Walkway", new Color(.35f,.37f,.35f));
        var ordinary = new GameObject("Ordinary mesh and collision; independent of splats").transform;
        Box("Ground collision", new Vector3(0,-.11f,2), new Vector3(22,.2f,22), stone, ordinary);
        Box("Arrival walkway", new Vector3(0,-.025f,-2), new Vector3(2,.05f,5), slab, ordinary);
        Box("Hut floor", new Vector3(0,-.025f,3), new Vector3(8,.05f,6), slab, ordinary);
        Box("Hut west wall", new Vector3(-4,1.4f,3), new Vector3(.2f,2.8f,6), wallMat, ordinary);
        Box("Hut east wall", new Vector3(4,1.4f,3), new Vector3(.2f,2.8f,6), wallMat, ordinary);
        Box("Hut rear wall", new Vector3(0,1.4f,6), new Vector3(8,2.8f,.2f), wallMat, ordinary);
        Box("Hut front left", new Vector3(-2.3f,1.4f,0), new Vector3(3.4f,2.8f,.2f), wallMat, ordinary);
        var testWall = Box("Hut front right collision test", new Vector3(2.3f,1.4f,0), new Vector3(3.4f,2.8f,.2f), wallMat, ordinary);
        Box("Door lintel", new Vector3(0,2.5f,0), new Vector3(1.2f,.6f,.2f), wallMat, ordinary);
        Box("Flat roof", new Vector3(0,2.85f,3), new Vector3(8.3f,.15f,6.3f), wallMat, ordinary);
        GameObject generatedMeshRoot=null;
        if(config.generatedMesh)
        {
            generatedMeshRoot=GeneratedMesh(config);
            foreach(var visual in ordinary.GetComponentsInChildren<MeshRenderer>())
                if(visual.name.StartsWith("Hut ") && visual.name!="Hut floor" || visual.name=="Flat roof" || visual.name=="Door lintel")visual.enabled=false;
            // Independent thin inside surfaces retain a readable room in both representations.
            Box("Inner west lining",new Vector3(-3.55f,1.35f,3),new Vector3(.06f,2.7f,5.16f),wallMat,ordinary);
            Box("Inner east lining",new Vector3(3.55f,1.35f,3),new Vector3(.06f,2.7f,5.16f),wallMat,ordinary);
            Box("Inner rear lining",new Vector3(0,1.35f,5.55f),new Vector3(7.16f,2.7f,.06f),wallMat,ordinary);
            Box("Inner front left lining",new Vector3(-2.15f,1.35f,.45f),new Vector3(2.8f,2.7f,.06f),wallMat,ordinary);
            Box("Inner front right lining",new Vector3(2.15f,1.35f,.45f),new Vector3(2.8f,2.7f,.06f),wallMat,ordinary);
            // Opaque authored interior surfaces are a hybrid layout correction, not a
            // claim that the Gaussian renderer now resolves arbitrary depth/occlusion.
            Box("Inner ceiling",new Vector3(0,2.63f,3),new Vector3(7.16f,.06f,5.16f),wallMat,ordinary);
            Box("Inner finished floor",new Vector3(0,.065f,3),new Vector3(7.16f,.07f,5.16f),slab,ordinary);
            Box("Doorframe left",new Vector3(-.68f,1.10f,-.03f),new Vector3(.16f,2.2f,.22f),green,ordinary);
            Box("Doorframe right",new Vector3(.68f,1.10f,-.03f),new Vector3(.16f,2.2f,.22f),green,ordinary);
            Box("Doorframe top",new Vector3(0,2.17f,-.03f),new Vector3(1.52f,.16f,.22f),green,ordinary);
        }
        var hinge = new GameObject("Interactive door hinge").transform; hinge.SetParent(ordinary); hinge.position = new Vector3(-.6f,0,0);
        var door = Box("Door leaf", new Vector3(.6f,1.05f,0), new Vector3(1.2f,2.1f,.14f), green, hinge);
        Box("Door handle", new Vector3(1.06f,1.03f,-.1f), new Vector3(.16f,.055f,.06f), slab, hinge);
        Box("Work bench", new Vector3(-2, .8f, 4.8f), new Vector3(2,.12f,.7f), green, ordinary);
        Box("Transit pedestal", new Vector3(5,.35f,-1), new Vector3(.45f,.7f,.45f), stone, ordinary);
        var cameraProp = Box("Field camera ordinary mesh", new Vector3(5,.88f,-1), new Vector3(.32f,.24f,.20f), green, ordinary);
        Box("Field camera lens", new Vector3(5,.88f,-1.13f), new Vector3(.12f,.12f,.10f), slab, ordinary);
        var pole = Box("Local lamp pole", new Vector3(4.8f,1.1f,.5f), new Vector3(.08f,2.2f,.08f), green, ordinary);
        var lamp = new GameObject("Toggleable local lamp").AddComponent<Light>();
        lamp.type = LightType.Point; lamp.range = 12; lamp.intensity = 8; lamp.color = new Color(1,.77f,.48f); lamp.shadows = LightShadows.Soft;
        lamp.transform.position = config.generatedMesh ? new Vector3(2,2.2f,-2) : new Vector3(4.8f,2.2f,.5f);
        if(config.generatedMesh)pole.transform.position=new Vector3(2,1.1f,-2);
        var moon = new GameObject("Fixed low directional fill").AddComponent<Light>();
        moon.type = LightType.Directional; moon.intensity = .65f; moon.color = new Color(.6f,.7f,1); moon.shadows = LightShadows.Soft;
        moon.transform.rotation = Quaternion.Euler(35,-30,0);

        var player = new GameObject("Isolated first person collision controller").AddComponent<CharacterController>();
        player.height = 1.8f; player.radius = .28f; player.center = new Vector3(0,.9f,0); player.skinWidth = .035f; player.stepOffset = .15f;
        player.transform.position = new Vector3(0,0,-4);
        var cam = new GameObject("Hybrid23 camera").AddComponent<Camera>(); cam.tag = "MainCamera";
        cam.transform.SetParent(player.transform,false); cam.transform.localPosition = new Vector3(0,1.6f,0);
        cam.clearFlags = CameraClearFlags.SolidColor; cam.backgroundColor = new Color(.035f,.05f,.08f);
        cam.nearClipPlane = .08f; cam.farClipPlane = 120; cam.fieldOfView = 70; cam.allowHDR = true; cam.allowMSAA = false;
        cam.gameObject.AddComponent<UniversalAdditionalCameraData>(); cam.gameObject.AddComponent<AudioListener>();
        var splats = new GameObject("Environment splats / " + (config.synthetic ? "original synthetic fixture" : "external candidate"));
        splats.SetActive(false); splats.transform.position = config.position; splats.transform.rotation = Quaternion.Euler(config.rotation); splats.transform.localScale = Vector3.one * config.scale;
        var gs = splats.AddComponent<GsplatRenderer>(); gs.GsplatAsset = asset; gs.SHDegree = Math.Min(3, (int)asset.SHBands); gs.AsyncUpload = false;
        GsplatProxyRelighting relighting=null;
        if (config.proxyRelighting)
        {
            if (!config.synthetic && !generatedMeshRoot) throw new Exception("No aligned external relighting proxy supplied");
            var proxy = new GameObject(config.generatedMesh ? "Exact generated surface lighting proxy" : "Synthetic aligned relighting proxy"); proxy.transform.SetParent(splats.transform,false);
            var proxyMat = Material("NeutralProxy", new Color(.5f,.5f,.5f)); proxyMat.mainTexture=null; EditorUtility.SetDirty(proxyMat);
            if(config.generatedMesh)
            {
                var sourceMesh=generatedMeshRoot.GetComponent<MeshFilter>().sharedMesh;
                if(!sourceMesh)throw new Exception("Matched generated lighting mesh missing");
                proxy.AddComponent<MeshFilter>().sharedMesh=sourceMesh;
                var proxyRenderer=proxy.AddComponent<MeshRenderer>();proxyRenderer.sharedMaterial=proxyMat;
                proxyRenderer.shadowCastingMode=ShadowCastingMode.On;proxyRenderer.receiveShadows=true;
            }
            else
            {
                Box("Proxy ground", new Vector3(5.95f,0,3.55f), new Vector3(4.5f,.05f,4.5f), proxyMat, proxy.transform);
                foreach (var rock in new[] { new Vector3(5,0,3.1f),new Vector3(6.7f,0,4.6f),new Vector3(7.1f,0,2.5f) })
                {
                    float radius = rock.x == 5 ? .7f : rock.x == 6.7f ? 1 : .55f;
                    var sphere = GameObject.CreatePrimitive(PrimitiveType.Sphere); sphere.name = "Proxy rock"; sphere.transform.SetParent(proxy.transform,false);
                    sphere.transform.localPosition = rock; sphere.transform.localScale = Vector3.one*radius*2; sphere.GetComponent<Renderer>().sharedMaterial = proxyMat;
                }
            }
            foreach (var c in proxy.GetComponentsInChildren<Collider>()) UnityEngine.Object.DestroyImmediate(c);
            relighting=proxy.AddComponent<GsplatProxyRelighting>();relighting.SourceCamera=cam;relighting.TextureScale=.5f;
            relighting.Blend=1;relighting.Brightness=2;relighting.Background=1;
            // Public binding initializes capture on activation; headless construction keeps it inactive.
            const int proxyLayer=30;
            foreach(var item in proxy.GetComponentsInChildren<Transform>(true))item.gameObject.layer=proxyLayer;
            cam.cullingMask &= ~(1<<proxyLayer);
        }
        var cyan = Box("Opaque cyan occlusion reference", config.generatedMesh ? new Vector3(2.6f,.8f,-1) : new Vector3(5,.65f,1.5f), new Vector3(.4f,1.3f,.14f), Material("OcclusionCyan",new Color(.12f,.65f,.75f),true), ordinary);
        var runtime = new GameObject("Hybrid23 runtime and checks").AddComponent<Hybrid23Runtime>();
        runtime.cam=cam; runtime.player=player; runtime.splatsRoot=splats; runtime.door=hinge; runtime.doorCollider=door.GetComponent<Collider>(); runtime.lamp=lamp; runtime.wall=testWall.GetComponent<Collider>(); runtime.cyanOccluder=cyan; runtime.generatedMeshRoot=generatedMeshRoot; runtime.relighting=relighting;
        runtime.playerStart=Pose("Player start",new Vector3(0,0,-4),new Vector3(0,0,0));
        runtime.doorTestStart=Pose("Door test start",new Vector3(0,0,-1),new Vector3(0,0,1));
        runtime.wallTestStart=Pose("Wall test start",new Vector3(2,0,-1),new Vector3(2,0,1));
        runtime.photoPose=Pose("Photo A B pose",new Vector3(8,2.3f,-6),config.generatedMesh ? new Vector3(0,1.2f,2) : new Vector3(2.5f,1.0f,2.5f));
        runtime.dataOrigin=config.dataOrigin; runtime.declaredSplatCount=config.declaredSplatCount;
        AssetDatabase.SaveAssets(); EditorSceneManager.SaveScene(scene,"Assets/Probe/Hybrid23.unity");
        var report=BuildPipeline.BuildPlayer(new BuildPlayerOptions{scenes=new[]{"Assets/Probe/Hybrid23.unity"},locationPathName="../Player/Hybrid23.x86_64",target=BuildTarget.StandaloneLinux64,options=BuildOptions.None});
        File.WriteAllText("../build-result.json",JsonUtility.ToJson(new Result{result=report.summary.result.ToString(),errors=(int)report.summary.totalErrors,splatCount=(int)asset.SplatCount,dataOrigin=config.dataOrigin,sourceCoordinates=config.sourceCoordinates,proxyRelighting=config.proxyRelighting,generatedMesh=config.generatedMesh,importedCompression=asset.Compression.ToString(),settingsResourcePath="Assets/Gsplat/Settings/Resources/GsplatSettings.asset"},true));
        if(report.summary.result!=BuildResult.Succeeded)throw new Exception("Hybrid23 build failed");
    }
}
