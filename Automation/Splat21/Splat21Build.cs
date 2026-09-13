using System;
using System.IO;
using System.Reflection;
using GaussianSplatting.Editor;
using GaussianSplatting.Runtime;
using UnityEditor;
using UnityEditor.Build.Reporting;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;

public static class Splat21Build
{
    const string Package = "Packages/org.nesnausk.gaussian-splatting/";
    static GaussianSplatAsset Import(string name)
    {
        var creator = ScriptableObject.CreateInstance<GaussianSplatAssetCreator>();
        var flags = BindingFlags.NonPublic | BindingFlags.Instance;
        var type = creator.GetType();
        type.GetField("m_InputFile", flags).SetValue(creator, Path.GetFullPath("Assets/Probe/"+name+".ply"));
        type.GetField("m_OutputFolder", flags).SetValue(creator, "Assets/Probe/Imported");
        type.GetField("m_ImportCameras", flags).SetValue(creator, false);
        foreach (var f in new[]{"m_FormatPos", "m_FormatScale", "m_FormatColor", "m_FormatSH"})
            type.GetField(f, flags).SetValue(creator, Enum.ToObject(type.GetField(f, flags).FieldType, 0));
        type.GetMethod("CreateAsset", flags).Invoke(creator, null);
        UnityEngine.Object.DestroyImmediate(creator);
        var asset = AssetDatabase.LoadAssetAtPath<GaussianSplatAsset>("Assets/Probe/Imported/"+name+".asset");
        if (asset == null) throw new Exception("Import failed: "+name);
        return asset;
    }
    public static void Build()
    {
        var small = Import("fixture"); var dense = Import("grid");
        if (small.splatCount != 784 || dense.splatCount != 50176) throw new Exception("Splat count mismatch");
        var pipeline = AssetDatabase.LoadAssetAtPath<UniversalRenderPipelineAsset>("Assets/Signal47/Art/PrototypePort/Signal47_URP.asset");
        pipeline.msaaSampleCount = 1;
        pipeline.supportsHDR = true;
        GraphicsSettings.defaultRenderPipeline = pipeline;
        QualitySettings.renderPipeline = pipeline;
        var renderer = AssetDatabase.LoadAssetAtPath<UniversalRendererData>("Assets/UniversalRenderer.asset");
        var featureType = typeof(GaussianSplatRenderer).Assembly.GetType("GaussianSplatting.Runtime.GaussianSplatURPFeature", true);
        foreach (var old in renderer.rendererFeatures.ToArray())
            if (old != null && old.GetType() == featureType) { renderer.rendererFeatures.Remove(old); UnityEngine.Object.DestroyImmediate(old, true); }
        var feature = (ScriptableRendererFeature)ScriptableObject.CreateInstance(featureType);
        feature.name = "Splat21 isolated probe";
        AssetDatabase.AddObjectToAsset(feature, renderer);
        renderer.rendererFeatures.Add(feature);
        EditorUtility.SetDirty(renderer); EditorUtility.SetDirty(pipeline);
        PlayerSettings.SetUseDefaultGraphicsAPIs(BuildTarget.StandaloneLinux64, false);
        PlayerSettings.SetGraphicsAPIs(BuildTarget.StandaloneLinux64, new[]{GraphicsDeviceType.Vulkan});
        PlayerSettings.productName = "SIGNAL47 Splat21 Isolated Probe";
        PlayerSettings.companyName = "SIGNAL47Tests";
        PlayerSettings.fullScreenMode = FullScreenMode.Windowed;
        PlayerSettings.defaultScreenWidth = 1280; PlayerSettings.defaultScreenHeight = 800;
        PlayerSettings.runInBackground = true;
        var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);
        var camera = new GameObject("Probe camera").AddComponent<Camera>();
        camera.tag = "MainCamera"; camera.clearFlags = CameraClearFlags.SolidColor;
        camera.backgroundColor = new Color(.025f,.035f,.05f); camera.nearClipPlane = .1f; camera.farClipPlane = 150;
        camera.allowMSAA = false; camera.allowHDR = true;
        camera.gameObject.AddComponent<UniversalAdditionalCameraData>();
        var go = new GameObject("Original synthetic splats"); go.SetActive(false);
        go.transform.rotation = Quaternion.Euler(0,0,180);
        var gs = go.AddComponent<GaussianSplatRenderer>(); gs.m_Asset = small; gs.m_SHOrder = 0;
        gs.m_ShaderSplats = AssetDatabase.LoadAssetAtPath<Shader>(Package+"Shaders/RenderGaussianSplats.shader");
        gs.m_ShaderComposite = AssetDatabase.LoadAssetAtPath<Shader>(Package+"Shaders/GaussianComposite.shader");
        gs.m_ShaderDebugPoints = AssetDatabase.LoadAssetAtPath<Shader>(Package+"Shaders/GaussianDebugRenderPoints.shader");
        gs.m_ShaderDebugBoxes = AssetDatabase.LoadAssetAtPath<Shader>(Package+"Shaders/GaussianDebugRenderBoxes.shader");
        gs.m_CSSplatUtilities = AssetDatabase.LoadAssetAtPath<ComputeShader>(Package+"Shaders/SplatUtilities.compute");
        var blocker = GameObject.CreatePrimitive(PrimitiveType.Cube); blocker.name = "Opaque depth reference";
        blocker.transform.position = new Vector3(.22f,.4f,-.6f); blocker.transform.localScale = new Vector3(.22f,.8f,.12f);
        var mat = AssetDatabase.LoadAssetAtPath<Material>("Assets/Probe/Occluder.mat");
        if (mat == null) { mat = new Material(Shader.Find("Universal Render Pipeline/Unlit")); AssetDatabase.CreateAsset(mat,"Assets/Probe/Occluder.mat"); }
        mat.color = new Color(.12f,.65f,.75f); EditorUtility.SetDirty(mat); blocker.GetComponent<Renderer>().sharedMaterial = mat;
        var runner = new GameObject("Probe runner").AddComponent<Splat21Runtime>();
        runner.splats=gs; runner.small=small; runner.dense=dense; runner.cam=camera; runner.occluder=blocker;
        AssetDatabase.SaveAssets(); EditorSceneManager.SaveScene(scene,"Assets/Probe/Splat21.unity");
        var report=BuildPipeline.BuildPlayer(new BuildPlayerOptions{scenes=new[]{"Assets/Probe/Splat21.unity"},locationPathName="../Player/Splat21.x86_64",target=BuildTarget.StandaloneLinux64, options=BuildOptions.None});
        File.WriteAllText("../build-result.json",JsonUtility.ToJson(new ProbeBuildResult{result=report.summary.result.ToString(),errors=(int)report.summary.totalErrors,small=small.splatCount,dense=dense.splatCount},true));
        if(report.summary.result!=BuildResult.Succeeded)throw new Exception("Build failed");
    }
    [Serializable] class ProbeBuildResult{public string result;public int errors,small,dense;}
}
