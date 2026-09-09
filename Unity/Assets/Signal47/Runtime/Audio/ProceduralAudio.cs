using UnityEngine;
namespace Signal47.Audio
{
    public static class ProceduralAudio
    {
        const int Rate=44100;
        public static AudioClip Carrier()=>Build("ReceiverCarrier",1f,t=>(Mathf.Sin(2*Mathf.PI*440*t)+.18f*Mathf.Sin(2*Mathf.PI*880*t))*.24f);
        public static AudioClip Ring()=>Build("PhoneRing",1.8f,(t)=>{float env=((t<.35f)||(t>.48f&&t<.83f))?1:0;return env*(Mathf.Sin(2*Mathf.PI*440*t)+.75f*Mathf.Sin(2*Mathf.PI*480*t))*.18f;});
        public static AudioClip Boom()=>Build("Boom",1.25f,t=>(Mathf.Sin(2*Mathf.PI*42*t)*Mathf.Exp(-2.2f*t)+Noise(t)*.18f*Mathf.Exp(-3*t))*.5f);
        public static AudioClip Smash()=>Build("Smash",.55f,t=>Noise(t)*Mathf.Exp(-7*t)*.42f + Mathf.Sin(2*Mathf.PI*(1450+2400*t)*t)*Mathf.Exp(-8*t)*.15f);
        public static AudioClip Printer()=>Build("Printer",1.2f,t=>{float tick=(t*14)%1;return (tick<.18f?Noise(t)*.16f:0)+Mathf.Sin(2*Mathf.PI*(1600+(int)(t*14)%3*180)*t)*.035f;});
        public static AudioClip FutureCall()=>Build("FutureCall",4.3f,t=>{float v=Noise(t)*.025f+Mathf.Sin(2*Mathf.PI*63*t)*.012f;if(t>1.75f&&t<2.8f)v+=Mathf.Sin(2*Mathf.PI*42*(t-1.75f))*Mathf.Exp(-3*(t-1.75f))*.35f;if(t>2.48f&&t<3.05f)v+=Noise(t)*Mathf.Exp(-8*(t-2.48f))*.35f;return v;});
        public static AudioClip Hum()=>Build("RoomHum",2f,t=>Mathf.Sin(2*Mathf.PI*58*t)*.035f+Mathf.Sin(2*Mathf.PI*116*t)*.009f);
        static float Noise(float t){unchecked{int n=(int)(t*Rate);n=(n<<13)^n;return 1f-((n*(n*n*15731+789221)+1376312589)&0x7fffffff)/1073741824f;}}
        static AudioClip Build(string name,float seconds,System.Func<float,float> fn){int count=Mathf.CeilToInt(seconds*Rate);float[] data=new float[count];for(int i=0;i<count;i++)data[i]=Mathf.Clamp(fn(i/(float)Rate),-1,1);var c=AudioClip.Create(name,count,1,Rate,false);c.SetData(data,0);return c;}
    }
}
