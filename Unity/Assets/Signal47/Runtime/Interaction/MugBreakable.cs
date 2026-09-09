using System.Collections;
using UnityEngine;
namespace Signal47.Interaction
{
    public sealed class MugBreakable:MonoBehaviour
    {
        public GameObject intact,broken; public IEnumerator DropAndBreak()
        {
            Vector3 start=intact.transform.localPosition;Quaternion rot=intact.transform.localRotation;float t=0;
            while(t<.7f){t+=Time.deltaTime;float n=Mathf.Clamp01(t/.7f);intact.transform.localPosition=start+new Vector3(0,-.99f*n*n,.35f*n);intact.transform.localRotation=rot*Quaternion.Euler(n*125,n*45,n*160);yield return null;}
            broken.transform.localPosition=start+new Vector3(0,-.99f,.35f);broken.transform.localRotation=Quaternion.identity;intact.SetActive(false);broken.SetActive(true);
        }
    }
}
