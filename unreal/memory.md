# Minne for unreal/

Beslutninger og fallgruver. Dagbok hører hjemme i log.md.

## Oppsett (2026-10-21)

- Prosjektet heter Signal47 og ligger i unreal/Signal47. UE 5.8.4 (binærutgaven for Linux) i ~/.local/opt/UnrealEngine-5.8.4, Vulkan SM6, Lumen i programvare, Nanite av for de importerte modellene.
- Pakkede bygg legges utenfor repoet, i ~/scratch-bm/Signal47-build, så Artifacts-regelen i rotens .gitignore ikke berøres.
- Akser: X nord, Y øst, Z opp, cm. Nettversjonens (x, y, z) i meter er (Y, Z, -X) ganger 100. Vinduet i nettversjonen ligger ved z -4.65 (nord), altså X +465 her.
- Rommet er 1200 (Y) x 900 (X) x 300 cm, vindu fra Z 95 til 275 langs hele nordveggen med sprosser hver 180 cm, dør mot sør ved Y -290 til -190 og dør mot øst ved X -210 til -110.
- Modellene importeres med det gamle FBX-importeren (FbxImportUI), uten materialer; hvert materialspor får en MaterialInstanceConstant av M_S47Flat med fargen fra Unity-materialfilen med samme navn (_BaseColor, 1 - _Smoothness, _Metallic, _EmissionColor).
- Fonten VT323 finnes bare som woff2 i web/. Konvertering krever fontTools med brotli, som ikke er installert. Reserve: JetBrains Mono fra THE BLACK MAP (OFL).
- Stjernehimmelen er en tekstur laget ved oppstart (UTexture2D::CreateTransient) på innsiden av en kule med skala 400; fiktiv himmel uten katalog.
- Eksponeringen er manuell (bias 0, fysisk kamera av) så natta blir mørk med vilje. Lamper i candela: taklys 7 til 9, skrivebordslampe 3, gårdslampe 14. Månen er et retningslys på 0,9 lux.

## Prologen (2026-10-21)

- AS47Story er en direkte port av Prologue.ts. Spilltid (GameTime) stopper mens et papir, konsollen, terminalen eller notatboken er åpen (HUD-en setter Story->SetPaused). Tidsplanen (After) går på spilltid.
- Historien når tingene i rommet bare gjennom AS47Saro sine settere og PropLocation(id). Nye ting får Tag(actor, id) og en linje i Label() og Use() i S47Story.
- De fire CRT-ene er render targets på 512 x 384 som HUD-en tegner tolv ganger i sekundet med FWidgetRenderer (SS47Crt). Skjermflaten er en tynn boks med M_S47Screen (Glow 0 når skjermen er av).
- Lyder slås opp ved navn i /Game/Licensed/Sfx, så /Game/Audio/Sfx, så /Game/Audio/Synth. Mangler en lyd, logges «no sound named» én gang og spillet går videre.
- (2026-10-22) Lyden er nå nettversjonens Web Audio portert til C++ (S47WebAudio): én kontekst på 48 kHz, spilt av én USynthComponent, med egen panorering mot lytteren (spillerens øye, satt i US47Audio::Tick). Opptakene dekodes fra WAV i Content/Audio/Raw og Content/Licensed/Raw (index.json). Et filnavn fra prologen (thud_soft) går gjennom grafen når opptaket finnes der, ellers som før til lydassetene.
- Lydmotoren følger Chromium der nettleseren avviker fra spesifikasjonen, fordi det er slik nettversjonen høres: en løkke gjennom en DelayNode er én kvant (128 rammer) lengre enn forsinkelsen, firkant og sagtann har toppen 1 etter Gibbs (nivå 0,848), og kompressoren er Chromiums DynamicsCompressorKernel. Kryssjekket mot Chromiums OfflineAudioContext: 1e-4 eller bedre på konvolutter, biquader, panorering, avspilling, kompressor og kam.
- Lyd-API-et tar låsen per kall; hold FS47AudioContext::FBatch over en hel graf. Bygg lange buffere (amRadio-melodien) før låsen tas, så lydtråden ikke venter.
- De kjøpte glb-modellene importeres av Interchange til /Game/Licensed/Models/<pakke>/StaticMeshes/<mesh> (crt_terminal har crt_terminal og crt_keyboard).
- Et medlem og en konstant kan ikke hete det samme (RelayFirst): clang tar medlemsfunksjonen.

## Tegningene og kapittel 2 (2026-10-22)

- Lerretstegningene (drawings.ts) tegnes med Slate i et render target og leses tilbake: `FWidgetRenderer(false)` (uten gammatrinnet) og et vanlig 8-bits mål, fargene med `FLinearColor::FromSRGBColor`. Da kommer lerretets bytes ut som de er og blandes som lerretet blander. Med `FWidgetRenderer(true)` blir fargene sRGB-kodet to ganger (papiret #efe8d6 ble #f8f5ec).
- Slates kantutjevnede linjer toner ut over to piksler, lerretets over én. Tegningene tegnes derfor i dobbel størrelse og midles ned.
- Nettversjonens Oswald er ett statisk snitt, Medium (500), oppgitt for vektene 200 til 700 (core/fonts.ts), så nettleseren tegner all Oswald i Medium, også titlene som ber om 600. `Content/Fonts/Oswald-Regular.ttf` er en variabel font som Slate tegner i standardsnittet 400. `Oswald-Medium.ttf` er 500-snittet av den, laget med fontTools (`varLib.instancer`, uv-venv i scratch); tegningene bruker den. `S47Style::Display` bruker fortsatt Regular.
- Redigeringsprogrammet kan kjøre spillet uten vindu og uten den genererte verdenen: `UnrealEditor Signal47.uproject -game -RenderOffscreen -nosound -S47SmokeTest -S47SelfTest`. Det holder for selvtesten og for logikk som ikke trenger verdenen.
- Lydnavnene i kapittel 2 er nettversjonens (`phoneRing`, `click`) gjennom `Sfx` og `LoopSfx`; stand-ins i S47AudioWeb gir `PhoneCall` unset, så tekstingen går, og `Ringback` regner ennå ikke med starten sin.
## Lagring og flyt (2026-10-22)

- Lagringene ligger i Saved/SaveGames: save-<sak>-<auto|manual>-<plass>.json med kortets felt og `state` (GameState v2 som i nettversjonen), bildene i photos/<nøkkel>.txt og referert som `idb:<nøkkel>` som i nettversjonen, active.json er saken som ble spilt sist, settings.json er innstillingene (main.ts store).
- Det som ennå ikke er portert (bilen, lommelykten, bortføringen, albumet, kurven, Caliche, ns.ln, ns.array, ns.hondo, kapitler uten C++-port) leses fra lagringen som åpnes og skrives tilbake uendret (`US47Flow::Carry`).
- Pausemenyen stopper verden med SetGamePaused, og lyden pauses med den. Verdenens tidtakere står da stille, så røyktestens tidtakere går på FTSTicker.
- Ethvert -S47-flagg unntatt -S47Picture og -S47Title starter rett i spill som før; uten flagg starter spillet på tittelen.
- I UE 5.8 har FJsonObject::Values nøkler av typen UE::TSharedString: `const auto&` i løkker, `FString(*Key)`.
- AActor har en virtuell Reset(); `AS47Story::Reset` er Prologue.reset og overstyrer den.
- Uten den genererte verdenen kan prosjektet prøves med UnrealEditor <uproject> -game -RenderOffscreen (tom verden): menyene, notatboken og lagringen virker.
## Kjøringen (2026-10-22)

- Kjøringen går i AS47Walker sin tick (CameraOverride → US47World::Frame), så bilen og kameraet flyttes i samme bilde; Unreal har ingen fast rekkefølge mellom gåeren og HUD-en.
- Bilens gruppe roteres i Euler YXZ (stigning, kurs, krengning) som i Drive.ts; FS47Truck holder Position og Rotation selv, så World.ts sin `rotation.y` er kursen.
- Et render target tegnet her trenger SetMap(rt, true) (radene motsatt vei av de eksporterte bildene), som CRT-ene.
- clang godtar ikke en struct med standardverdier inne i en UCLASS som standardargument; FS47TripOpts ligger derfor utenfor US47World.

## Radio og musikk (2026-10-22)

- `S47Radio` / `S47TripRadio` / `S47RadioVoice` / `S47Score` er portene av de tilsvarende TypeScript-filene. Innkoblingen er i Main, World og Flow. Prologens bordradio bruker fortsatt sin egen `amRadio`, som i nettversjonen. Tittellåten bruker lydmodulens `music`-løkke og musikkbussen, utenom Score.
- Lange opptak ligger som PCM16 WAV i `Audio/Raw/media/{music,radio}` og går gjennom FS47MediaSource inn i den samme Web Audio-grafen. Ikke legg dem i Audio.Buf: musikken alene ville brukt omtrent 1,4 GiB som float. Kilden leser 32768 rammer per bit, opptil fire biter, med videre innlesing på bakgrunnstråd. Hold aldri en FBatch rundt Open/Seek eller innlasting av stemmeklipp.
- `audio-raw.py --recordings-only` oppdaterer media, de fire RadioVoice-bankene og katalogene uten å erstatte eksisterende sfx-/telefon-/kjøpt-lydindeks. Kjørt mot hovedkopien: 22 lange opptak og 259 stemmeklipp. Alt under Audio/Raw er allerede staged som UFS.
- `M->TownSeen` inneholder blant annet radioens `ev:boy`, `ln:reel` og `tr:<id>` og lagres i nettversjonens `town`. Øvrige nattsystemer må dele dette settet. `Score->Held` har navngitte funksjoner for deres tilstand inntil de porteres og kobles inn.
- Radioens tekst bruker lydkontekstens klokke i `UI->Toast`, slik at en pause holder både innspilte ord og tekst. Selektiv `Disconnect(destination)` slipper bare den ene mottakeren.
- `-S47Radio=caliche|roswell|far` plasserer en testmottaker ved kameraet gjennom den vanlige grafen og logger stasjon, frekvens, segment og lyttere. `screenshots.sh` har fått en radio-kjøring. Se `RADIO_PORT.md` for tester, native avvik og integrasjonsarbeid.

## Verdenens eget liv (2026-10-22)

- `US47WorldLife::Step` kalles sist i `US47Main::Step`, i main.ts sin rekkefølge for steg 13: himmelen, lampene, de delte lysene, tåkefargen, horisonten, så områdenes oppdateringer.
- Himmelens gruppe tegnes ti ganger så langt ut som i three (kuppelen på 30 km); tonepassets tåke stopper ved 25 km (SKY_CUT). Gjennomsiktige ting i himmelen sorteres med prioritet (stjerner -4, komet -3, stjerneskudd -2, bånd -1).
- Lys som legges oppå bildet (kjegler, vinduslys, møll, støv) ganges med DEFOG = 1 / (1 - tåka bak), fordi tonepasset tåkelegger hele bildet etter dybden bak.
- Det three tegner med fog: false og som er ugjennomsiktig, tegnes inn i custom depth (r.CustomDepth=3): stencil 1 for de nære mesaene (horizon.ts hazed: bare daggryets dis, MPC HorizonHazeK), 2 for resten (SAROs dishes sett fra den gamle veien). M_WebTone og DEFOG leser dem.
- VertexColor-uttrykket har ingen navngitte utganger: koble med '' (RGB). Et navn den ikke har (`RGB`, `A`) gir en ukoblet inngang uten feilmelding, altså hvitt. Alfa leses i en Custom-node (`Parameters.VertexColor.a`). En VectorParameter i en Custom-inngang gir float3; bruk en float4-node når HLSL-en venter fire verdier.
- `UInstancedStaticMeshComponent::SetCustomData(Start, End, ...)`: End er den siste indeksen, ikke én forbi.
- ISM med per-instans-data i et materiale: `NumCustomDataFloats` før instansene, og PerInstanceCustomData i pikselshaderen virker.
- Floraens tilfeldighet (mulberry32, hash4, vnoise) må regnes i uint32 med nettversjonens overflyt; selvtesten sammenligner hver rute med `life.tests` i manifestet.
- `-ExecCmds="r.SetRes 1600x900w"` gir -RenderOffscreen-kjøringer i 1600 x 900 (ellers 888 x 500), så de kan legges ved siden av nettversjonens bilder.
- Python-skript i et skrapområde må ikke hete som standardmoduler (`inspect.py` stoppet Playwright).
