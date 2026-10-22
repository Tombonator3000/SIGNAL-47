# Ekte lys og skygger i SIGNAL / 47

22. oktober 2026, `port-light`. Rendering, lys og materialer er endret. Geometri, modellplassering og historie er bevart. Dette er en kildekandidat for Claudes integrasjon, ikke den installerte spillutgaven.

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
