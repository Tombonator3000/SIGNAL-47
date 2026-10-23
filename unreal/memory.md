# Minne for unreal/

Beslutninger og fallgruver. Dagbok hører hjemme i log.md.


## Grafikkpass under arbeid, 23. oktober 2026

- `port-gfx` fortsetter WIP `64d943e7`, med `unreal-port-core` ved `93ab1568` innflettet. Se øverste del av `LIGHTING.md` for gjeldende kildekandidat og nøyaktig teststatus. Ingen ny pakke eller installasjon er laget i denne fortsettelsen.
- WIP-ens startup-vernet lå feil i Web::BeginPlay og hoppet over verdensbyggingen. Det ligger nå i Tick. PSO-readiness må telle filcache, RHI, komponenter og shaderjobber; testklokker starter ved READY. Rå veggklokke og synlige sekunder er forskjellige målevinduer.
- `Scripts/unreal_guard.py require-idle` viser `free -g` og `pgrep`, krever minst 10 GiB MemAvailable og ingen tung Unreal-prosess. Bygg/pakking/skjermbildeskript bruker vernet. Toms installerte spill stoppes aldri. Grafikkverktøyene avbryter bare egne prosesser dersom noen starter en annen tung jobb under måling.
- Nye materialer finnes foreløpig bare som importkode. 118 opt-in overflater, seks lit glass og fire kontrollroms-CRT er kontrollert offline. Kelvora-filmflatenes verifiserte unlit-kontrakt beholdes. Regenerering, cook, bilder, kald/varm PSO-cache og 1440p-ytelse gjenstår.

## Gameplayretning (2026-10-23)

- Tom sluttet seg til aktiv anomaly spotting, mer praktisk bruk av Caliche og gradvis UFO-/alienopptrapping, og ba om en utdypet plan. Se `ANOMALY_PLAN.md`. Detaljene er anbefalt design; ny gameplay er ikke implementert.
- Første normalbesøk anbefales i ch1/tidlig ch2 rundt 02:40–02:55, ikke først i Survey Station. Trucken kan brukes etter prologen. Earl går 04:00; ekspeditørens gjentakelse krever minst to timer. Sene oppgaver kan ikke kreve Earl, kvinnen eller suvenirmannen.
- Gatetelefonens to hendelser og dinertelefonens Ward-samtale har forskjellige krav. A gjennom byen er ikke C ved finalen. Nye byankomster må ikke starte ch5 eller den gamle veien.
- Første pilot er N01/N02/A02/P01 i planen: kontrolltur, normalreferanser, et forklarlig kontrolltilfelle, aktiv bildesammenligning og et kort lydetterspill. Dagens lysbølge og 04:55-grense beholdes i denne piloten. Full natt, nye UFO-er og aliensekvens kommer i senere trinn.
- Pilotens 20–30 minutter er to avgrensede testdelstrekninger med bevis gjennom lagring og tydelig tidskutt. Det beviser ikke hele nattens tids-/kapittelkontinuitet. Runtime, menneskelig opplevelse og ny ytelse er UNVERIFIED.


## Native belysning (2026-10-22)

- Videre belysning er bare Unreal. `S47Lighting.cpp` gjør eksportens lampetilstand om til punktlys og spotter. Historienes SetFlood-API er beholdt; materialenes flood, hemisphere, FogExp2, additive lyskjegler og vindusprojeksjoner er av. Ingen Web/Unreal-bryter. Eldre lysnotater nedenfor beskriver porten før denne endringen.
- Lumen bruker Vulkan hardware ray tracing på RTX 3080 når tilgjengelig. Måne, skylight, levende daggry, native volumetrisk tåke og skygger fra lommelykt/billykter erstatter fylltermene. Førerhusets gamle himmelfyll er fjernet; instrumentgløden er svak. Fast eksponering, ACES og VHS/Clean beholdes.
- `S47_ONLY=materials` i import_web.py regenererer Base og Mat uten å skrive Tex, Geo, art eller WebData. Kreves for eldre generert Content. NativeLighting har asset-default 0 for delte historiske checkouter, men denne runtime setter alltid 1. Bruk private materialkopier ved parallelle jobber.
- Low bruker direkte lys, High/Ultra Lumen og volumetri. High/Low bruker 60 % intern oppløsning med TSR; Ultra 100 %. Se `LIGHTING.md` for faktisk ytelse og sammenligningsbilder, ikke trekk fps-konklusjoner fra kvalitetsnavnet.
- Rettelsesrundens High/1440p med TSR fra 60 %: rom 13,619 ms / 73,4 fps, gård 10,466 ms / 95,5 fps, p95 15,330/11,527 ms. Ingen andre tunge prosesser i målevinduet. Low/Clean 90,0 fps i rommet, Ultra 39,2 fps i gården. Faste editorutsnitt; pakket bygg og bevegelse gjenstår. Se LIGHTING.md.
- Før tung kjøring må også prosessnavnet `Signal47` sjekkes. Den installerte Linux-binærfila heter dette, ikke nødvendigvis Signal47-Linux-Shipping. En slik prosess gjorde første ytelsespass ugyldig selv om UnrealEditor-kontrollen var tom.
- Kopiert UBT Intermediate inneholder absolutte stier. Lag ny Intermediate i et worktree, og kontroller kompilatorens response-filer. `Scripts/build-editor.sh -NoUBA` brukes her etter UBA-feil på PCH-rename gjennom symlink. Byggskriptet videresender valgfrie flagg.

- Taklys i SARO ligger i mesh-diffuserne, med rectlys og PBR-emissive fra samme tilstand. Rommets gamle PointLights er tilstandsdrivere uten bAffectsWorld. Fire CRT-er og øvrige registrerte skjermer har egne grønne rectlys; HUD-ens Ops power-kall overstyrer ikke SARO mørkt, fordi faktisk Brightness også leser stasjonsstrømmen.
- GLB-bokser kan ha slot0..5 selv når N.Mats har én oppføring. Lysrørets underside er slot3. FixtureLighting binder alle diffuserflater til samme MID og følger NodeMids sine senere kopier; ikke oppdater bare materiale0 og la gamle emissive-flater stå igjen.
- Annex.flickerTube ligger ved pool3, mens portens FlickerFlood er4. Den fysiske koblingen remapper dette til det faktiske røret. Endre ikke historietidsplanen for å rette lyskilden.
- Arrayets første fem pools ligger ved Y3, men armaturene ved Y6,5. Native spotter står på armaturene og sikter mot skålene. Seks andre eksisterende feltarmaturer bruker sine eksporterte glødepunkter; de deler arrayets strøm/rødpuls. Ingen nye modeller.

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

## Caliche, veien og motellets natt (2026-10-22)

- `US47TownLayer` samler main.ts-koblingene til Town/TownEvents, Roadside, Herd/Pasture, NinthModel/NinthCow, MotelTv, MorningHolds/DinerCooler, Cutouts, Caliche og Diner. Modulene deler `Main.TownSeen`; TV-bildet ligger i `GameState.motelTv`. Trinn og restore er delt inn slik at telefonene og SameThread beholder main.ts-rekkefølgen.
- Caliches stemmer bruker `RecordedDialogue`, `NpcDialogue` og `EarlDialogue` gjennom RadioVoice og den eksisterende Web Audio-konteksten. LostNights må koble `Caliche.Reel`; kapittel fem bruker `TownLayer.Diner`. Kapittel tres `FreeShot` kobles når kapittelheaderen er tilgjengelig.
- Byens bilkollider må oppdateres både i AS47Web og i kjørekontrollerens verdikopi (`World.TownCarMoved`). Direkte områdehopp trenger Player.floor-funksjonen før Place; ellers kan en lav plassering fra SARO bli liggende under Caliches terreng.
- Stopp lydgrafer i native destruktorer uten å gå gjennom Main/Audio/Web. Under editorens avslutning kan Webs nodetabell allerede være frigitt. Roadside og TV har derfor egne nodebaserte stopp i destruktorene.
- Ny eksport/import kreves for `world.town.runtime.*`, `roadside:car`, delte lysplasser og herdets beamLit-materialparametre. Ikke erstatt disse med håndbygd geometri. `pointScreen` er ennå bare eksportmetadata. Se TOWN_PORT.md for nøyaktige grenser og testflagg.
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

## Kapittel seks og slutten (2026-10-22)

- `US47Chapter6` er Chapter6.ts: uspart kjøring, siste mila med CB og avstandsklokke, THE EVENT på 47 sekunder, nøkkel/dør, figuren og kamerauttrekket. `ClockAfter` har ch6 sist, og Away-krokene ivaretar main.ts sin E/Q-rekkefølge. `World.OldControl.Camera` går gjennom eksisterende `Walker.CameraOverride`; Walker endres ikke.
- `US47Flow::Finale` stanser 3D-spillet og bruker `SS47Ending` sine sanntids-callbacks til tittel, kreditering, fotografi og `ToTitle`. Ch6 har ingen SaveState eller checkpoint. Siste dinerlagring skal være Fortsett; den virkelige koblingen fra kapittel fem må integreres når dens port finnes.
- `AS47Web` har en merket sluttblokk med faktisk tåkeoverstyring og daggry-/solverdier med hooks. Verdensporten må binde hooks eller lese getters. Et statisk `M_WebSky`-panorama kan ikke tegne soloppgangen eller høydeavhengig daggry; ikke kall den visuelle delen ferdig med bare disse setterene.
- OldRoad-konstruktøren legger figurens sju glødepunkter sist i `world.oldRoad.glow.points`. De gjenbrukes direkte; ingen ny veigeometri eller eksport er nødvendig for kapitlet. `US47Away::ShowFrame` har valgfri gruppeopacity for Ending, mens bortføringen beholder sitt svarte underlag.
- Integrasjonskontrakter og faktisk testgrunnlag står i `CH6_PORT.md`.

## Retning og lokale TV-klipp (2026-10-22)

- Tom har bestemt at videre arbeid kun gjelder Unreal. Web og Unity er referanser.
- Unarius-MP4-er ligger i `/home/tom/Desktop/Unarius/`. Film skal bevege seg på TV-ene. Lokal kandidat, status og videreføring står i `TV_PROGRAMME.md`.

## Egen alien, 22. oktober 2026

- Ny figur med Blender-kilde, 2K PBR, rigg, idle og importprøvde UE 5.8.4-assets ligger i `/home/tom/Desktop/Alien - Egen modell/`. Se README der for import og rapporter. Gammel `/home/tom/Desktop/Alien/` er urørt referanse. Ny modell er klar som ressurs; integrasjon i pakket spill gjenstår.

## KELVORA-rekvisitter, 22. oktober 2026

- Egen CRT-TV og slitt radio i `/home/tom/Desktop/Retro TV og radio/`, med PBR, FBX/GLB, LOD1/2 og importprøvde Unreal-assets i `Unreal/Content/Kelvora/`. TV-skjerm er materialplass 1 med eksisterende filmteksturparametre. Assetene er ferdige; plassering og pakket film-/radioprøve gjenstår hos integrator.

## KELVORA plassert 22. oktober 2026

- `S47RetroProps.cpp` bygger egne TV-er ved `world.motel.tv.group` og `world.room6.objs.tv`, radio ved `room.objs.radio`. `BuildRetroProps` kjøres etter BuildRoom; nye barn følger områdenes synlighet via AdoptNode og gamle bruk-ID-er. Skjermenes egne MID-er registreres på de eksisterende materialhåndtakene, med startverdier fra gamle MID-er.
- FBX-screen UV0 er snudd i V av importøren. Eget unlit skjermmateriale inverterer V før EmU/EmV, fordi de eksisterende canvas-tegnerne setter EmV=(0,-1,1). Ikke fjern dette eller innfør ekstra film-vending. CRT-kildens ringpolygoner hadde motsatt retning av senteret; alle er nå vendt ut og testet i alle LOD-er. Unlit unngår at TV-ens eget punktlys dekker filmen med en hvit refleks. Piloten sjekker RGB, ikke alfa.
- `Content/Kelvora/Office` og `office-patches.json` klipper kun gamle TV-deler fra originalkontoret; g-ID-ene må regenereres ved ny verdenseksport. Kilder og importløype er beskrevet i Tools/kelvora/README.md. Originale Blender-kilder på Desktop/Retro TV og radio er oppdatert.
- Kandidat `scratch-bm/Signal47-kelvora/build` er installert i Games/Signal47, hashverifisert med 27 save/config-filer bevart. 1218 selvtester, ni scenarioer; siste materialversjon er også prøvd på begge TV-er og med av/på. Fullt 47-sekundersopptak/lagring bestod før siste V-only shaderendring. Ingen menneskelig lytting eller hel gjennomspilling i denne økten.
## Utelys og fotolab, 22. oktober 2026

- `port-light` er videreført etter at forrige lys ble integrert/pakket. Ny leveranse tar inn `unreal-port-core` til `3b44956e`, med gratismodellimport, Claudes mørkeromsregel og brettrettelser.
- `S47ExteriorLighting.cpp`: 30 korte, fysiske spotter fra eksisterende armaturer; `S47LabLighting.cpp`: adresserbar `Light("lab.normal"/"lab.safelight"/"lab.enlarger")`, intensitet 0 til 1, og fysisk normal-/rødt-/linse-/varsellys. `US47Chapter1::Darkroom` kopler renderdriverne ved faseskifte; ingen ny film-/dørregel i dette lyspasset.
- Bygg og 1218 selvtester OK, før/etter fra ni kameraer, labmoduser/halvstrøm/kutt/tilbake og faktisk kontaktkopitilstand prøvd. Rapport/API/bilder: `LIGHTING.md`, bevis i `~/scratch-bm/Signal47-lighting-2026-10-22/revision3`. Ny pakke, full fremkalling med ekte input og nye High/1440p-tider gjenstår hos integrator. Ingen endring av installert spill.
