using System.Collections;
using UnityEngine;
using Signal47.Core;
using Signal47.Signals;
using Signal47.Audio;
namespace Signal47.Debugging
{
    public sealed class ControlRoomPassSmokeChecks : MonoBehaviour
    {
        IEnumerator Start()
        {
            if(System.Array.IndexOf(System.Environment.GetCommandLineArgs(),"--signal47-smoke")<0)yield break;
            yield return null;yield return null;
            int crts=0,decks=0,chairs=0,cosmeticColliders=0,keyMeshes=0;bool geometry=true;
            foreach(var t in FindObjectsByType<Transform>(FindObjectsSortMode.None))
            {
                if(t.name=="CRT_SilhouetteUpgrade"){crts++;cosmeticColliders+=t.GetComponentsInChildren<Collider>(true).Length;}
                if(t.name=="KeyboardDeck")decks++;
                if(t.name.StartsWith("ChairUpgrade_")){chairs++;cosmeticColliders+=t.GetComponentsInChildren<Collider>(true).Length;}
                if(t.name=="KeyboardKeysCombined")
                {
                    keyMeshes++;var filter=t.GetComponent<MeshFilter>();
                    geometry&=filter&&filter.sharedMesh&&filter.sharedMesh.vertexCount>400&&filter.sharedMesh.bounds.size.x<1.1f;
                }
            }
            Check(crts==3,$"CRT upgrades: {crts}");Check(decks==3,$"Keyboard decks: {decks}");
            Check(chairs==3,$"Chair upgrades: {chairs}");Check(cosmeticColliders==0,$"New cosmetic colliders: {cosmeticColliders}");
            Check(keyMeshes==3&&geometry,"Three combined 52-key meshes inside keyboard width");
            var aux=FindObjectsByType<AuxiliaryCRTDisplay>(FindObjectsSortMode.None);int amber=0,green=0;bool statuses=true,stable=true;
            foreach(var screen in aux)
            {
                if(screen.amber)amber++;else green++;
                screen.RefreshFromSession();int before=screen.RefreshCount;screen.RefreshFromSession();stable&=screen.RefreshCount==before;
                bool powered=GameSession.Instance.director.ReceiverPowered;
                statuses&=powered?screen.StatusLabel!="STANDBY":screen.StatusLabel=="STANDBY";
            }
            Check(aux.Length==2&&amber==1&&green==1,"Two distinct auxiliary screen roles; main receiver untouched");
            Check(statuses&&stable,"Auxiliary state follows receiver; unchanged state does not redraw");
            int fixtures=0;bool lights=true;
            foreach(var light in FindObjectsByType<Light>(FindObjectsSortMode.None))if(light.name=="FixtureLight")
            {fixtures++;lights&=light.shadows==LightShadows.None&&light.intensity>=.69f&&light.intensity<=1.56f;}
            Check(fixtures==5&&lights,"Five bounded practical fixture lights; no new shadow lights");
            Check(SoundPalette.UiGain==.22f&&SoundPalette.WindGain==.16f&&SoundPalette.CeramicGain==.70f&&SoundPalette.TitleGain==.20f,"Versioned first-pass source gains");
            var wind=GameObject.Find("ExteriorWind");var ceramic=GameObject.Find("CeramicFoley");
            Check(wind&&ceramic&&Mathf.Abs(wind.GetComponent<AudioSource>().volume-SoundPalette.WindGain)<.001f&&Mathf.Abs(ceramic.GetComponent<AudioSource>().volume-SoundPalette.CeramicGain)<.001f,"Runtime sources use documented gains");
        }
        static void Check(bool ok,string message)
        {
            if(ok)Debug.Log("CONTROL_PASS_SMOKE_PASS "+message);else Debug.LogError("CONTROL_PASS_SMOKE_FAIL "+message);
        }
    }
}
