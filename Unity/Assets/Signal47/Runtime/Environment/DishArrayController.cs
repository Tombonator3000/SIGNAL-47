using UnityEngine;
namespace Signal47.Environment
{
    public sealed class DishArrayController:MonoBehaviour
    {
        public Transform[] dishPivots;
        public float duration=2.6f,stagger=.1f;
        float start;bool turning;Quaternion[] initial,bowlInitial;
        public float Progress=>turning?Mathf.Clamp01((Time.time-start)/(duration+stagger*Mathf.Max(0,dishPivots.Length-1))):0;
        public bool Completed=>turning&&Progress>=1;
        public void BeginTurn()
        {
            if(turning)return;turning=true;start=Time.time;initial=new Quaternion[dishPivots.Length];bowlInitial=new Quaternion[dishPivots.Length];
            for(int i=0;i<dishPivots.Length;i++){initial[i]=dishPivots[i].localRotation;bowlInitial[i]=dishPivots[i].GetChild(0).localRotation;}
        }
        public void RestoreAligned()
        {
            BeginTurn();start=Time.time-duration-stagger*Mathf.Max(0,dishPivots.Length-1)-1;
            for(int i=0;i<dishPivots.Length;i++)
            {
                dishPivots[i].localRotation=Quaternion.Euler(0,26,0);
                dishPivots[i].GetChild(0).localRotation=Quaternion.Euler(38,0,0);
            }
        }
        void Update()
        {
            if(!turning)return;
            for(int i=0;i<dishPivots.Length;i++){
                float t=Mathf.SmoothStep(0,1,Mathf.Clamp01((Time.time-start-stagger*i)/duration));
                dishPivots[i].localRotation=Quaternion.Slerp(initial[i],Quaternion.Euler(0,26,0),t);
                dishPivots[i].GetChild(0).localRotation=Quaternion.Slerp(bowlInitial[i],Quaternion.Euler(38,0,0),t);
            }
        }
    }
}
