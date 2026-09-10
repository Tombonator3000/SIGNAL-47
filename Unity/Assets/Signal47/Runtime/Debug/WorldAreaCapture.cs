using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using Signal47.Core;
namespace Signal47.Debugging
{
    // Fixed-camera visual evidence, not traversal. Original PNGs are never retouched.
    public sealed class WorldAreaCapture : MonoBehaviour
    {
        [Serializable] sealed class View { public string file,state; public Vector3 position,target; public float fov; }
        [Serializable] sealed class Manifest
        {
            public string unity,resolution,quality,gpu,renderer,buildId,method,mobilePreviews;
            public bool development;public View[] views;
        }
        string root;readonly List<View> views=new List<View>();
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        static void Init()
        {
            if((Array.IndexOf(System.Environment.GetCommandLineArgs(),"--signal47-world-capture")>=0 || Array.IndexOf(System.Environment.GetCommandLineArgs(),"--signal47-yard-capture")>=0)&&!FindFirstObjectByType<WorldAreaCapture>())new GameObject("WorldAreaCapture").AddComponent<WorldAreaCapture>();
        }
        void Awake(){DontDestroyOnLoad(gameObject);Application.runInBackground=true;root=Path.GetFullPath(Path.Combine(Application.dataPath,"../../WorldCapture"));Directory.CreateDirectory(root);StartCoroutine(Run());}
        IEnumerator Run()
        {
            GameSession session=null;
            for(int i=0;i<240&&!session;i++){session=GameSession.Instance;yield return null;}
            if(!session||!session.player||!session.player.viewCamera){Debug.LogError("WORLD_CAPTURE_FAIL missing player");Application.Quit(2);yield break;}
            // Explicit repeatable capture state; never counted as a real-input test.
            session.hud.StartShift();session.hud.CloseModal();session.director.PowerReceiver();
            session.player.enabled=false;session.hud.enabled=false;
            var interactor=session.player.GetComponent<Signal47.Interaction.PlayerInteractor>();if(interactor)interactor.enabled=false;
            yield return new WaitForSecondsRealtime(3);
            var camera=session.player.viewCamera;camera.fieldOfView=60;
            if(Array.IndexOf(System.Environment.GetCommandLineArgs(),"--signal47-yard-capture")>=0)
            {
                yield return Shot(camera,"yard-path.png",new Vector3(10.7f,1.78f,2),new Vector3(11,1,-13.2f),"fixed service path lighting review; NOT traversal");
                yield return Shot(camera,"yard-cabinet.png",new Vector3(10.7f,1.78f,-12.2f),new Vector3(12.1f,1.1f,-13.2f),"fixed S-03 lighting review; NOT progression");
                Debug.Log("YARD_CAPTURE_PASS 2");Application.Quit(0);yield break;
            }
            yield return Shot(camera,"01-control-room-array.png",new Vector3(0,1.65f,4.55f),new Vector3(0,1.8f,-22),"receiver powered; calibration not solved; HUD hidden for review");
            yield return Shot(camera,"02-workstation.png",new Vector3(-3.0f,1.65f,.4f),new Vector3(.1f,1.3f,-2.15f),"same powered control room; fixed review camera");
            yield return Shot(camera,"03-crt-detail.png",new Vector3(0,1.61f,-.3f),new Vector3(0,1.34f,-1.95f),"same powered control room; fixed review camera");
            yield return Shot(camera,"04-window-array.png",new Vector3(0,1.7f,-6.1f),new Vector3(0,3,-30),"normal array from the window; fixed review camera");
            yield return Shot(camera,"05-service-yard-array.png",new Vector3(-17,1.62f,-11.5f),new Vector3(7,3,-37),"exterior review position; NOT player traversal");
            GameObject motel=null;
            foreach(var t in Resources.FindObjectsOfTypeAll<Transform>())if(t.name=="RoadsideMotel_Blockout"&&t.gameObject.scene.IsValid()){motel=t.gameObject;break;}
            if(!motel){Debug.LogError("WORLD_CAPTURE_FAIL missing motel");Application.Quit(3);yield break;}
            bool wasActive=motel.activeSelf;motel.SetActive(true);
            yield return Shot(camera,"06-sierra-motor-court-blockout.png",motel.transform.TransformPoint(new Vector3(-34,1.65f,-18)),motel.transform.TransformPoint(new Vector3(-1,1.8f,2)),"motel blockout temporarily enabled for review; inactive in normal prologue; NOT accessible gameplay");
            motel.SetActive(wasActive);
            string stamp=Path.Combine(Application.dataPath,"../build-id.txt");
            var manifest=new Manifest{unity=Application.unityVersion,resolution=$"{Screen.width}x{Screen.height}",quality=QualitySettings.names[QualitySettings.GetQualityLevel()],gpu=SystemInfo.graphicsDeviceName,renderer=SystemInfo.graphicsDeviceType.ToString(),buildId=File.Exists(stamp)?File.ReadAllText(stamp).Trim():"unknown",development=Debug.isDebugBuild,method="six fixed runtime camera views; explicit receiver power setup; NOT traversal or performance evidence",mobilePreviews="Generated from original PNGs by evidence script; resizing/JPEG only",views=views.ToArray()};
            File.WriteAllText(Path.Combine(root,"capture-manifest.json"),JsonUtility.ToJson(manifest,true));
            Debug.Log("WORLD_CAPTURE_PASS "+views.Count);Application.Quit(0);
        }
        IEnumerator Shot(Camera camera,string file,Vector3 position,Vector3 target,string state)
        {
            camera.transform.SetPositionAndRotation(position,Quaternion.LookRotation(target-position,Vector3.up));
            yield return new WaitForSecondsRealtime(.5f);yield return new WaitForEndOfFrame();
            var image=new Texture2D(Screen.width,Screen.height,TextureFormat.RGB24,false);
            image.ReadPixels(new Rect(0,0,Screen.width,Screen.height),0,0);image.Apply(false,false);
            File.WriteAllBytes(Path.Combine(root,file),image.EncodeToPNG());Destroy(image);
            views.Add(new View{file=file,state=state,position=position,target=target,fov=camera.fieldOfView});
            Debug.Log("WORLD_CAPTURE_FRAME "+file);yield return null;
        }
    }
}
