#if UNITY_EDITOR
using UnityEngine;
using UnityEditor;
using Signal47.Interaction;
using Signal47.Events;
namespace Signal47.Editor
{
    public static class StoryPass
    {
        public static void Dress(GameObject printer,GameObject phone,PrologueDirector director,Material paperMaterial)
        {
            // Unpack before regrouping the existing handset parts for animation.
            if(PrefabUtility.IsPartOfPrefabInstance(phone))PrefabUtility.UnpackPrefabInstance(phone,PrefabUnpackMode.Completely,InteractionMode.AutomatedAction);
            var handset=new GameObject("HandsetPivot").transform;handset.SetParent(phone.transform,false);
            foreach(var r in phone.GetComponentsInChildren<MeshRenderer>())if(r.name.Contains("Handset"))r.transform.SetParent(handset,true);
            phone.GetComponent<PhoneInteractable>().handset=handset;
            foreach(var r in printer.GetComponentsInChildren<MeshRenderer>())if((r.name.Contains("Paper")||r.name.Contains("Perforation")))r.enabled=false;
            var feed=new GameObject("PrintedEvidence");feed.transform.SetParent(printer.transform,false);feed.transform.localPosition=new Vector3(0,.518f,-.16f);
            var sheet=Signal47SceneBuilder.Cube("TractorFeedSheet",Vector3.zero,new Vector3(.58f,.004f,.48f),paperMaterial,false);sheet.transform.SetParent(feed.transform,false);
            var ink=new GameObject("PrintedText");ink.transform.SetParent(feed.transform,false);ink.transform.localPosition=new Vector3(0,.004f,0);ink.transform.localRotation=Quaternion.Euler(90,180,0);
            var text=ink.AddComponent<TextMesh>();text.text="SARO / RX 03\nDIRECTION SOLVE\n\n1420.405 MHz\nDISTANCE: -39 LY\n\nSOURCE UNKNOWN";text.fontSize=64;text.characterSize=.004f;text.anchor=TextAnchor.MiddleCenter;text.alignment=TextAlignment.Center;text.color=new Color(.12f,.14f,.11f);
            director.printer=printer.AddComponent<PrinterMechanism>();director.printer.paper=feed.transform;
        }
    }
}
#endif
