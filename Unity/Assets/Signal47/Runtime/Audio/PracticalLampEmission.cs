using UnityEngine;
namespace Signal47.Audio
{
    // The visible emitter follows the physical experiment, including save restore.
    [RequireComponent(typeof(Renderer))]
    public sealed class PracticalLampEmission : MonoBehaviour
    {
        public Light source;
        Renderer surface; MaterialPropertyBlock block;
        static readonly int Emission=Shader.PropertyToID("_EmissionColor");
        void Awake(){surface=GetComponent<Renderer>();block=new MaterialPropertyBlock();}
        void LateUpdate()
        {
            float amount=source&&source.enabled?Mathf.Clamp01(source.intensity/3f):0;
            block.SetColor(Emission,new Color(.9f,.48f,.16f)*amount);
            surface.SetPropertyBlock(block);
        }
    }
}
