using UnityEngine;
using Signal47.Core;

namespace Signal47.Investigation
{
    public sealed class ServiceYardInvestigation : MonoBehaviour
    {
        public bool Active { get; private set; }
        public bool Completed { get; private set; }
        public bool Returned { get; private set; }
        public Signal47.Interaction.ServiceYardDoor door;
        public string Objective => GameSession.Instance.fieldCamera && (!GameSession.Instance.fieldCamera.Acquired || Completed) ? GameSession.Instance.fieldCamera.Objective : Returned ? "S-03 INVESTIGATION FILED // END OF CURRENT SLICE" : Completed ? "S-03 LOG FILED // RETURN TO THE CONTROL ROOM" : "CHECK MOTOR BUS S-03 // EAST SERVICE YARD";
        const string Log = "SIERRA ARRAY / MOTOR BUS S-03\nLOCAL CONTROLLER LOG // 23:44\n\nENCODER HEADING: 026 DEGREES\nSCHEDULED HEADING: 042 DEGREES\n\nCOMMANDS RECEIVED: 0\nLOCAL OVERRIDE: NONE\n\nThe antenna moved. The controller did not send a command.\n\nA thin line of condensation runs across the warm inspection glass.";
        void Update()
        {
            if(!Completed || Returned)return;var g=GameSession.Instance;var p=g.player.transform.position;
            if(p.x<8.9f && p.x>-9 && p.z>-7 && p.z<7)
            {
                Returned=true;g.notebook.Add("Returned to the control room with the S-03 motor log. The unauthorized alignment is documented.");
                g.hud.Toast("S-03 INVESTIGATION FILED",3);
            }
        }
        public void Begin()
        {
            var g=GameSession.Instance;
            if(Active || !g.director.SignalAcquired)return;
            Active=true;
            g.notebook.Add("The array left its track. Check the local motor controller in the east service yard.");
            g.hud.Toast("SERVICE ACCESS RELEASED // EAST DOOR",3f);
        }
        public void InspectCabinet()
        {
            var g=GameSession.Instance;
            if(!Active){g.hud.Toast("The service inspection is not active.");return;}
            if(!Completed)
            {
                Completed=true;
                g.notebook.Collect("motor-bus-s03","S-03 / No command recorded",Log);
                g.notebook.Add("S-03 registered a physical heading change without a motor command.");
            }
            g.hud.ShowPaper(Log);
        }
    }
}
