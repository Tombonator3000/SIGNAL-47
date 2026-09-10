using UnityEngine;
using UnityEngine.SceneManagement;
using Signal47.Player;
using Signal47.UI;
using Signal47.Investigation;
using Signal47.Events;
namespace Signal47.Core
{
    public sealed class GameSession : MonoBehaviour
    {
        public static GameSession Instance { get; private set; }
        public FirstPersonController player;
        public HUDController hud;
        public Notebook notebook;
        public PrologueDirector director;
        bool restarting;
        public bool CanControl => !restarting && hud != null && hud.Started && !hud.ModalOpen && !Signal47.Signals.SignalConsole.AnyOpen;
        void Awake(){Instance=this;
#if UNITY_STANDALONE_LINUX
            // Keep the working GLX path without vsync. A 60 Hz software cap on the
            // 75 Hz reference desktop spent ~15 ms waiting and produced 19.45 ms p95.
            // Follow the display rate, with a bounded budget on high-refresh monitors.
            QualitySettings.vSyncCount=0;
            Application.targetFrameRate=Mathf.Clamp(Mathf.RoundToInt((float)Screen.currentResolution.refreshRateRatio.value),60,120);
#endif
        }
        public void Restart()
        {
            if(restarting)return;
            restarting=true;Time.timeScale=1;AudioListener.pause=false;
            SceneManager.LoadSceneAsync(SceneManager.GetActiveScene().buildIndex);
        }
    }
}
