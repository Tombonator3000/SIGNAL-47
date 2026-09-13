using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using UnityEngine;
using Signal47.Core;
using Signal47.Chapter;
using Signal47.Investigation;
using Signal47.WorldCase22;
using Signal47.Station26;

namespace Signal47.Debugging
{
    public sealed class Station26Checks : MonoBehaviour
    {
        [Serializable] sealed class Report { public string result,method,unity,renderer; public List<string> checks=new(); public List<string> errors=new(); }
        readonly Report report=new(){method="API-driven integrated STATION 01 P06-P09 with durable save/reload; no native input or performance claim"};
        string root; bool finished; float deadline; int writes;
        WorldCaseController Case=>GameSession.Instance?GameSession.Instance.worldCase:null;
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)] static void Init(){if(Array.IndexOf(System.Environment.GetCommandLineArgs(),"--signal47-station26-checks")>=0&&!FindFirstObjectByType<Station26Checks>())new GameObject("Station26Checks").AddComponent<Station26Checks>();}
        void Awake(){DontDestroyOnLoad(gameObject);Application.runInBackground=true;deadline=Time.realtimeSinceStartup+300;Application.logMessageReceived+=Log;}
        void OnDestroy(){Application.logMessageReceived-=Log;}
        void Log(string m,string s,LogType t){if(m.StartsWith("CHAPTER_SAVE_OK ",StringComparison.Ordinal))writes++;if((t==LogType.Error||t==LogType.Exception||t==LogType.Assert)&&report.errors.Count<8)report.errors.Add(m);}
        void Check(bool ok,string name){if(!ok)throw new InvalidOperationException(name);report.checks.Add(name);Debug.Log("STATION26_PASS "+name);}
        string LightingState()
        {
            var states=new List<string>();var station=GameSession.Instance.station;
            foreach(var light in FindObjectsByType<Light>(FindObjectsInactive.Include,FindObjectsSortMode.None))
                if(light.type==LightType.Directional&&!light.transform.IsChildOf(station.transform))states.Add(light.name+":"+light.enabled);
            states.Sort(StringComparer.Ordinal);return string.Join("|",states);
        }
        void CheckEmitter(bool lit)
        {
            int count=0;
            foreach(var surface in GameSession.Instance.station.stationRoot.GetComponentsInChildren<Signal47.Audio.PracticalLampEmission>())
                if(surface.source==GameSession.Instance.station.testLamp)
                {var block=new MaterialPropertyBlock();surface.GetComponent<Renderer>().GetPropertyBlock(block);float value=block.GetColor("_EmissionColor").maxColorComponent;Check(lit?value>.05f:value<.001f,"Authored lamp emitter follows "+(lit?"exposed":"covered")+" state");count++;}
            Check(count>0,"Physical test lamp has a visible emitter response");
        }
        void Finish(bool ok,string reason=""){if(finished)return;finished=true;StopAllCoroutines();report.result=ok?"PASS":"FAIL: "+reason;if(root!=null)File.WriteAllText(Path.Combine(root,"result.json"),JsonUtility.ToJson(report,true));Application.Quit(ok?0:2);}
        void Update(){if(finished)return;if(report.errors.Count>0)Finish(false,"Runtime error: "+report.errors[0]);else if(Time.realtimeSinceStartup>deadline)Finish(false,"Timeout");}
        IEnumerator Start(){var a=System.Environment.GetCommandLineArgs();int oi=Array.IndexOf(a,"--signal47-save-dir");try{if(oi<0||oi+1>=a.Length)throw new InvalidOperationException("Dedicated station26 profile required");string d=ChapterSave.StorageDirectory;if(!Path.GetFileName(d).StartsWith("station26-test-",StringComparison.Ordinal)||!File.Exists(Path.Combine(d,"ALLOW_STATION26_TEST")))throw new InvalidOperationException("Dedicated station26 profile and marker required");root=Path.Combine(d,"Evidence");Directory.CreateDirectory(root);report.unity=Application.unityVersion;report.renderer=SystemInfo.graphicsDeviceType+" / "+SystemInfo.graphicsDeviceName;}catch(Exception e){Finish(false,e.ToString());yield break;}var stack=new Stack<IEnumerator>();stack.Push(Run());while(stack.Count>0&&!finished){object next=null;bool advanced=false;Exception failure=null;try{advanced=stack.Peek().MoveNext();if(advanced)next=stack.Peek().Current;}catch(Exception e){failure=e;}if(failure!=null){Finish(false,failure.ToString());yield break;}if(!advanced){stack.Pop();continue;}if(next is IEnumerator nested)stack.Push(nested);else yield return next;}Finish(report.errors.Count==0,report.errors.Count>0?report.errors[0]:"");}
        IEnumerator WaitFor(Func<bool> condition,float seconds,string failure){float until=Time.realtimeSinceStartup+seconds;while(!condition()&&Time.realtimeSinceStartup<until)yield return null;Check(condition(),failure);}
        IEnumerator Reload(string label){var before=GameSession.Instance;Check(ChapterSave.TryContinue(),label+": TryContinue accepts checkpoint");yield return WaitFor(()=>GameSession.Instance!=before&&!ChapterSave.IsRestoring,25,label+": scene reload completes");Check(GameSession.Instance.hud.Started,label+": reloaded session is playable");}
        IEnumerator Save(string label){Check(ChapterSave.CanSave,label+": checkpoint is available");int before=writes;ChapterSave.RequestCheckpoint();yield return WaitFor(()=>writes>before&&!ChapterSave.Saving,15,label+": checkpoint reaches durable completion");Check(File.Exists(ChapterSave.SavePath),label+": checkpoint file exists");}
        void Position(Transform target,float distance)
        {
            var g=GameSession.Instance;
            Vector3 body=target.position-Vector3.forward*distance;body.y=.12f;
            Vector3 eye=body+Vector3.up*g.player.viewCamera.transform.localPosition.y;
            Quaternion rotation=Quaternion.LookRotation(target.position-eye);
            g.player.RestorePose(body,rotation.eulerAngles.y,Mathf.DeltaAngle(0,rotation.eulerAngles.x));
        }
        IEnumerator Shot(string name)
        {
            yield return null;yield return new WaitForEndOfFrame();
            ScreenCapture.CaptureScreenshot(Path.Combine(root,name+".png"));
            yield return new WaitForSecondsRealtime(.3f);
        }
        IEnumerator Expose(string id,bool failArchive=false)
        {
            var g=GameSession.Instance;var f=g.fieldCamera;var target=g.station.FrameTarget(id);
            Check(target!=null,"Physical target exists for "+id);
            g.hud.CloseModal();Position(target,id==FieldCamera.StationMarkerPhotoId?4f:2.2f);
            yield return null;
            Check(f.FrameProblem()=="","Camera accepts framed target: "+f.FrameProblem());
            string archive=ChapterSave.PhotoArchiveDirectory,held=archive+".test-held";
            if(failArchive){Directory.Move(archive,held);File.WriteAllText(archive,"Disposable photo-export failure fixture.");}
            try
            {
                Check(f.TryExpose(),"Real camera renders "+id);
                if(failArchive)
                {
                    yield return WaitFor(()=>f.GetFrame(id)!=null&&f.ExportRetryAvailable,15,"Actual export failure retains retryable exposure");
                    var pending=f.GetFrame(id);string pixels=Convert.ToBase64String(pending.pendingPixels),path=pending.path;
                    Check(!pending.exported&&!f.SaveReady&&!g.station.Travel(false)&&g.station.InStation,"Unarchived exposure blocks travel without losing the field area");
                    Check(f.SaveStatus.Contains("RETURN FOLIO"),"Export failure directs player to local recovery");
                    var folio=GameObject.Find("Station26.ReturnFolio");Check(folio,"Physical return folio exists during export failure");
                    Position(folio.transform,1.4f);yield return null;
                    Vector3 eye=g.player.viewCamera.transform.position;
                    Check(Physics.Raycast(eye,(folio.GetComponent<Collider>().bounds.center-eye).normalized,out var hit,2.65f)&&hit.collider.gameObject==folio,"Return folio is reachable with failed export");
                    folio.GetComponent<StationAction>().Interact();
                    Check(g.station.ModalOpen&&g.station.CurrentPage=="travel","Physical return folio opens recovery panel");
                    yield return Shot("09-export-retry");
                    Check(g.station.RetryPhotoExport(),"Local retry invokes existing immutable export path");
                    yield return WaitFor(()=>f.ExportRetryAvailable,15,"Persistent export failure remains retryable");
                    Check(f.GetFrame(id)==pending&&pending.path==path&&Convert.ToBase64String(pending.pendingPixels)==pixels&&f.FrameCount==3,"Repeated failed export preserves the same original exposure");
                }
            }
            finally{if(failArchive){File.Delete(archive);Directory.Move(held,archive);}}
            if(failArchive)
            {
                Check(g.station.RetryPhotoExport(),"Local retry works after repairing the photo location");
                yield return WaitFor(()=>f.SaveReady,15,"Recovered field export becomes durable without leaving the station");
                Check(g.station.InStation&&!f.ExportRetryAvailable&&f.GetFrame(id).pendingPixels==null,"Recovered photo releases retained pixels and local retry");
                yield return Shot("10-export-recovered");g.station.Close();
                Position(target,id==FieldCamera.StationMarkerPhotoId?4f:2.2f);yield return null;
            }
            yield return WaitFor(()=>f.GetFrame(id)!=null&&f.GetFrame(id).exported&&f.SaveReady,15,"Exposure archives: "+id);
            var frame=f.GetFrame(id);
            Check(frame.texture && frame.width>=900 && frame.method=="field-observation","Real texture and capture metadata: "+id);
            yield return Shot(id);
        }
        IEnumerator Develop(string id){var g=GameSession.Instance;var f=g.fieldCamera;var c=g.chapter;Check(f.GetFrame(id)!=null&&f.GetFrame(id).exported,"develop target exists: "+id);c.Interact("develop");MethodInfo begin=typeof(ChapterInvestigation).GetMethod("BeginDevelopment",BindingFlags.Instance|BindingFlags.NonPublic),transfer=typeof(ChapterInvestigation).GetMethod("TransferPrint",BindingFlags.Instance|BindingFlags.NonPublic),collect=typeof(ChapterInvestigation).GetMethod("CollectPrint",BindingFlags.Instance|BindingFlags.NonPublic);Check(begin!=null&&transfer!=null&&collect!=null,"Chapter wet-bench APIs are present");begin.Invoke(c,null);yield return WaitFor(()=>c.LabStep==1&&c.LabRemaining<=0,6,id+" tank processing completes");transfer.Invoke(c,null);yield return WaitFor(()=>c.LabStep==2&&c.LabRemaining<=0,6,id+" fixer transfer completes");collect.Invoke(c,null);Check(f.GetFrame(id).developed&&f.GetFrame(id).inspected,"Chapter wet-bench APIs develop and inspect "+id);}
        IEnumerator WalkAndReach()
        {
            var g=GameSession.Instance;var st=g.station;var player=g.player;
            foreach(string name in new[]{"Transit","LampControl","CableInspection","TimingRecord"})
            {
                var obj=GameObject.Find("Station26."+name);Check(obj,"Physical interaction object exists: "+name);
                Position(obj.transform,1.4f);yield return null;
                Vector3 eye=player.viewCamera.transform.position;
                Check(Physics.Raycast(eye,(obj.GetComponent<Collider>().bounds.center-eye).normalized,out var hit,2.65f)&&hit.collider.GetComponent<StationAction>(),"Player-height interaction ray reaches "+name);
            }
            player.RestorePose(st.arrival.position,0,0);var cc=player.GetComponent<CharacterController>();player.enabled=false;
            try
            {
                foreach(var local in new[]{new Vector3(-5,0,2),new Vector3(0,0,3),new Vector3(5,0,2),new Vector3(2,0,8),new Vector3(0,0,9),new Vector3(0,0,12.3f),new Vector3(0,0,8),new Vector3(2,0,4),new Vector3(0,0,-6)})
                {
                    Vector3 target=st.stationRoot.transform.position+local;float until=Time.realtimeSinceStartup+7;
                    while(Time.realtimeSinceStartup<until)
                    {
                        Vector3 d=target-player.transform.position;d.y=0;if(d.magnitude<.18f)break;
                        cc.Move(d.normalized*Mathf.Min(d.magnitude,5*Time.deltaTime)+Vector3.down*2*Time.deltaTime);yield return null;
                    }
                    Vector3 delta=target-player.transform.position;delta.y=0;Check(delta.magnitude<.2f,"CharacterController follows field walking route "+local);
                }
            }
            finally{player.enabled=true;}
        }
        IEnumerator RecoveryAndFailure()
        {
            var g=GameSession.Instance;var st=g.station;g.hud.CloseModal();
            string primary=ChapterSave.SavePath, backup=Path.Combine(ChapterSave.StorageDirectory,"case.backup.json");
            yield return Save("Before travel failure");
            byte[] before=File.ReadAllBytes(primary);string held=primary+".test-held";
            Vector3 pose=g.player.transform.position;string state=st.CaptureState();
            File.Move(primary,held);Directory.CreateDirectory(primary);
            try
            {
                Check(st.Travel(true),"Travel failure fixture starts ordinary save path");
                yield return WaitFor(()=>!st.Traveling,12,"Write failure ends transfer attempt");
                Check(!st.InStation && st.CaptureState()==state && Vector3.Distance(g.player.transform.position,pose)<.08f,"Failed save preserves SARO and field state");
                Check(ChapterSave.Status.StartsWith("Save failed",StringComparison.Ordinal),"Save failure is visible to player");
                Check(Convert.ToBase64String(File.ReadAllBytes(held))==Convert.ToBase64String(before),"Last durable case unchanged by failed travel");
            }
            finally {Directory.Delete(primary);File.Move(held,primary);}
            yield return Save("Repaired save location");
            Check(st.Travel(true),"Travel succeeds after repairing test save location");
            yield return WaitFor(()=>st.InStation&&!st.Traveling,25,"Field revisited for recovery checks");
            foreach(string id in new[]{FieldCamera.PhotoId,FieldCamera.StationMarkerPhotoId})
            {
                g=GameSession.Instance;st=g.station;
                yield return Save("Before missing-photo fixture");
                byte[] saved=File.ReadAllBytes(primary),savedBackup=File.Exists(backup)?File.ReadAllBytes(backup):null;
                var frame=g.fieldCamera.GetFrame(id);string photo=frame.path,photoHeld=photo+".test-held";
                byte[] photoBytes=File.ReadAllBytes(photo);File.Move(photo,photoHeld);
                try
                {
                    yield return Reload("Missing "+id);g=GameSession.Instance;st=g.station;
                    if(id==FieldCamera.PhotoId)
                    {
                        Check(!g.chapter.Complete&&!st.InStation&&g.player.transform.position.x<30,"Missing base photo returns safely to SARO for repair");
                        Check(g.fieldCamera.GetFrame(FieldCamera.StationMarkerPhotoId)!=null&&g.fieldCamera.GetFrame(FieldCamera.StationCablePhotoId)!=null,"Base-photo recovery preserves both field originals");
                        Check(!st.Travel(true),"Incomplete base-photo repair cannot travel away");
                    }
                    else Check(st.InStation&&!st.P06Complete&&st.P07Complete&&!st.P08Complete&&!st.P09Complete,"Missing marker photo clears only dependent deductions");
                    yield return Save("Recoverable missing-photo checkpoint");
                }
                finally
                {
                    File.Move(photoHeld,photo);File.WriteAllBytes(primary,saved);
                    if(savedBackup!=null)File.WriteAllBytes(backup,savedBackup);else if(File.Exists(backup))File.Delete(backup);
                }
                Check(Convert.ToBase64String(File.ReadAllBytes(photo))==Convert.ToBase64String(photoBytes),"Test restores exact original photo bytes");
                yield return Reload("Recovered original fixture");
                Check(GameSession.Instance.chapter.Complete&&GameSession.Instance.station.P09Complete&&GameSession.Instance.fieldCamera.FrameCount==4,"Restored source fixture resumes complete field case");
            }
        }
        IEnumerator Run()
        {
            yield return new WaitForSecondsRealtime(2);
            var g=GameSession.Instance;
            Check(g&&g.station&&g.chapter&&g.worldCase&&g.fieldCamera,"Integrated main-game components exist");
            Check(!ChapterSave.HasSave&&!g.hud.Started,"Fresh disposable profile starts at menu");
            Check(!g.station.Travel(true),"Travel cannot bypass start menu");
            string seed=Path.Combine(ChapterSave.StorageDirectory,"Seed","case.json");
            var originals=new Dictionary<string,byte[]>();
            foreach(string photo in Directory.GetFiles(ChapterSave.PhotoArchiveDirectory,"*.jpg"))originals.Add(photo,File.ReadAllBytes(photo));
            Check(originals.Count==2,"Two original historical photo files copied");
            File.Copy(seed,ChapterSave.SavePath,false);yield return Reload("Historical v1");g=GameSession.Instance;
            Check(g.chapter.Complete&&g.fieldCamera.FrameCount==2&&!g.station.InStation,"Historical case restores without field progress");
            Check(!g.station.Travel(true),"Unsolved P05 blocks travel");
            Check(StationController.ValidateState("",out _)&&!StationController.ValidateState("{\"version\":99}",out _),"Old optional state accepted; unknown station version rejected");
            Check(Case.Open()&&Case.SelectPage("original")&&Case.SelectPage("amended")&&Case.SelectPage("compare"),"P04 sources accessible");
            Check(Case.SubmitComparison("omitted-c"),"Original comparison completes P04");
            Check(Case.SelectPage("maintenance")&&Case.SelectPage("index")&&Case.SelectPage("route"),"P05 source chain accessible");
            Check(Case.SubmitDestination("old-survey-station","STATION 01","triangle-bar"),"P05 unlocks supported route");Case.Close();
            string originalChapter=g.chapter.CaptureState(),archiveState=Case.CaptureState();
            var departure=GameObject.Find("Station26.DepartureFolio");
            g.player.RestorePose(new Vector3(1.15f,.12f,9.9f),90,0);yield return null;
            Vector3 eye=g.player.viewCamera.transform.position;
            Check(departure && Physics.Raycast(eye,(departure.GetComponent<Collider>().bounds.center-eye).normalized,out var departureHit,2.65f) && departureHit.collider.gameObject==departure,"Physical departure folio is reachable beside archive bench");
            Check(g.station.Open("travel"),"Travel review opens in SARO");yield return Shot("00-travel-review");g.station.Close();
            Check(!g.station.InStation,"Cancel keeps SARO");yield return Save("SARO archive");
            Color saroAmbient=RenderSettings.ambientLight,saroFog=RenderSettings.fogColor;float saroDensity=RenderSettings.fogDensity;string saroLights=LightingState();
            Check(g.station.Travel(true),"Confirmed departure starts");yield return WaitFor(()=>g.station.InStation&&!g.station.Traveling,25,"Departure finishes with saved destination");
            var st=g.station;yield return Shot("01-arrival");
            Check(RenderSettings.ambientLight!=saroAmbient&&LightingState()!=saroLights,"Field visit applies its own atmosphere and removes duplicate distant directional light");
            yield return WalkAndReach();
            string initial=st.CaptureState();
            Check(st.stationRoot.activeSelf&&g.player.transform.position.x>100,"Playable field area is active");
            Position(st.testLamp.transform,2);st.SetLampCovered(true);
            Check(st.ObserveLamp()&&st.P07Complete&&!st.P06Complete,"P07 can precede P06");
            st.RestoreState(initial);Check(st.CaptureState()==initial,"Alternate-order fixture resets exactly");
            Check(st.Open("marker")&&st.ReadSource("e09a"),"Transit source read");
            Check(!st.SubmitMarker("fixed-point-a"),"Unphotographed marker cannot be recorded");yield return Shot("02-transit-source");st.Close();
            yield return Expose(FieldCamera.StationMarkerPhotoId,true);
            Check(st.Open("marker")&&!st.SubmitMarker("b-moved")&&st.SubmitMarker("fixed-point-a"),"Real frame supports A; wrong marker rejected");st.Close();
            Check(st.P06Complete&&!st.P07Complete,"P06 can precede P07");
            Position(st.testLamp.transform,2);g.hud.SetPaused(true);st.SetLampCovered(true);
            Check(!st.LampCovered&&!st.ObserveLamp(),"Pause blocks lamp mutation and observation");g.hud.SetPaused(false);
            Check(!st.ObserveLamp(),"Uncovered lamp does not pass null test");yield return Shot("03-lamp-on");
            CheckEmitter(true);
            st.SetLampCovered(true);Check(st.ObserveLamp()&&st.P07Complete&&!st.testLamp.enabled,"Covered actual lamp records P07");yield return Shot("04-lamp-off");
            CheckEmitter(false);
            Position(st.cableTarget,2.2f);Check(st.InspectCable(),"Physical cable cut can be inspected nearby");
            Check(st.Open("cable")&&!st.SubmitCable("deliberate-cut"),"P08 waits for separate cable frame");st.Close();
            yield return Expose(FieldCamera.StationCablePhotoId);
            Check(st.Open("cable")&&!st.SubmitCable("weathering")&&st.SubmitCable("deliberate-cut"),"Photographed cut completes P08; unsupported cause rejected");yield return Shot("05-cable-finding");
            st.Close();var timing=GameObject.Find("Station26.TimingRecord");Check(timing,"Physical timing record exists in hut");Position(timing.transform,1.3f);
            Check(st.Open("timing")&&st.ReadSource("e10")&&!st.SubmitTiming("02:17:00")&&st.SubmitTiming("02:17:47"),"Timing source distinguishes carrier loss and relay observation");yield return Shot("06-timing-finding");st.Close();
            string stationState=st.CaptureState();yield return Save("Field complete");yield return Reload("Inside station");g=GameSession.Instance;st=g.station;
            Check(st.InStation&&st.CaptureState()==stationState&&g.fieldCamera.FrameCount==4,"Field state and all photos restore inside station");
            Check(st.Travel(false),"Return starts");yield return WaitFor(()=>!st.Traveling&&!st.InStation,25,"Return finishes with saved SARO location");
            Check(RenderSettings.ambientLight==saroAmbient&&RenderSettings.fogColor==saroFog&&Mathf.Abs(RenderSettings.fogDensity-saroDensity)<.00001f&&LightingState()==saroLights,"Return restores the exact SARO ambient, fog and directional-light state");
            string returnState=st.CaptureState();yield return Save("SARO return");yield return Reload("SARO return");g=GameSession.Instance;
            Check(g.station.CaptureState()==returnState&&!g.station.stationRoot.activeSelf,"Return state and inactive field area restore");
            Check(g.chapter.CaptureState()==originalChapter&&Case.CaptureState()==archiveState,"Original chapter and archive deductions preserved");
            Check(g.fieldCamera.FrameCount==4,"All four actual photo textures restored");
            yield return Develop(FieldCamera.StationMarkerPhotoId);yield return Shot("07-developed-marker");g.chapter.ClosePanel();
            yield return Develop(FieldCamera.StationCablePhotoId);yield return Shot("08-developed-cable");g.chapter.ClosePanel();
            yield return Save("Developed field originals");yield return Reload("Developed field originals");g=GameSession.Instance;
            foreach(string id in new[]{FieldCamera.StationMarkerPhotoId,FieldCamera.StationCablePhotoId})
            {
                var frame=g.fieldCamera.GetFrame(id);
                Check(frame!=null&&frame.texture&&frame.developed&&frame.inspected,"Developed original reopens: "+id);
                Check(g.chapter.OpenEvidence(id),"Notebook photo handler accepts "+id);g.chapter.ClosePanel();
            }
            foreach(var pair in originals)Check(Convert.ToBase64String(File.ReadAllBytes(pair.Key))==Convert.ToBase64String(pair.Value),"Original bytes preserved: "+Path.GetFileName(pair.Key));
            yield return RecoveryAndFailure();
            Check(report.errors.Count==0,"No runtime errors in integrated journey");
        }
    }
}
