using UnityEngine;
using Signal47.Core;
namespace Signal47.Signals
{
    public sealed class SignalConsole : MonoBehaviour, IInteractable
    {
        public static bool AnyOpen { get; private set; }
        public Font terminalFont;public SignalProfile[] profiles; public Renderer physicalScreen;
        public string Prompt=>"RX CONTROL CONSOLE";
        public float frequency=1419.620f,gain=27,bandwidth=82,azimuth=18;
        int stage; bool solved; string status="CALIBRATION REQUIRED"; Texture2D spectrum; float nextSpectrum; Color32[] pixels; Material screenMaterial; AudioSource carrier;
        public int CompletedStages=>stage;
        public float LockQuality=>SignalFeedback.Quality(profiles[Mathf.Min(stage,profiles.Length-1)],frequency,gain,bandwidth,azimuth);
        public void Interact()
        {
            var gs=GameSession.Instance;if(!gs.director.ReceiverPowered){gs.hud.Toast("No carrier. Receiver bank has no power.");return;}
            AnyOpen=true;gs.hud.SetCursor(false);RenderSpectrum();UpdatePhysicalScreen();
        }
        void Start(){
            if(physicalScreen)screenMaterial=physicalScreen.material;
            carrier=gameObject.AddComponent<AudioSource>();carrier.playOnAwake=false;carrier.loop=true;carrier.spatialBlend=1;carrier.minDistance=1;carrier.maxDistance=6;carrier.clip=Signal47.Audio.ProceduralAudio.Carrier();carrier.volume=0;carrier.Play();
            RenderSpectrum();UpdatePhysicalScreen();
        }
        void OnDisable(){AnyOpen=false;}
        void OnDestroy(){if(spectrum)Destroy(spectrum);if(screenMaterial)Destroy(screenMaterial);if(carrier&&carrier.clip)Destroy(carrier.clip);}
        void Update(){
            bool powered=GameSession.Instance && GameSession.Instance.director.ReceiverPowered;
            var director=GameSession.Instance?GameSession.Instance.director:null;if(director&&director.ArrayOverride)status=director.SignalAcquired?"SIGNAL ACQUIRED":"ARRAY CONTROL OVERRIDE // SOURCE UNKNOWN";
            if(carrier){float pulse=1;if(stage>=3){float t=Time.time%5;int count=t<2?4:7;float local=t<2?t:t-2.5f;pulse=local>=0&&local<count*.22f&&local%.22f<.1f?1:.08f;}carrier.volume=powered&&!GameSession.Instance.hud.TitleVisible?.075f*LockQuality*pulse:0;carrier.pitch=1+Mathf.Clamp((frequency-1420.405f)*.3f,-.2f,.2f);}
            if(powered && Time.time>=nextSpectrum){nextSpectrum=Time.time+.1f;RenderSpectrum();}
        }
        void OnGUI()
        {
            if(!AnyOpen)return;
            float scale=Mathf.Min(Screen.width/960f,Screen.height/760f);var previous=GUI.matrix;GUI.matrix=Matrix4x4.TRS(Vector3.zero,Quaternion.identity,Vector3.one*scale);
            float w=880,h=700,x=(Screen.width/scale-w)/2,y=(Screen.height/scale-h)/2;
            GUI.color=new Color(.03f,.09f,.055f,.98f);GUI.DrawTexture(new Rect(x,y,w,h),Texture2D.whiteTexture);GUI.color=Color.white;
            var green=new GUIStyle(GUI.skin.label){font=terminalFont,fontSize=22,normal={textColor=new Color(.55f,1f,.64f)},wordWrap=true};
            var head=new GUIStyle(green){fontSize=28,fontStyle=FontStyle.Bold};
            GUI.Label(new Rect(x+28,y+20,w-56,36),"SARO / RX CONTROL 03",head);GUI.Label(new Rect(x+28,y+58,w-56,28),status,green);GUI.Label(new Rect(x+w-218,y+24,190,28),$"CARRIER {LockQuality*100:0}%",green);
            if(spectrum!=null)GUI.DrawTexture(new Rect(x+28,y+96,w-56,220),spectrum,ScaleMode.StretchToFill,false);
            GUI.Label(new Rect(x+28,y+326,w-56,48),profiles[Mathf.Min(stage,profiles.Length-1)].referenceCard,green);
            GUI.Label(new Rect(x+28,y+553,310,30),"FINE TUNE / 1 kHz steps",green);
            if(GUI.Button(new Rect(x+310,y+553,90,28),"− 0.001"))frequency=Mathf.Max(1419.5f,frequency-.001f);
            if(GUI.Button(new Rect(x+410,y+553,90,28),"+ 0.001"))frequency=Mathf.Min(1420.7f,frequency+.001f);
            float sy=y+380; frequency=SliderRow("FREQUENCY MHz",frequency,1419.5f,1420.7f,ref sy,x,w,green,"F3");
            gain=SliderRow("GAIN",gain,0,100,ref sy,x,w,green,"F0");bandwidth=SliderRow("BANDWIDTH kHz",bandwidth,4,100,ref sy,x,w,green,"F0");azimuth=SliderRow("ARRAY AZ",azimuth,0,180,ref sy,x,w,green,"F0");
            if(GUI.Button(new Rect(x+28,y+h-62,260,38),solved?"SENT TO PRINTER":(stage<profiles.Length?profiles[stage].actionLabel:"DIRECTION SOLVE")))Action();
            if(GUI.Button(new Rect(x+w-168,y+h-62,140,38),"EXIT"))Close();
            if(solved)GUI.Label(new Rect(x+320,y+h-85,w-520,70),"SOURCE: UNKNOWN   1420.405 MHz\nRA 05h17m32s  DEC -05°23'14\"   S/N 4.71   DISTANCE SOLVE: SEE PRINTOUT",green);
            GUI.matrix=previous;
        }
        float SliderRow(string label,float value,float min,float max,ref float y,float x,float w,GUIStyle s,string fmt)
        {GUI.Label(new Rect(x+28,y,270,30),$"{label}: {value.ToString(fmt)}",s);value=GUI.HorizontalSlider(new Rect(x+310,y+6,w-350,18),value,min,max);y+=42;return value;}
        public void Action()
        {
            if(GameSession.Instance.director.palette)GameSession.Instance.director.palette.Click();
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
            solved=true;status="OUTPUT ROUTED TO PRINTER";GameSession.Instance.notebook.Add("1420.405 MHz — repeating 4 / 7 pulse grouping.");GameSession.Instance.notebook.Add("Direction solve routed to printer.");GameSession.Instance.director.OnSignalSolved();UpdatePhysicalScreen();
        }
        public void Close(){AnyOpen=false;GameSession.Instance.hud.SetCursor(true);}
        void RenderSpectrum()
        {
            const int width=512,height=220;
            if(spectrum==null){spectrum=new Texture2D(width,height,TextureFormat.RGBA32,false);spectrum.wrapMode=TextureWrapMode.Clamp;pixels=new Color32[width*height];}
            bool powered=GameSession.Instance && GameSession.Instance.director.ReceiverPowered;
            Color32 bg=new Color(.009f,.024f,.015f),grid=new Color(.04f,.13f,.065f),trace=new Color(.48f,1f,.58f),band=new Color(.023f,.08f,.043f);
            float peak=profiles[Mathf.Min(stage,profiles.Length-1)].targetFrequency;
            float peakX=(peak-1419.5f)/1.2f*(width-1),tunedX=(frequency-1419.5f)/1.2f*(width-1);
            float halfWidth=bandwidth/1000f/1.2f*width*.5f;
            for(int y=0;y<height;y++)for(int x=0;x<width;x++)pixels[y*width+x]=powered&&Mathf.Abs(x-tunedX)<halfWidth?band:bg;
            for(int x=0;x<width;x+=64)for(int y=0;y<height;y++)pixels[y*width+x]=grid;
            for(int y=30;y<height;y+=35)for(int x=0;x<width;x++)pixels[y*width+x]=grid;
            if(powered){
                for(int x=0;x<width;x++){
                    float noise=(Mathf.Sin(x*.17f+Time.time*4)*2+Mathf.Sin(x*.71f+Time.time*7)*3)*(bandwidth/35f+.2f);
                    float amp=12+110*LockQuality*gain/100;
                    int y=Mathf.Clamp(Mathf.RoundToInt(35+amp*Mathf.Exp(-Mathf.Pow((x-peakX)/7,2))+noise),1,height-2);
                    pixels[y*width+x]=trace;pixels[(y+1)*width+x]=trace;
                }
                int marker=Mathf.Clamp(Mathf.RoundToInt(tunedX),0,width-1);for(int y=0;y<height;y+=3)pixels[y*width+marker]=new Color(.85f,.70f,.28f);
            }
            spectrum.SetPixels32(pixels);spectrum.Apply(false,false);
            if(screenMaterial){screenMaterial.SetTexture("_BaseMap",spectrum);screenMaterial.SetColor("_BaseColor",Color.white);}
        }
        void UpdatePhysicalScreen(){if(physicalScreen==null)return;var m=screenMaterial;if(m==null)return;m.SetColor("_BaseColor",Color.white);if(m.HasProperty("_EmissionColor")){m.EnableKeyword("_EMISSION");m.SetColor("_EmissionColor",new Color(.08f,.9f,.22f)*(solved?2.5f:1.3f));}}
    }
}
