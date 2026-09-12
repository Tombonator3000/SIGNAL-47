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
            if((Array.IndexOf(System.Environment.GetCommandLineArgs(),"--signal47-menu14-checks")>=0||Array.IndexOf(System.Environment.GetCommandLineArgs(),"--signal47-recovery15-checks")>=0)&&!FindFirstObjectByType<Menu14Checks>())
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
            if(Array.IndexOf(System.Environment.GetCommandLineArgs(),"--signal47-recovery15-checks")<0)yield break;
            string chosen=File.ReadAllText(Path.Combine(ChapterSave.StorageDirectory,"Seed","previous-case.json"));
            string valid=Path.Combine(previous,"zz-valid"),fallback=Path.Combine(previous,"yy-backup"),broken=Path.Combine(previous,"xx-broken");
            Directory.CreateDirectory(valid);File.WriteAllText(Path.Combine(valid,"case.json"),chosen);
            Directory.CreateDirectory(fallback);File.WriteAllText(Path.Combine(fallback,"case.json"),"invalid selected primary");File.WriteAllText(Path.Combine(fallback,"case.backup.json"),chosen);
            Directory.CreateDirectory(broken);File.WriteAllText(Path.Combine(broken,"case.json"),"unreadable case");
            for(int i=0;i<5;i++){string dir=Path.Combine(previous,"page-"+i);Directory.CreateDirectory(dir);File.WriteAllText(Path.Combine(dir,"case.json"),chosen);}
            var info=ChapterSave.InspectPreviousCase("zz-valid");
            Check(info.Available&&!info.UsesBackup&&info.Label.Contains("11 Sep 2026"),"Previous-case preview reads real saved date without modifying the file");
            Check(!ChapterSave.TryContinuePrevious("zz-valid",info.Fingerprint)&&hud.Started&&!g.Transitioning,"Switching previous shifts is blocked while a live shift is open");
            g.Restart();for(int i=0;i<600&&GameSession.Instance==g;i++)yield return null;
            yield return new WaitForSecondsRealtime(1);g=GameSession.Instance;hud=g.hud;
            string activeBefore=File.ReadAllText(primary);
            Check(!ChapterSave.InspectPreviousCase("../Seed").Available&&!ChapterSave.TryContinuePrevious("../Seed",info.Fingerprint),"Previous-case identifiers cannot escape the archive root");
            Check(!ChapterSave.InspectPreviousCase("xx-broken").Available,"Unreadable entries remain unavailable instead of starting an empty case");
            Check(ChapterSave.InspectPreviousCase("yy-backup").UsesBackup,"A valid recovery copy is explicitly identified in the preview");
            int countBefore=Directory.GetDirectories(previous).Length;
            hud.OpenPreviousShifts();
            Check(hud.PreviousShiftsOpen&&hud.PreviousShiftCount>6&&!hud.ConfirmPreviousShift(),"Browser opens a multi-page catalog with no implicit selection");
            yield return Shot("08-previous-list");
            hud.MovePreviousPage(-1);
            Check(hud.PreviousShiftPage==0&&hud.VisiblePreviousShiftCount==6,"First page cannot move before the start of the catalog");
            hud.MovePreviousPage(1);hud.MovePreviousPage(1);
            Check(hud.PreviousShiftPage==1&&hud.VisiblePreviousShiftCount==4,"Last page shows the remaining entries and cannot move past the catalog");
            yield return Shot("08b-previous-last-page");hud.MovePreviousPage(-1);
            hud.SelectPreviousShift("zz-valid");
            Check(hud.SelectedPreviousShift=="zz-valid"&&!g.Transitioning&&File.ReadAllText(primary)==activeBefore,"Selecting a previous shift opens review without writing the active case");
            yield return Shot("09-previous-confirmation");
            hud.BackFromPreviousShifts();hud.BackFromPreviousShifts();
            Check(!hud.PreviousShiftsOpen&&File.ReadAllText(primary)==activeBefore&&Directory.GetDirectories(previous).Length==countBefore,"Cancel selection and browser preserve every active checkpoint and archive count");
            hud.OpenPreviousShifts();hud.SelectPreviousShift("zz-valid");
            File.WriteAllText(Path.Combine(valid,"case.json"),activeBefore);
            Check(!hud.ConfirmPreviousShift()&&!g.Transitioning&&File.ReadAllText(primary)==activeBefore,"Changed valid checkpoint is rejected against the reviewed fingerprint before any writes");
            File.WriteAllText(Path.Combine(valid,"case.json"),chosen);
            hud.BackFromPreviousShifts();hud.SelectPreviousShift("yy-backup");
            Check(!File.Exists(backup),"Fixture has no active backup before the blocked-destination test");
            Directory.CreateDirectory(backup);
            Check(!hud.ConfirmPreviousShift()&&!g.Transitioning&&File.ReadAllText(primary)==activeBefore,"Blocked backup destination aborts switching and retains the current primary");
            yield return Shot("10-previous-write-failure");
            Directory.Delete(backup);
            File.WriteAllText(backup,activeBefore);Directory.CreateDirectory(primary+".tmp");
            Check(!hud.ConfirmPreviousShift()&&!g.Transitioning&&File.ReadAllText(primary)==activeBefore,"Primary-write failure keeps the active primary after preserving its recovery copy");
            bool preservedPair=false;
            foreach(string dir in Directory.GetDirectories(previous))
                if(File.Exists(Path.Combine(dir,"case.backup.json"))&&File.Exists(Path.Combine(dir,"case.json"))
                    &&File.ReadAllText(Path.Combine(dir,"case.json"))==activeBefore&&File.ReadAllText(Path.Combine(dir,"case.backup.json"))==activeBefore)preservedPair=true;
            Check(preservedPair,"Both current checkpoint files survive together in Previous Shifts after a failed primary write");
            Directory.Delete(primary+".tmp");
            Check(hud.ConfirmPreviousShift(),"Explicit retry opens the reviewed previous recovery copy");
            Check(File.ReadAllText(primary)==chosen&&!File.Exists(backup),"Selected checkpoint is atomic primary and cannot fall back to the unrelated old active backup");
            bool preservedActive=false;
            foreach(string dir in Directory.GetDirectories(previous))
                if(File.Exists(Path.Combine(dir,"case.json"))&&File.ReadAllText(Path.Combine(dir,"case.json"))==activeBefore)preservedActive=true;
            Check(preservedActive,"Switching archives the previously active checkpoint for later access");
            Check(File.ReadAllText(Path.Combine(fallback,"case.backup.json"))==chosen&&File.ReadAllText(Path.Combine(fallback,"case.json"))=="invalid selected primary","Selected archive and its damaged primary remain unchanged");
            for(int i=0;i<600&&(GameSession.Instance==g||ChapterSave.IsRestoring);i++)yield return null;
            yield return new WaitForSecondsRealtime(1);g=GameSession.Instance;hud=g.hud;
            Check(hud.Started&&g.chapter.Complete&&ChapterSave.LastLoadUsedBackup&&g.fieldCamera.Frames.Count==2,"Previous recovery copy restores the full completed case and both photographs after scene reload");
            foreach(var pair in photoBytes)
                Check(Convert.ToBase64String(File.ReadAllBytes(pair.Key))==Convert.ToBase64String(pair.Value),"Original exposure unchanged after previous-case continuation: "+Path.GetFileName(pair.Key));
            hud.SetPaused(true);yield return Shot("11-previous-case-restored");
        }
    }
}
