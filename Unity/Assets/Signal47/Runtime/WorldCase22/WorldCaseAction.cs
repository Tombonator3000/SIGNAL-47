using UnityEngine;
using Signal47.Core;
namespace Signal47.WorldCase22
{
    public sealed class WorldCaseAction : MonoBehaviour,IInteractable
    {
        public WorldCaseController controller;
        public string Prompt=>controller&&controller.Available?"READ STATION 01 ARCHIVE DOSSIER":"ARCHIVE DOSSIER / FILE THE B-12 REPORT FIRST";
        public void Interact()
        {
            if(controller&&controller.Open())return;
            if(GameSession.Instance&&GameSession.Instance.hud)GameSession.Instance.hud.Toast("File the two-exposure report before opening the historical dossier.",4);
        }
    }
}
