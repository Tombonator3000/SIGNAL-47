using UnityEngine;
using Signal47.Core;

namespace Signal47.Chapter
{
    public sealed class ChapterAction : MonoBehaviour, IInteractable
    {
        public ChapterInvestigation investigation;
        public string action;
        public string label;
        public string Prompt => investigation ? investigation.ActionPrompt(action, label) : label;
        public void Interact() { if (investigation) investigation.Interact(action); }
    }
}
