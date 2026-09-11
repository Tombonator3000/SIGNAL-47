#if UNITY_EDITOR
using System;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.Rendering;

namespace Signal47.Editor
{
    // A sky-only continuation of the existing baked Visual10 scene.
    public static class NightSky12
    {
        const string Root = "Assets/Signal47/Art/NightSky12/";
        public static void Apply()
        {
            string path = Root + "NASA_DeepStarMap_8k.png";
            var importer = AssetImporter.GetAtPath(path) as TextureImporter;
            if (!importer) throw new FileNotFoundException("Run Blender/Source/prepare_night_sky.py first", path);
            importer.textureType = TextureImporterType.Default;
            importer.textureShape = TextureImporterShape.Texture2D;
            importer.sRGBTexture = true;
            importer.alphaSource = TextureImporterAlphaSource.None;
            importer.mipmapEnabled = true;
            importer.wrapModeU = TextureWrapMode.Repeat;
            importer.wrapModeV = TextureWrapMode.Clamp;
            importer.filterMode = FilterMode.Trilinear;
            importer.anisoLevel = 1;
            importer.maxTextureSize = 8192;
            importer.isReadable = false;
            var platform = importer.GetPlatformTextureSettings("Standalone");
            platform.overridden = true;
            platform.maxTextureSize = 8192;
            platform.format = TextureImporterFormat.BC7;
            platform.compressionQuality = 100;
            importer.SetPlatformTextureSettings(platform);
            importer.SaveAndReimport();
            var texture = AssetDatabase.LoadAssetAtPath<Texture2D>(path);
            if (texture.width != 8192 || texture.height != 4096) throw new InvalidOperationException("Sky map was unexpectedly resized");
            var shader = Shader.Find("Signal47/Catalog Night Sky");
            if (!shader || ShaderUtil.ShaderHasError(shader)) throw new InvalidOperationException("Catalogue sky shader failed to compile");
            var material = AssetDatabase.LoadAssetAtPath<Material>(Root + "SARO_NightSky.mat");
            if (!material) { material = new Material(shader); AssetDatabase.CreateAsset(material, Root + "SARO_NightSky.mat"); }
            material.shader = shader;
            material.SetTexture("_MainTex", texture);
            material.SetFloat("_Exposure", .65f);
            material.SetFloat("_Rotation", 115f);
            material.SetFloat("_Tilt", 33f);
            material.SetColor("_Zenith", new Color(.002f, .004f, .009f));
            material.SetColor("_Horizon", new Color(.013f, .016f, .024f));
            EditorUtility.SetDirty(material);
            RenderSettings.skybox = material;
            // Retain the original authored mesh/material; remove its rendering
            // so finite-distance square stars cannot overlay the catalogue.
            var legacy = GameObject.Find("V10.NightStars");
            if (legacy) foreach (var renderer in legacy.GetComponentsInChildren<Renderer>()) renderer.enabled = false;
            Debug.Log($"NIGHTSKY12_APPLIED map={texture.width}x{texture.height} format={texture.format} mipmaps={texture.mipmapCount} ambient={RenderSettings.ambientMode}");
        }
        public static void Build()
        {
            EditorSceneManager.OpenScene("Assets/Signal47/Scenes/Prototype/SARO_Prologue.unity");
            var before = LightmapSettings.lightmaps.Select(x => x.lightmapColor).ToArray();
            if (before.Length == 0 || RenderSettings.ambientMode != AmbientMode.Flat)
                throw new InvalidOperationException("Expected the baked Visual10 scene with explicit flat ambient lighting");
            Apply();
            if (!before.SequenceEqual(LightmapSettings.lightmaps.Select(x => x.lightmapColor)))
                throw new InvalidOperationException("Sky-only build changed laboratory lightmaps");
            EditorSceneManager.SaveScene(UnityEngine.SceneManagement.SceneManager.GetActiveScene());
            AssetDatabase.SaveAssets();
            Automation.BuildCurrentGauntletLinux();
            Debug.Log("NIGHTSKY12_BUILD_PASS preserved_lab_lightmaps=" + before.Length);
        }
    }
}
#endif
