# Volumetriske skyer og skumringslys (sky-pass)

24. oktober 2026, opprinnelig gren `sky-pass` fra `unreal-port-core` dc2820c2. Codex har overtatt sluttføringen etter Claudes limit på Toms bestilling; sky-koden er allerede integrert med natur/film i core1f471862. Toms bestilling: volumetriske skyer, få om natta og fritt om dagen, lys, skyer og stemning så nær konseptbildet (`~/scratch-bm/prompts/ref/dusk-concept.png`) som mulig. Codex eier natur (vegetasjon, terreng, mesaer) på `codex/caliche-nature`; dette passet rører bare himmel, skyer, sol/måne/skylight, tåke og eksponering.

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

## Bilder

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

## Prøver

- `-S47SelfTest` i pakket build: PASS, 1308 sjekker, inkludert tre nye skykontroller (dagdekning 0,400 til 0,698, natt høyst 0,15, største minuttsteg 0,0055, kveldsklokka går rundt). Første kjøring fant et reelt hopp i nattvariasjonen ved midnatt; rettet.
- Kort regresjon på samme pakke: room, event, outside, radio, nightsteps, caliche, beacon og Low ved 18:30 PASS.
- Editor `-game` har skylaget av med vilje (`-S47EditorClouds` slår det på): on-demand shaderkart i editoren mangler skypassene, og UE faller da tilbake til overflatemateriale og stopper på en assert i skyrendereren. Kokte builds er komplette.
- PSO-cache: se avsnittet nedenfor.
- UNVERIFIED: menneskelig gjennomspilling i bevegelse, skyenes drift over lang tid, alle 110 screenshots.sh.

Tuning for gjennomgang uten ny bygg: `-S47NoClouds`, `-S47CloudCover=`, `-S47CloudExt=`, `-S47CloudAmb=`, `-S47CloudGlow=`, `-S47CloudGlowEl=`, `-S47CloudSamples=`, `-S47CloudShadowKm=`, `-S47SkyBoost=`.

## PSO-cache

Nye shadere (skymaterialet, cloud fogging på stjerner og horisont, skyskygger) betyr at den bundlede graphics-PSO-cachen må tas opp på nytt fra denne cooken (`Scripts/pso-cache.py record`, deretter `expand` og ny pakking, se LIGHTING.md). Opptaket tas i korte bolker (én kvalitet og én klokke av gangen) med pauser, så naturpasset får maskinen innimellom. Det gamle opptaket under `~/scratch-bm/Signal47-sky/pso` er ufullstendig og bruker eldre pakke/shader keys. Det er bevart som historikk. Nytt opptak under `~/scratch-bm/Signal47-cloud-completion/pso` bruker den hashkontrollerte, integrerte CN5-pakken og frosne keys. Status: pågår; 4 s oppvarming og 4 s per utsnitt er et cacheopptak, ikke ny helruteytelse.
