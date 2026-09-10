using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using UnityEngine;
using UnityEngine.Profiling;
using Signal47.Core;
using Signal47.Signals;
namespace Signal47.Debugging
{
    // Observation only: never moves the player or invokes game actions.
    public sealed class GauntletProbe:MonoBehaviour
    {
        [Serializable] sealed class State
        {
            public double elapsed,gameTime;public float x,y,z,yaw,pitch,frequency,gain,bandwidth,azimuth;
            public int width,height,evidence,observations,stages;public bool started,modal,paused,console,powered,printed,ringing,answered,lineDead,impact,title,focused;
            public bool cameraAcquired,cameraRaised,photoTaken,photoCompared,photoOpen;public int rejectedFrames;public string photoPath,photoSave,frameProblem;
            public bool yardActive,yardComplete,yardReturned,doorOpen;
            public string prompt;public float mouseX,mouseY;public bool mouseDown;
        }
        [Serializable] sealed class Report
        {
            public string unity,os,cpu,gpu,renderer,resolution,preset,buildId,method;
            public bool development,focusedThroughout,gpuTimingAvailable;public int frames,stallsOver50ms,uniqueTimingSamples,gc0,gc1,gc2,targetFrameRate,vSyncCount;
            public double seconds,averageFps,p95ms,p99ms,maxMs,cpuMeanMs,gpuMeanMs,engineMemoryPeakMiB,mainWorkMeanMs,renderWorkMeanMs,presentWaitMeanMs,mainWorkP95Ms,presentWaitP95Ms;
        }
        string root;double nextState,last;bool recording,allFocused=true;int shot;long peakMemory;ulong lastTimingStamp;int[] gcStart=new int[3];
        readonly List<double> times=new List<double>(20000),cpus=new List<double>(),gpus=new List<double>(),mainWork=new List<double>(),renderWork=new List<double>(),presentWait=new List<double>();
        readonly System.Text.StringBuilder detail=new System.Text.StringBuilder();
        readonly FrameTiming[] timing=new FrameTiming[1];SignalConsole console;
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        static void Init(){if(Array.IndexOf(System.Environment.GetCommandLineArgs(),"--signal47-gauntlet")>=0&&!FindFirstObjectByType<GauntletProbe>())new GameObject("GauntletProbe").AddComponent<GauntletProbe>();}
        void Awake(){DontDestroyOnLoad(gameObject);root=Path.GetFullPath(Path.Combine(Application.dataPath,"../../Gauntlet"));Directory.CreateDirectory(root);Application.runInBackground=true;last=Time.realtimeSinceStartupAsDouble;}
        void Update()
        {
            double now=Time.realtimeSinceStartupAsDouble;
            if(recording)
            {
                times.Add((now-last)*1000);allFocused&=Application.isFocused;FrameTimingManager.CaptureFrameTimings();
                if(FrameTimingManager.GetLatestTimings(1,timing)>0 && timing[0].frameStartTimestamp!=lastTimingStamp)
                {
                    var t=timing[0];lastTimingStamp=t.frameStartTimestamp;
                    if(t.cpuFrameTime>0)cpus.Add(t.cpuFrameTime);if(t.gpuFrameTime>0)gpus.Add(t.gpuFrameTime);
                    mainWork.Add(t.cpuMainThreadFrameTime);renderWork.Add(t.cpuRenderThreadFrameTime);presentWait.Add(t.cpuMainThreadPresentWaitTime);
                    detail.Append(times.Count-1).Append(',').Append(t.frameStartTimestamp).Append(',').Append(t.cpuFrameTime.ToString("F6",CultureInfo.InvariantCulture)).Append(',').Append(t.cpuMainThreadFrameTime.ToString("F6",CultureInfo.InvariantCulture)).Append(',').Append(t.cpuRenderThreadFrameTime.ToString("F6",CultureInfo.InvariantCulture)).Append(',').Append(t.cpuMainThreadPresentWaitTime.ToString("F6",CultureInfo.InvariantCulture)).Append(',').Append(t.gpuFrameTime.ToString("F6",CultureInfo.InvariantCulture)).Append(',').Append(GC.CollectionCount(0)).Append('\n');
                }
            }
            last=now;
            if(now<nextState)return;nextState=now+.1;
            var g=GameSession.Instance;if(!g)return;if(!console)console=FindFirstObjectByType<SignalConsole>();
            var p=g.player.transform;var c=g.player.viewCamera.transform;
            var state=new State{elapsed=now,gameTime=Time.timeAsDouble,x=p.position.x,y=p.position.y,z=p.position.z,yaw=p.eulerAngles.y,pitch=Mathf.DeltaAngle(0,c.localEulerAngles.x),width=Screen.width,height=Screen.height,started=g.hud.Started,modal=g.hud.ModalOpen,paused=g.hud.Paused,console=SignalConsole.AnyOpen,powered=g.director.ReceiverPowered,printed=g.director.PrintoutAvailable,ringing=g.director.PhoneRinging,answered=g.director.PhoneAnswered,lineDead=g.director.LineDead,impact=g.director.ImpactOccurred,title=g.hud.TitleVisible,evidence=g.notebook.Evidence.Count,observations=g.notebook.Entries.Count,prompt=g.hud.InteractionPrompt,focused=Application.isFocused,frequency=console.frequency,gain=console.gain,bandwidth=console.bandwidth,azimuth=console.azimuth,stages=console.CompletedStages};
            if(g.yard){state.yardActive=g.yard.Active;state.yardComplete=g.yard.Completed;state.yardReturned=g.yard.Returned;state.doorOpen=g.yard.door.Open;}
            if(g.fieldCamera){var f=g.fieldCamera;state.cameraAcquired=f.Acquired;state.cameraRaised=f.Raised;state.photoTaken=f.HasPhoto;state.photoCompared=f.Compared;state.photoOpen=g.hud.PhotoOpen;state.rejectedFrames=f.RejectedFrames;state.photoPath=f.PhotoPath;state.photoSave=f.SaveStatus;state.frameProblem=f.FrameProblem();}
            var mouse=UnityEngine.InputSystem.Mouse.current;if(mouse!=null){state.mouseX=mouse.position.ReadValue().x;state.mouseY=mouse.position.ReadValue().y;state.mouseDown=mouse.leftButton.isPressed;}
            File.WriteAllText(Path.Combine(root,"state.tmp"),JsonUtility.ToJson(state));File.Delete(Path.Combine(root,"state.json"));File.Move(Path.Combine(root,"state.tmp"),Path.Combine(root,"state.json"));
            peakMemory=Math.Max(peakMemory,Profiler.GetTotalAllocatedMemoryLong());
            var command=Path.Combine(root,"command.txt");if(!File.Exists(command))return;string action=File.ReadAllText(command).Trim();File.Delete(command);
            if(action=="record")
            {
                times.Clear();cpus.Clear();gpus.Clear();mainWork.Clear();renderWork.Clear();presentWait.Clear();lastTimingStamp=0;
                detail.Clear();detail.Append("observation_frame,frame_timestamp,cpu_total_ms,main_work_ms,render_work_ms,present_wait_ms,gpu_ms,gc0_total\n");
                for(int i=0;i<3;i++)gcStart[i]=GC.CollectionCount(i);
                last=now;allFocused=Application.isFocused;recording=true;peakMemory=0;
            }
            else if(action=="finish")Finish();
            else if(action=="shot")ScreenCapture.CaptureScreenshot(Path.Combine(root,$"journey-{++shot:00}.png"));
        }
        static double Mean(List<double> v){double sum=0;foreach(var n in v)sum+=n;return v.Count>0?sum/v.Count:0;}
        static double Percentile(List<double> values,double p){if(values.Count==0)return 0;var sorted=new List<double>(values);sorted.Sort();return sorted[Math.Max(0,(int)Math.Ceiling(sorted.Count*p)-1)];}
        void Finish()
        {
            if(!recording||times.Count==0)return;recording=false;double sum=0;int stalls=0;var csv=new System.Text.StringBuilder("frame,elapsed_ms\n");for(int i=0;i<times.Count;i++){sum+=times[i];if(times[i]>50)stalls++;csv.Append(i).Append(',').Append(times[i].ToString("F6",CultureInfo.InvariantCulture)).Append('\n');}
            File.WriteAllText(Path.Combine(root,"frames.csv"),csv.ToString());File.WriteAllText(Path.Combine(root,"frame-work.csv"),detail.ToString());
            string id=Path.Combine(Application.dataPath,"../build-id.txt");
            var report=new Report{unity=Application.unityVersion,os=SystemInfo.operatingSystem,cpu=SystemInfo.processorType,gpu=SystemInfo.graphicsDeviceName,renderer=SystemInfo.graphicsDeviceType.ToString(),resolution=$"{Screen.width}x{Screen.height}",preset=QualitySettings.names[QualitySettings.GetQualityLevel()],buildId=File.Exists(id)?File.ReadAllText(id).Trim():"unknown",development=UnityEngine.Debug.isDebugBuild,focusedThroughout=allFocused,frames=times.Count,seconds=sum/1000,averageFps=times.Count*1000/sum,p95ms=Percentile(times,.95),p99ms=Percentile(times,.99),maxMs=Percentile(times,1),stallsOver50ms=stalls,cpuMeanMs=Mean(cpus),gpuMeanMs=Mean(gpus),gpuTimingAvailable=gpus.Count>0,uniqueTimingSamples=mainWork.Count,mainWorkMeanMs=Mean(mainWork),renderWorkMeanMs=Mean(renderWork),presentWaitMeanMs=Mean(presentWait),mainWorkP95Ms=Percentile(mainWork,.95),presentWaitP95Ms=Percentile(presentWait,.95),gc0=GC.CollectionCount(0)-gcStart[0],gc1=GC.CollectionCount(1)-gcStart[1],gc2=GC.CollectionCount(2)-gcStart[2],targetFrameRate=Application.targetFrameRate,vSyncCount=QualitySettings.vSyncCount,engineMemoryPeakMiB=peakMemory/1048576.0,method="Monotonic Unity realtime Update intervals; nearest-rank percentiles; no frames removed; unique delayed FTM timestamps (not same-frame correlation); zero GPU timing means unavailable"};
            File.WriteAllText(Path.Combine(root,"performance.json"),JsonUtility.ToJson(report,true));UnityEngine.Debug.Log("SIGNAL47_GAUNTLET_MEASURED "+times.Count+" frames");
        }
        void OnGUI(){if(Event.current.type==EventType.MouseDown||Event.current.type==EventType.MouseUp)UnityEngine.Debug.Log($"GAUNTLET_MOUSE {Event.current.type} {Event.current.mousePosition}");}
        void OnApplicationQuit(){Finish();}
    }
}
