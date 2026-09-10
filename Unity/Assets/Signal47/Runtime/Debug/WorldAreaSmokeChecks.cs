#if UNITY_EDITOR || DEVELOPMENT_BUILD
using System;
using System.Collections;
using UnityEngine;

namespace Signal47.Debugging
{
    /// <summary>
    /// Focused regression checks for the stylized exterior/world-art pass.
    /// Errors are intentionally logged through Unity so the existing SmokeRun
    /// captures a failure without coupling this slice to the main test harness.
    /// </summary>
    public sealed class WorldAreaSmokeChecks:MonoBehaviour
    {
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        static void Init()
        {
            if(Array.IndexOf(System.Environment.GetCommandLineArgs(),"--signal47-smoke")>=0 && !FindFirstObjectByType<WorldAreaSmokeChecks>())
                new GameObject("WorldAreaSmokeChecks").AddComponent<WorldAreaSmokeChecks>();
        }

        IEnumerator Start()
        {
            yield return null;
            var motel=FindSceneObjectIncludingInactive("RoadsideMotel_Blockout");
            Check(GameObject.Find("WorldAreaArt") && GameObject.Find("ArrayNightArt") && motel,"stylized world area blockout is present");
            Check(motel && !motel.activeSelf,"future motel prototype stays inactive during the prologue");
            var flood=GameObject.Find("ArrayFlood_0");
            Check(flood && flood.GetComponent<Light>() && flood.GetComponent<Light>().shadows==LightShadows.None,"array mood lights are lightweight and shadowless");
            var scrub=GameObject.Find("DesertScrubRoot");
            Check(scrub && scrub.transform.childCount>=40,"desert silhouette breakup is populated");
        }

        static GameObject FindSceneObjectIncludingInactive(string name)
        {
            foreach(var t in Resources.FindObjectsOfTypeAll<Transform>())
                if(t.name==name && t.gameObject.scene.IsValid())return t.gameObject;
            return null;
        }

        static void Check(bool ok,string message)
        {
            if(ok)UnityEngine.Debug.Log("WORLD_PASS_SMOKE_PASS "+message);
            else UnityEngine.Debug.LogError("WORLD_PASS_SMOKE_FAIL "+message);
        }
    }
}
#endif
