using UnityEngine;
using Signal47.Core;

namespace Signal47.Station26
{
    /// <summary>Physical interaction proxy for the station's bench, lamp, cable and log.</summary>
    public sealed class StationAction : MonoBehaviour, IInteractable
    {
        public StationController controller;
        // Station26Build serializes the selected interaction as "page".  Keep
        // action as a compatibility alias for hand-authored scene proxies.
        public string page;
        public string action;
        public string label;

        string Selected => string.IsNullOrEmpty(action) ? page : action;
        public string Prompt => controller ? controller.ActionPrompt(Selected, label) : label;
        public void Interact() { if (controller) controller.Interact(Selected); }
    }
}
