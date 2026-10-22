# Native lys og skygger i SIGNAL / 47

Arbeidsdato: 22. oktober 2026. Gren: `port-light`. Bare Unreal er endret. Geometri, plasseringer, spilltekst og historie er bevart. Dette er en kildekandidat, ikke en oppdatering av den installerte spillutgaven.

## Lysbanen

`S47Lighting.cpp` bygger ekte lamper fra eksportens eksisterende pools. Høye utelamper bruker brede, nedovervendte spotter; innelamper og andre praktiske kilder bruker punktlys. Enhetene er candela, med lyskildestørrelse i centimeter og invers kvadratisk demping. Eksportens rekkevidde bestemmer attenueringen. `SetFlood`, `SetFloodPool`, `SetFloodColor` og `SetFloodScale` beholder historienes tilstand og styrer nå native lys. Materialkopier av den samme lampen og lyktene deles uten å legge til ekstra lys.

Kontrollrommets eksisterende lys er oppgradert med skygger. `FS47Light` beholder historienes opprinnelige intensitetsverdier utad, mens den fysiske lysstyrken skaleres i Unreal. SAROs strømbrudd, rør, skjermglød og dimming bruker de samme setterene. Lommelykta bruker 420 cd, to billykter bruker 4200 cd hver; alle har skygger og volumetrisk spredning. HDR-fargen i kapittel 6 normaliseres før den sendes til lyset, og styrken flyttes til intensiteten så den ikke klippes. Førerhusets gamle pool for «sky through glass» er fjernet; native himmel og utelamper lyser gjennom rutene. Bare den svake instrumentgløden får et eget lys.

Gamle Ultra-proxylys, additive kjegler, bakkehaze og vindusprojeksjoner er pensjonert. Vinduer har i stedet native rectlys ved de eksporterte rutene; disse følger nivå, farge og slukking. Himmel, stjerner, ikke-skyggekastende flora og kameravendte støvpartikler er ekskludert fra strålesporingen. De er fortsatt synlige i bildet.

Månen er svak og kald. Skylight fanger den levende himmelen ved områdebytte og vesentlige daggryendringer. Et eget retningslys følger spillets eksisterende solretning og daggry. `ExponentialHeightFog` med volumetrisk tåke erstatter materialtåka. Skjermer, TV-er og skilt beholder sine emissive materialer og deltar i Lumen; små kilder støttes av de eksisterende lampetilstandene.

Eksponeringen er fast. `M_WebTone` beholder ACES-kurven og VHS/Clean, men legger ikke på FogExp2. Ingen lysbryter mellom Web og Unreal finnes i innstillinger eller kommandolinje. Kontrollrommets CRT-render targets bruker `FWidgetRenderer(false)`, slik at grønnfargen ikke sRGB-kodes to ganger.

## Kvalitet

| Innstilling | Lumen | Skygger | Volumetrisk tåke | Intern oppløsning |
| --- | --- | --- | --- | --- |
| Low | Av | Enklere, relevante lokale lys beholder skygger | Av | 60 %, TSR til valgt utgang |
| High, standard | På, scene/final gather 0,5 | VSM, local bias 1, fire SMRT-stråler med to prøver | 16 pikslers rutenett, 64 dybdelag | 60 %, TSR |
| Ultra | På, scene/final gather 1 | VSM, local bias 0 | 8 pikslers rutenett, 96 dybdelag | 100 % |

I 2560 x 1440 tegner High omtrent 1536 x 864 internt. Dette må oppgis sammen med fps-tallene. High bruker 100 % TSR-historikk; Ultra bruker motorens dyrere Epic-historikk. Low/High bruker VSMs fire mip-nivåer for bedre ytelse med eksportens ikke-Nanite-mesher. Grove skyggekart og volumetrisk skyggelegging beholdes. VHS/Clean velges uavhengig av Quality. Lysstyrkeinnstillingen er bevart; eksponeringen er fortsatt fast.

## Generert innhold

Materialsteget må kjøres før denne kandidaten brukes med gammelt generert innhold. Full eksport eller geometriimport er ikke nødvendig. Ny `NativeLighting` i `MPC_Web` har default 0 for kompatibilitet med hovedkopiens eldre runtime; kandidatens runtime setter alltid 1. Alle materialbidrag for flood, hemisphere og gamle frontlykter blir da null.

Under arbeidet brukes lokale kopier av `Content/Web/Base` og `Mat`, kontrollert å være separate filer fra hovedkopien. Materialsteget `S47_ONLY=materials` skriver bare Base og Mat. Det skriver ikke manifest, art, Geo eller Tex. Audio, Licensed og WebData lånes med midlertidige symlinker, som fjernes før commit.

```sh
cd unreal/Signal47
S47_ONLY=materials S47_WEB="$HOME/scratch-bm/Signal47-web" \
  "$HOME/.local/opt/UnrealEngine-5.8.4/Engine/Binaries/Linux/UnrealEditor-Cmd" \
  "$PWD/Signal47.uproject" -run=pythonscript \
  -script="$PWD/Tools/webexport/import_web.py" \
  -ddc=InstalledNoZenLocalFallback -unattended -nullrhi
```

## Bevis og kontroll

Bevismappe: `/home/tom/scratch-bm/Signal47-lighting-2026-10-22/`. Den inneholder originale PNG-er, kjørelogger, testkommandoer og sammenstillinger. Bildene er ekte Vulkan-runtime, ikke konseptbilder eller render fra Blender. Førbildene kommer fra `~/SIGNAL-47`, etterbildene fra dette worktreet. Begge sider bruker 1600 x 900 og 100 % intern oppløsning, samme fase og kameraflagg, normalt åtte sekunder. For nattveien holdes bilen stille med `-S47Throttle=0` for et sammenlignbart kamera.

| Utsnitt | Side ved side, under bevismappa | Hva som kontrolleres |
| --- | --- | --- |
| Kontrollrom | `compare/room.png` | Grønne CRT-er, bordlys, skygger fra inventar |
| Gården | `compare/outside.png` | Kalde antenner, varme lamper, rekkverk og skilt |
| Annex/arkiv | `compare/annex.png` | Skap, vegghjørne, kaldt innelys |
| Lommelykt | `compare/flashlight.png` | Avgrenset kjegle, lys på rekkverk og gangbane |
| SARO mørkt | `compare/dark.png` | Reléet slukker rommets lys og CRT-er |
| STATION 01 | `compare/station.png` | Porten, utebelysning og lys ved hytta |
| Motelgård | `compare/court.png` | Søyle-, vindus- og møbelskygger under svalgangen |
| Rom 6 | `compare/room6.png` | Kald TV-glød og varm sengelampe |
| Diner | `compare/diner.png` | Disk, kaffekanne og lesbart innelys |
| Nattvei | `compare/road.png` | Ekte frontlykter og mørkt førerhus |
| Gammel vei ved daggry | `compare/dawn.png` | Lys fra daggryhimmelen og frontlyktene |

Hovedkopien er nyere enn grenens utgangspunkt: førbildene inneholder enkelte senere UI-/TV-rettelser. TV-programmet i førbildet fra rom 6 er film, mens denne grenen viser sin eksisterende snø. Ingen TV-historie eller programinnhold er endret her. Den forskjellen er ikke et lysbevis. Sammenlign møblene, veggene og skyggene.

Områdebildene er tatt gjennom kalibreringspasset. Kontrollrom, gård, lommelykt og begge veiprøvene er tatt på nytt etter siste skygge-/toneoptimalisering. Sammenstillingene bruker alltid 100 % intern oppløsning ved 1600 x 900; High-profilens 60 % gjelder de separate 1440p-målingene.

Det første native-passet ble for lyst inne. Punktlysene i kontrollrommet ble kalibrert ned fra faktor 12 til 1,5; innendørs pools fikk lavere styrke og kortere rekkevidde. Det ga mørke hjørner og lesbare flater uten å vaske ut CRT-ene. VSMs grense for lokale lys per piksel ble økt til 32 etter en faktisk overflow-advarsel. Bildene i sammenstillingene bruker den korrigerte kandidaten.

GPU-profilen viste at VSM-projeksjon var den største enkeltkostnaden. High fikk derfor færre SMRT-prøver og normal TSR-historikk. Tonepassets avslåtte materialtåke hopper også over dybde-/stencillesningene i hver VHS-prøve; ACES og selve VHS-effekten er beholdt.

Fra førersetet gir det første VSM-passet fortsatt `Non-Nanite Marking Job Queue overflow`. Motorens shader behandler de resterende sidene sekvensielt ved denne grensen, og det er ikke observert manglende skygger. Dette er en gjenværende kostnad ved oppvarming av skyggekartene, ikke en reparert advarsel. Nattveiens etterbilde tas etter 20 sekunder, med bilen stille og skyggekartene varme. Advarselen står i `after/road.log`.

### Teststatus

- PASS: editorbygget med `Scripts/build-editor.sh -NoUBA`, advarsler som feil. Siste logg: `build-final.log` i bevismappa.
- PASS: `-S47SelfTest`, 1142 kontroller i denne grenen. Hovedkopiens høyere antall kommer fra senere arbeid og er ikke grenens testantall.
- PASS: elleve målrettede native bilder med `smoke test OK`, uten kritisk feil, inkludert strømbrudd, lommelykt og alle etterspurte hovedområder.
- PASS: Vulkan logger at ray tracing og ray tracing-shadere er aktivert. Lumen hardware ray tracing er valgt. Software-fallback er konfigurert gjennom motorens vanlige støttevalg, men er ikke separat målt.
- PASS: isolert materialgenerering lagret mastere og instanser, deretter validert i native runtime. Importprosessen har fortsatt det kjente avslutningskrasjet etter `S47WEB done`; prosessens exitstatus er derfor ikke PASS.
- PASS: 60 fps-målet i de to avtalte High-utsnittene, både gjennomsnitt og p95 under 16,7 ms. Dette er 1440p-utgang med TSR fra 60 % intern oppløsning, ikke nativ 1440p.

### RTX 3080, Vulkan, 2560 x 1440

Siste prøver, 20 sekunder hver, fem sekunders oppvarming før sampling. `stat unit` er synlig i `perf-final/*.png`; de akkumulerte tallene kommer fra `S47LightPerf` i de tilsvarende loggene. Skjermfangsten er med i siste rammetidssnitt.

| Utsnitt og kvalitet | Ramme, ms | p95, ms | Game, ms | Draw, ms | GPU, ms | FPS i snitt | Rammer |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Kontrollrom, High/VHS | 14,701 | 15,704 | 3,457 | 6,479 | 13,682 | 68,0 | 891 |
| Gården, High/VHS | 11,029 | 12,383 | 3,206 | 5,786 | 10,183 | 90,7 | 1275 |
| Kontrollrom, Low/Clean | 12,310 | 13,130 | 3,039 | 5,006 | 11,458 | 81,2 | 1144 |
| Gården, Ultra/VHS | 26,856 | 29,609 | 3,145 | 6,177 | 25,136 | 37,2 | 503 |

Ultra prioriterer full intern oppløsning og dyrere skygger; det når ikke 60 fps på denne maskinen. High er 60 fps-profilen. Trådtidene går parallelt og skal ikke summeres.

Kontrollrom og Low registrerte hver én annen PID i ett halvt sekund ved prosessens t=6,05 s, før native-verdenen og målevinduet. For kontrollrommet var dette rundt 17:24:27 UTC; sampling begynte 17:24:42. Ingen andre tunge prosesser ble registrert under sampling i noen av de fire sluttprøvene. Råregistreringene ligger i `perf-final/*-contention.json`. Dette skiller de korte oppstartsregistreringene fra de forkastede prøvene med faktisk samtidig spilling.

### Målemetode og begrensninger

Ytelse måles med `stat unit`, `-S47LightPerf`, 2560 x 1440, VSync og bildegrense av. Loggen oppgir gjennomsnittlig rammetid, p95, game, draw og GPU etter oppvarming. `S47GpuProfile` kan i tillegg ta én GPU-profil etter sju sekunder. Dette er korte editorprøver av faste utsnitt, ikke en måling av hele reisen eller en pakket shipping-build.

Første målesett er forkastet. En samtidig installert spillprosess het `Signal47` og ble ikke fanget av kontrollen som bare så etter UnrealEditor/Signal47-Linux. Neste forsøk traff andre agenters nye tester mellom kontroll og oppstart. Kontroll og målelogg omfatter nå også `Signal47`, og krever åtte sammenhengende ledige sekunder før oppstart. Andre tunge prosesser under selve målevinduet gjør prøven uegnet som bevis for 60 fps; oppstartsregistreringer vurderes separat og oppgis ovenfor.

Pakket bygg, faktisk input gjennom hele natta, tidsstøy/ghosting under langvarig bevegelse og subjektiv vurdering på Toms skjerm gjenstår. Det fulle screenshots.sh er bevisst ikke kjørt, i tråd med bestillingen. Den installerte utgaven og standardstarteren er ikke endret.

Kompilatorens response-filer og den ferdige binærfilens native-banner er kontrollert mot dette worktreet.

Første førbildeforsøk feilet fordi disken var full og Zen-cachet var utilgjengelig. Omkjøringen bruker `-ddc=InstalledNoZenLocalFallback`. Det kopierte byggcachet beholdt først absolutte stier til hovedkopien; disse forsøkene bygget hovedkopiens eksisterende kode og gir ingen testdekning for kandidaten. Worktreets kopierte Intermediate ble deretter fjernet og bygget fra bunnen. UBA feilet på PCH-rename gjennom symlink; `Scripts/build-editor.sh` tar derfor nå valgfrie flagg, og `-NoUBA` løste byggingen. Første etterbilder fra feil binærfil er forkastet. Ingen andre agenters data er slettet. Tunge jobber sjekkes før oppstart; andre agenter kan starte arbeid mens en prøve allerede går.

## Kilder

Prosjektbibliotekets søk fant [Scenario Labs, Unreal lighting/rendering](https://github.com/scenario-labs/skills/blob/f0adb5b5ebc1fc8f7e333554d2aeb767c5558d6b/skills/game-engines/unreal/scenario-unreal-lighting-rendering/SKILL.md), UE 5.8 og MIT. Referansen er brukt for enheter, eksponering og støtte ved små emittere. Dens helpers er merket ikke kjørt i Unreal og er ikke gjenbrukt. Motorens lokale C++-API og [Epics Lumen-dokumentasjon](https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-technical-details-in-unreal-engine) er kontrollert. Runtime-prøvene her avgjør prosjektets teststatus.
