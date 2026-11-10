# Hendelsesmanus (event scripts)

**Status:** Under arbeid. **Gjelder:** SIGNAL / 47, Unreal, Freeform (D46-11, kodemerke `D46-20261110`; kamera-slaget og de tre kameramomentene N1110-3, kodemerke `N1110`). **Sist kontrollert:** 10. november 2026, kildekode bare: ikke bygget eller prøvd i spillet.

Et hendelsesmanus er én JSON-fil per hendelse, `unreal/Signal47/Content/Director/Events/<id>.json`. Fila sier hva som skjer *inne i* en hendelse spillet allerede har: lyder, lys, linjer, tanker, funn og kartmerker i rekkefølge, og eventuelle porter som kan holde hendelsen tilbake. Ingen ny mekanikk: hvert slag kaller de samme funksjonene hendelsene brukte før.

## Hvem bestemmer hva

| Hva | Hvor | Datastyrt i dag? |
| --- | --- | --- |
| Hvilke hendelser, nivå, budsjett, fra dag, tier, timer, odds per natt, nedkjøling, behov | `Content/Director/director.json` (innebygd kopi `S47RhythmDefaults.inl`) | Ja |
| Om den kan starte der spilleren er (i bygget, i bilen, til fots), sted, peiling, lastebilen | Hendelsens kode (`S47Happenings.cpp`, `S47FreeformDirector.cpp`) | Nei |
| Slagene inne i hendelsen (lyd, lys, linje, funn, kamera) | Før: hardkodet. Nå: hendelsesmanuset for de seks som er koblet | Ja, for `array_burst`, `town_blackout`, `hood_warm`, `eyes_window`, `beam_figure`, `dish_lights` |
| Frittstående, egne hendelser med figurer, himmel og antenner | `Content/Director/scripted/*.json` (Regissørverktøyet, F10 «Director scripts») | Ja |
| F10-linjer | `S47DevFreeform.cpp`, `S47HappeningsDev.cpp`, `S47DevEvents.cpp`; nå også F10 «Event scripts» | – |

Koblet nå: `array_burst`, `town_blackout` og `hood_warm` (nattens hendelser i `S47Happenings`), og fra N1110 de tre kameramomentene `eyes_window`, `beam_figure` og `dish_lights` (se eget avsnitt). `roof_steps` og `s03_failure` ligger i `S47FreeformDirector`, som en annen agent eier i dag. De kobles i en senere runde på samme måte: koden gir ord og punkter, og manuset står for slagene. Det samme gjelder resten av hendelsene.

## Slik leses det

1. Innebygde kopier av de koblede manusene (`S47EventScriptDefaults.inl`, samme tekst som filene).
2. `Content/Director/Events/*.json` (pakkes med spillet via `Signal47.Build.cs`).
3. `Saved/Director/Events/*.json` (egne kopier i et installert spill).
4. `-S47EventDir=<mappe>`.

Senere kilder vinner. En fil som ikke kan leses, eller som stoppes av kontrollen, blir avvist og logget. Da spilles den forrige kopien. `"enabled": false` i en koblet hendelse betyr at de innebygde slagene spilles. F10 «Event scripts» → «Read the event scripts again» leser alt på nytt uten omstart.

## Eksempel

```json
{
  "version": 1,
  "id": "array_burst",
  "name": "The array catches a burst off the main beam",
  "enabled": true,
  "gates": { "from_day": 1, "hours": ["21:30", "04:00"], "tier": "hint", "areas": ["saro"], "odds": 1, "if": ["indoors"] },
  "beats": [
    { "at": 0, "do": "map", "place": "{target}" },
    { "at": 0, "do": "tone", "hz": 1400, "secs": 0.05, "gain": 0.06, "wave": "square", "repeat": 6, "every": 0.12 },
    { "at": 0, "do": "printer", "gain": 0.3, "delay": 1, "stop": 3 },
    { "at": 0, "do": "evidence", "key": "array_burst_d{burst_night}", "text": "At {clock} the array caught a short burst ..." },
    { "at": 0.8, "do": "toast", "text": "ARRAY / Off the main beam: about {az} degrees. On your map ({map_key}).", "secs": 5 },
    { "at": 3, "do": "thought", "text": "What was that?", "if": [{ "if": "weather", "is": "clear" }] },
    { "at": 6, "do": "line", "who": "dale", "text": "You get that on the printer? Over.", "wait": ["in_saro"], "max": 60 }
  ]
}
```

(Fila som følger med, har tomme porter og bare de fem første slagene. Det er akkurat det koden gjorde før.) Alle slag finnes i `example_every_beat.json`, som er slått av og bare kjøres fra F10.

## Slag

Felles for alle slag: `at` (sekunder fra start), `if` (betingelser som må holde når tiden kommer, ellers hoppes slaget over), `wait` med `max` (slaget venter på betingelsene i høyst `max` sekunder og holder slagene etter seg imens; ellers hoppes det over). Posisjon gis som `pos` [x, y, z] (scenemeter i området spilleren er i) eller `point` (et navngitt punkt fra koden, for eksempel `truck`, eller `here`).

| `do` | Felt | Kaller |
| --- | --- | --- |
| `sound` | `sound` (opptakets navn), `pos`/`point`, `who` (radio, cb, walkie), `gain`, `rate`, `lp`, `delay`, `stop` | `US47Audio::Sfx` |
| `tone` | `hz`, `secs`, `gain`, `wave` (square, sine, sawtooth, triangle), `delay`, `repeat`, `every` | `US47Audio::Beep` |
| `printer` | `gain` (0,3), `delay`, `stop`, `text` (linja på skjermen) | skriveren ved racket |
| `light` | `kind`: `town` (gatelysene i Caliche slukner ett etter ett), `lamp` (lommelykta flimrer `secs`), `gear` (lykt, walkie, bil og skjermer svarer på et tegn med `strength` i `secs`) | `FS47Town::LampWave`, `FS47Flashlight::Flicker`, `FS47Omen::Post` |
| `tv` | `pos`/`point`; med `strength` og `secs` brytes skjermbildet opp | `US47Audio::TvHiss`, `FS47Omen::Post` |
| `line` | `who`: dale, voice, cb, radio (1420 AM); `text` | `FS47FieldRadio`, `FS47Station::CutIn` |
| `thought` | `text`, `secs` | `US47UI::Thought` |
| `toast` | `text`, `secs` | `US47UI::Toast` |
| `evidence` | `key`, `text` | `FS47FreeformSession::RecordEvidence` |
| `map` | `place` | `FS47Places::Hear` |
| `prop` | `node` (scene-nøkkel, `room.objs.mug`), `by` [x, y, z] eller `to: true` | `AS47Web::SetPosition` (lagres ikke) |
| `show` / `hide` | `node` | `AS47Web::SetVisible` (lagres ikke) |
| `shake` | `strength` 0–1 | gåerens egen risting |
| `camera_snap` | `pos`/`point` (det som ses på), `fov` (grader, 20–90, 45 som standard), `in`/`hold`/`out` (sekunder: snapp mot, hold, tilbake; 0,25/1,2/0,9 som standard, til sammen 0,3–5 s), `shake` 0–1 (0 som standard), `scare` (soft, hard, braam, swell: et kjøpt skrekkslag samtidig) | `FS47CameraMoment` (N1110), `FS47FieldRadio::Scare` |

Ord i tekst: `{clock}`, `{day}`, `{night}` og `{map_key}` finnes alltid. `array_burst` gir også `{az}`, `{spread}`, `{target}` og `{burst_night}`. `hood_warm` gir punktet `truck`; `eyes_window` gir `eyes`, `beam_figure` gir `figure` og `dish_lights` gir `lights`. Et ord hendelsen ikke gir, blir stående som skrevet, og kontrollen advarer om det.

## Kameramomenter (N1110-3)

`camera_snap` tar synsfeltet i 0,6–2,5 sekunder: utsynet snapper mot punktet og trekkes inn til `fov` grader over `in` sekunder, holder `hold` og glir tilbake over `out`. Spilleren går, ser og plukker som før under laget (`AS47Walker::ViewLayer`); bare det som tegnes tas. Ikke i bilen eller når noe annet holder kameraet. Innstillingen **Camera moments** (Settings, lagret som `cameraMoments`) slår snappen av; lyden og funnet kommer likevel. Med **Head motion and bumps** av er snappen redusert: halv zoom, ingen risting, lengre vei inn. Annen kode kan be om det samme gjennom `S47EventScript::CameraSnap` (oppmerksomhetssystemets krok).

Tre hendelser bruker det, hver sjelden og høyst én gang per natt (rytmen i `director.json`, pluss manusets porter):

| Hendelse | Når og hvor (koden) | Manuset |
| --- | --- | --- |
| `eyes_window` | etter 02, inne i kontrollrommet, ved vinduet spilleren ser ut av: to store mørke øyne med en våt glans rett utenfor glasset (prop `eyes`, borte etter 1,9 s) | snapp til 38°, hold 1,2 s, `hard`, litt risting; utstyret svarer; tanke og funn |
| `beam_figure` | etter midnatt, ute til fots med lykta på: en skikkelse ved enden av lysstrålen, 34 m unna (nattens figur, borte etter 1,7 s) | snapp til 42°, hold 1 s, `soft`; lykta flimrer; tanke og funn |
| `dish_lights` | etter 01 ved SARO, ute eller ved et vindu mot antennene: lys henger stille over fatene i fem sekunder og slukner (prop `lander`, borte ved 5,45 s) | brum, utstyret svarer, snapp til 36° ved 5 s, `swell`; tanke og funn |

Selvtesten kontrollerer kurven, se-mot-regningen og redusert bevegelse (`S47CameraMoment::SelfTest`) og at de tre manusene har ett `camera_snap` ved sitt punkt, 0,6–2,5 s, 30–45°, med slag, funn og porter.

Betingelser: de samme som i regissørverktøyet (`finding`, `no_finding`, `case_open`, `case_closed`, `case_none`, `weather`, `moon_min`, `moon_max`, `moon_up`, `indoors`, `outdoors`, `in_truck`, `on_foot`, `near`, `far`) og i tillegg `in_saro` og `area` (`{ "if": "area", "is": "saro" }`). Kort form: `"indoors"`.

## Porter

`gates` er valgfrie og spørres før rytmen, aldri ved F10-start: `from_day`, `to_day`, `hours` [fra, til] (klokketimer, over midnatt når fra > til), `tier` (hint, glimpse, full: det nattens tier minst må tillate på den timen), `early`, `areas`, `odds` (0–1, fra spillets frø, likt hver gang samme natt) og `if`. Portene kan bare stramme inn det `director.json` og koden allerede tillater.

## Regler og kontroll

Spillertekst kontrolleres med regissørverktøyets kontroll (`S47Script::LintText`, spoilergrensene) og i tillegg mot ordet «taken». Et ukjent slag eller en ukjent betingelse avvises med navn. Selvtesten (`-S47SelfTest`, `S47EventScript::SelfTest`) kontrollerer innebygde kopier og alle filer, at de tre koblede manusene har de gamle slagene, rekkefølge, ord, kontroll og porter. Den sier fra med NOTE når en fil avviker fra den innebygde kopien.

F10 «Event scripts» → «Run: <id>» spiller et manus med en gang, med prøveord der koden ellers ville gitt sine. I et vanlig spill lagres ikke funn fra F10. I en utviklerkjøring lagres de.
