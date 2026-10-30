> **Status: Gjeldende produksjonsmanus** · Laget 30.10.2026 av Claude på Toms bestilling: «lag så detaljert plan og historie for sc01-sc13 så når det er din tur er det bare å plassere inn». Bygger på [tidslinjen](TIDSLINJE_SC01_SC13_2026-10-29.md) og [revisjonen](HISTORIE_SC02_SC13_REVISJON.md). Kode-referansene er lest på main `4ce9244b` (stier under `unreal/Signal47/Source/Signal47/Private/` hvis ikke annet står).

# Produksjonsmanus SC01 til SC13

## Slik brukes dokumentet

Hvert scenario har samme oppsett: status, eier, inn og ut, mål og oppgaver ordrett, arbeidsliste med fil og linje, ny spilltekst ordrett (engelsk), og én rask sjekk. Arbeidsregelen er «iterer først» (`unreal/AGENTS.md`): bygg og én rask sjekk per endring, alle kontrakter samlet ved aktslutt. Rutene til jaktene er målt mot `Content/WebData/manifest.json` (gangsoner minus kollidere, 0,27 m klaring).

Pakkescener (SC01 til SC03) ligger i `unreal/Narrative/HorrorStory/modules/unified_scenarios.json`, men spillet leser den genererte `generated/HorrorPackData.inl`. Etter endring i JSON: kjør `build_pack.py` på nytt.

| SC | Tid | Status 30.10 morgen | Eier |
|---|---|---|---|
| SC01 | 18:00-23:40 | Ferdig | - |
| 00:13-02:00 | strømbrudd, trafotur | Ferdig | - |
| SC02 | 02:00 | Ferdig | - |
| SC03 | 02:15-02:45 | Tekster rettet (PR245); klokkeregel 02:44 gjenstår | Claude |
| SC04 | 02:55-03:15 | Ferdig (PR239) | - |
| SC05 | 03:15-03:25 | Rettet (PR244) | - |
| SC06 | 03:25-03:40 | Rettet (PR246) | - |
| SC07 | 03:40-03:55 | Rettet (PR246) | - |
| SC08 | 04:00-04:20 | Ferdig (PR240) | - |
| SC09 | 04:25-04:45 | Stasjonshendelsen og Tomás rettet (30.10); Noras samtale gjenstår | Claude |
| SC10 | 04:50-05:00 | Ny sluttreplikk og SARO som neste mål (30.10) | - |
| SC11 | 05:00-05:10 | Ferdig (PR250) | - |
| SC12 | 05:12-05:20 | Gammel kode, sheriff og patruljeark mangler | Claude |
| SC13 | 05:20-05:29:47 | Ferdig i grunnen, mål og tekster gamle | Claude |

---

## SC01 Kveldsrunden, 18:00-23:40

Ferdig og installert. Ingen arbeid. Kjørepermen 20:30 (`S47Observing.cpp` `UseBinder`, dokument `obs_runbinder`) er frøet til SC07; se der.

## SC02 Tilbake til bilen, 02:00

Ferdig. Ingen arbeid.

---

## SC03 Hjelpen som kom, 02:15-02:45 (Claude)

**Inn:** SC02-flukten vist, klokka hopper til 02:15 (`S47Horror.cpp:702-703`). **Ut:** sheriffens spørsmål vist, spinet åpner SC04 (`S47Spine.cpp`).

Forløpet er riktig og bygget (`S47HorrorSC03.cpp` fasene Report, Wait, Arrival, Meet, Lift, Retreat, Calm, Deputy, Complete). Det som gjenstår er tekst og to små regler.

### Arbeidsliste
1. Sheriffens replikker i `unified_scenarios.json` (scenen `SC03_RECEIVER_QUESTION`, rundt linje 657 og 669) peker på klokka ni. Ny tekst:
   - DEPUTY: `Your outfit up the road runs a receiver all night. Was it on when she went up?`
   - DESCRIPTION (uendret): `It was. The SARO receiver has been on passive monitoring since the shift began.`
   - DEPUTY: `Then I want its log and tonight's strip chart for this report. You have the keys. I'll follow you up to the gate.`
2. Ortiz linje 452: `Nobody needs you any closer to that yard tonight.` → `Nobody needs you any closer to that yard before daylight.`
3. Interne SC04-scener i pakken (rundt 732, 753, 1803) sier «strip chart from 21:00». SC04 eies nå av `S47SC04.cpp`; slett eller rett til `tonight's strip chart` så ingen gamle tekster kan vises.
4. Kjør `build_pack.py` og bygg.
5. Sjekk mot sheriffens vakt: `S47Timeline::DeputyOnDuty` (`Public/S47Timeline.h:40`) slutter 02:45. SC03-sheriffen er egen modell. Hvis SC03 varer forbi 02:45 skal ikke byens sheriff dukke opp i tillegg. La SC03 sette klokka til `max(klokke, 02:44)` når Complete nås, ikke mer.

**Rask sjekk:** `-S47HorrorContract=SC03` (eller utviklerskjermens F9-punkt foran SC03) og les sheriffens replikk.

---

## SC04 Sikre originalene, 02:55-03:15

Ferdig (PR239). Én tekstretting hører til SC05 (punkt 2 der).

---

## SC05 Samtalen som ligger foran, 03:15-03:25 (Claude)

**Inn:** SC04 ferdig, klokka 03:15, `HoldsResidual` usann. **Ut:** kortet «CHAPTER ONE / THE SECOND EXPOSURE» (`S47Story.cpp:908-912`), SC06 begynner.

Forløpet i `S47Story.cpp` er riktig og leser klokka levende: lås residualen (`StagePassed` 641-648), retningsløsning, utskrift (dato fra levende klokke, `S47Docs.cpp:266`), telefonen uten stemme, 47 sekunder, rommet, alle antenner til 026, kapittelkort. Fire ting er feil.

### Arbeidsliste
1. **Konsollen er åpen før SC04.** `HoldsResidual` gjelder bare mens SC04 er åpen, så residualen kan låses rett etter T01 og hele SC05 kan spilles før Ortiz. Endre `FS47Spine::HoldsResidual()` til: sann når rammen er på og (SC02 ikke flyktet ennå, eller SC03 lever, eller SC04 er 0 eller 1). Konsolltekst når den holdes (`S47Story.cpp:608`): `Rx.Status = "RECEIVER ON PASSIVE. NO SURVEY WORK TONIGHT"`, toast `The county has the night now. Nothing to tune until the originals are safe.` Merk: `S47NightSequenceTest.cpp:688-697` låser residualen rett etter 02:15 og må flytte den sjekken til etter SC04 (eller bruke spinet uten ramme).
2. **SC04-sperren motsier SC05.** SC04 sier at slew og printer stopper. Ny tekst i `S47SC04.cpp` `ScheduleDoc` og toast (rundt 140 og 173):
   - toast: `AUTO HOLD. The 06:00 series and the scheduled slews stop. Manual control and the printer stay live.`
   - dokumentlinjer: `SERIES 06:00 SURVEY ........ HELD` / `SCHEDULED SLEWS ............ HELD` / `MANUAL CONTROL .............. LIVE` / `PRINTER ..................... LIVE` / `RECEIVER .................... ON / PASSIVE`
3. **Gamle oppgavetekster.** `S47Story.cpp:335` «Babysit the survey until 06:00»: i kveldstidslinjen etter SC03 vises `The 06:00 series is suspended (Ward)` som ferdig rad. `crtLeft` (447) «Nothing logged since midnight.» → `Logged tonight: the 00:13 outage, the 01:52 restore. Nothing scheduled since.`
4. **00:14 der det skulle stått låseklokka.** Lagre klokka når residualen låses (`Night().LockAt`, ny verdi, lagres med natta). Bruk den i stedet for `TimelineText("02:14")` i `S47Story.cpp:69` (dekoderens Recorded), `:993` (tapeA) og notatene `AfterPrologue` `:940-946`. Uten lagret verdi (eldre saker): behold dagens tekst.
5. **Neste-hint etter SC04.** Den ekte SC04-avslutningen viser ingen pekepinn. Etter Wards replikk: toast `03:15. The residual is still there under the hydrogen line. The console.` (i `S47Spine.cpp` der SC04 settes til 2).
6. **Hopp.** Utviklerskjermens SC05-punkter (`-S47Jump=residual|printed|countdown`) lander 00:13. Legg til `-S47SpineStart=SC05`: setter SC04 til 2 og klokka til 03:15 før hoppet, så teksten blir som i ekte spill.

**Rask sjekk:** utviklerskjermen «SC05 The residual on the console» med ny start; lås, løs, riv av, svar, se notatet med 03:1x.

---

## SC06 Det andre motivet, 03:25-03:40 (Claude)

**Inn:** kapittelkortet etter SC05. **Ut:** `AfterChapterOne` (`S47Chapter2.cpp:969`), kort «CHAPTER TWO / THE AMENDED RECORD».

Kapittel 1 (`S47Chapter1.cpp`) er ferdig spillbart og leser klokka levende. Feilene er i hoppene og i stempler som hviler på standardverdien til `EventClock` (00:15:12, `S47Story.cpp:1060`).

### Arbeidsliste
1. Kveldsstandarden for `EventClock` når ingen hendelse har skjedd (hopp): 03:16:30. Da blir hoppet til kapittel 1 03:17:40 og S-03-loggen (`S47Chapter1.cpp:1442`) riktig.
2. `AfterPrologue` (`S47Story.cpp:936-982`): loggposten «RX BANK 3 K3 CLOSED» ved `TimelineTime(23:44)` blir 21:44 i kveldstidslinjen. Bruk 01:52 (T01-gjenopprettingen) i kveldstidslinjen.
3. Fjern død kode for den gamle sluttkortteksten (`S47Chapter1.cpp:235-241`).

**Rask sjekk:** utviklerskjermen «SC06 Chapter one», åpne S-03-loggen og se stempelet.

---

## SC07 Det Dale ikke leverte inn, 03:40-03:55 (Claude)

**Inn:** kort «CHAPTER TWO», `BeginChapter2`. **Ut:** kapittel 2 slutter, spinet holder for SC08 (`S47Chapters.cpp:50`).

### Arbeidsliste
1. **Klokka.** `S47Chapter2.cpp:921` bruker `EventClock + 15 min`; gjør det til `max(klokke, 03:40)` i kveldstidslinjen. Hoppet (`:950`) `EventClock + 35 min` → `max(klokke, 03:40)`.
2. **Frøet fra 20:30.** Kjørepermen (`obs_runbinder`, side fra 1979 med «AMENDED / SERVICE COPY») og E06 (1947, samme stempel) skal kobles:
   - E06 får en ny siste linje: `Same stamp as the 1979 page in the run binder: AMENDED / SERVICE COPY. Thirty-two years apart, the same hand.`
   - Ny regel på saksbrettet (`S47Chapter2.cpp:78-97`): kortene `obs_runbinder` og `e06` gir funnet `F_SAME_STAMP`, tekst `The station amends its own record the same way every time, and files the copy where nobody reads it.` (krever at `obs_runbinder` vises som kort; legg den til i brettets kortliste hvis den mangler).
3. **Permen er borte etter SC04.** SC04 tok kjørepermen «for the county», men samme hylleobjekt gir E06 kl. 03:40. Ny etikett i `S47Chapter2.cpp:529-535` etter SC04: `The service copies, behind where the run binder stood`.
4. **Brettkortet «PRINTOUT, 02:14»** (`S47Chapter2.cpp:673`): bruk utskriftens faktiske stempel (`Night().SolveAt` eller utskriftsdokumentets dato).
5. Fjern den døde kontordørteksten «Locked. Dale took the key home.» (`S47Chapter2.cpp:546`); `S47Office.cpp:446` eier døra.

**Rask sjekk:** utviklerskjermen «SC07 Chapter two», hent E06 og se den nye linja og brettfunnet.

---

## SC08 De peker på deg, 04:00-04:20

Ferdig (PR240). Mulig senere justering etter Toms spilling: den korte veien opp ryggen er et rent kappløp (spiller 3,0 m/s, kropp 2,1 m/s); kan gjøres strammere ved å la kroppen dukke opp ved R1 i stedet for i enden av raden.

---

## SC09 Den avbrutte oppstillingen, 04:25-04:45 (Codex)

**Inn:** `BeginChapter3`, 04:25. **Ut:** `S47Chapter3.cpp` `Finish` (1177), kort «CHAPTER FOUR / ROOM 6».

### Arbeidsliste
1. **Død hendelse.** `Public/S47StationTaken.h:39` og `S47Away.cpp:79,534`: `STATION_LAST` = 04:35 − 18:50 = 04:16:10, så `StationDue()` er alltid for sent når kapittel 3 starter 04:25. Bruk `S47Timeline::Chapter4(E) − 18 min 50 s` (04:31:10 i kveldstidslinjen). Rett selvtesten (`S47StationTaken.cpp:133,147`).
2. **Noras samtale på feil sted.** (Åpen 30.10: kontrolltelefonen har ingen kapittel 3-krok, og en ny `Inter->Add("phone")` ville overstyre telefonen for alle kapitler. Trenger en ringekrok i `AS47Story` som kapittel 3 kan bruke, slik `PhoneHook` i kapittel 2.) Revisjonen: Nora ringer først når begge feltkopiene er fremkalt. Flytt samtalen fra feltelefonen på STATION 01 (`S47Chapter3.cpp:899-903, 923-959`) til SARO: når `Collect` på våtbenken er ferdig (`:1158`), ringer kontrolltelefonen etter 3 s. Replikkene:
   - `So you went out there.`
   - `This line still rings in the motel office. I heard it ring at four, every night, for a year.`
   - `You have the frames now. Look at them before you believe them.`
   - `Room 6, Sierra Motor Court. Bring the original record and both prints.`
   Feltelefonen på stasjonen gir da bare summetone: `A dial tone, and a long way off, another phone ringing that nobody answers.`
3. **Tomás.** Feltboka (`S47FieldBook.cpp:229`) har «T. VEGA, FIELD BOOK 3». Første side får `Tomás Vega, field engineer. If found, return to STATION 01.` så navnet finnes i SC09.
4. **Feltkopiene.** Tegningsetiketten «FIELD COPY / KEEP WITH THE TRANSIT» (`S47Drawings.cpp:166`) er nok; ingen ny gjenstand.

**Rask sjekk:** `-S47EveningTimeline -S47SpineStart=on -S47Jump=chapter3`, fremkall, svar Nora i kontrollrommet.

---

## SC10 Rom 6, 04:50-05:00 (Codex)

**Inn:** `BeginChapter4`, 04:50. **Ut:** `Finish` (1202), spinet holder for SC11.

### Arbeidsliste
1. **Noras siste replikker** (`S47Chapter4.cpp:1193-1196`). Ny rekkefølge:
   - `East from the station there is a survey bolt in the road. C crosses there.`
   - `Tomás walked out to the closing point and did not walk back.`
   - `Don't finish his observation. Get the originals to the courier, and don't stop at C.`
2. **Neste mål etter rommet** (`:474`, notat `:1206`, kort `:260`): under rammen går turen til SARO. Objektiv `SARO // THE REMOTE SHEETS COME IN AT 05:04. GET THE ORIGINALS OUT`, notat `Next: SARO. Hondo and Site 11 are sending their sheets. Then the courier.` Uten ramme (eldre saker) beholdes dagens tekst.
3. Finalens ekko (`S47Chapter6.cpp:410` «Do not stop on the line.») endres til `A woman's voice: "Don't stop at C."` så det speiler den nye replikken.

**Rask sjekk:** `-S47Jump=chapter4` med rammen, snakk ferdig med Nora, les objektivet.

---

## SC11 SARO blir avsøkt, 05:00-05:10 (Codex, bygges som SC04/SC08)

**Inn:** kapittel 4 slutter, spinet holder SC11 (`S47Spine.cpp` `HoldChapterEnd(SC11)`), spilleren kjører fra motellet. **Ut:** spilleren kjører bort fra SARO med originalene; klokka `max(klokke, 05:10)`, `AfterSC11` (kapittel 5 kl. 05:12).

Ny klasse `FS47SC11` (`Public/S47SC11.h`, `Private/S47SC11.cpp`), samme mønster som `FS47SC04`: `Begin/Save/Stop/Step/Label/Use/Objective/Tasks`, eies av `FS47Spine` (`Act11`), lagres som `sc11d`, plassholderstolpen skjules. Kroppen er `FS47PaleBody` under `"#root"`.

### Forløpet
| Steg | Hva | Gjenstand (id) | Etikett |
|---|---|---|---|
| Drive | Kjør til SARO (egen bil eller servicebil). Ved ankomst: klokka `max(klokke, 05:00)`. | | |
| Collect | Mottakerens original fra hendelsesloggen | `printer` | `Tear off the receiver original from the event log` |
| Collect | Det påbegynte hendelsesarket på pulten | `logbook` | `Take the event sheet you started` |
| Telex | Når begge er tatt: klokka 05:04, telex-skriveren skriver Hondo og Site 11 linje for linje (`FS47SaroDark` sin skrivemåte, `S47SaroDark.cpp:320-329`), ca. 40 s spilltid | `telex` | `Tear off the remote sheets` (når ferdig) |
| Siege | Lys søker bygningen utenfra, kroppen søker inne | | |
| Out | Ut til bilen og kjør | `truck` / egen bil | |

### Tekst (engelsk, ordrett)
- Ankomst-toast: `SARO, five in the morning. Every window dark but yours.`
- Objektiv Collect: `SARO // THE ORIGINALS: THE RECEIVER'S EVENT LOG AND THE SHEET YOU STARTED`
- Objektiv Telex: `SARO // 05:04. THE TELEX IS PRINTING: HONDO, THEN SITE 11`
- Objektiv Siege: `SARO // SOMETHING IS SEARCHING THE BUILDING. GET THE ORIGINALS TO THE TRUCK`
- Telex-dokument `sc11_remote`, tittel `Remote sheets, Hondo and Site 11`, side:
  `TELEX 05:04 HONDO VALLEY RADIO STATION / SOURCE H-1 ... RANGE NEGATIVE / FIRST LIMIT 05:29:47 / REQUEST PROCEDURE`
  `TELEX 05:05 SITE 11 / SOURCE S-11 ... RANGE NEGATIVE / FIRST LIMIT 05:29:47 / SAME DIRECTION AS SARO`
  `(Three stations, three sources, one time. Nobody asked for a procedure for this.)`
- Notat ved Telex: `05:04. Hondo and Site 11 on the telex. Three sources, the same first limit: 05:29:47.`
- Siegestart-toast: `Light comes across the control-room windows from outside. Not a car. It moves like someone looking.`
- Lysbryter-toast (lokal lampe av): `The tubes go out. The light outside goes on searching.`
- Fangst: svart, ny start ved telexen med originalene beholdt, klokka står.
- Utkjøring, notat: `05:10. Out of SARO with the originals. Behind me the light stayed on the building.`

### Jakten
Inne: kroppen kommer inn gjennom labdøra (`labDoor`, sone x 12..13,6, z -1,7..-0,7) og går mot kontrollrommet via landingen og servicedøra, eller gjennom annekset hvis korridordøra er åpen. Ute: et søkelys (bruk `S47Props.cpp:906-945` `AddBeacon` eller en `USpotLightComponent` som sveiper) går langs kontrollromsvinduene i en fast syklus (12 s). Står spilleren i lyskjeglen i mer enn 1 s, hører kroppen det som en lyd der spilleren står.

Rutepunkter (målt, alle kanter fri):

| Node | x, z |
|---|---|
| crWest | -4.2, 0.3 |
| crCentre | 0, 0.3 |
| crEast | 3.4, 0.3 |
| crSouthW | -2.4, 2.9 |
| crNW | -3.5, -1.4 |
| crNE | 3.5, -1.4 |
| eastDoorIn | 4.6, 1.6 |
| landing | 7.5, 1.6 |
| stairTop | 10.2, 1.6 |
| stairFoot | 10.2, 4.5 |
| padWest | 7.2, 4.8 |
| truckDoor | 7.2, 7.5 |
| walkNode | 9.5, -0.2 |
| labPath | 11.0, -1.2 |
| labDoor | 12.8, -1.2 |
| labIn | 14, -1.2 |

Kanter: crWest-crCentre, crCentre-crEast, crCentre-crSouthW, crSouthW-crWest, crWest-crNW, crEast-crNE, crNW-crNE, crEast-eastDoorIn, eastDoorIn-landing (servicedøra), landing-stairTop, stairTop-stairFoot, stairFoot-padWest, padWest-truckDoor, walkNode-landing, walkNode-stairTop, labPath-walkNode, labDoor-labPath (labdøra), labIn-labDoor. Annekset: gjenbruk SC04-nodene via crSouthW-corridorSouth (korridordøra).

To veier ut:
- **Kort:** servicedøra, landingen, trappa og bilen, når kroppen er i annekset og lyset er på andre siden.
- **Lang:** korridoren, nødutgangen og nordsida rundt til bilen (SC04-runden baklengs), når kroppen står ved labdøra.

Lokal lampe: hvis det finnes en bryter for lysrørene i kontrollrommet (`S47Lighting`), gjør den til en gjenstand `crLights` med etiketten `Switch off the tubes`. Mørkt rom gjør at kroppen ser kortere (14 → 7 m), men søkelyset utenfra går som før.

### Testflagg og utviklerstart
- Kontrakter `-S47SpineContract=sc11play` og `sc11long` med `-S47Jump=chapter4`, mønster `FS47Spine::SC08Test`.
- Utviklerskjermen: `SC11 SARO is searched`, `-S47EveningTimeline -S47SpineStart=SC11 -S47Jump=chapter4`.

---

## SC12 Det dere rekker å sende, 05:12-05:20 (Codex)

**Inn:** `BeginChapter5`, 05:12. **Ut:** `Finish` lagrer i steget `road` (`S47Chapter5.cpp:810-816`), `BeginChapter6`.

### Arbeidsliste
1. **Notatet ved start** (`:248`) «Out of room 6.» → `Out of SARO with the originals. The diner is the nearest phone that isn't ours.`
2. **Ward-samtalen blir påkrevd og ny** (`:575-580, 727-754`; i dag valgfri, oppgave `:346`). Ny samtale fra betalingstelefonen:
   - WARD: `I read the telex. All of it.`
   - REYES (valg, lest fra arkene): `Three sources. H-1, S-11, and ours, SARO-3. First limit for all three: 05:29:47.`
   - WARD: `H-1, S-11, SARO-3. 05:29:47. I have it written down, with the time you called.`
   - WARD: `Get the originals to the courier. Don't wait for me.`
   Gir dokumentet `sc12_warning`, tittel `Warning receipt, Ward`, side `05:1x. Read to Dr. Ward from the Mesa Diner payphone: H-1, S-11, SARO-3. First limit 05:29:47. Read back correctly.` (klokka levende). Fjern «Six o'clock, Reyes» og «before the morning series» (`:747-748, 540, 751`).
3. **Sheriffen 05:15.** Ny figur (gjenbruk SC03-sheriffens modell, `S47HorrorSC03.cpp` Deputy-delene) som kommer inn kl. 05:15, setter seg ved disken og legger fra seg patruljearket. Replikk ved prat: `Road's open north. I drove it at four-thirty. Nothing on it but a bolt the county keeps meaning to pull.` Gjenstand `diner:patrol`, etikett `The deputy's patrol sheet`, dokument `sc12_patrol`: `PATROL SHEET / 14 APR / 04:30 OLD ROAD NORTH TO MILE 11, CLEAR. SURVEY BOLT AT 8.15 STILL IN THE ROAD. COURIER POINT MILE 10.4.`
   `S47Timeline::DeputyOnDuty` (04:30) passer.
4. **Sjåføren går ut** (revisjonen). Fjern samtaleemnene `:708-722` og `DriverMore`; veiopplysningen kommer fra patruljearket. Figuren kan bli sittende uten samtale.
5. **Veggkartet P13** (`:761-804`): riktig svar bm8; legg til teksten `COURIER 10.4` ved bolten på E14 (`S47Drawings.cpp:326`). Det gale svaret «bolt past the ten-mile post» (`:773`) endres til `the mile-ten turnout` så det ikke forveksles med kurermålet.
6. **Avreisekontroll.** «Drive the old road» (`:367`) krever `sc12_warning` og originalene; ellers toast `Not yet. Ward has to have the sources before you go.`

**Rask sjekk:** `-S47Jump=chapter5` med rammen, ring Ward, vent til 05:15, les patruljearket, kjør.

---

## SC13 Siste vei mot Roswell, 05:20-05:29:47 (Codex)

**Inn:** `BeginChapter6`, minst 05:20. **Ut:** finalen 05:29:47 og `Flow->Finale()`.

### Arbeidsliste
1. Objektiv og oppgave (`S47Chapter6.cpp:178-179`): `THE OLD ROAD // THE COURIER WAITS AT MILE 10.4. PASS C WITHOUT STOPPING` / oppgave `Carry the originals to the courier at mile 10.4`.
2. Skilt `10.4 COURIER` ved veien (bare på kart og skilt; finalen ved C 8.15 avbryter reisen, revisjonen punkt 11).
3. Lagring: kapittel 5 lagrer allerede ved avreise fra dineren (`S47Chapter5.cpp:810-816`); det er det sikre punktet. Ingen endring.
4. Ekkoet `S47Chapter6.cpp:410`, se SC10 punkt 3.

**Rask sjekk:** `-S47Jump=chapter6` og les objektivet; `chapter6:line` for ekkoet.

---

## Helnattskontrakt

Codex' NightR-kjede (`-S47NightSequenceContract` utvidet) går fra ny sak. Etter «iterer først»: kjør den ved aktslutt, ikke per endring. Når SC11 er bygget: kjeden følger SARO, Caliche, motell, SARO, diner, vei med lagring og innlasting ved hver overgang.

## Valg for Tom

1. SC08: skal den korte veien være strammere (kroppen ved R1), eller er kappløpet greit? Avgjøres etter at du har spilt det.
2. SC11: skal den lokale lampebryteren være med i første versjon, eller vente?
3. SC13: finalens ekko «Don't stop at C.» i stedet for «Do not stop on the line.»?
