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
  "notes": "N1110. Played by S47Happenings after midnight, out on foot with the flashlight on: at the far end of the beam somebody stands on the land, the view snaps to it, and it is gone. The night goes quiet first (silence), and the code draws the body 3 s later (S47Hap::SilenceLead) and takes it away 1.7 s after that; this file says what the camera, the lamp and the notebook do. Point: figure (its chest). The camera_snap plays only after 02:00 (the code leaves it out earlier: silence and the figure alone). It stands 24 m out. Words: {clock} {day} {night}.",
  "enabled": true,
  "gates": { "tier": "glimpse", "hours": [0, 4.5], "if": ["outdoors", "on_foot"] },
  "beats": [
    { "at": 0, "do": "silence", "secs": 6, "level": 0.03 },
    { "at": 3.3, "do": "camera_snap", "point": "figure", "fov": 42, "in": 0.3, "hold": 1.0, "out": 0.8, "shake": 0.15, "scare": "soft" },
    { "at": 3.4, "do": "light", "kind": "lamp", "strength": 0.5, "secs": 1.6 },
    { "at": 3.4, "do": "light", "kind": "gear", "strength": 0.3, "secs": 2.5 },
    { "at": 5.4, "do": "thought", "text": "There was somebody standing out there.", "secs": 4 },
    { "at": 5.4, "do": "evidence", "key": "beam_figure_d{day}", "text": "At {clock}, out on foot, the flashlight picked out somebody standing dead still at the far end of the beam, twenty-some yards off. When I got the light back on the spot there was nobody." }
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
    { "at": 4.2, "do": "camera_snap", "point": "lights", "fov": 36, "in": 0.3, "hold": 0.9, "out": 0.9, "scare": "swell" },
    { "at": 7.0, "do": "thought", "text": "They were right over the dishes. And then they just went out.", "secs": 4.5 },
    { "at": 7.0, "do": "evidence", "key": "dish_lights_d{day}", "text": "At {clock} a cluster of lights hung over the array, dead still, for about five seconds. Then they went out all at once, like a switch." }
  ]
})S47JSON")},
 {TEXT("horizon_figure"),TEXT(R"S47JSON({
  "version": 1,
  "id": "horizon_figure",
  "name": "Somebody on the rise, a long way off",
  "notes": "N1110-5. Played by S47Happenings when the figure is put on the rise (out on foot, or at a control room window): the night's own sound drops and a low hum sits under it. The code draws the figure and decides whether it is seen (horizon_figure_gone says what then). The hour and the tier are the rhythm's (director.json) and the code's (a glimpse keeps farther off). Words: {clock} {day} {night}.",
  "enabled": true,
  "gates": {},
  "beats": [
    { "at": 0, "do": "silence", "secs": 7, "level": 0.05 },
    { "at": 1, "do": "tone", "hz": 52, "secs": 6, "gain": 0.03, "wave": "sine" },
    { "at": 2.5, "do": "light", "kind": "gear", "strength": 0.2, "secs": 3 }
  ]
})S47JSON")},
 {TEXT("horizon_figure_gone"),TEXT(R"S47JSON({
  "version": 1,
  "id": "horizon_figure_gone",
  "name": "The figure on the rise is gone",
  "notes": "N1110-5. Played by S47Happenings when the figure on the rise is gone, only if the player saw it. Deniable: a question, not a statement. The finding is the one the code kept before. Words: {dir} (the direction it stood in) {clock} {day} {night}.",
  "enabled": true,
  "gates": {},
  "beats": [
    { "at": 0, "do": "evidence", "key": "horizon_figure_d{day}", "text": "At {clock} a figure stood on the rise to the {dir}, a long way off, dead still. Then there was nobody on the rise." },
    { "at": 0.3, "do": "light", "kind": "gear", "strength": 0.25, "secs": 2.5 },
    { "at": 1.8, "do": "thought", "text": "Was that somebody on the rise? There is nobody there now.", "secs": 4.5 }
  ]
})S47JSON")},
 {TEXT("figure_near"),TEXT(R"S47JSON({
  "version": 1,
  "id": "figure_near",
  "name": "Somebody out there in the dark, following",
  "notes": "N1110-5. Played by S47Happenings when the figure that follows a walker begins (out on foot; a danger, one a night). The code's own warning (crickets, the lamp) stays; this adds the hum and a thought. Its stages call figure_near_steps, figure_near_close and figure_near_light, and its end figure_near_end. Words: {clock} {day} {night}.",
  "enabled": true,
  "gates": {},
  "beats": [
    { "at": 0, "do": "tone", "hz": 48, "secs": 8, "gain": 0.03, "delay": 2, "wave": "sine" },
    { "at": 6, "do": "thought", "text": "No crickets. No wind. Just my own boots.", "secs": 4, "if": ["on_foot"] }
  ]
})S47JSON")},
 {TEXT("figure_near_steps"),TEXT(R"S47JSON({
  "version": 1,
  "id": "figure_near_steps",
  "name": "Steps in the gravel, off to one side",
  "notes": "N1110-5. Played by S47Happenings when the figure stands off to one side and three steps are heard. Point: figure (where it stands). Words: {clock} {day} {night}.",
  "enabled": true,
  "gates": {},
  "beats": [
    { "at": 0, "do": "sound", "sound": "stepGravel2", "point": "figure", "gain": 0.35, "lp": 2400 },
    { "at": 0, "do": "sound", "sound": "stepGravel3", "point": "figure", "gain": 0.35, "lp": 2400, "delay": 0.55 },
    { "at": 0, "do": "sound", "sound": "stepGravel4", "point": "figure", "gain": 0.35, "lp": 2400, "delay": 1.1 },
    { "at": 0.2, "do": "light", "kind": "gear", "point": "figure", "strength": 0.3, "secs": 3 },
    { "at": 2.2, "do": "thought", "text": "Those were footsteps. They were not mine.", "secs": 4, "if": ["on_foot"] }
  ]
})S47JSON")},
 {TEXT("figure_near_close"),TEXT(R"S47JSON({
  "version": 1,
  "id": "figure_near_close",
  "name": "One step, close behind",
  "notes": "N1110-5. Played by S47Happenings when the figure has come right up behind the walker and one step is heard (the night's danger is met). Point: figure. The camera_snap plays only after 02:00 (the code leaves it out earlier: the step and the thought alone). Words: {clock} {day} {night}.",
  "enabled": true,
  "gates": {},
  "beats": [
    { "at": 0, "do": "sound", "sound": "stepGravel4", "point": "figure", "gain": 0.5, "lp": 2400 },
    { "at": 0.4, "do": "camera_snap", "point": "figure", "fov": 40, "in": 0.25, "hold": 0.9, "out": 0.9, "shake": 0.2, "scare": "soft" },
    { "at": 2.4, "do": "thought", "text": "Right behind me.", "secs": 3.5, "if": ["on_foot"] }
  ]
})S47JSON")},
 {TEXT("figure_near_light"),TEXT(R"S47JSON({
  "version": 1,
  "id": "figure_near_light",
  "name": "The light comes down where it stood",
  "notes": "N1110-5. Played by S47Happenings when the light comes down over where the figure stood. The code's own hush, boom and the lamp going out stay; this adds the sub tone, the gear and a thought. Point: figure. Words: {clock} {day} {night}.",
  "enabled": true,
  "gates": {},
  "beats": [
    { "at": 0, "do": "tone", "hz": 45, "secs": 6, "gain": 0.06, "wave": "sine" },
    { "at": 0, "do": "light", "kind": "gear", "point": "figure", "strength": 0.8, "secs": 6 },
    { "at": 0.3, "do": "shake", "strength": 0.2 },
    { "at": 1.2, "do": "thought", "text": "A light, coming down right where it stood. Move.", "secs": 4 }
  ]
})S47JSON")},
 {TEXT("figure_near_end"),TEXT(R"S47JSON({
  "version": 1,
  "id": "figure_near_end",
  "name": "What the walker kept of the figure",
  "notes": "N1110-5. Played by S47Happenings when the figure is over, only if the player met it. The code builds the sentences (what was seen, in order, then how it ended); the script keeps them. Words: {place} {seen} {end} {clock} {day} {night}.",
  "enabled": true,
  "gates": {},
  "beats": [
    { "at": 0, "do": "evidence", "key": "figure_near_d{day}", "text": "At {clock}, out on {place}: {seen}{end}" }
  ]
})S47JSON")},
 {TEXT("landing_warning"),TEXT(R"S47JSON({
  "version": 1,
  "id": "landing_warning",
  "name": "The night goes quiet before lights come down",
  "notes": "N1110-5. Played by S47Happenings 1.5 s into a landing (the crickets stop and a dog barks far off in the code; at 6 s the code makes the power falter). Silence before the sight, a low hum, a thought by where the player is, the gear answering and Dale on the walkie. Words: {clock} {day} {night}.",
  "enabled": true,
  "gates": {},
  "beats": [
    { "at": 0, "do": "tone", "hz": 48, "secs": 9, "gain": 0.03, "wave": "sine" },
    { "at": 2, "do": "thought", "text": "Every cricket out here just quit at once.", "secs": 4, "if": ["outdoors"] },
    { "at": 2, "do": "thought", "text": "The whole building has gone quiet, like it is holding its breath.", "secs": 4, "if": ["in_saro"] },
    { "at": 4.5, "do": "light", "kind": "gear", "strength": 0.5, "secs": 4 },
    { "at": 11, "do": "line", "who": "dale", "text": "You out there? The needles went flat on me a second ago. Over." }
  ]
})S47JSON")},
 {TEXT("landing_seen_log"),TEXT(R"S47JSON({
  "version": 1,
  "id": "landing_seen_log",
  "name": "Lights seen coming down",
  "notes": "N1110-5. Played by S47Happenings when lights that were seen have come down and gone out on the ground: the bearing goes on the map (the code), the finding is kept. Words: {az} {spread} {landing_night} {clock} {day} {night} {map_key}.",
  "enabled": true,
  "gates": {},
  "beats": [
    { "at": 0, "do": "evidence", "key": "landing_seen_d{landing_night}", "text": "At {clock} lights came down out on the land, about {az} degrees from SARO, give or take {spread}, and went out on the ground." },
    { "at": 0, "do": "toast", "text": "Bearing on your map ({map_key}).", "secs": 5 },
    { "at": 0, "do": "shake", "strength": 0.12 },
    { "at": 0.2, "do": "light", "kind": "gear", "strength": 0.6, "secs": 4 },
    { "at": 2.5, "do": "thought", "text": "Straight down, slow, and then out like a pulled plug.", "secs": 4.5 }
  ]
})S47JSON")},
 {TEXT("landing_logged"),TEXT(R"S47JSON({
  "version": 1,
  "id": "landing_logged",
  "name": "Lights the array logged",
  "notes": "N1110-5. Played by S47Happenings when lights nobody saw have come down and the array logged them. The beeps, the printer, the line on the screen and the thought are for one in the building only. Words: {az} {spread} {landing_night} {clock} {day} {night} {map_key}.",
  "enabled": true,
  "gates": {},
  "beats": [
    { "at": 0, "do": "tone", "hz": 1320, "secs": 0.06, "gain": 0.05, "wave": "square", "repeat": 4, "every": 0.18, "if": ["in_saro"] },
    { "at": 0, "do": "printer", "gain": 0.3, "delay": 0.9, "stop": 3, "if": ["in_saro"] },
    { "at": 0, "do": "toast", "text": "ARRAY / A bearing logged, on your map ({map_key}).", "secs": 5, "if": ["in_saro"] },
    { "at": 0, "do": "evidence", "key": "landing_logged_d{landing_night}", "text": "At {clock} the array logged something low on the horizon coming down, about {az} degrees from SARO, give or take {spread}." },
    { "at": 2.5, "do": "thought", "text": "Something just came down out there, and the array knows it.", "secs": 4.5, "if": ["in_saro"] }
  ]
})S47JSON")},
 {TEXT("car_light"),TEXT(R"S47JSON({
  "version": 1,
  "id": "car_light",
  "name": "A white light over the stalled truck",
  "notes": "N1110-5. Played by S47Happenings when the light stands over the truck on the road (a danger, one a night; the stall and the boom are the code's). Silence, a low hum, the gear answering, and two thoughts. Words: {clock} {day} {night}.",
  "enabled": true,
  "gates": {},
  "beats": [
    { "at": 0, "do": "silence", "secs": 14, "level": 0.03 },
    { "at": 0, "do": "tone", "hz": 44, "secs": 20, "gain": 0.05, "wave": "sine" },
    { "at": 0.5, "do": "light", "kind": "gear", "strength": 0.9, "secs": 10 },
    { "at": 3, "do": "thought", "text": "The engine is dead, and there is a white light sitting right over the cab.", "secs": 5 },
    { "at": 10, "do": "thought", "text": "Stay in the truck.", "secs": 3.5 }
  ]
})S47JSON")},
 {TEXT("car_light_end"),TEXT(R"S47JSON({
  "version": 1,
  "id": "car_light_end",
  "name": "The white light is gone from the road",
  "notes": "N1110-5. Played by S47Happenings when the light over the truck is over. The finding is the one the code kept before; {outcome} says what the player did, with its first space. Words: {outcome} {clock} {day} {night}.",
  "enabled": true,
  "gates": {},
  "beats": [
    { "at": 0, "do": "evidence", "key": "car_light_d{day}", "text": "At {clock} a white light stood over the truck on the road and the engine died under it.{outcome}" },
    { "at": 0.6, "do": "thought", "text": "The light is gone. It is just the road and the dark again.", "secs": 4.5 }
  ]
})S47JSON")},
 {TEXT("roof_steps"),TEXT(R"S47JSON({
  "version": 1,
  "id": "roof_steps",
  "name": "Slow steps on the roof",
  "notes": "N1110. Played by S47FreeformDirector when the steps begin above the station (night, SARO). The night goes still first, a low hum sits under it, and the gear answers faintly. The code keeps the steps themselves (it holds them back about 3.5 s so the quiet comes first), the report and the finding. The yard stage is roof_steps_yard, the end roof_steps_end. Words: {clock} {day} {night}.",
  "enabled": true,
  "gates": {},
  "beats": [
    { "at": 0, "do": "silence", "secs": 6, "level": 0.05 },
    { "at": 0.4, "do": "tone", "hz": 50, "secs": 5, "gain": 0.025, "wave": "sine" },
    { "at": 2.5, "do": "light", "kind": "gear", "strength": 0.2, "secs": 2 },
    { "at": 4, "do": "thought", "text": "Footsteps overhead. Slow ones.", "secs": 4, "if": ["indoors"] }
  ]
})S47JSON")},
 {TEXT("roof_steps_yard"),TEXT(R"S47JSON({
  "version": 1,
  "id": "roof_steps_yard",
  "name": "Out in the yard, the steps above go on",
  "notes": "N1110. Played by S47FreeformDirector once, when the player has gone out into the yard under the roof while the steps are still going. Point: roof (where they are). No view is taken from the player. Words: {clock} {day} {night}.",
  "enabled": true,
  "gates": {},
  "beats": [
    { "at": 0, "do": "silence", "secs": 5, "level": 0.04 },
    { "at": 0.4, "do": "sound", "sound": "stepConcrete0", "point": "roof", "gain": 0.4, "lp": 1800 },
    { "at": 1.2, "do": "light", "kind": "gear", "strength": 0.3, "secs": 2.5 },
    { "at": 1.6, "do": "sound", "sound": "stepConcrete0", "point": "roof", "gain": 0.35, "lp": 1800 },
    { "at": 2.9, "do": "sound", "sound": "stepConcrete0", "point": "roof", "gain": 0.3, "lp": 1800 },
    { "at": 3.8, "do": "thought", "text": "Somebody is walking up there. I do not see anybody.", "secs": 4, "if": ["outdoors"] }
  ]
})S47JSON")},
 {TEXT("roof_steps_end"),TEXT(R"S47JSON({
  "version": 1,
  "id": "roof_steps_end",
  "name": "The roof goes quiet",
  "notes": "N1110. Played by S47FreeformDirector when the steps stop, only if the player was outside or close enough to have heard them. The finding is the code's own (key roof_steps, unchanged). Words: {clock} {day} {night}.",
  "enabled": true,
  "gates": {},
  "beats": [
    { "at": 0.5, "do": "light", "kind": "gear", "strength": 0.15, "secs": 1.5 },
    { "at": 2, "do": "thought", "text": "Nothing is walking up there now.", "secs": 4 }
  ]
})S47JSON")},
 {TEXT("s03_failure"),TEXT(R"S47JSON({
  "version": 1,
  "id": "s03_failure",
  "name": "A dish drops off the array",
  "notes": "N1110. Played by S47FreeformDirector when the service fault takes a dish (SARO). The code picks the dish, sets the fault and keeps the repair, the work order and the finding (key s03_failure, written when it is mended). The night goes still, two relay clicks in the rack, the printer, the gear answers. Words: {clock} {day} {night}.",
  "enabled": true,
  "gates": {},
  "beats": [
    { "at": 0, "do": "silence", "secs": 4, "level": 0.05 },
    { "at": 0.8, "do": "sound", "sound": "rack_switch", "pos": [-3, 1, 1], "gain": 0.3 },
    { "at": 1.5, "do": "sound", "sound": "rack_switch", "pos": [-3, 1, 1], "gain": 0.22 },
    { "at": 1.6, "do": "light", "kind": "gear", "strength": 0.35, "secs": 2 },
    { "at": 2.2, "do": "printer", "gain": 0.25, "stop": 2 },
    { "at": 3.5, "do": "thought", "text": "That was a relay letting go. Something has dropped off the array.", "secs": 4.5, "if": ["indoors"] }
  ]
})S47JSON")},
 {TEXT("s03_failure_mended"),TEXT(R"S47JSON({
  "version": 1,
  "id": "s03_failure_mended",
  "name": "The dish is back on the array",
  "notes": "N1110. Played by S47FreeformDirector when the service fault has been mended. The finding is the code's own (key s03_failure). Words: {clock} {day} {night}.",
  "enabled": true,
  "gates": {},
  "beats": [
    { "at": 1, "do": "thought", "text": "The array is answering again.", "secs": 3.5, "if": ["in_saro"] }
  ]
})S47JSON")},
};
