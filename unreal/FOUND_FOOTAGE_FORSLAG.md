# Found Footage-modus for SIGNAL / 47: forslag og implementeringsplan

28. oktober 2026. Skrevet av Claude etter lesing av `~/scratch-bm/wt/sc02` (gren `claude/sc02`, `eaf9914e`, main pluss SC02) og Codex-arbeidskopien `~/Documents/Codex/2026-10-23-f-r-timeout-p-en-chat/game` (`codex/film-looks`, `37c32596`). Codex-grenen er allerede helt innflettet i sc02, og `film_tone.hlsl` er identisk i begge.

Dette er bare forskning og skriving. Ingen kode er endret, ingen Unreal-, bygg- eller pakkejobb er kjørt, ingenting er pushet. Alle ytelsestall for den nye modusen er estimater til de er målt.

Prosjektbiblioteket er søkt først (`"found footage camcorder VHS post process unreal"` ga null treff, `"VHS film look post process" --match any` ga bare Godot-shaderskills og urelaterte ressurser). Ingen bibliotekskilde er brukt. FILM_LOOKS.md nevner et tidligere ntsc-rs-kort merket UNVERIFIED; det er heller ikke brukt her. Grunnlaget er spillets egen kode, egne dokumenter og UE 5.8.4-kilden i `~/.local/opt/UnrealEngine-5.8.4`.

## 0. Kortversjon

- Spillet har allerede nesten hele grunnmuren: en bildemodusmekanikk (`S47Picture`, `ApplyPicture`, `MPC_Web`, `M_WebTone` bygd av `import_web.py`), en VHS-gren i shaderen, menyer som allerede er "et VHS-bånd" (TAPE B, OSD-dato med røde og cyan kanter), en kalm-modus for tilgjengelighet, en lydgraf med kompressor og filtre, og en tom krok (`A.Glitch` i `S47Away.cpp`) som webversjonen brukte til sporingsfeil.
- Det som mangler er linse, forsterkningsstøy og eksponeringsjakt, forstyrrelser drevet av hendelser, brent inn dato og REC, kameramontering og pust, og camcorderlyd.
- Perioderiktig "bodycam" i 1986 er enten en skulderbåret VHS/VHS-C-camcorder eller et lite overvåkingskamera på brystsele med kabel til en bærbar VHS-opptaker og en dato/klokke-generator. Jeg anbefaler brystsele som standard fordi den er håndfri og passer spillets lommelykt og speilreflekskamera.
- Anbefalt form: begge deler, i to trinn. Først en ny bildemodus `CAMCORDER 1986` (Settings > Picture), som er reversibel og ikke rører lagringer eller historie. Deretter, hvis Tom vil, en valgfri saksramme `FOUND FOOTAGE` ved ny sak, med båndklipp, spoling ved fangst og tidsstempel som del av fortellingen.
- Tre faste regler: kameraet legger aldri til noe som ikke finnes i verden (R1), forstyrrelser kommer bare fra faktiske kilder og forfattede hendelser, aldri fra et tilfeldighetssystem, og fotografiene forblir rene (R4). Den siste er allerede sikret av shaderens `Vhs`-port, så lenge den nye grenen legges etter porten.

## 1. Hva finnes i dag

### 1.1 Bildefilteret (gjenbrukes direkte)

| Del | Fil og sted | Hva den gjør | Bruk i found footage |
|---|---|---|---|
| Profilkontrakt | `Source/Signal47/Public/S47Picture.h`, `Private/S47Picture.cpp` | `Normalize`, `Mode` (off=0, vhs=1, carpenter=2, 16mm=3, super8=4), `DefaultForQuality` (Low gir off, ellers carpenter), `Ids`, `Labels`, `Description`, `SelfTest` | Ny id `camcorder`, Mode 5, alias `footage`, `found`, `bodycam`, `vhs-c`. Selvtesten "menu starts with Carpenter and covers every mode" må oppdateres fra 5 til 6. |
| Anvendelse | `Private/S47Flow.cpp` `US47Flow::ApplyPicture` (ca. linje 1067 til 1098) | Skriver `PictureMode`, `Vhs`, `PictureStrength`, `PictureCalm` til `MPC_Web`, leser tilbake og logger `S47: picture %s MPC mode=...` | Samme vei, pluss ny `MPC_Footage`. Loggen er allerede et testanker. |
| Lagring og flagg | `S47Flow.cpp` linje 254 til 265 og 1031 til 1035 | `settings.json`-nøklene `picture`, `pictureStrength`; `-S47Picture=`, `-S47PictureStrength=`, `-S47PictureCalm=`. `HasTestFlag()` (linje 131 til 139) unntar alt som starter med `-S47Picture` | Nye innstillingsflagg må hete `-S47Picture...` for å bli regnet som innstillinger og ikke som testkjøring. |
| Postvolum | `Private/S47Web.cpp` linje 714 til 753 | Ett ubundet `APostProcessVolume` (`LightingPost`), `AEM_Manual`, bloom, vignett, bevegelsesuskarphet og AO av, `M_WebTone` som `WeightedBlendable` | Samme volum. Eventuell ekte autoeksponering slås på bare i camcorder-modus (se 3.5). |
| Shader | `Tools/webexport/film_tone.hlsl` | Custom-node. Struct `W` med `sc()` (tåke, ACES, sRGB), `hash`, `yiq`/`rgb`, `grain`, `bright`. Port: `if (Mode < 0.5 \|\| Vhs <= 0.00001 \|\| strength <= 0.00001) return clean;`. VHS-gren: YIQ, fargeblødning, linjejitter, hodebytte nederst, korn, skannlinjer. Kalm stopper alt som beveger seg | Ny gren for Mode 5 etter porten. YIQ-, korn- og hodebyttekoden gjenbrukes. |
| Materialbygg | `Tools/webexport/import_web.py` `build_picture()` (linje 1495 til 1532) og MPC-blokken (linje 702 til 722) | Lager `M_WebTone` som `MD_POST_PROCESS`, `BL_REPLACING_TONEMAPPER`, kobler MPC-parametre til Custom-noden. `S47_ONLY=picture` bygger bare MPC og tonematerialet. MPC-parametre legges bare til der de mangler | Samme mønster for nye parametre og eventuelt en egen `footage.hlsl` som limes inn i samme Custom-kode. |
| Meny | `Private/SS47Settings.cpp` linje 269 til 290 (Picture-fanen) og 365 til 374 (Accessibility) | `AddChoice` for Picture, `AddSlider` for styrke, "Flashing images" (SLOW FADES stopper allerede flimmer, registreringsuro og båndforstyrrelser), "Head motion and bumps" | Nye rader for camcorder. Eksisterende tilgjengelighetsvalg får utvidet hjelpetekst i stedet for nye brytere. |
| Partikkelstørrelse | `Private/S47NightLife.cpp` `LinesNow` (linje 77 til 83) | VHS (Mode 1) holder møll og støv i 480-linjers fotavtrykk | Bør bli en hjelper `S47Picture::IsTape(Mode)` som gjelder både VHS og camcorder. |
| Testverktøy | `Scripts/graphics-review.py` (`--picture` med faste valg, linje 435), `Scripts/screenshots.sh` (`settings-picture`, `camera`, `event` og flere) | Bildematrise og pakkede kjøringer med `unreal_guard.py require-idle` | `camcorder` legges til valgene; nye `shot`-linjer (se 3.10). |

Målt kostnad for dagens filmfilter (FILM_LOOKS.md, High, 2560 x 1440): Carpenter koster +0,487 ms i kontrollrommet og +0,384 ms ved Caliche over Clean.

### 1.2 Spillerkameraet

- `Private/S47Walker.cpp` `AS47Walker::Tick` (linje 239 til 279): `Bob += Moved * 7.2`, vertikal gange `sin(Bob) * 0.018` m, rull `sin(Bob * 0.5) * 0.004` rad, landingsdypp, `Shake` som avtar med 1,4 per sekund og gir tilfeldig rystelse. Alt er stengt av `bHeadMotion`. `OnStep` kalles hver 0,8 m. Øyet er `FloorY + Lift + Eye` med `Eye = 1.62`.
- `Fov` er vertikal (70 som standard) og gjøres om med `HorizontalFov`. `CameraOverride` overtar kameraet i lastebilen; der følger `L->Drive->bCameraMotion` også `bHeadMotion` (`S47World.cpp` linje 1834).
- Rystekilder: `AS47HUD::Shake` (hendelsen i prologen gir 1,25), `S47Phones` `Ad.Shake`, `S47ArchivePhone` (0,15 i kalm, 0,42 ellers).
- `EyeUE()` og `ForwardUE()` brukes på 39 steder i 24 filer, blant annet interaksjon, feltkamera, `S47Seen`, takeaway og mange tester. Kameraposisjonen er altså også spillogikkens øye.
- `Private/SS47ScreenView.cpp` `ToPicture` (linje 240 til 257) projiserer musen tilbake i verden for å klikke på CRT-skjermer. Linseforvrengning ville forskyve disse klikkene.
- Spilleren har ingen synlig kropp eller armer i første person.

### 1.3 Feltkamera, album og bevis

- `US47FieldCamera` (`S47FieldCamera.h/.cpp`): hevet kamera smalner skjermen til en 16:10-ramme, og "what is inside the frame is exactly what the photograph records". `Render` (linje 157 til 215) bruker en `SceneCapture2D` med `SCS_FinalColorLDR` som arver verdens postvolum, setter `Vhs=0` og `ToneK * 1.9` under opptaket og setter dem tilbake. Kaptur-komponenten overstyrer ikke autoeksponering.
- `S47MotelTv.cpp` (linje 314 til 328) gjør det samme og overstyrer eksplisitt `AEM_Manual`.
- `US47Flow::CaptureThumb` (linje 1259 og videre) tar lagringsminiatyrer med verdens postprosess, altså med filmlooken på.
- Webversjonens `web/SPILLDESIGN.md` linje 71 tok allerede stilling: feltkameraet er et filmkamera med våtbenk, så REC og dato hører ikke hjemme i søkeren; "VHS-følelsen ligger på hele natten («et bånd noen fant»), aldri på selve fotografiet (R4)". `web/todo.md` linje 163: "Camcorder-modus med REC og dato er ikke laget ... venter på Tom." Dette forslaget er svaret på det åpne punktet, og holder seg til samme regel.

### 1.4 VHS og tid i fortellingen

- Menyene er allerede et bånd: `S47Theme.h` beskriver "a 1986 VHS tape over a SARO operations terminal", `SS47Osd` tegner OSD-tekst med røde og cyan kanter, `S47Theme::Stamp`/`DateStamp` gir "APR.13 1986  23:41:07", `S47GameStyle.h` `OsdLine` gir "APR.13 1986  23:58" (testet i `S47GameStyle.cpp` linje 316), `S47Theme::Scanlines` finnes, og versjonsfeltet er "UNREAL x · TAPE B" (`S47Flow.cpp` linje 537).
- Åpningen (`S47Opening.cpp`): "forty-seven seconds will refuse to belong to any clock", "The record calls it interference.", "The record is incomplete."
- Prologen (`S47Story.cpp`): `LightsDip` (linje 613: `DipT`, `CrtGlitch`, "LINE POWER DIP 0.7 S"), `StartCountdown` (linje 755: "STATION CLOCK WWVB REFERENCE LOST", 47 sekunder), `Event` (linje 767: `Boom(1.25)`, `Shake(1.25)`, `Flicker = 1.8`, `CrtGlitch = 0.5`, "HOLDOVER 00:00:47").
- Tapt tid: `S47Abduction.cpp` kaller `D.Glitch(1)`, `D.Glitch(1 - St * 2)` og `D.Glitch(0)` (linje 402, 417, 1055), men `S47Away.cpp` linje 159 til 161 binder `A.Glitch` til en tom lambda med kommentaren at webens `vhs.glitch` ikke finnes i `M_WebTone`. `A.SkipClock` hopper klokka.
- Nattens ene spenningskurve: `FS47NightCurve` i `S47Curve.h` med `Phase` (off, plain, build, peak, rest), `Level` 0..1 og `Strength` 0..1 over natten. Kommentaren sier at nattens ting spør denne ene kurven før de starter.
- SC02-trusselen: `FS47HorrorThreat` med tilstandene Idle, Warn, Investigate, Search, Chase, Capture, Withdrawn og `FSense::bPlayerSeesBody`. Pust spilles som `Audio->Gasp` ved kroppen.
- `FS47AnomalyDread::Pulse` dypper en gatelampe eller blinker ON AIR. `SS47Crt` har `CrtGlitch`.
- Dales kontor: `office:tape`/`office:fieldDeck`, `VHS_DALE_FIELD_01` "Service Road / Field 01" har et 72-sekunders manus men er ikke produsert (`DALE_OFFICE_DOCUMENTS.md` linje 39, `MISSION_FLOW_1800_ROSWELL.md` linje 124). Manuset sier "Camera date incorrect. Use position, not timestamp." `EV_DALE_ARRAY_SERVICE` sier "A recording can establish sound and elapsed time; it cannot establish bearing."

### 1.5 Lyden

- `US47Audio` er en port av Web Audio (`S47WebAudio.h`): `Master` til `Destination`, med `SfxBus`, `Music`, `Amb`, `RadioBus` (`S47Audio.cpp` linje 45 til 49). Nodetyper: Gain, BiquadFilter, Panner, StereoPanner, Delay, WaveShaper og DynamicsCompressor, som eksponerer `Reduction` i dB.
- Grafen spilles ut gjennom `US47Synth` (en `USynthComponent`) til UE sin master-submix. Innspilte replikker (`FS47RecordedDialogue`) går gjennom grafen. Noen få lyder kan gå utenom som native `PlaySound2D`/`SpawnSound` (`US47Audio::Play`/`LoopComponent`).
- Forbilde: TV-høyttaleren i `FS47TvProgramme` (TV_PROGRAMME.md) er allerede en kjede med highpass 250 Hz to ganger, lowpass 4 kHz, resonans 1,4 kHz, tanh-metning og mono.
- Testmønster: `S47Audio.cpp` linje 385 til 405 rendrer grafen offline og måler nivåer (pipetesten). Samme mønster passer for en camcorderkjede.
- `AudioMixer` er allerede i `Signal47.Build.cs`. I UE 5.8.4 finnes `UAudioMixerBlueprintLibrary::AddMasterSubmixEffect` og `SubmixEffectEQ`/`SubmixEffectDynamicsProcessor` i AudioMixer-modulen (kontrollert i motorkilden).
- `S47Wind::Wind().Strength` gir vindstyrke.

### 1.6 Motorfakta kontrollert i UE 5.8.4-kilden

- `MaxNumParameterCollectionsPerMaterial = 2` (`ParameterCollection.h` linje 18). `M_WebTone` bruker én i dag, så én ny MPC går, men da er taket nådd.
- `BL_ReplacingTonemapper` setter `POST_PROCESS_MATERIAL_BEFORE_TONEMAP=0` (`PostProcessMaterial.cpp` linje 265), og shaderens tre-argument `SceneTextureLookup` deler ikke på `PreExposure`. Tonepasset leser altså forhåndseksponert scenefarge. Med `AEM_Manual` er den konstant; med histogram-autoeksponering vil den følge tilpasningen. Det kan gi ekte AGC nesten gratis, men må prøves (3.5).
- `r.EyeAdaptation.PreExposureOverride` finnes (`PostProcessEyeAdaptation.cpp` linje 69), og `MaterialExpressionEyeAdaptation` finnes.
- `View.ClipToPrevClip` finnes i view-uniformbufferen (`SceneView.h` linje 941), men planen trenger den ikke.
- Oppskaleringen (TSR) skjer før tonemapping i `PostProcessing.cpp` (linje 1143 mot 1579). Tonepasset kjører derfor i vinduets oppløsning, ikke i `r.ScreenPercentage`.

### 1.7 Hva mangler

- Linse: forvrengning, kromatisk aberrasjon, myke kanter. Unreal-VHS-en har bevisst ingen radial aberrasjon.
- Sensor: forsterkningsstøy som øker i mørket, eksponeringsjakt, høylysslør eller etterslep, skjevhet ved panorering.
- Bånd og avspilling drevet av hendelser: sporingsfeil, vertikal rulling, brumstriper, fargebortfall, dropout. Ingen inngang i shaderen for dette i dag.
- Brent inn dato og klokke, REC, båndteller. Bare menyene har OSD.
- Kameramontering, kroppsrytme, pust, dreieetterslep.
- Camcorderlyd: mono mikrofon, ALC-pumping, susing, vind, håndteringslyd, operatørens pust.
- Innstillinger, lagring, flagg og tester for alt dette.

## 2. Design

### 2.1 Hvilket kamera finnes i 1986

Det fantes ingen kroppskameraer i 1986. To løsninger er troverdige for en feltoperatør ved et observatorium:

1. **Skulderbåret camcorder.** JVC GR-C1 (VHS-C, 1984) og fullformat VHS-camcordere fra 1985 til 1986 veide 2 til 3 kg og ble båret på skulderen eller i hånden. Elektronisk søker med liten svart-hvitt-CRT, innebygd mono elektretmikrofon med automatisk nivåkontroll (ALC), og billedrør (Saticon, Newvicon) eller tidlige MOS/CCD-brikker som trengte omtrent 7 til 10 lux. Rør gir etterslep og kometstriper fra sterke lys.
2. **Brystsele med overvåkingskamera.** Et lite CCTV-kamera (ofte svart-hvitt rør) på en hjemmelaget sele, kabel til en bærbar VHS-opptaker i skulderveske (bærbare VHS-opptakere fantes fra slutten av 1970-tallet), og en egen dato/klokke-generator mellom kamera og opptaker. Slike generatorer var vanlige i 1980-tallets overvåkingsvideo, så et innbrent stempel er naturlig.

Båndlengde: en T-120 på laveste hastighet holder omtrent seks timer, altså hele natten på én kassett, med dårligst mulig kvalitet. Det forklarer et kraftig, støyende bilde. VHS-C TC-20 holder 20 minutter på SP og 60 på EP.

**Anbefaling:** standard montering `CHEST RIG` (håndfri, passer lommelykt, speilrefleks og bruk av ting) i farge, med `SHOULDER` som alternativ følelse og `BLACK AND WHITE` som valgfri fargeprofil (overvåkingskamera eller søker).

**Diegetisk forankring (krever Toms ja, ikke nødvendig for bildemodusen):** SARO har et feltvideosett i Operations-kontoret, der VHS-spilleren og FIELD 01 allerede står. Det er samme sett Dale brukte, kameraet med feil dato. Reyes stiller datoen ved vaktstart, derfor viser stempelet APR.13 1986. Dales eget bånd kan senere vise den gale datoen, og samme shader kan lage FIELD 01 (se 2.9).

**Fire regler for modusen:**

- F1. Kameraet legger aldri til noe. Ingen figur, lyd eller skikkelse finnes bare på båndet (R1).
- F2. Forstyrrelser kommer bare fra fysiske kilder og forfattede hendelser som spillet allerede har, aldri fra et eget tilfeldighetssystem. Det følger ANOMALY_SCARIER og `S47Curve.h`.
- F3. Fotografier og bevisbilder forblir rene (R4). Den eksisterende `Vhs`-porten sikrer dette når den nye grenen ligger etter den.
- F4. Tidsstempelet er ikke bevis. FIELD 01 sier selv "Use position, not timestamp".

### 2.2 Bildet som signalkjede

Effektene legges i den rekkefølgen et ekte signal går, slik at de samspiller riktig: optikk, sensor, tegngenerator, båndopptak, avspilling. Tegngeneratoren kommer etter optikken (stempelet bøyes ikke av linsen) men før båndet (stempelet blør og hopper med bildet).

| Ledd | Effekt | Startverdier | Periodegrunnlag | Kalm (SLOW FADES) | Low |
|---|---|---|---|---|---|
| Optikk | Tønneforvrengning | k1 = 0,10, k2 = 0,03, skalering så hjørnene holder seg innenfor bildet (omtrent 8 prosent beskjæring) | Vidvinkelkonverter (0,5x) på camcorder, eller kort C-mount-linse på CCTV | Uendret (statisk) | Uendret |
| Optikk | Lateral kromatisk aberrasjon | 1,5 px i hjørnene, rødt utover, blått innover | Webversjonen brukte 1,5 px i ytterste 20 prosent (SPILLDESIGN.md) | Uendret | Slått sammen med fargeprøvene |
| Optikk | Myke kanter og vignett | Vignett 0,25, uskarphet mot kantene | Billig konverterlinse | Uendret | Bare vignett |
| Sensor | AGC-forsterkning | 1 til 8 ganger (0 til +18 dB), se 3.5 | Automatisk forsterkning i alle forbrukerkameraer | Ingen overshoot, langsommere | Samme |
| Sensor | Forsterkningsstøy | Amplitude proporsjonal med forsterkning og mørke, grovt rutenett (omtrent 240 linjer), fargeflekker i mørke flater | Lavlysstøy | Frosset korn (som i dag) | Bare luma |
| Sensor | Høylysslør og etterslep | Sterke punkter (lamper, lommelyktkjegle) strekkes mot panoreringen, 3 til 4 prøver | Rørkameraets kometstriper | Av | Av |
| Sensor | Skannskjevhet | Linjeforskyvning proporsjonal med y og panoreringsfart | Rør og MOS skanner linje for linje | Av | Samme (ren ALU) |
| Sensor | Avmetning i mørke | Metning ned mot 0,6 når forsterkningen er høy | Lavlys | Uendret | Uendret |
| Tegngenerator | Dato og klokke | "APR.13 1986" og "PM 11:58:07" nederst til høyre | Dato/klokke-generator | Statisk | Statisk |
| Tegngenerator | REC og båndteller (valgfritt) | Rød prikk og REC øverst til venstre, blinker 1 Hz | Søkermerker | Blinker ikke | Samme |
| Bånd | Luma- og fargebåndbredde | Luma omtrent 240 TV-linjer, farge omtrent 30 til 40, gjenbruk av YIQ-koden | VHS: luma rundt 3 MHz, farge rundt 0,5 MHz | Uendret | 3 fargeprøver i stedet for 6 |
| Avspilling | Linjejitter og hodebytte | Som dagens VHS-gren | VHS | Av (som i dag) | Samme |
| Avspilling | Sporingsfeil | Bånd av forskjøvne linjer og snø, styrt av `FfTear` | VHS | Erstattes av 0,5 s støytoning | Samme |
| Avspilling | Brumstripe | Mørkt horisontalt bånd som ruller 0,2 til 1 Hz nær strøm | 60 Hz-innstråling | Står stille | Samme |
| Avspilling | Fargebortfall | Bildet går svart-hvitt i 0,2 til 0,5 s ved kraftig feil | Fargedreperen i videospilleren ved ustabil synk | Myk overgang uten flimmer | Samme |
| Avspilling | Vertikal rulling | Bildet glir en halv rute, bare ved Event | V-hold | Av | Samme |
| Avspilling | Dropout | Hvite horisontale streker, sjelden | Oksidavfall | Av | Samme |
| Avspilling | Interlace-kamming | Oddelinjer forskyves med halve panoreringen | 59,94 halvbilder per sekund | Av | Samme |

To grep gjør dette billig uten bildehistorikk: panoreringsfarten (`FfPan`) beregnes i C++ fra kameraets dreining per ramme og sendes til shaderen. Den gir kamming, etterslep og skjevhet uten at materialet trenger forrige bilde.

Tilgjengelighet: også i full modus skal ingen stor flate blinke mer enn tre ganger i sekundet (WCAG 2.3.1). Kalm-modus, som spillet allerede har, gjør alle forstyrrelser om til rolige toninger.

Lesbarhet: spillet foregår om natten, og et ekte kamera på 7 lux ville vist nesten svart. Forsterkningen må gi minst like mye lesbar detalj som Clean i de fire faste nattkameraene, med mer støy i stedet for mindre lys. Akseptkriteriet fra SPILLDESIGN.md passer: en tester nevner stemningen uoppfordret, uten å spørre om bildet er ødelagt.

### 2.3 Kamerafølelsen

Tall er utgangspunkt for justering, ikke fasit.

| Del | Brystsele | Skulder |
|---|---|---|
| Høyde | 1,36 m (0,26 under øyet), 0,10 m frem, 2 grader ned | 1,57 m, 0,11 m til høyre, 0,05 m frem |
| Gangerytme | Vertikalt 0,024 m (i dag 0,018), rull pluss minus 0,8 grader ved halv steglengde, gir pluss minus 0,4 grader | Vertikalt 0,015 m, rull pluss minus 1,2 grader (vekten på skulderen) |
| Steg | Ved hver `OnStep`: 6 mm ned over 120 ms og 0,3 grader nikk | 3 mm og 0,2 grader |
| Løping | Alt ganger 1,8 | Alt ganger 1,6 |
| Pust i ro | 0,25 Hz (15 per minutt), 5 mm løft, 0,15 grader nikk | 0,25 Hz, 2 mm |
| Anstrengelse | Etter løping og i SC02 Chase: opp til 0,5 Hz og 2,5 ganger utslag, avtar over 20 s | Samme |
| Dreieetterslep | Kritisk dempet fjær, 70 ms | 50 ms |
| Skvett | Når `Shake` passerer 0,5: 2 grader nikk ned og tilbake over 300 ms | Samme |

- Siktet (ringen) står midt på skjermen, og interaksjonsstrålen går fra det rendrete kameraet. Da treffer man det man ser, også med brystsele. Dreieetterslepet holdes lite så siktet ikke føles gummiaktig.
- I lastebilen brukes ingen monteringsforskyvning (førerhusets geometri), bare bildet og den eksisterende kjørebevegelsen.
- Hevet feltkamera: forskyvning, etterslep og linse toner til null over 0,2 s, fordi rammen må vise nøyaktig det fotografiet får. Det er et krav, ikke et valg.
- CRT-skjermvisning (`SS47ScreenView`), notatbok, brett og andre paneler: linse og monteringsbevegelse til null, så klikk og projeksjon stemmer.
- "Head motion and bumps" av: all monteringsbevegelse, pust, steg, skvett og dreieetterslep blir null. Høyden beholdes, den beveger seg ikke.
- Brystkameraet ser ned dit kroppen skulle vært. Spilleren har ingen kropp i første person, så et blikk rett ned viser bakken. Det er trolig greit; en enkel arm med lommelykt kan vurderes senere.

### 2.4 Lyden

Kjeden settes inn mellom `Master` og `Destination` i Web Audio-grafen, med en krysstoning mot en ren forbikobling slik at av er bitidentisk med i dag (pipetesten må fortsatt bestå):

`Master`, inngangsgain, highpass 90 Hz, lowpass 7 kHz (EP-kvalitet), topp +3 dB ved 2,5 kHz (elektretmikrofonens nærvær), mild tanh-metning, innsetting av sus, vind, håndtering og pust, kompressor som ALC, stereobredde, vow og flutter (modulert forsinkelse), `Destination`.

- **ALC og pumping.** Kompressoren settes omtrent til terskel minus 34 dB, knekk 6, forhold 10, attack 4 ms, release 1,5 s, med høy makeup. Stille rom løftes omtrent 10 dB. Et smell (`Boom` i Event) trykker nivået ned, og stillheten etterpå kommer langsomt tilbake. Det er den mest gjenkjennelige found footage-signaturen, og den passer spillets "verden lytter".
- **Sus.** Støy rundt minus 46 dBFS før kompressoren, så den høres i stillhet og forsvinner under høye lyder. Nivået kan følge `Reduction` fra kompressornoden.
- **Vind.** Lavpasset støy (40 til 300 Hz), styrt av `S47Wind::Wind().Strength` når spilleren er ute og ikke i lastebilen. Vindstøt får ALC til å pumpe.
- **Håndtering.** En kort 70 Hz-dunk med 90 ms forløp ved hvert steg (brystsele 0,10, skulder 0,05), og klesrasling proporsjonal med dreiefart.
- **Operatørens pust.** Nær mikrofonen, syntetisert som eksisterende `Gasp`. Nesten uhørlig i ro, tydelig etter løping og i Chase, et skarpt innpust ved skvett. Den dukkes under replikker som TV-lyden gjør (`VoicesDuck`).
- **Mono.** Valg `LIGHT` (stereobredde 35 prosent, standard), `MONO` og `OFF`. SC02 bruker retningslyd (skritt i grusen og pust ved kroppen), og full mono gjør sniking vanskeligere. Det må stå i hjelpeteksten.
- **Musikk.** Åpent spørsmål. Forslag: i bildemodusen går partituret utenom kjeden; i saksrammen dempes det eller bare diegetisk lyd (radio, TV) spilles.
- **Native lyder.** Hvis trinn 0 finner viktige lyder utenom grafen, kan de få en submix-effekt. Grafens egen utgang (`US47Synth`) går også gjennom master-submixen, så en master-effekt ville behandle grafen to ganger. Løsningen er en egen submix for de native lydene, ikke master.

### 2.5 Skrekkhendelser som driver forstyrrelser

Forstyrrelsen er et omhyllingsuttrykk: høyeste verdi av kildene nedenfor, glattet i C++ og sendt til shaderen som `FfTear`, `FfHum`, `FfKill`, `FfDrop` og `FfRoll`. Alle hendelser er seedet, slik at skjermbilder blir like fra kjøring til kjøring med `-S47PictureCalm=1`.

| Kilde | Kode | Effekt | Verdi og varighet | Kalm |
|---|---|---|---|---|
| Nattkurven | `M->Night->Curve` (`S47Curve.h`) | Grunnstøy og svak brum i build, ingen rifter | `0,10 x Level x Strength`; null i rest | Bare støy |
| Strøm i nærheten | Trafostasjonen ved Caliche Junction (SUBSTATION.md), riksveiens kraftlinje (`S47HighwayPower`), SAROs stativ og trafo | Rullende brumstripe | 0,3 ganger avstandsfall fra 4 til 25 m | Stillestående stripe |
| CRT-er i bildet | `SS47Crt`, `Story->CrtGlitch` | Rullende stripe og moaré er naturlig når man filmer en CRT; `CrtGlitch` gir i tillegg en rift | Så lenge `CrtGlitch > 0` | Toning |
| Lysdypp | `AS47Story::LightsDip` (`DipT` 0,7 s) | AGC jakter: bildet åpner seg når lyset faller og overskyter når det kommer tilbake; kort brum | 0,7 s pluss 1 s etterjakt | Uten overshoot |
| 47 sekunder | `StartCountdown` (WWVB borte) | Svakt stigende brum, ingen rifter; mulig stempelfrys (2.6) | 47 s | Samme uten bevegelse |
| Hendelsen | `AS47Story::Event` | Sporingstap 1,2 s: rift, halv rute rulling, fargebortfall 0,4 s, dropout; ALC slås ned av smellet | 1,2 s, så 2 s gjenoppretting | 0,6 s støytoning og mørkning |
| Mørkepuls | `FS47AnomalyDread::Pulse` | Liten AGC-dypp og 0,3 s brum | 0,3 til 1 s | Toning |
| Tapt tid | `S47Abduction` `D.Glitch(k)`; `S47Away.cpp` linje 161 | Den tomme lambdaen kobles til `FfTear = k`, med webens egen kurve (1, så 1 minus 2 ganger st, så 0) | Som webversjonen | Toning |
| Tapt tid, klokke | `A.SkipClock` | I saksrammen: båndklipp (én rute regnbuestøy) og stempelet hopper | Øyeblikk | Klipp uten støyrute |
| SC02-trusselen | `FS47HorrorThreat::State()`, avstand, `bPlayerSeesBody` | Warn 0,15, Investigate 0,2, Search 0,3, Chase 0,55; full styrke ved 6 m, null ved 30 m; ganger 1,3 når kroppen er i bildet | Mens tilstanden varer | Støy og mørkning |
| Fangst | `ConsumeCapture()` | Dropout til snø 0,8 s; i saksrammen spoling tilbake (REW-støystriper) til sjekkpunktet | 0,8 s pluss spoling | Toning til svart |
| Skvett | `Shake > 0,5` | Kort jitter 0,2 s og monteringsskvett | 0,2 s | Bare skvett hvis Head motion er på |
| Glimt | `FS47Glimpses` | **Ingen forstyrrelse.** Kameraet tar det opp rent. En glitch ville gjort kameraet til en detektor og brutt R1 | | |

Tilleggsregler: høyst én kraftig rift per 10 sekunder, aldri mer enn tre store lysskifter per sekund, og trusselforstyrrelsen bare når kroppen er i bildet eller nærmere enn 12 m. Trafostasjonen ligger der trusselen er, og gir samme brum. Spilleren kan ikke vite om det er strømmen eller den. Det er ønsket tvetydighet, og det hindrer at forstyrrelsen blir en radar.

### 2.6 Tidsstempelet og de 47 sekundene

- Format: dagens `OsdLine` er "APR.13 1986  23:58". Amerikanske forbrukerkameraer viste ofte tolvtimers klokke. Forslag: linje én "APR.13 1986", linje to "PM 11:58:07". Ved midnatt blir det APR.14, som `DayLine` allerede gjør.
- Stilen bør være båndets: VT323 med røde og cyan kanter som `SS47Osd`, men tegnet inn i bildet før båndleddene, så det blør og hopper med bildet.
- **Åpent kanonspørsmål:** under nedtellingen (WWVB borte i 47 sekunder) kan stempelet fryse på sekundet linja døde mens båndtelleren går videre, og ved Event hoppe 47 sekunder frem. Det er åpningens løfte ("forty-seven seconds will refuse to belong to any clock") gjort synlig, uten at noe overnaturlig legges til i verden. Det endrer likevel hva båndet "vet", så det må Tom bestemme.
- Tapt tid (`SkipClock`) gir et stempelhopp. Det er det kameraet faktisk ville vist.

### 2.7 Meny, lagring og flagg

**Settings > Picture**

- Picture: `CARPENTER 35 MM`, `DESERT 16 MM`, `SUPER 8`, `VHS TAPE`, `CAMCORDER 1986`, `CLEAN`. Carpenter er fortsatt først og standard; Low er fortsatt Clean som standard.
- Picture strength: blander bare bildet. Montering og lyd følger egne valg.
- Rader som bare kan brukes når Camcorder er valgt (Theme `Choice` har allerede `Usable`):
  - Camcorder mount: `CHEST RIG` / `SHOULDER`
  - Lens: `WIDE` / `STANDARD` (STANDARD fjerner forvrengningen)
  - Camcorder display: `DATE AND REC` / `DATE` / `NONE`
  - Colour: `COLOUR` / `BLACK AND WHITE`
- Beskrivelse i `S47Picture::Description`-stil: "A 1986 home camcorder on a chest rig: a wide lens that bends the edges, noisy gain in the dark, a burned-in date and time, mono sound and tape interference near power lines and when something happens. Head motion, Flashing images and Lens soften it."

**Settings > Sound:** Camcorder sound: `LIGHT` / `MONO` / `OFF`.

**Settings > Accessibility:** ingen nye brytere. "Flashing images: SLOW FADES" gjelder også rifter, rulling, fargebortfall, dropout og AGC-overshoot. "Head motion and bumps" gjelder også montering, pust, steg og etterslep. Hjelpetekstene utvides. Hvis Tom ønsker det, kan Lens speiles her.

**settings.json** (innstillinger, ikke saksdata): `picture: "camcorder"`, `camMount: "chest"`, `camLens: "wide"`, `camOsd: "date+rec"`, `camColour: "colour"`, `camSound: "light"`.

**Kommandolinje**

- Innstillinger, unntatt fra `HasTestFlag`: `-S47Picture=camcorder`, `-S47PictureMount=chest|shoulder`, `-S47PictureLens=wide|standard`, `-S47PictureOsd=full|date|none`, `-S47PictureColour=colour|mono`, `-S47PictureSound=light|mono|off`. Som i dag skriver de ikke til `settings.json`.
- Bare for test, og derfor bevisst regnet som testflagg: `-S47FootageGlitch=0..1`, `-S47FootageGain=1..8`, `-S47FootageEvent=dip|event|chase|capture`.

**Saksrammen (trinn 2):** valget tas ved ny sak (`FS47OpeningChoice` får et felt) og lagres i saken, for eksempel `"footage": {"v": 1, "counter": 0}`. Lagringsmenyen kan merke slike saker "TAPE". Miniatyrene bærer allerede bildeprofilen (`CaptureThumb`).

### 2.8 Bildemodus, egen spillmodus eller begge

| | Bildemodus `CAMCORDER 1986` | Saksramme `FOUND FOOTAGE` |
|---|---|---|
| Hva endres | Bilde, kamerafølelse, lyd, stempel | I tillegg: introkort som båndetikett, båndklipp i stedet for svarttoninger ved kapittelkort og lasting, spoling ved fangst, "END OF TAPE" til slutt, stempel i fortellingen |
| Lagring | Bare `settings.json` | Nytt felt i saken; gamle saker uendret |
| Historie og kanon | Ingen | Krever Toms godkjenning (stempelfrys, hvem som fant båndet, musikk) |
| Risiko | Lav, reversibel | Middels, berører Opening, Flow og sluttkort |
| Test | Selvtester og bildematrise | I tillegg historieregresjon med rammen på |

**Anbefaling: begge, bildemodus først.** Bildemodusen gir mesteparten av verdien og kan leveres og vurderes alene. Saksrammen bygges på samme modul når Tom har sett og godkjent bildet. Hvis saksrammen er på, låses Picture til Camcorder for den saken, mens tilgjengelighetsvalgene alltid kan endres.

### 2.9 Gevinst for FIELD 01

`VHS_DALE_FIELD_01` er ikke produsert. Den krever bevegelig bilde, miljølyd, stopp og spoling. Samme shadergren og lydkjede kan lage den i motoren: en iscenesatt kamerarute ved serviceveien, rendret med camcorderprofilen og Dales feil dato, vist på kontor-TV-en eller i fullskjerm. Det er en egen oppgave, men camcorderarbeidet gjør den mye mindre.

## 3. Implementeringsplan

Alle Unreal-jobber kjøres sekvensielt med `Scripts/unreal_guard.py require-idle` (én jobb om gangen). Genererte Web-assets regenereres i arbeidskopiens private filer og committes ikke, som i FILM_LOOKS.md. Eierskap avklares i OPPGAVER.md før start, siden filmfiltrene var Codex sitt arbeid.

### Trinn 0. Avklaring (én kort, sekvensiell jobbrunde)

1. To MPC-er i `M_WebTone`: `MPC_Web` pluss ny `MPC_Footage`. Taket er 2, så dette må prøves i editoren.
2. Autoeksponering: slå `LightingPost` til `AEM_Histogram` i en prøve og se om tonepassets inngang følger tilpasningen (1.6). Mål stabilitet, lag og pris.
3. OSD i rendermål: `FWidgetRenderer` tegner `SS47Osd` inn i en `UTextureRenderTarget2D` på Linux/Vulkan i pakket spill.
4. Lyder utenfor grafen: logg alle `PlaySound2D`/`SpawnSound` i en hel kort regresjon og avgjør om de trenger egen behandling.
5. Grunnlinje: Clean, VHS og Carpenter på Low og High med `graphics-review.py` i de samme kameraene og klokkeslettene som FILM_LOOKS.md.

Ferdig når hvert punkt har en skriftlig PASS eller en valgt reserveløsning.

### Trinn 1. Profilkontrakten

- `S47Picture.h/.cpp`: id `camcorder`, Mode 5, alias, etikett, beskrivelse, `IsTape(Mode)`. Oppdater selvtestene (6 profiler, Carpenter først, Low fortsatt off).
- `S47NightLife.cpp` `LinesNow`: bruk `IsTape`.
- `SS47Settings.cpp`: valget i listen.
- `Scripts/graphics-review.py`: `camcorder` i `--picture`.
- `S47Web.cpp` linje 744 til 752 trenger ingen endring for selve modusen, fordi den allerede bruker `S47Picture::Mode`.

### Trinn 2. Materialet

- Ny `Tools/webexport/footage.hlsl` med struct `F` (linse, sensor, OSD, bånd), limt inn etter `film_tone.hlsl` i samme Custom-kode av `import_web.py`. Ny gren `[branch] if (Mode > 4.5)` legges **etter** porten `Vhs <= 0.00001 ... return clean`, så feltkamera, TV-kamera og andre opptak som setter `Vhs=0` fortsatt får rent bilde.
- Ny `/Game/Web/Base/MPC_Footage` med fem vektorparametre i stedet for mange skalarer: `FfLens` (forvrengning, aberrasjon, vignett, detaljnivå), `FfSensor` (forsterkning, støy, avmetning, svart-hvitt), `FfTape` (rift, brum, fargebortfall, dropout), `FfPan` (panorering x og y, rulling, kamming), `FfStamp` (sekunder, dag, OSD-flagg, blink).
- Hvorfor egen MPC: å legge parametre til `MPC_Web` endrer oppsettet som alle scenematerialer bruker, og kan ugyldiggjøre langt flere shadere og PSO-er enn tonepasset. Bare `M_WebTone` refererer `MPC_Footage`.
- `build_picture()` får nye `CustomInput`-er og kobler `MaterialExpressionCollectionParameter` fra den nye MPC-en. `S47_ONLY=picture` lager begge MPC-ene og tonematerialet.
- Hvis OSD-et går gjennom rendermål: `RT_FootageOsd` lages av `import_web.py` og leses i Custom-noden.
- PSO: tonepasset endres, så `Scripts/pso-cache.py` kjøres og cook/runtime-GUID kontrolleres som i FILM_LOOKS.md.

### Trinn 3. Kjøretidsmodulen

- Ny `Public/S47Footage.h` og `Private/S47Footage.cpp`. Mønsteret er `FS47HorrorThreat::FSense`: en adapter fyller en ren inngangsstruct, og selvtesten fyller den for hånd.
  - `FS47FootageIn`: på/av, styrke, kalm, hodebevegelse, hevet kamera, skjermvisning, modal, kjøring, klokke, fase, `CrtGlitch`, `DipT`, `Flicker`, kurvefase, nivå og styrke, trusseltilstand, avstand og synlighet, tapt tid-glitch, mørkepuls, avstand til strømkilder, vind, ute/inne, løping, steg, dreiefart.
  - `FS47FootageOut`: MPC-verdier, monteringsforskyvning og rotasjon, lydparametre.
  - `static FS47FootageOut Step(const FS47FootageIn&, FState&, double Dt)` er deterministisk, og `static bool SelfTest(TArray<FString>&)` følger husets stil.
- `US47Main` eier `TSharedPtr<FS47Footage> Footage`. Den steppes i `US47Main::Step` (kalt fra `AS47HUD::Tick`), og skriver `MPC_Footage` hver ramme slik `US47WorldLife` skriver `WindTime`.
- `S47Away.cpp` linje 161: den tomme `A.Glitch` kobles til `Footage->AbductionGlitch(k)`.
- Logglinjer som testankre: `S47: footage on mount=chest lens=wide osd=date+rec sound=light`, `S47: footage event dip|event|chase|capture tear=...`.

### Trinn 4. Kameraet

- `S47Walker.h/.cpp`: et felt `FS47Mount Mount` (forskyvning i kameraets lokale rom, gir, nikk, rull) som legges på i den siste `SetActorLocationAndRotation`, og et dreieetterslep på den rendrete rotasjonen. `Pos` og `Eye` endres ikke. `OnStep` sier fra til Footage. Rendret gir og nikk per ramme publiseres for `FfPan`.
- Alt skaleres med `bHeadMotion`, bortsett fra den statiske høyden. Hevet feltkamera og skjermvisning toner monteringen til null.
- Gå gjennom de 39 `EyeUE()`-kallene. De fleste er tester og synlighetskontroller der det rendrete kameraet er riktig. Trusselens siktlinje til spillerens øye fungerer også på 1,36 m. Interaksjonsrekkevidder kontrolleres på hylle-, telefon- og bordhøyde.
- Ingen FOV-endring i første omgang. Linsen beskjærer omtrent 8 prosent, og en senere utvidelse av FOV kan kompensere.

### Trinn 5. Eksponering og AGC

- **Vei A (hvis trinn 0 bekrefter den):** i camcorder-modus settes `LightingPost` til `AEM_Histogram` med min/maks lysstyrke som gir 0 til +18 dB og trege hastigheter (opp omtrent 0,6, ned omtrent 1,2) for jakt. Shaderen leser `EyeAdaptation` for å skalere støyen med faktisk forsterkning. C++ legger på en kort overshoot ved lysdypp, Event og lommelykt av/på.
  - Alle opptak som arver verdens postvolum må da overstyre `AEM_Manual` med samme bias: `US47FieldCamera::Render`, `US47Flow::CaptureThumb` (eller bevisst la miniatyren ha AGC), probe-opptaket i `S47Away.cpp` (rundt linje 691). `S47MotelTv` gjør det allerede. Uten dette ville fotografiene blitt autoeksponert, og `ToneK * 1.9`-trikset ville sluttet å virke.
- **Vei B (reserve):** shaderen leser 9 faste punkter på skjermen (samme punkter for alle piksler, så hurtigbufferen gjør det nesten gratis) og regner ut en øyeblikkelig forsterkning. Treghet og overshoot kommer fra C++ ut fra kjente hendelser. Ærlig begrensning: vei B mangler ekte scenestyrt etterslep.

### Trinn 6. Stempel og søkermerker

- Anbefalt: `SS47Osd` tegnes inn i `RT_FootageOsd` med `FWidgetRenderer` én gang per sekund og ved blink, og shaderen legger det inn før båndleddene. Det gir spillets egen OSD-stil, og stempelet blør og hopper med bildet.
- Reserve: et lag i `SS47Hud` med `SS47Osd`, som er raskt å lage men ser påklistret ut.
- Plassering: dato og klokke nederst til høyre over tekstebåndet, REC øverst til venstre. Målet (objective) flyttes én linje ned når REC vises. Armbåndsuret beholdes; om camcorderklokka skal erstatte det, er et spørsmål til Tom.

### Trinn 7. Lyden

- `S47Audio.h/.cpp`: `FCam`-kjede som bygges etter `Master` i `Init`, med `SetCamcorder(Mode, Mount, Sound)`. `US47Audio::Tick` oppdaterer sus fra `Reduction`, vind, håndtering og pust. Kontroller i trinn 0 at `FS47AudioParam` kan moduleres av en oscillator (vow og flutter); ellers settes forsinkelsen fra `Tick`.
- `US47Flow::ApplyPicture` og `ApplyVolume` kaller `SetCamcorder`.
- Selvtester med offline-rendring som pipetesten: av gir bitidentisk forbikobling; en 1 kHz-tone bare i venstre kanal gir nesten like kanaler i MONO og 35 prosent bredde i LIGHT; 12 kHz dempes mer enn 18 dB og 50 Hz mer enn 12 dB; etter et kraftig utbrudd faller forsterkningen og er tilbake innen 2 s; susen i stillhet holder seg innenfor fastsatte grenser.

### Trinn 8. Innstillinger og Flow

- `FS47PauseOpts`: nye felt og callbacks. `SS47Settings.cpp`: nye rader. `US47Flow`: lese, lagre og kommandolinje for nøklene i 2.7.
- `US47Flow::SelfTest`: JSON-rundtur for hver nøkkel, og "runtime override applies ... without saving" for de nye flaggene, etter samme mønster som dagens bildetester.

### Trinn 9. Ytelse og Low

- Teksturprøver per piksel, estimert: dagens VHS-gren bruker rundt 13. Camcorder High rundt 20 til 24 (linse og aberrasjon slått sammen med fargeprøvene, 4 for høylys, 3 til 4 for etterslep når det panoreres, 9 sammenfallende lysmålerprøver i vei B). Camcorder Low rundt 9 til 10 (3 fargeprøver, ingen høylysglød, ikke etterslep).
- Tonepasset kjører i vinduets oppløsning etter TSR, så kostnaden følger vinduet.
- Mulig gevinst: en camcorder har omtrent 240 linjers luma. Camcorder-modus kan senke `r.ScreenPercentage` til 50 (Low og High bruker 60 i dag, Ultra 100, i `S47Lighting.cpp` linje 230). Det kan spare mer enn filteret koster, og TSR-uro skjules av båndets uskarphet. Endringen må gjøres i `S47Lighting` som eneste eier av variabelen, og måles.
- Histogram-autoeksponering (vei A) koster typisk rundt 0,1 ms. Må måles.
- Stempel én gang per sekund og lydkjeden er ubetydelige.
- Akseptkrav (forslag): Low på 1920 x 1080 høyst 0,5 ms over Clean i p95; High på 2560 x 1440 høyst 0,5 ms over Carpenter. Målt med `graphics-review.py` i samme kameraer og klokkeslett som FILM_LOOKS.md, og rapportert som korte vinduer, ikke som løfte om låst bildefrekvens.

### Trinn 10. Tester som passer eksisterende kontrakter

Selvtester (tallet øker fra dagens rundt 1 593):

- `S47Picture`: id, alias, Mode 5, rekkefølge, standard per kvalitet.
- `S47Footage::SelfTest`: stempelformat før og etter midnatt; kalm gir null rift, rulling og dropout; Head motion av gir null monteringsbevegelse; hevet kamera gir null linse og forskyvning innen 0,25 s; glimt gir ingen forstyrrelse; hver hendelse i 2.5 gir forventet verdi og varighet; høyst én kraftig rift per 10 s; ingen stor lysveksling over 3 Hz.
- Lyd: offline-testene i trinn 7.
- Flow: JSON-rundtur og overstyring uten lagring.

Nye linjer i `Scripts/screenshots.sh` (eksisterende `shot`-funksjon og loggankre):

```sh
shot ff-room "S47: picture camcorder MPC mode=5" "-S47Picture=camcorder -S47PictureCalm=1 -S47Face=room"
shot ff-outside "S47: footage on mount=chest" "-S47Picture=camcorder -S47PictureCalm=1 -S47Face=outside"
shot ff-dip "S47: footage event dip" "-S47Picture=camcorder -S47Face=crtCenter -S47Jump=shift -S47FootageEvent=dip"
shot ff-event "jumped to event" "-S47Picture=camcorder -S47Face=window -S47Jump=event"
shot ff-camera "frame01 exposed" "-S47Picture=camcorder -S47Jump=chapter1 -S47Ch1Drive=photo"
shot ff-settings "S47: settings opened at picture" "-S47Picture=camcorder -S47Title -S47Open=settings:picture"
```

- Bevisintegritet: `ff-camera` skal gi samme utskriftslysstyrke (`FilmLook`-snittet) som `camera` uten modusen, innenfor en toleranse, og loggen skal vise at `Vhs` var 0 under opptaket. Verden er levende, så pikselidentitet kan ikke kreves.
- Meny: faktiske Slate-tastetrykk med `-S47Press` velger Camcorder og CHEST RIG, omstart beholder valgene, og R gjenoppretter standard, som filmfiltrenes menyprøve.
- `graphics-review.py --picture camcorder` i fire nattkameraer, Low og Ultra, styrke 0 og 50 prosent.
- Lyd: logglinje for kjeden av og på i en pakket kjøring.
- Menneskelige prøver som ikke kan erstattes av automatikk og føres som UNVERIFIED til Tom har spilt: bevegelsesubehag, lesbarhet om natten, sniking i SC02 med MONO, og om stemningen virker.

### Trinn 11 (senere). Saksrammen

`FS47OpeningChoice` og åpningsvisningen får valget; Flow lagrer feltet i saken; kapittelkort og lasting bruker båndklipp; fangst i SC02 bruker spoling; sluttkortet viser "END OF TAPE" og båndteller; stempelets 47-sekunders oppførsel hvis Tom sier ja. Historieregresjonene (T01, T02, SC01, SC02) kjøres med rammen på.

## 4. Risiko og åpne spørsmål

### Risiko

1. **Lesbarhet om natten.** Et ekte kamera ville vist nesten svart. Forsterkning, gulv for svartnivå og Brightness må kalibreres mot Clean i de faste nattkameraene.
2. **Bevegelsesubehag.** Forvrengning, pust og etterslep kan gi kvalme. Konservative standardverdier, Lens STANDARD og Head motion av må fungere fullt ut.
3. **Fotosensitivitet.** Rifter og fargebortfall kan blinke. Kalm og tre-hertzregelen må testes særskilt.
4. **Bevis (R4).** Den nye grenen må ligge etter `Vhs`-porten, og alle opptak må overstyre autoeksponering hvis vei A brukes.
5. **Radar i SC02.** Forstyrrelse som følger trusselen kan bli et hjelpemiddel. Avgrenset til synlig eller nær kropp, maskert av trafostasjonens brum.
6. **Mono mot sniking.** Retningshørsel er en del av SC02. LIGHT er derfor standard.
7. **Autoeksponering påvirker andre deler.** Bare aktiv i camcorder-modus, men opptak, miniatyrer og TV må kontrolleres.
8. **MPC-taket.** `M_WebTone` når to MPC-er. Neste behov må inn i `MPC_Footage`.
9. **PSO-bunten.** Endret tonepass krever ny PSO-innspilling og GUID-kontroll.
10. **Projeksjon.** Forvrengning ødelegger `SS47ScreenView`-klikk og feltkameraets ramme, derfor av i de tilstandene. Fremtidige verdensprojiserte HUD-merker må ta hensyn til det samme.
11. **Øyet flyttes.** `EyeUE()` blir brystet. Faste plasseringer i tester påvirkes ikke fordi modusen er av i testene, men et utvalg historietester bør kjøres med modusen på.
12. **Omfang.** Saksrammen berører historien. Den holdes adskilt fra bildemodusen.
13. **Motor-API.** `FWidgetRenderer` i pakket Linux, oscillatormodulert forsinkelse i lydgrafen og submix-ruting er ikke prøvd i dette prosjektet.
14. **Samordning.** Filmfiltrene var Codex sitt arbeid, og flere grener er åpne. Eierskap avtales i OPPGAVER.md, og tunge jobber går én om gangen.

### Spørsmål til Tom

1. Bildemodus først og saksramme senere, eller vil du ha saksrammen fra start?
2. Standard montering: brystsele (håndfri overvåkingskamera) eller skulder (klassisk camcorder)?
3. Farge som standard, med svart-hvitt som valg, eller omvendt?
4. Skal REC vises? Strengt tatt er REC et søkermerke og havner ikke på båndet. Forslaget har DATE AND REC som standard fordi det er det folk forventer.
5. Tolvtimers "PM 11:58" eller tjuefiretimers "23:58" i stempelet?
6. Skal stempelet fryse under de 47 sekundene og hoppe ved Event?
7. Skal SC02-trusselen gi forstyrrelse i det hele tatt, og i så fall bare når den er i bildet?
8. Musikk i camcorder-modus: uendret, dempet eller bare diegetisk lyd?
9. Skal camcorderklokka erstatte armbåndsuret i HUD-en?
10. Diegetisk opphav: SARO-feltvideosettet fra Operations (samme som Dale brukte), eller ingen forklaring i spillet?
11. Skal samme modul brukes til å produsere Dales FIELD 01-bånd etterpå?
12. Hvem eier arbeidet: Claude, Codex eller en deling som ved filmfiltrene?
