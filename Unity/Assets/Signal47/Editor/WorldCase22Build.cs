#if UNITY_EDITOR
using System;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using Signal47.Core;
using Signal47.WorldCase22;
using Object = UnityEngine.Object;
namespace Signal47.Editor
{
    public static class WorldCase22Build
    {
        const string Scene="Assets/Signal47/Scenes/Prototype/SARO_Prologue.unity";
        static Material Mat(string name)=>AssetDatabase.LoadAssetAtPath<Material>("Assets/Signal47/Art/Chapter09/CH09_"+name+".mat");
        public static void Build()
        {
            EditorSceneManager.OpenScene(Scene);
            var previous=GameObject.Find("WorldCase22");if(previous)Object.DestroyImmediate(previous);
            var g=Object.FindFirstObjectByType<GameSession>();
            int originalColliders=Object.FindObjectsByType<Collider>(FindObjectsSortMode.None).Length;
            if(!g||!g.chapter||originalColliders!=154||LightmapSettings.lightmaps.Length!=2)throw new Exception("Expected preserved Resources19 game scene");
            var root=new GameObject("WorldCase22");
            var controller=root.AddComponent<WorldCaseController>();g.worldCase=controller;
            controller.font=AssetDatabase.LoadAssetAtPath<Font>("Assets/Signal47/Art/ThirdParty/VT323/VT323-Regular.ttf");
            var source=AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Signal47/Art/Archive17/A17_Folio.fbx");
            if(!source)throw new Exception("Original archive folio missing");
            var model=(GameObject)PrefabUtility.InstantiatePrefab(source);model.name="WorldCase22.Folio";model.transform.SetParent(root.transform,false);model.transform.rotation=Quaternion.Euler(0,90,0);
            var renderers=model.GetComponentsInChildren<Renderer>();
            var bounds=renderers[0].bounds;foreach(var r in renderers)bounds.Encapsulate(r.bounds);
            model.transform.localScale*=.46f/bounds.size.z;
            bounds=renderers[0].bounds;foreach(var r in renderers)bounds.Encapsulate(r.bounds);
            var bench=GameObject.Find("CH09.ArchiveBench.FormicaTop").GetComponent<Renderer>();
            model.transform.position=new Vector3(2.65f,bench.bounds.max.y+.0015f,10.40f)-new Vector3(bounds.center.x,bounds.min.y,bounds.center.z);
            foreach(var r in renderers)
            {
                r.sharedMaterials=r.sharedMaterials.Select(m=>Mat(m&&m.name.Contains("Paper")||m&&m.name.Contains("Label")?"Paper":m&&m.name.Contains("Clip")?"Steel":m&&m.name.Contains("Tab")?"Red":"Cork")).ToArray();
                GameObjectUtility.SetStaticEditorFlags(r.gameObject,0);
            }
            var target=new GameObject("WorldCase22.FolioHit");target.transform.SetParent(root.transform,false);target.transform.position=new Vector3(2.17f,1.10f,10.40f);
            var hit=target.AddComponent<BoxCollider>();hit.size=new Vector3(.28f,.34f,.62f);
            target.AddComponent<WorldCaseAction>().controller=controller;
            var labelBounds=renderers.First(r=>r.name.Contains("Label field")).bounds;
            var caption=new GameObject("WorldCase22.Caption");caption.transform.SetParent(root.transform,false);caption.transform.SetPositionAndRotation(new Vector3(labelBounds.center.x,labelBounds.max.y+.0005f,labelBounds.center.z),Quaternion.Euler(90,90,0));
            var text=caption.AddComponent<TextMesh>();text.text="STATION 01\nARCHIVE DOSSIER";text.font=controller.font;text.fontSize=64;text.characterSize=.011f;text.anchor=TextAnchor.MiddleCenter;text.alignment=TextAlignment.Center;text.color=new Color(.09f,.12f,.08f);caption.GetComponent<Renderer>().sharedMaterial=controller.font.material;
            var captionSize=caption.GetComponent<Renderer>().bounds.size;
            if(captionSize.x>0&&captionSize.z>0)caption.transform.localScale*=Mathf.Min(labelBounds.size.x*.90f/captionSize.x,labelBounds.size.z*.90f/captionSize.z);
            var lightObject=new GameObject("WorldCase22.TaskLight");lightObject.transform.SetParent(root.transform,false);
            var light=lightObject.AddComponent<Light>();light.type=LightType.Point;light.transform.position=new Vector3(2.4f,2.0f,10.3f);light.color=new Color(1,.9f,.72f);light.intensity=.32f;light.range=2.0f;light.shadows=LightShadows.None;
            int after=Object.FindObjectsByType<Collider>(FindObjectsSortMode.None).Length;
            if(after!=155||LightmapSettings.lightmaps.Length!=2)throw new Exception("Unexpected integration collision/lightmap change");
            EditorUtility.SetDirty(g);EditorSceneManager.SaveScene(UnityEngine.SceneManagement.SceneManager.GetActiveScene());AssetDatabase.SaveAssets();
            Directory.CreateDirectory("../Artifacts/WorldCase22");
            bounds=renderers[0].bounds;foreach(var r in renderers)bounds.Encapsulate(r.bounds);
            if(Mathf.Abs(bounds.min.y-bench.bounds.max.y-.0015f)>.0005f)throw new Exception("Folio must rest on the archive bench");
            File.WriteAllText("../Artifacts/WorldCase22/scene-audit.json",JsonUtility.ToJson(new Audit{beforeColliders=originalColliders,afterColliders=after,lightmaps=2,hitCenter=hit.bounds.center,hitSize=hit.bounds.size,folioBottom=bounds.min.y,benchTop=bench.bounds.max.y},true));
            Automation.BuildCurrentGauntletLinux();
        }
        [Serializable] class Audit{public int beforeColliders,afterColliders,lightmaps;public float folioBottom,benchTop;public Vector3 hitCenter,hitSize;}
    }
}
#endif
