// D46-20261110: the wired events' scripts, built in: the same text as Content/Director/Events/<id>.json when they were written
// (S47EventScript.h). The file wins when it is there and clean; the self-test says when the two differ.
static const TCHAR* const S47EventScriptBuiltIn[][2]={
 {TEXT("array_burst"),TEXT(R"S47JSON({
  "version": 1,
  "id": "array_burst",
  "name": "The array catches a burst off the main beam",
  "notes": "Played by S47Happenings when the burst starts. The code picks the place and the bearing; this file says what the player hears, sees and keeps. Words: {az} {spread} {target} {burst_night} {map_key} {clock} {day} {night}.",
  "enabled": true,
  "gates": {},
  "beats": [
    { "at": 0, "do": "map", "place": "{target}" },
    { "at": 0, "do": "tone", "hz": 1400, "secs": 0.05, "gain": 0.06, "wave": "square", "repeat": 6, "every": 0.12 },
    { "at": 0, "do": "printer", "gain": 0.3, "delay": 1, "stop": 3 },
    { "at": 0, "do": "evidence", "key": "array_burst_d{burst_night}", "text": "At {clock} the array caught a short burst off the main beam, about {az} degrees from SARO, give or take {spread}. The printer ran a line by itself." },
    { "at": 0.8, "do": "toast", "text": "ARRAY / Off the main beam: about {az} degrees. On your map ({map_key}).", "secs": 5 }
  ]
})S47JSON")},
 {TEXT("town_blackout"),TEXT(R"S47JSON({
  "version": 1,
  "id": "town_blackout",
  "name": "Every light in Caliche goes out",
  "notes": "Played by S47Happenings when the blackout starts in Caliche. Words: {clock} {day} {night}.",
  "enabled": true,
  "gates": {},
  "beats": [
    { "at": 0, "do": "light", "kind": "town" },
    { "at": 0, "do": "tone", "hz": 60, "secs": 40, "gain": 0.05, "delay": 1, "wave": "sine" },
    { "at": 0, "do": "tone", "hz": 120, "secs": 40, "gain": 0.025, "delay": 1, "wave": "square" },
    { "at": 0, "do": "evidence", "key": "blackout_d{day}", "text": "At {clock} every light in Caliche went out, one after another down the street, and the transformer hummed loud enough to hear across town. Then they came back." }
  ]
})S47JSON")},
 {TEXT("hood_warm"),TEXT(R"S47JSON({
  "version": 1,
  "id": "hood_warm",
  "name": "The parked truck's hood is warm",
  "notes": "Played by S47Happenings when the player comes back to the truck after a long while away. Point: truck (a metre over where it stands). Words: {clock} {day} {night}.",
  "enabled": true,
  "gates": {},
  "beats": [
    { "at": 0, "do": "sound", "sound": "hap:tick", "point": "truck", "gain": 0.55 },
    { "at": 0, "do": "sound", "sound": "hap:tick", "point": "truck", "gain": 0.4, "rate": 0.9, "delay": 2.6 },
    { "at": 0, "do": "evidence", "key": "hood_warm_d{day}", "text": "At {clock} the truck had been sitting a good while, and its engine was ticking as it cooled when I came back to it. The hood was warm to the hand." }
  ]
})S47JSON")},
};
