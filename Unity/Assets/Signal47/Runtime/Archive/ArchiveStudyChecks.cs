using System;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace Signal47.Archive
{
    public sealed class ArchiveStudyChecks : MonoBehaviour
    {
        [Serializable] sealed class Report
        {
            public string result,method="API-driven standalone Archive16 study; no native input or performance claim",unity,gpu;
            public List<string> checks=new();
        }
        string root;Report report=new();float deadline;
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        static void Init()
        {
            if(Array.IndexOf(System.Environment.GetCommandLineArgs(),"--signal47-archive16-checks")>=0)new GameObject("Archive16 Checks").AddComponent<ArchiveStudyChecks>();
        }
        void Check(bool pass,string name){if(!pass)throw new Exception(name);report.checks.Add(name);Debug.Log("ARCHIVE16_CHECK "+name);}
        void Finish(bool ok,string error="")
        {
            enabled=false;report.result=ok?"PASS":"FAIL: "+error;
            if(root!=null)File.WriteAllText(Path.Combine(root,"result.json"),JsonUtility.ToJson(report,true));
            Debug.Log("ARCHIVE16_"+report.result);Application.Quit(ok?0:2);
        }
        void Update(){if(Time.realtimeSinceStartup>deadline)Finish(false,"timeout");}
        IEnumerator Start()
        {
            deadline=Time.realtimeSinceStartup+60;var args=System.Environment.GetCommandLineArgs();int i=Array.IndexOf(args,"--archive16-evidence");
            if(i<0||i+1>=args.Length||!Path.GetFileName(args[i+1]).StartsWith("archive16-test-",StringComparison.Ordinal)||!File.Exists(Path.Combine(args[i+1],"ALLOW_ARCHIVE16_TEST")))
            {Finish(false,"dedicated marked evidence directory required");yield break;}
            root=args[i+1];Application.runInBackground=true;report.unity=Application.unityVersion;report.gpu=SystemInfo.graphicsDeviceName;
            var r=Run();while(true){object next;try{if(!r.MoveNext())break;next=r.Current;}catch(Exception e){Finish(false,e.ToString());yield break;}yield return next;}Finish(true);
        }
        IEnumerator Shot(string file)
        {
            yield return new WaitForSecondsRealtime(.3f);yield return new WaitForEndOfFrame();
            var t=ScreenCapture.CaptureScreenshotAsTexture();File.WriteAllBytes(Path.Combine(root,file+".png"),t.EncodeToPNG());Destroy(t);
        }
        IEnumerator Run()
        {
            yield return new WaitForSecondsRealtime(2);var s=FindFirstObjectByType<ArchiveStudy>();
            Check(s&&s.folio&&s.original&&s.amended,"Study has physical folio and both source documents");
            Check(!FindFirstObjectByType<Signal47.Chapter.ChapterSave>()&&!FindFirstObjectByType<Signal47.Core.GameSession>(),"Standalone scene has no chapter/save components");
            Check(!s.DocumentOpen&&!s.Complete,"First launch starts at an unsolved physical desk");
            yield return Shot("01-desk");s.SetOverview(true);yield return Shot("02-room");s.SetOverview(false);
            Check(!s.SubmitFinding(1)&&!s.Complete,"Answer cannot complete the study outside the document comparison");
            Check(!s.OpenFromRay(new Ray(s.view.transform.position,Vector3.up)),"A ray missing the folio cannot open documents");
            Check(s.OpenFromRay(new Ray(s.view.transform.position,s.view.transform.forward)),"Desk view centre ray reaches the physical folio collider");
            Check(s.OriginalRead&&!s.AmendedRead&&!s.SelectPage(2),"Comparison is unavailable until both versions have been visited");
            yield return Shot("03-original");
            Check(s.SelectPage(1)&&s.AmendedRead,"Amended copy can be opened");yield return Shot("04-amended");
            Check(!s.SelectPage(7)&&s.Page==1,"Invalid page leaves the current document unchanged");
            s.CloseDocument();Check(!s.DocumentOpen&&!s.Complete,"Closing the document leaves the finding unresolved");
            Check(s.OpenFromRay(new Ray(s.view.transform.position,s.view.transform.forward))&&s.SelectPage(2),"Reopening retains read state and permits comparison");
            yield return Shot("05-comparison");
            Check(!s.SubmitFinding(0)&&!s.Complete&&s.Feedback.Length>0,"Unsupported motive claim gives feedback without progression");yield return Shot("06-unsupported");
            Check(!s.SubmitFinding(2)&&!s.Complete,"Incorrect identical-record claim remains retryable");
            Check(s.SubmitFinding(1)&&s.Complete,"Documented omitted-reference finding completes the study");yield return Shot("07-supported");
            s.CloseDocument();Check(s.Complete&&!s.DocumentOpen,"Closing after success preserves the session finding");yield return Shot("08-complete-desk");
            s.ResetStudy();Check(!s.Complete&&!s.OriginalRead&&!s.AmendedRead&&!s.DocumentOpen,"Explicit reset returns to a fresh unsolved study");
            Check(s.OpenFromRay(new Ray(s.view.transform.position,s.view.transform.forward))&&s.SelectPage(1)&&s.SelectPage(0)&&s.SelectPage(2),"Documents can be revisited in a different order before comparison");
            Check(!s.SubmitFinding(-1)&&!s.Complete,"Invalid answer cannot complete the study");
        }
    }
}
