using UnityEngine;
namespace Signal47.Environment
{
    public sealed class DishArrayController:MonoBehaviour
    {
        public Transform[] dishPivots; public float duration=3.2f; float start; bool turning; Quaternion[] initial;
        public float Progress=>turning?Mathf.Clamp01((Time.time-start)/duration):0;
        public void BeginTurn(){if(turning)return;turning=true;start=Time.time;initial=new Quaternion[dishPivots.Length];for(int i=0;i<dishPivots.Length;i++)initial[i]=dishPivots[i].localRotation;}
        void Update(){if(!turning)return;float t=Mathf.SmoothStep(0,1,Progress);for(int i=0;i<dishPivots.Length;i++)dishPivots[i].localRotation=initial[i]*Quaternion.Euler(0,26*t,0);}
    }
}
