using UnityEngine;
using Signal47.Core;
namespace Signal47.Interaction
{
    public sealed class PaperInteractable:MonoBehaviour,IInteractable
    {
        public enum PaperKind{Clipboard,Printer} public PaperKind kind; public string Prompt=>kind==PaperKind.Clipboard?"SHIFT CLIPBOARD":"DOT-MATRIX PRINTER";
        public void Interact()
        {
            if(kind==PaperKind.Printer&&!GameSession.Instance.director.PrintoutAvailable){GameSession.Instance.hud.Toast("Printer idle. No queued output.");return;}
            string text=kind==PaperKind.Clipboard?"SARO // NIGHT SHIFT LOG\n\nSHIFT: 23:30 — 07:30\nLOCAL: 23:41\n\n[ ] POWER RECEIVER BANK 3\n[ ] CALIBRATE ORION OBSERVATION\n[ ] IDENTIFY / NOTCH LOCAL INTERFERENCE\n\nWEATHER: dry / distant electrical activity\nARRAY: scheduled track nominal\n\n\"Night’s yours. Don’t break anything. —R.\"":"SARO // DIRECTION SOLVE OUTPUT\n\nSOURCE: UNKNOWN\nFREQ: 1420.405 MHz\nRA: 05h 17m 32s\nDEC: -05° 23' 14\"\nS/N: 4.71\n\nDISTANCE SOLVE: -39 LY\n\n*** RANGE SIGN ERROR ***\n*** RE-RUN REQUIRED ***";
            GameSession.Instance.hud.ShowPaper(text);
        }
    }
}
