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
        public bool ReceiverPowered{get;private set;} public bool PrintoutAvailable{get;private set;} public bool PhoneRinging{get;private set;} public bool PhoneAnswered{get;private set;}
        public AudioSource printerSource,phoneSource,eventSource,ambienceSource; public MugBreakable mug; public DishArrayController dishes;
        bool sequenceStarted;
        void Start(){if(ambienceSource){ambienceSource.clip=ProceduralAudio.Hum();ambienceSource.loop=true;ambienceSource.Play();}if(phoneSource)phoneSource.clip=ProceduralAudio.Ring();}
        public void PowerReceiver(){ReceiverPowered=true;GameSession.Instance.notebook.Add("Receiver bank 3 powered on.");GameSession.Instance.hud.Toast("RECEIVER BANK 3 // ONLINE");}
        public void OnSignalSolved()
        {
            if(sequenceStarted)return;sequenceStarted=true;PrintoutAvailable=true;if(printerSource){printerSource.clip=ProceduralAudio.Printer();printerSource.Play();}StartCoroutine(RingDelay());
        }
        IEnumerator RingDelay(){yield return new WaitForSeconds(3.6f);PhoneRinging=true;if(phoneSource){phoneSource.loop=true;phoneSource.Play();}GameSession.Instance.hud.Toast("A telephone rings somewhere in the control room.",2.6f);}
        public void AnswerPhone()
        {
            if(!PhoneRinging)return;PhoneRinging=false;PhoneAnswered=true;if(phoneSource){phoneSource.Stop();phoneSource.loop=false;phoneSource.clip=ProceduralAudio.FutureCall();phoneSource.Play();}
            GameSession.Instance.notebook.Add("Desk phone received an unlogged incoming call.");GameSession.Instance.hud.Toast("The handset carries only room tone and static…",2.8f);StartCoroutine(FutureSequence());
        }
        IEnumerator FutureSequence()
        {
            yield return new WaitForSeconds(4.3f);GameSession.Instance.notebook.Add("Call audio contained a heavy impact and ceramic break.");GameSession.Instance.hud.Toast("The line goes dead.",1.5f);
            yield return new WaitForSeconds(16f);if(eventSource){eventSource.clip=ProceduralAudio.Hum();eventSource.volume=.12f;eventSource.Play();}
            yield return new WaitForSeconds(15f); // 31 seconds elapsed; second subtle anomaly
            if(eventSource){eventSource.clip=ProceduralAudio.Printer();eventSource.volume=.06f;eventSource.Play();}
            yield return new WaitForSeconds(16f); // exact 47 seconds after line dead
            if(eventSource){eventSource.volume=1;eventSource.clip=ProceduralAudio.Boom();eventSource.Play();}GameSession.Instance.hud.Toast("A heavy impact rolls across the desert.",1.7f);
            yield return StartCoroutine(mug.DropAndBreak());if(eventSource){eventSource.clip=ProceduralAudio.Smash();eventSource.Play();}GameSession.Instance.notebook.Add("Coffee mug broke immediately after a distant impact.");
            yield return new WaitForSeconds(3f);dishes.BeginTurn();
            yield return new WaitForSeconds(1.344f);GameSession.Instance.hud.Toast("SIGNAL ACQUIRED",3f);
            yield return new WaitForSeconds(2.976f);GameSession.Instance.hud.ShowTitle();
        }
    }
}
