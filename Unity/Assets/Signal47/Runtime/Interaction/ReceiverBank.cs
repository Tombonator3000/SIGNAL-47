using UnityEngine;
using Signal47.Core;
namespace Signal47.Interaction
{
    public sealed class ReceiverBank : MonoBehaviour,IInteractable
    {
        public Renderer[] leds; public string Prompt=>"RECEIVER BANK 3";
        public void Interact(){var d=GameSession.Instance.director;if(d.ReceiverPowered){GameSession.Instance.hud.Toast("Receiver bank already online.");return;}d.PowerReceiver();foreach(var r in leds){var m=r.material;m.SetColor("_BaseColor",new Color(.1f,1f,.25f));if(m.HasProperty("_EmissionColor")){m.EnableKeyword("_EMISSION");m.SetColor("_EmissionColor",new Color(.1f,1f,.2f)*2);}}}
    }
}
