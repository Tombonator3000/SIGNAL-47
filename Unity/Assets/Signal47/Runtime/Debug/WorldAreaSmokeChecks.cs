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
            // Give the existing SmokeRun one frame to subscribe to log messages.
            yield return null;
            Check(GameObject.Find("WorldAreaArt") && GameObject.Find("ArrayNightArt") && GameObject.Find("RoadsideMotel_Blockout"),"stylized world area blockout is present");
            var flood=GameObject.Find("ArrayFlood_0");
            Check(flood && flood.GetComponent<Light>() && flood.GetComponent<Light>().shadows==LightShadows.None,"array mood lights are lightweight and shadowless");
            var scrub=GameObject.Find("DesertScrubRoot");
            Check(scrub && scrub.transform.childCount>=40,"desert silhouette breakup is populated");
        }

        static void Check(bool ok,string message)
        {
            if(ok)UnityEngine.Debug.Log("WORLD_PASS_SMOKE_PASS "+message);
            else UnityEngine.Debug.LogError("WORLD_PASS_SMOKE_FAIL "+message);
        }
    }
}
#endif
