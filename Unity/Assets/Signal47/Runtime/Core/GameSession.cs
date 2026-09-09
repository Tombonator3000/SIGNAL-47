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
        public bool CanControl => hud != null && hud.Started && !hud.ModalOpen && !Signal47.Signals.SignalConsole.AnyOpen;
        void Awake(){Instance=this;}
        public void Restart(){Time.timeScale=1;AudioListener.pause=false;SceneManager.LoadScene(SceneManager.GetActiveScene().buildIndex);}
    }
}
