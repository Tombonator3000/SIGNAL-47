using UnityEngine;
using UnityEngine.InputSystem;

namespace Signal47.Archive
{
    // Standalone production study. No GameSession, save system or story progression.
    public sealed class ArchiveStudy : MonoBehaviour
    {
        public Camera view;
        public Collider folio;
        public Font font;
        public Material labelMaterial;
        void OnEnable(){Font.textureRebuilt+=RefreshFontAtlas;}
        void OnDisable(){Font.textureRebuilt-=RefreshFontAtlas;}
        void RefreshFontAtlas(Font changed){if(changed==font&&labelMaterial)labelMaterial.mainTexture=font.material.mainTexture;}
        public TextAsset original,amended;
        public Transform deskView,roomView;
        public bool DocumentOpen { get; private set; }
        public bool Complete { get; private set; }
        public int Page { get; private set; }
        public bool OriginalRead { get; private set; }
        public bool AmendedRead { get; private set; }
        public string Feedback { get; private set; }="";
        bool overview,exitOpen;GUIStyle text,title,small,button,paper,heading;
        public void SetOverview(bool value)
        {
            overview=value;var pose=overview?roomView:deskView;
            view.transform.SetPositionAndRotation(pose.position,pose.rotation);
        }
        void Start(){RefreshFontAtlas(font);Cursor.lockState=CursorLockMode.None;Cursor.visible=true;SetOverview(false);Application.targetFrameRate=60;}
        public bool OpenFromRay(Ray ray)
        {
            if(exitOpen||DocumentOpen||!Physics.Raycast(ray,out var hit,4)||hit.collider!=folio)return false;
            DocumentOpen=true;Page=0;OriginalRead=true;Feedback="";return true;
        }
        public bool SelectPage(int value)
        {
            if(!DocumentOpen||value<0||value>2||value==2&&!(OriginalRead&&AmendedRead))return false;
            Page=value;if(value==0)OriginalRead=true;if(value==1)AmendedRead=true;Feedback="";return true;
        }
        public bool SubmitFinding(int value)
        {
            if(!DocumentOpen||Page!=2||!OriginalRead||!AmendedRead||value<0||value>2)return false;
            if(value!=1){Feedback="That goes beyond the evidence. Compare the references described in both versions.";return false;}
            Complete=true;Feedback="SUPPORTED / The original records A, B and C. The amended copy omits the closing sight line and attributes the mark to development. This establishes a changed report, not a motive.";return true;
        }
        public void CloseDocument(){DocumentOpen=false;}
        public void ResetStudy(){DocumentOpen=false;Complete=false;OriginalRead=false;AmendedRead=false;Page=0;Feedback="";exitOpen=false;SetOverview(false);}
        void Update()
        {
            var k=Keyboard.current;
            if(k!=null)
            {
                if(k.escapeKey.wasPressedThisFrame){if(DocumentOpen)CloseDocument();else exitOpen=!exitOpen;}
                if(!DocumentOpen&&!exitOpen&&k.tabKey.wasPressedThisFrame)SetOverview(!overview);
                if(!DocumentOpen&&!exitOpen&&k.eKey.wasPressedThisFrame)OpenFromRay(new Ray(view.transform.position,view.transform.forward));
                if(DocumentOpen)
                {
                    if(k.digit1Key.wasPressedThisFrame)SelectPage(0);
                    if(k.digit2Key.wasPressedThisFrame)SelectPage(1);
                    if(k.digit3Key.wasPressedThisFrame)SelectPage(2);
                }
            }
            if(!DocumentOpen&&!exitOpen&&Mouse.current!=null&&Mouse.current.leftButton.wasPressedThisFrame)
                OpenFromRay(view.ScreenPointToRay(Mouse.current.position.ReadValue()));
        }
        void Styles()
        {
            if(text!=null)return;
            text=new GUIStyle(GUI.skin.label){font=font,fontSize=23,wordWrap=true,normal={textColor=new Color(.83f,.87f,.73f)}};
            title=new GUIStyle(text){fontSize=37};small=new GUIStyle(text){fontSize=19};
            paper=new GUIStyle(text){fontSize=24,normal={textColor=new Color(.13f,.16f,.12f)}};
            heading=new GUIStyle(paper){fontSize=28};
            button=new GUIStyle(GUI.skin.button){font=font,fontSize=22,wordWrap=true};
        }
        void Panel(Rect r,Color c){GUI.color=c;GUI.DrawTexture(r,Texture2D.whiteTexture);GUI.color=Color.white;}
        void OnGUI()
        {
            Styles();var old=GUI.matrix;GUI.matrix=Matrix4x4.TRS(Vector3.zero,Quaternion.identity,new Vector3(Screen.width/1280f,Screen.height/800f,1));
            if(exitOpen)
            {
                Panel(new Rect(290,230,700,330),new Color(.035f,.065f,.05f,.98f));
                GUI.Label(new Rect(330,265,630,100),"ARCHIVE STUDY / PAUSED\nThis standalone study does not save progress.",text);
                if(GUI.Button(new Rect(330,390,290,55),"RETURN TO STUDY",button))exitOpen=false;
                if(GUI.Button(new Rect(650,390,290,55),"QUIT STUDY",button))Application.Quit();
            }
            else if(DocumentOpen)DrawDocuments();
            else
            {
                Panel(new Rect(24,22,520,86),new Color(.02f,.04f,.03f,.92f));
                GUI.Label(new Rect(43,29,480,42),"SARO / RECORDS ROOM",title);
                GUI.Label(new Rect(43,71,490,30),"P04 / STANDALONE INTERACTION STUDY",small);
                Panel(new Rect(230,667,820,108),new Color(.02f,.04f,.03f,.95f));
                GUI.Label(new Rect(252,678,770,30),Complete?"FINDING RECORDED / Review the source documents again.":"Examine the two versions of the field protocol.",text);
                GUI.Label(new Rect(252,718,770,45),"Click the folder / E to inspect    TAB / change view    ESC / pause",small);
                if(GUI.Button(new Rect(1080,24,175,44),overview?"DESK VIEW":"ROOM VIEW",button))SetOverview(!overview);
                if(Complete&&GUI.Button(new Rect(1080,79,175,44),"RESET STUDY",button))ResetStudy();
            }
            GUI.matrix=old;
        }
        void DrawDocuments()
        {
            Panel(new Rect(0,0,1280,800),new Color(.025f,.043f,.033f,.99f));
            GUI.Label(new Rect(42,24,1060,43),"STATION 01 / COMPARE THE RECORD",title);
            if(GUI.Button(new Rect(1110,24,130,43),"CLOSE / ESC",button))CloseDocument();
            if(GUI.Button(new Rect(42,88,330,44),"1 / ORIGINAL PROTOCOL",button))SelectPage(0);
            if(GUI.Button(new Rect(388,88,330,44),"2 / AMENDED COPY",button))SelectPage(1);
            GUI.enabled=OriginalRead&&AmendedRead;
            if(GUI.Button(new Rect(734,88,506,44),"3 / COMPARE AND RECORD FINDING",button))SelectPage(2);
            GUI.enabled=true;
            if(Page<2)
            {
                Panel(new Rect(148,159,984,542),new Color(.83f,.81f,.69f));
                GUI.Label(new Rect(183,182,910,48),Page==0?"ORIGINAL FIELD PROTOCOL / E07":"AMENDED SERVICE COPY / E06",heading);
                GUI.Label(new Rect(183,244,900,422),Page==0?original.text:amended.text,paper);
                GUI.Label(new Rect(150,723,1000,46),"Read both versions, then compare. The documents remain available in any order.",text);
            }
            else
            {
                Panel(new Rect(42,158,587,365),new Color(.83f,.81f,.69f));Panel(new Rect(650,158,590,365),new Color(.79f,.79f,.68f));
                GUI.Label(new Rect(64,172,550,36),"ORIGINAL / E07",heading);GUI.Label(new Rect(672,172,548,36),"AMENDED / E06",heading);
                GUI.Label(new Rect(64,215,540,296),"The arrangement used three references: A, the fixed survey point; B, the optical comparison vane; and C, the closing sight line.\n\nThe run was interrupted before the final observation was entered. Preserve the original plates and the complete arrangement drawing.",paper);
                GUI.Label(new Rect(672,215,540,296),"The additional mark is attributed to a fault in plate development. The lamp and comparison vane are sufficient to describe the test. The incomplete closing sight line has been omitted from the service copy.\n\nNo repeat observation is required.",paper);
                GUI.Label(new Rect(42,536,1160,32),"Which finding is supported by these two versions?",text);
                for(int i=0;i<3;i++)
                {
                    string[] answers={"The author deliberately caused the anomaly.","The service copy omits a reference from the original arrangement.","Both versions describe the same complete test."};
                    if(GUI.Button(new Rect(42,579+i*46,1198,40),answers[i],button))SubmitFinding(i);
                }
                GUI.Label(new Rect(43,721,1190,74),Feedback,small);
            }
        }
    }
}
