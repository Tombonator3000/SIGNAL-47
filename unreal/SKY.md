# Volumetriske skyer og skumringslys (sky-pass)

24. oktober 2026, opprinnelig gren `sky-pass` fra `unreal-port-core` dc2820c2. Codex har overtatt sluttføringen etter Claudes limit på Toms bestilling; sky-koden er allerede integrert med natur/film i core1f471862. Toms bestilling: volumetriske skyer, få om natta og fritt om dagen, lys, skyer og stemning så nær konseptbildet (`~/scratch-bm/prompts/ref/dusk-concept.png`) som mulig. Codex eier natur (vegetasjon, terreng, mesaer) på `codex/caliche-nature`; dette passet rører bare himmel, skyer, sol/måne/skylight, tåke og eksponering.


## Integrert sluttkontroll, Codex 24. oktober 2026

Claudes sky-/lysarbeid f55b760c og 04d98231 var allerede integrert med natur/film i CN5. Codex har fullført ny cache, pakking, sluttbilder og regresjon på denne samlede kilden etter Claudes limit. Naturmulighetene er lagret som åpne oppgaver i todo.md. Dette ferdigstiller skyoppgaven, men gir ikke fotoets detaljnivå i fjell og vegetasjon.

Bevisrot: `~/scratch-bm/Signal47-cloud-completion`. `final-package.json` binder executable og alle fem innholdscontainere. 721 Source/Config/Content-filer er kontrollert mot kildebindingen. Den nye cache-cooken har byteidentiske 21 107 kokte prosjektassets og shaderfiler mot opptaksgrunnlaget.

- PASS: 84 korte opptaksbolker dekker 54 kameramotiv ganger Low/High/Ultra og natt/daggry. 4 s oppvarming og 4 s hold er cacheopptak, ikke stabil ytelsesmåling.
- PASS: 4329 stabile PSO-er i SPC på 98 886 byte. Cook konsumerer 623 oppskrifter, avviser 0 og slår sammen 3706; tre ugyldige compute-varianter fjernes av cook. Ferdig binær cache har 620 PSO-er, inklusive 285 graphics. Runtime åpner samme GUID 01A16DA4E3017B5A8D9B7F441D9C718C og behandler alle 285 graphics. Cachebevisets ni kontroller består.
- PASS: 1328 selvtestsjekker og seks vanlige funksjonsprøver, inklusive levende stjernehull, komet før/etter og pause/gjenopptakelse. De opprinnelige kometbildene hadde en antenne foran utsnittet; fire separate før/etter-bilder fra fri veiskulder bekrefter komet og stjernehull visuelt. De gamle bildene beholdes som diagnostikk.
- PASS: 14 ukonkurrerte kameraturer og 48 gjennomgåtte originalbilder ved dag, 17:20, 18:30, 21:00, 03:00, daggry og midnatt, samt Low, Ultra, standard Carpenter og NoClouds. Fire ekstra himmelhendelsesbilder gir 52 faktisk vurderte bilder. High/Clean viser tydelig ordnet rutedithering i enkelte månelyse skyer og fin støy i skykanter; dette er åpen kvalitetsgjeld, ikke fotorealistisk godkjenning. Full visuell avgrensning står i visual-acceptance.json.
- PASS: hele screenshots.sh med 110 unike logger, 110 OK og 110 naturkontroller. Det første av de to tidligere like starhole-filnavnene heter nå starhole-noticed; scenariet og flaggene er uendret.
- PASS: lokalt flettet i unreal-port-core og installert i ~/Games/Signal47/Linux. Alle 40 payloadfiler er hashkontrollert, 48 Saved-filer er byteidentiske før/etter, og forrige payload er sikkerhetskopiert. Originalbilder, to 1:1-kontrollutsnitt, kilder og rapporter ligger i ~/Desktop/Signal47 skyer og lys.

Den første strenge shader-key-sammenligningen avviste endrede navn etter cook. Avvisningen er bevart. En separat kontroll dokumenterer at FName er case-insensitivt: ingen tidligere normalisert objekt-/shaderidentitet, PipelineHash eller OutputHash er borte eller endret. 112 nye aliaser for åtte eksisterende MI_W-materialer matcher gamle M_Web_Lit_Opaque-signaturer nøyaktig. Begge sett har 3405 unike kompilerte shaderutganger, Global er byteidentisk og alle 21 107 prosjektassetfiler er byteidentiske. stable-key-compatibility.json har den snevre kompatibilitetskontrollen; de gamle avviste rårapportene er ikke overskrevet.

Én siste skumringsmåling ble avbrutt da brukeren startet installert spill. Guard avsluttet bare testens egen prosess. Den avbrutte prøven ligger i karantene; samme utsnitt er tatt om uten konkurranse før sluttmålingene. Ingen andres prosess ble stoppet.

### Målinger på den integrerte pakken

Dette er et måleutdrag fra den nåværende fullverdenspakken i `Signal47-cloud-completion/final-build`, ikke historiske sky-only-tester. Målt på RTX 3080 / i7-12700KF, Vulkan, NVIDIA 595.99, 2560 × 1440, VSync og FPS-grense av. Alle 14 turer og begge oppstarter har gyldige, ukonkurrerte capture-metadata med samme executable og alle fem innholdscontainere. Dette er gyldige måledata, ikke et løfte om jevn bildefrekvens.

Executable SHA256: `92660b26686afe841f2fbd1c168331b21bad0a5c8497a428282f09416970dca9`. PAK SHA256: `31d2dd669e8fccf9f146531f75568de20e3673559fedaf51245a5014bb2850bd`. Full identitet og kildenes SHA256 står i `current-performance-summary.json`.

Turene bruker faste kameraer med pauset verden, 8 s innledende oppvarming og 7 s hold per vinkel. Det faktiske målevinduet er cirka 3,2 s per vinkel. Snitt er vektet over målte bilder; p95-kolonnen nedenfor er høyeste individuelle vinkel-p95, ikke en sammenslått p95 for hele turen. GPU-kolonnen er målt GPU-snitt; snitt/p95 for bildetid er faktisk veggklokketid mellom bilder og inkluderer hakking.

| Tur | Klokke / kvalitet / bilde | Vinkler | Prøvetid totalt (s) | Bildetid snitt (ms) | Høyeste vinkel-p95 (ms) | GPU-snitt (ms) |
|---|---|---:|---:|---:|---:|---:|
| dawn | 05:40 / high / off | 4 | 12.81 | 11.47 | 20.06 (sky-saro) | 10.79 |
| day | 12:00 / high / off | 4 | 12.80 | 11.44 | 20.31 (sky-saro) | 10.76 |
| dusk | 18:30 / high / off | 4 | 12.80 | 11.58 | 20.17 (sky-saro) | 10.90 |
| dusk-default-carpenter | 18:30 / high / carpenter | 2 | 6.40 | 12.75 | 16.94 (sky-commute) | 12.04 |
| dusk-no-clouds | 18:30 / high / NoClouds / off | 4 | 12.79 | 10.73 | 19.48 (sky-saro) | 10.08 |
| early-night | 21:00 / high / off | 4 | 12.80 | 10.68 | 23.70 (sky-saro) | 10.02 |
| heaviest-day | 12:00 / high / off | 4 | 12.80 | 12.80 | 15.95 (highway) | 12.08 |
| heaviest-dusk | 18:30 / high / off | 4 | 12.80 | 13.36 | 19.43 (highway) | 12.37 |
| late-afternoon | 17:20 / high / off | 4 | 12.83 | 11.85 | 21.97 (sky-saro) | 11.16 |
| low-fallback-dusk | 18:30 / low / off | 4 | 12.80 | 8.78 | 11.90 (sky-commute) | 8.20 |
| midnight-after | 00:00 / high / off | 2 | 6.40 | 10.54 | 22.01 (sky-saro) | 9.91 |
| midnight-before | 23:59 / high / off | 2 | 6.39 | 10.38 | 18.96 (sky-saro) | 9.74 |
| night | 03:00 / high / off | 4 | 12.79 | 10.52 | 19.11 (sky-saro) | 9.86 |
| ultra-dusk | 18:30 / ultra / off | 2 | 6.38 | 22.38 | 25.85 (sky-commute) | 21.69 |

Snitt under 16,67 ms er ikke alene bevis på stabilt 60 FPS. Alle konkrete overskridelser av p95-budsjettet er bevart per vinkel i JSON; SARO-vinklene og highway/street-north-route i den tyngre skumringsturen har målte overskridelser. Ultra-resultatet gjelder den separate Ultra-innstillingen, ikke High.

High mot NoClouds ved 18:30, samme fire vinkler og innstillinger:

| Vinkel | High snitt / p95 (ms) | NoClouds snitt / p95 (ms) | Forskjell snitt High − NoClouds (ms) | Forskjell GPU-snitt (ms) |
|---|---:|---:|---:|---:|
| sky-commute | 14.22 / 15.19 | 13.37 / 14.63 | 0.85 | 0.83 |
| sky-west | 10.12 / 10.87 | 9.33 / 10.01 | 0.79 | 0.76 |
| sky-up | 10.60 / 11.27 | 9.71 / 10.55 | 0.89 | 0.87 |
| sky-saro | 12.21 / 20.17 | 11.39 / 19.48 | 0.82 | 0.78 |

Den observerte forskjellen inkluderer skyene og tilhørende skylight/fallback, og isolerer derfor ikke skyshaderens kostnad. Dette er én sekvensiell måling per innstilling, med cirka 3,2 s per vinkel; forskjellene er ikke et statistisk estimat fra gjentatte forsøk.

Oppstart er målt separat med High og standard Carpenter-bilde i chapter1-default, ikke som en fast dag-/skumringskameratur. READY er tid fra verdensoppstart. Første synlige bilde kommer etter at oppstartsdekningen slipper. Prosessens totale kjøretid inkluderer venting, skjermbilde og avslutning og er ikke presentert som lastetid.

| Oppstart | READY (s) | Første synlige (s fra verdensstart) | Første 10 s snitt / p95 / maks (ms) | Første synlige 10 s snitt / p95 / maks (ms) | Synlig FPS, bilder/10 s | Synlige intervaller >50 ms |
|---|---:|---:|---:|---:|---:|---:|
| startup-cold | 8.878 | 8.900 | 31.77 / 29.42 / 3664.34 | 15.56 / 27.90 / 37.82 | 64.2 | 0 |
| startup-warm | 6.362 | 6.734 | 29.33 / 31.75 / 2476.34 | 17.27 / 25.16 / 372.56 | 57.9 | 6 |

Begge oppstartsvinduer varer 10 s; hele råintervaller som krysser vindusgrensen inngår. De første 10 s omfatter oppstartsdekning og lange lasteintervaller. Synlig FPS er antall bilder/10 s, ikke 1000 delt på oppgitt snitt. Cold har ny isolert driver-cache og `-clearPSODriverCache`; warm gjenbruker samme driver-cache. OS-diskcache er ikke tømt. Dette er ett forsøk av hver type.

Warm når READY tidligere, men har i dette ene forsøket en synlig hitch på 372,56 ms. Første synlige 10 s oppnår derfor ikke et stabilt 60 FPS-resultat. Ved READY viser loggene null ventende PSO/RHI-/material-/shaderjobber; det beviser ikke fravær av senere strømme- eller kompileringshakking. Rå trace og køtopper er bevart i JSON.


Menneskelig gjennomspilling, lytting, langvarig skybevegelse og editorens opt-in-skyer er fortsatt UNVERIFIED. Kun pakket Vulkan på denne maskinen er godkjent. Låst 60 FPS er ikke dokumentert; varm oppstart har ett synlig hakk på 372,56 ms. Videre utbedring av oppstartshakking og naturens fotolikhet er åpne TODO-punkter.


## Hva som er gjort

**Ett VolumetricCloud-lag uten SkyAtmosphere.** Den levende himmelen (S47Sky, kuppelen 30 km ute med stjerner, komet, stjernehull og lyn) er beholdt som himmel. Skylaget komponeres foran kuppelen. SkyAtmosphere ble vurdert og valgt bort: den ville tatt over horisontfarger og solens transmittans i alle områder og krevd ny kalibrering av den faste eksponeringen. Uten den bruker skyene egen planet (6360 km); laget løftes med `d²/2R` over øyet fordi områdene ligger opptil 80 km fra origo (`S47Clouds.cpp`).

- Materiale `/Game/Sky/M_S47_Clouds`, bygget av `Tools/sky/import_clouds.py` fra `Tools/sky/clouds.hlsl`. Prosedyrisk, ingen teksturer: brede banker (cirka 10 km) med brutte celler (cirka 1,6 km), flate mørke baser og lave avrundede topper, alto/stratocumulus 2,9 til 4,5 km. Laget tynnes ut fra 10 til 26 km, slik at horisontbåndet viser seg under det som i konseptet. Konservativ tetthet hopper over tom luft. Avansert volumutgang: faser g 0,62 / −0,28, multippel spredning 0,7.
- Cirrus males på kuppelen (`Tools/calichestreet/atmosphere.hlsl`, parametrene `Cirrus`, `CirrusDrift`), i alle områder når skylaget er på. Den beholder solnedgangsfargene lengst og er bare et svakt slør om natta.
- `Tools/sky/import.sh` bygger skymaterialet og de tre Caliche-himmelmaterialene på nytt (én tung jobb, guard først). Stjernene og horisontbåndet har nå UE cloud fogging, så ekte skyer skjuler stjernene bak seg. De malte skyene i kuppelen (Caliche-konseptet) blir bare liggende svakt helt nede ved horisonten, der kuppelen skjuler de volumetriske.

**Dekning fra spillets klokke** (`FS47Sky::CloudCoverAt/CirrusAt`, kun rendering, ingen historieporter endret, timelineVersion 2):

| Klokke | Dekning | Merknad |
|---|---|---|
| 07:00 til 18:50 | 0,40 til 0,70 | brutt lag, langsom deterministisk variasjon; ekstra tett rundt 17:40 til 19:00 (konseptet) |
| 18:50 til 21:00 | faller | laget brytes opp |
| 21:00 til 04:30 | 0,07 til 0,15 | få spredte skyer som av og til går over månen og stjernene, aldri overskyet |
| 04:30 til 06:30 | bygger seg opp igjen | |

Skyene driver med vind fra vest-sørvest (cirka 9 m/s) og endrer form sakte.

**Historiens himmel holdes lesbar.** Skymaterialet har to «hold åpent»-retninger med myke, fillete kanter: stjernehullet der antennene peker fra 02:15 (az 026, el 32, styrke = `HoleK`), og Halleys komet når den er oppe. Om dagen brukes kometplassen til en lav sol, slik at ettermiddagssola på innkjøringen og soloppgangen når veien der spilleren står, mens skyskygger fortsatt glir over ørkenen ellers. Lyn på horisonten (`Flash`) lyser opp skyene innenfra mot lynretningen. Meteorer kommer bare om natta, når dekningen er lav. Ingen historiehendelse krever i dag helt klar himmel utover dette; ny hendelse som trenger det kan bruke de samme retningene.

**Lyset.**
- `DawnSun` er atmosfærelys 0: lyser skyene om dagen og kaster skyskygger på bakken (High/Ultra).
- Nytt `CloudLight` er atmosfærelys 1 og lyser bare skyene (ingen lyskanal, skygge, bounce eller tåke): etter solnedgang varm oransje til dyp rød fra solens asimut til cirka −8°, om natta månens kalde kant. Retningen holdes kunstnerisk 1,5° over horisonten. Et lys under skyplanetens horisont skygges av planeten per prøve og ble til glitrestøy; dette er den bøyde regelen (fysisk står sola under horisonten).
- Skyenes «himmellys» er en kunstnerisk emissiv: kjølig blå om dagen, støvete fiolett grå i skumringen, nesten ingenting om natta.
- Solens bakkelys får en kort hale etter solnedgang (til cirka −2,8°, retning holdt 0,6° over horisonten), slik at vestvendte klipper fanger det siste lyset som i konseptet. `FS47Sky::Sun` og historieverdiene er uendret.
- Kuppelens Caliche-palett i skumring: bredt oransje til fersken bånd ved horisonten, sterkest under sola men varmt hele veien rundt, gjennom lilla til støvete fiolettblått oppover.
- Skylight fanges nå i sanntid (High/Ultra), så omgivelseslyset følger skyene. Fordi mørke skybaser erstatter lys himmel i fangsten, kompenseres intensiteten med dekningen (`-S47SkyBoost`, standard 2,4) om dagen og i skumringen; natta har få skyer og er like mørk som før.
- Tåke: i ettermiddag og skumring blir fjerne mesaer støvete blåfiolette, med varm retningsspredning mot den lave eller nettopp nedgåtte sola.
- Low: skylaget er av, cirrusen og de malte Caliche-skyene står igjen, skylight fanges ved endring som før.

Natta er uendret mørk med sterk kontrast (skylight 0,055, måne 0,055 lux, indirekte 0,52). Gatelampene tennes som før.

## Historiske bilder fra sky-pass

Pakkede bilder, High/VHS, 1600 × 900, kveldstidslinjen. `sky-commute` er innkjøringens start sør for Caliche, mot nord langs veien som i konseptet; `sky-west` mot solnedgangen, `sky-up` oppover, `sky-saro` ved SARO.

| Bilde | Sti |
|---|---|
| Konseptet ved siden av 18:30 | `~/scratch-bm/Signal47-sky/final/dusk-vs-concept.png` |
| Innkjøringen 17:20, 18:30, 21:00, 03:00, 05:40, 12:00 | `~/scratch-bm/Signal47-sky/final/commute-times.png` |
| Opp mot himmelen 21:00, 03:00, 05:40 | `~/scratch-bm/Signal47-sky/final/up-night-dawn.png` |
| Enkeltbilder | `~/scratch-bm/Signal47-sky/final/<HHMM>/<view>.png` |
| 1440p dusk/dag | `~/scratch-bm/Signal47-sky/perf/*/` |

## Historiske målinger før ferdig natur/film, RTX 3080, 2560 × 1440

Snitt/p95 i ms over cirka 3,5 s per kamera (statisk kamera, `graphics-review`-måling), uten konkurrerende tung jobb.

| Kamera | Dusk High | Dusk High uten skyer | Dag High | Dusk Low | Dusk Ultra |
|---|---:|---:|---:|---:|---:|
| sky-commute | 12,71 / 14,07 | 11,98 / 13,41 | 12,77 / 14,14 | 10,19 / 11,49 | 22,69 / 23,86 |
| sky-up | 11,27 / 12,50 | 10,50 / 11,98 | 11,30 / 12,58 | | |
| street-north-route | 12,57 / 13,74 | 11,83 / 13,34 | 12,93 / 14,69 | 10,17 / 11,33 | 23,73 / 25,95 |
| array-field | 11,43 / 14,44 | 10,81 / 15,06 | 11,77 / 16,49 | | |
| yard | 12,83 / 19,78 | 11,91 / 16,66 | 12,88 / 17,11 | 9,70 / 11,02 | 24,89 / 38,61 |
| highway | 10,96 / 12,44 | 10,12 / 11,54 | 10,85 / 12,05 | | |

Historiske High-snitt i disse korte utsnittene er høyst 12,93 ms, cirka 77 fps. Gårdens p95 er 19,78 ms; dette dokumenterer ikke låst 60 fps. På denne eldre pakken er skyenes målte snittforskjell cirka 0,7 til 0,9 ms. Ultra ligger på cirka 40 til 44 fps i utsnittene. Målingene gjelder før ferdig natur-/film-integrasjon og er ikke steady state eller helrutebevis.

## Historiske prøver fra sky-pass

- `-S47SelfTest` i pakket build: PASS, 1308 sjekker, inkludert tre nye skykontroller (dagdekning 0,400 til 0,698, natt høyst 0,15, største minuttsteg 0,0055, kveldsklokka går rundt). Første kjøring fant et reelt hopp i nattvariasjonen ved midnatt; rettet.
- Kort regresjon på samme pakke: room, event, outside, radio, nightsteps, caliche, beacon og Low ved 18:30 PASS.
- Editor `-game` har skylaget av med vilje (`-S47EditorClouds` slår det på): on-demand shaderkart i editoren mangler skypassene, og UE faller da tilbake til overflatemateriale og stopper på en assert i skyrendereren. Kokte builds er komplette.
- PSO-cache: se avsnittet nedenfor.
- På dette historiske tidspunktet var full regresjon ikke utført. Gjeldende sluttstatus står øverst; menneskelig gjennomspilling og langvarig drift er fortsatt UNVERIFIED.

Tuning for gjennomgang uten ny bygg: `-S47NoClouds`, `-S47CloudCover=`, `-S47CloudExt=`, `-S47CloudAmb=`, `-S47CloudGlow=`, `-S47CloudGlowEl=`, `-S47CloudSamples=`, `-S47CloudShadowKm=`, `-S47SkyBoost=`.

## Historisk PSO-status før Codex overtakelse

Nye shadere (skymaterialet, cloud fogging på stjerner og horisont, skyskygger) betyr at den bundlede graphics-PSO-cachen må tas opp på nytt fra denne cooken (`Scripts/pso-cache.py record`, deretter `expand` og ny pakking, se LIGHTING.md). Opptaket tas i korte bolker (én kvalitet og én klokke av gangen) med pauser, så naturpasset får maskinen innimellom. Det gamle opptaket under `~/scratch-bm/Signal47-sky/pso` er ufullstendig og bruker eldre pakke/shader keys. Det er bevart som historikk. Nytt opptak under `~/scratch-bm/Signal47-cloud-completion/pso` bruker den hashkontrollerte, integrerte CN5-pakken og frosne keys. Dette opptaket er nå komplett; gjeldende sluttbevis står øverst. 4 s oppvarming og 4 s per utsnitt er cacheopptak, ikke ny helruteytelse.
