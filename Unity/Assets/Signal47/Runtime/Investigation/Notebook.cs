using System;
using System.Collections.Generic;
using UnityEngine;
namespace Signal47.Investigation
{
    [Serializable]
    public sealed class EvidenceRecord
    {
        public string Id,Title,Body;
        public EvidenceRecord(string id,string title,string body){Id=id;Title=title;Body=body;}
    }
    public sealed class Notebook : MonoBehaviour
    {
        [Serializable]
        public sealed class SavedState
        {
            public int version=1;
            public double elapsedSeconds;
            public string[] entries,observations;
            public EvidenceRecord[] evidence;
        }
        readonly List<string> entries=new();
        readonly HashSet<string> observations=new();
        readonly List<EvidenceRecord> evidence=new();
        double clockOffset;
        public IReadOnlyList<string> Entries=>entries;
        public IReadOnlyList<EvidenceRecord> Evidence=>evidence;
        public double LocalClockSeconds=>23*3600+41*60+Time.timeSinceLevelLoad+clockOffset;
        public void Add(string entry)
        {
            if(!observations.Add(entry))return;
            var clock=TimeSpan.FromSeconds(LocalClockSeconds);
            entries.Add($"{clock.Hours:00}:{clock.Minutes:00}:{clock.Seconds:00}  {entry}");
        }
        public bool Collect(string id,string title,string body)
        {
            if(evidence.Exists(e=>e.Id==id))return false;
            evidence.Add(new EvidenceRecord(id,title,body));Add("Filed: "+title+".");return true;
        }
        public bool RemoveEvidence(string id)=>evidence.RemoveAll(e=>e.Id==id)>0;
        public string CaptureState()=>JsonUtility.ToJson(new SavedState{elapsedSeconds=Time.timeSinceLevelLoad+clockOffset,entries=entries.ToArray(),observations=new List<string>(observations).ToArray(),evidence=evidence.ToArray()});
        public static bool ValidateState(string json,out string reason)
        {
            reason="Invalid notebook in saved case.";
            try
            {
                if(string.IsNullOrEmpty(json)||json.Length>524288)return false;
                var state=JsonUtility.FromJson<SavedState>(json);
                if(state==null||state.version!=1||state.entries==null||state.observations==null||state.evidence==null||state.entries.Length>2048||state.observations.Length>2048||state.evidence.Length>256||double.IsNaN(state.elapsedSeconds)||double.IsInfinity(state.elapsedSeconds)||state.elapsedSeconds<0||state.elapsedSeconds>1e8)return false;
                foreach(var entry in state.entries)if(string.IsNullOrEmpty(entry)||entry.Length>8192)return false;
                var seen=new HashSet<string>();
                foreach(var observation in state.observations)if(string.IsNullOrEmpty(observation)||observation.Length>8192||!seen.Add(observation))return false;
                seen.Clear();
                foreach(var item in state.evidence)if(item==null||string.IsNullOrEmpty(item.Id)||item.Id.Length>128||string.IsNullOrEmpty(item.Title)||item.Title.Length>512||item.Body==null||item.Body.Length>32768||!seen.Add(item.Id))return false;
                reason="";return true;
            }
            catch(ArgumentException){return false;}
        }
        public void RestoreState(string json)
        {
            if(!ValidateState(json,out string reason))throw new ArgumentException(reason,nameof(json));
            var state=JsonUtility.FromJson<SavedState>(json);
            Clear();entries.AddRange(state.entries);observations.UnionWith(state.observations);evidence.AddRange(state.evidence);
            clockOffset=state.elapsedSeconds-Time.timeSinceLevelLoad;
        }
        public void Clear(){entries.Clear();observations.Clear();evidence.Clear();clockOffset=0;}
    }
}
