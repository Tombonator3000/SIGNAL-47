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
 {TEXT("eyes_window"),TEXT(R"S47JSON({
  "version": 1,
  "id": "eyes_window",
  "name": "Eyes at the window",
  "notes": "N1110. Played by S47Happenings after two in the morning, inside SARO's control room, at the window the player is looking out of: a pair of big dark eyes with a wet glint are outside the glass for a moment. The night goes quiet first (silence), and the code draws them 3 s later (S47Hap::SilenceLead) and takes them away 1.9 s after that; this file says what the camera, the gear and the notebook do. Point: eyes (just outside the pane, at a face's height). Words: {clock} {day} {night}.",
  "enabled": true,
  "gates": { "tier": "full", "hours": [2, 5], "if": ["in_saro"] },
  "beats": [
    { "at": 0, "do": "silence", "secs": 6, "level": 0.03 },
    { "at": 3.2, "do": "camera_snap", "point": "eyes", "fov": 38, "in": 0.2, "hold": 1.2, "out": 0.9, "shake": 0.25, "scare": "hard" },
    { "at": 3.3, "do": "light", "kind": "gear", "strength": 0.5, "secs": 4 },
    { "at": 5.6, "do": "thought", "text": "Something was right there. Right at the glass.", "secs": 4.5 },
    { "at": 5.6, "do": "evidence", "key": "eyes_window_d{day}", "text": "At {clock} something looked in through the control room window. Two big dark eyes, wet in the light, a hand's width from the glass. Then nothing out there but the night." }
  ]
})S47JSON")},
 {TEXT("beam_figure"),TEXT(R"S47JSON({
  "version": 1,
  "id": "beam_figure",
  "name": "A figure at the end of the flashlight beam",
  "notes": "N1110. Played by S47Happenings after midnight, out on foot with the flashlight on: at the far end of the beam somebody stands on the land, the view snaps to it, and it is gone. The night goes quiet first (silence), and the code draws the body 3 s later (S47Hap::SilenceLead) and takes it away 1.7 s after that; this file says what the camera, the lamp and the notebook do. Point: figure (its chest). Words: {clock} {day} {night}.",
  "enabled": true,
  "gates": { "tier": "glimpse", "hours": [0, 4.5], "if": ["outdoors", "on_foot"] },
  "beats": [
    { "at": 0, "do": "silence", "secs": 6, "level": 0.03 },
    { "at": 3.3, "do": "camera_snap", "point": "figure", "fov": 42, "in": 0.22, "hold": 1.0, "out": 0.8, "shake": 0.15, "scare": "soft" },
    { "at": 3.4, "do": "light", "kind": "lamp", "strength": 0.5, "secs": 1.6 },
    { "at": 3.4, "do": "light", "kind": "gear", "strength": 0.3, "secs": 2.5 },
    { "at": 5.4, "do": "thought", "text": "There was somebody standing out there.", "secs": 4 },
    { "at": 5.4, "do": "evidence", "key": "beam_figure_d{day}", "text": "At {clock}, out on foot, the flashlight picked out somebody standing dead still at the far end of the beam, maybe thirty yards off. When I got the light back on the spot there was nobody." }
  ]
})S47JSON")},
 {TEXT("dish_lights"),TEXT(R"S47JSON({
  "version": 1,
  "id": "dish_lights",
  "name": "Lights over the dishes",
  "notes": "N1110. Played by S47Happenings after one in the morning at SARO, outdoors or at a control room window toward the array: the night goes quiet (silence) as a cluster of lights hangs dead still over the dishes for five seconds, the view snaps up to them, and they go out. The code draws them and takes them away at 5.45 s; this file says what is heard, what the camera does and what the notebook keeps. Point: lights. Words: {clock} {day} {night}.",
  "enabled": true,
  "gates": { "tier": "glimpse", "hours": [1, 5], "areas": ["saro"] },
  "beats": [
    { "at": 0, "do": "silence", "secs": 7, "level": 0.03 },
    { "at": 0, "do": "tone", "hz": 58, "secs": 5.6, "gain": 0.04, "wave": "sine" },
    { "at": 0.6, "do": "light", "kind": "gear", "strength": 0.4, "secs": 6 },
    { "at": 5.0, "do": "camera_snap", "point": "lights", "fov": 36, "in": 0.3, "hold": 0.9, "out": 0.9, "scare": "swell" },
    { "at": 7.0, "do": "thought", "text": "They were right over the dishes. And then they just went out.", "secs": 4.5 },
    { "at": 7.0, "do": "evidence", "key": "dish_lights_d{day}", "text": "At {clock} a cluster of lights hung over the array, dead still, for about five seconds. Then they went out all at once, like a switch." }
  ]
})S47JSON")},
};
