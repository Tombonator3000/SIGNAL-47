using System.Collections;
using UnityEngine;
using UnityEngine.InputSystem;
using Signal47.Investigation;
using Signal47.Chapter;
namespace Signal47.UI
{
    public sealed partial class HUDController : MonoBehaviour
    {
        public Notebook notebook;public Font terminalFont;
        public bool Started { get; private set; }
        public bool Paused { get; private set; }
        public bool TitleVisible { get; private set; }
        public bool SettingsOpen => settingsOpen;
        public bool NewShiftConfirmation { get; private set; }
        public bool ModalOpen => PreviousShiftsOpen || NewShiftConfirmation || settingsOpen || photoOpen || paperOpen || notebookOpen || Paused || TitleVisible || (Core.GameSession.Instance && Core.GameSession.Instance.chapter && Core.GameSession.Instance.chapter.ModalOpen);
        public string InteractionPrompt { get; set; } = "";
        string toast=""; bool paperOpen,notebookOpen,photoOpen; string paper=""; float toastUntil;
        GUIStyle mono, big, button, paperStyle;
        Vector2 noteScroll,paperScroll;bool returnToNotebook,settingsOpen;
        bool confirmationFocusPending,confirmationAudioPaused;float confirmationTimeScale;string confirmationMessage="";
        void Update()
        {
            if(ChapterSave.QuitPending || ChapterSave.IsRestoring || Core.GameSession.Instance.Transitioning)return;
            if(PreviousShiftsOpen)
            {
                if(Keyboard.current!=null&&Keyboard.current.escapeKey.wasPressedThisFrame)BackFromPreviousShifts();
                return;
            }
            if(NewShiftConfirmation)
            {
                if(Keyboard.current!=null && Keyboard.current.escapeKey.wasPressedThisFrame)CancelNewShift();
                return;
            }
            if(settingsOpen)
            {
                if(Keyboard.current!=null && Keyboard.current.escapeKey.wasPressedThisFrame){PlayerSettings.Save();settingsOpen=false;}
                return;
            }
            if (!Started || TitleVisible) return;
            var chapter=Core.GameSession.Instance.chapter;
            if(chapter && chapter.ModalOpen)
            {
                if(Keyboard.current!=null && (Keyboard.current.escapeKey.wasPressedThisFrame || Keyboard.current.tabKey.wasPressedThisFrame))
                {chapter.ClosePanel();notebookOpen=false;SetCursor(true);}
                return;
            }
            if (Signals.SignalConsole.AnyOpen) { if (Keyboard.current != null && Keyboard.current.escapeKey.wasPressedThisFrame) FindFirstObjectByType<Signals.SignalConsole>().Close(); return; }
            if (Keyboard.current != null && Keyboard.current.tabKey.wasPressedThisFrame && !Paused && !paperOpen && !photoOpen)
            { notebookOpen=!notebookOpen; SetCursor(!notebookOpen); }
            if (Keyboard.current != null && Keyboard.current.escapeKey.wasPressedThisFrame)
            {
                if (settingsOpen) { settingsOpen=false; PlayerSettings.Save(); }
                else if (photoOpen) { ClosePhoto(); }
                else if (paperOpen) { ClosePaper(); }
                else if (notebookOpen) { notebookOpen=false; SetCursor(true); }
                else { SetPaused(!Paused); }
            }
        }
        public void SetPaused(bool value){Paused=value;Time.timeScale=value?0:1;AudioListener.pause=value;SetCursor(!value);}
        public void StartShift(){Started=true;SetPaused(false);SetCursor(true);Toast("SHIFT LOG // 23:41 LOCAL",2f);}
        public void RequestNewShift()
        {
            if(NewShiftConfirmation||ChapterSave.Saving||ChapterSave.IsRestoring||ChapterSave.QuitPending)return;
            if(!Started&&!ChapterSave.HasSave){BeginNewShift();return;}
            confirmationTimeScale=Time.timeScale;confirmationAudioPaused=AudioListener.pause;
            NewShiftConfirmation=true;confirmationFocusPending=true;confirmationMessage="";
            Time.timeScale=0;AudioListener.pause=true;SetCursor(false);
        }
        public void CancelNewShift()
        {
            if(!NewShiftConfirmation)return;
            NewShiftConfirmation=false;confirmationFocusPending=false;
            Time.timeScale=confirmationTimeScale;AudioListener.pause=confirmationAudioPaused;
            SetCursor(Started&&!Paused&&!TitleVisible);
        }
        public bool ConfirmNewShift()=>NewShiftConfirmation&&BeginNewShift();
        bool BeginNewShift()
        {
            if(!ChapterSave.StartNew()){confirmationMessage=ChapterSave.Status;return false;}
            NewShiftConfirmation=false;confirmationFocusPending=false;
            if(Started)Core.GameSession.Instance.Restart();else StartShift();
            return true;
        }
        public void ResumeFromSave(){Started=true;TitleVisible=false;settingsOpen=false;CloseModal();Toast("NIGHT SHIFT // CHECKPOINT RESTORED",3);}
        public void Toast(string msg,float seconds=1.8f){toast=msg;toastUntil=Time.unscaledTime+seconds;}
        public void ShowPaper(string content){returnToNotebook=notebookOpen;notebookOpen=false;paper=content;paperScroll=Vector2.zero;paperOpen=true;if(Core.GameSession.Instance.director.palette)Core.GameSession.Instance.director.palette.Click();SetCursor(false);}
        void ClosePaper(){paperOpen=false;notebookOpen=returnToNotebook;returnToNotebook=false;SetCursor(!notebookOpen);}
        public void ContinueToServiceYard(){var g=Core.GameSession.Instance;if(!TitleVisible || !g.yard)return;g.yard.Begin();if(!g.yard.Active)return;TitleVisible=false;CloseModal();}
        void ClosePhoto(){photoOpen=false;notebookOpen=true;SetCursor(false);}
        void ShowEvidence(EvidenceRecord item)
        {
            var chapter=Core.GameSession.Instance.chapter;
            if(chapter && chapter.OpenEvidence(item.Id)){notebookOpen=false;photoOpen=false;SetCursor(false);}
            else ShowPaper(item.Body);
        }
        public bool PhotoOpen=>photoOpen || (Core.GameSession.Instance && Core.GameSession.Instance.chapter && Core.GameSession.Instance.chapter.ModalOpen);
        public void ShowNotebook(){notebookOpen=true;SetCursor(false);}
        public void ShowTitle(){paperOpen=false;notebookOpen=false;Paused=false;Time.timeScale=1;AudioListener.pause=false;TitleVisible=true;Signals.SignalConsole console=FindFirstObjectByType<Signals.SignalConsole>();if(console)console.Close();SetCursor(false);}
        public void CloseModal(){photoOpen=false;paperOpen=false;notebookOpen=false;returnToNotebook=false;settingsOpen=false;if(Core.GameSession.Instance.chapter)Core.GameSession.Instance.chapter.ClosePanel();SetPaused(false);SetCursor(true);}
        public void SetCursor(bool lockIt){Cursor.lockState=lockIt?CursorLockMode.Locked:CursorLockMode.None;Cursor.visible=!lockIt;}
        void Styles()
        {
            if(mono!=null)return;
            mono=new GUIStyle(GUI.skin.label){font=terminalFont,fontSize=20,normal={textColor=new Color(.72f,.87f,.73f)},wordWrap=true};
            big=new GUIStyle(mono){font=terminalFont,fontSize=42,alignment=TextAnchor.MiddleCenter,fontStyle=FontStyle.Bold};
            button=new GUIStyle(GUI.skin.button){font=terminalFont,fontSize=22};
            paperStyle=new GUIStyle(GUI.skin.box){fontSize=17,alignment=TextAnchor.UpperLeft,wordWrap=true,normal={textColor=new Color(.15f,.13f,.10f)}};
        }
        void OnGUI()
        {
            var field=Core.GameSession.Instance.fieldCamera;
            if(field && (field.Capturing || field.Raised))return;
            Styles();
            if(ChapterSave.IsRestoring || ChapterSave.QuitPending || Core.GameSession.Instance.Transitioning)
            {
                GUI.color=new Color(.018f,.026f,.022f,.98f);GUI.DrawTexture(new Rect(0,0,Screen.width,Screen.height),Texture2D.whiteTexture);GUI.color=Color.white;
                GUI.Label(new Rect(0,Screen.height*.5f-25,Screen.width,60),ChapterSave.QuitPending?"SAVING BEFORE EXIT…":ChapterSave.IsRestoring?"RESTORING THE NIGHT SHIFT…":"PREPARING THE NIGHT SHIFT…",big);return;
            }
            if(PreviousShiftsOpen){DrawPreviousShifts();return;}
            if(settingsOpen){DrawSettings();return;}
            if(NewShiftConfirmation){DrawNewShiftConfirmation();return;}
            if(!Started)
            {
                GUI.color=new Color(.025f,.04f,.035f,.97f);GUI.DrawTexture(new Rect(Screen.width*.5f-290,Screen.height*.5f-190,580,515),Texture2D.whiteTexture);GUI.color=Color.white;
                GUI.Label(new Rect(Screen.width*.5f-230,Screen.height*.5f-125,460,70),"SIGNAL / 47",big);
                GUI.Label(new Rect(Screen.width*.5f-250,Screen.height*.5f-35,500,60),"THE SECOND EXPOSURE // CHAPTER ONE\n23:41 // NEW MEXICO, 1986",new GUIStyle(mono){alignment=TextAnchor.MiddleCenter});
                bool hasSave=ChapterSave.HasSave;
                GUI.enabled=!ChapterSave.Saving;
                if(GUI.Button(new Rect(Screen.width*.5f-150,Screen.height*.5f+55,300,52),hasSave?"CONTINUE CHECKPOINT":"START NIGHT SHIFT",button))
                {if(hasSave)ChapterSave.TryContinue();else RequestNewShift();}
                GUI.enabled=hasSave&&!ChapterSave.Saving;
                if(GUI.Button(new Rect(Screen.width*.5f-150,Screen.height*.5f+117,300,42),hasSave?"NEW NIGHT SHIFT":"NO SAVED CHECKPOINT",button))RequestNewShift();
                GUI.enabled=true;
                if(GUI.Button(new Rect(Screen.width*.5f-150,Screen.height*.5f+169,145,38),"SETTINGS",button))settingsOpen=true;
                if(GUI.Button(new Rect(Screen.width*.5f+5,Screen.height*.5f+169,145,38),"QUIT",button))Application.Quit();
                if(GUI.Button(new Rect(Screen.width*.5f-150,Screen.height*.5f+214,300,38),"PREVIOUS SHIFTS",button))OpenPreviousShifts();
                GUI.Label(new Rect(Screen.width*.5f-270,Screen.height*.5f+265,540,50),ChapterSave.Status,new GUIStyle(mono){fontSize=16,alignment=TextAnchor.MiddleCenter});
                if(settingsOpen)DrawSettings();
                return;
            }
            if(!string.IsNullOrEmpty(InteractionPrompt) && !ModalOpen)
                GUI.Label(new Rect(Screen.width*.5f-210,Screen.height-95,420,40),InteractionPrompt,new GUIStyle(mono){alignment=TextAnchor.MiddleCenter});
            if(!photoOpen && Time.unscaledTime<toastUntil) GUI.Label(new Rect(24,24,600,35),toast,mono);
            if(Core.GameSession.Instance.yard && Core.GameSession.Instance.yard.Active && !ModalOpen)
                GUI.Label(new Rect(24,65,Screen.width-48,55),Core.GameSession.Instance.chapter?Core.GameSession.Instance.chapter.Objective:Core.GameSession.Instance.yard.Objective,mono);
            if(paperOpen)
            {
                float height=Mathf.Min(540,Screen.height-40);var r=new Rect(Screen.width*.5f-290,(Screen.height-height)/2,580,height);
                GUI.color=new Color(.88f,.84f,.70f);GUI.DrawTexture(r,Texture2D.whiteTexture);GUI.color=Color.white;
                var ink=new GUIStyle(mono){fontSize=17,normal={textColor=new Color(.15f,.13f,.10f)}};
                float textHeight=ink.CalcHeight(new GUIContent(paper),r.width-70);
                paperScroll=GUI.BeginScrollView(new Rect(r.x+24,r.y+24,r.width-48,r.height-100),paperScroll,new Rect(0,0,r.width-70,textHeight));
                GUI.Label(new Rect(0,0,r.width-70,textHeight),paper,ink);GUI.EndScrollView();
                if(GUI.Button(new Rect(r.x+190,r.yMax-58,200,42),returnToNotebook?"BACK TO NOTEBOOK":"CLOSE",button))ClosePaper();
            }
            if(notebookOpen)
            {
                var r=new Rect(Screen.width*.5f-330,40,660,Screen.height-80);GUI.color=new Color(.025f,.045f,.035f,.97f);GUI.DrawTexture(r,Texture2D.whiteTexture);GUI.color=Color.white;
                GUI.Label(new Rect(r.x+24,r.y+18,r.width-48,35),"FIELD NOTEBOOK // NIGHT SHIFT",mono);
                if(GUI.Button(new Rect(r.xMax-110,r.y+14,90,32),"CLOSE")){notebookOpen=false;SetCursor(true);}
                float width=r.width-70,contentHeight=90+notebook.Evidence.Count*48;
                foreach(var entry in notebook.Entries)contentHeight+=mono.CalcHeight(new GUIContent(entry),width)+16;
                noteScroll=GUI.BeginScrollView(new Rect(r.x+24,r.y+65,r.width-48,r.height-90),noteScroll,new Rect(0,0,width,contentHeight));
                float y=0;GUI.Label(new Rect(0,y,width,28),"DOCUMENTS // SELECT TO READ",mono);y+=38;
                foreach(var item in notebook.Evidence){if(GUI.Button(new Rect(0,y,width,38),item.Title,button))ShowEvidence(item);y+=48;}
                if(notebook.Evidence.Count==0){GUI.Label(new Rect(0,y,width,28),"No documents collected.",mono);y+=32;}
                foreach(var entry in notebook.Entries){float h=mono.CalcHeight(new GUIContent(entry),width);GUI.Label(new Rect(0,y,width,h),entry,mono);y+=h+16;}
                GUI.EndScrollView();
            }
            if(photoOpen && field && field.DrawPhotograph(button,mono))ClosePhoto();
            if(Paused)
            {
                GUI.color=new Color(.025f,.04f,.035f,.98f);GUI.DrawTexture(new Rect(Screen.width*.5f-235,Screen.height*.5f-130,470,ChapterSave.CanQuitWithoutSaving?460:400),Texture2D.whiteTexture);GUI.color=Color.white;
                GUI.Label(new Rect(Screen.width*.5f-170,Screen.height*.5f-88,340,40),"PAUSED",new GUIStyle(big){fontSize=28});
                if(GUI.Button(new Rect(Screen.width*.5f-120,Screen.height*.5f-15,240,45),"RESUME",button)){SetPaused(false);}
                if(GUI.Button(new Rect(Screen.width*.5f-120,Screen.height*.5f+42,240,45),"NEW NIGHT SHIFT",button))RequestNewShift();
                GUI.enabled=ChapterSave.CanSave;
                if(GUI.Button(new Rect(Screen.width*.5f-120,Screen.height*.5f+99,240,40),"SAVE CHECKPOINT",button))ChapterSave.RequestCheckpoint();
                GUI.enabled=true;
                if(GUI.Button(new Rect(Screen.width*.5f-120,Screen.height*.5f+149,115,38),"SETTINGS",button))settingsOpen=true;
                if(GUI.Button(new Rect(Screen.width*.5f+5,Screen.height*.5f+149,115,38),"QUIT",button))Application.Quit();
                GUI.Label(new Rect(Screen.width*.5f-215,Screen.height*.5f+199,430,75),ChapterSave.CanSave || ChapterSave.CanQuitWithoutSaving?ChapterSave.Status:ChapterSave.SaveAvailability,new GUIStyle(mono){fontSize=17,alignment=TextAnchor.MiddleCenter});
                if(ChapterSave.CanQuitWithoutSaving && GUI.Button(new Rect(Screen.width*.5f-195,Screen.height*.5f+281,390,38),"QUIT WITHOUT SAVING CHANGES",button))ChapterSave.QuitWithoutSaving();
            }
            if(TitleVisible)
            {
                GUI.color=new Color(0,0,0,.96f);GUI.DrawTexture(new Rect(0,0,Screen.width,Screen.height),Texture2D.whiteTexture);GUI.color=Color.white;
                GUI.Label(new Rect(0,Screen.height*.5f-100,Screen.width,90),"SIGNAL / 47",new GUIStyle(big){fontSize=64,normal={textColor=Color.white}});
                GUI.Label(new Rect(0,Screen.height*.5f,Screen.width,35),"THE NIGHT IS NOT OVER // CHECK THE LOCAL CONTROLLER",new GUIStyle(mono){alignment=TextAnchor.MiddleCenter,fontSize=20,normal={textColor=new Color(.75f,.78f,.75f)}});
                GUI.Label(new Rect(30,Screen.height-92,Screen.width-60,70),"Music: ‘Signal to Noise’ — Scott Buckley • CC BY 4.0\nNo-piano mix, excerpt with fades • scottbuckley.com.au\ncreativecommons.org/licenses/by/4.0/",new GUIStyle(mono){fontSize=13,alignment=TextAnchor.MiddleCenter});
                if(Core.GameSession.Instance.yard && GUI.Button(new Rect(Screen.width*.5f-145,Screen.height*.5f+45,290,42),"CONTINUE / SERVICE YARD",button))ContinueToServiceYard();
                if(GUI.Button(new Rect(Screen.width*.5f-120,Screen.height*.5f+95,240,48),"NEW NIGHT SHIFT",button))RequestNewShift();
            }
            if(Core.GameSession.Instance.chapter && Core.GameSession.Instance.chapter.ModalOpen)Core.GameSession.Instance.chapter.DrawPanel();
            if(settingsOpen)DrawSettings();
        }
        void DrawNewShiftConfirmation()
        {
            float width=Mathf.Min(580,Screen.width-40),x=(Screen.width-width)/2,y=Screen.height*.5f-185;
            GUI.color=new Color(.018f,.026f,.022f,.98f);GUI.DrawTexture(new Rect(0,0,Screen.width,Screen.height),Texture2D.whiteTexture);
            GUI.color=new Color(.055f,.085f,.065f,1);GUI.DrawTexture(new Rect(x,y,width,380),Texture2D.whiteTexture);GUI.color=Color.white;
            GUI.Label(new Rect(x+24,y+20,width-48,45),"START A NEW NIGHT SHIFT?",new GUIStyle(big){fontSize=28});
            GUI.Label(new Rect(x+28,y+84,width-56,115),"Begin again at 23:41. Progress since the last checkpoint will be lost.\n\nPrevious checkpoints are kept in a local recovery folder. Your original photographs are preserved.",mono);
            GUI.SetNextControlName("keep-current-shift");
            if(GUI.Button(new Rect(x+28,y+220,(width-68)/2,48),"KEEP CURRENT SHIFT",button))CancelNewShift();
            if(GUI.Button(new Rect(x+40+(width-68)/2,y+220,(width-68)/2,48),"START NEW SHIFT",button))ConfirmNewShift();
            GUI.Label(new Rect(x+28,y+286,width-56,72),confirmationMessage+"\nESC / Keep current shift",new GUIStyle(mono){fontSize=16});
            if(confirmationFocusPending&&Event.current.type==EventType.Repaint)
            {GUI.FocusControl("keep-current-shift");confirmationFocusPending=false;}
        }
        void DrawSettings()
        {
            float x=Screen.width*.5f-250,y=Screen.height*.5f-170;
            GUI.color=new Color(.055f,.085f,.065f,1);GUI.DrawTexture(new Rect(x,y,500,390),Texture2D.whiteTexture);GUI.color=Color.white;
            GUI.Label(new Rect(x+25,y+20,450,40),"NIGHT SHIFT / SETTINGS",big);
            GUI.Label(new Rect(x+30,y+83,440,30),$"MASTER VOLUME // {PlayerSettings.MasterVolume*100:0}%",mono);
            PlayerSettings.MasterVolume=GUI.HorizontalSlider(new Rect(x+30,y+125,440,25),PlayerSettings.MasterVolume,0,1);
            GUI.Label(new Rect(x+30,y+161,440,30),$"MOUSE SENSITIVITY // {PlayerSettings.MouseSensitivity:0.000}",mono);
            PlayerSettings.MouseSensitivity=GUI.HorizontalSlider(new Rect(x+30,y+205,440,25),PlayerSettings.MouseSensitivity,.02f,.25f);
            PlayerSettings.Fullscreen=GUI.Toggle(new Rect(x+30,y+245,440,32),PlayerSettings.Fullscreen," FULLSCREEN / NATIVE DISPLAY",new GUIStyle(GUI.skin.toggle){font=terminalFont,fontSize=22});
            PlayerSettings.Apply();
            if(GUI.Button(new Rect(x+30,y+311,195,45),"RESTORE DEFAULTS",button)){PlayerSettings.MasterVolume=1;PlayerSettings.MouseSensitivity=.085f;PlayerSettings.Fullscreen=false;PlayerSettings.Apply();}
            if(GUI.Button(new Rect(x+245,y+311,225,45),"SAVE AND CLOSE",button)){PlayerSettings.Save();settingsOpen=false;}
        }
    }
}
