
using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using System.Reflection;
using System.Diagnostics;
using Newtonsoft.Json.Linq;
using Signal47.Chapter;
using Signal47.Core;
using UnityEngine;
class Program {
 static readonly BindingFlags Flags=BindingFlags.NonPublic|BindingFlags.Instance|BindingFlags.Static;
 static ChapterSave save;
 static void Call(string name)=>typeof(ChapterSave).GetMethod(name,Flags).Invoke(save,null);
 static object Field(string name)=>typeof(ChapterSave).GetField(name,Flags).GetValue(null);
 static void Set(string name,object value)=>typeof(ChapterSave).GetField(name,Flags).SetValue(null,value);
 static void Assert(bool yes,string name){if(!yes)throw new Exception("FAIL "+name);Console.WriteLine("PASS "+name);}
 static string Stored()=>JObject.Parse((string)JObject.Parse(File.ReadAllText(ChapterSave.SavePath))["payload"])["chapter"].Value<string>();
 static void Pump(Func<bool> stop,int limit=3000){var sw=Stopwatch.StartNew();while(!stop()&&sw.ElapsedMilliseconds<limit){Call("Update");Thread.Sleep(4);}}
 static (ManualResetEventSlim,Task) HoldWriter(){var entered=new ManualResetEventSlim();var release=new ManualResetEventSlim();var t=Task.Run(()=>{lock(Field("FileGate")){entered.Set();release.Wait();}});entered.Wait();return(release,t);}
 static void Prepare(string value){UnityEngine.Application.DidQuit=false;Set("quitAllowed",false);Set("quitPending",false);Set("writeFailed",false);GameSession.Instance.chapter.TestState=value;ChapterSave.RequestCheckpoint();Call("Update");}
 static int Main(){
  try{
   save=new ChapterSave();Call("Awake");
   var held=HoldWriter();Prepare("comparison-before-file");GameSession.Instance.chapter.TestState="filed-complete-latest";ChapterSave.RequestCheckpoint();UnityEngine.Application.Quit();Assert(!UnityEngine.Application.DidQuit&&ChapterSave.QuitPending,"Quit defers while older write owns disk");held.Item1.Set();held.Item2.Wait();Pump(()=>UnityEngine.Application.DidQuit);Assert(UnityEngine.Application.DidQuit,"Quit permitted after queue drains");Assert(Stored()=="filed-complete-latest","Queued newer completion snapshot survives slow older write");
   held=HoldWriter();Prepare("old-forced");GameSession.Instance.chapter.TestState="new-forced";ChapterSave.RequestCheckpoint();var delayed=Task.Run(()=>{Thread.Sleep(100);held.Item1.Set();});Call("OnApplicationQuit");delayed.Wait();held.Item2.Wait();Assert(Stored()=="new-forced","Fallback OnApplicationQuit drains newer queued snapshot with shared budget");
   held=HoldWriter();Prepare("old-timeout");GameSession.Instance.chapter.TestState="new-timeout";ChapterSave.RequestCheckpoint();UnityEngine.Application.Quit();var clock=Stopwatch.StartNew();Pump(()=>!ChapterSave.QuitPending,2600);Assert(!UnityEngine.Application.DidQuit&&!ChapterSave.QuitPending&&clock.ElapsedMilliseconds>=1900&&clock.ElapsedMilliseconds<2600,"Slow disk cancels Quit within the two-second budget and keeps process alive");Assert(GameSession.Instance.hud.Paused&&GameSession.Instance.chapter.TestState=="new-timeout","Timeout preserves latest live world and leaves readable pause");held.Item1.Set();held.Item2.Wait();Pump(()=>!ChapterSave.Saving&&!(bool)Field("checkpointRequested"));Call("Update");Assert(Stored()=="new-timeout"&&!UnityEngine.Application.DidQuit,"Latest snapshot finishes after timeout without unexpected exit");
   var fail=new TaskCompletionSource<bool>();Set("writeJob",fail.Task);UnityEngine.Application.Quit();fail.SetException(new IOException("deliberate failed write"));Call("Update");Assert(!UnityEngine.Application.DidQuit&&!ChapterSave.QuitPending,"Write failure cancels Quit instead of losing active world");
   var receiver=new Signal47.Interaction.ReceiverBank{leds=new[]{new Renderer()}};receiver.ApplyPoweredVisuals();var material=receiver.leds[0].material;Assert(material.emission&&Math.Abs(material.colors["_BaseColor"].g-1)<.0001&&Math.Abs(material.colors["_EmissionColor"].g-2)<.0001,"Receiver restored through same green base and emission state as power-on");
   Call("OnDestroy");Console.WriteLine("Focused quit-regression checks complete; Unity API/scene/input remain mocked.");return 0;
  }catch(Exception e){Console.WriteLine(e);return 1;}
 }
}
