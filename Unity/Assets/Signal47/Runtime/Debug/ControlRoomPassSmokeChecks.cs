using System.Collections;
using UnityEngine;

namespace Signal47.Debugging
{
    public sealed class ControlRoomPassSmokeChecks : MonoBehaviour
    {
        IEnumerator Start()
        {
            if (System.Array.IndexOf(System.Environment.GetCommandLineArgs(), "--signal47-smoke") < 0) yield break;
            yield return null;

            int crtUpgrades=0, keyboardDecks=0, chairUpgrades=0;
            foreach (var t in FindObjectsByType<Transform>(FindObjectsSortMode.None))
            {
                if (t.name=="CRT_SilhouetteUpgrade") crtUpgrades++;
                else if (t.name=="KeyboardDeck") keyboardDecks++;
                else if (t.name.StartsWith("ChairUpgrade_")) chairUpgrades++;
            }

            Check(crtUpgrades>=3,$"CRT upgrades present: {crtUpgrades}");
            Check(keyboardDecks>=3,$"Grouped keyboards present: {keyboardDecks}");
            Check(chairUpgrades>=3,$"Chair upgrades present: {chairUpgrades}");

            // Cosmetic pass must not add colliders under upgraded monitor/chair roots.
            int cosmeticColliders=0;
            foreach(var root in FindObjectsByType<Transform>(FindObjectsSortMode.None))
            {
                if(root.name!="CRT_SilhouetteUpgrade" && !root.name.StartsWith("ChairUpgrade_"))continue;
                cosmeticColliders+=root.GetComponentsInChildren<Collider>(true).Length;
            }
            Check(cosmeticColliders==0,$"Control-room cosmetic colliders: {cosmeticColliders}");
        }

        static void Check(bool ok,string message)
        {
            if(ok) Debug.Log("CONTROL_PASS_SMOKE_PASS "+message);
            else Debug.LogError("CONTROL_PASS_SMOKE_FAIL "+message);
        }
    }
}
