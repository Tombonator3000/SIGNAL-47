using UnityEngine;
using Signal47.Core;
namespace Signal47.Interaction
{
    public sealed class PaperInteractable:MonoBehaviour,IInteractable
    {
        public enum PaperKind{Clipboard,Printer}
        public PaperKind kind;
        public const string Printout="SARO // DIRECTION SOLVE OUTPUT\n\nSOURCE: UNKNOWN\nFREQ: 1420.405 MHz\nRA: 05h 17m 32s\nDEC: -05° 23' 14\"\nS/N: 4.71\n\nDISTANCE SOLVE: -39 LY\n\n*** RANGE SIGN ERROR ***\n*** RE-RUN REQUIRED ***";
        const string ShiftLog="SARO // NIGHT SHIFT LOG\n\nSHIFT: 23:30 — 07:30\nLOCAL: 23:41\n\n1. POWER RECEIVER BANK 3\n2. CALIBRATE ORION OBSERVATION\n3. IDENTIFY / NOTCH LOCAL INTERFERENCE\n\nWEATHER: dry / distant electrical activity\nARRAY: scheduled track nominal\n\n\"Night’s yours. Don’t break anything. —R.\"";
        public string Prompt=>kind==PaperKind.Clipboard?"READ SHIFT CLIPBOARD":GameSession.Instance.director.PrintoutAvailable?"READ PRINTOUT":"DOT-MATRIX PRINTER";
        public void Interact()
        {
            var g=GameSession.Instance;
            if(kind==PaperKind.Printer&&!g.director.PrintoutAvailable){g.hud.Toast(g.director.printer&&g.director.printer.Printing?"The print head is still moving.":"Printer idle. No queued output.");return;}
            string text=kind==PaperKind.Clipboard?ShiftLog:Printout;
            bool collected=g.notebook.Collect(kind==PaperKind.Clipboard?"shift-log":"direction-solve",kind==PaperKind.Clipboard?"Night shift instructions":"Direction solve printout",text);
            if(collected&&kind==PaperKind.Printer)g.notebook.Add("The printed distance is negative: -39 LY.");
            g.hud.ShowPaper(text);
        }
    }
}
