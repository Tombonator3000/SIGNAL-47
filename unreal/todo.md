# Todo for unreal/

## Native lys og skygger (2026-10-22)

- [x] Erstatt materialenes flood/hemi og gamle projeksjoner med ekte lamper; behold historienes tilstand og delte lyskilder.
- [x] Lommelykt og billykter med skygger, volumetrisk tåke, Lumen, svak måne/skylight og levende daggry. Fast eksponering og VHS/Clean, uten lysbryter mellom Web og Unreal.
- [x] Lokalt editorbygg og målrettede native scenarier fra SARO til den gamle veien. Side-ved-side-bilder og detaljer i LIGHTING.md.
- [x] Taklys fra faktiske armaturer i rom/korridor/annex/lab, synkron emissive, skrivebordslamper fra skjermåpningene og egen grønn glød foran hver registrerte skjerm.
- [x] Varme, skyggekastende array-spotter fra de elleve eksisterende feltarmaturene. Grå diffuserflater og Ops-ghost under strømbrudd rettet.
- [x] High/1440p med TSR fra 60 %: rom 73,4 fps, gård 95,5 fps, p95 15,330/11,527 ms uten andre tunge prosesser i målevinduet. Low/Clean 90,0 fps i rommet, Ultra 39,2 fps i gården. Detaljer i LIGHTING.md.
- [ ] Pakket bygg og menneskelig gjennomspilling med de nye lysene. Standardutgaven er ikke oppdatert av dette arbeidet.
- [ ] Kjør det isolerte materialsteget ved integrasjon med eldre generert Content, før kandidat eller pakket utgave brukes.

## Prøven av prologen

- [x] Prosjektet bygger, modellene er importert, og den pakkede builden starter med kontrollrommet, gården og himmelen (2026-10-21).
- [x] Alle fjorten testkjøringene gjennom med verdenen fra nettversjonen (2026-10-21).
- [ ] Tom går rundt i rommet og ut på gården og sier om prøven holder: lys, stemning, modellenes skala og retning.
- [x] VT323 og Special Elite som i nettversjonen (2026-10-21).
- [x] Verdenen hentes fra nettversjonen (Tools/webexport), så modellene står som der (2026-10-21).
- [ ] Bildet sammenlignet med nettversjonen fra de samme stedene: rommet, gården, antennene, vinduet, racket.

## Hvis prøven holder (hele konverteringen)

- [ ] Prologen spilt av et menneske fra 23:41 til sluttkortet.
- [ ] Historien som JSON felles for web og Unreal, slik at tekst og valg ikke må skrives to steder.
- [ ] Feltkameraet og fotolaben.
- [x] Lyd og musikk fra web/public (2026-10-21).
- [x] `core/Audio.ts`, `signalVoice.ts` og `ambience.ts` portert på en Web Audio i C++, kryssjekket mot Chromium (2026-10-22).
- [ ] Natta ute i gang som i main.ts: `StartNight()` etter `StartRoomTone()`, `NightLife(dt, spillerens posisjon, klokka)` hver ramme, `SetSpace(room|yard|lab)` ved områdebytte, `Suspend` i pausen, volumene fra innstillingene.
- [ ] Prologen spiller tonene med `Beep(...)` og nettversjonens tall i stedet for filene fra make-synth.py, og radioen i høyde 1,45 som i Prologue.ts.
- [ ] ReadAloud og RecordedDialogue (stemmebankene under /Game/Audio/Voice, katalogene i Content/Audio/Raw/voices.json); `decoder.ts` trenger en ConvolverNode.
- [x] RadioVoice, radio.ts, tripRadio.ts og score.ts med kilde for lange opptak, volum, innkobling og selvtester (2026-10-22).
- [ ] Radio-/musikkporten prøvd i samlet pakket bygg med hele screenshots.sh og menneskelig lytting/input. Koble øvrige nattsystemer til Score.Held og samme Main.TownSeen; se RADIO_PORT.md.
- [ ] Lagring ved kontrollpunkter.
- [ ] Kapittel 1 til 6 (web/src/story) og områdene. Verdenene er med i eksporten (AS47Web::Enter).
- [x] Kjøringen (web/src/drive) og turene i World.ts (2026-10-22). Gjenstår: eksporten kjøres på nytt for `drive` i manifestet, lydene laget med Web Audio (dekk, vind, startmotor, ferist), lyktenes mønster (headSlots) i flomlysmaterialet, lyktene på dineren, byen og motellet (shareLights), radioen.
- [x] Glødepunktene blinker som i glow.ts (2026-10-22, `S47Glow`).

## Kapittel 2 (2026-10-22)

- [x] Kapittel 2 portert (S47Chapter2, S47Annex, S47PhoneScripts, S47Drawings2, S47Panels2), selvtest og gjennomkjøring uten verden.
- [ ] Dørene (Doors.onChange) videre til kapitlenes `DoorChanged`, som Prologue.doorChanged.
- [ ] Lagringen kobles til `US47Chapter2::OnSave` og `Host->LoadCase`.
- [ ] Kapittelkortets ring (`bHoldRing`, `RingOut`, `RingAgain`, `RingShown`) når kortet er portert; kapittel 1 sin slutt kaller `US47Chapter2::AfterChapterOne(method)`.
- [x] Bryterbordets linjelamper og opptakerens papirstrimmel (archive047.ts): i export.js (2026-10-22); kjør eksporten på nytt for å få håndtakene (C++ finner dem uten).
- [ ] S47Drawings og S47Drawings2, S47Panels og S47Panels2 slås sammen.
- [x] Lagring ved kontrollpunkter, tittel, pause og notatbok (2026-10-22, `S47Saves`, `S47Flow`).
- [ ] Kapittel 1 til 6 (web/src/story), kjøringen (web/src/drive) og områdene. Verdenene er med i eksporten (AS47Web::Enter).
- [x] Glødepunktene blinker som i glow.ts (2026-10-22, `S47Glow`).

## Kapittel 4 (2026-10-22)

- [x] Kapittel 4 portert (S47Chapter4, S47Room6, S47Motel, S47Walkie, S47CB, S47NpcDialogue, samtalepanelet i S47Panels), selvtest og kjøringer i -game uten pakket build.
- [ ] Med kapittel 3: P11 og P12 trenger nålene P08, e10.carrier og hut.rx på saksbrettet (kapittel 3 sin regelbok). Kapittel 3 sin slutt kaller `US47Chapter4::BeginChapter4(nullptr)`, og kapittel 3 sin `truckTrip` går foran i `US47World::TruckChoice`.
- [ ] Kjør `Scripts/audio-raw.py` (eller `import-audio.sh`) på nytt i hovedkopien, så replikkene i rom 6 har stemme (rollestemmene er nå 50).
- [ ] Hjelperne fra main.ts: `Taken` (abduction.ts), `OfficePhone` (officePhone.ts), og at CB-anropet venter på bortføringen og truckTaken.
- [ ] Lyset fra vinduene på gangveien (windowLight.ts) og papirbildene (`cardField`, `letterPaper`) i dokumentvisningen.
- [ ] Continue i rom 6 prøvd med en ekte lagring.

## Etter lagring og flyt

- [ ] Kjør screenshots.sh med den pakkede builden (title, pause) og se tittelkameraet over gården og miniatyrbildene i lagringene med verdenen på plass.
- [ ] Saksbrettets oppsett i lagringen: sett `M->Flow->SaveBoard` og `LoadBoard` til CaseBoard sin Save()/Load().
- [x] Feltkameraet og rullen i Kit-fanen: `M->Flow->HaveCamera` og `RollText` (2026-10-22).
- [ ] Musikk- og radionivået (Music, Radio) inn i lydmodulen når den har egne busser; volumet virker allerede.
- [ ] Feltkameraet og rullen i Kit-fanen: sett `M->Flow->HaveCamera` og `RollText` når kapittel 1 og albumet er portert.
- [x] Musikk- og radionivået (Music, Radio) inn i lydmodulens busser, inklusive tittellåten (2026-10-22).
- [ ] Worn VHS (bildet `heavy`) i M_WebTone; nå bare Clean og VHS.
- [ ] Fortelleren (Serling) til åpningskortet og kapittelkortet når stemmene er portert.

## Nattens lag (2026-10-22)

- [x] Loggen, kurven, nattens hendelser, ropene, glimtene, lommelykta og albumet (`US47Night`), med lagringen.
- [ ] Krokene i `FS47NightHooks` settes når systemene porteres: bortføringen, veikanten (stillheten), byens hendelser, skrittene på taket, motellets fjernsyn, lysene på B, Earls spole, SARO mørk, kontortelefonen, arkivtelefonen, matrisen tilbake, papfiguren, bilen på B, tatt i bilen, opptaket, morgenen, den niende kua, stjernehullet og bilradioen.
- [x] Det nærmere blikket (zoomK, `S47Phones`, 2026-10-22). Fotskrittene (`player.onStep`) gjenstår.
- [ ] Glimtets og albumfigurens malte omriss (runde 14) og albumets bilder (runde 15) er ikke i eksporten; figuren fra koden brukes.
- [ ] Lommelyktas mønster (core/cookies.ts) og fall-off: nå Unreals invers kvadrat med intensiteten avstemt 4 m ut.

## Telefonene, arkivet og opptegnelsene (2026-10-22)

- [x] Arkivtelefonen og matrisen den vipper, kontortelefonen, skrittene på taket, feltboka og konvolutten, blikkene og det nærmere blikket, tråden (`US47Phones`).
- [ ] Krokene fra andre porter i `US47Phones`: `Reel047` (reel047.ts på dekket), `DecoderPlaying`/`DecoderStop` (Decoder.ts), `Dark` (saroDark), `CbBusy`, `AbductionCount`, `DishRogue`/`DishBack` (dishesFollow, arrayBack), `Frames`, `FbGet`/`FbSet` og `StationDrawer` (kapittel 3), `Foretell` (beitet og lysene på B), `StarHoleExamine`.
- [ ] Kapittel 4: `OfficePhone` på kontortelefonens proxy (`court:officePhone`) spør `M->Phones->OfficePhone` først.
- [ ] Kapittel 2s slutt venter på skrittene: settes kapittel 3s begynnelse som ny `OnEnd`, går den gjennom `M->Phones->Ch2End`.
- [ ] Opplesningen av et nærmere blikk etter 650 ms (Serling) når fortelleren er portert.
- [ ] Dishenes røde lys blinker i takt mens de er vippet (DishArray.beaconPhase) når glødepunktene blinker.
## SARO sine egne hendelser (2026-10-22)

- [x] SARO mørk og telexen fra Hondo Valley, matrisen tilbake, antennene som følger, lysene på B, spole 047, Earls spole og opptaket (`US47SaroNight`), med lagringen.
- [ ] Bortføringens port setter `US47SaroNight::Abduct` (stage, warn, backAt, data.last, overPhase, recordings, holdRecord, data.dishes) og bruker `FS47Walker` fra S47Recording.h for den som står der spilleren ble tatt; til da spilles varselet for antennene av testflagget.
- [ ] Radioens port setter `BoyCallTimes`, `CallerVoice` og `BoyVoices` (Earls spole), og musikken holdes av `ReelTapeBusy`, `SaroDarkBusy`, `ArrayBackBusy` og `RecWalkBusy` (musicHeld).
- [ ] Arkivtelefonens port spiller `Reel047` (update, play, reset) og setter `TipMoving`/`TipHolds` (archiveDishes.ts); feltboka setter `LineLights->bAnyWay`; det nærmere blikket setter `Zoom`.
- [ ] Hjørneklokka i sekunder: én kjede i main.ts sin rekkefølge (abduction.warnClock, saroDark, archive, ch6) i stedet for hver sin `ClockSeconds`.
- [ ] Telexrullen på maskinen (ControlRoom.setTelexRoll) tegnes ikke; papiret og loggen har meldingen.
- [ ] ConvolverNode i S47WebAudio: reverbet i decoder.ts er en kambank som står i for den.
- [ ] Glødepunktene utenom antennenes røde lamper blinker ikke ennå.

## Caliche, veien og motellets natt (2026-10-22)

- [x] C++-port av Town/TownEvents, Roadside, Herd/Pasture, NinthModel/NinthCow, MotelTv, MorningHolds/DinerCooler, Cutouts, Caliche og Diner med main.ts-koblinger, lagringsfelt og testflagg.
- [x] Caliches RecordedDialogue/NpcDialogue/EarlDialogue, innspilte replikkøer, replay og stopp. Generell ReadAloud/Serling gjenstår hos fortellerporten.
- [ ] Regenerer webeksport og import for byens delte lamper/glødepunkter, fjernbilens lys, townSlots/neonSlots og herdets beamLit. Implementer pointScreen i native billboards.
- [ ] Koble LostNights til Caliche.Reel og kapittel fem til TownLayer.Diner. Bygg og prøv kapittel tres FreeShot etter integrasjon; kapittel 3/5 finnes ikke i grenens utgangspunkt.
- [ ] Samlet pakket bygg, hele screenshots.sh, Continue fra disk, ekte input, menneskelig lytting og sammenligning av de regenererte bildene mot nettversjonen. Se TOWN_PORT.md.

## Verdenens eget liv (2026-10-22)

- [x] Glødepunktene, floraen og vinden, den levende himmelen og horisonten, lyset i lufta, vinduslyset, nattelivet, Ultra-lysene og de delte lysene (`US47WorldLife`).
- [x] Toppunktfargene i importen og de nære mesaene uten tåke (hvite kanter ved STATION 01).
- [ ] Eksporten og importen kjøres på nytt i hovedkopien (`Tools/webexport/import.sh`).
- [ ] Krokene i `US47WorldLife::Hooks` settes av kapitlene: `SkyLift` og `SunElev` (kapittel 6, soloppgangen), `SaroDarkAdapt` (SARO mørk), `MorningBusy` (morgenen), `LifeQuiet` (stillheten).
- [ ] `S47ArchiveDishes` bruker `FS47GlowPoints::SetPhase` og `HoldAt` for de vippede dishenes røde lys (DishArray.beaconPhase).
- [ ] `US47Night::Dawn` bruker sin egen formel; bør lese `Sky->Dawn`.
- [ ] Resten av World.update: neonflimmeret på motellet og dineren, byen, beitet og flokkene.
- [ ] Bildevalget Off: stjernene, båndet og stjerneskuddet tegnes uten tonekurve i nettversjonen (ShaderMaterial uten tonemapping_fragment); her går de gjennom M_WebTone som med VHS.
- [ ] Floraen: månelyset skaleres ikke med fSky (bare halvkulelyset), og spotlysenes fFace er ikke portert. Low-nivåets kortere tegneavstand for støv og hvilende tumbleweed er tilnærmet.

## Integrasjon etter bølge 4 (2026-10-22)

- [ ] Kapittel 5 (hjelper) og kapittel 6 (Codex, `port-ch6`) flettes inn; kapittel 5 sin slutt inn i `BeginChapter6`, `ch6.clockShow` sist i hjørneklokka, `US47WorldLife::Hooks.SkyLift`/`SunElev` og `US47Away` sine `Ch6*`-kroker.
- [ ] Opptaksvandringen (`S47Recording`) og bortføringens `FS47JacketWalker`/`Recordings()`: to figurtyper; vandringen bruker sin testopptakelse til de deler én.
- [ ] `US47Away::AfterAbduction` er ikke satt: stationTaken, «station passed», opptaksvandringen og parabolene steppes hver for seg (kapittel 3 og `US47SaroNight`), ikke i bortføringens blokk.
- [ ] Arkivets tippede paraboler: `FS47GlowPoints::SetPhase`/`HoldAt` for de røde lampene (verdenens liv).
- [ ] Byens fjernbil: fast pikselstørrelse (`pointScreen`) for billboards.
- [ ] Kapittel 4: papirkunst `cardField` og `letterPaper` mangler i WebData/art; WindowLights på gangen, readAloud og Earls stemme.
- [ ] CRT-gamma (dobbel koding), Serling-fortelleren, jakka «Cold out», kameraet senket på kortet (fra kapittel 1-hjelperen).
- [ ] `US47Night::Dawn` bør lese `Sky->Dawn` i stedet for egen formel.
- [ ] Pakket bygg: full `Scripts/screenshots.sh` etter ny eksport og import, og en gjennomspilling med ekte input og lyd.

## Kapittel seks og avslutningen (2026-10-22)

- [x] Chapter6.ts og Ending.ts i C++, med main.ts sin finale og testhoppene chapter6, chapter6:lastmile og chapter6:line.
- [x] Siste mila, CB-klokka, key/door og kamerastillingene med selvtester; native scenarier ved avreise og på linja, THE EVENT og kamerauttrekket fram til kontrollert tittelbilde.
- [ ] Integrer kapittel fem sin OnEnd og dinerlagring med BeginChapter6; prøv Fortsett fra denne lagringen etter avslutningen.
- [ ] Bind daggry-/soloverstyringene til verdensportens dynamiske himmel og kontroller mesa, soloppgang og stjerner visuelt mot nettversjonen.
- [ ] Pakket bygg, hele screenshots.sh, menneskelig input og lytting; øvrige uferdige motorlydgrafer hører til motorporten.

## Toms ideer (2026-10-22)

- [ ] Kassettspilleren: bytte den egenlagde tape deck-modellen med en bedre modell. NB: opptakeren i «Utstyr lyd UFO» på skrivebordet er pakket ut av «They Are Here»-demoen (UModel); den kan ikke brukes uten å kjøpe originalpakken (Horror Engine). Den kjøpte Vintage Liminal-pakken har kassetter og en Broadcast VCR, ingen kassettspiller.
- [ ] Tonene: en kombinasjon gir en annen låt og låser opp noe (hemmelighet, hint eller spor). Poenget må bestemmes.
- [ ] Ut av bilen overalt (feil) og hopp: under arbeid.
- [x] Egen alien laget i Blender med nye teksturer, 50-beins rigg og idle: `/home/tom/Desktop/Alien - Egen modell/`. FBX/GLB og UE 5.8.4-import består. Gammel modell er urørt referanse. Integrasjon og prøving i pakket spill gjenstår.
- [ ] Terminalene i verden: kameraet zoomer inn på den fysiske skjermen, og skjermbildet vises på skjermmodellen (som i Black Map). Under arbeid for mottakerkonsollen og driftsterminalen.
- [ ] Grafikk: mer detaljerte modeller, bedre lys og skygger (neste store steg).


## Unarius, overlevert til Claude 22. oktober

- [ ] Fortsett codex/unarius-tv fra TV_PROGRAMME.md: bruk siste bygde binærfil med tidsrettelsen, verifiser faktisk bevegelig film på begge TV-er over tid, koble filmlyd om ønsket, prøv fallback og 47-sekundershendelse/opptak/lagring på nytt, integrer og pakk før standardutgaven erstattes.
- [x] Separat egenprodusert Blender-alien er levert i `Desktop/Alien - Egen modell`, med importprøvde Unreal-assets. `Desktop/Alien` er urørt referanse; den nye figuren er ikke integrert i spillet ennå.

## Etter Toms utelys-/fotolabprøve, 22. oktober 2026

- [x] 30 utvendige armaturspotter ved SARO og motellet, navngitte labdrivere og kopling til innflettet fremkallingsregel; bygg, 1218 selvtester og målrettede bilder. Se `LIGHTING.md`.
- [ ] Claude gjennomgår `revision3/compare`, pakker i avtalt kø, prøver lukket dør, rød fremkalling, forstørrer, tåket kopi/gjentakelse og normal lysretur med ekte input; SARO-kutt/tilbake og Room 6s lys.
- [ ] Ny High/1440p `stat unit` med én tung jobb om gangen etter dette utelyspasset. Forrige lysleveranses tall er bevart som historikk, ikke målt på nytt her.
