using UnityEngine;
namespace Signal47.Signals
{
    public static class SignalFeedback
    {
        static float RangeDistance(float value,Vector2 range)=>Mathf.Max(range.x-value,0,value-range.y);
        public static float Quality(SignalProfile p,float frequency,float gain,float bandwidth,float azimuth)
        {
            float f=Mathf.Max(0,Mathf.Abs(frequency-p.targetFrequency)-p.frequencyTolerance)/Mathf.Max(p.frequencyTolerance*4,.025f);
            float g=RangeDistance(gain,p.gainRange)/22;
            float b=RangeDistance(bandwidth,p.bandwidthRange)/25;
            float a=RangeDistance(azimuth,p.azimuthRange)/25;
            return Mathf.Exp(-(f*f+g*g+b*b+a*a));
        }
    }
}
