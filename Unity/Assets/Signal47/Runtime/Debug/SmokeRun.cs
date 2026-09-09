#if UNITY_EDITOR || DEVELOPMENT_BUILD
using System;
using System.Collections;
using System.IO;
using UnityEngine;
using Signal47.Core;
using Signal47.Signals;
namespace Signal47.Debugging
{
    public sealed class SmokeRun:MonoBehaviour
    {
        bool failed; float deadline; string output;
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        static void Init(){if(Array.IndexOf(System.Environment.GetCommandLineArgs(),"--signal47-smoke")>=0 && !FindFirstObjectByType<SmokeRun>())new GameObject("SmokeRun").AddComponent<SmokeRun>();}
        void Awake(){Application.runInBackground=true;QualitySettings.vSyncCount=0;Application.targetFrameRate=60;DontDestroyOnLoad(gameObject);deadline=Time.realtimeSinceStartup+180;Application.logMessageReceived+=Log;}
        void Log(string condition,string trace,LogType type){if(type==LogType.Exception||type==LogType.Error||type==LogType.Assert)failed=true;}
        void Update(){if(Time.realtimeSinceStartup>deadline){UnityEngine.Debug.LogError("SMOKE_TIMEOUT");Application.Quit(1);}}
        void Check(bool ok,string message){if(!ok){failed=true;UnityEngine.Debug.LogError("SMOKE_FAIL "+message);}else UnityEngine.Debug.Log("SMOKE_PASS "+message);}
        IEnumerator Shot(string name){if(SystemInfo.graphicsDeviceType!=UnityEngine.Rendering.GraphicsDeviceType.Null){yield return new WaitForSecondsRealtime(.2f);ScreenCapture.CaptureScreenshot(Path.Combine(output,name+".png"));yield return new WaitForSecondsRealtime(.3f);}}
        IEnumerator Start()
        {
            output=Path.GetFullPath(Path.Combine(Application.dataPath,"../../Screenshots"));Directory.CreateDirectory(output);
            yield return null;var g=GameSession.Instance;Check(g!=null,"session exists");if(!g){Application.Quit(1);yield break;}
            yield return Shot("01-start");g.hud.StartShift();yield return null;
            var console=FindFirstObjectByType<SignalConsole>();Check(console!=null,"console exists");
            console.Interact();Check(!SignalConsole.AnyOpen,"unpowered receiver blocks console");
            FindFirstObjectByType<Signal47.Interaction.ReceiverBank>().Interact();console.Interact();Check(SignalConsole.AnyOpen,"powered receiver opens console");
            console.Action();Check(!g.director.PrintoutAvailable,"incorrect parameters cannot solve");
            var anomaly=console.profiles[2];
            float quality=SignalFeedback.Quality(anomaly,1420.405f,82,12,83);
            Check(quality>.99f,"aligned instruments yield strong carrier");
            Check(SignalFeedback.Quality(anomaly,1420.2f,82,12,83)<quality*.5f,"frequency detuning reduces carrier");
            Check(SignalFeedback.Quality(anomaly,1420.405f,15,12,83)<quality*.5f,"low gain reduces carrier");
            Check(SignalFeedback.Quality(anomaly,1420.405f,82,90,83)<quality*.5f,"wide bandwidth reduces carrier");
            Check(SignalFeedback.Quality(anomaly,1420.405f,82,12,18)<quality*.5f,"wrong azimuth reduces carrier");
            Check(FindFirstObjectByType<Signal47.Interaction.ReceiverBank>().leds.Length>0,"refined receiver retains controllable LEDs");
            foreach(var p in console.profiles){console.frequency=p.targetFrequency;console.gain=p.gainRange.x;console.bandwidth=p.bandwidthRange.x;console.azimuth=p.azimuthRange.x;console.Action();}
            yield return Shot("02-console");console.Action();console.Close();
            Check(!g.director.PrintoutAvailable,"solve waits for physical paper feed");
            yield return new WaitForSeconds(1.5f);Check(g.director.PrintoutAvailable && g.director.printer.Ready && g.director.printer.paper.gameObject.activeSelf,"printed sheet finishes feeding");
            Check(g.notebook.Evidence.Count==0,"unread printout is not filed automatically");
            Signal47.Interaction.PaperInteractable printout=null;
            foreach(var item in FindObjectsByType<Signal47.Interaction.PaperInteractable>(FindObjectsSortMode.None))if(item.kind==Signal47.Interaction.PaperInteractable.PaperKind.Printer)printout=item;
            printout.Interact();printout.Interact();Check(g.notebook.Evidence.Count==1 && g.notebook.Evidence[0].Body.Contains("-39"),"reading printout files one copy of the evidence");
            yield return Shot("06-evidence");g.hud.CloseModal();
            var cc=g.player.GetComponent<CharacterController>();cc.enabled=false;g.player.transform.position=new Vector3(3.1f,.05f,-.55f);g.player.transform.rotation=Quaternion.Euler(0,180,0);g.player.enabled=false;g.player.viewCamera.transform.localRotation=Quaternion.Euler(18,0,0);cc.enabled=true;
            yield return Shot("09-physical-printout");g.player.viewCamera.transform.localRotation=Quaternion.identity;g.player.enabled=true;
            var mug=g.director.mug;var rest=mug.transform.position;mug.Interact();Check(mug.Held && mug.transform.parent==g.player.viewCamera.transform,"mug can be held");mug.ReturnToDesk();Check(!mug.Held && Vector3.Distance(rest,mug.transform.position)<.001f && mug.GetComponent<Collider>().enabled,"mug returns to its desk");
            yield return new WaitForSeconds(3.8f);Check(g.director.PhoneRinging,"phone rings after printout");
            var phone=FindFirstObjectByType<Signal47.Interaction.PhoneInteractable>();phone.Interact();Check(g.director.PhoneAnswered,"phone answered");
            cc.enabled=false;g.player.transform.position=new Vector3(4.5f,.05f,1.85f);g.player.transform.rotation=Quaternion.Euler(0,155,0);g.player.enabled=false;g.player.viewCamera.transform.localRotation=Quaternion.Euler(12,0,0);cc.enabled=true;
            yield return Shot("10-handset");g.player.viewCamera.transform.localRotation=Quaternion.identity;g.player.enabled=true;
            yield return new WaitForSeconds(.5f);Check(phone.OffHook && phone.handset.localPosition.y>.2f,"answering lifts physical handset");
            g.hud.SetPaused(true);float pausedAt=Time.time;yield return new WaitForSecondsRealtime(.6f);Check(Mathf.Approximately(Time.time,pausedAt),"pause freezes sequence clock");g.hud.SetPaused(false);
            var controller=g.player.GetComponent<CharacterController>();controller.enabled=false;g.player.transform.position=new Vector3(-3.4f,.05f,1.2f);g.player.transform.rotation=Quaternion.Euler(0,145,0);controller.enabled=true;yield return Shot("03-control-room");
            controller.enabled=false;g.player.transform.position=new Vector3(0,.05f,.6f);g.player.transform.rotation=Quaternion.Euler(0,180,0);controller.enabled=true;
            Physics.SyncTransforms();bool hitConsole=Physics.Raycast(g.player.viewCamera.transform.position,(console.transform.position+Vector3.up*.65f-g.player.viewCamera.transform.position).normalized,out var hit,2.65f)&&hit.collider.GetComponentInParent<SignalConsole>()==console;
            Check(hitConsole,"console remains reachable past furniture");
            yield return Shot("05-crt");
            yield return new WaitUntil(()=>g.director.LineDead);yield return new WaitForSeconds(.5f);
            Check(!phone.OffHook && phone.handset.localPosition.y<.03f,"handset settles when connection ends");
            Check(g.notebook.Evidence.Count==2,"telephone observation is filed as written notes");
            g.hud.ShowNotebook();yield return Shot("07-notebook");g.hud.CloseModal();
            mug.Interact();yield return Shot("08-held-mug");
            while(!g.director.ImpactOccurred)yield return null;
            Check(g.director.ImpactAt-g.director.LineDeadAt>=47 && g.director.ImpactAt-g.director.LineDeadAt<47.25,"impact occurs exactly 47 seconds after dead line within one frame");
            yield return new WaitUntil(()=>mug.IsBroken);Check(!mug.Held && mug.transform.parent!=g.player.viewCamera.transform,"impact drops a held mug into the room");
            mug.Interact();int observations=g.notebook.Entries.Count;mug.Interact();Check(g.notebook.Entries.Count==observations,"examining shards deduplicates observation");
            while(!g.hud.TitleVisible)yield return null;
            Check(g.director.dishes.Completed && g.director.SignalAcquired,"entire array finishes aligning before title");
            foreach(var pivot in g.director.dishes.dishPivots)Check(Quaternion.Angle(pivot.localRotation,Quaternion.Euler(0,26,0))<.1f,"dish reaches acquired heading");Check(!g.director.mug.intact.activeSelf && g.director.mug.broken.activeSelf,"mug breaks before title");
            yield return Shot("04-title");console.Interact();g.Restart();yield return null;yield return null;
            Check(!SignalConsole.AnyOpen,"restart clears console state");Check(!GameSession.Instance.hud.Started,"restart returns to start");Check(Time.timeScale==1,"restart restores time");
            Check(GameSession.Instance.notebook.Evidence.Count==0,"restart clears evidence");
            var deskMug=GameSession.Instance.director.mug;yield return deskMug.DropAndBreak();Check(deskMug.IsBroken && deskMug.transform.position.z<-.025f,"unheld mug slides off the desk and breaks on floor");
            File.WriteAllText(Path.Combine(output,"smoke-result.txt"),failed?"FAIL":"PASS");UnityEngine.Debug.Log(failed?"SIGNAL47_SMOKE_FAIL":"SIGNAL47_SMOKE_PASS");Application.Quit(failed?1:0);
        }
    }
}
#endif
