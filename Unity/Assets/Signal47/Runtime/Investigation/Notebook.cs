using System.Collections.Generic;
using UnityEngine;
namespace Signal47.Investigation
{
    public sealed class Notebook : MonoBehaviour
    {
        readonly List<string> entries = new();
        public IReadOnlyList<string> Entries => entries;
        public void Add(string entry) { if (!entries.Contains(entry)) entries.Add(entry); }
        public void Clear() => entries.Clear();
    }
}
