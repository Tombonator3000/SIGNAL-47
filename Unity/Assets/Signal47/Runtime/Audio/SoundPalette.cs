using UnityEngine;
using Signal47.Core;
namespace Signal47.Audio
{
    public sealed class SoundPalette:MonoBehaviour
    {
        // Source-gain first pass, not a claim of a measured final listener mix.
        // Ring, future-call, printer and event timing/gain remain owned by the existing director.
        public const float UiGain=.22f, WindGain=.16f, CeramicGain=.70f, TitleGain=.20f;
        public AudioClip printer,phone,smash,wind,click,switchClick,titleMusic;
        AudioSource ui,windSource,music,foley;bool titleStarted;
        void Awake()
        {
            ui=Source(gameObject,0,UiGain);music=Source(gameObject,0,0);
            var air=new GameObject("ExteriorWind");air.transform.SetParent(transform);air.transform.position=new Vector3(0,1.5f,-9);windSource=Source(air,1,WindGain);windSource.minDistance=3;windSource.maxDistance=28;windSource.clip=wind;windSource.loop=true;
            var point=new GameObject("CeramicFoley");point.transform.SetParent(transform);foley=Source(point,1,CeramicGain);foley.minDistance=1;foley.maxDistance=16;
        }
        static AudioSource Source(GameObject go,float spatial,float volume){var s=go.AddComponent<AudioSource>();s.playOnAwake=false;s.spatialBlend=spatial;s.volume=volume;return s;}
        void Start(){if(wind)windSource.Play();}
        public void Click(bool physical=false){var clip=physical?switchClick:click;if(clip)ui.PlayOneShot(clip);}
        public void BreakAt(Vector3 position){foley.transform.position=position;foley.PlayOneShot(smash);}
        void Update()
        {
            var g=GameSession.Instance;if(!g)return;
            if(g.hud.TitleVisible&&!titleStarted){titleStarted=true;music.clip=titleMusic;music.Play();}
            music.volume=Mathf.MoveTowards(music.volume,titleStarted?TitleGain:0,Time.unscaledDeltaTime*.12f);
            windSource.volume=Mathf.MoveTowards(windSource.volume,titleStarted?0:WindGain,Time.unscaledDeltaTime*.15f);
        }
    }
}
