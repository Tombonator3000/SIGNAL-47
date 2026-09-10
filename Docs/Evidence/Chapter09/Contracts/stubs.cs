
using System;
using System.Collections.Generic;
using Newtonsoft.Json;
namespace UnityEngine {
 public class Object { public static implicit operator bool(Object x)=>x!=null; public static T FindFirstObjectByType<T>() where T:Object,new()=>new T(); }
 public class MonoBehaviour:Object{}
 public struct Vector3 { public float x,y,z; }
 public class Transform:Object {public Vector3 position,eulerAngles;}
 public static class Mathf {public static float Abs(float n)=>Math.Abs(n);}
 public static class JsonUtility {static readonly JsonSerializerSettings settings=new(){ConstructorHandling=ConstructorHandling.AllowNonPublicDefaultConstructor}; public static string ToJson(object o,bool pretty=false)=>JsonConvert.SerializeObject(o,pretty?Formatting.Indented:Formatting.None,settings); public static T FromJson<T>(string s)=>JsonConvert.DeserializeObject<T>(s,settings);}
 public static class Application {public static string persistentDataPath=System.IO.Path.Combine(System.IO.Path.GetTempPath(),"signal47-quit-test-unused");public static string version="QUIT-REGRESSION";public static event Func<bool> wantsToQuit;public static bool DidQuit;public static void Quit(){bool allowed=true;if(wantsToQuit!=null)foreach(Func<bool> h in wantsToQuit.GetInvocationList())allowed &= h();if(allowed)DidQuit=true;}}
 public static class Time {public static float timeScale=1;}
 public static class AudioListener {public static bool pause;}
 public static class Debug {public static void Log(object x)=>Console.WriteLine(x); public static void LogWarning(object x)=>Console.WriteLine(x);}
 public struct Color {public float r,g,b,a;public Color(float r,float g,float b,float a=1){this.r=r;this.g=g;this.b=b;this.a=a;}public static Color operator*(Color x,float n)=>new(x.r*n,x.g*n,x.b*n,x.a*n);}
 public sealed class Material {public Dictionary<string,Color> colors=new();public bool emission;public void SetColor(string key,Color color)=>colors[key]=color;public bool HasProperty(string key)=>true;public void EnableKeyword(string key)=>emission=true;}
 public sealed class Renderer:Object {public Material material=new();}
}
namespace Signal47.Core {
 public interface IInteractable {string Prompt{get;}void Interact();}
 public class GameSession:UnityEngine.Object {public static GameSession Instance=new();public FakePlayer player=new();public FakeHUD hud=new();public Investigation.Notebook notebook=new();public Investigation.FieldCamera fieldCamera=new();public Chapter.ChapterInvestigation chapter=new();public FakeYard yard=new();public FakeDirector director=new();public void Restart(){}}
 public class FakePlayer:UnityEngine.Object {public UnityEngine.Transform transform=new();public float ViewPitch;public void RestorePose(UnityEngine.Vector3 v,float y,float p){}}
 public class FakeHUD:UnityEngine.Object {public bool Started=true,Paused;public string ToastText="";public void SetPaused(bool p){Paused=p;}public void ResumeFromSave(){}public void Toast(string s,float seconds=0){ToastText=s;}}
 public class FakeYard:UnityEngine.Object {public bool Active=true,Completed=true,Returned=true;public FakeDoor door=new();public void RestoreState(bool c,bool r){}}
 public class FakeDoor:UnityEngine.Object {public bool Open=true;public void RestoreState(bool v){}}
 public class FakeDirector:UnityEngine.Object {public bool SignalAcquired=true,ReceiverPowered=true;public FakeMug mug=new();public UnityEngine.Object printer=new(),dishes=new();public void RestoreCompleted(UnityEngine.Vector3 v){}public void PowerReceiver(){ReceiverPowered=true;}}
 public class FakeMug:UnityEngine.Object {public UnityEngine.Transform transform=new();}
}
namespace Signal47.Signals {public class SignalConsole:UnityEngine.Object {public float frequency=1420.405f,gain=82,bandwidth=12,azimuth=83;public void RestoreCompleted(float f,float g,float b,float a){}}}
namespace Signal47.Investigation {
 public class Notebook:UnityEngine.Object {public string CaptureState()=>"{}";public void RestoreState(string s){}public static bool ValidateState(string s,out string r){r="";return s!=null;}}
 public class FieldCamera:UnityEngine.Object {public bool Capturing,SaveReady=true;public string SaveStatus="";public string CaptureState()=>"{}";public void RestoreState(string s){}public static bool ValidateState(string s,out string r){r="";return s!=null;}}
}
namespace Signal47.Chapter {
 public class ChapterInvestigation:UnityEngine.Object {public bool CanSave=true;public string TestState="old";public string CaptureState()=>TestState;public void RestoreState(string s){}public static bool ValidateState(string s,out string r){r="";return s!=null;}}
 public static class PlayerSettings {public static void Load(){}public static void Apply(){}}
}
