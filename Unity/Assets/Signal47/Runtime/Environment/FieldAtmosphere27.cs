using System.Collections.Generic;
using UnityEngine;
namespace Signal47.Environment
{
    /// <summary>Field lighting is local to this visit; restore the SARO settings on return.</summary>
    public sealed class FieldAtmosphere27 : MonoBehaviour
    {
        readonly List<Light> disabledLights=new List<Light>();
        Color ambient,fogColor;float fogDensity;
        void OnEnable()
        {
            ambient=RenderSettings.ambientLight;fogColor=RenderSettings.fogColor;fogDensity=RenderSettings.fogDensity;
            RenderSettings.ambientLight=new Color(.14f,.18f,.25f);
            RenderSettings.fogColor=new Color(.035f,.055f,.08f);RenderSettings.fogDensity=.006f;
            disabledLights.Clear();
            foreach(var light in FindObjectsByType<Light>(FindObjectsSortMode.None))
                if(light.type==LightType.Directional&&light.enabled&&!light.transform.IsChildOf(transform))
                {disabledLights.Add(light);light.enabled=false;}
        }
        void OnDisable()
        {
            RenderSettings.ambientLight=ambient;RenderSettings.fogColor=fogColor;RenderSettings.fogDensity=fogDensity;
            foreach(var light in disabledLights)if(light)light.enabled=true;
            disabledLights.Clear();
        }
    }
}
