# Hendelsesmanus (event scripts)

**Status:** Under arbeid. **Gjelder:** SIGNAL / 47, Unreal, Freeform (D46-11, kodemerke `D46-20261110`). **Sist kontrollert:** 10. november 2026, kildekode bare: ikke bygget eller prøvd i spillet.

Et hendelsesmanus er én JSON-fil per hendelse, `unreal/Signal47/Content/Director/Events/<id>.json`. Fila sier hva som skjer *inne i* en hendelse spillet allerede har: lyder, lys, linjer, tanker, funn og kartmerker i rekkefølge, og eventuelle porter som kan holde hendelsen tilbake. Ingen ny mekanikk: hvert slag kaller de samme funksjonene hendelsene brukte før.

## Hvem bestemmer hva

| Hva | Hvor | Datastyrt i dag? |
| --- | --- | --- |
| Hvilke hendelser, nivå, budsjett, fra dag, tier, timer, odds per natt, nedkjøling, behov | `Content/Director/director.json` (innebygd kopi `S47RhythmDefaults.inl`) | Ja |
| Om den kan starte der spilleren er (i bygget, i bilen, til fots), sted, peiling, lastebilen | Hendelsens kode (`S47Happenings.cpp`, `S47FreeformDirector.cpp`) | Nei |
| Slagene inne i hendelsen (lyd, lys, linje, funn) | Før: hardkodet. Nå: hendelsesmanuset for de tre som er koblet | Ja, for `array_burst`, `town_blackout`, `hood_warm` |
| Frittstående, egne hendelser med figurer, himmel og antenner | `Content/Director/scripted/*.json` (Regissørverktøyet, F10 «Director scripts») | Ja |
| F10-linjer | `S47DevFreeform.cpp`, `S47HappeningsDev.cpp`, `S47DevEvents.cpp`; nå også F10 «Event scripts» | – |

Koblet nå: `array_burst`, `town_blackout` og `hood_warm` (nattens hendelser i `S47Happenings`). `roof_steps` og `s03_failure` ligger i `S47FreeformDirector`, som en annen agent eier i dag. De kobles i en senere runde på samme måte: koden gir ord og punkter, og manuset står for slagene. Det samme gjelder resten av hendelsene.

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

Ord i tekst: `{clock}`, `{day}`, `{night}` og `{map_key}` finnes alltid. `array_burst` gir også `{az}`, `{spread}`, `{target}` og `{burst_night}`. `hood_warm` gir punktet `truck`. Et ord hendelsen ikke gir, blir stående som skrevet, og kontrollen advarer om det.

Betingelser: de samme som i regissørverktøyet (`finding`, `no_finding`, `case_open`, `case_closed`, `case_none`, `weather`, `moon_min`, `moon_max`, `moon_up`, `indoors`, `outdoors`, `in_truck`, `on_foot`, `near`, `far`) og i tillegg `in_saro` og `area` (`{ "if": "area", "is": "saro" }`). Kort form: `"indoors"`.

## Porter

`gates` er valgfrie og spørres før rytmen, aldri ved F10-start: `from_day`, `to_day`, `hours` [fra, til] (klokketimer, over midnatt når fra > til), `tier` (hint, glimpse, full: det nattens tier minst må tillate på den timen), `early`, `areas`, `odds` (0–1, fra spillets frø, likt hver gang samme natt) og `if`. Portene kan bare stramme inn det `director.json` og koden allerede tillater.

## Regler og kontroll

Spillertekst kontrolleres med regissørverktøyets kontroll (`S47Script::LintText`, spoilergrensene) og i tillegg mot ordet «taken». Et ukjent slag eller en ukjent betingelse avvises med navn. Selvtesten (`-S47SelfTest`, `S47EventScript::SelfTest`) kontrollerer innebygde kopier og alle filer, at de tre koblede manusene har de gamle slagene, rekkefølge, ord, kontroll og porter. Den sier fra med NOTE når en fil avviker fra den innebygde kopien.

F10 «Event scripts» → «Run: <id>» spiller et manus med en gang, med prøveord der koden ellers ville gitt sine. I et vanlig spill lagres ikke funn fra F10. I en utviklerkjøring lagres de.
