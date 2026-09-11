#if UNITY_EDITOR
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.Rendering;
using Unity.Collections;
using Object=UnityEngine.Object;

namespace Signal47.Editor
{
    // CPU light baking needs a graphics device for the material Meta pass.
    // Run in a batch editor with DISPLAY, without -nographics. Only the fixed lab is baked.
    public static class Visual10LightBake
    {
        const string Root="Assets/Signal47/Art/Visual10/";
        static float InvalidUvArea(MeshRenderer renderer,out float area)
        {
            var mesh=renderer.GetComponent<MeshFilter>().sharedMesh;area=0;float invalid=0;
            using(var snapshot=MeshUtility.AcquireReadOnlyMeshData(mesh))
            {
                var data=snapshot[0];if(!data.HasVertexAttribute(VertexAttribute.TexCoord1)){var b=renderer.bounds.size;area=2*(b.x*b.y+b.x*b.z+b.y*b.z);return 1;}
                using(var uv=new NativeArray<Vector2>(data.vertexCount,Allocator.Temp))
                using(var vertices=new NativeArray<Vector3>(data.vertexCount,Allocator.Temp))
                {
                    data.GetUVs(1,uv);data.GetVertices(vertices);
                    for(int sub=0;sub<data.subMeshCount;sub++)using(var indices=new NativeArray<int>(data.GetSubMesh(sub).indexCount,Allocator.Temp))
                    {
                        data.GetIndices(indices,sub,true);
                        for(int i=0;i+2<indices.Length;i+=3)
                        {
                            int a=indices[i],b=indices[i+1],c=indices[i+2];
                            var ab=renderer.transform.TransformVector(vertices[b]-vertices[a]);var ac=renderer.transform.TransformVector(vertices[c]-vertices[a]);float triangle=Vector3.Cross(ab,ac).magnitude*.5f;
                            if(triangle<1e-10f)continue;area+=triangle;
                            var u=uv[b]-uv[a];var v=uv[c]-uv[a];float uvArea=Mathf.Abs(u.x*v.y-u.y*v.x);
                            if(float.IsNaN(uvArea)||float.IsInfinity(uvArea)||uvArea<1e-12f)invalid+=triangle;
                        }
                    }
                }
            }
            return area>0?invalid/area:1;
        }
        static void ProbeFallback(MeshRenderer r,string reason)
        {
            GameObjectUtility.SetStaticEditorFlags(r.gameObject,GameObjectUtility.GetStaticEditorFlags(r.gameObject)&~StaticEditorFlags.ContributeGI);
            r.receiveGI=ReceiveGI.LightProbes;r.lightProbeUsage=LightProbeUsage.BlendProbes;
            Debug.Log("VISUAL10_PROBE_FALLBACK "+r.name+" "+reason);
        }
        public static void Build()
        {
            if(SystemInfo.graphicsDeviceType==GraphicsDeviceType.Null)throw new InvalidOperationException("Lab bake needs a real editor graphics device.");
            // Only fixed laboratory FBX assets need a separate packed lightmap UV channel.
            foreach(var folder in new[]{"Assets/Signal47/Art/Chapter09",Root+"Models"})
                foreach(var path in Directory.GetFiles(folder,"*.fbx"))
                {
                    string name=Path.GetFileNameWithoutExtension(path);
                    if(name.Contains("FieldCabinet")||name.Contains("DryTuft")||name.Contains("RadioDish"))continue;
                    var importer=AssetImporter.GetAtPath(path) as ModelImporter;
                    if(importer&&(!importer.generateSecondaryUV||importer.secondaryUVMarginMethod!=ModelImporterSecondaryUVMarginMethod.Manual||importer.secondaryUVPackMargin!=2))
                    {importer.generateSecondaryUV=true;importer.secondaryUVMarginMethod=ModelImporterSecondaryUVMarginMethod.Manual;importer.secondaryUVPackMargin=2;importer.SaveAndReimport();}
                }
            Signal47SceneBuilder.BuildVerticalSlice();
            var settings=AssetDatabase.LoadAssetAtPath<LightingSettings>(Root+"LabLighting.lighting");
            if(!settings){settings=new LightingSettings();AssetDatabase.CreateAsset(settings,Root+"LabLighting.lighting");}
            settings.bakedGI=true;settings.realtimeGI=false;settings.lightmapper=LightingSettings.Lightmapper.ProgressiveCPU;
            settings.lightmapResolution=10;settings.lightmapMaxSize=512;settings.lightmapPadding=4;
            settings.directSampleCount=32;settings.indirectSampleCount=128;settings.environmentSampleCount=32;settings.maxBounces=2;
            settings.directionalityMode=LightmapsMode.CombinedDirectional;
            Lightmapping.lightingSettings=settings;Lightmapping.giWorkflowMode=Lightmapping.GIWorkflowMode.OnDemand;
            // Unity material validation during a bake consults these flags when rebuilding
            // shader keywords. RealtimeGI is disabled; retain an emissive flag for URP keyword validation, without adding baked sources.
            var emitters=new HashSet<Material>();
            foreach(var renderer in Object.FindObjectsByType<Renderer>(FindObjectsSortMode.None))foreach(var mat in renderer.sharedMaterials)
                if(mat&&mat.HasProperty("_EmissionColor")&&mat.GetColor("_EmissionColor").maxColorComponent>0)
                {mat.globalIlluminationFlags=MaterialGlobalIlluminationFlags.RealtimeEmissive;mat.EnableKeyword("_EMISSION");EditorUtility.SetDirty(mat);emitters.Add(mat);}
            var baked=new Dictionary<MeshRenderer,float>();
            var region=new Bounds(new Vector3(0,1.6f,10.75f),new Vector3(8.5f,3.9f,7.0f));int count=0;
            foreach(var r in Object.FindObjectsByType<MeshRenderer>(FindObjectsSortMode.None))
            {
                if(!r.enabled||!r.gameObject.activeInHierarchy||r.GetComponent<TextMesh>()||!region.Contains(r.bounds.center)||r.name=="CH09.DevelopingPrint")continue;
                // Tiny tools use the room probes and realtime light, avoiding subtexel UV islands.
                if(r.bounds.size.sqrMagnitude<.045f){ProbeFallback(r,"small component");continue;}
                float invalid=InvalidUvArea(r,out float area);
                var extent=r.bounds.size;float boxArea=2*(extent.x*extent.y+extent.x*extent.z+extent.y*extent.z);
                bool slenderMetal=r.sharedMaterials.All(m=>m&&m.name.Contains("_Steel"))&&area/Mathf.Max(.0001f,boxArea)<.20f;
                if(slenderMetal){ProbeFallback(r,"open metal frame, surface="+area+" bounds surface="+boxArea);continue;}
                if(invalid>.02f)
                {
                    if(area<=.15f){ProbeFallback(r,"invalid UV2 fraction="+invalid+" area="+area);continue;}
                    throw new InvalidOperationException("Required lab mesh has invalid UV2: "+r.name+" fraction="+invalid+" area="+area);
                }
                var flags=GameObjectUtility.GetStaticEditorFlags(r.gameObject)|StaticEditorFlags.ContributeGI;
                GameObjectUtility.SetStaticEditorFlags(r.gameObject,flags);r.receiveGI=ReceiveGI.Lightmaps;r.scaleInLightmap=1;count++;baked.Add(r,area);
            }
            var root=new GameObject("V10.BakedLabLighting").transform;
            foreach(float x in new[]{-.85f,.85f})foreach(float z in new[]{9.4f,12.2f})
            {
                var go=new GameObject("V10.BakedFluorescentArea");go.transform.SetParent(root);go.transform.position=new Vector3(x,2.94f,z);go.transform.rotation=Quaternion.LookRotation(Vector3.down,Vector3.forward);
                var light=go.AddComponent<Light>();light.type=LightType.Rectangle;light.lightmapBakeType=LightmapBakeType.Baked;light.areaSize=new Vector2(.21f,.88f);light.color=new Color(1,.90f,.73f);light.intensity=2.0f;light.bounceIntensity=1;
            }
            // The source's diffuse area contribution is baked; a subdued realtime component
            // remains for moving players/tools and specular response.
            foreach(string name in new[]{"CH09.LabEntrancePractical","CH09.LabSinkPractical","V10.EntranceRightTubeLight","V10.SinkRightTubeLight"})
            {var light=GameObject.Find(name).GetComponent<Light>();light.intensity*=.40f;}
            var probeRoot=new GameObject("V10.LabLightProbes");probeRoot.transform.SetParent(root);var group=probeRoot.AddComponent<LightProbeGroup>();var points=new List<Vector3>();
            foreach(float x in new[]{-2f,0,2f})foreach(float y in new[]{.25f,1.35f,2.65f})foreach(float z in new[]{8.3f,10.5f,13.2f})points.Add(new Vector3(x,y,z));group.probePositions=points.ToArray();
            EditorUtility.SetDirty(settings);EditorSceneManager.MarkSceneDirty(EditorSceneManager.GetActiveScene());EditorSceneManager.SaveOpenScenes();AssetDatabase.SaveAssets();
            double start=EditorApplication.timeSinceStartup;Debug.Log("VISUAL10_BAKE_BEGIN renderers="+count+" CPU 10texels/m 512atlas");
            if(!Lightmapping.Bake())throw new InvalidOperationException("Lab CPU bake failed.");
            if(LightmapSettings.lightmaps.Length==0||!Lightmapping.lightingDataAsset)throw new InvalidOperationException("Bake returned without lightmaps/data.");
            foreach(var entry in baked)
            {
                var r=entry.Key;var scale=r.lightmapScaleOffset;
                bool valid=r.lightmapIndex>=0&&r.lightmapIndex<LightmapSettings.lightmaps.Length&&LightmapSettings.lightmaps[r.lightmapIndex].lightmapColor&&scale.x>0&&scale.y>0&&!float.IsInfinity(scale.x)&&!float.IsInfinity(scale.y);
                if(!valid){if(entry.Value<=.15f)ProbeFallback(r,"subtexel chart omitted from atlas");else throw new InvalidOperationException("Required baked renderer has no valid atlas: "+r.name);}
            }
            foreach(var mat in emitters){mat.globalIlluminationFlags=MaterialGlobalIlluminationFlags.RealtimeEmissive;mat.EnableKeyword("_EMISSION");EditorUtility.SetDirty(mat);}
            Debug.Log("VISUAL10_BAKE_PASS seconds="+(EditorApplication.timeSinceStartup-start)+" maps="+LightmapSettings.lightmaps.Length+" data="+AssetDatabase.GetAssetPath(Lightmapping.lightingDataAsset));
            EditorSceneManager.SaveOpenScenes();AssetDatabase.SaveAssets();
            foreach(var mat in emitters)
            {
                BaseShaderGUI.SetMaterialKeywords(mat);
                if(!mat.IsKeywordEnabled("_EMISSION"))throw new InvalidOperationException("URP validation disabled visible emitter: "+mat.name);
            }
            AssetDatabase.SaveAssets();
            Automation.BuildCurrentGauntletLinux();
            foreach(var mat in emitters)if(!mat.IsKeywordEnabled("_EMISSION"))throw new InvalidOperationException("Build disabled visible emitter: "+mat.name);
            Debug.Log("VISUAL10_EMISSION_VALIDATED count="+emitters.Count);
        }
    }
}
#endif
