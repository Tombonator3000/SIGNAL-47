# Forslag: samlet director for hendelser

**Status:** Godkjent av Tom 7. november, se DECISIONS. **Gjelder:** Freeform først, Story bare som lesevisning. **Sist kontrollert:** 7. november 2026 mot `d70555a8`. Grunnlag: [HENDELSER.md](HENDELSER.md).

## Svar til Tom

Ja til én oversikt og ett regelsett. Nei til omskriving. Hendelsene beholder sin egen kode for hva de gjør. Det som flyttes, er spørsmålet **«får du starte nå, og hvis ikke, hvorfor?»** og tallene som styrer rytmen.

Mye finnes allerede og gjenbrukes:

- `FS47FreeformDirector` er regissør for 22 hendelser, men ikke for dørbesøk, titting, lysrom, besøk på stedet, motorvei eller formasjoner.
- `Narrative/Freeform/events.json` har katalogen, men kompileres inn gjennom `generate.py`, og sjanser og hverdagsregler står i C++.
- Story har allerede `FS47NightCurve::May(kind)` (`S47Curve.cpp:64`) med fasene plain, build, peak og rest. Samme navn og tankegang brukes i Freeform. Story-kurven endres ikke.
- `M->OneAtATime()` (`S47Main.cpp:83`) finnes, men ser i Freeform bare motorveien og Director.

## Hva som mangler i dag

1. **Ingen felles ro.** `RestUntil` (8 min etter fare, 30 min etter bortføring) gjelder bare Directors egne hendelser og mysteriet. Dør, titting, lysrom, stedsbesøk, motorvei og formasjon sjekker den ikke. Bibelen krever ro etter fare.
2. **Ingen felles grense for fare per natt.** Hvert system kaster egne terninger.
3. **Ingen forklaring.** `Eligible` svarer bare ja/nei. Ingen kan se i spillet hvorfor noe uteblir.
4. **F10 kan ikke starte Directors hendelser.** `Force` finnes, men brukes bare av prøvene.
5. **Tall krever bygg** og står spredt i minst åtte filer.

## 1. Datafilen

`Content/Director/director.json`, stages med én linje i `Signal47.Build.cs`, samme mønster som `FreeformTerrain/*.json`:

```csharp
RuntimeDependencies.Add("$(ProjectDir)/Content/Director/*.json", StagedFileType.UFS);
```

Lesing med `FFileHelper::LoadFileToString` og tydelig avvisning, slik `S47Relief.cpp:131-140` gjør. `generate.py` validerer og bygger inn samme fil som standardverdier, så en manglende eller ugyldig fil gir dagens oppførsel og en logglinje.

**Justering uten bygg:** Spillet leser også `Saved/Director/director.override.json` hvis den finnes, og panelet viser «OVERRIDE AKTIV» med en knapp for å laste på nytt. Filen i `Saved` overlever installasjon, derfor varselet. Dette er eneste nye vei; pakket standard endres bare ved bygg.

```json
{
  "version": 1,
  "globals": {
    "their_hours": [2, 6],
    "tick_seconds": [240, 420],
    "rest_after": { "danger": 480, "other": 45, "taken": 1800 },
    "budget_per_day": { "minor": [2, 4], "invest": [0, 2], "sky": [0, 1] },
    "danger_per_night": null
  },
  "events": [
    { "id": "service_presence", "system": "director", "slot": "ground", "level": "danger",
      "from_day": 2, "hours": "their_hours", "areas": ["saro"],
      "draw": { "per_night": 0.35 }, "cooldown_days": [2, 4],
      "needs": ["native_body", "no_bodies_out", "not_driving"], "enforce": true },
    { "id": "door_visit", "system": "indoor", "slot": "ground", "level": "danger",
      "from_day": 1, "hours": "their_hours",
      "draw": { "per_night": 0.45 }, "min_nights_between": 2,
      "needs": ["indoors", "door_found", "dark"], "enforce": false },
    { "id": "sky_formation", "system": "sky", "slot": "background", "level": "hint",
      "hours": [21.5, 5.5], "schedule": "fixed_table", "enforce": false }
  ]
}
```

Tallene over er dagens verdier. **Ingen språk for uttrykk i JSON:** `needs` er navn på små C++-funksjoner. Validatoren kjenner listen. `their_hours` er ett sted i stedet for dagens 10 kall til `TheirHours` i spillkoden, og verdien settes til det den andre agenten ender på.

- `slot`: `ground` er én om gangen, `background` kan gå samtidig, men viker.
- `level`: `hint`, `observation`, `presence` eller `danger`. Dette er grunnlaget for rytmen.
- `enforce`: `false` betyr at regelen bare vurderes og logges. Bryter for hvert system.

## 2. `May(id)`

Ny klasse i nye filer, `S47Rhythm.h/.cpp`. Ingen terninger og ingen endring av tilstand i `May`:

```cpp
struct FS47Verdict { bool bMay=true; FName Why; FString Detail; double RetryClock=-1; };
FS47Verdict FS47Rhythm::May(FName Id) const;   // felles: dag, klokke, nedkjøling, plass, ro, budsjett
void FS47Rhythm::Started(FName Id);
void FS47Rhythm::Ended(FName Id, bool bDanger);
```

Rekkefølgen for hvert system blir:

1. `Rules.May(id)`: felles rytme.
2. Systemets egne `needs`: sikt, dør, avstand, kropp i bygget.
3. Systemets terning, som før.
4. Systemet starter og melder `Started`.

Faste grunner, så panelet og loggen sier det samme: `off`, `tutorial`, `too_early_day`, `outside_hours`, `cooldown`, `slot_taken:<id>`, `rest`, `budget`, `not_here`, `needs:<navn>`, `not_drawn_tonight`.

Terningene flyttes ikke i første omgang. Dagens lagrede trekk (`PendingThreatNight` og lignende) må overleve Continue, og Bibelen forbyr nye kast ved gjeninnlasting.

## 3. F10-panelet

Ny gruppe i `S47DevEvents.cpp`, «Director», med egen visning i spillet:

```text
DIRECTOR   DAG 3  02:41  natt  [OVERRIDE AKTIV]
Små 1/3  Undersøkelser 0/1  Himmel 0/1  Fare i natt 0/-
NÅ        ingen
RO        212 s igjen, etter service_presence
NESTE     Director ser om 94 s · formasjon 03:10 · dørbesøk om 12 s innendørs
KAN NÅ    peek  door_visit
KAN IKKE  service_presence  budget
          close_pass        too_early_day (dag 4)
          observer          cooldown (1 dag)
          site_visit        needs:armed_site
          highway_ufo       not_here (må kjøre)
[Start nå]  [Last JSON på nytt]  [Kopier til logg]
```

«Start nå» bruker `Force` og virker bare i utviklerkjøring, som i dag. Story-kjøring viser `Curve->Text()` og hva `May` svarer for hver art, kun lesing.

## 4. Trinn

Hvert trinn kan flettes alene. **Trinn 2 til 4 rører `S47FreeformDirector.cpp` og `S47FarmSite.cpp`, og starter først når den andre agentens arbeid med døgn, pris på tid, bortføring og fremgang er flettet.** Trinn 0 og 1 rører dem ikke.

| Trinn | Innhold | Oppførsel | Størrelse | Kontroll |
| --- | --- | --- | --- | --- |
| 0 | Legg oversikten i `Docs/Design/`, og før beslutning i DECISIONS. | Ingen | S | `check_docs.py` |
| 1 | `director.json` som speil av dagens verdier, loader, stage-linje, validator. F10-gruppe som lister alle 22 og starter dem med `Force`, og leser JSON. | Ingen | S–M | Egenprøve: JSON og C++-konstanter er like. |
| 2 | `Eligible` får en variant som returnerer grunn. Små `WhyNot()` i Indoor, Door, FarmSite, Highway, Sky. Panelet viser KAN/KAN IKKE. | Ingen | M | Egenprøve: samme svar som `Eligible` over rutenett av dag/klokke/sted. |
| 3 | Director og himmel leser tall fra JSON: sjanser, timer, nedkjøling, budsjett, tick. | Ingen med standardverdier | M | Som over, pluss override-prøve. |
| 4 | `May` i skyggemodus for alle systemer. `enforce:false`, logger bare «ville blitt stoppet». | Ingen | S–M | Én kveld med loggen: Hvor ofte ville den grepet inn? |
| 5 | Sett `enforce:true` per system: titting, dør, lysrom, stedsbesøk, motorvei, formasjon. **Første reelle endring.** | Ja, ett system per fletting | S hver | Rask spillprøve. |
| 6 | Felles `danger_per_night`, og rytmen Tom vil ha. | Ja | S | Simulator og spillprøve. |
| 7 | Story: bare hvis Tom ber om det etter Freeform. | Ingen til da | – | – |

Lagring: Trinn 1 til 4 legger ikke noe i lagringen. Trinn 5 får et valgfritt felt `rhythm` i Directors objekt; gamle lagringer starter med tomt felt og leser nedkjøling fra `LastDays`. `Begin` godtar ukjente felt, så `v` forblir 1.

Valgfritt, lite tillegg: `Tools/director/simulate.py` kaster dagens tall over 30 netter med en valgt spillertype (innendørs på SARO, ute, på veien) og viser hva som ville skjedd. Ingen motorjobb. Verden modelleres ikke, så det er en rytmeskisse, ikke bevis.

**Status for trinnene, 7. november kveld:** 0 er gjort. 1 er flettet som director-1 og -2: F10 og rytmefil, uten endret oppførsel. 2 til 4 er flettet som director-3: grunn i panelet, tall fra JSON og skyggelogg i alle systemer. Grunnen i panelet leses ved siden av `Eligible`, ikke fra den. Nedkjølingens 2 til 4 dager og `their_hours` står fortsatt i koden. 5 og 6 ligger som director-5a til -5e og er ikke bygget. Det samme gjelder de små hendelsene i director-4. Override virker bare med `-S47DirectorOverride` eller F10 i en testkjøring.

## 5. Hva som blir i kode

- Hva hendelsene gjør: faser, kropper, sikt, lys, lyd, `FS47Taken`, dørenes logikk.
- Verdensspørsmål: `needs`-funksjonene, som kan-se, nær matestasjon, finn dør, under tak.
- Lagring og Continue: engangsvirkninger, «lagre før virkning» i `Apply` (`S47FreeformDirector.cpp:561-585`), migrering.
- Storys faste tider og `FS47NightCurve`. Story får ingen tilfeldige hopp.
- Sikkerhet ved oppvåkning: ingen softlock og riktig avregning av klokke.
- Saksdata og fasit forblir egne filer, se [EDITOR-FORSLAG.md](EDITOR-FORSLAG.md), ikke i rytmefilen.

## 6. Tom må avgjøre

1. Skal ro etter fare gjelde alle systemer? Bibelen sier ja, koden gjør det ikke. Anbefaling: ja, trinn 5.
2. Skal det være høyst ett farlig møte per natt på tvers av systemene? Bibelen nevner «maksimalt ett», og det må da avklares hva som teller. Anbefaling: dør, stedsbesøk, `service_presence` og bortføring teller; titting og lys gjør det ikke.
3. Skal `Saved/Director/director.override.json` få virke i det vanlige installerte spillet, eller bare i utviklerkjøring? Anbefaling: bare med en egen kommandolinje-bryter når du spiller, så rytmen ikke endres ved uhell.
