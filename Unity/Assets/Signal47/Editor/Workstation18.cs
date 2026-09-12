#if UNITY_EDITOR
using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using Object=UnityEngine.Object;

namespace Signal47.Editor
{
    // Presentation only. Preserve authored scene, live displays, collider geometry and lighting data.
    public static class Workstation18
    {
        const string Art="Assets/Signal47/Art/Workstation18/";
        const string Scene="Assets/Signal47/Scenes/Prototype/SARO_Prologue.unity";
        [Serializable] sealed class AssetSource { public string name; public float[] dimensions_blender_xyz; public int triangles,render_meshes; }
        [Serializable] sealed class Sources { public AssetSource[] assets; }
        [Serializable] sealed class Imported { public string name; public Vector3 size; public int triangles,renderers; public bool boundsPass,meshPass; }
        [Serializable] sealed class Report { public string unity; public int retainedColliders,retainedLightmaps,disabledRenderers; public bool displayTransformsPreserved; public List<Imported> models=new(); }
        static Report report;
        static Sources sources;
        static Material Material(string name)
        {
            string path=Art+"W18_"+name+".mat";var m=AssetDatabase.LoadAssetAtPath<Material>(path);
            if(!m){m=new Material(Shader.Find("Universal Render Pipeline/Lit"));AssetDatabase.CreateAsset(m,path);}
            Color c=name switch {
                "Cream"=>new Color(.57f,.54f,.44f),"Key"=>new Color(.70f,.66f,.54f),
                "Steel"=>new Color(.28f,.32f,.30f),"Fabric"=>new Color(.12f,.18f,.15f),
                "Amber"=>new Color(.60f,.26f,.06f),_=>new Color(.028f,.038f,.034f)};
            m.SetColor("_BaseColor",c);m.SetFloat("_Smoothness",name=="Steel"?.38f:name=="Fabric"?.08f:.22f);
            m.SetFloat("_Metallic",name=="Steel"?.65f:0);EditorUtility.SetDirty(m);return m;
        }
        static void Model(string name,Transform parent,Vector3 position,Quaternion rotation)
        {
            string path=Art+"W18_"+name+".fbx";
            var importer=(ModelImporter)AssetImporter.GetAtPath(path);if(!importer)throw new FileNotFoundException(path);
            if(!importer.isReadable){importer.isReadable=true;importer.SaveAndReimport();}
            var source=AssetDatabase.LoadAssetAtPath<GameObject>(path);var placement=new GameObject("W18."+name);
            placement.transform.SetParent(parent,false);placement.transform.localPosition=position;placement.transform.localRotation=rotation;
            var model=(GameObject)PrefabUtility.InstantiatePrefab(source);model.transform.SetParent(placement.transform,false);
            var renderers=model.GetComponentsInChildren<MeshRenderer>();var bounds=renderers[0].bounds;int triangles=0;bool meshPass=true;
            foreach(var r in renderers)
            {
                r.sharedMaterials=r.sharedMaterials.Select(m=>Material(m.name.Replace("W18_","").Split('.')[0])).ToArray();
                bounds.Encapsulate(r.bounds);var mesh=r.GetComponent<MeshFilter>().sharedMesh;triangles+=mesh.triangles.Length/3;
                meshPass&=mesh.uv.Length==mesh.vertexCount && mesh.normals.Length==mesh.vertexCount && mesh.normals.All(n=>n.sqrMagnitude>.5f);
                // New movable-scale furniture uses scene lighting/probes; existing baked data is retained.
                GameObjectUtility.SetStaticEditorFlags(r.gameObject,StaticEditorFlags.BatchingStatic);
            }
            var expected=sources.assets.Single(a=>a.name==name);var d=expected.dimensions_blender_xyz;
            bool sizePass=(bounds.size-new Vector3(d[0],d[2],d[1])).sqrMagnitude<.000004f;
            if(!sizePass||!meshPass||triangles!=expected.triangles||renderers.Length!=expected.render_meshes)
                throw new InvalidOperationException($"W18 import failed {name}: {bounds.size}, tris {triangles}, renderers {renderers.Length}, mesh {meshPass}");
            report.models.Add(new Imported{name=name,size=bounds.size,triangles=triangles,renderers=renderers.Length,boundsPass=sizePass,meshPass=meshPass});
        }
        static string ColliderSignature()=>string.Join("\n",Object.FindObjectsByType<Collider>(FindObjectsSortMode.InstanceID).Select(c=>$"{c.GetInstanceID()} {c.enabled} {c.bounds.center:R} {c.bounds.size:R}"));
        public static void Apply()
        {
            if(GameObject.Find("Workstation18"))return;
            sources=JsonUtility.FromJson<Sources>(File.ReadAllText(Art+"source-manifest.json"));report=new Report{unity=Application.unityVersion};
            var colliders=ColliderSignature();var maps=LightmapSettings.lightmaps.Select(m=>m.lightmapColor).ToArray();
            var terminals=Object.FindObjectsByType<Transform>(FindObjectsSortMode.None).Where(t=>t.name=="SM_CRT_Terminal_A").ToArray();
            var seats=Object.FindObjectsByType<Transform>(FindObjectsSortMode.None).Where(t=>t.name=="ChairSeat").ToArray();
            if(terminals.Length!=3||seats.Length!=3)throw new InvalidOperationException("Expected three existing terminals and chairs");
            var displays=terminals.SelectMany(t=>t.GetComponentsInChildren<Renderer>()).Where(r=>r.name=="PhysicalDisplay"||r.name=="StatusDisplay").ToArray();
            string before=string.Join("\n",displays.Select(r=>$"{r.GetInstanceID()} {r.transform.localToWorldMatrix}"));
            var root=new GameObject("Workstation18").transform;
            foreach(var terminal in terminals)
            {
                foreach(var r in terminal.GetComponentsInChildren<Renderer>())
                    if(!displays.Contains(r)&&r.enabled){r.enabled=false;report.disabledRenderers++;}
                // A separate placement root retains the FBX axis/unit transforms underneath it.
                var station=new GameObject("W18.Station").transform;station.SetParent(root,false);station.SetPositionAndRotation(terminal.position,terminal.rotation);
                Model("CRT",station,Vector3.zero,Quaternion.identity);Model("Keyboard",station,Vector3.zero,Quaternion.identity);
            }
            foreach(var r in Object.FindObjectsByType<MeshRenderer>(FindObjectsSortMode.None))
                if(new[]{"ChairSeat","ChairBack","ChairPost","ChairBase"}.Contains(r.name)||r.GetComponentsInParent<Transform>().Any(t=>t.name.StartsWith("ChairUpgrade_")))
                    if(r.enabled){r.enabled=false;report.disabledRenderers++;}
            foreach(var seat in seats)Model("Chair",root,new Vector3(seat.position.x,0,seat.position.z),Quaternion.identity);
            report.displayTransformsPreserved=before==string.Join("\n",displays.Select(r=>$"{r.GetInstanceID()} {r.transform.localToWorldMatrix}"))&&displays.All(r=>r.enabled);
            if(colliders!=ColliderSignature()||!maps.SequenceEqual(LightmapSettings.lightmaps.Select(m=>m.lightmapColor))||!report.displayTransformsPreserved)
                throw new InvalidOperationException("Workstation pass altered interaction, display or baked lighting anchors");
            report.retainedColliders=Object.FindObjectsByType<Collider>(FindObjectsSortMode.None).Length;report.retainedLightmaps=maps.Length;
            Directory.CreateDirectory("../Artifacts/Workstation18");File.WriteAllText("../Artifacts/Workstation18/unity-import.json",JsonUtility.ToJson(report,true));
            Debug.Log("WORKSTATION18_APPLIED models="+report.models.Count);
        }
        public static void Build()
        {
            EditorSceneManager.OpenScene(Scene);Apply();
            // Also validate reimported FBXs when this saved scene already contains the pass.
            sources=JsonUtility.FromJson<Sources>(File.ReadAllText(Art+"source-manifest.json"));
            var current=new List<Imported>();
            foreach(var t in GameObject.Find("Workstation18").GetComponentsInChildren<Transform>())
            {
                string name=t.name.Replace("W18.","");var expected=sources.assets.SingleOrDefault(a=>a.name==name);if(expected==null)continue;
                var rs=t.GetComponentsInChildren<MeshRenderer>();var b=rs[0].bounds;int tris=0;bool meshPass=true;
                foreach(var r in rs){b.Encapsulate(r.bounds);var m=r.GetComponent<MeshFilter>().sharedMesh;tris+=m.triangles.Length/3;meshPass&=m.uv.Length==m.vertexCount&&m.normals.Length==m.vertexCount&&m.normals.All(n=>n.sqrMagnitude>.5f);}
                var d=expected.dimensions_blender_xyz;bool boundsPass=(b.size-new Vector3(d[0],d[2],d[1])).sqrMagnitude<.000004f;
                if(!boundsPass||!meshPass||tris!=expected.triangles||rs.Length!=expected.render_meshes)throw new InvalidOperationException("Reimport validation failed: "+name);
                current.Add(new Imported{name=name,size=b.size,triangles=tris,renderers=rs.Length,boundsPass=boundsPass,meshPass=meshPass});
            }
            if(current.Count!=9)throw new InvalidOperationException("Expected nine validated workstation instances");
            string evidence="../Artifacts/Workstation18/unity-import.json";
            if(report==null)report=File.Exists(evidence)?JsonUtility.FromJson<Report>(File.ReadAllText(evidence)):new Report{unity=Application.unityVersion};
            report.models=current;File.WriteAllText(evidence,JsonUtility.ToJson(report,true));
            EditorSceneManager.SaveScene(UnityEngine.SceneManagement.SceneManager.GetActiveScene());AssetDatabase.SaveAssets();
            Automation.BuildCurrentGauntletLinux();Debug.Log("WORKSTATION18_BUILD_PASS");
        }
    }
}
#endif
