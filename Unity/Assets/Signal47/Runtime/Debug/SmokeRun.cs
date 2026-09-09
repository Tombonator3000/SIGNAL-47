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
        void Awake(){Application.runInBackground=true;DontDestroyOnLoad(gameObject);deadline=Time.realtimeSinceStartup+100;Application.logMessageReceived+=Log;}
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
            g.director.PowerReceiver();console.Interact();Check(SignalConsole.AnyOpen,"powered receiver opens console");
            console.Action();Check(!g.director.PrintoutAvailable,"incorrect parameters cannot solve");
            foreach(var p in console.profiles){console.frequency=p.targetFrequency;console.gain=p.gainRange.x;console.bandwidth=p.bandwidthRange.x;console.azimuth=p.azimuthRange.x;console.Action();}
            yield return Shot("02-console");console.Action();Check(g.director.PrintoutAvailable,"three stages unlock printout");console.Close();
            yield return new WaitForSeconds(3.8f);Check(g.director.PhoneRinging,"phone rings after printout");
            g.director.AnswerPhone();Check(g.director.PhoneAnswered,"phone answered");
            g.hud.SetPaused(true);float pausedAt=Time.time;yield return new WaitForSecondsRealtime(.6f);Check(Mathf.Approximately(Time.time,pausedAt),"pause freezes sequence clock");g.hud.SetPaused(false);
            g.player.transform.position=new Vector3(0,.05f,2.2f);g.player.transform.rotation=Quaternion.Euler(0,180,0);yield return Shot("03-control-room");
            float start=Time.time;while(!g.hud.TitleVisible){yield return null;}
            Check(Time.time-start>=47,"future sequence retains hidden delay");Check(!g.director.mug.intact.activeSelf && g.director.mug.broken.activeSelf,"mug breaks before title");
            yield return Shot("04-title");console.Interact();g.Restart();yield return null;yield return null;
            Check(!SignalConsole.AnyOpen,"restart clears console state");Check(!GameSession.Instance.hud.Started,"restart returns to start");Check(Time.timeScale==1,"restart restores time");
            File.WriteAllText(Path.Combine(output,"smoke-result.txt"),failed?"FAIL":"PASS");UnityEngine.Debug.Log(failed?"SIGNAL47_SMOKE_FAIL":"SIGNAL47_SMOKE_PASS");Application.Quit(failed?1:0);
        }
    }
}
#endif
