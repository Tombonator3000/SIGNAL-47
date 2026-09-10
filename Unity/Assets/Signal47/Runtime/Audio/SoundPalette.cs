using UnityEngine;
using Signal47.Core;
namespace Signal47.Audio
{
    public sealed class SoundPalette:MonoBehaviour
    {
        public AudioClip printer,phone,smash,wind,click,switchClick,titleMusic;
        AudioSource ui,windSource,music,foley;bool titleStarted;
        void Awake()
        {
            ui=Source(gameObject,0,1);music=Source(gameObject,0,0);
            var air=new GameObject("ExteriorWind");air.transform.SetParent(transform);air.transform.position=new Vector3(0,1.5f,-9);windSource=Source(air,1,.16f);windSource.minDistance=3;windSource.maxDistance=28;windSource.clip=wind;windSource.loop=true;
            var point=new GameObject("CeramicFoley");point.transform.SetParent(transform);foley=Source(point,1,.70f);foley.minDistance=1;foley.maxDistance=16;
        }
        static AudioSource Source(GameObject go,float spatial,float volume){var s=go.AddComponent<AudioSource>();s.playOnAwake=false;s.spatialBlend=spatial;s.volume=volume;return s;}
        void Start(){if(wind)windSource.Play();}
        public void Click(bool physical=false){var clip=physical?switchClick:click;if(clip)ui.PlayOneShot(clip,physical?.12f:.22f);}
        public void BreakAt(Vector3 position){foley.transform.position=position;foley.PlayOneShot(smash);}
        void Update()
        {
            var g=GameSession.Instance;if(!g)return;
            if(g.hud.TitleVisible&&!titleStarted){titleStarted=true;music.clip=titleMusic;music.Play();}
            music.volume=Mathf.MoveTowards(music.volume,g.hud.TitleVisible?.24f:0,Time.unscaledDeltaTime*.12f);
            windSource.volume=Mathf.MoveTowards(windSource.volume,g.hud.TitleVisible?0:.16f,Time.unscaledDeltaTime*.15f);
        }
    }
}
