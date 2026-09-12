using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityEngine;
using Signal47.Core;
using Signal47.Signals;
using Signal47.Chapter;

namespace Signal47.Debugging
{
    // Opt-in, isolated API scenarios. No desktop input and no normal-play side effects.
    public sealed class Workstation18Checks:MonoBehaviour
    {
        [Serializable] sealed class CheckResult { public string name; public bool pass; }
        [Serializable] sealed class Report
        {
            public string result="RUNNING",method="API-driven real Unity player; not native input or performance measurement",unity,gpu,renderer,resolution,buildId;
            public List<CheckResult> checks=new();public List<string> screenshots=new();
        }
        Report report=new();string root;float deadline;
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        static void Init()
        {
            if(Array.IndexOf(System.Environment.GetCommandLineArgs(),"--signal47-workstation18-checks")>=0&&!FindFirstObjectByType<Workstation18Checks>())
                new GameObject("Workstation18Checks").AddComponent<Workstation18Checks>();
        }
        void Awake()
        {
            Application.runInBackground=true;deadline=Time.realtimeSinceStartup+150;
            root=Path.Combine(ChapterSave.StorageDirectory,"Evidence");
            if(!File.Exists(Path.Combine(ChapterSave.StorageDirectory,"ALLOW_WORKSTATION18_TEST"))){Debug.LogError("WORKSTATION18_TEST_PROFILE_REQUIRED");Application.Quit(2);enabled=false;return;}
            Directory.CreateDirectory(root);Application.logMessageReceived+=Log;
            report.unity=Application.unityVersion;report.gpu=SystemInfo.graphicsDeviceName;report.renderer=SystemInfo.graphicsDeviceType.ToString();
            string stamp=Path.Combine(Application.dataPath,"../build-id.txt");report.buildId=File.Exists(stamp)?File.ReadAllText(stamp):"unknown";StartCoroutine(Run());
        }
        void OnDestroy()=>Application.logMessageReceived-=Log;
        void Log(string message,string stack,LogType type){if(type==LogType.Error||type==LogType.Exception||type==LogType.Assert)report.checks.Add(new CheckResult{name=message,pass=false});}
        void Update(){if(Time.realtimeSinceStartup>deadline){Check(false,"scenario timeout");Finish();}}
        void Check(bool pass,string name){report.checks.Add(new CheckResult{name=name,pass=pass});Debug.Log("WORKSTATION18_CHECK "+pass+" "+name);}
        void Finish(){report.resolution=$"{Screen.width}x{Screen.height}";report.result=report.checks.All(c=>c.pass)?"PASS":"FAIL";File.WriteAllText(Path.Combine(root,"result.json"),JsonUtility.ToJson(report,true));Application.Quit(report.result=="PASS"?0:1);enabled=false;}
        IEnumerator Shot(string name,Vector3 at,Vector3 target,bool ui=false)
        {
            var g=GameSession.Instance;var c=g.player.viewCamera;g.player.enabled=false;
            var interactor=g.player.GetComponent<Signal47.Interaction.PlayerInteractor>();interactor.enabled=false;
            g.hud.enabled=ui;c.fieldOfView=60;c.transform.SetPositionAndRotation(at,Quaternion.LookRotation(target-at));
            yield return new WaitForSecondsRealtime(.6f);yield return new WaitForEndOfFrame();
            var image=new Texture2D(Screen.width,Screen.height,TextureFormat.RGB24,false);image.ReadPixels(new Rect(0,0,Screen.width,Screen.height),0,0);image.Apply();
            File.WriteAllBytes(Path.Combine(root,name+".png"),image.EncodeToPNG());Destroy(image);report.screenshots.Add(name+".png");g.hud.enabled=true;
        }
        IEnumerator Run()
        {
            yield return null;var g=GameSession.Instance;Check(g&&g.player&&g.director,"main scene and player loaded");if(!g){Finish();yield break;}
            var art=GameObject.Find("Workstation18");Check(art,"new workstation family integrated into main scene");
            if(!art){Finish();yield break;}
            Check(art.GetComponentsInChildren<Collider>(true).Length==0,"cosmetic models add no interaction blockers");
            var transforms=art.GetComponentsInChildren<Transform>();
            foreach(var name in new[]{"CRT","Keyboard","Chair"})Check(transforms.Count(t=>t.name=="W18."+name)==3,"three "+name+" instances");
            var console=FindFirstObjectByType<SignalConsole>();
            Check(console&&console.physicalScreen&&console.physicalScreen.enabled,"original live screen remains connected and visible");
            Check(console.profiles.Length==3&&Mathf.Abs(console.profiles[0].targetFrequency-1419.900f)<.0002f&&Mathf.Abs(console.profiles[1].targetFrequency-1420.110f)<.0002f&&Mathf.Abs(console.profiles[2].targetFrequency-1420.405f)<.0002f,"three authored frequency constants unchanged");
            Check(Signal47.Events.PrologueDirector.FutureDelay==47,"authored 47-second delay unchanged");
            g.hud.StartShift();g.hud.CloseModal();console.Interact();Check(!SignalConsole.AnyOpen,"unpowered console blocks use");
            g.director.PowerReceiver();yield return new WaitForSecondsRealtime(1);
            yield return Shot("01-control-room",new Vector3(0,1.65f,4.55f),new Vector3(0,1.8f,-22));
            yield return Shot("02-workstation",new Vector3(-3,1.65f,.4f),new Vector3(.1f,1.3f,-2.15f));
            yield return Shot("03-crt-detail",new Vector3(0,1.61f,-.3f),new Vector3(0,1.34f,-1.95f));
            yield return Shot("04-chair-detail",new Vector3(3.4f,1.18f,1.1f),new Vector3(.8f,.64f,-.6f));
            Physics.SyncTransforms();var origin=new Vector3(0,1.65f,.6f);var aim=console.transform.position+Vector3.up*.65f;
            bool hit=Physics.Raycast(origin,(aim-origin).normalized,out var ray,2.65f,Physics.DefaultRaycastLayers,QueryTriggerInteraction.Ignore);
            Check(hit&&ray.collider.GetComponentInParent<SignalConsole>()==console,"existing approach ray reaches console past furniture");
            console.Interact();Check(SignalConsole.AnyOpen,"powered console opens");
            yield return Shot("05-console-ui",new Vector3(0,1.61f,-.3f),new Vector3(0,1.34f,-1.95f),true);
            console.Action();Check(!g.director.PrintoutAvailable,"wrong settings do not produce evidence");
            foreach(var p in console.profiles){console.frequency=p.targetFrequency;console.gain=(p.gainRange.x+p.gainRange.y)/2;console.bandwidth=(p.bandwidthRange.x+p.bandwidthRange.y)/2;console.azimuth=(p.azimuthRange.x+p.azimuthRange.y)/2;console.Action();}
            console.Action();int entries=g.notebook.Entries.Count;console.Action();Check(g.notebook.Entries.Count==entries,"repeated completed action adds no duplicate notes");
            console.Close();Check(!SignalConsole.AnyOpen,"close releases console modal");
            float ringDeadline=Time.realtimeSinceStartup+8;
            while(!g.director.PhoneRinging&&Time.realtimeSinceStartup<ringDeadline)yield return null;
            Check(g.director.PrintoutAvailable&&g.director.PhoneRinging,"signal solve produces printout and phone call");
            if(!g.director.PhoneRinging){Finish();yield break;}
            yield return Shot("06-locked-screen",new Vector3(0,1.61f,-.3f),new Vector3(0,1.34f,-1.95f));
            g.director.AnswerPhone();yield return new WaitUntil(()=>g.director.LineDead);
            g.hud.SetPaused(true);float pausedAt=Time.time;yield return new WaitForSecondsRealtime(.6f);Check(Mathf.Approximately(pausedAt,Time.time),"pause preserves sequence clock");g.hud.SetPaused(false);
            yield return new WaitUntil(()=>g.director.ImpactOccurred);double delta=g.director.ImpactAt-g.director.LineDeadAt;
            Check(delta>=47&&delta<47.25,"actual impact delay remains 47 seconds within one frame tolerance");
            // The director consumes the dish completion on its next coroutine turn.
            yield return new WaitUntil(()=>g.director.SignalAcquired);Check(g.director.dishes.Completed,"array sequence completes after model replacement");
            Finish();
        }
    }
}
