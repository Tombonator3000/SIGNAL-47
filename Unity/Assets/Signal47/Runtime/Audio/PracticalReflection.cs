using UnityEngine;
using UnityEngine.Rendering;
namespace Signal47.Audio
{
    /// <summary>Refresh after the physical reference settles, never on every moving frame.</summary>
    [RequireComponent(typeof(ReflectionProbe))]
    public sealed class PracticalReflection : MonoBehaviour
    {
        public Light practical;
        public Transform reference;
        ReflectionProbe probe;
        Quaternion lastRotation;
        float lastIntensity, changedAt;
        bool pending=true;
        int renderId=-1;
        void Awake()
        {
            probe=GetComponent<ReflectionProbe>();
            if(SystemInfo.graphicsDeviceType==GraphicsDeviceType.Null){enabled=false;return;}
            probe.refreshMode=ReflectionProbeRefreshMode.ViaScripting;
            changedAt=Time.realtimeSinceStartup;
        }
        void LateUpdate()
        {
            float intensity=practical?practical.intensity:0;
            Quaternion rotation=reference?reference.rotation:Quaternion.identity;
            if(Mathf.Abs(intensity-lastIntensity)>.001f || Quaternion.Angle(rotation,lastRotation)>.01f)
            {lastIntensity=intensity;lastRotation=rotation;changedAt=Time.realtimeSinceStartup;pending=true;}
            if(renderId>=0)
            {
                if(!probe.IsFinishedRendering(renderId))return;
                Debug.Log("VISUAL10_REFLECTION_READY "+name+" texture="+(probe.texture?probe.texture.width:0)+" intensity="+lastIntensity);
                renderId=-1;
            }
            if(pending && Time.realtimeSinceStartup-changedAt>=.25f)
            {pending=false;renderId=probe.RenderProbe();}
        }
    }
}
