// Generated from ../events.json. Rebuild with Narrative/Freeform/generate.py.
static const TCHAR* FreeformEventsJson=TEXT(R"S47JSON({
  "version": 1,
  "events": [
    {"id":"extra_pulse","family":"instrument","day":1,"night":false,"duration":240,"action":"compare","finding":"An extra pulse group survives a separate receiver check. The source is unresolved."},
    {"id":"roof_steps","family":"minor","day":1,"night":true,"duration":45,"action":"field","finding":"Roof-area impacts were checked again from the service yard. Their source is unresolved."},
    {"id":"s03_failure","family":"maintenance","day":1,"night":false,"duration":900,"action":"service","finding":"S-03 was isolated, repaired and physically tested before returning to service."},
    {"id":"observer","family":"investigation","day":1,"night":true,"duration":100,"action":"photo","finding":"A moving observer used real cover near the array. Its photograph is retained without an origin claim."},
    {"id":"feeder_fault","family":"investigation","day":2,"night":false,"duration":900,"action":"feeder","finding":"A recorded feeder disturbance was checked with the affected circuit isolated."},
    {"id":"service_presence","family":"threat","day":2,"night":true,"duration":100,"action":"escape","finding":"A warned service-yard encounter ended with escape or physical shelter."},
    {"id":"wrong_source","family":"instrument","day":2,"night":false,"duration":240,"action":"compare","finding":"The extra component does not follow the expected sky-source direction. Local origin remains a possibility."},
    {"id":"unsigned_telex","family":"investigation","day":3,"night":false,"duration":240,"action":"telex","finding":"An unsigned telex references a previously logged observation. Its line and time differ from the reference."},
    {"id":"empty_car","family":"minor","day":2,"night":true,"duration":180,"action":"field","finding":"An unoccupied vehicle was inspected. The warm bonnet and clock reading are logged; absence of a driver proves nothing else."},
    {"id":"response_lights","family":"investigation","day":2,"night":true,"duration":100,"action":"control","finding":"The distant lights were compared with a controlled receiver test rather than their first apparent resemblance."},
    {"id":"landing_trace","family":"investigation","day":3,"night":false,"duration":300,"action":"sample","finding":"A marked surface and loose fragment were photographed and inspected in place before sampling."},
    {"id":"time_disagreement","family":"instrument","day":2,"night":false,"duration":240,"action":"compare","finding":"Two records disagree on event time. The independent internal reference excludes one timing path."},
    {"id":"duplicate_print","family":"instrument","day":3,"night":false,"duration":240,"action":"compare","finding":"An additional print was compared with the retained original and spool log. The original remains unchanged."},
    {"id":"flyover","family":"minor","day":2,"night":true,"duration":36,"action":"photo","finding":"A moving light and spatial passage were recorded independently of the receiver's stored observation."},
    {"id":"quiet_zone","family":"investigation","day":3,"night":true,"duration":150,"action":"field","finding":"The apparent quiet region was checked from both sides of a recorded field boundary."},
    {"id":"close_pass","family":"major","day":4,"night":true,"duration":65,"action":"photo","finding":"A close light-bearing passage produced independent visual and instrument records. No identity is assigned."}
  ]
}
)S47JSON");
