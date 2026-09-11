#if UNITY_EDITOR
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;
using Signal47.Core;
using Object = UnityEngine.Object;

namespace Signal47.Editor
{
    // Reproducible presentation-only pass after the complete chapter dressing.
    public static class Visual10Pass
    {
        const string Root = "Assets/Signal47/Art/Visual10/";
        static Transform root;
        static Material Chapter(string name) => AssetDatabase.LoadAssetAtPath<Material>("Assets/Signal47/Art/Chapter09/CH09_" + name + ".mat");
        static Texture2D Map(string path, bool normal = false, bool linear = false, bool readable = false)
        {
            var importer = (TextureImporter)AssetImporter.GetAtPath(path);
            if (!importer) throw new FileNotFoundException(path);
            var type = normal ? TextureImporterType.NormalMap : TextureImporterType.Default;
            if (importer.textureType != type || importer.sRGBTexture == linear || importer.isReadable != readable || importer.anisoLevel != 4)
            {
                importer.textureType = type; importer.sRGBTexture = !linear; importer.isReadable = readable;
                importer.anisoLevel = 4; importer.maxTextureSize = 2048; importer.SaveAndReimport();
            }
            return AssetDatabase.LoadAssetAtPath<Texture2D>(path);
        }
        static void Pbr(Material mat, string family, Color tint, float smooth, float metallic, float normal = .35f)
        {
            var dir = Root + "Textures/" + family + "/";
            mat.SetTexture("_BaseMap", Map(dir + family + "_diff_2k.jpg")); mat.SetColor("_BaseColor", tint);
            mat.SetTexture("_BumpMap", Map(dir + family + "_nor_gl_2k.jpg", true, true)); mat.SetFloat("_BumpScale", normal); mat.EnableKeyword("_NORMALMAP");
            string packed = dir + mat.name + "_metal_smooth.png";
            {
                var rough = Map(dir + family + "_rough_2k.jpg", false, true, true);
                var pixels = rough.GetPixels32();
                for (int i = 0; i < pixels.Length; i++) pixels[i] = new Color32((byte)(metallic * 255), 0, 0, (byte)(255 * smooth * (mat.name=="CH09_Linoleum" ? .85f+.15f*(1-pixels[i].r/255f) : .35f + .65f * (1 - pixels[i].r / 255f))));
                var texture = new Texture2D(rough.width, rough.height, TextureFormat.RGBA32, false, true);
                texture.SetPixels32(pixels); texture.Apply(); var bytes=texture.EncodeToPNG();
                if(!File.Exists(packed)||!File.ReadAllBytes(packed).SequenceEqual(bytes)) File.WriteAllBytes(packed,bytes); Object.DestroyImmediate(texture);
                AssetDatabase.ImportAsset(packed); Map(dir + family + "_rough_2k.jpg", false, true);
            }
            // Pack true metalness plus remapped source roughness for each finish.
            mat.SetTexture("_MetallicGlossMap", Map(packed, false, true)); mat.EnableKeyword("_METALLICSPECGLOSSMAP");
            mat.SetFloat("_Metallic", metallic); mat.SetFloat("_Smoothness", 1);
            mat.SetTexture("_OcclusionMap", null);
            EditorUtility.SetDirty(mat);
        }
        static Material NewMaterial(string name, string family, Color tint, float smooth, float metal = 0, float normal = .35f)
        {
            var path = Root + name + ".mat"; var m = AssetDatabase.LoadAssetAtPath<Material>(path);
            if (!m) { m = new Material(Shader.Find("Universal Render Pipeline/Lit")) { name = name }; AssetDatabase.CreateAsset(m, path); }
            Pbr(m, family, tint, smooth, metal, normal); return m;
        }
        static void Rendering()
        {
            var urp = (UniversalRenderPipelineAsset)GraphicsSettings.defaultRenderPipeline;
            var so = new SerializedObject(urp);
            so.FindProperty("m_AdditionalLightShadowsSupported").boolValue = true;
            so.FindProperty("m_SoftShadowsSupported").boolValue = true;
            so.FindProperty("m_AdditionalLightsPerObjectLimit").intValue = 8;
            so.FindProperty("m_ReflectionProbeBoxProjection").boolValue = true;
            so.FindProperty("m_ReflectionProbeBlending").boolValue = true;
            so.ApplyModifiedPropertiesWithoutUndo();
            urp.msaaSampleCount = 2; urp.shadowDistance = 42; urp.additionalLightsShadowmapResolution = 2048;
            var data = AssetDatabase.LoadAssetAtPath<UniversalRendererData>("Assets/UniversalRenderer.asset");
            var ao = data.rendererFeatures.OfType<ScreenSpaceAmbientOcclusion>().FirstOrDefault();
            if (!ao) { ao = ScriptableObject.CreateInstance<ScreenSpaceAmbientOcclusion>(); ao.name = "Visual10 Contact Occlusion"; AssetDatabase.AddObjectToAsset(ao, data); data.rendererFeatures.Add(ao); }
            var settings = new SerializedObject(ao); var s = settings.FindProperty("m_Settings");
            s.FindPropertyRelative("Downsample").boolValue = false;
            s.FindPropertyRelative("Source").enumValueIndex = 0;
            s.FindPropertyRelative("Intensity").floatValue = 1.25f;
            s.FindPropertyRelative("Radius").floatValue = .14f;
            s.FindPropertyRelative("DirectLightingStrength").floatValue = .35f;
            s.FindPropertyRelative("Samples").enumValueIndex = 1;
            s.FindPropertyRelative("Falloff").floatValue = 32;
            settings.ApplyModifiedPropertiesWithoutUndo(); ao.SetActive(true); EditorUtility.SetDirty(data); EditorUtility.SetDirty(urp);
            var profile = AssetDatabase.LoadAssetAtPath<VolumeProfile>(Root + "Visual10Volume.asset");
            if (!profile) { profile = ScriptableObject.CreateInstance<VolumeProfile>(); AssetDatabase.CreateAsset(profile, Root + "Visual10Volume.asset"); }
            if (!profile.TryGet<Bloom>(out var bloom)) bloom = profile.Add<Bloom>(true);
            bloom.intensity.Override(.12f); bloom.threshold.Override(1.1f); bloom.scatter.Override(.55f);
            if (!profile.TryGet<Tonemapping>(out var tone)) tone = profile.Add<Tonemapping>(true);
            tone.mode.Override(TonemappingMode.Neutral);
            if (!profile.TryGet<ColorAdjustments>(out var color)) color = profile.Add<ColorAdjustments>(true);
            color.postExposure.Override(.25f); color.contrast.Override(8); color.saturation.Override(-5);
            EditorUtility.SetDirty(profile);
            var volume = new GameObject("Visual10 Film Response").AddComponent<Volume>(); volume.transform.SetParent(root); volume.isGlobal = true; volume.sharedProfile = profile; volume.priority = 10;
            foreach (var camera in Object.FindObjectsByType<Camera>(FindObjectsSortMode.None))
            { var c = camera.GetUniversalAdditionalCameraData(); c.renderPostProcessing = true; c.antialiasing = AntialiasingMode.FastApproximateAntialiasing; }
        }
        static void Shadows(string name, float intensity = -1)
        {
            var go = GameObject.Find(name); if (!go) return; var l = go.GetComponent<Light>(); if (!l) return;
            l.shadows = LightShadows.Soft; l.shadowBias = .04f; l.shadowNormalBias = .15f; l.shadowResolution = LightShadowResolution.Low;
            var extra=go.GetComponent<UniversalAdditionalLightData>() ?? go.AddComponent<UniversalAdditionalLightData>();
            extra.softShadowQuality=SoftShadowQuality.High;
            var shadowSettings=new SerializedObject(extra);shadowSettings.FindProperty("m_AdditionalLightsShadowResolutionTier").intValue=1;shadowSettings.ApplyModifiedPropertiesWithoutUndo();
            if (intensity >= 0) l.intensity = intensity;
        }
        static void Probe(string name, Vector3 position, Vector3 size)
        {
            var go = new GameObject(name); go.transform.SetParent(root); go.transform.position = position;
            var p = go.AddComponent<ReflectionProbe>(); p.mode = ReflectionProbeMode.Realtime; p.refreshMode = ReflectionProbeRefreshMode.OnAwake;
            p.timeSlicingMode = ReflectionProbeTimeSlicingMode.AllFacesAtOnce; p.resolution = 128; p.size = size + Vector3.one*.8f; p.boxProjection = true; p.blendDistance = .25f;
            // Original exact-room bounds faded to zero at the floor and walls.
            var refresh=go.AddComponent<Signal47.Audio.PracticalReflection>();
            if(name=="V10.B12Reflection"){var chapter=Object.FindFirstObjectByType<Signal47.Chapter.ChapterInvestigation>();refresh.practical=chapter.experimentLight;refresh.reference=chapter.referenceVane;}
            p.nearClipPlane = .1f; p.farClipPlane = 65; p.intensity = .9f;
        }
        static GameObject Box(string name, Vector3 position, Vector3 size, Material mat)
        {
            var go = Signal47SceneBuilder.Cube(name, position, size, mat, false); go.transform.SetParent(root, true); return go;
        }
        static void Materials()
        {
            Pbr(Chapter("Green"), "green_metal_rust", new Color(.8f,.83f,.75f), .26f, .12f);
            Pbr(Chapter("UpperWall"), "painted_plaster_wall", new Color(.77f,.72f,.59f), .12f, 0, .22f);
            Pbr(Chapter("LowerWall"), "painted_plaster_wall", new Color(.23f,.32f,.25f), .24f, 0, .17f);
            Pbr(Chapter("Cream"), "painted_plaster_wall", new Color(.84f,.79f,.65f), .32f, .04f, .08f);
            Pbr(Chapter("Concrete"), "concrete_floor_02", new Color(.76f,.77f,.74f), .12f, 0, .30f);
            Pbr(Chapter("Linoleum"), "concrete_floor_02", new Color(.79f,.78f,.71f), .91f, 0, .065f);
            // The lab's sealed floor has gentler colour variation than outdoor unfinished concrete.
            string source=Root+"Textures/concrete_floor_02/concrete_floor_02_diff_2k.jpg", quiet=Root+"Textures/concrete_floor_02/V10_LabSealedFloor.png";
            var image=Map(source,false,false,true);var colors=image.GetPixels32();
            for(int i=0;i<colors.Length;i++)
            {var c=colors[i];colors[i]=new Color32((byte)(150*.65f+c.r*.35f),(byte)(150*.65f+c.g*.35f),(byte)(150*.65f+c.b*.35f),255);}
            var result=new Texture2D(image.width,image.height,TextureFormat.RGBA32,false);result.SetPixels32(colors);result.Apply();
            var bytes=result.EncodeToPNG();if(!File.Exists(quiet)||!File.ReadAllBytes(quiet).SequenceEqual(bytes))File.WriteAllBytes(quiet,bytes);
            Object.DestroyImmediate(result);AssetDatabase.ImportAsset(quiet);Map(source);Chapter("Linoleum").SetTexture("_BaseMap",Map(quiet));EditorUtility.SetDirty(Chapter("Linoleum"));
            Chapter("Steel").SetColor("_BaseColor", new Color(.60f,.63f,.61f)); Chapter("Steel").SetFloat("_Smoothness", .48f); Chapter("Steel").SetFloat("_Metallic", .65f); EditorUtility.SetDirty(Chapter("Steel"));
            Chapter("Stripe").SetColor("_BaseColor", new Color(.48f,.48f,.40f)); Chapter("Stripe").SetFloat("_Smoothness", .05f); Chapter("Stripe").SetColor("_EmissionColor", new Color(.025f,.025f,.018f)); EditorUtility.SetDirty(Chapter("Stripe"));
            var cabinet=NewMaterial("V10_CabinetEnamel","green_metal_rust",new Color(1.25f,1.20f,1.04f),.42f,.08f,.5f);
            var meterPaper=NewMaterial("V10_MeterPaper","painted_plaster_wall",new Color(.46f,.44f,.35f),.08f,0,.02f);
            var fieldSteel=NewMaterial("V10_FieldSteel","concrete_floor_02",new Color(.9f,.92f,.91f),.55f,.55f,.07f);
            foreach(var r in Object.FindObjectsByType<MeshRenderer>(FindObjectsSortMode.None))
            {
                bool field=r.transform.position.z < -14;
                if(!field)continue;
                r.sharedMaterials=r.sharedMaterials.Select(m=> m==Chapter("Green") && (r.transform.IsChildOf(GameObject.Find("CH09_FieldCabinet").transform)) ? cabinet : m==Chapter("Paper") && r.transform.IsChildOf(GameObject.Find("CH09_FieldCabinet").transform) ? meterPaper : m==Chapter("Steel") ? fieldSteel : m).ToArray();
            }
            var ground = NewMaterial("V10_Desert", "dry_ground_rocks", new Color(.64f,.65f,.62f), .06f, 0, .7f);
            ground.SetTextureScale("_BaseMap", new Vector2(45,30));
            var desert = GameObject.Find("Desert"); if (desert) desert.GetComponent<Renderer>().sharedMaterial = ground;
            var asphalt=NewMaterial("V10_ServiceAsphalt","concrete_floor_02",new Color(.19f,.20f,.20f),.08f,0,.18f);asphalt.SetTextureScale("_BaseMap",new Vector2(3,28));var road=GameObject.Find("ServiceRoad");if(road)road.GetComponent<Renderer>().sharedMaterial=asphalt;
            var joint = NewMaterial("V10_Grout", "concrete_floor_02", new Color(.18f,.17f,.14f), .08f);
            for (float x=-3.5f;x<4;x+=.5f) Box("V10.LabTileJoint",new Vector3(x,.003f,10.75f),new Vector3(.005f,.004f,6.45f),joint);
            for (float z=8;z<14;z+=.5f) Box("V10.LabTileJoint",new Vector3(0,.003f,z),new Vector3(7.85f,.004f,.005f),joint);
            foreach (var r in Object.FindObjectsByType<MeshRenderer>(FindObjectsSortMode.None))
            {
                // Depth-normal AO needs reliable solid surfaces. Labels retain their font material.
                if (r.GetComponent<TextMesh>()) continue;
                if (r.name.StartsWith("CH09.")) r.receiveShadows = true;
            }
        }
        static void Lighting()
        {
            RenderSettings.ambientLight = new Color(.17f,.21f,.28f);
            RenderSettings.fogColor = new Color(.027f,.042f,.066f); RenderSettings.fogDensity = .0035f;
            if (RenderSettings.skybox) { RenderSettings.skybox.SetFloat("_Exposure", .075f); RenderSettings.skybox.SetColor("_Tint",new Color(.34f,.46f,.66f)); EditorUtility.SetDirty(RenderSettings.skybox); }
            Shadows("CH09.LabEntrancePractical", 5.8f); Shadows("CH09.LabSinkPractical", 5.0f);
            Shadows("CH09.WetBenchSafelight", 3.6f); Shadows("CH09.ArchiveTaskLight", 1.65f);
            Shadows("CH09.ExperimentLight", 3.0f); Shadows("CH09.PathLamp", 18f); Shadows("CH09.B12ReadLight", 5.5f);
            Shadows("AmberWorkLight06"); Shadows("CoolWorkLight06"); Shadows("DeskLamp", 1.4f);
            var path=GameObject.Find("CH09.PathLamp").GetComponent<Light>();
            path.transform.LookAt(new Vector3(10.5f,.15f,-21f));path.range=8;path.spotAngle=110;path.innerSpotAngle=55;
            var read=GameObject.Find("CH09.B12ReadLight").GetComponent<Light>();
            read.transform.position=new Vector3(12.17f,2.7f,-19.25f); read.transform.LookAt(new Vector3(11.67f,.65f,-20.14f));
            read.color=new Color(1f,.69f,.35f); read.spotAngle=95; read.innerSpotAngle=60;
            foreach(string name in new[]{"CH09.B12ReadLightHood","CH09.B12ReadLightPole"}) GameObject.Find(name).transform.position+=new Vector3(0,0,1.3f);
            for(int i=0;i<7;i++)
            {
                float angle=(30+i*20)*Mathf.Deg2Rad;
                var tick=Box("V10.VoltmeterGraduation",new Vector3(11.52f+Mathf.Cos(angle)*.073f,.914f+Mathf.Sin(angle)*.068f,-20.091f),new Vector3(.003f,i%3==0?.015f:.008f,.002f),Chapter("Rubber"));
                tick.transform.rotation=Quaternion.Euler(0,0,90-(30+i*20));
            }
            var identifier=GameObject.Find("CH09.B12CabinetIdentifier"); identifier.transform.position=new Vector3(11.87f,.94f,-20.139f);
            var sink=GameObject.Find("CH09.LabSinkPractical").GetComponent<Light>();
            sink.transform.position=new Vector3(-.85f,2.88f,12.2f);sink.transform.LookAt(new Vector3(0,.65f,12.9f));sink.intensity=2.7f;sink.spotAngle=118;
            var sinkPartner=Object.Instantiate(sink.gameObject,root);sinkPartner.name="V10.SinkRightTubeLight";sinkPartner.transform.position=new Vector3(.85f,2.88f,12.2f);sinkPartner.transform.LookAt(new Vector3(0,.65f,12.9f));
            var entrance=GameObject.Find("CH09.LabEntrancePractical").GetComponent<Light>();entrance.transform.position=new Vector3(-.85f,2.88f,9.4f);entrance.transform.LookAt(new Vector3(0,.3f,10.2f));entrance.intensity=3f;entrance.spotAngle=118;
            var entrancePartner=Object.Instantiate(entrance.gameObject,root);entrancePartner.name="V10.EntranceRightTubeLight";entrancePartner.transform.position=new Vector3(.85f,2.88f,9.4f);entrancePartner.transform.LookAt(new Vector3(0,.3f,10.2f));
            foreach(float x in new[]{-.85f,.85f})
            {
                Box("V10.SinkCeilingFixture",new Vector3(x,3.08f,12.2f),new Vector3(.20f,.12f,.92f),Chapter("Steel"));
                Box("V10.SinkCeilingDiffuser",new Vector3(x,3.012f,12.2f),new Vector3(.14f,.025f,.79f),Chapter("WarmDiffuser"));
            }
            var bounce=new GameObject("V10.LabFluorescentBounce");bounce.transform.SetParent(root);bounce.transform.position=new Vector3(0,2.5f,10.6f);
            var fill=bounce.AddComponent<Light>();fill.type=LightType.Point;fill.color=new Color(1f,.87f,.69f);fill.intensity=.65f;fill.range=5.5f;fill.shadows=LightShadows.None;
            // A wallward pool comes from the same safelight, without another shadow atlas face.
            var red = new GameObject("V10.SafelightWallBounce"); red.transform.SetParent(root); red.transform.position = new Vector3(-3.57f,2.23f,12.35f);
            var l = red.AddComponent<Light>(); l.type=LightType.Point; l.color=new Color(1,.12f,.045f); l.intensity=.50f; l.range=3.8f;
            Probe("V10.LabReflection", new Vector3(0,1.7f,10.8f),new Vector3(8,3.4f,6.5f));
            Probe("V10.ControlReflection", new Vector3(0,1.7f,1),new Vector3(18.8f,3.5f,13));
            Probe("V10.B12Reflection", new Vector3(10.5f,1.6f,-20.5f),new Vector3(6,4,8));
        }
        static void DryGrass()
        {
            const string dir=Root+"Textures/grass_medium_02/";
            var diffuse=Map(dir+"grass_medium_02_dry_diff_1k.png",false,false,true);
            var alpha=Map(dir+"grass_medium_02_alpha_1k.png",false,true,true);
            var pixels=diffuse.GetPixels32();var mask=alpha.GetPixels32();
            for(int i=0;i<pixels.Length;i++)pixels[i].a=mask[i].r;
            var atlas=new Texture2D(diffuse.width,diffuse.height,TextureFormat.RGBA32,false);atlas.SetPixels32(pixels);atlas.Apply();
            string output=dir+"V10_DryGrass_RGBA.png";var bytes=atlas.EncodeToPNG();if(!File.Exists(output)||!File.ReadAllBytes(output).SequenceEqual(bytes))File.WriteAllBytes(output,bytes);
            Object.DestroyImmediate(atlas);AssetDatabase.ImportAsset(output);Map(dir+"grass_medium_02_dry_diff_1k.png");Map(dir+"grass_medium_02_alpha_1k.png",false,true);
            string materialPath=Root+"V10_DryGrass_Atlas.mat";var mat=AssetDatabase.LoadAssetAtPath<Material>(materialPath);
            if(!mat){mat=new Material(Shader.Find("Universal Render Pipeline/Lit"));AssetDatabase.CreateAsset(mat,materialPath);}
            mat.SetTexture("_BaseMap",Map(output));mat.SetColor("_BaseColor",new Color(.95f,.91f,.79f));mat.SetFloat("_AlphaClip",1);mat.SetFloat("_Cutoff",.4f);mat.EnableKeyword("_ALPHATEST_ON");
            mat.SetFloat("_Cull",0);mat.SetFloat("_Smoothness",.05f);mat.renderQueue=2450;mat.SetOverrideTag("RenderType","TransparentCutout");EditorUtility.SetDirty(mat);
            var rng=new System.Random(1047);
            for(int i=0;i<170;i++)
            {
                Vector2[] centers={new Vector2(7.5f,-19),new Vector2(13.7f,-21.3f),new Vector2(7,-24.3f),new Vector2(10.5f,-25.3f),new Vector2(14.8f,-16),new Vector2(5.5f,-16.5f)};
                var center=centers[i%centers.Length];float angle=(float)rng.NextDouble()*Mathf.PI*2,radius=Mathf.Sqrt((float)rng.NextDouble())*1.7f;
                float x=center.x+Mathf.Cos(angle)*radius,z=center.y+Mathf.Sin(angle)*radius;
                if(x>8.05f&&x<13.2f&&z>-24)continue;
                string name="V10_DryTuft_"+(char)('A'+i%3);var asset=AssetDatabase.LoadAssetAtPath<GameObject>(Root+"Models/"+name+".fbx");
                // Single-mesh FBX roots carry the centimetre/axis conversion. Preserve them.
                var placement=new GameObject("V10.DryTuftPlacement").transform;placement.SetParent(root,false);placement.position=new Vector3(x,-.103f,z);placement.rotation=Quaternion.Euler(0,(float)rng.NextDouble()*360,0);placement.localScale=Vector3.one*(.9f+(float)rng.NextDouble()*.75f);
                var go=(GameObject)PrefabUtility.InstantiatePrefab(asset);go.transform.SetParent(placement,false);
                foreach(var renderer in go.GetComponentsInChildren<Renderer>())
                {
                    renderer.sharedMaterial=mat;renderer.shadowCastingMode=ShadowCastingMode.TwoSided;
                    if(renderer.bounds.size.y<.10f||renderer.bounds.size.y>.8f)throw new InvalidDataException("Dry tuft imported height outside physical budget: "+renderer.bounds);
                    if(i<5)Debug.Log("VISUAL10_TUFT_WORLD_BOUNDS "+renderer.bounds);
                }
            }
        }
        static void ExteriorDetail()
        {
            // Correct only the reversed inside surface, preserving pivots, silhouette and moving feed supports.
            var corrected=AssetDatabase.LoadAssetAtPath<GameObject>(Root+"Models/V10_RadioDish_BowlCorrected.fbx");
            var bowl=corrected.GetComponentInChildren<MeshFilter>().sharedMesh;
            var enamel=NewMaterial("V10_DishEnamel","painted_plaster_wall",new Color(.68f,.72f,.72f),.22f,0,.10f);enamel.SetFloat("_Cull",0);EditorUtility.SetDirty(enamel);
            int correctedCount=0;
            foreach(var filter in Object.FindObjectsByType<MeshFilter>(FindObjectsSortMode.None))
                if(filter.name.EndsWith("__DishBowl")) {filter.sharedMesh=bowl;filter.GetComponent<Renderer>().sharedMaterial=enamel;correctedCount++;}
            Debug.Log("VISUAL10_DISH_INSIDE_NORMALS count="+correctedCount+" vertices="+bowl.vertexCount);
            var vertices=new System.Collections.Generic.List<Vector3>(); var triangles=new System.Collections.Generic.List<int>();
            var rng=new System.Random(4710);
            void Triangle(Vector3 a,Vector3 b,Vector3 c)
            { int n=vertices.Count; vertices.Add(a);vertices.Add(b);vertices.Add(c); triangles.Add(n);triangles.Add(n+1);triangles.Add(n+2); }
            foreach(var r in Object.FindObjectsByType<MeshRenderer>(FindObjectsSortMode.None)) if(r.name.StartsWith("Mesa_")) r.enabled=false;
            // Continuous low mesa profile replaces the retained early cube skyline.
            Vector2[] ridge={new Vector2(-150,2),new Vector2(-123,3),new Vector2(-102,5),new Vector2(-90,5.3f),new Vector2(-79,2.2f),new Vector2(-61,2),new Vector2(-46,4.2f),new Vector2(-28,4.5f),new Vector2(-16,2.8f),new Vector2(4,2),new Vector2(21,3.5f),new Vector2(45,3.2f),new Vector2(67,4.8f),new Vector2(82,4.6f),new Vector2(99,2),new Vector2(150,1.5f)};
            for(int i=1;i<ridge.Length;i++)
            {
                Vector3 a=new Vector3(ridge[i-1].x,-2,-119),b=new Vector3(ridge[i].x,-2,-119);
                Vector3 c=new Vector3(ridge[i].x,ridge[i].y,-119),d=new Vector3(ridge[i-1].x,ridge[i-1].y,-119);
                Triangle(a,b,c);Triangle(a,c,d);
            }
            var horizonMesh=new Mesh{name="V10_MesaHorizon"};horizonMesh.SetVertices(vertices);horizonMesh.SetTriangles(triangles,0);horizonMesh.RecalculateNormals();horizonMesh.RecalculateBounds();
            string horizonPath=Root+"V10_MesaHorizon.asset";var horizonAsset=AssetDatabase.LoadAssetAtPath<Mesh>(horizonPath);
            if(horizonAsset){EditorUtility.CopySerialized(horizonMesh,horizonAsset);Object.DestroyImmediate(horizonMesh);horizonMesh=horizonAsset;}else AssetDatabase.CreateAsset(horizonMesh,horizonPath);
            var horizon=new GameObject("V10.LowMesaHorizon");horizon.transform.SetParent(root);horizon.AddComponent<MeshFilter>().sharedMesh=horizonMesh;
            var horizonMat=NewMaterial("V10_DistantMesa","painted_plaster_wall",new Color(.10f,.12f,.16f),.02f);horizonMat.SetFloat("_Cull",0);EditorUtility.SetDirty(horizonMat);horizon.AddComponent<MeshRenderer>().sharedMaterial=horizonMat;
            vertices.Clear();triangles.Clear();
            // Sparse creosote-like dry branching plants around the slab, leaving all walking space clear.
            for(int i=0;i<95;i++)
            {
                float x=4+(float)rng.NextDouble()*13,z=-13-(float)rng.NextDouble()*15;
                if(x>8.1f&&x<13.1f&&z>-24) continue;
                var origin=new Vector3(x,-.10f,z); float h=.16f+(float)rng.NextDouble()*.28f;
                for(int j=0;j<15;j++)
                {
                    float angle=(float)rng.NextDouble()*Mathf.PI*2;
                    var tip=origin+new Vector3(Mathf.Cos(angle)*h*.65f,h*(.4f+(float)rng.NextDouble()*.6f),Mathf.Sin(angle)*h*.65f);
                    var side=new Vector3(Mathf.Sin(angle),0,-Mathf.Cos(angle))*.009f;
                    Triangle(origin-side,origin+side,tip);
                    var branch=Vector3.Lerp(origin,tip,.56f);Triangle(branch-side,branch+side,tip+new Vector3(.06f,.035f,.04f));
                }
            }
            var mesh=new Mesh{name="V10_DryScrub"};mesh.SetVertices(vertices);mesh.SetTriangles(triangles,0);mesh.RecalculateNormals();mesh.RecalculateBounds();
            string path=Root+"V10_DryScrub.asset";var old=AssetDatabase.LoadAssetAtPath<Mesh>(path);
            if(old){EditorUtility.CopySerialized(mesh,old);Object.DestroyImmediate(mesh);mesh=old;}else AssetDatabase.CreateAsset(mesh,path);
            var scrub=new GameObject("V10.DryDesertPlants");scrub.transform.SetParent(root);scrub.AddComponent<MeshFilter>().sharedMesh=mesh;
            var stem=NewMaterial("V10_DryStem","painted_plaster_wall",new Color(.37f,.32f,.21f),.06f);stem.SetFloat("_Cull",0);EditorUtility.SetDirty(stem);var legacyScrubRenderer=scrub.AddComponent<MeshRenderer>();legacyScrubRenderer.sharedMaterial=stem;legacyScrubRenderer.enabled=false; // Replaced by photographed CC0 dry grass geometry; retain source mesh for history.
            var oldScrub=GameObject.Find("DesertScrubRoot");if(oldScrub)foreach(var r in oldScrub.GetComponentsInChildren<Renderer>())r.enabled=false;
            // Actual sky geometry, never a photographic overlay: deterministic dim stellar points.
            vertices.Clear();triangles.Clear();
            for(int i=0;i<650;i++)
            {
                float a=(float)rng.NextDouble()*Mathf.PI*2,e=.08f+(float)rng.NextDouble()*1.40f;
                var dir=new Vector3(Mathf.Cos(a)*Mathf.Cos(e),Mathf.Sin(e),Mathf.Sin(a)*Mathf.Cos(e));
                var center=dir*160;float size=.035f+(float)rng.NextDouble()*.095f;
                var side=Vector3.Cross(dir,Vector3.up).normalized*size;var up=Vector3.Cross(dir,side).normalized*size;
                Triangle(center-side-up,center+side-up,center+side+up);Triangle(center-side-up,center+side+up,center-side+up);
            }
            var stars=new Mesh{name="V10_Stars"};stars.SetVertices(vertices);stars.SetTriangles(triangles,0);stars.RecalculateBounds();
            path=Root+"V10_Stars.asset";old=AssetDatabase.LoadAssetAtPath<Mesh>(path);
            if(old){EditorUtility.CopySerialized(stars,old);Object.DestroyImmediate(stars);stars=old;}else AssetDatabase.CreateAsset(stars,path);
            var starMaterial=AssetDatabase.LoadAssetAtPath<Material>(Root+"V10_Stars.mat");
            if(!starMaterial){starMaterial=new Material(Shader.Find("Universal Render Pipeline/Unlit"));AssetDatabase.CreateAsset(starMaterial,Root+"V10_Stars.mat");}
            starMaterial.SetFloat("_Cull",0);starMaterial.SetColor("_BaseColor",new Color(.63f,.72f,.86f));EditorUtility.SetDirty(starMaterial);
            var sky=new GameObject("V10.NightStars");sky.transform.SetParent(root);sky.AddComponent<MeshFilter>().sharedMesh=stars;
            var sr=sky.AddComponent<MeshRenderer>();sr.sharedMaterial=starMaterial;sr.shadowCastingMode=ShadowCastingMode.Off;sr.receiveShadows=false;
        }
        static GameObject Model(string name, Vector3 position, float yaw = 0)
        {
            var asset = AssetDatabase.LoadAssetAtPath<GameObject>(Root + "Models/" + name + ".fbx");
            if (!asset) throw new FileNotFoundException(name);
            var placement=new GameObject(name+".Placement");placement.transform.SetParent(root,false);
            placement.transform.SetPositionAndRotation(position,Quaternion.Euler(0,yaw,0));
            var go = (GameObject)PrefabUtility.InstantiatePrefab(asset); go.transform.SetParent(placement.transform,false);
            foreach(var r in go.GetComponentsInChildren<Renderer>())
            {
                r.sharedMaterials = r.sharedMaterials.Select(m => name.Contains("Lamp") && m.name=="CH09_Cream" ? Chapter(name=="V10_WallLamp"?"RedDiffuser":"WarmDiffuser") : Chapter(m.name.Replace("CH09_", "").Split('.')[0]) ?? Chapter("Green")).ToArray();
                if (name=="V10_FieldLampShade" && position.z < 0 && r.sharedMaterials.Any(m=>m==Chapter("WarmDiffuser")))
                {
                    var response=r.gameObject.AddComponent<Signal47.Audio.PracticalLampEmission>();
                    response.source=GameObject.Find("CH09.ExperimentLight").GetComponent<Light>();
                }
            }
            var bounds = go.GetComponentsInChildren<Renderer>()[0].bounds;
            foreach(var r in go.GetComponentsInChildren<Renderer>()) bounds.Encapsulate(r.bounds);
            Debug.Log("VISUAL10_MODEL " + name + " bounds=" + bounds);
            return placement;
        }
        static void Hide(params string[] names)
        {
            foreach(var r in Object.FindObjectsByType<MeshRenderer>(FindObjectsSortMode.None))
                if(names.Any(n => r.name == n || r.name.StartsWith(n + "."))) r.enabled = false;
        }
        static void Props()
        {
            Hide("CH09.WetBench", "CH09.ArchiveBench", "CH09.SinkLeg");
            foreach(float x in new[]{-.72f,.72f}) foreach(float z in new[]{13.23f,13.65f})
            {
                var leg=GameObject.CreatePrimitive(PrimitiveType.Cylinder);leg.name="V10.SinkTubularLeg";leg.transform.SetParent(root);leg.transform.position=new Vector3(x,.40f,z);leg.transform.localScale=new Vector3(.033f,.40f,.033f);Object.DestroyImmediate(leg.GetComponent<Collider>());leg.GetComponent<Renderer>().sharedMaterial=Chapter("Steel");
                Box("V10.SinkFoot",new Vector3(x,.018f,z),new Vector3(.066f,.036f,.066f),Chapter("Rubber"));
            }
            Model("V10_LabBench",new Vector3(-2.77f,0,11.25f),180);
            Model("V10_LabBench",new Vector3(2.77f,0,11.25f));
            Hide("CH09.SafelightHood", "CH09.SafelightBracket", "CH09.SafelightLens");
            Model("V10_WallLamp",new Vector3(-3.88f,2.48f,12.35f),90);
            Hide("CH09.ReferenceLampShroud", "CH09.ReferenceLampDiffuser");
            var fieldLamp=Model("V10_FieldLampShade",new Vector3(9.35f,2.20f,-21.72f));
            fieldLamp.transform.rotation=Quaternion.FromToRotation(Vector3.down,new Vector3(.8f,-1f,.35f));
            Hide("CH09.ArchiveLampShade", "CH09.ArchiveLampDiffuser");
            Model("V10_FieldLampShade",new Vector3(2.91f,2.30f,10.45f));
            var stools=Object.FindObjectsByType<Transform>(FindObjectsSortMode.None).Where(t=>t.name=="CH09_LabStool").ToArray();
            foreach(var stool in stools)stool.position+=new Vector3(0,0,1.7f);
            Model("V10_MetalBin",new Vector3(1.27f,0,13.41f));
            Model("V10_SinkCloth",new Vector3(-.50f,1.02f,13.19f),180);
            foreach(float x in new[]{-3.9f,3.9f})Box("V10.CeilingCoving",new Vector3(x,3.14f,10.75f),new Vector3(.12f,.10f,6.5f),Chapter("Trim"));
            Box("V10.CeilingNorthCoving",new Vector3(0,3.14f,13.85f),new Vector3(7.85f,.10f,.12f),Chapter("Trim"));
            // Wall hardware and utility storage occupy only non-walkable margins.
            Box("V10.LabStorageBody",new Vector3(-2.85f,1.08f,13.65f),new Vector3(1.05f,2.16f,.46f),Chapter("Green"));
            for(int i=0;i<2;i++)
            {
                Box("V10.LabStorageDoor",new Vector3(-3.105f+i*.51f,1.07f,13.405f),new Vector3(.492f,2.07f,.025f),Chapter("Green"));
                Box("V10.LabStorageHandle",new Vector3(-2.91f+i*.12f,1.03f,13.365f),new Vector3(.02f,.18f,.035f),Chapter("Steel"));
            }
        }
        static void LabDetails()
        {
            Hide("CH09.LabCeilingFixture","CH09.LabCeilingDiffuser","V10.SinkCeilingFixture","V10.SinkCeilingDiffuser");
            string tubePath=Root+"V10_FluorescentTube.mat";var tube=AssetDatabase.LoadAssetAtPath<Material>(tubePath);
            if(!tube){tube=new Material(Shader.Find("Universal Render Pipeline/Lit"));AssetDatabase.CreateAsset(tube,tubePath);}
            tube.SetColor("_BaseColor",new Color(.95f,.94f,.85f));tube.EnableKeyword("_EMISSION");tube.SetColor("_EmissionColor",new Color(2.2f,2.0f,1.65f));tube.SetFloat("_Smoothness",.3f);EditorUtility.SetDirty(tube);
            foreach(float x in new[]{-.85f,.85f})foreach(float z in new[]{9.4f,12.2f})
            {var fixture=Model("V10_TwinFluorescent",new Vector3(x,2.97f,z));foreach(var renderer in fixture.GetComponentsInChildren<Renderer>())renderer.sharedMaterials=renderer.sharedMaterials.Select(m=>m==Chapter("WarmDiffuser")?tube:m).ToArray();}
            Model("V10_MeasuringJug",new Vector3(-2.85f,.965f,12.76f),25);
            Model("V10_ChemicalFunnel",new Vector3(-3.10f,.965f,12.73f),-15);
            Model("V10_PrintTongs",new Vector3(-2.31f,.965f,12.15f),35);
        }
        static void Audio(GameSession session)
        {
            AudioClip Clip(string name)
            {
                var path = Root+"Audio/"+name+".ogg";
                var importer = (AudioImporter)AssetImporter.GetAtPath(path);
                importer.forceToMono=true; var settings=importer.defaultSampleSettings;
                settings.loadType=AudioClipLoadType.DecompressOnLoad; importer.defaultSampleSettings=settings; importer.SaveAndReimport();
                return AssetDatabase.LoadAssetAtPath<AudioClip>(path);
            }
            session.chapter.filmHandlingClip=Clip("impactSoft_medium_000");
            session.chapter.mechanicalClip=Clip("impactMetal_light_000");
            var player=Object.FindFirstObjectByType<Signal47.Player.FirstPersonController>();
            var steps=player.gameObject.AddComponent<Signal47.Audio.SurfaceFootsteps>();
            steps.clips=new[]{Clip("footstep_concrete_000"),Clip("footstep_concrete_001"),Clip("footstep_concrete_002")};
        }
        public static void Dress(GameSession session)
        {
            if (GameObject.Find("Visual10Environment")) return;
            // The legacy world pass otherwise runs on sceneSaving, after all visual changes,
            // resetting desert material, sky and fog. Order it explicitly and idempotently.
            if(!GameObject.Find("WorldAreaArt")) WorldAreaPass.Dress(Object.FindFirstObjectByType<Signal47.Environment.DishArrayController>());
            root = new GameObject("Visual10Environment").transform;
            Materials(); Props(); Rendering(); Lighting(); LabDetails(); ExteriorDetail(); DryGrass(); Audio(session);
            foreach(var r in root.GetComponentsInChildren<Renderer>()) GameObjectUtility.SetStaticEditorFlags(r.gameObject, StaticEditorFlags.BatchingStatic);
            Debug.Log("VISUAL10_APPLIED PBR CC0 / local shadows / contact occlusion / practical reflections");
        }
    }
}
#endif
