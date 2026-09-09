using System.Collections;
using UnityEngine;
namespace Signal47.Interaction
{
    public sealed class PrinterMechanism:MonoBehaviour
    {
        public Transform paper;
        public float duration=1.2f;
        public bool Printing{get;private set;}
        public bool Ready{get;private set;}
        Vector3 paperPosition;
        void Awake(){if(paper){paperPosition=paper.localPosition;paper.gameObject.SetActive(false);}}
        public IEnumerator Print()
        {
            if(Printing||Ready)yield break;
            Printing=true;
            if(paper)paper.gameObject.SetActive(true);
            float start=Time.time;
            while(Time.time-start<duration){if(paper)paper.localPosition=paperPosition+Vector3.forward*Mathf.Lerp(-.38f,0,(Time.time-start)/duration);yield return null;}
            if(paper)paper.localPosition=paperPosition;
            Printing=false;Ready=true;
        }
    }
}
