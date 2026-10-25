# SIGNAL / 47 i Unreal

Hele SIGNAL / 47 i Unreal Engine 5.8.4: prologen «Night Shift», kapittel 1 til 6, natta med alle hendelsene, kjøringen og slutten. Videre spillarbeid gjelder desktoputgaven i Unreal. `web/` og Unity er historiske referanser. Verden hentes ut av nettversjonen selv, og all spillogikk er portert til C++ fil for fil fra `web/src`.

Startet 21. oktober 2026 etter Toms spørsmål om en Unreal-versjon. Prologen kom først; 22. oktober ble resten av spillet portert (Tom valgte en ren C++-port framfor å kjøre nettversjonens kode i en innebygd nettleser).

Gjeldende retting og teststatus: [QA_FIXES_2026-10-25.md](QA_FIXES_2026-10-25.md). Q3 består 110/110 skriptede scenarioer, 1 424 selvtester og komplette High/Low-finaler helt til meny. Feltfoto, materialbindinger, utgang ved lagringsfeil, bindingsnavn, tekststørrelse og P13 er rettet. VSM-profilering og menneskelig fullnatt med lyd/fysisk Xbox står fortsatt åpne. [Før-rapporten fra PR157](SPILLTEST_2026-10-25.md) beholder de opprinnelige krasjene og feilene.

PR158 er flettet, og samme Q3 er installert med 40 verifiserte payloadfiler og 40 uendrede Saved-filer. Installert oppstart med egen profil består. Gjeldende pakke og to eldre komplette testbygg beholdes etter kontrollert opprydding.

Samlet videre plan for historie og skrekk: [HORROR_STORY_PLAN.md](HORROR_STORY_PLAN.md). Kosmisk etterforskning, gradvis kommende invasjon, stabile skyggemannregler og en faktisk skjul/flukt-sekvens ved SARO. Detaljerte oppdrag og produksjonsetapper H1 til H6 ligger øverst i [todo.md](todo.md). Dette er planlagt for senere implementering, ikke nye funksjoner i dagens spill.

H1.2 til H1.4 er nå produsert som engelske forfatterkilder i [Narrative/HorrorStory](Narrative/HorrorStory/README.md): ti oppdrag, 36 scener og fem dokumenter med validert generator. Ingen ny runtimehistorie eller nye opptak er hevdet. Neste konkrete runtime-inkrement er den fulle T01-service-/returreisen; utkastet holdes adskilt fra Q3 til rettelsen er levert.

Ny fortellerretning og manus: [ANOMALY_PLAN.md](ANOMALY_PLAN.md) og [kveldspakken](Narrative/EveningShift/README.md). Pakken inneholder innkjøring/diner, Dales telefon-/radiodialog, kontorfunn, dokumenter og VHS-manus. Innkjøring, telefonoverlevering og ny tidslinje har runtime i kandidaten på `codex/evening-runtime`; se [runtime-rapporten](EVENING_RUNTIME.md). Senere kontor-/felt-/filminnhold er fortsatt forfattergrunnlag. Claude har siden integrert og installert runtimeåpningen. De 109 nye takesene er koblet til lydløypa på samme gren; se [lydrapporten](EVENING_AUDIO.md). Codex oppdaterer ikke installasjonen.

Første anomaly-pilot på `codex/anomaly-pilot` gir N01/N02/A02/P01 med kamera, aktive sammenligninger, fysisk oppfølging og lagring. [ANOMALY_RUNTIME.md](ANOMALY_RUNTIME.md) skiller kilde, pakket kandidat, testbevis og gjenstående menneskelig prøve. Ingen installasjon eller publisering inngår i denne leveransen.

## Spill

```sh
~/Games/Signal47/Linux/Signal47.sh
```

Eller «SIGNAL / 47» i programmenyen. Spillet starter på tittelen (Continue, New night, Load case, Settings). WASD går (Shift løper), musa ser, E eller klikk bruker det du ser på, høyre mus ser nærmere. Tab eller N er notatboken (M, I og H åpner kartet, kitet og kontrollene i den), Esc eller P er pausen med innstillinger, lagring og lasting. I bilen: W og S gass og brems (S i stillstand rygger), A og D styrer, R eller klikk på radioen skifter stasjon, E i stillstand går ut (og vrir om nøkkelen når motoren har dødd). V leser beskrivelsen av det du ser på når opplesningen er slått på i innstillingene. Lagringene og innstillingene ligger ved siden av spillet, i `~/Games/Signal47/Linux/Signal47/Saved/SaveGames`.

Takeaway kan settes ned og plukkes opp med E/A på merkede plasser i dineren, ved bilen og på SARO. B på Xbox eller Q på tastatur åpner matmenyen med nedsetting, bortstuing, fremtaking og kasting. D-pad/piltaster velger og A/Enter bekrefter. Etter telefonoverleveringen tilbyr skrivebordet og annexbordet **Eat supper (12 minutes)**. Posen skjules ved verktøybruk, og plassering/måltid lagres. SARO har en egen besøksplass før skiltet, atskilt fra servicebilen. Se [SARO-ankomst og matstyring](SARO_ARRIVAL.md) for aktuell kandidat og teststatus, og [den opprinnelige takeaway-flyten](EVENING_TAKEAWAY.md).
Xbox-styring er lagt til med analoge stikker, biltriggere og menyer/paneler. Se [kontroller og faktisk teststatus](GAMEPAD.md). Hastighet og invertert Y finnes under Settings > Controls.

## Last ned på en annen Linux-maskin

[Linux-testutgave 23. oktober 2026](https://github.com/Tombonator3000/SIGNAL-47/releases/tag/playtest-linux-2026-10-23). Repoet og utgivelsen er private; logg inn med en GitHub-konto som har tilgang. Last ned begge ZIP-filene under Assets, pakk dem ut i samme målmappe og start `Signal47-Linux/Play.sh`. `Play-Low.sh` gir 1280 x 720, Low og Clean. Utfyllende instruksjoner og kontrollsummer følger utgivelsen. Steam Deck/SteamOS og gamepad er ikke testet. Pakken inneholder ingen personlige lagringer, logger eller innstillinger.

## Kom i gang

Unreal Engine 5.8.4 for Linux må ligge i `~/.local/opt/UnrealEngine-5.8.4` (eller `UE_ROOT`). Samme oppsett som THE BLACK MAP. Den private klonen av assets må ligge i `web/src/assets/licensed` (som for nettversjonen), og venv-en `~/.venvs/signal47` med Playwright må finnes.

```sh
cd unreal/Signal47
Scripts/build-editor.sh        # kompilerer editor-målet (ikke mens editoren er åpen)
Tools/webexport/import.sh      # bygger nettversjonen, eksporterer scenen fra nettleseren og importerer den (rundt 25 min)
Scripts/import-audio.sh        # all lyd og musikk til Content/Audio og Content/Licensed
Scripts/package-linux.sh       # pakket build til ~/scratch-bm/Signal47-build
Scripts/screenshots.sh UT      # 110 scenarioer med skjermbilder og loggkontroll
~/scratch-bm/Signal47-build/Linux/Signal47.sh   # spill (-S47Picture=off uten VHS-bildet)
```

Spillet starter på tittelen (Continue, New night, Load case, Settings); med et `-S47`-flagg går det rett i spill som før. WASD går (Shift løper), musa ser, E eller klikk bruker det du ser på. Tab eller N er notatboken (M, I og H åpner kartet, kitet og kontrollene i den), Esc eller P er pausen med innstillinger, lagring og lasting. Lagringene og innstillingene ligger i `Saved/SaveGames`.

## Slik kommer nettversjonen inn

Verden bygges ikke for hånd. `Tools/webexport/export.py` starter nettversjonen i Chromium, gjør klar alle områdene og henter hele three.js-scenen ut med `export.js`: hver node med matrise og synlighet, all geometri som glb, hvert materiale med farger, kart og UV-matriser, alle teksturer, lysene, de falske flomlysene (`kit.ts floodlit`), glødepunktene, dørene med åpen og lukket stilling, antennene, alt som kan brukes, gangsonene og kolliderne per område, og himmelen per område som et ekvirektangulært bilde (8192 x 4096). `Tools/webexport/import_web.py` gjør det om til Unreal-assets under `/Game/Web` (ignorert av Git) og kopierer `manifest.json` til `Content/WebData`.

Ved oppstart leser `AS47Web` manifestet og bygger scenen node for node på de samme stedene. Belysningen kommer fra native punktlys, spotter, vinduslys, måne og skylight, med skygger, volumetrisk tåke og Lumen på High/Ultra. Historienes eksisterende lampetilstand driver disse lysene. SAROs rectlys sitter i de faktiske diffuserne og lampeskjermene, hver registrerte skjerm har sitt eget grønne flatelys, og arrayets feltspotter lyser varmt på parabolene. Materialenes gamle flood- og halvkulebidrag er av; `M_WebTone` beholder ACES-kurven og VHS/Clean med fast eksponering, mens tåka tegnes av Unreal. Se `LIGHTING.md` for importsteg, sammenligninger og faktisk teststatus.

Spillogikken bruker nettversjonens koordinater (meter, y opp); bare tegningen regnes om. three (x, y, z) er Unreal (100x, 100z, 100y).

## Det som finnes

- `Source/Signal47`: C++-modulen.
  - `AS47Web` er nettversjonens verden, bygget fra eksporten, med setterne historien trenger (spaken, lampene, CRT-ene, skriveren, telefonrøret, kruset, kaffen, klokka, lysrørene, antennene, dørene).
  - `S47Three` regner mellom three.js og Unreal (matriser, Euler-vinkler, kamera, farger).
  - `AS47Story` er prologen «Night Shift», portert fra `web/src/story/Prologue.ts`: de samme fasene i samme rekkefølge på spilltid som stopper når noe leses, og de samme plasseringene.
  - `S47Docs`, `S47Ops` og `S47Rx` er dokumentene, operasjonsterminalen og mottakeren med spekteret, portert ord for ord og tall for tall og sjekket mot nettversjonen (`-S47SelfTest`).
  - `AS47HUD` og `SS47Widgets` er overlegget, papirene, notatboken, mottakerkonsollen, operasjonsterminalen og sluttkortet i Slate. `SS47Crt` tegner de fire CRT-ene til render targets.
  - `US47Audio` er `core/Audio.ts`: opptakene ved navn og alle lydene nettversjonen lager med Web Audio (bærebølgen, signalet, telefonlinja, radioene, tordenen, natta ute), portert graf for graf på en liten Web Audio i C++ (`S47WebAudio`, `S47SignalVoice`, `S47Ambience`), sjekket mot Chromiums egen gjengivelse.
  - `AS47Walker` er spilleren fra `player/Player.ts`: gangsonene og kolliderne, øyehøyde, gange, løp og blikk som i nettversjonen.
- `Scripts/import-audio.sh`: all lyd (frie lyder, kjøpte lyder, musikk, radio, stemmebankene) med ffmpeg til `Content/Audio` og `Content/Licensed` (begge ignorert av Git), pluss tonene fra `Scripts/make-synth.py`. Først skriver `Scripts/audio-raw.py` opptakene lydmotoren dekoder selv, med oppslagstabellene, til `Content/Audio/Raw` og `Content/Licensed/Raw` (bare python3 og ffmpeg, ingen editor).
- `Content/Fonts`: VT323 (OFL) og Special Elite (Apache 2.0) som i nettversjonen, med lisensene, og JetBrains Mono (OFL) som reserve.

## Regler

Se `AGENTS.md` i denne mappa. Kort: Unity-mappa er arkiv og røres ikke, kjøpte eller lisensierte assets legges ikke i det offentlige repoet, spilltekst på engelsk, dokumenter på norsk, og alt logges i `log.md`.

## Begrensninger

- Hele spillet er portert og kjører i den pakkede utgaven (`Scripts/screenshots.sh`: 86 kjøringer fra prologen til slutttittelen). Det er ikke spilt gjennom av et menneske ennå: lyden er sjekket i tester, ikke med øret, og ekte mus og tastatur er bare prøvd i korte økter. Avvik fra nettversjonen er kommentert i koden og samlet i `todo.md`.
- Ni shader-materialer fra nettversjonen (vann, enkelte himmeleffekter) er skjult. Himmelen tegnes levende (`S47Sky`: kuppelen, stjernene, stjernehullet, stjerneskudd, kometen og den malte horisonten); bildet fra eksporten brukes bare når livsmaterialene mangler.
- Museretningen opp og ned er ikke prøvd av et menneske.
- Bare testet på denne maskinen (RTX 3080, Vulkan, Ubuntu 26.04).
