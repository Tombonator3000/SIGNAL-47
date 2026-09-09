using UnityEngine;
using Signal47.Core;
namespace Signal47.Signals
{
    public sealed class SignalConsole : MonoBehaviour, IInteractable
    {
        public static bool AnyOpen { get; private set; }
        public SignalProfile[] profiles; public Renderer physicalScreen;
        public string Prompt=>"RX CONTROL CONSOLE";
        public float frequency=1419.620f,gain=27,bandwidth=82,azimuth=18;
        int stage; bool solved; string status="CALIBRATION REQUIRED"; Texture2D spectrum; float nextSpectrum;
        public void Interact()
        {
            var gs=GameSession.Instance;if(!gs.director.ReceiverPowered){gs.hud.Toast("No carrier. Receiver bank has no power.");return;}
            AnyOpen=true;gs.hud.SetCursor(false);RenderSpectrum();UpdatePhysicalScreen();
        }
        void Start(){RenderSpectrum();UpdatePhysicalScreen();}
        void OnDisable(){AnyOpen=false;if(spectrum)Destroy(spectrum);}
        void Update(){if(AnyOpen && Time.unscaledTime>=nextSpectrum){nextSpectrum=Time.unscaledTime+.1f;RenderSpectrum();}}
        void OnGUI()
        {
            if(!AnyOpen)return;
            float scale=Mathf.Min(Screen.width/960f,Screen.height/760f);var previous=GUI.matrix;GUI.matrix=Matrix4x4.TRS(Vector3.zero,Quaternion.identity,Vector3.one*scale);
            float w=880,h=700,x=(Screen.width/scale-w)/2,y=(Screen.height/scale-h)/2;
            GUI.color=new Color(.03f,.09f,.055f,.98f);GUI.DrawTexture(new Rect(x,y,w,h),Texture2D.whiteTexture);GUI.color=Color.white;
            var green=new GUIStyle(GUI.skin.label){fontSize=16,normal={textColor=new Color(.55f,1f,.64f)},wordWrap=true};
            var head=new GUIStyle(green){fontSize=24,fontStyle=FontStyle.Bold};
            GUI.Label(new Rect(x+28,y+20,w-56,36),"SARO / RX CONTROL 03",head);GUI.Label(new Rect(x+28,y+58,w-56,28),status,green);
            if(spectrum!=null)GUI.DrawTexture(new Rect(x+28,y+96,w-56,220),spectrum,ScaleMode.StretchToFill,false);
            GUI.Label(new Rect(x+28,y+326,w-56,48),profiles[Mathf.Min(stage,profiles.Length-1)].referenceCard,green);
            float sy=y+380; frequency=SliderRow("FREQUENCY MHz",frequency,1419.5f,1420.7f,ref sy,x,w,green,"F3");
            gain=SliderRow("GAIN",gain,0,100,ref sy,x,w,green,"F0");bandwidth=SliderRow("BANDWIDTH kHz",bandwidth,4,100,ref sy,x,w,green,"F0");azimuth=SliderRow("ARRAY AZ",azimuth,0,180,ref sy,x,w,green,"F0");
            if(GUI.Button(new Rect(x+28,y+h-62,260,38),solved?"PRINT COMPLETE":(stage<profiles.Length?profiles[stage].actionLabel:"DIRECTION SOLVE")))Action();
            if(GUI.Button(new Rect(x+w-168,y+h-62,140,38),"EXIT"))Close();
            if(solved)GUI.Label(new Rect(x+320,y+h-85,w-520,70),"SOURCE: UNKNOWN   1420.405 MHz\nRA 05h17m32s  DEC -05°23'14\"   S/N 4.71   DISTANCE SOLVE: -39 LY",green);
            GUI.matrix=previous;
        }
        float SliderRow(string label,float value,float min,float max,ref float y,float x,float w,GUIStyle s,string fmt)
        {GUI.Label(new Rect(x+28,y,270,30),$"{label}: {value.ToString(fmt)}",s);value=GUI.HorizontalSlider(new Rect(x+310,y+6,w-350,18),value,min,max);y+=42;return value;}
        public void Action()
        {
            if(solved)return;
            if(stage<profiles.Length)
            {
                var p=profiles[stage];if(!p.Pass(frequency,gain,bandwidth,azimuth)){status="NO LOCK // ADJUST PARAMETERS";return;}
                GameSession.Instance.notebook.Add(p.successNote);stage++;
                if(stage==1){status="LOCAL CARRIER DRIFT";frequency=1419.98f;gain=55;bandwidth=58;azimuth=42;}
                else if(stage==2){status="RESIDUAL CARRIER";frequency=1420.18f;gain=53;bandwidth=28;azimuth=48;}
                else {status="PATTERN LOCK // PULSE GROUP 4 / 7";frequency=1420.405f;gain=82;bandwidth=12;azimuth=83;}
                UpdatePhysicalScreen();return;
            }
            solved=true;status="OUTPUT ROUTED TO PRINTER";GameSession.Instance.notebook.Add("1420.405 MHz — repeating 4 / 7 pulse grouping.");GameSession.Instance.notebook.Add("Direction solve returned -39 LY.");GameSession.Instance.director.OnSignalSolved();UpdatePhysicalScreen();
        }
        public void Close(){AnyOpen=false;GameSession.Instance.hud.SetCursor(true);}
        void RenderSpectrum()
        {
            if(spectrum==null){spectrum=new Texture2D(512,220,TextureFormat.RGBA32,false);spectrum.wrapMode=TextureWrapMode.Clamp;}
            Color bg=new(.01f,.035f,.02f), grid=new(.05f,.18f,.09f), trace=new(.45f,1f,.57f);var px=new Color[512*220];for(int i=0;i<px.Length;i++)px[i]=bg;
            for(int gx=0;gx<512;gx+=64)for(int yy=0;yy<220;yy++)px[yy*512+gx]=grid;for(int gy=30;gy<220;gy+=35)for(int xx=0;xx<512;xx++)px[gy*512+xx]=grid;
            float peak=stage==0?1419.9f:stage==1?1420.11f:1420.405f;float peakX=(peak-1419.5f)/1.2f*511;
            for(int xx=0;xx<512;xx++){float noise=Mathf.Sin(xx*.17f+Time.unscaledTime*4)*2+Mathf.Sin(xx*.043f)*3;float amp=stage>=2?58:50;float yy=158-amp*Mathf.Exp(-Mathf.Pow((xx-peakX)/14,2))+noise;int iy=Mathf.Clamp(Mathf.RoundToInt(yy),1,218);px[iy*512+xx]=trace;px[(iy+1)*512+xx]=trace;}
            spectrum.SetPixels(px);spectrum.Apply(false,false);if(physicalScreen){physicalScreen.material.SetTexture("_BaseMap",spectrum);physicalScreen.material.SetColor("_BaseColor",Color.white);}
        }
        void UpdatePhysicalScreen(){if(physicalScreen==null)return;var m=physicalScreen.material;m.SetColor("_BaseColor",Color.white);if(m.HasProperty("_EmissionColor")){m.EnableKeyword("_EMISSION");m.SetColor("_EmissionColor",new Color(.08f,.9f,.22f)*(solved?2.5f:1.3f));}}
    }
}
