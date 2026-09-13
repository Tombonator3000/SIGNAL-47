using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Globalization;
using GaussianSplatting.Runtime;
using UnityEngine;
using UnityEngine.Rendering;

public class Splat21Runtime : MonoBehaviour
{
    public GaussianSplatRenderer splats;
    public GaussianSplatAsset small, dense;
    public Camera cam;
    public GameObject occluder;
    [Serializable] public class Phase {public string name;public int splatCount,frames;public double seconds,meanMs,p95Ms,p99Ms;public bool validAsset,validBuffers;}
    [Serializable] public class Result {public string unity,renderer,gpu,scope;public int width,height,errors;public bool compute,shaderSupported,completed;public string failureReason;public List<Phase> phases=new();}
    Result result=new(); string output; int errorCount; bool failed; double deadline; readonly List<string> errors=new();
    void OnError(string message,string stack,LogType type){if(type==LogType.Error||type==LogType.Exception||type==LogType.Assert){errorCount++;if(errors.Count<16)errors.Add(message);}}
    void Abort(string reason,int code=1)
    {
        if(failed)return;failed=true;result.failureReason=reason;
        splats.gameObject.SetActive(false);Save();Application.Quit(code);
    }
    void Update()
    {
        if(output==null||failed)return;
        if(errorCount>0)Abort("Runtime error; probe stopped at first observed error");
        else if(Time.realtimeSinceStartupAsDouble>deadline)Abort("Probe deadline exceeded",3);
    }
    void OnDestroy(){Application.logMessageReceived-=OnError;}
    IEnumerator Start()
    {
        var args=Environment.GetCommandLineArgs(); var idx=Array.IndexOf(args,"--probe-output");
        output=idx>=0&&idx+1<args.Length?args[idx+1]:Path.Combine(Application.persistentDataPath,"Probe");
        deadline=Time.realtimeSinceStartupAsDouble+50;Directory.CreateDirectory(output); Application.logMessageReceived+=OnError;
        QualitySettings.vSyncCount=0;Application.targetFrameRate=-1;
        result.unity=Application.unityVersion;result.renderer=SystemInfo.graphicsDeviceType.ToString();result.gpu=SystemInfo.graphicsDeviceName;
        result.scope="Isolated synthetic URP probe; frame intervals are not SIGNAL47 game performance";
        result.width=Screen.width;result.height=Screen.height;result.compute=SystemInfo.supportsComputeShaders;result.shaderSupported=splats.m_ShaderSplats.isSupported;
        if(SystemInfo.graphicsDeviceType!=GraphicsDeviceType.Vulkan||!result.compute||!result.shaderSupported){Abort("Requires Vulkan and supported compute/shaders",2);yield break;}
        foreach(var phase in new[]{"baseline-close","splats-close","baseline-wide","splats-wide"})
        {
            if(failed||errorCount>0){Abort("Runtime error between phases");yield break;}
            bool enabled=phase.StartsWith("splats");bool wide=phase.EndsWith("wide");
            splats.gameObject.SetActive(false);splats.m_Asset=wide?dense:small;
            cam.transform.position=wide?new Vector3(20,16,-20):new Vector3(2,1.7f,-2);
            cam.transform.LookAt(new Vector3(0,.25f,0));occluder.SetActive(!wide);
            splats.gameObject.SetActive(enabled);
            var warm=Time.realtimeSinceStartupAsDouble+2;
            while(Time.realtimeSinceStartupAsDouble<warm){yield return null;if(failed||errorCount>0){Abort("Runtime error during warmup");yield break;}}
            var start=Time.realtimeSinceStartupAsDouble;var previous=start;var samples=new List<double>();
            while(Time.realtimeSinceStartupAsDouble-start<5)
            {yield return null;if(failed||errorCount>0){Abort("Runtime error during measurement");yield break;}var now=Time.realtimeSinceStartupAsDouble;samples.Add((now-previous)*1000);previous=now;}
            File.WriteAllLines(Path.Combine(output,phase+"-frame-ms.csv"),samples.ConvertAll(v=>v.ToString("R",CultureInfo.InvariantCulture)));
            samples.Sort();double seconds=previous-start;
            result.phases.Add(new Phase{name=phase,splatCount=enabled?splats.splatCount:0,frames=samples.Count,seconds=seconds,meanMs=seconds*1000/samples.Count,p95Ms=samples[(int)((samples.Count-1)*.95)],p99Ms=samples[(int)((samples.Count-1)*.99)],validAsset=splats.HasValidAsset,validBuffers=splats.HasValidRenderSetup});
            yield return new WaitForEndOfFrame();if(failed||errorCount>0){Abort("Runtime error before capture");yield break;}var capture=ScreenCapture.CaptureScreenshotAsTexture();
            File.WriteAllBytes(Path.Combine(output,phase+".png"),capture.EncodeToPNG());Destroy(capture);
            Save();
        }
        if(failed||errorCount>0){Abort("Runtime error after capture");yield break;}
        result.completed=true;Save();Application.Quit(errorCount==0?0:1);
    }
    void Save(){result.errors=errorCount;File.WriteAllText(Path.Combine(output,"result.json"),JsonUtility.ToJson(result,true));File.WriteAllLines(Path.Combine(output,"errors.txt"),errors);}
}
