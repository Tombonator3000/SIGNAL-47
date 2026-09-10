using System;
using System.IO;
using UnityEngine;
using Signal47.Core;

namespace Signal47.Chapter
{
    public static class PlayerSettings
    {
        [Serializable] sealed class Values
        {
            public int version=2;
            public float masterVolume=1,mouseSensitivity=.085f;
            public bool fullscreen;
            public int windowWidth=1280,windowHeight=800;
        }
        static Values values=new();
        static bool loaded,displayInitialized,displayRequestPending,requestedFullscreen;
        static int requestedWidth,requestedHeight;
        public static string Status{get;private set;}="";
        public static float MasterVolume
        {
            get{Load();return values.masterVolume;}
            set{Load();values.masterVolume=Finite(value)?Mathf.Clamp01(value):1;}
        }
        public static float MouseSensitivity
        {
            get{Load();return values.mouseSensitivity;}
            set{Load();values.mouseSensitivity=Finite(value)?Mathf.Clamp(value,.02f,.25f):.085f;}
        }
        public static bool Fullscreen
        {
            get{Load();return values.fullscreen;}
            set
            {
                Load();
                if(value&&!values.fullscreen)RememberWindowSize();
                values.fullscreen=value;
            }
        }
        static bool Finite(float value)=>!float.IsNaN(value)&&!float.IsInfinity(value);
        static bool ValidWindowSize(int width,int height)=>width>=320&&height>=200&&width<=16384&&height<=16384;
        static void RememberWindowSize()
        {
            // SetResolution completes later. Never mistake an in-flight fullscreen
            // framebuffer for the last window size the player chose.
            if(!displayRequestPending&&!Screen.fullScreen&&ValidWindowSize(Screen.width,Screen.height))
            {values.windowWidth=Screen.width;values.windowHeight=Screen.height;}
        }
        public static void Load()
        {
            if(loaded)return;loaded=true;values.fullscreen=Screen.fullScreen;RememberWindowSize();
            if(string.IsNullOrEmpty(ChapterSave.StorageDirectory)){Status="Settings location unavailable.";return;}
            string path=Path.Combine(ChapterSave.StorageDirectory,"settings.json");
            try
            {
                if(!File.Exists(path))return;
                if(new FileInfo(path).Length>4096)throw new InvalidDataException("Settings file is too large.");
                var saved=JsonUtility.FromJson<Values>(File.ReadAllText(path));
                if(saved==null||(saved.version!=1&&saved.version!=2)||!Finite(saved.masterVolume)||!Finite(saved.mouseSensitivity)||saved.masterVolume<0||saved.masterVolume>1||saved.mouseSensitivity<.02f||saved.mouseSensitivity>.25f)
                    throw new InvalidDataException("Settings data is invalid.");
                if(saved.version==1)
                {
                    // Version 1 had only a boolean. Keep its preferences and use the
                    // current window as the first return size without rewriting on load.
                    saved.windowWidth=values.windowWidth;saved.windowHeight=values.windowHeight;saved.version=2;
                }
                if(!ValidWindowSize(saved.windowWidth,saved.windowHeight))throw new InvalidDataException("Saved window dimensions are invalid.");
                values=saved;
            }
            catch(Exception error) when(error is IOException||error is UnauthorizedAccessException||error is ArgumentException)
            {Status="Using default settings; the unreadable file was preserved.";Debug.LogWarning("CHAPTER_SETTINGS_LOAD_FAILED "+error.Message);}
        }
        public static void Apply()
        {
            Load();AudioListener.volume=values.masterVolume;
            if(GameSession.Instance&&GameSession.Instance.player)GameSession.Instance.player.lookSensitivity=values.mouseSensitivity;
            ApplyDisplayMode();
        }
        static void ApplyDisplayMode()
        {
            var mode=values.fullscreen?FullScreenMode.FullScreenWindow:FullScreenMode.Windowed;
            if(!displayInitialized||requestedFullscreen!=values.fullscreen)
            {
                RequestDisplayMode(mode);return;
            }
            if(displayRequestPending)
            {
                if(Screen.fullScreenMode==mode&&Screen.width==requestedWidth&&Screen.height==requestedHeight)
                {displayRequestPending=false;RememberWindowSize();}
                return;
            }
            if(Screen.fullScreenMode!=mode){RequestDisplayMode(mode);return;}
            if(values.fullscreen)
            {
                var native=Screen.currentResolution;
                if(native.width>0&&native.height>0&&(Screen.width!=native.width||Screen.height!=native.height))RequestDisplayMode(mode);
            }
            else RememberWindowSize();
        }
        static void RequestDisplayMode(FullScreenMode mode)
        {
            int width=values.windowWidth,height=values.windowHeight;
            if(values.fullscreen)
            {
                var native=Screen.currentResolution;
                width=native.width>0?native.width:Screen.width;height=native.height>0?native.height:Screen.height;
            }
            // Record the request before calling Unity: IMGUI invokes Apply multiple
            // times per frame, while the compositor applies this change asynchronously.
            displayInitialized=true;requestedFullscreen=values.fullscreen;requestedWidth=width;requestedHeight=height;
            displayRequestPending=Screen.fullScreenMode!=mode||Screen.width!=width||Screen.height!=height;
            if(displayRequestPending)
            {
                Screen.SetResolution(width,height,mode);
                Debug.Log("CHAPTER_DISPLAY_REQUEST "+width+"x"+height+" mode="+mode);
            }
        }
        public static void Save()
        {
            Load();Apply();
            try
            {
                if(string.IsNullOrEmpty(ChapterSave.StorageDirectory))throw new IOException("Settings location unavailable.");
                Directory.CreateDirectory(ChapterSave.StorageDirectory);
                ChapterSave.WriteAtomic(Path.Combine(ChapterSave.StorageDirectory,"settings.json"),JsonUtility.ToJson(values,true),null);
                Status="Settings saved.";
            }
            catch(Exception error) when(error is IOException||error is UnauthorizedAccessException||error is ArgumentException||error is NotSupportedException)
            {Status="Settings apply for this session but could not be saved.";Debug.LogWarning("CHAPTER_SETTINGS_SAVE_FAILED "+error.Message);}
        }
    }
}
