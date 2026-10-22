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
- [ ] ReadAloud, RecordedDialogue og RadioVoice (stemmebankene under /Game/Audio/Voice, katalogene i Content/Audio/Raw/voices.json), musikken (score.ts) og radioen (radio.ts); `decoder.ts` trenger en ConvolverNode, `score.ts` og `radio.ts` en kilde for lange lydfiler.
- [ ] Lagring ved kontrollpunkter.
- [ ] Kapittel 1 til 6 (web/src/story) og områdene. Verdenene er med i eksporten (AS47Web::Enter).
- [x] Kjøringen (web/src/drive) og turene i World.ts (2026-10-22). Gjenstår: eksporten kjøres på nytt for `drive` i manifestet, lydene laget med Web Audio (dekk, vind, startmotor, ferist), lyktenes mønster (headSlots) i flomlysmaterialet, lyktene på dineren, byen og motellet (shareLights), radioen.
- [ ] Glødepunktene blinker som i glow.ts.

## Kapittel 2 (2026-10-22)

- [x] Kapittel 2 portert (S47Chapter2, S47Annex, S47PhoneScripts, S47Drawings2, S47Panels2), selvtest og gjennomkjøring uten verden.
- [ ] Dørene (Doors.onChange) videre til kapitlenes `DoorChanged`, som Prologue.doorChanged.
- [ ] Lagringen kobles til `US47Chapter2::OnSave` og `Host->LoadCase`.
- [ ] Kapittelkortets ring (`bHoldRing`, `RingOut`, `RingAgain`, `RingShown`) når kortet er portert; kapittel 1 sin slutt kaller `US47Chapter2::AfterChapterOne(method)`.
- [ ] Bryterbordets linjelamper og opptakerens papirstrimmel (archive047.ts) mangler i eksporten.
- [ ] S47Drawings og S47Drawings2, S47Panels og S47Panels2 slås sammen.
- [x] Lagring ved kontrollpunkter, tittel, pause og notatbok (2026-10-22, `S47Saves`, `S47Flow`).
- [ ] Kapittel 1 til 6 (web/src/story), kjøringen (web/src/drive) og områdene. Verdenene er med i eksporten (AS47Web::Enter).
- [ ] Glødepunktene blinker som i glow.ts.

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
- [ ] Feltkameraet og rullen i Kit-fanen: sett `M->Flow->HaveCamera` og `RollText` når kapittel 1 og albumet er portert.
- [ ] Musikk- og radionivået (Music, Radio) inn i lydmodulen når den har egne busser; volumet virker allerede.
- [ ] Worn VHS (bildet `heavy`) i M_WebTone; nå bare Clean og VHS.
- [ ] Fortelleren (Serling) til åpningskortet og kapittelkortet når stemmene er portert.
