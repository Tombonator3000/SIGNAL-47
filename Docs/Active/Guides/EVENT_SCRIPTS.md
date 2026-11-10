# Hendelsesmanus (event scripts)

**Status:** Under arbeid. **Gjelder:** SIGNAL / 47, Unreal, Freeform (D46-11, kodemerke `D46-20261110`; kamera-slaget og de tre kameramomentene N1110-3, og fire hendelser i flere slag N1110-5, kodemerke `N1110`). **Sist kontrollert:** 10. november 2026, kildekode bare: ikke bygget eller prøvd i spillet.

Et hendelsesmanus er én JSON-fil per hendelse, `unreal/Signal47/Content/Director/Events/<id>.json`. Fila sier hva som skjer *inne i* en hendelse spillet allerede har: lyder, lys, linjer, tanker, funn og kartmerker i rekkefølge, og eventuelle porter som kan holde hendelsen tilbake. Ingen ny mekanikk: hvert slag kaller de samme funksjonene hendelsene brukte før.

## Hvem bestemmer hva

| Hva | Hvor | Datastyrt i dag? |
| --- | --- | --- |
| Hvilke hendelser, nivå, budsjett, fra dag, tier, timer, odds per natt, nedkjøling, behov | `Content/Director/director.json` (innebygd kopi `S47RhythmDefaults.inl`) | Ja |
| Om den kan starte der spilleren er (i bygget, i bilen, til fots), sted, peiling, lastebilen | Hendelsens kode (`S47Happenings.cpp`, `S47FreeformDirector.cpp`) | Nei |
| Slagene inne i hendelsen (lyd, lys, linje, funn, kamera) | Før: hardkodet. Nå: hendelsesmanuset for de som er koblet | Ja, for `array_burst`, `town_blackout`, `hood_warm`, `eyes_window`, `beam_figure`, `dish_lights` og (N1110-5) `horizon_figure`, `figure_near`, `landing`, `car_light`, hver i flere manus, ett per trinn |
| Frittstående, egne hendelser med figurer, himmel og antenner | `Content/Director/scripted/*.json` (Regissørverktøyet, F10 «Director scripts») | Ja |
| F10-linjer | `S47DevFreeform.cpp`, `S47HappeningsDev.cpp`, `S47DevEvents.cpp`; nå også F10 «Event scripts» | – |

Koblet nå: `array_burst`, `town_blackout` og `hood_warm` (nattens hendelser i `S47Happenings`), og fra N1110 de tre kameramomentene `eyes_window`, `beam_figure` og `dish_lights` (se eget avsnitt). Fire av de skumleste hendelsene har fått flere slag (avsnittet «Fire hendelser i flere trinn», N1110-5). `roof_steps` og `s03_failure` ligger i `S47FreeformDirector`, som en annen agent eier i dag. De kobles i en senere runde på samme måte: koden gir ord og punkter, og manuset står for slagene. Det samme gjelder resten av hendelsene.

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
| `silence` | `secs` (0,5–30), `level` (hva som er igjen av nattens lyd, 0,04 som standard) | `FS47Ambience::Hush` (N1110, SOUNDSCAPE): sirisser og vind stopper brått og kommer sakte tilbake |
| `tube` | `tube` (2–7; kontrollrommets takrør, 0 og 1 er arbeidslyset og røres aldri) | `FS47FreeformSession::BreakTube` (N1110, PLAY-31-34): røret knitrer og er av for godt i dette spillet; lagres (`brokenTubes`), holder gjennom strømretur, Continue og områdebytte; F10 knitrer bare |
| `camera_snap` | `pos`/`point` (det som ses på), `fov` (grader, 20–90, 45 som standard), `in`/`hold`/`out` (sekunder: snapp mot, hold, tilbake; 0,25/1,2/0,9 som standard, til sammen 0,3–5 s), `shake` 0–1 (0 som standard), `scare` (soft, hard, braam, swell: et kjøpt skrekkslag samtidig) | `FS47CameraMoment` (N1110), `FS47FieldRadio::Scare` |

Ord i tekst (N1110-5: de nye trinnmanusene gir ordene i tabellene under): `{clock}`, `{day}`, `{night}` og `{map_key}` finnes alltid. `array_burst` gir også `{az}`, `{spread}`, `{target}` og `{burst_night}`. `hood_warm` gir punktet `truck`; `eyes_window` gir `eyes`, `beam_figure` gir `figure` og `dish_lights` gir `lights`. Et ord hendelsen ikke gir, blir stående som skrevet, og kontrollen advarer om det.

## Fire hendelser i flere trinn (N1110-5)

Hendelsene `horizon_figure`, `figure_near`, `landing` og `car_light` (alle i `S47Happenings`) har fått sine slag flyttet ut i manuslaget. Koden beholder det den alene vet (sted, retning, hvem som ser, når et trinn er nådd, porter, regler, bokføring, kartmerket, lommelyktas egen slukking og knallen) og kaller et manus på hvert trinn. Spilles ikke manuset (ingen løper, for mange manus på gang), gjør koden det den gjorde før. Funnene har samme nøkler og samme tekst som før (ledgeren og samme-minutt-kontrollen leser dem). Porter står tomme i alle tolv: rytmen i `director.json`, nattens tier og koden bestemmer fortsatt når og hvor; trinnmanusene har ingen egne porter. Kamera snapper kun ett sted (`figure_near_close`), og bare etter 02:00 (koden slår snappen av før det, som for `beam_figure`). Én fare per natt gjelder som før (`figure_near` og `car_light` er farer i koden).

| Hendelse | Manus (kalles når) | Slagene | Ord og punkter |
| --- | --- | --- | --- |
| `horizon_figure` | `horizon_figure` (figuren settes på høyden) | 0 s stillhet 7 s; 1 s brum 52 Hz; 2,5 s utstyret svarer svakt | – |
| | `horizon_figure_gone` (borte, og bare hvis den ble sett) | funn; 0,3 s utstyr; 1,8 s tanke som spørsmål | `{dir}` |
| `figure_near` (fare) | `figure_near` (den begynner) | 0 s ingenting; 2 s brum 48 Hz; 6 s tanke «ingen sirisser» til fots | – |
| | `figure_near_steps` (står til siden; tre skritt) | tre skritt i grusen med 0,55 s mellom, utstyret svarer, tanke etter 2,2 s | punkt `figure` |
| | `figure_near_close` (rett bak; ett skritt) | ett skritt, 0,4 s `camera_snap` (myk, bare etter 02:00), tanke etter 2,4 s | punkt `figure` |
| | `figure_near_light` (lyset kommer ned) | brum 45 Hz, utstyret svarer sterkt, risting, tanke | punkt `figure` |
| | `figure_near_end` (over, og bare hvis den ble møtt) | funnet | `{place}` `{seen}` `{end}` |
| `landing` | `landing_warning` (1,5 s inn: sirisser og hund i koden) | brum; tanke ute eller på SARO; 4,5 s utstyret svarer; 8 s Dale på walkien | – |
| | `landing_seen_log` (sett, og ned) | funn, linje på skjermen, risting, utstyret svarer, tanke | `{az}` `{spread}` `{landing_night}` |
| | `landing_logged` (bare arrayet så det) | fire pip, skriveren fra 0,9 s, linje og tanke for den som er i bygget; funn | `{az}` `{spread}` `{landing_night}` |
| `car_light` (fare) | `car_light` (lyset over bilen; stopp og knall i koden) | stillhet 14 s, brum 44 Hz, utstyret svarer, to tanker | – |
| | `car_light_end` (borte) | funn med utfall; tanke | `{outcome}` |

Et forced landing (F10) som ingen så bruker den gamle koden for pip og funn. Ordene er korte setninger koden bygger: `{seen}` er det som ble sett i rekkefølge (eller skrittene når man ikke så noe), `{end}` hvordan det endte (med mellomrom først), `{outcome}` hva spilleren gjorde (med mellomrom først). F10 «Event scripts» → «Run: <id>» spiller hvert trinn med prøveord. Selvtesten kontrollerer funnnøklene og tekstene, stillheten først, de tre skrittene, snappen (ett sted), pip og skriver og at ingen har egne porter.

## Stillhet før, flimmer under (N1110)

Regelen for de store hendelsene: nattens lyd stopper først (`silence`), så kommer synet, og utstyret svarer samtidig (`light` med `gear` eller `lamp`, 1–3 s). `eyes_window` og `beam_figure` venter `S47Hap::SilenceLead` (3 s) med å tegne øynene og skikkelsen, slik at stillheten kommer først; manusets slag ligger 3 s senere. `dish_lights` har stillheten over de fem sekundene lysene henger. Også `attention_visit` og `attention_warn_lights` åpner med stillhet. `attention_sab_mains` tar to av de fire takrørene i kontrollrommet for godt (`tube` 2 og 3) når strømmen kommer tilbake. Oppmerksomhetssystemet og FS47Soundscape stilner også nattens lyd av seg selv når noe farlig nærmer seg (FS47Omen); `silence` er den forfattede varianten.

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

## Dyr som varsel (N1110, ANIMALS-20261108)

`animals_warn_birds` og `animals_warn_coyote` spilles av `FS47AnimalWarn` (ikke av en hendelses egen kode), like før `horizon_figure`, `beam_figure`, `dish_lights` og `figure_near` når spilleren er ute til fots: en flokk vaktler letter alle på én gang, eller en coyote 18 til 30 m unna stopper og stirrer på spilleren i 7 sekunder og løper. Hendelsen holdes tilbake de sju sekundene og starter som ellers. Dyrene er de spillet allerede har (`FS47Animals::Warn`); manuset sier tanke og funn, og portene (`hours`, `odds`) sier når og hvor ofte: de samme hver gang samme natt spørres. Høyst ett varsel per natt, aldri i første skift. `"enabled": false` slår varselet av. Militæret (MILITARY-20261108) var allerede kodet i `S47Visitors`.
