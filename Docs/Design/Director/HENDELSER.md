# Hendelser i Freeform og Story

**Status:** Referanse, kildekontroll. **Gjelder:** `claude/day-d-20261107` på `d70555a8`. **Sist kontrollert:** 7. november 2026.

Bare det som er lest i kode. Filer er under `unreal/Signal47/Source/Signal47/Private/` hvis ikke annet står. Linjenumre kan flytte seg: en annen agent endrer nå `S47FreeformDirector.cpp` og `S47FarmSite.cpp`. «Beregnet» betyr regnet ut fra koden, ikke målt i spill.

## Viktigst

1. **Ro etter fare gjelder ikke alle.** `RestUntil` og `RestFor(1800)` stopper bare Directors hendelser og mysteriet. Dørbesøk, titting, lysrom, stedsbesøk, motorvei og formasjon sjekker den ikke. Etter en bortføring har bare stedsbesøk egen ro (`SetCalm`, 1 time). Bibelen krever ro for alle.
2. **Ingen felles grense for fare per natt.** `service_presence`, stedsbesøk, dørbesøk, lysrom og bilens lys har egne terninger.
3. **Directors 22 hendelser kan ikke startes fra F10.** Ingen menylinje kaller `Director->Force`. Dør, lysrom, skumringslys og mysterium har heller ingen F10-linje.
4. **Kl. 02 til 06 er trangt (beregnet).** Natten har 11 timer på 45 aktive minutter, så vinduet er om lag 16 minutter. Director ser etter noe hvert 4. til 7. minutt, altså rundt 3 forsøk. Etter `service_presence` går 8 av disse minuttene til ro.
5. **Budsjettet for små hendelser kan ikke fylles over tid (beregnet).** Det er 5 små hendelser med 2 til 4 dagers nedkjøling, omtrent 1,7 per dag i snitt, mot budsjett 2 til 4.
6. **Utviklerkjøring viser ikke vanlig natt.** I `bDevRun` skjer ikke lyskuler om natten, mysterium, ku, notat, oppvåkning eller lys over bilen uten testverdier. F10-prøving sier derfor lite om rytmen.
7. **`TheirHours` (kl. 02 til 06) står i 10 kall i spillkoden.** Toms beslutning 7. november om at kl. 02 ikke er fast må endres flere steder.

## Freeform: fellesregler for Director

Gjelder alle i tabell A. Kode: `Eligible` `S47FreeformDirector.cpp:462`, `Start` `:506`, planlegger `Step` `:943`.

- **Blokkeres alltid av:** annen Director-hendelse, motorvei opptatt, `Taken` opptatt, stedsbesøk på bakken (`:464-467`), meny/`Held`.
- **Blokkeres ved naturlig start av:** første skift, at bilen kjøres (`FreeformSaveBlock`, `:480`), `RestUntil` (`:488`).
- **Sted:** `saro` hvis ikke annet står (`:476`). Gården ligger i samme område.
- **Ro etter slutt:** 45 s, 480 s etter trussel (`:1173`), 1800 s etter bortføring (`S47Taken.cpp:350`).
- **Nedkjøling:** 2 til 4 dager, fast ut fra ID og dag (`:432`, `:492`). Ikke for `dusk_orb`.
- **Forsøk:** hvert 240 til 420 aktive sekund (`:968`), én tilfeldig blant dem som er klare (`:986-990`).
- **Dagsbudsjett (`:952`):** små 2 til 4, undersøkelser 0 til 2, himmel 0 eller 1. Hvilket budsjett er hardkodet (`:988`), ikke styrt av `family`.
- **F10:** Nei for hele tabell A. `Force` virker bare i utviklerkjøring (`:540`).
- **Felles prøve:** `S47FreeformTest` løkke over alle ID-er med `Force`, unntatt `dusk_orb` (`S47FreeformTest.cpp:1766-1790`). Andre prøver står i tabellen.

## Tabell A: Director-hendelser

Data i `unreal/Narrative/Freeform/events.json`, de tre siste i kode (`:78-80`). «Klokke» er krav utover `night` (kl. 19 til 06).

| Hendelse | Kode | Når | Sjanse og budsjett | Også blokkert av | Spilleren ser | Prøve utover felles | Merknad |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `extra_pulse` | `:486`, `:606` | Dag 1, hele døgnet. Krever arkivert normalmåling. | Undersøkelse, likt blant klare | Se krav | Avvik på målingen | `S47FreeformEvidenceTest`, SelfTest | |
| `wrong_source` | `:486` | Dag 2, ellers som over | Undersøkelse | Se krav | Avvik | | |
| `time_disagreement` | `:486` | Dag 2, ellers som over | Undersøkelse | Se krav | Avvik | | |
| `duplicate_print` | `:486` | Dag 3, ellers som over | Undersøkelse | Se krav | Ekstra utskrift | | |
| `unsigned_telex` | `:487`, `:615` | Dag 3. Krever minst ett funn. | Undersøkelse | Se krav | Skriver og anonym telex | | |
| `feeder_fault` | `:501`, `:612` | Dag 2, hele døgnet | Undersøkelse | Strømfeil finnes | Strømfeil | SelfTest | `duration` 900 brukes ikke ved naturlig start: slutter med en gang (`:1000-1004`) |
| `landing_trace` | `:503` | Dag 3, hele døgnet | Undersøkelse | Stedet er i synsfeltet innen 40 m | Merket bakke og bit | Fotoprøve i `S47FreeformTest` | Dagslys tillatt |
| `s03_failure` | `:500`, `:607` | Dag 1, hele døgnet | Liten | Antennefeil, strømfeil | Antennefeil | `S47MenuPointerTest` | Som `feeder_fault` |
| `roof_steps` | `:496`, `:1072` | Dag 1, natt, også kveld | Liten | Takforsøket eier vinduet | Skritt på taket | SelfTest | |
| `empty_car` | `:502`, `:513` | Dag 2, natt | Liten. Lys over bilen 35 % kl. 02 til 06 | Bilen i synsfeltet | Tom bil, ev. lys over den | `S47MenuPointerTest` | Lyset kan ende i bortføring. Aldri lys i utviklerkjøring uten testverdi |
| `flyover` | `:1072` | Dag 2, natt | Liten | | Lys over himmelen, dunk | `S47MenuPointerTest` | |
| `roomlight_return` | `:498`, `:725` | Dag 1. Spiller slo av lys, gikk, 60 s, over 25 m unna | Liten | Se krav | Lys tent igjen | `S47ReactiveLightTest`, SelfTest | |
| `response_lights` | `:988` | Dag 2, natt | Himmel | | Fjerne lys | `S47DevEvents` | `family` sier investigation, men budsjettet er himmel |
| `quiet_zone` | `:1084` | Dag 3, natt | Himmel | | Romlyden blir borte i en sone | Bare felles | Samme. Ingen egen prøve |
| `observer` | `:491`, `:499`, `:814` | Dag 1, kl. 02 til 06 | 50 % per natt, ellers ikke før 3 dager etter sist | Kropp mangler, andre kropper ute | Skikkelse ved antennene | `S47MenuPointerTest`, SelfTest | |
| `service_presence` | `:491`, `:493`, `:802` | Dag 2 kl. 19, praktisk kl. 02 til 06 | 35 % per natt. Én per dag | Som `observer` | Skrap, skikkelse jager | `S47FreeformJourneyTest`, `S47MenuPointerTest`, SelfTest | Kontakt gir lokal gjenopptakelse (`:1058`), ikke bortføring |
| `close_pass` | `:491`, `:494`, `:803` | Dag 4 kl. 19, praktisk kl. 02 til 06 | 10 % per natt. Minst 3 dager | | Lav farkost like ved | `S47UfoVisualTest`, `S47MenuPointerTest` | |
| `feeder_presence` | `:470`, `:815`, `:960` | Dag 2, kl. 02 til 06. Innen 12 m av mateskap på veien | 40 % per natt (felles med neste) | Som `observer` | Skikkelse ved skapet | `S47MenuPointerTest` | Spilleren må stå der, for eksempel ved reparasjon eller mysteriets hint |
| `array_presence` | `:497`, `:963` | Dag 3, kl. 02 til 06. Innen 12 m av serviceskap | Delt 40 % | Antennefeil, som `observer` | Skikkelse, kan gi feil | `S47MenuPointerTest` | Spilleren må stå der |
| `farm_lights` | `:808`, `:978`, `:589` | Dag 2. Fra `LightAfter`: 35 % kl. 21 til 23:30, ellers 00:30 til 02:30. Fra SARO | 45 % per natt (felles med neste), 50/50 mot byen | | Radiotips, lys over gården. Drar ved 130 m | `S47MenuPointerTest`, SelfTest | Ved slutt merkes stedet (`:1156`) |
| `town_lights` | `:471`, `:522` | Samme. Fra SARO, vei eller diner | Delt 45 %. Sted likt blant 4 | Ingen sted klart | Tips, lys over kino, fôr, kirke eller bensin. Drar ved 70 m | `S47MenuPointerTest`, SelfTest | |
| `dusk_orb` | `S47DuskOrb.cpp:78` og `:82`; Director `:958` og `:966` | Skumring 18:30 til 19:15 hver kveld, sjeldnere etter foto. Natt 20:15 til 23:15 i fast rekke | Fast tabell, ingen terning (`S47DuskOrb.cpp:9-44`) | Må være på SARO | 1 til 3 lyskuler ved S-02/S-03, flykter ved nærhet | `S47DuskOrbTest`, `S47NormalJourney`, SelfTest | Nattbesøk aldri i utviklerkjøring uten testflagg (`:966`). Går i kvelden, se føring |

## Tabell B: Andre Freeform-systemer

| Hendelse | Kode | Når og krav | Sjanse og nedkjøling | Blokkeres av | Spilleren ser | F10 | Prøve | Merknad |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Merket sted | `S47FarmSite.cpp:170`, `S47FreeformDirector.cpp:1156` | Når lysene slutter. Til neste kl. 12 | 60 % at besøk er mulig (`:178`). Merkene kommer alltid | | Ni merker, dis, lyd, funn | Gjennom besøk | `S47MenuPointerTest` | |
| Stedsbesøk | `S47FarmSite.cpp:371-391` | Kl. 02 til 06, mørkt, innen 55 m, til fots, 12 s | Følger av merket sted. Hvem: `:52`, gård krabbe 30 %, høy 20 % fra kl. 03. By 12 % og 15 %. Ellers Roswell | Director-hendelse, kropper ute, `Taken`, dørbesøk, ro 1 t | Varsel, bilen starter ikke (`:419`), kropp, jakt | Ja, 3 linjer | `S47MenuPointerTest` | Ser ikke `RestUntil` |
| Radiomelding om stedet | `S47FarmSite.cpp:221` | Kl. 02 til 06, over 100 m unna, merket i kvelden | Fast én gang | | Andre oppringing | Nei | Ikke funnet | |
| Bortføring | `S47Taken.cpp:61`, `:75`, `:91`, `:309` | Fra stedsbesøk, lysrom, dørbesøk (`S47DoorVisit.cpp:443`), bil | Tapt tid: Roswell 35 til 70 min, krabbe 120 til 240, høy 300 til 420 (`:38-40`) | Allerede opptatt, kjører | Lys, løft, gap, notat | Ja | SelfTest, `S47MenuPointerTest` | Andre agent endrer dette nå |
| Etterspill | `S47Aftermath.cpp:79`, `:90`, `:125` | Etter bortføring eller besøk på gården | Notat 40 % (ikke Roswell), ku 50 %, høyst 2 | Utviklerkjøring: nei | Feil klokke på OPS, linje i notater, ku borte | Nei | SelfTest | |
| Titting | `S47IndoorDread.cpp:384-438` | Kl. 02 til 06, innendørs 20 s, mørkt, vindu i sikte | 50 % per natt | `Quiet` (`:167`): motorvei, Director-hendelse, `Taken`, besøk, kropper | Hode i vinduet, dukker | Ja, «At the window» | SelfTest, `S47MenuPointerTest` | Ser ikke `RestUntil`. Director kan starte over den, og den trekker seg |
| Dørbesøk | `S47DoorVisit.cpp:264-288` | Kl. 02 til 06, innendørs 25 s, dør funnet | 45 % per natt, minst 2 netter imellom (`S47DoorVisit.h:39`) | `Quiet` | Noe prøver døren, merker etterpå | Nei | SelfTest, `S47MenuPointerTest` | Ser ikke `RestUntil` |
| Lysrommet | `S47IndoorDread.cpp:440-456` | Kl. 02 til 06, samme rom som før, 25 s, 3 dager etter sist | 40 % per natt | `Quiet` | Lys gjennom vindu, mulig bortføring | Nei. «Massive light» er egen visning | SelfTest (lagring), `S47MenuPointerTest` | Ser ikke `RestUntil` |
| Motorvei | `S47HighwayUfo.cpp:242-262`, `:404` | Kjører over 1 m/s på åpen strekning, kl. 02 til 06 | Ingen terning. 3 netter imellom. Følge/patrulje veksler | Director-hendelse | Lys, sheriff eller følge | Ja, 2 linjer | `S47HighwayUfoTest`, SelfTest | Ser ikke `RestUntil` |
| Formasjoner | `S47SkyVisitors.cpp:243`, `:608` | Fra 00:30 natt 1, deretter kl. 21:30 til 05:30, utendørs | Fast tabell, 75 til 110 min | `FormationQuiet` (`:597`) | Høye lys i formasjon | Ja | `S47DevEvents`-kontrakt, SelfTest | Ser ikke `RestUntil` eller dørbesøk. Viker når noe starter |
| Høyt himmelspor | `S47SkyVisitors.cpp:379-387`, `:444` | Dag 2, start kl. 21 til 03, utendørs | 22 % per natt | Ingen | Lys høyt oppe | Bare forhåndsvisning | SelfTest | **Kolliderer:** sjekker ikke andre hendelser |
| Mysterium: spor | `S47FreeformDirector.cpp:818`, `:920` | Dag 2, fra kl. 20 til 02:30. Ved stativet, eller ved skapet før kl. 12 | 30 % per natt til saken åpner | `MysteryQuiet` (`:900`), kjøring | Viser spretter, skap summer | Nei | SelfTest | Aldri i utviklerkjøring uten testverdi |
| Vekket av søvn | `S47Freeform.cpp:1765`, `S47FreeformDirector.cpp:826` | Ved venting forbi kl. 02, natt 2 og senere | 85 % hvis noe er klart, aldri to netter på rad | Første skift, utviklerkjøring | «Something woke you» | Nei | Testbryter `bTestWake`, bruk ikke kontrollert | |
| Feltprøver | `S47CaseFieldTests.cpp:116`, `:301` | Spilleren forbereder. Vindu 01:30 til 04:30 | Følger valgt forklaring | Egen tilstand. Holder `roof_steps` unna | Taklyd eller port | Nei | `S47CaseFieldTest`, SelfTest | |
| Dagskort | `S47FreeformJobs.cpp:43`, `:70` | 3 per dag, foreslått kl. 19 til 22:30 | Frø per dag og plass, ingen frist | | Oppgaver på OPS | Nei | SelfTest | |
| Slitt anlegg | `S47FreeformWorld.cpp:119`, `S47ArrayWear.cpp` | Ny stasjon: 15 av 27 nede | Fast | | Spilleren reparerer greiner | Nei | `S47ArrayWearTest` | |
| Radio nær fare | `S47FieldRadio.cpp:172-181` | Når noe nærmer seg og radio er tatt | Avstand | | Knitring | Følger hendelsen | SelfTest | Følger, starter ingenting |

## Kollisjoner og det som aldri kan skje

**Kolliderer:**

- Directors `Eligible` ser ikke `IndoorDread`. En hendelse kan starte under dør eller titting. Disse viker (`S47DoorVisit.cpp:387`), så ingen krasj, men scenen brytes.
- `FormationQuiet` og `MysteryQuiet` ser `PeekActive`, ikke `DoorActive`.
- Høyt himmelspor går under alt, også bortføring, så lenge spilleren er utendørs.
- Fra bibelen: «minst åtte aktive minutters ro», «ingen ny jakt ved oppvåkning». Koden dekker bare Director og stedsbesøk. Etter bortføring kan dørbesøk, titting og motorvei komme med en gang klokken tillater det.

**Aldri eller nesten aldri:**

- `duration` for `s03_failure` og `feeder_fault` virker bare når den er tvunget.
- Budsjett for små hendelser kan ikke fylles over tid (beregnet).
- `feeder_presence` og `array_presence`: sjeldne, siden spilleren må være ved skapet kl. 02 til 06.
- Sjansene 35, 10 og 50 % er i praksis lavere: Natten kastes én gang (`:802-814`) uten å se nedkjølingen på 2 til 4 dager, så kastet kan gå tapt (`:492`, `:796`). `observer` tillater kast etter 3 dager, mens nedkjølingen kan være 4.

**Ikke kontrollert:** vanlig bakgrunnslyd og meteorer (`S47Ambience.cpp`, `S47Sky.cpp`), og om `Clock +15 minutes` i F10 virker i Freeform. Linjen endrer Storys klokke.

## Story

Kartlagt med en underagent som leste kilden målrettet. Jeg har kontrollert stikkprøver: `bRandom`, SARO mørk, himmellys uten `OneAtATime`, `SurveyVersion`, `bHeld`. `S47Chapter1` til `6`, `S47NightLife`, `S47Abduction`, `S47Observing` og `S47Substation` er bare lest med grep. Filer uten sti står under `Private/`, og tall uten filnavn hører til filen foran. Ingen `-S47...`-flagg er kjørt.

**Rammer:**

- **Tidslinje:** bare nytt spill gir kveldstidslinjen. `-S47Jump` gir den gamle (`S47Flow:905`), og da er Spine og himmellys av. Slike prøver viser derfor andre regler enn spilleren møter.
- **Kapitler:** prologue = SC01 til SC05, ch1 = SC06, ch2 = SC07 og SC08, ch3 = SC09, ch4 = SC10 og SC11, ch5 = SC12, ch6 = SC13 (`S47Spine:166-184`).
- **BN** er nattens felles sperre (`S47Night:759-760`): ingen panel eller filmsekvens, kapittel fra ch1 og ikke ch6, `Dawn` under 0,3, og ingen stillhet, bortføring, byhendelse, takskritt, spole, kontortelefon, array eller arkiv.
- **Kurven** finnes allerede som `May(kind)` (`S47Curve:64`) og gjelder bare ch1 til ch5: 600 s rolig, oppbygging 390 til 170 s, topp 300 til 240 s, hvile 480 til 720 s. Innslagsnivå: sign og cutout ,12; post ,2; call ,3; event ,45; scare ,5; long ,6; glimpse og abduct bare på toppen. Per oppbygging høyst 1 til 2 av call, event og cutout. Høyst én 47-sekunders per kapittel. Står stille under Spine-vern, panel, film, telefon, rom 6 og dineren i ch5 (`S47Night:416`).

| Hendelse | Kode | Når | Sjanse og nedkjøling | Blokkeres av | Spilleren ser | F10 | Prøve | Merknad |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| **A. Fast løp** | | | | | | | | |
| Kjøretur, diner | `S47Evening:278-286`, `:510-513` | 17:20 | Fast | Åpning | Radio, servitør | Nei | `-S47EveningRoute=`, `-S47HorrorArrival` | |
| Dales overlevering | `S47Evening:680-682`, `:482` | 18:00, SARO | Fast | Scene, panel | Telefon | Nei | `-S47HorrorContract=SC01` | |
| Observasjonsrunder | `S47Observing:322`, `:542-549` | Til ca. 22:50 | Fast | Modal | Array, skriver | Nei | SelfTest | |
| Slag på vinduet | `S47Horror:369`, `S47HorrorVisual:454-466` | Etter normalsjekk, blenda ned | Én gang | Blenda oppe | Slag, spor | Nei | `-S47HorrorContract=SC01` | Før kl. 02, se 8 |
| Residual, strømbrudd | `S47Story:684-686`, `:746-779` | 00:13:47 | Fast | Film | Mørke, telefon | Nei | `-S47NightSequenceContract` | |
| Fylkesanrop, T01 | `S47Evening:157-213`, `S47Substation:198` | 01:35, kjøring | Fast | Modal | Lampe, strøm | Nei | `-S47T01Contract`, `-S47SubstationLiveTest` | |
| SC02 første møte | `S47Horror:607-644`, `S47HorrorThreat:516-522` | 02:00, til fots etter T01 | Fast. Fanget gir lokal omstart | Modal, dialog | Blek kropp, jakt | Nei | `-S47HorrorContract=SC02`, SelfTest | |
| SC03 Ortiz tas | `S47HorrorSC03:677-706` | Etter rapport | Fast | Meny | Lys, løft | Nei | `-S47HorrorContract=SC03`, `-S47SC03LookAway` | |
| SC04 anneks | `S47Spine:314-316`, `S47SC04:326-372` | 02:55 til 03:15 | Fast | Spine-vern | Jakt | Nei | `-S47SpineContract=sc04*`, SelfTest | |
| SC05 47 sekunder | `S47Story:869-911` | Etter SC04, 03:15 | Fast | Konsoll låst | Nedtelling, antenner på 026 | Nei | Ikke kontrollert | |
| SC06 B-12 | `S47Chapter1:1259-1291` | Etter SC05 | Trinn | Dør | Foto | Nei | SelfTest, ikke lest | |
| SC07 arkiv | `S47Chapter2:265`, `:456` | Fra 03:40 | Fast | Ward først | Ward, arkiv | Nei | `S47Chapter2`-sjekker | |
| Wards tilbakeringing | `S47Chapter2:492-498`, `:901-906` | Ch2 | Fast | Film | Telefon | Nei | Ikke kontrollert | |
| SC08 Earl, jakt | `S47SC08:145`, `:305-322`, `:440-448` | 04:00 til 04:20 | Fast | Spine-vern | Spole, jakt | Nei | `-S47SpineContract=sc08*` | |
| SC09 STATION 01 | `S47Chapter3:622`, `:778-795`, `:952-960` | 04:25 | Fast | Sikre utskrifter | Stemme, Nora | Nei | `-S47SpineContract=sc09` | Se 2 |
| SC10 rom 6 | `S47Chapter4:370`, `:727-755` | 04:50 | Fast | Ingen tilfeldige | Nora, radio 4+7 | Nei | Ikke kontrollert | |
| SC11 beleiring | `S47Spine:337`, `S47SC11:196-250` | 05:00, telex 05:04 | Fast | Spine-vern | Søkelys, jakt | Nei | `-S47SpineContract=sc11*` | |
| SC12 diner | `S47Chapter5:403`, `:760-776` | 05:12, fra 05:15 | Fast | Ward før avreise | Ward, patruljeark | Nei | `S47Chapter5`-sjekker | |
| SC13, THE EVENT | `S47Chapter6:264-297`, `:627-641` | 05:20 til 05:29:47 + 34 s | Fast | Ch6 stenger alle | CB, motor dør, uttrekk | Nei | `-S47FinaleMovie=`, `-S47Auto=` | |
| **B. Tilfeldig eller vilkårsstyrt** | | | | | | | | |
| Små tegn | `S47Curve:167-175`, `S47Night:437-447` | Ute eller kjøring, nivå fra ,12 | Hvert 28 til 60 s, tettere med nivå | Opptatt | Radio, dash, lykt | Nei | Ikke kontrollert | |
| Skritt bak deg | `S47NightEvents:55`, `:135-149` | Ute, stoppet etter 8 s gange | Delt ventetid 90 til 260 s. Maks 3 | BN, kurve event | Skritt | Nei | SelfTest | |
| Bank på taket | `S47NightEvents:56` | Kjøring | Delt. Maks 1 | BN | Bank, notat | Nei | `-S47FieldBook`, SelfTest | |
| Radio på selv | `S47NightEvents:57` | 14 til 40 m fra bil | Delt. Maks 1 | BN | 1090 på | Nei | SelfTest | |
| Nyhetsbulletin | `S47NightEvents:58`, `:165-173` | Kjøring på 1420 | Delt. Maks 1, long | BN, long | 1947-melding | Nei | SelfTest | |
| Telefon ett ring | `S47NightEvents:59` | Ute under 30 m | Delt. Maks 2 | BN | Ett ring | Nei | SelfTest | |
| Servicedør åpen | `S47NightEvents:60`, `:209` | Ute over 25 m, ikke i syn | Delt. Maks 1 | BN | Dør | Nei | SelfTest | |
| Stillhet 47 s | `S47NightEvents:61`, `S47Roadside:145-151` | Veiskulder, gått 100 m | Én gang, long | BN | All lyd borte | Nei | `-S47Roadside=` | |
| Fjernt anrop | `S47Calls:32-49` | Ute med lykt | 45 til 220 s, 12 % horn | Kurve call | Rop | Nei | SelfTest | |
| Roswell-kropper | `S47RoswellGroup:180-188`, `S47Night:775` | SARO, vei, bare topp | 120 til 300 s. 30 % tak, 25 % to. Maks 8 | Kurve glimpse | Små kropper | Ja, spawn | `S47RoswellAIProbe`, SelfTest | |
| Pappfigur | `S47Cutouts:44-71` | Fot, kjøring | 40 til 200 s | Kurve cutout | Papp-alien | Nei | `-S47Cutout` | |
| Figur ved billys | `S47TownLayer:314-320` | Over 7 m/s, etter 2 møter | Én gang | Kurve post | Figur | Nei | `-S47Cutout=` | Delvis lest |
| Bil langt unna | `S47Roadside:98`, `:128` | Skulder, z over 150 | 25 til 60 s, 15 min pause | Kurve event | Lys stopper | Nei | `-S47Roadside=` | |
| Figur på moteltak | `S47Roadside:162-164` | Ved motellet | 60 %, ellers aldri | Kurve glimpse | Stille figur | Nei | `-S47Roadside=` | Se 10 |
| By: papp, deputybil, kiosk, gatelys | `S47TownEvents:66-128` | Ch1+, i byen, 02:40 til 05:19 etter type | Én gang hver, 10 min mellom | Kurve | Papp, tom bil, ring, lys | Nei | `-S47Town=` | |
| Kuene på 026 | `S47Pasture:59-89` | 02:40 til 05:00 | 10 min gap | BN-lignende | Flokk peker | Nei | `-S47Pasture`, SelfTest | |
| Motell-TV | `S47MotelTv:142-150` | 02:30 til 05:05 | 10 min gap | Kurve long | Bærebølge 47 s | Nei | `-S47MotelTv`, SelfTest | |
| Lys på linje B | `S47LineLights:85-87`, `S47SaroNight:401-408` | Kjøring z 120 til 240 | Én gang | Kurve event | Lys i rekke | Nei | `-S47LineLights`, runtime-prøve | |
| Bortføring | `S47Abduction:43-49`, `:311-318`, `S47Away:519-524` | Ch1 til ch4, ute | Trykk 4 til 8 min, maks 3, 40 min gap, varsel 47 s, tapt tid 0 | Spine, rom 6, ch5, kurve | Stillhet, lys | Nei | `-S47Abduct`, `-S47LostNights` | Se 9 |
| Tatt i bilen | `S47TruckTaken:30`, `:74-87` | Ch1 til ch4, sørover | Maks 2 | Kurve abduct | Stall, frost | Nei | `-S47TruckTaken=`, SelfTest | |
| Bilen på B | `S47TruckOnB:32-58` | Ch2 til ch5 | Fast trinn | Trouble | Blink | Nei | `-S47TruckB=` | |
| Opptaket går | `S47Recording:62-79` | Ch3 til ch5, etter bortføring | Én per opptak | Kurve scare | Figur | Nei | `-S47RecWalk` | |
| Takskritt | `S47RoofSteps:91-100`, `S47Phones:356-360` | Ch2, arkiv | Én gang | Scare, arkiv | Skritt, frost | Nei | `-S47Roof` | |
| Arkivtelefon | `S47ArchivePhone:174-184` | Ch2 | Maks 2, 90 s | Andre | 47 s linje | Nei | `-S47Archive=` | |
| Spole 047 | `S47ArchivePhone:838-850` | Spiller | Fast | Deck | Puls 4+7 | Nei | `-S47Reel047` | |
| Array tilbake | `S47SaroNight:358-369`, `S47ArrayBack:232-246` | Ch2 til ch5 | Spiller | Opptatt | Antenner, lamper | Nei | `-S47Array`, `-S47Follow` | |
| Kontortelefon | `S47OfficePhone:260-267` | Ch4, før rom 6 | 1,4 eller 8 s | BN-lignende | Bjelle, skritt | Nei | `-S47Office` | |
| Niende ku | `S47NinthCow:107-134` | Ch3, STATION 01 | Én gang | Telefon | Ku, stillhet | Nei | `-S47Ninth`, SelfTest | |
| Morgen holder pusten | `S47MorningHolds:94` | Ch5 inne, 05:13 til 05:20 | Én gang | `OneAtATime` | Alt stopper | Nei | `-S47Morning`, SelfTest | |
| Hull i stjernene | `S47StarHole:119-121`, `S47Away:635-655` | Ute, lenge sett | Én gang | `OneAtATime` | Notat | Nei | `-S47StarHole` | |
| Tråd ingen knyttet | `S47SameThread:44`, `:110` | Per bortføring | Per telling | | Tråd på tavla | Nei | `-S47Same` | |
| Earls spole | `S47LostNights:315-320` | Spiller | Én gang | Quiet | Spole | Nei | `-S47ReelTape` | Delvis lest |
| A02, N02, P01 | `S47AnomalyWorld:213-280` | Etter midnatt; P01 04:00 til 12:00 | Én per trinn | `Free` | Figur flyttet, vaskeri, skriver | Nei | `-S47AnomalyRoute=` | |
| Himmellys SKY01 til 05 | `S47SkyVisitors:100-113`, `:738-741` | 21:30, så per kapittel | Fast, 80 til 240 s | Meny, film. Skjult i ch6 og rom 6 | 1 til 9 lys | Forhåndsvisning | `-S47SkyVisit=`, SelfTest | Se 7 |
| Motorvei-UFO | `S47HighwayUfo:242-262`, `:404` | 19 til 06, sørover, ikke ch6 | Én per natt | `OneAtATime`, film | Skrog, sheriffbil | Ja | `-S47HighwayUfoTest=` | Se 6 |
| Natteradio | `S47TripRadio:229-261` | På tur, etter kapittel | Én per merke | Sign-off | Earl, innringere | Nei | SelfTest | |
| Liv i natta | `S47NightLife:388-401`, `:573-595` | Alltid | Bil 200 til 480 s, ugress 180 til 420 s | `LifeQuiet` | Bakgrunn | Nei | SelfTest | |
| **C. Aldri i kveldsløpet** | | | | | | | | |
| SARO mørk 02:29 | `S47SaroDark:147`, `:168-172` | 00:29 til 00:40 og ch1 | Én gang | Umulig | Mørke og figur | Nei | `-S47Dark` | **Død**, se 1 |
| STATION 01: alle går | `S47Away:533-552`, `S47StationTaken:41-90` | Stage `return` | | Umulig | Hytta låses | Nei | Ikke kontrollert | **Død**, se 2 |
| Tilfeldig biltrøbbel | `S47Trouble:107-114` | `bRandom` | 40/30/15/15 % | Settes aldri | | Nei | Ikke kontrollert | **Død**, se 3 |

**Død kode eller vilkår som ikke kan oppfylles:**

1. SARO mørk krever i kveldsløpet klokke 00:29 til 00:40 og samtidig ch1, som starter rundt 03:15 (`S47Spine:324`). Hondo-telexen går derfor alltid.
2. `StationTaken` krever stage `return` etter oppringing (`S47Away:536`). Nye saker har `SurveyVersion=1` (`S47Chapter3:448`), der Nora ringer SARO og kapitlet avsluttes (`:952-960`).
3. `bRandom` settes aldri (grep). Tilfeldig biltrøbbel er død.
4. Spine-plakatene har `bShow=false` og ingen kollisjon (`S47Spine:301-304`).
5. Gammel Glimpses-gren (`S47Glimpses:125-144`, `:292-299`) brukes bare uten Roswell-gruppe.

**Kollisjoner og avvik fra bibelen:**

6. Motorvei-UFO står ikke i bibelens tidslinje og har ingen kapittelsperre. Før kl. 02 kan den utløses hvis spilleren kjører forbi T01 til z≥650. Det går imot «ingen alien før ca. 02:00». Om veien er åpen dit, er ikke kontrollert.
7. Himmellys ser ikke `OneAtATime`, så de kan gå under jakt og topper. De står heller ikke i bibelen.
8. Slag på vinduet i SC01 er før kl. 02. Bibelen sier «uro». Tom bør bekrefte.
9. Bortføring og bilbortføring finnes i Story, ch1 til ch4, uten tapt tid. Eneste bryter er innstillingen `bAbductOn`.
10. Uavklart: Bibelen kutter den separate svarte figuren. Figuren på moteltaket og radiolinjen «somebody on the roof» finnes fortsatt (`S47Roadside:20`, `S47TripRadio`). Om det er samme figur, er ikke avgjort.
11. Sjåførens dialog er igjen (`S47Chapter5:722-743`), men oppgaven er byttet ut med patruljearket (`:356`). Om dialogen er sperret, er ikke kontrollert.
12. Nedre tider 02:30 og 02:40 for TV, kiosk og kuer virker aldri i kveldsløpet, fordi alle krever ch1.
13. F10 dekker i Story bare Roswell-spawn, himmellys, formasjon, motorvei og klokke +15 min, foruten forhåndsvisningene av enkeltfigurer og lys.

**Ikke kontrollert i Story:** fullt 1947-foto og tredje silhuett i `Flow->Finale`, klokkehevingen i SC06, utløsere for scenene D21 til D61 og Dale-walkie, bulletin etter 04:00, `Life->Dawn`. `S47HorrorPack` og `S47EveningPack` er bare data og ikke gjennomgått.
