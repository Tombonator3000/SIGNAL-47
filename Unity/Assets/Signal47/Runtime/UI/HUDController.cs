using System.Collections;
using UnityEngine;
using UnityEngine.InputSystem;
using Signal47.Investigation;
namespace Signal47.UI
{
    public sealed class HUDController : MonoBehaviour
    {
        public Notebook notebook;
        public bool Started { get; private set; }
        public bool Paused { get; private set; }
        public bool TitleVisible { get; private set; }
        public bool ModalOpen => paperOpen || notebookOpen || Paused || TitleVisible;
        public string InteractionPrompt { get; set; } = "";
        string toast=""; bool paperOpen,notebookOpen; string paper=""; float toastUntil;
        GUIStyle mono, big, button, paperStyle;
        Vector2 noteScroll,paperScroll;bool returnToNotebook;
        void Update()
        {
            if (!Started || TitleVisible) return;
            if (Signals.SignalConsole.AnyOpen) { if (Keyboard.current != null && Keyboard.current.escapeKey.wasPressedThisFrame) FindFirstObjectByType<Signals.SignalConsole>().Close(); return; }
            if (Keyboard.current != null && Keyboard.current.tabKey.wasPressedThisFrame && !Paused && !paperOpen)
            { notebookOpen=!notebookOpen; SetCursor(!notebookOpen); }
            if (Keyboard.current != null && Keyboard.current.escapeKey.wasPressedThisFrame)
            {
                if (paperOpen) { ClosePaper(); }
                else if (notebookOpen) { notebookOpen=false; SetCursor(true); }
                else { SetPaused(!Paused); }
            }
        }
        public void SetPaused(bool value){Paused=value;Time.timeScale=value?0:1;AudioListener.pause=value;SetCursor(!value);}
        public void StartShift(){Started=true;SetPaused(false);SetCursor(true);Toast("SHIFT LOG // 23:41 LOCAL",2f);}
        public void Toast(string msg,float seconds=1.8f){toast=msg;toastUntil=Time.unscaledTime+seconds;}
        public void ShowPaper(string content){returnToNotebook=notebookOpen;notebookOpen=false;paper=content;paperScroll=Vector2.zero;paperOpen=true;SetCursor(false);}
        void ClosePaper(){paperOpen=false;notebookOpen=returnToNotebook;returnToNotebook=false;SetCursor(!notebookOpen);}
        public void ShowNotebook(){notebookOpen=true;SetCursor(false);}
        public void ShowTitle(){paperOpen=false;notebookOpen=false;Paused=false;Time.timeScale=1;AudioListener.pause=false;TitleVisible=true;Signals.SignalConsole console=FindFirstObjectByType<Signals.SignalConsole>();if(console)console.Close();SetCursor(false);}
        public void CloseModal(){paperOpen=false;notebookOpen=false;returnToNotebook=false;SetPaused(false);SetCursor(true);}
        public void SetCursor(bool lockIt){Cursor.lockState=lockIt?CursorLockMode.Locked:CursorLockMode.None;Cursor.visible=!lockIt;}
        void Styles()
        {
            if(mono!=null)return;
            mono=new GUIStyle(GUI.skin.label){fontSize=16,normal={textColor=new Color(.62f,1f,.70f)},wordWrap=true};
            big=new GUIStyle(mono){fontSize=42,alignment=TextAnchor.MiddleCenter,fontStyle=FontStyle.Bold};
            button=new GUIStyle(GUI.skin.button){fontSize=18};
            paperStyle=new GUIStyle(GUI.skin.box){fontSize=17,alignment=TextAnchor.UpperLeft,wordWrap=true,normal={textColor=new Color(.15f,.13f,.10f)}};
        }
        void OnGUI()
        {
            Styles();
            if(!Started)
            {
                GUI.Box(new Rect(Screen.width*.5f-250,Screen.height*.5f-160,500,320),"");
                GUI.Label(new Rect(Screen.width*.5f-230,Screen.height*.5f-125,460,70),"SIERRA ARRAY\nRADIO OBSERVATORY",new GUIStyle(big){fontSize=27});
                GUI.Label(new Rect(Screen.width*.5f-180,Screen.height*.5f-35,360,50),"NIGHT SHIFT // 23:41 // NEW MEXICO, 1986",mono);
                if(GUI.Button(new Rect(Screen.width*.5f-130,Screen.height*.5f+55,260,52),"START NIGHT SHIFT",button)) StartShift();
                return;
            }
            if(!string.IsNullOrEmpty(InteractionPrompt) && !ModalOpen)
                GUI.Label(new Rect(Screen.width*.5f-210,Screen.height-95,420,40),InteractionPrompt,new GUIStyle(mono){alignment=TextAnchor.MiddleCenter});
            if(Time.unscaledTime<toastUntil) GUI.Label(new Rect(24,24,600,35),toast,mono);
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
                foreach(var item in notebook.Evidence){if(GUI.Button(new Rect(0,y,width,38),item.Title,button))ShowPaper(item.Body);y+=48;}
                if(notebook.Evidence.Count==0){GUI.Label(new Rect(0,y,width,28),"No documents collected.",mono);y+=32;}
                foreach(var entry in notebook.Entries){float h=mono.CalcHeight(new GUIContent(entry),width);GUI.Label(new Rect(0,y,width,h),entry,mono);y+=h+16;}
                GUI.EndScrollView();
            }
            if(Paused)
            {
                GUI.Box(new Rect(Screen.width*.5f-190,Screen.height*.5f-115,380,230),"");
                GUI.Label(new Rect(Screen.width*.5f-170,Screen.height*.5f-88,340,40),"PAUSED",new GUIStyle(big){fontSize=28});
                if(GUI.Button(new Rect(Screen.width*.5f-120,Screen.height*.5f-15,240,45),"RESUME",button)){SetPaused(false);}
                if(GUI.Button(new Rect(Screen.width*.5f-120,Screen.height*.5f+42,240,45),"RESTART SLICE",button)) Core.GameSession.Instance.Restart();
            }
            if(TitleVisible)
            {
                GUI.color=new Color(0,0,0,.96f);GUI.DrawTexture(new Rect(0,0,Screen.width,Screen.height),Texture2D.whiteTexture);GUI.color=Color.white;
                GUI.Label(new Rect(0,Screen.height*.5f-100,Screen.width,90),"SIGNAL / 47",new GUIStyle(big){fontSize=64,normal={textColor=Color.white}});
                GUI.Label(new Rect(0,Screen.height*.5f,Screen.width,45),"NEW MEXICO — 1986",new GUIStyle(mono){alignment=TextAnchor.MiddleCenter,fontSize=20,normal={textColor=new Color(.75f,.78f,.75f)}});
                if(GUI.Button(new Rect(Screen.width*.5f-120,Screen.height*.5f+95,240,48),"RESTART SLICE",button)) Core.GameSession.Instance.Restart();
            }
        }
    }
}
