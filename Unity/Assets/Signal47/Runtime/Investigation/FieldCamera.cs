using System;
using System.Collections;
using System.IO;
using System.Threading.Tasks;
using UnityEngine;
using UnityEngine.InputSystem;
using UnityEngine.Rendering;
using UnityEngine.Experimental.Rendering;
using Signal47.Core;

namespace Signal47.Investigation
{
    public sealed class FieldCamera : MonoBehaviour
    {
        public bool Acquired { get; private set; }
        public bool Raised { get; private set; }
        public bool Capturing { get; private set; }
        public bool HasPhoto => photograph;
        public bool Compared { get; private set; }
        public int RejectedFrames { get; private set; }
        public string SaveStatus { get; private set; }="";
        public string PhotoPath { get; private set; }="";
        public Texture2D photograph;
        public AudioClip shutter;
        public const string PhotoId="s03-field-photograph";
        public static readonly Vector3 Subject=new Vector3(8,4.8f,-34);
        AudioSource sound;bool busy;Task saveJob;string frameTime="",buildId="";
        [Serializable] sealed class PhotoMetadata { public string subject,utc,build;public Vector3 position,forward;public int width,height; }
        public string Objective => !Acquired?"FIELD KIT // COLLECT THE CAMERA BESIDE THE EAST DOOR":!HasPhoto?"S-03 // C: RAISE CAMERA / PHOTOGRAPH THE CENTRAL ANTENNA":!Compared?"RETURN INSIDE // TAB: OPEN THE PHOTOGRAPH AND COMPARE THE LOG":"S-03 // PHOTOGRAPH AND MOTOR LOG FILED";
        void Awake(){string id=Path.Combine(Application.dataPath,"../build-id.txt");buildId=File.Exists(id)?File.ReadAllText(id).Trim():Application.version;sound=gameObject.AddComponent<AudioSource>();sound.playOnAwake=false;sound.spatialBlend=0;}
        public void TakeCamera(){if(Acquired)return;Acquired=true;GameSession.Instance.notebook.Add("Collected the field camera. Photograph the antenna profile from the S-03 service apron.");GameSession.Instance.hud.Toast("FIELD CAMERA // C TO RAISE / SPACE TO TAKE PHOTOGRAPH",4);}
        void Update()
        {
            var g=GameSession.Instance;if(!g)return;
            if(saveJob!=null && saveJob.IsCompleted)
            {
                SaveStatus=saveJob.IsFaulted?"SAVE FAILED — photograph remains in this notebook":"SAVED";
                if(saveJob.IsFaulted)Debug.LogWarning("FIELD_PHOTO_SAVE_FAILED "+saveJob.Exception.GetBaseException().Message);
                saveJob=null;
            }
            if(!g.CanControl){if(!Capturing)Raised=false;return;}
            var keys=Keyboard.current;if(keys==null||!Acquired)return;
            if(keys.cKey.wasPressedThisFrame && !busy)Raised=!Raised;
            if(Raised && keys.spaceKey.wasPressedThisFrame && !busy)
            {
                string reason=FrameProblem();
                if(reason!=""){RejectedFrames++;g.hud.Toast(reason,3);return;}
                if(HasPhoto){g.hud.Toast("FRAME 01 ALREADY FILED // TAB TO REVIEW",3);return;}
                StartCoroutine(Expose());
            }
        }
        public string FrameProblem()
        {
            var g=GameSession.Instance;var c=g.player.viewCamera;
            if(!g.yard || !g.yard.Completed)return "READ THE S-03 MOTOR LOG FIRST";
            if(c.transform.position.x<9.5f || c.transform.position.z>-9 || c.transform.position.z<-17)return "MOVE TO THE S-03 SERVICE APRON";
            Vector3 point=c.WorldToViewportPoint(Subject);float distance=Vector3.Distance(c.transform.position,Subject);
            if(point.z<0 || point.x<.36f || point.x>.64f || point.y<.30f || point.y>.65f || distance>32)return "ALIGN THE CENTRAL ANTENNA IN THE FRAME";
            if(Physics.Linecast(c.transform.position,Subject,out var hit,Physics.DefaultRaycastLayers,QueryTriggerInteraction.Ignore) && hit.distance<distance-4)return "VIEW OBSTRUCTED // FIND A CLEAR LINE OF SIGHT";
            return "";
        }
        IEnumerator Expose()
        {
            busy=true;Capturing=true;
            // Hide editable UI for this exposure; capture the actual player's rendered scene.
            yield return new WaitForEndOfFrame();
            int width=Screen.width,height=Screen.height;
            var full=RenderTexture.GetTemporary(width,height,0,RenderTextureFormat.ARGB32);
            var small=RenderTexture.GetTemporary(960,Mathf.RoundToInt(960f*height/width),0,RenderTextureFormat.ARGB32);
            ScreenCapture.CaptureScreenshotIntoRenderTexture(full);Graphics.Blit(full,small);RenderTexture.ReleaseTemporary(full);
            var c=GameSession.Instance.player.viewCamera;
            var metadata=new PhotoMetadata{subject="S-03 / array profile",utc=DateTime.UtcNow.ToString("O"),build=buildId,position=c.transform.position,forward=c.transform.forward,width=small.width,height=small.height};
            Capturing=false;Raised=false;if(shutter)sound.PlayOneShot(shutter,.28f);
            if(SystemInfo.supportsAsyncGPUReadback)
            {
                var request=AsyncGPUReadback.Request(small,0,TextureFormat.RGBA32);
                while(!request.done)yield return null;
                if(request.hasError){RenderTexture.ReleaseTemporary(small);busy=false;GameSession.Instance.hud.Toast("EXPOSURE FAILED // TRY AGAIN",3);yield break;}
                FinishPhoto(request.GetData<byte>().ToArray(),metadata);
            }
            else
            {
                var previous=RenderTexture.active;RenderTexture.active=small;
                var copy=new Texture2D(small.width,small.height,TextureFormat.RGBA32,false);copy.ReadPixels(new Rect(0,0,small.width,small.height),0,0);copy.Apply();RenderTexture.active=previous;
                FinishPhoto(copy.GetRawTextureData<byte>().ToArray(),metadata);Destroy(copy);
            }
            RenderTexture.ReleaseTemporary(small);busy=false;
        }
        void FinishPhoto(byte[] pixels,PhotoMetadata metadata)
        {
            photograph=new Texture2D(metadata.width,metadata.height,TextureFormat.RGBA32,false);photograph.LoadRawTextureData(pixels);photograph.Apply();
            frameTime=TimeSpan.FromSeconds(23*3600+41*60+Time.timeSinceLevelLoad).ToString(@"hh\:mm\:ss");
            string directory=Path.Combine(Application.persistentDataPath,"FieldPhotos");
            PhotoPath=Path.Combine(directory,"S03-"+DateTime.UtcNow.ToString("yyyyMMdd-HHmmss")+"-"+Guid.NewGuid().ToString("N").Substring(0,8)+".jpg");
            string file=PhotoPath,json=JsonUtility.ToJson(metadata,true);SaveStatus="SAVING";
            // Unity's array JPG encoder is thread-safe; file writing never blocks an Update.
            saveJob=Task.Run(()=>{var jpg=ImageConversion.EncodeArrayToJPG(pixels,GraphicsFormat.R8G8B8A8_UNorm,(uint)metadata.width,(uint)metadata.height,0,92);Directory.CreateDirectory(directory);File.WriteAllBytes(file,jpg);File.WriteAllText(Path.ChangeExtension(file,".json"),json);});
            var g=GameSession.Instance;g.notebook.Collect(PhotoId,"PHOTO 01 / S-03 antenna profile","A photograph taken from the service apron. Compare with the controller log inside.");
            g.hud.Toast("FRAME 01 EXPOSED // TAB TO REVIEW PHOTOGRAPH",4);Debug.Log("FIELD_PHOTO_CAPTURED "+metadata.width+"x"+metadata.height);
        }
        public void Compare()
        {
            var g=GameSession.Instance;var p=g.player.transform.position;
            if(!HasPhoto || Compared || !g.yard.Completed || p.x>=8.9f || p.x<=-9 || p.z<=-7 || p.z>=7)return;
            Compared=true;g.notebook.Add("Compared the antenna profile with S-03: encoder 026 degrees, scheduled 042 degrees, zero commands. The photograph preserves the visible alignment; it does not establish the cause.");
            g.hud.Toast("COMPARISON FILED // CAUSE REMAINS OPEN",4);
        }
        public bool DrawPhotograph(GUIStyle button,GUIStyle text)
        {
            GUI.color=new Color(0,0,0,.92f);GUI.DrawTexture(new Rect(0,0,Screen.width,Screen.height),Texture2D.whiteTexture);GUI.color=Color.white;
            float scale=Mathf.Min(Screen.width/1280f,Screen.height/800f);var old=GUI.matrix;GUI.matrix=Matrix4x4.TRS(new Vector3((Screen.width-1280*scale)/2,(Screen.height-800*scale)/2,0),Quaternion.identity,Vector3.one*scale);
            GUI.color=new Color(.91f,.895f,.84f);GUI.DrawTexture(new Rect(100,65,1080,670),Texture2D.whiteTexture);GUI.color=Color.white;
            var ink=new GUIStyle(text){font=GameSession.Instance.hud.terminalFont,fontSize=25,normal={textColor=new Color(.14f,.18f,.15f)}};
            GUI.Label(new Rect(132,92,950,40),"S A R O / P H O T O G R A P H I C  R E C O R D",ink);
            GUI.color=new Color(.30f,.36f,.30f,.7f);GUI.DrawTexture(new Rect(132,134,1016,1),Texture2D.whiteTexture);GUI.DrawTexture(new Rect(823,153,1,454),Texture2D.whiteTexture);GUI.color=Color.white;
            GUI.color=new Color(.96f,.94f,.88f);GUI.DrawTexture(new Rect(132,153,678,454),Texture2D.whiteTexture);GUI.color=Color.white;
            if(photograph)GUI.DrawTexture(new Rect(145,166,652,428),photograph,ScaleMode.ScaleToFit);
            GUI.Label(new Rect(840,153,305,380),"FRAME 01\n\nS-03 / ARRAY PROFILE\n"+frameTime+" LOCAL\n\nCONTROLLER LOG\nEncoder: 026 degrees\nScheduled: 042 degrees\nCommands received: 0\n\n"+(Compared?"COMPARISON FILED\nCause remains open.":"Compare this visible profile\nwith the controller record."),ink);
            GUI.Label(new Rect(145,613,970,35),SaveStatus=="SAVED"?"FRAME 01 / LOCAL ARCHIVE COPY":SaveStatus,new GUIStyle(ink){fontSize=20});
            bool FlatButton(Rect r,string title,bool enabled,bool primary)
            {
                GUI.color=enabled&&primary?new Color(.20f,.29f,.22f):new Color(.77f,.78f,.71f);GUI.DrawTexture(r,Texture2D.whiteTexture);GUI.color=Color.white;
                var label=new GUIStyle(ink){alignment=TextAnchor.MiddleCenter,normal={textColor=enabled&&primary?new Color(.97f,.95f,.85f):new Color(.16f,.20f,.16f)}};
                GUI.Label(r,title,label);return enabled && GUI.Button(r,GUIContent.none,GUIStyle.none);
            }
            var p=GameSession.Instance.player.transform.position;bool canCompare=!Compared && p.x<8.9f && p.x>-9 && p.z>-7 && p.z<7;
            if(FlatButton(new Rect(132,665,460,42),Compared?"COMPARISON FILED":"COMPARE WITH S-03 LOG",canCompare,true))Compare();
            if(!Compared && !canCompare)GUI.Label(new Rect(840,546,285,65),"Return to the control room to compare the records.",ink);
            bool close=FlatButton(new Rect(690,665,458,42),"BACK TO NOTEBOOK",true,false);GUI.matrix=old;return close;
        }
        void OnGUI()
        {
            var g=GameSession.Instance;if(!g || !Acquired || Capturing || g.hud.ModalOpen || !g.hud.Started)return;
            if(!Raised)return;
            var ivory=new Color(.92f,.89f,.72f);float w=Screen.width,h=Screen.height;
            void line(float x,float y,float width,float height){GUI.color=ivory;GUI.DrawTexture(new Rect(x,y,width,height),Texture2D.whiteTexture);GUI.color=Color.white;}
            for(int right=0;right<2;right++)for(int bottom=0;bottom<2;bottom++){float x=w*(right==0?.065f:.935f),y=h*(bottom==0?.10f:.83f);line(x-(right==1?45:0),y,45,2);line(x,y-(bottom==1?45:0),2,45);}
            line(w/2-16,h/2,32,1);line(w/2,h/2-16,1,32);
            var style=new GUIStyle(GUI.skin.label){font=g.hud.terminalFont,fontSize=Mathf.RoundToInt(h*.028f),normal={textColor=ivory},wordWrap=true};
            GUI.Label(new Rect(w*.03f,18,w*.8f,38),"S A R O / F I E L D  C A M E R A",style);
            GUI.color=new Color(.018f,.023f,.020f,.96f);GUI.DrawTexture(new Rect(0,h*.90f,w,h*.1f),Texture2D.whiteTexture);GUI.color=Color.white;
            GUI.Label(new Rect(w*.03f,h*.925f,w*.14f,40),"FRAME 01",style);
            GUI.Label(new Rect(w*.18f,h*.925f,w*.48f,52),HasPhoto?"FRAME FILED // TAB TO REVIEW":FrameProblem()==""?"ANTENNA IN FRAME // READY":FrameProblem(),style);
            GUI.Label(new Rect(w*.72f,h*.925f,w*.27f,45),"C LOWER   SPACE SHUTTER",style);
        }
        void OnDestroy(){if(photograph)Destroy(photograph);}
    }
}
