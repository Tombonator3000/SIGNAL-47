#if UNITY_EDITOR
using System;
using System.IO;
using System.Linq;
using UnityEngine;
using UnityEditor;
using UnityEditor.SceneManagement;
using Signal47.Interaction;
using Signal47.Environment;
using Object=UnityEngine.Object;
namespace Signal47.Editor
{
    public static class Resources19
    {
        const string Scene="Assets/Signal47/Scenes/Prototype/SARO_Prologue.unity";
        [Serializable] sealed class Item { public string name; public Vector3 position,scale,size,center,localPosition,localScale,localEuler,meshSize; }
        [Serializable] sealed class Report { public Item[] phone,dish; public int colliders,lightmaps; }
        static Item Describe(Renderer r)=>new Item{name=r.name,position=r.transform.position,scale=r.transform.lossyScale,size=r.bounds.size,center=r.bounds.center,localPosition=r.transform.localPosition,localScale=r.transform.localScale,localEuler=r.transform.localEulerAngles,meshSize=r.TryGetComponent<MeshFilter>(out var f)?f.sharedMesh.bounds.size:Vector3.zero};
        public static void Audit()
        {
            EditorSceneManager.OpenScene(Scene);
            var p=Object.FindFirstObjectByType<PhoneInteractable>();var d=Object.FindFirstObjectByType<DishArrayController>();
            var r=new Report{phone=p.GetComponentsInChildren<Renderer>().Select(Describe).ToArray(),dish=d.dishPivots[0].parent.GetComponentsInChildren<Renderer>().Select(Describe).ToArray(),colliders=Object.FindObjectsByType<Collider>(FindObjectsSortMode.None).Length,lightmaps=LightmapSettings.lightmaps.Length};
            Directory.CreateDirectory("../Artifacts/Resources19");File.WriteAllText("../Artifacts/Resources19/before.json",JsonUtility.ToJson(r,true));
            Debug.Log("RESOURCES19_AUDIT_PASS phone="+p.transform.lossyScale);
        }
        static void Model(string name,Transform parent)
        {
            string path="Assets/Signal47/Art/Resources19/"+name+".fbx";
            var importer=(ModelImporter)AssetImporter.GetAtPath(path);
            if(!importer)throw new FileNotFoundException(path);
            if(!importer.isReadable){importer.isReadable=true;importer.SaveAndReimport();}
            var go=(GameObject)PrefabUtility.InstantiatePrefab(AssetDatabase.LoadAssetAtPath<GameObject>(path));
            go.name=name;go.transform.SetParent(parent,false);
            foreach(var r in go.GetComponentsInChildren<MeshRenderer>())
            {
                r.sharedMaterials=r.sharedMaterials.Select(old=>
                {
                    string n=old.name.Split('.')[0];string mp="Assets/Signal47/Art/Resources19/"+n+".mat";
                    var m=AssetDatabase.LoadAssetAtPath<Material>(mp);
                    if(!m){m=new Material(Shader.Find("Universal Render Pipeline/Lit"));AssetDatabase.CreateAsset(m,mp);}
                    m.SetColor("_BaseColor",n.EndsWith("Enamel")?new Color(.60f,.63f,.57f):new Color(.27f,.31f,.29f));
                    m.SetFloat("_Smoothness",.26f);m.SetFloat("_Metallic",n.EndsWith("Steel")?.65f:0);EditorUtility.SetDirty(m);return m;
                }).ToArray();
                // These follow dish animation; never mark them static or rebake the room.
                GameObjectUtility.SetStaticEditorFlags(r.gameObject,0);
            }
        }
        public static void Apply()
        {
            var p=Object.FindFirstObjectByType<PhoneInteractable>();
            var d=Object.FindFirstObjectByType<DishArrayController>();
            if(!p||!p.handset||d.dishPivots.Length!=11)throw new InvalidOperationException("Missing authored phone/array");
            p.transform.localScale=Vector3.one*.3f;
            Object.FindFirstObjectByType<MugBreakable>().transform.localScale=Vector3.one*.35f;
            if(!p.receiverLead)
            {
                var lead=new GameObject("ReceiverLead");lead.transform.SetParent(p.transform,false);
                p.receiverLead=lead.AddComponent<LineRenderer>();p.receiverLead.useWorldSpace=true;p.receiverLead.positionCount=12;
                p.receiverLead.startWidth=.0025f;p.receiverLead.endWidth=.0025f;p.receiverLead.enabled=false;
                string mp="Assets/Signal47/Art/Resources19/R19_Cord.mat";var m=AssetDatabase.LoadAssetAtPath<Material>(mp);
                if(!m){m=new Material(Shader.Find("Universal Render Pipeline/Lit"));m.SetColor("_BaseColor",new Color(.025f,.03f,.025f));AssetDatabase.CreateAsset(m,mp);}
                p.receiverLead.sharedMaterial=m;
            }
            // Match the physical housing and receiver, with a small interaction margin.
            var c=p.GetComponent<BoxCollider>();c.center=new Vector3(0,.24f,0);c.size=new Vector3(.80f,.50f,.65f);
            foreach(var pivot in d.dishPivots)
            {
                if(!pivot.GetChild(0).Find("R19_BowlDetail"))Model("R19_BowlDetail",pivot.GetChild(0));
                if(!pivot.parent.Find("R19_PedestalDetail"))Model("R19_PedestalDetail",pivot.parent);
                var beam=pivot.parent.GetComponentsInChildren<Renderer>().Single(r=>r.name.EndsWith("__AzimuthBeam"));
                beam.transform.localPosition=new Vector3(0,-.60f,0);
            }
            if(!GameObject.Find("Resources19"))new GameObject("Resources19");
            Debug.Log("RESOURCES19_APPLIED");
        }
        public static void ClearanceAudit()
        {
            EditorSceneManager.OpenScene(Scene);Clearance(false);
        }
        static void Clearance(bool requireClear)
        {
            var d=Object.FindFirstObjectByType<DishArrayController>();var lines=new System.Collections.Generic.List<string>();
            foreach(var pivot in d.dishPivots)
            {
                var bowl=pivot.GetChild(0);var pr=pivot.localRotation;var br=bowl.localRotation;
                for(int pose=0;pose<=10;pose++)
                {
                    pivot.localRotation=Quaternion.Slerp(pr,Quaternion.Euler(0,26,0),pose/10f);bowl.localRotation=Quaternion.Slerp(br,Quaternion.Euler(38,0,0),pose/10f);
                    foreach(var f in pivot.parent.GetComponentsInChildren<MeshFilter>())
                    {
                        if(f.transform.IsChildOf(pivot)||f.name=="ArrayBeacon")continue;
                        float worst=float.NegativeInfinity;
                        foreach(var vertex in f.sharedMesh.vertices)
                        {
                            var v=bowl.InverseTransformPoint(f.transform.TransformPoint(vertex));float r2=v.x*v.x+v.z*v.z;
                            if(r2<4.1f*4.1f)worst=Mathf.Max(worst,v.y-r2/12f);
                        }
                        if(worst>-.003f){lines.Add(pivot.parent.name+" pose="+pose+" "+f.name+" intrusion_m="+worst.ToString("R"));}
                    }
                }
                pivot.localRotation=pr;bowl.localRotation=br;
            }
            Directory.CreateDirectory("../Artifacts/Resources19");File.WriteAllLines("../Artifacts/Resources19/clearance.txt",lines.Count==0?new[]{"PASS: all sampled pedestal mesh vertices behind the parabola in 11 sampled poses per antenna"}:lines.ToArray());
            if(requireClear&&lines.Count>0)throw new InvalidOperationException("Pedestal intersects bowl: "+string.Join("; ",lines));
        }
        public static void Build()
        {
            EditorSceneManager.OpenScene(Scene);Apply();Clearance(true);
            var p=Object.FindFirstObjectByType<PhoneInteractable>();var d=Object.FindFirstObjectByType<DishArrayController>();
            var r=new Report{phone=p.GetComponentsInChildren<Renderer>().Select(Describe).ToArray(),dish=d.dishPivots[0].parent.GetComponentsInChildren<Renderer>().Select(Describe).ToArray(),colliders=Object.FindObjectsByType<Collider>(FindObjectsSortMode.None).Length,lightmaps=LightmapSettings.lightmaps.Length};
            if(r.colliders!=154||r.lightmaps!=2)throw new InvalidOperationException("Unexpected collider/lightmap change");
            Directory.CreateDirectory("../Artifacts/Resources19");File.WriteAllText("../Artifacts/Resources19/after.json",JsonUtility.ToJson(r,true));
            EditorSceneManager.SaveScene(UnityEngine.SceneManagement.SceneManager.GetActiveScene());AssetDatabase.SaveAssets();
            Automation.BuildCurrentGauntletLinux();
        }
    }
}
#endif
