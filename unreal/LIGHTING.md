# Ekte lys og skygger i SIGNAL / 47

## Grafikkleveranse 23. oktober 2026, cache og oppstart verifisert

`port-gfx` viderefører WIP `64d943e7`, `be7c89e5` og `30d407be`. Core `ae95b09a` med Claudes Props er innflettet via `a0eeed37`; nyeste core `9d7571e2`, dokumentasjon om videre gameplay, via `88353b0c`. S47Props og Config/Props beholdes uendret fra core. Egne renderendringer flytter ingen geometri. Kandidaten ligger i `~/scratch-bm/Signal47-build`; installert spill og standardstarter er ikke erstattet.

**Bundlet graphics-PSO-cache, kald/varm oppstart og 40 før/etter-par er PASS.** Første ti synlige sekunder på sluttpakken har ingen rammer over 33,333 ms: kald 1111 rammer / 111,1 fps, varm 1095 / 109,5 fps. Sluttpakkens sju indikatorprøver og korrigerte strømtilstandsbilder er også PASS. Alle åtte lengre High-prøver og seks sluttregresjoner er PASS. De to identisk-geometri fyllparene er også visuelt PASS. PASS gjelder navngitte bevis, ikke hel gjennomspilling med menneskelig input.

Bevisrot: `/home/tom/scratch-bm/Signal47-gfx-2026-10-23/`. Linux/Vulkan SM6 på RTX 3080, 10 240 MiB VRAM, driver 595.99.02 og 31 GiB RAM. Egne tunge jobber kjøres sekvensielt etter free/pgrep og minst 10 GiB tilgjengelig. Tom avsluttet selv gammel PID 7609 før 07:52:33, startet installert spill som PID 171598 kl. 11:00:59 og avsluttet før 11:06; opptaket ventet. Ingen installert prosess er stoppet. Konkurrerende, avbrutte eller feiloppløste kjøringer brukes ikke som fps-bevis.

### Nattkontrast, skygger og overflater

Skylight om natta er 0,055 mot tidligere 0,18, månen 0,055 mot 0,16 lux med kaldere farge, og lampenes indirekte nattbidrag 0,52 mot 1. Tidligere daggryendepunkt beholdes. Direkte lamper, lommelykt, billykter og fast eksponering har samme styrke. VHS-sortløftet er redusert fra `(0.026, 0.031, 0.033)` til `(0.014, 0.018, 0.022)`, med svak vignett og eksisterende korn. Ubelyste hjørner og mark er mørkere, mens varme lampefelt og leselige gangflater skiller seg tydeligere ut.

Kontaktspor og kildestørrelser følger rør, pærer, skjermflater og instrumentstripe. STATION 01, hytta, dineren og Caliche har kortere fysiske lyskilder ved eksisterende armaturer. High/Ultra har Lumen, med surface-cache-refleksjon på High og hit lighting på Ultra; Low enklere direkte lys og AO. Low/High bruker TSR fra 60 % intern oppløsning, Ultra 100 %.

118 flate materialer har normaldetalj, ruhetsvariasjon og bred fargevariasjon i fysisk skala, med deterministisk RGBA-detaljtekstur og verteksfargens smussbidrag. Fordeling: maling 23, vinyl 22, metall 18, betong 14, tre 12, jord 11, puss 7, gulv 5, vei 4 og asfalt 2. Papir, mennesker, flora og opprinnelige PBR-kart er bevart. Seks glassmaterialer beholder dielektrisk respons. Fire SARO-CRT-er bruker separat lit glassmaster med ruhet 0,22/specular 0,5 og opprinnelig bilde-/strømkontrakt. Avslått fosfor har null emisjon. KELVORA-filmflatene beholder unlit. `static/materials-review.json` og faktisk pakket runtime dokumenterer materialene.

### RX-bank og små emissive flater

Rackets 18 LED-instanser brukte ett kildemateriale på seks GLB-spor; gammel binding traff bare slot0. Alle seks har nå samme levende materiale og riktig grønn/amber instansefarge. SetLed/FlickerLed, faktisk receiver/K3-strøm og SARO-kutt driver dem. Konsollens emisjonskart og prosessorens bilde-/glass-/VU-flater får tilsvarende strømvern, også etter materialkopier under dimming.

Etter Props-bygging oppdages fem native glTF-emitterspor: DP-80s to grønne dioder i én materialslot, CB-basens to målere og lampe, samt 3278-displayet. Bare EmissiveFactor skaleres med stasjonsstrøm. Opprinnelig EmissiveStrength, emisjonskart, PBR-kropp og glass beholdes. Oppdagelsen caches på adopterte noder uten å endre Props.

V5 består 1220 selvtestsjekker og fem apparatvinkler med 30 fasebilder, `tests/indicators-v5`. Rack-off/dark er visuelt slukket; online/flimmer/restore lyser tydelig. DP-80, CB og prosessor er strømprøvd med bevarte kart/styrker og MID-kloning i dark. V9 viser faktisk 3278-display fra frontsiden, `audit/terminal-v9-audit.json`. Receiver-off betyr ikke at separate stasjonsapparater er av; dark tester SARO-kutt.

PSO-matrisens dark-bilder er ikke selvstendig strømbevis: den pausede grafikkharnessen slo ikke av alle virkelige lampedrivere. Dette er rettet diagnostisk med uniform-strømverdier og gjenoppretting ved neste kamera; ingen shaderpermutasjon er endret. Sluttpakkens komplette seksfasetest og `after/final-state-correction` er nå PASS og gir korrekt strømbevis for alle seks valgte grafikktilstander. De sju sluttprøvene, inklusive selvtest (1220), tok 228,8 s; 36 fasebilder er kontrollert med samme slutt-ELF og fem containere i `audit/final-indicators-audit.json`. Ikke bruk gamle matrise-dark-bilder som PASS. V3s tidligere feilaktige off-bilde og ugyldige terminalkameraer er bevart som forkastet historikk.

### 40 ekte før/etter-par

`compare/index.html` viser 20 nattpar og 20 daggrypar med begge uretusjerte original-PNG-er. `compare/night-manifest.json`, `dawn-manifest.json` og `audit/areas-v3-audit.json` knytter 80 originale bildehash, 40 par og 202 lokale gallerilenker til metadata/pakkehash. Alle 20 natt- og 20 daggryetterbilder er visuelt kontrollert, `audit/areas-v3-visual-audit.md`.

Kameraene dekker kontrollrom/CRT, annex/archive/korridor, normal/safelight/printlab, gård/servicegård/parkering, motell/rom6, STATION 01/hytte, vei/diner inne/ute, Caliche, gammel vei og cab. Natta har mørkere ubelyste flater med lokale varme lyspøler. Daggry har lysere terreng, avgrensede vindusskygger og fortsatt mørke lukkede rom. De mørkeste veiene er synlige som svak kald vei/horisont; stillbilder alene beviser ikke tilgjengelighet under fri ferdsel.

Før-kilden er 93ab1568 med spillkode lik c124920b, diagnostikk og private gamle Base/Mat-assets, ingen nye Props eller lasteskjerm. Referansen bruker korrigerte kameraer i rom6 og på veien; gamle under-gulv-/feilviewport-/svarte cab-opptak er forkastet. Etterbildene er fra opptaks-ELF 8611c1938b815f1cb786ebb5ae004c64234567177995417aa3f9da17be64ba95 med nyere Props. Disse versjonsparene isolerer derfor ikke modelltillegg fra lys/materialendringer. `after/previous-night` er PASS for gammelt fyll på identisk ny geometri/materialer, kontrollrom og gård. `compare/contrast-control.png` og `compare/contrast-yard.png` er visuelt PASS, med hash/proveniens i `compare/contrast-manifest.json`. Identiske 9243 cooked render-/asset-/shaderfiler isolerer her fyllforskjellen: mørkere ubelyste hjørner/mark, mens lampenes lokale pøler beholder styrken.

Lommelykt og frontlys er også tegnet i ekte av/på-tilstander, `audit/night-light-states-v3.png`. Frontlysenes effekt på bakken er tydelig. Lommelykta svært nær S-03 gir en utbrent flekk; bildet viser virkning, men er ikke bevis for leselig panel ved normal gåavstand. Dette er oppgitt uten å endre originalbildet.

### Levert Vulkan-PSO-cache

`pso-final-v3/recordings.json` er komplett og ukonkurrerende: 38 tilstander × Low/Clean, High/VHS, Ultra/VHS × 02:15/05:40, 228 tilstander i 60 korte prosesser. Warmup 4 s / hold 4 s gir cirka 1,7 s rå rammetider per tilstand. De 20 verdenskameraene suppleres av 18 tilstander for strøm, receiver, portable lys, menyer og terminalfront. Frosne .shk, forespørsel, ELF-/containerhash og faktiske PNG/CSV/logger følger opptaket. Resume gjenbruker bare identiske, hashkontrollerte PASS-blader; avvist materiale bevares utenfor aktivt tre.

Kildens `Signal47/Build/Linux/PipelineCaches/S47_Signal47_SF_VULKAN_SM6.spc` er 92 902 byte, SHA256 `adc3bfa7a80f891caa687416fe064dd0206ea1c4d8963e6160cf117b7037babd`, med 4215 stabile PSO-er og 37 480 stabile nøkkelreferanser. Ekspansjonen består på 7,6 s, `pso-final-v3/expanded.json`. Cooken løser dem til 602 unike PSO-er, 0 avvist og 3613 sammenslått. Tre TSR-computevarianter filtreres av motorens kontroll for mulige ustøttede RHI-funksjoner. Sluttfilen har 599 binære PSO-er: 222 graphics, 310 compute og 67 RT.

`pso-bundle-proof.json` har alle ni kontroller PASS. Cooken laster vår .spc; cachebytes er identiske i cookutdata/Metadata. Pakket runtime åpner samme fil/GUID `01A16826498D7808B71EA771B4891BF2`, køer 599 oppgaver og behandler 222 bundled graphics-PSO-er (type 1). Fysisk Open-linje og prosjektaliaset Signal47 matches på GUID og antall, uten å godta usercache. Binær cache er 156 709 byte, SHA256 `f3c1e45b3cf329c20bbfd37fd41138a21cf38a1944b392d095b77d2950466293`. Et compute-only antall eller bare en vellykket pakking ville ikke gitt dette beviset.

Alle 9243 registrerte cooked material-/mesh-/tekstur-/shaderfiler, inkludert shaderarkivene, er byteidentiske mellom opptak og sluttcook. Global.shk er også byteidentisk. Signal47.shk er 460 092 mot 470 335 byte med forskjellig serialiseringsrekkefølge, men eksakt samme 21 184 logiske nøkkel-/pipeline-/outputhash-tupler, 3308 hashposter og 2217 navn, ingen shader-outputendring. Frosne opptaksnøkler er urørt. `static/final-cook-equivalence.json` og `stable-keys-logical-comparison.json` skiller logisk likhet fra ulik filhash; de lokale UE 5.8-kildebanene viser at TSet/navn/hashindekser lagres i møteorden. Sluttpakke/ELF er ikke samme fil som opptaket: C++-diagnostikkens uniform-/restore-rettelse og bundlen er lagt til etter opptaket, uten endring i renderpayload eller shadernøkkelinnhold.

Den tidligere brede auto-precachingen ga rundt 4800 forespørsler og 73,4 s kald READY. Nå er Components/GlobalShaders-auto av, Vulkan-precaching og filcache på, og faktisk bundne PSO-er inkludert. Ingen generell compute-mengde legges automatisk til. Sluttpakken bygget/cooket på 83,7 s uten materialfallback. V2s to shutdown-SIG11-kjøringer er forkastet; etter opprydding av diagnostikkens statiske Slate-tittel består hele v3, også den tidligere feilsekvensen med logPSO. ELF SHA256 er `c69b1e0ab2f481a4eeb23c1d25e1c4a527fd141389f40ab0b6086a826a91eef5`; alle containerhash ligger i sluttmålingenes meta.json.

### Lasteskjerm og de første ti sekundene

S47Startup viser S47Theme-terminalen under verdenslasting og renderoppvarming. Verdenen tegnes bak skjermen mens spill-/testtid, input og lyd venter. READY krever tomme relevante compilekøer, minst tre sekunder klargjøring og minst 45 rolige frames, uten absolutt fps-krav. Den målrettede mellomkandidaten før bundle hadde 9,184 s kald READY; sluttpakkens faktiske tall er:

| Oppstart, 1440p High/VHS | READY fra verdensstart | Rammer første 10 verdenssekunder | Rammer første 10 synlige sekunder | Synlig p95 / maks |
| --- | ---: | ---: | ---: | ---: |
| Før kald, uten lasteskjerm | ingen port | 24 / 2,4 fps | 455 / 45,5 fps | 22,905 / 1296,925 ms |
| Før varm | ingen port | 663 / 66,3 fps | 1141 / 114,1 fps | 8,623 / 688,740 ms |
| Etter kald, pakket cache | 11,337 s | 120 / 12,0 fps | 1111 / 111,1 fps | 16,678 / 31,585 ms |
| Etter varm, samme drivercache | 5,594 s | 788 / 78,8 fps | 1095 / 109,5 fps | 18,596 / 31,888 ms |

Etter kald synlig sekund 1 til 10: 108, 110, 108, 113, 112, 110, 112, 113, 113, 112 rammer. Etter varm: 107, 110, 109, 108, 110, 110, 111, 110, 110, 110. Begge har null synlige intervaller over 33,333 ms og 50 ms. Før kald hadde henholdsvis 9 og 7; før varm 8 og 3. Dette dokumenterer at de store oppstartshakkene nå ligger bak skjermen; det er ikke en påstand om at hvert synlig bilde holder 16,667 ms.

`startup/before-cold-1440`, `before-warm-1440`, `after-cold-1440` og `after-warm-1440` har komplette rå spor, gyldig 2560 × 1440 og ingen konkurrerende tung prosess. Verdensvinduet starter før AS47Web bygges; synlig-vinduet bruker første HUD/READY som proxy, uten fysisk presentasjons- eller OS-prosessstartmåling. Hele rå frameintervaller som overlapper vinduet tas med i hitch-statistikken. Lastebildet er faktisk runtime, `startup/after-cold-1440/loading.png`.

### 1440p, korte kameramålinger og sluttprøver

`final-measurements.json` og `.md` oppsummerer de 40 normale verdensutsnittene fra opptaket. Alle High-gjennomsnitt er over 60 fps. Nattens svakeste er 96,1 fps, største kamera-p95 17,281 ms; daggry 90,1 fps og 21,772 ms. Dette er omtrent 1,7 s prøve per kamera, med enkeltintervaller over 16,667 ms, ikke endelig steady-state-bevis.

| Fire tyngste korte High-utsnitt | Natt fps / p95 ms | Daggry fps / p95 ms |
| --- | ---: | ---: |
| motel | 96,1 / 11,924 | 90,1 / 13,186 |
| cab | 100,8 / 16,923 | 90,9 / 20,423 |
| control | 104,6 / 17,281 | 95,1 / 21,772 |
| annex | 106,2 / 16,884 | 100,6 / 18,580 |

| Kvalitet, 40 verdensutsnitt | Intern TSR | Kamera-fps, min–maks | Vektet fps |
| --- | ---: | ---: | ---: |
| Low/Clean | 60 % | 115,1–318,7 | 171,7 |
| High/VHS | 60 % | 90,1–198,7 | 122,6 |
| Ultra/VHS | 100 % | 41,6–92,9 | 60,2 |

Vektet fps er sum rå rammetelling delt på sum rå intervalltid, ikke snitt av kameraenes fps. Ultra prioriterer full oppløsning og har flere utsnitt under 60 fps; målet gjelder High. `after/heaviest-night` og `after/heaviest-dawn` er begge PASS på ferdig cachepakke, hold 15 s og cirka 7,2 s rå rammetider per kamera. De fire tyngste utsnittene fra den korte matrisen er prøvd ved begge klokker:

| Lengre High/1440p | Natt fps / p95 ms | Daggry fps / p95 ms |
| --- | ---: | ---: |
| cab | 99,45 / 18,142 | 100,92 / 13,934 |
| motel | 97,84 / 13,893 | 93,81 / 13,002 |
| control | 105,02 / 16,875 | 98,73 / 17,910 |
| annex | 106,73 / 17,080 | 103,58 / 16,143 |

Alle åtte gjennomsnitt er 93,81–106,73 fps. Største p95 er 18,142 ms, så dette er ikke låst 60 fps for hvert intervall. `heaviest-summary.json` bevarer alle rå tellinger, måle-/pakkehash og vektet sammenfatning. Ingen konkurrerende tung prosess inngår.

Sluttpakkens indikatorserie `tests/final-indicators` er komplett PASS: selvtest (1220) og seks apparatvinkler, rack, console, printer, processor, auxiliary og terminal, hver med seks kontrollerte strømfaser. `audit/final-indicators-audit.json` knytter 36 fasebilder til slutt-ELF/containerhash. Korrigerte `after/final-state-correction` viser også faktisk av/på for rack, native apparater og terminal. `tests/final-regression` er komplett PASS på 151,3 s: tittel, pause, notatbok, kjøring, THE EVENT og valgt 30 fps. Den synlige cap30-tracen har 285 intervaller etter 0,5 s, gjennomsnitt 33,333457 ms, som bekrefter 30 fps-valget etter READY. Fokus-/pauseovergangen prøves gjennom disse diagnostiske menytilstandene; menneskelig input er ikke utført av denne suiten. Disse diagnostiske scenarioene erstatter ikke menneskelig helgjennomspilling og lytting.

### Reproduksjon av genererte materialer før integrasjon

Content/Web/Base og Content/Web/Mat følger ikke kildecommiten. En integrator med eksisterende Geo/Tex/WebData fra eksporten, øvrige spillassets og manifestet i `~/scratch-bm/Signal47-web` må kjøre hele materialsteget. Bruk private Base/Mat hvis flere checkouter deler generert innhold. Ingen annen tung Unreal-jobb må kjøre, og minst 10 GiB skal være tilgjengelig:

```sh
cd unreal/Signal47
bash Scripts/build-editor.sh
python3 Scripts/unreal_guard.py require-idle
S47_ONLY=materials S47_WEB="$HOME/scratch-bm/Signal47-web" \
  "$HOME/.local/opt/UnrealEngine-5.8.4/Engine/Binaries/Linux/UnrealEditor-Cmd" \
  "$PWD/Signal47.uproject" -run=pythonscript \
  -script="$PWD/Tools/webexport/import_web.py" \
  -ddc=InstalledNoZenLocalFallback -unattended -nullrhi -nosplash
bash Scripts/package-linux.sh
```

Materials bygger mastere og alle 799 MI-er med 118 overflateprofiler, seks glassvalg og fire CRT-valg uten å skrive Geo/Tex/art/WebData. `S47_ONLY=masters` er bare grafreparasjon; den hopper over MI-løkken og aktiverer derfor ikke nye profiler i eldre instanser. Kontroller importens antall/profiloppsummering og S47WEB done. Et kjent avslutningskrasj etter lagring er ikke alene PASS; cook og runtime må fortsatt valideres.

Package-linux cooker kildens leverte .spc til binær .upipelinecache og avviser Failed to compile Material / Default Material will be used in game selv ved UAT exit0. IoStore beholdes; SkipZenStore unngår lokal Zen-opploggheng. Første materialcook med 179 fallback-MI-er er forkastet; StaticSwitchParameter bruker nå korrekte True/False-pinner med feilsjekk. Nye materialer, Props eller shaderbaner krever ny cachevurdering/opptak fra én cook med frosne nøkler. Ikke gjenbruk eldre .rec-filer ukritisk eller godta en foreldet cache i Saved/Cooked. Generert Web/Audio/Licensed/Free-assets committes ikke. Midlertidige Content/Audio, Content/Licensed og Content/Free-lenker er fjernet; bare lenkene ble fjernet, kildemappene er bevart (`static/content-link-cleanup.json`). Genererte OpenOrder-logger er gjenopprettet. Props/config er byteidentisk core; Python-/shellsyntaks og diffkontroll består. Kilde og bundlet SPC leveres i samme vanlige sluttcommit på port-gfx.

### Leveringsstatus

Rendering, cache, oppstart, 40 bildepar, indikatorfaser, lengre ytelsesprøver og kort regresjon er verifisert. Leveransen er klar for Claude-gjennomgang, med alle bestilte grafikkprøver og bildepar dokumentert. Kilden er ryddet og leveres sammen med den stabile cachen i port-gfx. De eldre feilede/avbrutte prøvene beholdes som historikk, uten å telle som PASS.

Dokumentasjonen nedenfor gjelder tidligere navngitte bygg.


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

Volumetriske skyer og skumringslys fra 24.10: se SKY.md.
