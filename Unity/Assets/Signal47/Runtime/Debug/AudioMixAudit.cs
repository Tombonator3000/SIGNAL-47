using System;
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using Signal47.Core;
namespace Signal47.Debugging
{
    /// <summary>Optional GAME listener sampling only; never records microphones or desktop audio.</summary>
    public sealed class AudioMixAudit:MonoBehaviour
    {
        [Serializable] sealed class Phase
        {
            public string name;public long samples,samplesAtOrAboveOne;public int windows,nonSilentWindows;
            public double peak,rms;[NonSerialized]public double sumSquares;
        }
        [Serializable] sealed class Report
        {
            public string method,buildId,coverage,subjectiveListening="UNVERIFIED";
            public int sampleRate;public bool audioDataObserved;public Phase[] phases;
        }
        readonly Dictionary<string,Phase> phases=new Dictionary<string,Phase>();
        readonly float[] samples=new float[1024];float nextSample,nextWrite;bool observed;string root;
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        static void Init()
        {
            if(Array.IndexOf(System.Environment.GetCommandLineArgs(),"--signal47-audio-audit")>=0&&!FindFirstObjectByType<AudioMixAudit>())new GameObject("AudioMixAudit").AddComponent<AudioMixAudit>();
        }
        void Awake(){DontDestroyOnLoad(gameObject);root=Path.GetFullPath(Path.Combine(Application.dataPath,"../../Gauntlet"));Directory.CreateDirectory(root);}
        void Update()
        {
            if(Time.unscaledTime<nextSample)return;nextSample=Time.unscaledTime+.05f;
            var g=GameSession.Instance;if(!g)return;var d=g.director;
            string name=g.hud.Paused?"paused":g.hud.TitleVisible?"title":d.ImpactOccurred?"impact":d.LineDead?"line-dead":d.PhoneAnswered?"future-call":d.PhoneRinging?"ringing":d.ReceiverPowered?"receiver":"room";
            if(!phases.TryGetValue(name,out var phase)){phase=new Phase{name=name};phases.Add(name,phase);}
            phase.windows++;bool nonSilent=false;
            // A mono output has only channel zero. Non-stereo layouts are not fully covered by this audit.
            int channels=AudioSettings.speakerMode==AudioSpeakerMode.Mono?1:2;
            for(int channel=0;channel<channels;channel++)
            {
                AudioListener.GetOutputData(samples,channel);
                foreach(float sample in samples)
                {
                    double absolute=Math.Abs(sample);phase.samples++;phase.peak=Math.Max(phase.peak,absolute);phase.sumSquares+=sample*(double)sample;
                    if(absolute>=1)phase.samplesAtOrAboveOne++;if(absolute>0.000001)nonSilent=true;
                }
            }
            if(nonSilent){phase.nonSilentWindows++;observed=true;}
            if(Time.unscaledTime>=nextWrite){nextWrite=Time.unscaledTime+5;Write();}
        }
        void Write()
        {
            var list=new List<Phase>(phases.Values);foreach(var phase in list)phase.rms=phase.samples>0?Math.Sqrt(phase.sumSquares/phase.samples):0;
            string stamp=Path.Combine(Application.dataPath,"../build-id.txt");
            var report=new Report{method="AudioListener.GetOutputData, 1024-sample recent-history windows every >=50ms, game channels 0/1 (mono 0 only). All returned samples included. WINDOWS ARE NOT A CONTINUOUS RECORDING; no true-peak or LUFS claim.",buildId=File.Exists(stamp)?File.ReadAllText(stamp).Trim():"unknown",coverage=observed?"SAMPLED GAME OUTPUT ONLY; clipping between windows and subjective balance remain UNVERIFIED":"UNVERIFIED: no nonzero game output received",sampleRate=AudioSettings.outputSampleRate,audioDataObserved=observed,phases=list.ToArray()};
            File.WriteAllText(Path.Combine(root,"audio-audit.json"),JsonUtility.ToJson(report,true));
        }
        void OnApplicationQuit(){if(root!=null)Write();}
    }
}
