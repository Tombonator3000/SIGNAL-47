using System;
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using Signal47.Core;

namespace Signal47.Debugging
{
    // Opt-in sampled output audit. It is separate from the performance run and does
    // not claim continuous capture, loudspeaker playback or subjective listening.
    public sealed class AudioOutputAudit : MonoBehaviour
    {
        [Serializable] sealed class Level
        {
            public string state;public int windows,samples,overFullScale;
            public double peak,rms;[NonSerialized] public double squares;
        }
        [Serializable] sealed class Report
        {
            public string method,buildId;public int sampleRate;public bool dspAdvanced;
            public Level[] levels;
        }
        readonly float[] buffer=new float[2048];readonly Dictionary<string,Level> levels=new Dictionary<string,Level>();
        double startDsp;float next;
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        static void Init()
        {
            if(Array.IndexOf(System.Environment.GetCommandLineArgs(),"--signal47-audio-audit")>=0&&!FindFirstObjectByType<AudioOutputAudit>())
                new GameObject("AudioOutputAudit").AddComponent<AudioOutputAudit>();
        }
        void Awake(){DontDestroyOnLoad(gameObject);startDsp=AudioSettings.dspTime;}
        void Update()
        {
            if(Time.unscaledTime<next)return;next=Time.unscaledTime+.04f;
            var g=GameSession.Instance;if(!g)return;
            string state=!g.hud.Started?"start":g.hud.Paused?"paused":g.hud.TitleVisible?"title":g.director.ImpactOccurred?"impact":g.director.LineDead?"line-dead":g.director.PhoneAnswered?"call":g.director.PhoneRinging?"ring":g.director.printerSource.isPlaying?"printer":g.director.ReceiverPowered?"receiver":"room";
            if(g.hud.Started&&!g.hud.Paused&&g.yard&&g.yard.Active&&g.chapter)
                state="chapter-"+(g.chapter.ModalOpen?g.chapter.CurrentPanel:g.chapter.Stage);
            if(!levels.TryGetValue(state,out var level))levels[state]=level=new Level{state=state};
            AudioListener.GetOutputData(buffer,0);level.windows++;
            foreach(float sample in buffer){level.peak=Math.Max(level.peak,Math.Abs(sample));level.squares+=sample*sample;level.samples++;if(Math.Abs(sample)>=1)level.overFullScale++;}
        }
        void OnApplicationQuit()
        {
            foreach(var level in levels.Values)level.rms=Math.Sqrt(level.squares/Math.Max(1,level.samples));
            string dir=Path.GetFullPath(Path.Combine(Application.dataPath,"../../Gauntlet"));Directory.CreateDirectory(dir);
            string stamp=Path.Combine(Application.dataPath,"../build-id.txt");
            var report=new Report{method="Unity AudioListener channel 0: 2048-sample windows about every 40 ms; windows may overlap or leave gaps. No subjective listening claim.",buildId=File.Exists(stamp)?File.ReadAllText(stamp).Trim():"unknown",sampleRate=AudioSettings.outputSampleRate,dspAdvanced=AudioSettings.dspTime>startDsp,levels=new List<Level>(levels.Values).ToArray()};
            File.WriteAllText(Path.Combine(dir,"audio-audit.json"),JsonUtility.ToJson(report,true));
        }
    }
}
