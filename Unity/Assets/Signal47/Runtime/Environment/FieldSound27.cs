using UnityEngine;
using Signal47.Core;
namespace Signal47.Environment
{
    public sealed class FieldSound27 : MonoBehaviour
    {
        public AudioSource wind,generator;
        void OnEnable(){if(wind)wind.Play();if(generator)generator.Play();}
        void OnDisable(){if(wind)wind.Stop();if(generator)generator.Stop();}
        void Update()
        {
            var g=GameSession.Instance;bool audible=g&&g.hud&&g.hud.Started&&!g.hud.Paused&&!g.hud.TitleVisible;
            if(wind)wind.volume=Mathf.MoveTowards(wind.volume,audible?.10f:0,Time.unscaledDeltaTime*.2f);
            if(generator)generator.volume=Mathf.MoveTowards(generator.volume,audible?.16f:0,Time.unscaledDeltaTime*.3f);
        }
    }
}
