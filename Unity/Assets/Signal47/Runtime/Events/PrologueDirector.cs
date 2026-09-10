using System.Collections;
using UnityEngine;
using Signal47.Core;
using Signal47.Environment;
using Signal47.Interaction;
using Signal47.Audio;
namespace Signal47.Events
{
    public sealed class PrologueDirector:MonoBehaviour
    {
        public bool ReceiverPowered{get;private set;}
        public bool PrintoutAvailable{get;private set;}
        public bool PhoneRinging{get;private set;}
        public bool PhoneAnswered{get;private set;}
        public bool LineDead{get;private set;}
        public bool ImpactOccurred{get;private set;}
        public bool ArrayOverride{get;private set;}
        public bool SignalAcquired{get;private set;}
        public double LineDeadAt{get;private set;}
        public double ImpactAt{get;private set;}
        public const double FutureDelay=47;
        public AudioSource printerSource,phoneSource,eventSource,ambienceSource;
        public MugBreakable mug;public DishArrayController dishes;public PrinterMechanism printer;
        public SoundPalette palette;bool sequenceStarted;AudioClip futureCall;
        void Start(){if(ambienceSource){ambienceSource.volume=.36f;ambienceSource.clip=ProceduralAudio.Hum();ambienceSource.loop=true;ambienceSource.Play();}if(printerSource)printerSource.volume=.42f;if(phoneSource){phoneSource.volume=.68f;phoneSource.clip=palette?palette.phone:ProceduralAudio.Ring();}futureCall=ProceduralAudio.FutureCall(palette?palette.smash:null);}
        public void PowerReceiver(){if(ReceiverPowered)return;ReceiverPowered=true;if(palette)palette.Click(true);GameSession.Instance.notebook.Add("Receiver bank 3 powered on.");GameSession.Instance.hud.Toast("RECEIVER BANK 3 // ONLINE");}
        public void OnSignalSolved()
        {
            if(sequenceStarted)return;sequenceStarted=true;StartCoroutine(PrintAndRing());
        }
        IEnumerator PrintAndRing()
        {
            double ringAt=Time.timeAsDouble+3.6;
            if(printerSource){printerSource.clip=palette?palette.printer:ProceduralAudio.Printer();printerSource.Play();}
            if(printer)yield return printer.Print();else yield return new WaitForSeconds(1.2f);
            PrintoutAvailable=true;GameSession.Instance.hud.Toast("The printer feeds out a sheet.");
            yield return Until(ringAt);PhoneRinging=true;
            if(phoneSource){phoneSource.loop=true;phoneSource.Play();}
        }
        public void AnswerPhone()
        {
            if(!PhoneRinging)return;PhoneRinging=false;PhoneAnswered=true;
            if(phoneSource){phoneSource.Stop();phoneSource.volume=.90f;phoneSource.loop=false;phoneSource.clip=futureCall;phoneSource.Play();}
            GameSession.Instance.notebook.Add("Desk phone received an unlogged incoming call.");
            GameSession.Instance.hud.Toast("The handset carries only room tone and static…",2.8f);StartCoroutine(FutureSequence());
        }
        void OnDestroy(){if(futureCall)Destroy(futureCall);}
        public void RestoreCompleted(Vector3 brokenMugPosition)
        {
            // Chapter checkpoints are only taken after the live 47-second sequence.
            // Loading restores its consequences without replaying timed events or audio.
            StopAllCoroutines();sequenceStarted=true;
            ReceiverPowered=true;PrintoutAvailable=true;PhoneRinging=false;PhoneAnswered=true;
            LineDead=true;ImpactOccurred=true;ArrayOverride=true;SignalAcquired=true;
            ImpactAt=Time.timeAsDouble;LineDeadAt=ImpactAt-FutureDelay;
            if(phoneSource){phoneSource.Stop();phoneSource.loop=false;}
            if(printerSource)printerSource.Stop();if(eventSource)eventSource.Stop();
            if(dishes)dishes.RestoreAligned();if(printer)printer.RestoreReady();if(mug)mug.RestoreBroken(brokenMugPosition);
        }
        static IEnumerator Until(double deadline){while(Time.timeAsDouble<deadline)yield return null;}
        IEnumerator FutureSequence()
        {
            yield return new WaitForSeconds(4.3f);LineDead=true;LineDeadAt=Time.timeAsDouble;
            GameSession.Instance.notebook.Collect("call-notes","Unlogged telephone call","FIELD TRANSCRIPT // INTERNAL LINE\n\nNo voice. Room hum, static, a heavy impact, then breaking ceramic.\n\nThe connection ended without a dial tone.\n\nNo recording was made. This is a written observation.");
            GameSession.Instance.hud.Toast("The line goes dead.",1.5f);
            yield return Until(LineDeadAt+16);if(eventSource){eventSource.clip=ProceduralAudio.Hum();eventSource.volume=.12f;eventSource.Play();}
            yield return Until(LineDeadAt+31);if(eventSource){eventSource.clip=ProceduralAudio.Printer();eventSource.volume=.06f;eventSource.Play();}
            yield return Until(LineDeadAt+FutureDelay);ImpactAt=Time.timeAsDouble;ImpactOccurred=true;
            if(eventSource){eventSource.volume=1;eventSource.clip=ProceduralAudio.Boom();eventSource.Play();}
            yield return mug.DropAndBreak();
            if(palette)palette.BreakAt(mug.transform.position);else if(eventSource){eventSource.transform.position=mug.transform.position;eventSource.clip=ProceduralAudio.Smash();eventSource.Play();}
            GameSession.Instance.notebook.Add("Coffee mug broke immediately after a distant impact.");
            yield return new WaitForSeconds(3f);ArrayOverride=true;dishes.BeginTurn();
            yield return new WaitUntil(()=>dishes.Completed);SignalAcquired=true;
            GameSession.Instance.notebook.Add("All dishes left their scheduled track and aligned together.");
            yield return new WaitForSeconds(1.5f);GameSession.Instance.hud.ShowTitle();
        }
    }
}
