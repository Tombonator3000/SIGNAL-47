# Todo for unreal/

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
- [ ] Glødepunktene blinker som i glow.ts.

## Kapittel 2 (2026-10-22)

- [x] Kapittel 2 portert (S47Chapter2, S47Annex, S47PhoneScripts, S47Drawings2, S47Panels2), selvtest og gjennomkjøring uten verden.
- [ ] Dørene (Doors.onChange) videre til kapitlenes `DoorChanged`, som Prologue.doorChanged.
- [ ] Lagringen kobles til `US47Chapter2::OnSave` og `Host->LoadCase`.
- [ ] Kapittelkortets ring (`bHoldRing`, `RingOut`, `RingAgain`, `RingShown`) når kortet er portert; kapittel 1 sin slutt kaller `US47Chapter2::AfterChapterOne(method)`.
- [x] Bryterbordets linjelamper og opptakerens papirstrimmel (archive047.ts): i export.js (2026-10-22); kjør eksporten på nytt for å få håndtakene (C++ finner dem uten).
- [ ] S47Drawings og S47Drawings2, S47Panels og S47Panels2 slås sammen.
- [x] Lagring ved kontrollpunkter, tittel, pause og notatbok (2026-10-22, `S47Saves`, `S47Flow`).
- [ ] Kapittel 1 til 6 (web/src/story), kjøringen (web/src/drive) og områdene. Verdenene er med i eksporten (AS47Web::Enter).
- [ ] Glødepunktene blinker som i glow.ts.

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

## Caliche, veien og motellets natt (2026-10-22)

- [x] C++-port av Town/TownEvents, Roadside, Herd/Pasture, NinthModel/NinthCow, MotelTv, MorningHolds/DinerCooler, Cutouts, Caliche og Diner med main.ts-koblinger, lagringsfelt og testflagg.
- [x] Caliches RecordedDialogue/NpcDialogue/EarlDialogue, innspilte replikkøer, replay og stopp. Generell ReadAloud/Serling gjenstår hos fortellerporten.
- [ ] Regenerer webeksport og import for byens delte lamper/glødepunkter, fjernbilens lys, townSlots/neonSlots og herdets beamLit. Implementer pointScreen i native billboards.
- [ ] Koble LostNights til Caliche.Reel og kapittel fem til TownLayer.Diner. Bygg og prøv kapittel tres FreeShot etter integrasjon; kapittel 3/5 finnes ikke i grenens utgangspunkt.
- [ ] Samlet pakket bygg, hele screenshots.sh, Continue fra disk, ekte input, menneskelig lytting og sammenligning av de regenererte bildene mot nettversjonen. Se TOWN_PORT.md.
