using UnityEngine;
using Signal47.Core;
namespace Signal47.Interaction
{
    public sealed class PhoneInteractable:MonoBehaviour,IInteractable
    {
        public string Prompt=>"DESK PHONE";
        public void Interact(){var d=GameSession.Instance.director;if(d.PhoneRinging)d.AnswerPhone();else GameSession.Instance.hud.Toast(d.PhoneAnswered?"No dial tone.":"Internal line. No call.");}
    }
}
