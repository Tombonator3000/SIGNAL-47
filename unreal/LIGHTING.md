# Ekte lys og skygger i SIGNAL / 47

## Grafikkleveranse 23. oktober 2026, foreløpig resultat

`port-gfx` viderefører WIP `64d943e7` med bedre lampeskygger, overflater, CRT-glass, mørkere natt, RX-indikatorer og en egen lastefase. `unreal-port-core` er flettet til `ae95b09a` via `a0eeed37`, etter prosessorgrensesnittet i `575311ca`. Claudes nye Props er dermed med i sluttkandidaten; modul og plasseringsfiler beholdes byte for byte fra core. Egne renderendringer endrer ikke geometri eller modellplassering. Versjonsparene fra før-kilden viser også Claudes innflettede modeller; en separat `-S47PreviousNight`-prøve sammenligner fylllyset med identisk ny geometri. Kandidaten bygges i `~/scratch-bm/Signal47-build`; standardinstallasjonen er ikke erstattet.

**Bygg, før-serie, pakket selvtest og to kandidatutsnitt er verifisert. Endelig PSO-bundle, oppstartssammenligning og full etter-serie gjenstår.** PASS nedenfor gjelder den navngitte prøven og pakken, ikke hele spillet eller en senere pakke.

Bevisroten er `/home/tom/scratch-bm/Signal47-gfx-2026-10-23/`. Maskinen har 31 GiB RAM, NVIDIA RTX 3080 med 10 240 MiB VRAM og driver 595.99.02, på Linux/Vulkan SM6. Toms installerte spill, PID 7609, ble observert avsluttet klokken 07:52 Europe/Oslo og ble aldri stoppet av agenten. Egne tunge jobber er kjørt sekvensielt etter prosess- og minnekontroll. Bare målinger med tom contention-liste brukes til fps; separate eller overlappende jobber gir ikke ytelsesbevis. `static/installed-game-grace.watch.jsonl`, `static/gpu.txt` og hver kjørings `meta.json`/`.job.json` bevarer grunnlaget.

### Lys, overflater og indikatorer

Nattens skylight er senket fra 0,18 til 0,055, månen fra 0,16 til 0,055 lux med kaldere farge, og lampenes indirekte bidrag fra 1 til 0,52. Verdiene glir mot den tidligere daggryresponsen. Direkte lampestyrke, lommelykt, billykter og fast eksponering beholdes, slik at lyspølene skiller seg tydeligere fra de ubelyste flatene. VHS-sortløftet er redusert fra `(0.026, 0.031, 0.033)` til `(0.014, 0.018, 0.022)`, med svak vignett og eksisterende korn. `-S47PreviousNight` er en isolert sammenligning av gammelt fyllnivå på de nye materialene; den erstatter ikke den separate før-pakken.

Kontaktspor og kildestørrelser følger rør, pærer, skjermflater og instrumentstripe. STATION 01, hytta, dineren og Caliche har kortere, fysiske lyskilder ved de eksisterende armaturene. High/Ultra beholder Lumen, med surface-cache-refleksjon på High og hit lighting på Ultra; Low bruker enklere direkte lys og AO. Low/High beholder 60 prosent intern oppløsning med TSR, Ultra 100 prosent.

118 eksisterende flate materialer får normaldetalj, ruhetsvariasjon og bred fargevariasjon i fysisk skala, med én deterministisk RGBA-detaljtekstur og verteksfargens smussbidrag. De fordeler seg på maling 23, vinyl 22, metall 18, betong 14, tre 12, jord 11, puss 7, gulv 5, vei 4 og asfalt 2. Papir, mennesker, flora, opprinnelige PBR-kart og emissive/gjennomsiktige flater er utelukket. Seks lit glassmaterialer beholder dielektrisk respons. De fire SARO-CRT-ene får en separat lit glassmaster med ruhet 0,22 og specular 0,5; `Map/Color/Brightness/Untonemapped` fortsetter å drive det opprinnelige bildet. Avslått fosfor har null emissive. KELVORA-filmflatene beholder sin unlit-kontrakt. Profilvalg, kartvern og determinisme er kontrollert i `static/materials-review.json`.

RX-rackets 18 indikatorinstanser brukte ett kildemateriale på seks GLB-spor, mens den gamle bindingen bare traff slot0. Alle seks spor får nå samme levende materiale og riktig instansefarge. `SetLed`/`FlickerLed`, mottakerens strøm og SARO-kutt driver lampene; kopier ved flimmer og utskrift beholder strømvernet. Konsollens emisjonskart, skriverlampen og prosessorens display/VU-flater får tilsvarende vern. CPU-kontrollen av den faktiske gatefunksjonen er PASS i `static/indicator-gates-review.json`.

V3-indikatorbildenes «off» isolerte ikke avslått tilstand: levende historieflimmer kunne tenne banklampene igjen. De bildene brukes derfor ikke som bevis for korrekt av/på. Testharnessen er rettet til å styre faktisk receiver-/K3-tilstand gjennom overgangene. V4 er visuelt kontrollert i `audit/indicators-v4-audit.json`: rack, konsoll og skriver fra v4-suiten, og en separat gyldig prosessor-retest etter kapasitetsavbrudd. Etter innflettingen får den nye DP-80-lampen, CB-basens tre flater og 3278-displayet fem egne strømstyrte native emissive-slots. EmissiveFactor skaleres, mens opprinnelig styrke, kart, kropp og glass beholdes. V5-prøven av disse modellene er PENDING.

### Materialcook og pakket kandidat

Første materialcook returnerte UAT exit0, men 179 materialinstanser falt tilbake til standardmaterialet. Denne pakken er merket FAIL og er ikke visuelt bevis: `build-logs/candidate-package-initial.validation.json`. Feilen var tilkoblingen til `StaticSwitchParameter`: Unreal Python forventer inngangene `True`/`False`. Importøren bruker nå disse navnene og avbryter ved mislykket tilkobling. `S47_ONLY=masters` reparerte eksisterende masterobjekter uten å bytte materialinstansenes foreldre eller regenerere geometri/teksturer. Kildekontroll og faktisk kjøring ligger i `static/material-switch-review.json` og `build-logs/candidate-master-repair.log`.

`package-linux.sh` avviser nå både «Failed to compile Material» og «Default Material will be used in game», selv om UAT melder suksess. Den reparerte materialpakken og den ferske pipeline-kandidaten består denne kontrollen. Sistnevnte bygget og pakket på 76,9 s uten overlapp, med stabile Vulkan-nøkler og uten de 179 shaderfeilene: `build-logs/pipeline-candidate-package.log` og `.job.json`.

Den lokale UE 5.8.4-stagingen kunne henge i Zen-opploggen etter ny cook, også med begrenset .NET-parallellitet. Pakking bruker derfor `-AdditionalCookerOptions=-SkipZenStore`, med IoStore beholdt. En fersk før-cook med denne løsningen besto på 36,9 s. Dette endrer lagringsløypa for cook, ikke baseline- eller kandidatbelysningen; motorfiler er ikke redigert. Tunge kompilatorjobber og shaderarbeidere er begrenset for maskinens minnebudsjett.

### Oppstart og PSO

`S47Startup` viser en terminal i S47Theme-stilen under synkron lasting og renderoppvarming. Hele verdenen bygges før ventefasen; WIP-feilen som returnerte fra `BeginPlay` før rom, himmel og native lys er fjernet. Verdenen tegnes bak skjermen mens spilltid, input, lyd og testhandlinger venter. Klar-signalet krever tom filcache-, RHI-, komponent- og shaderkø, minst tre sekunder etter klargjøring og en rolig periode med minst 45 frames. Ingen absolutt fps-grense kan holde et 30 fps-valg i endeløs lasting. Fokus, pause og overgangsframen er beskyttet; faktisk inputprøve gjenstår.

Den brede automatiske precachingen ga rundt 4 800 forespørsler og 73,4 s til READY i en tidligere kald kandidatprøve. Steady-state-fps skjulte dermed en uakseptabel ventetid. Nå er `r.PSOPrecache.Components=0` og `r.PSOPrecache.GlobalShaders=0`; Vulkan-precaching og filcache er fortsatt aktive. `ExcludePrecachePSO=0`, `IncludeAllPrecachedPSOs=0`, `PSOPrecacheIncludeMode=0` og `IncludeComputePSODuringCook=0` skal la bundlen dekke faktisk brukte graphics-/compute-PSO-er uten å legge til alle generiske varianter. Den ferske pipeline-pakken har disse verdiene.

Kildebanene er kontrollert i lokal UE 5.8.4: eksplisitt `PrecachePSOs()` respekterer component-cvaren; `.rec` lagres med `BoundPSOsOnly`; `IncludeMode=0` lar filcachen ta med registrerte entries uansett tidligere auto-resultat. `HasPipelineFileCache`/`GetTotalPSOCount` logges før verdensbygging, men et positivt antall alene beviser ikke full dekning. `static/pso-recording-source-audit.txt` dokumenterer dette. Nye Props-, material- eller renderendringer krever ny vurdering og nye opptak.

Den endelige bundlen skal bygges fra samme cook som de frosne `.shk`-nøklene og kontrolleres i cook, ferdig pakke og runtime. **Opptak, utvidelse, pakket cache, runtime-antall og kald/varm sluttmåling er PENDING.** En `.spc` i kilden eller en bestått pakking alene er ikke bevis for levert cache.

### Faktiske førmålinger og korte kandidatprøver

Før-kilden er `93ab1568`, med spillkode lik `c124920b`, bare trace-/kamerainstrumentering og private gamle Base/Mat-assets. `baseline-source/baseline-manifest.json` knytter kilde og innhold til hashene; pakken ligger separat i `baseline-package`. Den har ingen ny lasteskjerm, kandidatlys/-materialer eller ny PSO-konfigurasjon.

Hele før-serien har 20 kameraer ved natt 02:15 og 20 ved daggry 05:40 i 2560 × 1440 High/VHS. Nattens gyldige serie er batch1–4 fra `before-night-1440-v2` pluss `before-night-tail`; den gamle siste v2-batchen ble forkastet etter Vulkan device loss ved den store teleporteringen fra gammel vei til førerhuset. Førerhuset har nå en egen prosessgrense. De opprinnelige batch03/04 ved begge klokker er også forkastet fordi rom 6 og veien hadde arvet feil gulvhøyde. De erstattes av `before-camera-fix-night` og `before-camera-fix-dawn`, åtte utsnitt hver; øvrig daggrygrunnlag er `before-dawn-1440`. Feil kamera-kommaliste, feil viewport og et svart cab-bilde ble fanget av kontrollene; cab-opptaket venter nå på den virkelige kjøreovergangen før prøvetiden starter. Bare de validerte bildene og måleradene inngår.

`startup/before-cold-1440` og `startup/before-warm-1440` har komplette ti-sekundersvinduer og tom contention-liste. Begge skiller konstruksjonstiden fra synlig runtime. Synlig nullpunkt i før-pakken er første HUD-tick, en renderproxy, ikke en måling av fysisk skjermlatens. Rå intervaller som krysser ti-sekundersgrensen beholdes i hitch-statistikken.

| Før, første ti synlige sekunder | Frames | Frames per sekund, sekund 1 til 10 | Største frame |
| --- | ---: | --- | ---: |
| Kald, 2560 × 1440 High/VHS | 455, 45,5 fps | 0, 4, 38, 58, 56, 57, 61, 60, 61, 60 | 1296,9 ms |
| Varm, samme profil/drivercache | 1141, 114,1 fps | 9, 121, 122, 127, 128, 126, 128, 127, 127, 126 | 688,7 ms |

Dette er før-resultater. Endelig etter-måling må tas på pakken som faktisk inneholder den verifiserte PSO-bundlen.

`after/visual-check` er en gyldig kort kandidatprøve med to stillestående nattutsnitt, cirka 3,2 s sampling per utsnitt. Den viser kontrollrom 8,255 ms / 121,1 fps, p95 9,127 ms, og gård 7,169 ms / 139,5 fps, p95 7,831 ms. Begge ligger over 60 fps i denne prøven. De erstatter ikke gjennomgangen av de tyngste utsnittene, Low/Ultra eller hele 216-visningsmatrisen. Pakket `-S47SelfTest` består 1218 sjekker; siste bekreftede enkeltkjøring ligger i `tests/indicators-v4/selftest`, mens resten av V4-indikatorsuiten ennå ikke er komplett.

### Status før sluttføring

| Leveranse | Status og faktisk grunnlag |
| --- | --- |
| Kildeintegrasjon, materialreparasjon og streng pakkevalidering | PASS, siste pipeline-pakke 76,9 s; ingen materialfallback i cook |
| Før natt/daggry, 20 + 20 kameraer i 1440p | PASS, sammensatt nattserie og full daggryserie som angitt over |
| Før kald/varm oppstart og pakket selvtest | PASS, komplette trace-vinduer og 1218 selvtestsjekker |
| Kandidatens kontrollrom og gård, natt High/1440p | PASS for disse to korte utsnittene, 121,1/139,5 fps |
| Indikatorer av/på/flimmer/halvstrøm/SARO-kutt/tilbake | PENDING, full V4-serie og visuell kontroll; V3-off er ikke gyldig av-bevis |
| Endelig PSO-opptak, utvidelse, cook, pakket fil og runtime-antall | PENDING, må knyttes til samme cook og innhold |
| Endelig kald/varm oppstart, lastebilde, input og 30 fps-valg | PENDING, må kjøres på endelig cachepakke |
| 40 før/etter-bildepar og komplett etter-matrise, 36 tilstander × 2 klokker × 3 kvaliteter | PENDING, 216 målinger og vurdering av tyngste utsnitt |

De 20 verdenskameraene dekker kontrollrom/CRT, annex/korridor, tre labmoduser, gård/servicegård/parkering, motellet/rom 6, STATION 01/hytte, vei/diner/Caliche, gammel vei og førerhus. De 16 ekstra tilstandene dekker strøm, indikatorer, mottaker, lommelykt, frontlykter og menyer. Alle nye ytelses- og bildekonklusjoner skal vise til gyldig `meta.json`, faktisk pakkehash og fullført opptak. Den historiske dokumentasjonen under skiller fortsatt mellom tidligere kilde-, editor- og pakkebevis.

22. oktober 2026, `port-light`. Rendering, lys og materialer er endret. Dette lyspasset bevarer geometri og modellplassering; innflettede spillrettelser fra Claude er beholdt. Dette er en kildekandidat for Claudes integrasjon, ikke den installerte spillutgaven.

## Ny runde: utelys og fotolab, 22. oktober

Tom har spilt den pakkede leveransen fra `7c36dd2a`. Denne runden bygger videre på den. `unreal-port-core` er først flettet rent til `c79c6689` (gratismodellenes importløype), så til `3b44956e` (Claudes mørkeromsregel, varmere signalprosessor, brett og notatbok). Den installerte utgaven er urørt.

### Utvendig lys

`S47ExteriorLighting.cpp` erstatter de berørte brede punktlysene med 30 spotter ved de eksisterende armaturenes åpninger. Alle har fysisk kildestørrelse, myke skygger, kortere rekkevidde og volumetrisk spredning. Vegglampene lyser skrått ned langs fasaden; stolpene lyser på gangsti og asfalt. Dette gir lyspøler og rekkverksskygger, med mørke, kalde partier mellom lampene. Måne, himmel, eksponering og den godkjente varme antennerekken er bevart.

| Armaturer | Antall | Styrke og rekkevidde |
| --- | ---: | --- |
| Faktiske veistolpehoder, `ext.glow[11..18]` | 8 | 90 cd, 16 m; erstatter tre pool-sentre mellom stolpene |
| Vestveggen og sørveggen på østfløyen | 4 | 35 cd, 6 m; de to delte poolene driver fire faktiske vegglamper |
| Gangsti og biloppstillingsplass ved SARO | 4 | 65 cd, 9 m |
| B-12s arbeidslykt | 1 | 12 cd, 3 m; følger eksisterende nivå/dimming |
| Labdør, annexets sørvegg og rømningsdørens baldakin | 3 | 30/35/25 cd, 5/6/5 m |
| Motellets åtte dørarmaturer | 8 | 10 cd, 4 m; kort kast ned på vegg og gang |
| Motellets to parkeringslamper | 2 | 150 cd, 12 m; kilden sitter under den faktiske diffuseren |

Motellet står nå 650 m sør for SARO, slik hovedgrenens geometri angir. Det er kontrollert med eget kamera. Kontorets gamle «spill»-pool uten armatur er undertrykt; kontorlampe og det eksisterende native vinduslyset står for lyset der. Kopier av de erstattede poolene undertrykkes også, så samme lampe ikke gir dobbelt lys gjennom et annet materialsett. Kjeglene følger eksisterende flood- og strømtilstander; kombinert pool/glow-dimming brukes én gang. Room 6s dørglød følger samme tilstand og kan tennes igjen etter slukking.

De store additive glødskivene rundt disse lampene er krympet til glimt på 22 eller 40 cm og flyttet til de faktiske åpningene. Native lys og tåke gir lyspøl og lysspredning. Gårdens fire pæreflater har egne PBR-materialer på alle GLB-flater; emissive slukker med strømmen. Ingen ny armatur eller bygningsgeometri er lagt til.

### Fotolab og API for Claude

`S47LabLighting.cpp` har fem fysiske lyskilder: eksisterende rødt veggglass, takglass, arbeidslampens skjermåpning, forstørrerens linse og varsellampen ved døra. Takets eksisterende armatur brukes som hvit diffuser i normalmodus og rød i safelight-modus. Normalmodus bruker 4000 K, 45 lm i taket og 18 lm i arbeidslampen. Safelight bruker `#ff2616`, 24 lm ved benken og 12 lm i taket; den hvite arbeidslampen er av. Den 12 cm brede linseåpningen ved `(17, 1.50, -5.45)` har et smalt lys rett under seg, 8 lm og 1,2 m rekkevidde, ned på platen. Varsellampen er 0,5 lm. Dette er nivåer kalibrert for spillets faste eksponering.

`Web->Light(TEXT("lab.normal"))`, `lab.safelight` og `lab.enlarger` er logiske drivere med normalisert intensitet 0 til 1. De avgir ikke lys selv. Faktiske kilder, farge og diffusere følger dem sammen med labens eksisterende strømskala og individuelle lampenivå. Default er normal 1, safelight 0, enlarger 0. For selvstendig styring setter hver modus alle tre:

```cpp
// Normal room, before handling light-sensitive material.
Web->Light(TEXT("lab.normal")).SetIntensity(1.f);
Web->Light(TEXT("lab.safelight")).SetIntensity(0.f);
Web->Light(TEXT("lab.enlarger")).SetIntensity(0.f);
// Developing, after checking that the door is shut.
Web->Light(TEXT("lab.normal")).SetIntensity(0.f);
Web->Light(TEXT("lab.safelight")).SetIntensity(1.f);
Web->Light(TEXT("lab.enlarger")).SetIntensity(0.f);
// Printing: keep the safelight, expose only while the enlarger's lamp is on.
Web->Light(TEXT("lab.enlarger")).SetIntensity(1.f);
```

Koblingen til Claudes innflettede regel er ferdig: `US47Chapter1::Darkroom` setter normal/safelight ved faseskifte og forstørreren under det eksisterende kombinerte tank-/kontaktkopisteget (`Busy == "tank"`). Den slukkes når kopien venter på fiksering. Dørkrav, tåket kopi, negativ, tidsplan og lagring er Claudes kode. Det nye feltet `bPrintingLight` husker bare lysets faseskifte. Regelen beholder sin røde lampedimming på 1,5, som gir 36/18 lm i den faktiske fremkallingsfasen. SARO-kutt slukker også disse lysene; gjeninnkobling bevarer valgt modus.

Gamle røde lab-glødskiver er skjult. Glassflatene har PBR-emissive som følger de reelle kildene, også etter materialkopier fra strømendringer. Plastmeshen med skap, tank og lampehus gjøres ikke selvlysende. Labens fire gamle utfyllingslys er undertrykt. Det kreves ingen ny eksport, materialimport eller endring av delt generert innhold i denne runden.

### Nye bilder og kontroller

Bevismappe: `/home/tom/scratch-bm/Signal47-lighting-2026-10-22/revision3/`. `before` er den innflettede kandidaten `c79c6689` med uendret lys, `after` er denne kandidatens siste binærfil med hovedgren til `3b44956e`. Samme faste kameraer, kapittel 1, 1600 × 900, 100 % intern oppløsning og VHS. Hele bildene er beholdt uten retusjering. UI-lapper og møllenes fase kan variere. Den gamle kandidaten hadde ingen labmoduser; dens tre labbilder viser samme permanente røde lys og hvite arbeidslampe.

| Utsnitt | Side ved side, relativt til bevismappa |
| --- | --- |
| Gård/servicegård mot bygningene | `compare/yard.png` |
| Østdøra | `compare/east-door.png` |
| Labdøra | `compare/lab-door.png` |
| Parkeringsplass og sti mot antennene | `compare/lot.png` |
| Fra veien mot vestfasade og rømningsdør | `compare/road.png` |
| Motellets fasade og gang | `compare/motel.png` |
| Normal lablys, åpen dør | `compare/lab-normal.png` |
| Safelight, lukket dør | `compare/lab-safe.png` |
| Safelight med lys fra forstørreren | `compare/lab-print.png` |
| De tre nye labmodusene ved siden av hverandre | `compare/lab-modes.png` |

`after/lab-story.png` viser i tillegg rødt lys på karene ved faktisk kontaktkopi/fikseringstilstand, drevet av `-S47Ch1Drive=wet`. `iteration1` beholder den forkastede prøven med for store glødskiver og for svakt rødt benklys.

- PASS: editorbygg med `Scripts/build-editor.sh -NoUBA`, advarsler som feil, etter hovedgrenens siste fletting og kalibreringen. `build-final.log`.
- PASS: 1218 selvtestsjekker og målrettet kontaktkopiscenario. `after/lab-story.log`.
- PASS: ni kamerautsnitt i én 33-sekunders prøve; tre labmoduser kontrollerer at hvitt arbeidslys, safelight og forstørrer slukker i riktige moduser. `after/review.log`.
- PASS: labens halvstrøm, fullstendig strømkutt med null emissive og gjeninnkobling, via eksisterende `SetFloodScale`. Samme logg.
- PASS: 30 utelyskilder logget med kildeposisjon, mål, effekt og rekkevidde. Vulkan, uten fatalfeil eller materialshaderfeil i de nye kjøringene. `verification.json` knytter kildefiler, binærfil og bildepar sammen.

Det er brukt fire runtime-kjøringer: én førprøve, én forkastet etterprøve, én endelig etterprøve på 33 s hver og én selvtest/kontaktkopiprøve på 14 s. Før oppstart er øvrige Unreal-/Signal47-jobber kontrollert og avventet. Andre agenter startet jobber under førprøven; selvtesten registrerte én ekstra PID i ett oppstartsintervall. Den endelige kameraturen registrerte ingen andre tunge prosesser. Rådata ligger i `meta.json`-filene. Disse kjøringene brukes til bilder og funksjonskontroll, ikke ytelsesmåling.

Ingen ny, ren 1440p-måling er gjort i denne korte runden. Tallene lenger ned gjelder forrige leveranse: High/1440p med TSR 60 % ga kontrollrom 13,619 ms og gård 10,466 ms. De bekrefter ikke ytelsen med de nye utelysene. VSMs eksisterende marking-job-advarsel forekommer fortsatt ved oppvarming og vises i de første bildene; den er ikke skjult. Editorens eksisterende Python-toolset-feil ved oppstart er også registrert som før.

### Det integrator må kontrollere

Ingen pakking eller installasjon er gjort. `Scripts/package-linux.sh` er ikke kjørt. Claude må etter gjennomgang pakke i avtalt kø, prøve dør lukket før fremkalling, rød benk uten hvitt arbeidslys, lys fra forstørreren under prosesssteget, åpnet dør/tåket kopi og normal belysning etter avslutning, samt SARO-kutt og tilbake. Kontroller også motellets Room 6 av/på og stolpe-/rekkverksskygger i bevegelse. Gjenta High/1440p `stat unit` med én tung jobb om gangen før denne utgaven vurderes mot 60 fps-målet. Pakket kjøring og gjennomspilt fremkalling med ekte input er fortsatt UNVERIFIED for denne leveransen. Fullt `screenshots.sh` er ikke kjørt.

Resten av fila bevarer dokumentasjon og målinger fra forrige lysleveranse.

## Rettelser etter gjennomgangen

`unreal-port-core` er lest og flettet inn: først til `a3550f21` i `5f139abd`, deretter til `7d167000` i `9992ab7c` (notatbokas faktiske tastebindinger og oppgavelista). Flettingen beholder de nye terminalene på skjermene, Unarius med lyd, den kjøpte signalprosessoren, fri ferdsel og de øvrige spillrettelsene. En konflikt i `S47HUD.cpp` gjaldt bare kommentaren ved den felles CRT-gammarettingen.

### Lyset kommer fra armaturene

`S47FixtureLighting.cpp` bruker de faktiske meshenes transform og bounds. De gamle `roomLights.ceiling`, `lamps` og `crt` er bare tilstandsdrivere, med `bAffectsWorld=false`; de avgir ikke lenger lys mellom armaturene. Historienes eksisterende intensitets- og synlighets-API er bevart.

| Kilde i eksporten | Fysisk lyskilde |
| --- | --- |
| `room.tubes[0..3]`, noder 177, 178, 181, 182 | Fire rectlys, 120 × 26 cm, 1 cm under diffuseren, rett ned, kjølig `#eef3ff` |
| Annex/korridor, noder 484, 485, 490, 491, materiale `annex.tubeOn` | Fire tilsvarende rectlys under de faktiske rørene |
| De to skrivebordslampene, separate pærer 246 og 167 | Rectlys i skjermenes åpninger, 20 × 20 cm, langs skjermenes helning, varm farge fra den opprinnelige lampen |
| Labens røde glass, `yard.redGlass[0..1]`, noder 428 og 429 | 30 × 12 cm vegglys og 22 × 22 cm taklys fra glassflatene |
| Labens arbeidslampe | 20 cm kilde utenfor den skrå skjermåpningen, posisjon fra den isolerte armaturgeometrien |
| Arkivets grønne skrivebordslampe, node 555 | 30 × 10 cm kilde rett under skjermen |

De to permanent slukkede lysrørene i kontrollrommet, noder 179 og 180, forblir slukket. På High og Ultra kaster alle disse kildene skygger. Rørenes diffusere og pærene bruker PBR-materialer med emissive som følger samme dimming som rectlyset; av betyr null emissive og en passiv grå flate. Kalibrerte nivåer ved full strøm er 36 lm per romarmatur, 32 lm i annexet og 18/14 lm i rommets skrivebordslamper. Dette er kunstnerisk kalibrering for spillets faste eksponering, ikke et krav om at rommet skal holde vanlig kontorbelysning.

Lysrørenes GLB-er har seks materialflater selv om manifestet angir ett materiale. Undersiden er `slot3`; den gamle bindingen traff bare kortenden i `slot0`. Alle flater får nå samme levende diffuser-materiale. Det følger også materialkopier som historien oppretter ved flimmer, så ingen gammel emissive-kopi blir hengende igjen.

Annex-porten hadde en feilkopling: det flimrende røret er ved `(-4.6, 2.735, 8)`, men `FlickerFlood=4` lå ved det andre arkivarmaturet. Den fysiske lyskoblingen legger nå denne tilstanden på det faktisk flimrende røret. Ingen historie eller tidsplan er endret. Labens gamle records-desk-pool uten noen lampemodell er undertrykt; den virkelige arbeidslampen og Lumen gir lyset der.

### Grønt lys foran skjermene

`S47ScreenLighting.cpp` fitter rectlys til skjermflatene, med grønt `#8cffa4`, myke skygger og kort rekkevidde. De fire CRT-glassene er faktisk 35,49 × 28,35 cm. De tre ved vinduet vender sørover i rommet; driftsterminalen vender østover. Hvert bruker 20 lm og 1,8 m rekkevidde. Dermed får tastatur, bord og nærliggende flater sin egen lokale grønnfarge.

Lysene følger skjermens egen strøm, faktisk synlighet, materialfarge og lysstyrke. Status/spektrum/tracking følger dessuten RX-bankens glød. `SS47Crt` sender samme svarte glitchramme til lyset som til skjermbildet. Ops beholder sin egen strøm. SARO mørkt slukker begge. Skjermene ved S-03 og i annexet har egne flatelys; annexets gamle punktpool er erstattet. Begge signalprosessorvariantene støttes, og den nye modellens eksisterende display adopteres først når det er opprettet. Den gamle rackskjermen lyser ikke gjennom den skjulte modellen. Signalprosessorens display og emissive følger SARO-strøm og dip.

Eksporten har ikke egne CRT-er i arkivet eller STATION 01. Annexets registrerte `screens`-flate er automatens belyste front; denne er tatt med. STATION 01s mottaker har analog skala og beholder sitt eksisterende varme lys. Amber VU-metre gjøres ikke grønne. Den kjøpte oscilloskopmodellen har ingen aktiv skjermflate eller strømsetter i denne eksporten.

Raske skjerm- og rørlys har volumetrisk spredning 0 for å unngå etterslep ved flimmer. Lommelykt, frontlykter og faste utelamper gir fortsatt lysstråler i den ekte volumetriske tåka.

### Den varme antennerekken

De fem opprinnelige array-poolene beskrev sentre ved Y=3 m. De fysiske armaturene står ved Y=6,5 m. De var dessuten feilaktig rettet ned i første native-kandidat. Nå står spottene på armaturene og lyser skrått opp/inn på S-03, S-02, S-01, S-04 og S-05. Målpunktene beregnes fra parabolenes eksporterte plassering, skala og gjeldende retning.

Seks ytterligere eksisterende feltarmaturer, eksportens `ext.glow.points[5..10]`, lyser mot S-07, S-06, S-09, S-10, S-11 og S-12. Ingen ny lampemodell er lagt til. Alle elleve har varm natriumfarge, fysisk kildestørrelse og skygger. De deler arrayets eksisterende av/på- og rødpulstilstand. De røde flylysene er bevart. Første prøve ble for lys på skålene; styrken ble senket til 1800/540/500/500/500 cd for de fem fremste og 480 cd for de seks andre.

## Felles renderoppsett

Ingen Web/Unreal-bryter finnes. Materialenes flood- og hemisphere-bidrag, gamle Ultra-proxyer, additive lyskjegler og vindusprojeksjoner er av. `SetFlood`, `SetFloodPool`, `SetFloodColor`, `SetFloodScale` og delte lampetilstander driver native lys. Lommelykta bruker 420 cd og to frontlykter 4200 cd hver, med skygger og volumetrisk spredning. Vinduer bruker native rectlys ved de eksporterte rutene.

Lumen bruker Vulkan hardware ray tracing på RTX 3080 når tilgjengelig. Måne og skylight er svake og kalde; sol og himmel følger daggryet. `ExponentialHeightFog` med volumetri erstatter materialtåka. Eksponeringen er fast. `M_WebTone` beholder ACES-kurven og VHS/Clean, uten FogExp2. CRT-render targets bruker `FWidgetRenderer(false)` for én sRGB-koding.

| Kvalitet | Lumen | Skygger | Volumetri | Intern oppløsning |
| --- | --- | --- | --- | --- |
| Low | Av | Enklere lokale skygger | Av | 60 %, TSR |
| High, standard | Scene/final gather 0,5 | VSM, local bias 1, fire SMRT-stråler × to prøver | 16 piksler, 64 dybdelag | 60 %, TSR |
| Ultra | Scene/final gather 1 | VSM, local bias 0, flere prøver | 8 piksler, 96 dybdelag | 100 % |

High i 2560 × 1440 tegner omtrent 1536 × 864 internt. Low/High bruker 100 % TSR-historikk. Ultra prioriterer full oppløsning. VHS/Clean og lysstyrke er uavhengige av Quality.

## Generert innhold før integrasjon

Materialsteget fra første leveranse kreves med eldre generert innhold. Full eksport eller geometriimport er ikke nødvendig. `NativeLighting` i `MPC_Web` har default 0 for kompatibilitet med andre checkouter; denne runtime setter alltid 1. Denne rettelsesrunden bruker eksisterende materialparametre og trenger ikke et nytt importsteg utover dette.

Arbeidet bruker private kopier av `Content/Web`, ikke hovedkopiens delte Base/Mat. `S47_ONLY=materials` skriver bare Base og Mat, ikke Geo, Tex, art eller WebData. Importen har et kjent avslutningskrasj etter `S47WEB done`; lagrede assets må valideres i runtime. Audio, Licensed og WebData lånes med midlertidige lenker, som fjernes før commit.

```sh
cd unreal/Signal47
S47_ONLY=materials S47_WEB="$HOME/scratch-bm/Signal47-web" \
  "$HOME/.local/opt/UnrealEngine-5.8.4/Engine/Binaries/Linux/UnrealEditor-Cmd" \
  "$PWD/Signal47.uproject" -run=pythonscript \
  -script="$PWD/Tools/webexport/import_web.py" \
  -ddc=InstalledNoZenLocalFallback -unattended -nullrhi
```

## Bilder og målinger

Bevismappe: `/home/tom/scratch-bm/Signal47-lighting-2026-10-22/`. Første levering er bevart i `before`, `after`, `compare` og `perf-final`. Rettelsene ligger under `revision2`. PNG-ene er ekte Vulkan-runtime. `iteration1` beholder den forkastede, overbelyste prøven. De endelige sammenstillingene bruker 1600 × 900 og 100 % intern oppløsning. De nye utsnittene bruker normalt 20 sekunder; de bevarte førbildene fra første levering bruker åtte. Kamera og kapittelhopp er like; notatlapper og annen tidsstyrt UI kan være forskjellige. Den nye signalprosessoren i rommet kommer fra hovedgrenens fletting, ikke et modellbytte i lysarbeidet. Varsler er ikke slått av; de siste fangstene venter på oppvarming av skyggekartene.

### Siste sammenligninger

Alle stier nedenfor er relative til bevismappa. `revision2/compare` er den nye leveransen; de gamle `compare`-bildene i rotmappa dokumenterer første kandidat.

| Utsnitt | Side ved side |
| --- | --- |
| Kontrollrom | `revision2/compare/room.png` |
| Arrayet fra gården | `revision2/compare/outside.png` |
| Arrayet gjennom vinduet | `revision2/compare/window.png` |
| Takarmaturer, kamera vendt opp | `revision2/compare/ceilings.png` |
| Annex/arkiv | `revision2/compare/annex.png` |
| Korridor | `revision2/compare/corridor.png` |
| CRT-er og grønn glød på tastatur/bord | `revision2/compare/crt.png` |
| Ops og skrivebordslampe | `revision2/compare/ops.png` |
| Labens safelights og arbeidslampe | `revision2/compare/lab.png` |
| SARO mørkt | `revision2/compare/dark.png` |
| Lommelykt og rekkverkskygger | `revision2/compare/flashlight.png` |

`revision2/compare/candidate-outside.png` viser den rettede gården ved siden av første native-kandidats svarte paraboler. `revision2/compare/ops-power.png` viser samme Ops-kamera med og uten stasjonsstrøm. Sammenstillingene beholder hele bildet; de er ikke retusjert.

### RTX 3080, Vulkan, 2560 × 1440

25 sekunders kjøringer med `stat unit`, VSync og bildegrense av. `S47LightPerf` sampler etter fem sekunders oppvarming. Selvtesten kjøres separat fra ytelsesprøvene; den nye hovedgrenens ekstra lyd-/UI-kontroller skal ikke inngå i en rammetidsmåling. Skjermfangsten er med i de siste akkumulerte tallene.

| Utsnitt | Ramme, ms | p95, ms | Game, ms | Draw, ms | GPU, ms | FPS | Rammer |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Kontrollrom, High/VHS | 13,619 | 15,330 | 3,881 | 6,777 | 12,718 | 73,4 | 1407 |
| Gård, High/VHS | 10,466 | 11,527 | 3,183 | 5,606 | 9,702 | 95,5 | 1824 |
| Kontrollrom, Low/Clean | 11,113 | 12,168 | 2,930 | 4,794 | 10,366 | 90,0 | 1721 |
| Gård, Ultra/VHS | 25,503 | 28,216 | 3,060 | 5,866 | 23,999 | 39,2 | 725 |

High når 60 fps-målet i begge avtalte faste utsnitt, også etter p95. Dette er TSR fra 60 % intern oppløsning til 1440p, ikke nativ 1440p. Ultra når ikke 60 fps. Trådtidene overlapper og summeres ikke.

Ingen annen tung PID er registrert i noen av de fire målevinduene. Kontrollrommets monitor så én ekstra PID i ett halvt sekund ved prosessens t=6,05 s under initialisering, rundt 18:18:49,84 UTC. Sampling begynte først 18:19:03,63 UTC. Råregistreringer, eksakte kommandoer og samplingstider ligger i `revision2/perf/*-contention.json`, `*-meta.json` og `revision2/results.json`. Kontroll før oppstart krever åtte sammenhengende ledige sekunder og omfatter også prosessnavnet `Signal47`.

Målingene bruker den endelige renderkoden. Den etterfølgende flettingen av notatbokas tastetabell endrer ingen lys- eller renderkode; den er bygget og selvtestet på nytt. Dette er editorens `-game`, ikke en måling av pakket shipping eller en hel kjøretur.

### Teststatus og grenser

- PASS: siste flettede editorbygg, `Scripts/build-editor.sh -NoUBA`, advarsler som feil. `revision2/build-merged-final.log`.
- PASS: 1203 selvtestsjekker etter siste fletting. `revision2/after/room.log`.
- PASS: tolv målrettede Vulkan-utsnitt med `smoke test OK`, uten kritiske feil. Elleve før/etter-par og to supplerende sammenstillinger.
- PASS: armaturenes faktiske posisjoner, størrelser og normaler logget i runtime. Annex-flimmeret driver node490 mellom 32 og 10,47 lm; de andre rørene holdes stabile. Sluttbildene viser hele den aktive diffuseren lysende.
- PASS: SARO mørkt slukker rør, skrivebordslamper, CRT-flatelys og CRT-emissive; Ops-prøven er også tatt med eget kamera. En materialkopi fra flimmer kan ikke beholde gammel glød på boksens underside.
- PASS: Vulkan logger aktiv ray tracing og ray tracing-shadere. Lumen hardware ray tracing er valgt; separat software-fallback er ikke målt.

`[VSM] Non-Nanite Marking Job Queue overflow` forekommer fortsatt under oppvarming, nå også i SARO-utsnittene med flere reelle kilder. Motorens shader behandler overskytende arbeid sekvensielt. Ingen manglende skygger er sett i de inspiserte bildene; advarselen og oppstartskostnaden er ikke reparert eller skjult. Sluttfangstene venter på at kartene blir varme. Dette må også kontrolleres ved bevegelse i den pakkede utgaven. Motorens eksisterende Python-toolset-feil ved editorstart og Vulkan-allokeringsvarsler ved avslutning forekommer også i førkjøringene; de er ikke kompilator- eller materialshaderfeil i kandidaten.

### Integratorens pakkekontroll

`Scripts/package-linux.sh` er ikke kjørt: den skriver til den delte `~/scratch-bm/Signal47-build`, slik Tom uttrykkelig ba oss unngå. En eldre pakke kan ikke verifisere denne nye C++-kandidaten. Claude må etter fletting:

1. Regenerere materialene ovenfor i avtalt innholdskopi før cook, og kontrollere at `NativeLighting` følger med.
2. Bygge/pakke og kjøre `-S47SelfTest` i den ferske pakken.
3. Kontrollere rom, vindu, gård, korridor, annex og lab; taklyset må sitte i diffuseren, skjermlyset foran glasset og parabolene være varmt lesbare.
4. Kontrollere SARO mørkt og tilbake, relédip/CRT-glitch, annex-flimmer, arrayets slukking/rødpuls, lommelykt og frontlykter.
5. Gjenta High/1440p med `stat unit`, én tung jobb om gangen. Se etter savnede shaderpermutasjoner, lyslekkasjer og VSM-advarsler under bevegelse.

Pakket kjøring, menneskelig vurdering på Toms skjerm og langvarig bevegelse er fortsatt UNVERIFIED. Fullt `screenshots.sh` er ikke kjørt, i tråd med bestillingen. Den installerte utgaven og starteren er uendret.

## Kilder

Prosjektbibliotekets søk fant [Scenario Labs, UE 5.8 lighting/rendering](https://github.com/scenario-labs/skills/blob/f0adb5b5ebc1fc8f7e333554d2aeb767c5558d6b/skills/game-engines/unreal/scenario-unreal-lighting-rendering/SKILL.md), MIT. Referansen er brukt for fysiske enheter og små emittere; dens helpers er merket uprøvd i Unreal og er ikke gjenbrukt. Motorens lokale C++-API, eksportens GLB-bounds og prosjektets faktiske runtime-prøver er grunnlaget for implementasjonen. [Epics Lumen-dokumentasjon](https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-technical-details-in-unreal-engine) ble kontrollert i første runde.
