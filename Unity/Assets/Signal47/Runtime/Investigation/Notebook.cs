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
        readonly List<string> entries=new();
        readonly HashSet<string> observations=new();
        readonly List<EvidenceRecord> evidence=new();
        public IReadOnlyList<string> Entries=>entries;
        public IReadOnlyList<EvidenceRecord> Evidence=>evidence;
        public void Add(string entry)
        {
            if(!observations.Add(entry))return;
            var clock=TimeSpan.FromSeconds(23*3600+41*60+Time.timeSinceLevelLoad);
            entries.Add($"{clock.Hours:00}:{clock.Minutes:00}:{clock.Seconds:00}  {entry}");
        }
        public bool Collect(string id,string title,string body)
        {
            if(evidence.Exists(e=>e.Id==id))return false;
            evidence.Add(new EvidenceRecord(id,title,body));Add("Filed: "+title+".");return true;
        }
        public void Clear(){entries.Clear();observations.Clear();evidence.Clear();}
    }
}
