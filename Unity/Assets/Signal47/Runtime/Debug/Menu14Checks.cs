using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Threading.Tasks;
using UnityEngine;
using Signal47.Core;
using Signal47.Chapter;

namespace Signal47.Debugging
{
    // Explicit opt-in, isolated fixtures and API-driven checks. Not native input evidence.
    public sealed class Menu14Checks : MonoBehaviour
    {
        [Serializable] sealed class Report
        {
            public string result,method="API-driven Unity menu/save checks; no native input or performance certification",unity,renderer,profile;
            public List<string> checks=new();
        }
        Report report=new();string root;float deadline;
        const BindingFlags PrivateStatic=BindingFlags.NonPublic|BindingFlags.Static;
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        static void Init()
        {
            if(Array.IndexOf(System.Environment.GetCommandLineArgs(),"--signal47-menu14-checks")>=0&&!FindFirstObjectByType<Menu14Checks>())
                new GameObject("Menu14Checks").AddComponent<Menu14Checks>();
        }
        void Awake(){DontDestroyOnLoad(gameObject);Application.runInBackground=true;deadline=Time.realtimeSinceStartup+150;}
        void Update(){if(Time.realtimeSinceStartup>deadline)Finish(false,"Timeout");}
        void Check(bool value,string name)
        {
            if(!value)throw new InvalidOperationException(name);
            report.checks.Add(name);Debug.Log("MENU14_PASS "+name);
        }
        void Finish(bool pass,string reason)
        {
            enabled=false;report.result=pass?"PASS":"FAIL: "+reason;
            if(root!=null)File.WriteAllText(Path.Combine(root,"result.json"),JsonUtility.ToJson(report,true));
            Debug.Log("MENU14_"+report.result);Application.Quit(pass?0:2);
        }
        IEnumerator Start()
        {
            // Deliberately refuse ordinary save directories, even with the opt-in flag.
            var args=System.Environment.GetCommandLineArgs();int index=Array.IndexOf(args,"--signal47-save-dir");
            if(index<0||index+1>=args.Length||!Path.GetFileName(ChapterSave.StorageDirectory).StartsWith("menu14-test-",StringComparison.Ordinal)
                ||!File.Exists(Path.Combine(ChapterSave.StorageDirectory,"ALLOW_MENU14_TEST")))
            {Finish(false,"A dedicated menu14-test-* profile and marker are required");yield break;}
            root=Path.Combine(ChapterSave.StorageDirectory,"Evidence");Directory.CreateDirectory(root);
            report.profile=ChapterSave.StorageDirectory;report.unity=Application.unityVersion;report.renderer=SystemInfo.graphicsDeviceName;
            var routine=Run();
            while(true)
            {
                object next;
                try{if(!routine.MoveNext())break;next=routine.Current;}
                catch(Exception error){Finish(false,error.ToString());yield break;}
                yield return next;
            }
            Finish(true,"");
        }
        IEnumerator Shot(string name)
        {
            if(SystemInfo.graphicsDeviceType==UnityEngine.Rendering.GraphicsDeviceType.Null)yield break;
            yield return null;
            yield return new WaitForEndOfFrame();
            var texture=ScreenCapture.CaptureScreenshotAsTexture();
            File.WriteAllBytes(Path.Combine(root,name+".png"),texture.EncodeToPNG());Destroy(texture);
        }
        IEnumerator Run()
        {
            yield return new WaitForSecondsRealtime(2);
            var g=GameSession.Instance;var hud=g.hud;
            string primary=ChapterSave.SavePath,backup=Path.Combine(ChapterSave.StorageDirectory,"case.backup.json");
            string previous=Path.Combine(ChapterSave.StorageDirectory,"PreviousCases");
            Check(!ChapterSave.HasSave&&!hud.Started,"Isolated launch begins at the fresh menu");
            yield return Shot("01-fresh-menu");
            File.WriteAllText(primary,"primary checkpoint fixture");File.WriteAllText(backup,"backup checkpoint fixture");
            ChapterSave.RefreshMenuStatus();
            Check(ChapterSave.Status.Contains("checkpoint was found"),"Existing case is not mislabeled as having no checkpoint");
            Directory.CreateDirectory(ChapterSave.PhotoArchiveDirectory);
            string photo=Path.Combine(ChapterSave.PhotoArchiveDirectory,"original.fixture");File.WriteAllText(photo,"unchanged original exposure");
            yield return null;yield return Shot("02-existing-checkpoint");
            Check(!hud.ConfirmNewShift()&&File.Exists(primary),"Confirm without an open confirmation cannot clear a checkpoint");
            float scale=Time.timeScale;
            hud.RequestNewShift();
            Check(hud.NewShiftConfirmation&&!hud.Started&&Time.timeScale==0,"Existing checkpoint opens a paused confirmation before any transition");
            Check(File.ReadAllText(primary)=="primary checkpoint fixture"&&File.ReadAllText(backup)=="backup checkpoint fixture","Opening confirmation leaves both checkpoint files unchanged");
            yield return Shot("03-confirmation");
            hud.CancelNewShift();
            Check(!hud.NewShiftConfirmation&&!hud.Started&&Time.timeScale==scale,"Cancel returns to the original menu and time state");
            Check(!Directory.Exists(previous),"Cancel creates no recovery directory and performs no retirement");
            hud.StartShift();hud.SetPaused(true);hud.RequestNewShift();hud.CancelNewShift();
            Check(hud.Started&&hud.Paused&&Time.timeScale==0&&AudioListener.pause,"Cancel from pause preserves the live paused world");
            yield return Shot("04-paused");
            var job=typeof(ChapterSave).GetField("writeJob",PrivateStatic);
            var pending=new TaskCompletionSource<bool>();job.SetValue(null,pending.Task);
            Check(!ChapterSave.StartNew()&&File.ReadAllText(primary)=="primary checkpoint fixture","An in-flight writer blocks New without deleting files");
            job.SetValue(null,null);pending.SetResult(true);
            File.WriteAllText(previous,"deliberately blocked recovery destination");
            hud.RequestNewShift();
            Check(!hud.ConfirmNewShift()&&hud.NewShiftConfirmation&&hud.Started&&!g.Transitioning,"Recovery-copy failure keeps confirmation and current world instead of reloading");
            Check(File.ReadAllText(primary)=="primary checkpoint fixture"&&File.ReadAllText(backup)=="backup checkpoint fixture","Failed recovery copy preserves both active files byte for byte");
            yield return Shot("05-recovery-copy-failure");
            File.Delete(previous);
            Check(hud.ConfirmNewShift(),"Explicit retry can start after the recovery destination is repaired");
            string[] retired=Directory.GetDirectories(previous);
            Check(retired.Length==1&&File.ReadAllText(Path.Combine(retired[0],"case.json"))=="primary checkpoint fixture"
                &&File.ReadAllText(Path.Combine(retired[0],"case.backup.json"))=="backup checkpoint fixture","New preserves exact primary and backup bytes together in a unique recovery directory");
            Check(!File.Exists(primary)&&!File.Exists(backup)&&File.ReadAllText(photo)=="unchanged original exposure","New clears only active checkpoints and keeps the original photo archive");
            for(int i=0;i<240&&GameSession.Instance==g;i++)yield return null;
            yield return new WaitForSecondsRealtime(1);
            g=GameSession.Instance;hud=g.hud;
            Check(!hud.Started&&!hud.NewShiftConfirmation&&!ChapterSave.HasSave,"Actual scene reload returns to a fresh menu");
            yield return Shot("06-new-menu-after-reload");
            hud.RequestNewShift();
            Check(hud.Started&&!hud.NewShiftConfirmation&&Time.timeScale==1,"Fresh profile starts directly without an unnecessary confirmation");
            hud.SetPaused(true);hud.RequestNewShift();hud.CancelNewShift();
            Check(hud.Paused&&hud.Started,"An unsaved live shift is also protected by confirmation");
            // An invalid checkpoint exercises the real Continue rejection path.
            File.WriteAllText(primary,"damaged checkpoint fixture");
            Check(!ChapterSave.TryContinue()&&!g.Transitioning&&File.ReadAllText(primary)=="damaged checkpoint fixture","Rejected Continue preserves the file and does not start a different case");
            hud.RequestNewShift();Check(hud.ConfirmNewShift(),"A second explicitly confirmed retirement succeeds");
            Check(Directory.GetDirectories(previous).Length==2&&File.ReadAllText(Path.Combine(retired[0],"case.json"))=="primary checkpoint fixture","Repeated New never overwrites the earlier recovery directory");
            for(int i=0;i<240&&GameSession.Instance==g;i++)yield return null;
            yield return new WaitForSecondsRealtime(1);
            string seed=Path.Combine(ChapterSave.StorageDirectory,"Seed","case.json");
            Check(File.Exists(seed),"A copied historical completed checkpoint is available for continuation regression");
            File.Copy(seed,primary,false);ChapterSave.RefreshMenuStatus();
            var photoBytes=new Dictionary<string,byte[]>();
            foreach(string path in Directory.GetFiles(ChapterSave.PhotoArchiveDirectory,"*.jpg"))photoBytes.Add(path,File.ReadAllBytes(path));
            Check(photoBytes.Count>=2,"Continuation fixture includes actual original exposure files");
            g=GameSession.Instance;
            Check(ChapterSave.TryContinue(),"Real completed version-1 checkpoint passes Continue validation");
            for(int i=0;i<600&&(GameSession.Instance==g||ChapterSave.IsRestoring);i++)yield return null;
            yield return new WaitForSecondsRealtime(1);
            g=GameSession.Instance;hud=g.hud;
            Check(hud.Started&&!ChapterSave.IsRestoring&&g.chapter.Complete,"Continue restores the completed case through an actual scene reload");
            Check(g.fieldCamera.Frames.Count==2&&g.fieldCamera.SaveStatus=="RESTORED FROM LOCAL PHOTO ARCHIVE","Both historical photographs decode from the isolated photo archive");
            hud.SetPaused(true);hud.RequestNewShift();hud.CancelNewShift();
            Check(hud.Started&&hud.Paused&&!g.Transitioning&&File.ReadAllText(primary)==File.ReadAllText(seed),"Cancel after Continue preserves the restored case and exact checkpoint");
            foreach(var pair in photoBytes)
                Check(Convert.ToBase64String(File.ReadAllBytes(pair.Key))==Convert.ToBase64String(pair.Value),"Original exposure unchanged after Continue and canceled New: "+Path.GetFileName(pair.Key));
            yield return Shot("07-restored-case-pause");
        }
    }
}
