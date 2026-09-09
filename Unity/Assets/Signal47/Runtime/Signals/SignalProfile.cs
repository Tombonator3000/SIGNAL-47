using UnityEngine;
namespace Signal47.Signals
{
    [CreateAssetMenu(menuName="SIGNAL47/Signal Profile")]
    public sealed class SignalProfile : ScriptableObject
    {
        public string stageName; public string actionLabel; [TextArea] public string referenceCard;
        public float targetFrequency, frequencyTolerance;
        public Vector2 gainRange, bandwidthRange, azimuthRange;
        public string successNote;
        public bool Pass(float f,float g,float bw,float az)=>Mathf.Abs(f-targetFrequency)<=frequencyTolerance&&g>=gainRange.x&&g<=gainRange.y&&bw>=bandwidthRange.x&&bw<=bandwidthRange.y&&az>=azimuthRange.x&&az<=azimuthRange.y;
    }
}
