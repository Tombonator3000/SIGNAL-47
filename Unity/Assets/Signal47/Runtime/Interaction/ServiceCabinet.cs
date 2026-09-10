using UnityEngine;
using Signal47.Core;
using Signal47.Investigation;

namespace Signal47.Interaction
{
    public sealed class ServiceCabinet : MonoBehaviour, IInteractable
    {
        public ServiceYardInvestigation investigation;
        public string Prompt => investigation.Completed ? "READ MOTOR BUS S-03" : "CHECK MOTOR BUS S-03";
        public void Interact(){investigation.InspectCabinet();}
    }
}
