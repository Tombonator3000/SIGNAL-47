> **Status: Referanse** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../../Active/STATUS.md).

# Lumen Lite, tåkespredning og avstandsstyrte skygger

Oppdatert status 27. oktober: punkt 5 er faktisk levert lokalt. [Atmosfærerapporten](../../Evidence/UnrealDeliveries/ATMOSPHERE_2026-10-27.md) dokumenterer småsteinskygger, gressvind, lydretting og FSSS som forblir av etter måling. Eldre kartlegging nedenfor beskriver tilstanden før de senere terreng-/MetaHuman-leveransene.

Senere oppfølging samme dag: Tom godkjente de to første visuelle stegene. Pure Sky er installert for dagtid med overgang til uendret eksisterende natt, se [SKY.md](../../Evidence/UnrealDeliveries/SKY.md). Tolv utvendige armaturer har fått originale Blender-modeller og rettet montering/lys; [NEAR_DETAILS_2026-10-26.md](../../Evidence/UnrealDeliveries/NEAR_DETAILS_2026-10-26.md) har gjeldende sluttkontroll og leveringsstatus. Vurderingen nedenfor beskriver grunnlaget før dette arbeidet. Terreng og natur er senere gjennomført i R2, se [TERRAIN_ROUTE_2026-10-26.md](../../Evidence/UnrealDeliveries/TERRAIN_ROUTE_2026-10-26.md). Lumen Lite og FSSS er fortsatt uprøvde forslag.

Vurdert 26. oktober 2026 for SIGNAL / 47, Unreal Engine 5.8.4 på Linux/Vulkan. Bestilt av Tom for senere bruk. Dette er kildeundersøkelse og anbefalinger, ikke implementerte grafikkendringer eller nye ytelsesmålinger.

Grunnlaget er dagens arbeidskopi på `codex/unified-night-scenario`, HEAD `41430ade` med lokale R5/NP1-endringer, installert motorkilde og eksisterende R5-logger. Branchens HEAD alene beskriver derfor ikke hele den undersøkte arbeidskopien. Ingen motor-, kilde-, asset-, kvalitets- eller installasjonsinnstilling er endret av denne vurderingen.

## Konklusjon og prioritet

| Tema | Hva vi bruker nå | Anbefaling til senere |
| --- | --- | --- |
| Lumen Lite / probe occlusion | High/Ultra bruker vanlig sanntids-Lumen. Low slår GI av. Ingen profil bruker aktiv Lite-GI. | Vurder et eget Medium/Lite-nivå. Behold High som sammenligningsgrunnlag. |
| Fog Screen Space Scattering, FSSS | Volumetrisk tåke på High/Ultra, men FSSS er ikke aktivert. | Prøv en svak effekt ved inngangen i skumring/natt etter at lampehus og plassering er rettet. |
| Skygger fra fjerne småobjekter | Runtime-småflora er allerede uten skygger. Et separat lag med mange busker og steiner har skygger på. | Første kandidat til besparelse: kortere skyggeavstand for disse småobjektene, med fortsatt synlige modeller. |

Foreslått rekkefølge er småobjektskygger, deretter et Medium/Lite-alternativ, og til slutt FSSS som et visuelt valg. Dette er en prioritering fra kildegjennomgangen, ikke en målt rangering av GPU-flaskehalser.

## 1. Lumen Lite og hva «2x raskere» betyr

«Lumen probe exclusion» tolkes her som **Lumen Lite med probe occlusion**. Dette er en sannsynlig identifikasjon ut fra funksjonsnavnet og hastighetspåstanden, ikke bekreftelse av en eventuell annen video eller plugin Tom har sett.

Epic introduserer Lite i UE 5.8 som Beta. Modusen bruker irradiance fields og interpolerte prober med okklusjon, og beskrives som omtrent dobbelt så rask som Lumen High Quality. Det er fortsatt dynamisk GI, ikke et bytte til ferdigbakt lys. Påstanden gjelder belysningsarbeidet i den sammenligningen, ikke hele spillets FPS. [Epic: UE 5.8](https://www.unrealengine.com/news/unreal-engine-5-8-is-now-available), [Lumen-ytelsesguide](https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-performance-guide-for-unreal-engine).

Et tenkt GPU-begrenset eksempel: 20 ms totalt, hvor GI bruker 8 ms. Halveres bare GI til 4 ms, blir totalen 16 ms, altså omtrent 50 til 62,5 FPS. Dette er regning, ikke en SIGNAL / 47-måling.

### Faktisk kvalitetsstyring i spillet

| Spillprofil | `sg.GlobalIlluminationQuality` | Aktiv GI i postprocess | Refleksjoner | Intern oppløsning |
| --- | ---: | --- | --- | ---: |
| Low | 1 | `None`, uttrykkelig overstyrt | Screen Space Reflections | 60 % |
| High | 2 | Lumen | Lumen, Surface Cache | 60 % |
| Ultra | 3 | Lumen | Lumen, Hit Lighting for Reflections | 100 % |

Kilder: [S47Lighting.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47Lighting.cpp), særlig linje 216–263; [DefaultEngine.ini](../../../unreal/Signal47/Config/DefaultEngine.ini), linje 8–25; [S47Web.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47Web.cpp), linje 710–711; [S47Flow.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47Flow.cpp), linje 1042–1047.

Konfigurasjonen ber om Lumen hardware ray tracing på støttet maskinvare, med distance fields som fallback. High/Ultra bruker dermed sanntids-Lumen til indirekte lys og refleksjoner i en verden med skiftende dagslys, bevegelige dører og lys som slås av/på. Vanlig direkte lys og skyggemetode er egne deler av renderingen. Lite kan fortsatt gi dynamisk indirekte lys, med lavere detaljpresisjon.

UE 5.8.4s `Engine/Config/BaseScalability.ini`, linje 326–399, velger `r.Lumen.FinalGatherMethod=0` for Medium/Lite og `1` for High. Medium-refleksjoner bruker SSR. Men spillets Low setter samtidig postprocess-GI til `None`. Det holder derfor **ikke** å se `sg.GlobalIlluminationQuality=1` eller endre én konsollvariabel og konkludere med at Lite er i bruk. GI-metoden må også være Lumen, og kvalitetsvalget må beholdes gjennom vanlig meny, oppstart og lasting.

Dette støttes av allerede eksisterende R5-logger: High-opptaket har `FinalGatherMethod=1` og «Lumen on»; Low-dokumentopptaket har `FinalGatherMethod=0`, men «Lumen off». Det er ikke kjørt nye opptak. Lokal motor bekrefter dispatch i `Renderer/Private/Lumen/Lumen.cpp`, linje 55–61 og 199–217, samt GI-vilkåret i `LumenDiffuseIndirect.cpp`, linje 290–299.

**Anbefaling:** undersøk et eget Medium/Lite-nivå mellom dagens Low og High. Det kan gi bedre lys enn Low, men vil da også legge til arbeid som Low nå unngår. High har allerede 60 % intern oppløsning og Lumen-kvalitet 0,5, så Epics standardsammenligning kan ikke brukes som en målt 2x-gevinst her. Særlig kontor/lab, lyslekkasje ved vegger, døråpninger, emissive detaljer og glass må vurderes i et senere forsøk.

## 2. Fog Screen Space Scattering for atmosfære

UE 5.8 har innebygd **Fog Screen Space Scattering**, merket Experimental. Den tilfører simulert flergangsspredning til tåken, som kan gjøre støvdis og lampeglød mykere. Den beskrevne støtten gjelder Volumetric Fog og Local Fog Volumes. Den skal ikke forveksles med tredjepartsproduktet med lignende navn; ingen ny plugin er nødvendig for å undersøke motorens funksjon. [Epic: UE 5.8](https://www.unrealengine.com/news/unreal-engine-5-8-is-now-available), [komponent-API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UExponentialHeightFogComponent).

Spillet oppretter allerede `AExponentialHeightFog` med volumetrisk tåke, height falloff 0,18, scattering distribution 0,35 og lys blågrå albedo. Tetthet og farge følger område og tidspunkt. Volumetrisk rekkevidde går fra 140 m mot 1200 m ved Caliche. Dagens High bruker grid 16/64 og Ultra 8/96; Low slår volumetrisk tåke av. Dette ligger i [S47Lighting.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47Lighting.cpp), linje 176–183, 216–237 og 313–329.

### Aktivering er ikke det samme som global tillatelse

| Kontroll i installert UE 5.8.4 | Standard / betydning |
| --- | --- |
| `bEnableFSSS` på tåkekomponenten | `false`; prosjektet setter den ikke på |
| `r.Fog.ScreenSpaceScattering` | `1`; tillater funksjonen, aktiverer ikke komponenten alene |
| `r.Fog.SeparateComposition` | `-1`, automatisk; EffectsQuality 0–2 setter den til 0 |
| `FSSSSceneColorScatteringAmountScale` / `FSSSSceneColorScatteringAmountPower` | 1 / 1 |
| `FSSSSpreadScale` / `FSSSBlurControl` | 0,1 / 0,5 |
| `r.Fog.ScreenSpaceScattering.MaxExposedLuminance` / `r.Fog.ScreenSpaceScattering.TAA` | 10 / 1 |

Motorkilder: `Engine/Private/Components/ExponentialHeightFogComponent.cpp`, linje 115 og videre; `Renderer/Private/FogSeparateComposition.cpp`, linje 14 og aktiveringsvilkårene; `Engine/Config/BaseScalability.ini`, EffectsQuality fra linje 823. Spillets High/Ultra-navn setter ikke `sg.EffectsQuality` direkte. Den eksisterende R5-loggen viser EffectsQuality 3 med separat komposisjon tillatt. Det beviser ikke at FSSS ble tegnet: komponentflagget er fortsatt av.

**Anbefaling:** ett svakt FSSS-alternativ på High/Ultra ved SARO i skumring og natt. Bevar først dagens tetthet, lysstyrker og eksponering, slik at effekten kan vurderes alene. Uprøvde startverdier kan være amount scale 0,15–0,25, spread 0,05–0,1, power 1 og blur 0,5. Low bør ha en uttrykkelig FSSS-av-policy; `r.VolumetricFog=0` alene er ikke en sikker FSSS-bryter.

FSSS tilfører skjermteksturer, uskarphetsfiltrering og temporal filtrering. Gevinsten er atmosfære, ikke gratis ytelse. SIGNAL / 47 har ingen måling av kostnaden. Senere vurdering må se etter halo ved dører/antenner, etterslep med lommelykt, redusert skiltlesbarhet og tap av nattkontrast. Hold lampemodellering og riktig plassering som egne oppgaver. Større volumetrisk rekkevidde fordeler også dagens dybdesnitt over mer plass. [Epic: Volumetric Fog](https://dev.epicgames.com/documentation/en-us/unreal-engine/volumetric-fog-in-unreal-engine).

## 3. Skygger fra fjerne planter og småobjekter

Prinsippet er fornuftig: modellen kan fortsatt være synlig selv om dens lille direkte skygge ikke lenger beregnes. Men flere lag er allerede optimalisert, og avstand til spilleren er ikke det samme som om en skygge er synlig. Et objekt utenfor kamerabildet kan fortsatt kaste skygge inn i bildet.

### Hvor det faktisk gjenstår arbeid

| Lag i eksisterende verden | Modellens avstandsgrense | Skyggestatus i kilde/manifest |
| --- | ---: | --- |
| Runtime-småflora | 36–260 m etter art, kortere på Low | Skygger, RT og distance-field-bidrag allerede av |
| Caliche Grass_A/B, 14 000 instanser | 90 m | Av |
| Caliche sage, 10 998 instanser | 220 m | På for hele HISM-familien |
| Caliche yucca, 1 300 instanser | 260 m | På |
| Caliche juniper, 800 instanser | 1200 m | På |
| Caliche_B og Sandstone_A, 5 666 instanser | 110 m | På |
| Highway-overgangens sage | 220 m | På |
| Eldre scatter-komponenter | Ingen generell grense ved oppretting | Skygger hardkodet på |

Dette er manifest-/kildetall, ikke antall samtidige synlige instanser eller målte shadow draws. Caliche_A-småstein er også allerede shadowless.

Kilder: [S47Flora.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47Flora.cpp), linje 28, 594–622 og 644; [naturmanifest](../../../unreal/Signal47/Content/CalicheStreet/Nature/nature-placement.json); [S47CalicheNature.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47CalicheNature.cpp), linje 119 og 163; [S47HighwayLandscape.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47HighwayLandscape.cpp), linje 73; [S47Props.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47Props.cpp), linje 415 og 599. De aktuelle Caliche-instansene er bevisst non-Nanite med tre LOD-er, se [import_assets.py](../../../unreal/Signal47/Tools/calichenature/import_assets.py), linje 270.

### Foreslåtte startavstander, ikke målte fasitsvar

| Gruppe | High | Low |
| --- | ---: | ---: |
| Småstein som fortsatt kaster skygge | 25–30 m | 15–20 m |
| Små sage/yucca | 50–60 m | 30–40 m |
| Juniper | Vurder 200–300 m og eksisterende fjernskygger | Kortere etter faktisk synlighet |
| Bygninger, antenner, klipper og viktige stolper | Bevar store silhuetter og lysblokkering | Bruk konkrete unntak |

Behold eksisterende modellavstander ved første forsøk. Ikke forkort synlig vegetasjon bare for å spare skygger. Dagens HISM samler hele familier over store områder: en bryter basert på avstanden til komponentens origo kan fjerne skygger fra planter rett ved spilleren. En mulig implementering er mindre romlige celler med avstand til nærmeste cellegrense og hysterese. Dette må avveies mot flere komponenter/draw calls; ikke endre hele familien hver frame.

### VSM, distance fields og Lumen må holdes fra hverandre

Prosjektet bruker Virtual Shadow Maps. Lokal UE 5.8.4 setter nye directional lights til 400 m dynamisk skyggeavstand, 512 m distance-field-avstand og DF-skygger på. Spillet endrer ikke disse avstandene. High bruker `sg.ShadowQuality=2`, avstandsskala 0,85 og DF på; Low bruker nivå 1, skala 0,7 og DF av. Det antyder omtrent 340/280 m for nærskyggeområdet før øvrige vilkår. Dette er avledet fra konstruktør og konfigurasjon, ikke avlest i en ny spillkjøring.

For non-Nanite beskriver Epic overgang fra VSM til billigere distance-field-fjernskygger. Nanite fortsetter med VSM. Juniper med 1200 m modellavstand betyr derfor ikke nødvendigvis VSM-skygger helt ut til 1200 m. Småstein og sage er fortsatt innenfor nærsonen. Ikke start med å krympe hele solens shadow clipmaps: tomme clipmap-nivåer har liten kostnad, mens store fjerne former fortsatt kan være viktige. [Epic: Virtual Shadow Maps](https://dev.epicgames.com/documentation/en-us/unreal-engine/virtual-shadow-maps-in-unreal-engine).

Verifiserte motorvalg for senere oppslag:

- `r.Shadow.Virtual.UseFarShadowCulling` er allerede 1. Å sette den til 0 slår av denne cullingen og er ikke et sparetiltak.
- `r.Shadow.Virtual.Clipmap.FirstLevel` / `r.Shadow.Virtual.Clipmap.LastLevel` er 6/22. Ikke endre globalt uten påvist behov.
- `r.Shadow.RadiusThreshold` er 0,05/0,04/0,01 på dagens Low/High/Ultra. VSM bruker terskelen for uncached non-Nanite når `r.Shadow.Virtual.NonNanite.UseRadiusThreshold=1`; det er ingen målrettet vegetasjonsbryter.
- `r.Shadow.Virtual.NonNanite.IncludeInCoarsePages` er 1. Et senere forsøk med dette må også kontrollere skygger i volumetrisk tåke. Dagens kode beholder coarse pages bevisst.
- `WorldPositionOffsetDisableDistance` kan passe senere vindplanter. Nåværende `FLORA_WPO` bruker også WPO til å trekke planter bort ved avstandsgrensen, så en generell av-bryter kan ødelegge eksisterende culling.

Motorkilder: `Engine/Private/Components/DirectionalLightComponent.cpp:1041`, `Engine/Config/BaseScalability.ini:167`, `Renderer/Private/ShadowSetup.cpp:388`, `Renderer/Private/VirtualShadowMaps/VirtualShadowMapClipmap.cpp:57` og `VirtualShadowMapArray.cpp:437,497`.

`Visible in Ray Tracing` påvirker hardware-Lumen og refleksjoner, mens `Affect Distance Field Lighting` påvirker distance-field-scenen. Dette er egne valg, ikke synonymer for å spare direkte skygger. Samtidig krever distance-field-påvirkning `CastShadow=true` i motoren, så å slå av skyggekasting kan også endre software-fallbacken. Bevar store lysblokkere, og vurder denne sammenhengen ved senere implementering. `MaxDrawDistance` på et lys fjerner både lyset og skyggen. [Epic: Lumen tekniske detaljer](https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-technical-details-in-unreal-engine).

`SetCullDistances` gir heller ikke automatisk pen uttoning; materialet må bruke fade-verdien. Epics vanlige foliage-avstandsculling/fading gjelder ikke på samme måte for Nanite-foliage. Ved senere bruk av nye Fab-planter må den konkrete avstandspolitikken derfor sjekkes på nytt. [Epic: foliage](https://dev.epicgames.com/documentation/en-us/unreal-engine/foliage-mode-in-unreal-engine).

## Sammenheng med den vurderte terrengoppgraderingen

Mesh Terrain er allerede undersøkt som et eget, innebygd Experimental-verktøy i UE 5.8.4. Det er relevant for klipper og overheng, men løser ikke i seg selv dagens grove, gjentatte høydeprofiler. [Epic: Mesh Terrain](https://dev.epicgames.com/documentation/en-us/unreal-engine/mesh-terrain-in-unreal-engine).

Dagens geologimesh bruker brede profilbelter med 8 m sampling i mellomdistansen, og kjøreterrengets celler går fra 10 til 120 m. Se [geology.py](../../../unreal/Signal47/Tools/calichestreet/backdrop/geology.py), linje 65–168, og [S47RoadTerrain.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47RoadTerrain.cpp), linje 103–139. Første kandidat er mer varierte grunnformer, tørre bekkefar og fotvifter med utvalgte steinmesher, samt én avgrenset Mesh Terrain-klippeprøve. Delt geometri flyttes mellom SARO- og veikoordinater i [S47HighwayLandscape.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47HighwayLandscape.cpp); nye flater må bevare denne koblingen og de spillbare høydefunksjonene.

Begge Fab-pakkene er nedlastet som dokumentert i [ankomst-/kontorrapporten](../../Evidence/UnrealDeliveries/ARRIVAL_OFFICE_2026-10-26.md). De er ennå ikke migrert inn i spillverdenen. Nye steiner/planter må få bevisste regler for LOD, skygger og RT-bidrag. Mesh Terrain er ikke aktivert av dette arbeidet.

## Avgrenset kontroll ved en senere implementering

Ingen ny kjøring trengs for å lagre disse funnene. Når ett tiltak faktisk implementeres, sammenlignes samme korte rute og kamera med samme oppløsning, tidspunkt, kvalitetsnivå og eksponering:

1. SARO ute med småflora og lav sol: mål skyggepass og se etter synlig inn-/utkobling av skygger.
2. Kontor/inngang med dør og skiftende lys: vurder GI, glass og lyslekkasje for Lite.
3. Kort kameravending og lommelykt i skumring: vurder FSSS-halo, etterslep og lesbarhet.

Kjør bare de utsnittene som berøres av tiltaket. Logg faktisk GPU-frametid og relevante Lumen/VSM/fog-pass; ikke bruk gjennomsnittlig FPS alene. Gjør én endring om gangen. R5s tidligere AlwaysResident RT-geometri på opptil 79,811 MiB er en avgrenset minnemåling, ikke et ytelsesbevis for disse forslagene. Ingen ny helnatt, stor testmatrise eller pakking er utført her.

## Tillegg: mest synlig kvalitetsløft for innsatsen

Tom ba samme dag om prioritering med tilgjengelig utstyr, nedlastede assets og [Nico Lindes HDRI-video](https://www.youtube.com/watch?v=Yq1Y2FWj5aQ). Dette er en anbefaling, ikke en bestilling eller bekreftelse av implementering. GPU lest med nvidia-smi: RTX 3080 med 10 240 MiB. Ingen ny spillkjøring.

### Foreslått produksjonsrekkefølge

| Prioritet | Konkret forbedring | Begrunnelse og omfang |
| --- | --- | --- |
| 1 | Samlet himmel, horisont og lys | Stor bildeflate og stemning gjennom hele innkjøringen. Sammenlign en Poly Haven Pure Sky med forbedrede eksisterende skyformer i samme SARO-motiv. Samordne solretning, himmellys, eksponering og dis. |
| 2 | Synlige mangler ved inngangen og andre nærbilder | Fullfør lampehus, montering, diffusorer, lysplassering og materialoverganger. Spilleren ser disse på nært hold. Bevar de nylig modellerte inngangs- og kontordelene og forbedre konkret gjenstående svakheter. |
| 3 | Terrengformer og natur langs spillerens rute | Bruk tilgjengelige steiner, klipper og materialer i meningsfulle grupper. Bryt rette terrengbelter, forbedre skråninger og overgangen asfalt, grus, jord og fundament. Prioriter SARO og veiens utsikt før hele kartet. |
| 4 | Én komplett, troverdig person | Én UE Optimized MetaHuman med 1986-klær, blikk, pust, passende kroppsanimasjon og munnbevegelse er en avgrenset første leveranse. Velg en person spilleren faktisk møter nært. Figurens opptreden inngår i kvalitetsmålet. |
| 5 | Atmosfære og ytelsesrom | Finpuss eksisterende stedlig lyd og svak vegetasjonsbevegelse; vurder FSSS der lys og tåke tjener scenen. Begrens overflødige småobjektskygger før ny effektkostnad. Lumen Lite er et mulig kvalitetsnivå, ikke i seg selv et visuelt hovedløft. |

Rangeringen er en faglig vurdering ut fra synlighet, gjenbruk og integrasjonsarbeid. Den er ikke en målt gevinsttabell eller et tidsestimat. Ingen nye kjøp anbefales før vi har brukt de nedlastede pakkene og eksisterende CC0-ressurser. Old West og ørkenpakken brukes selektivt for å bevare New Mexico i 1986; en komplett western- eller postapokalyptisk demoby er ikke målbildet.

### Poly Haven er relevant, men HDRI er et fotografi

Videoens beskrivelse og kapitler er lest i YouTube. Beskrivelsen lenker til Poly Haven Pure Skies og 2G Academy, med HDRI-oppsett fra 1:55 og lysjustering fra 2:59. Full transkripsjon ble ikke tilgjengelig i panelet, og hele videoen er ikke avspilt. De tekniske anbefalingene her er kontrollert mot Epic og prosjektkilden, ikke antatt fra videotittelen.

[Poly Haven](https://polyhaven.com/license) tilbyr disse assets under CC0. Velg Pure Sky uten innbakte bygninger, trær eller fjell, slik at vår egen horisont og verden fortsatt bestemmer landskapet. To konkrete kandidater til senere visuell sammenligning er [Kloppenheim 06 Pure Sky](https://polyhaven.com/a/kloppenheim_06_puresky), med skyer og lavt varmt lys, og [Qwantani Dusk 2 Pure Sky](https://polyhaven.com/a/qwantani_dusk_2_puresky), med svakere skyer og skumringsglød. Kandidatene er ikke lastet ned eller prøvd i SIGNAL / 47 her.

Aktiv Unreal bruker egen himmelkuppel, stjerner og volumetriske skyer uten SkyAtmosphere, se [SKY.md](../../Evidence/UnrealDeliveries/SKY.md), [S47Sky.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47Sky.cpp) og [S47Clouds.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47Clouds.cpp). SKY.md dokumenterer også rutedithering og kantstøy i enkelte High/Clean-skyer; bedre form og stabilitet er derfor en konkret kandidat i samme himmelpass. Qwantani Night Pure Sky finnes allerede i historiske Unity-assets, men det er ikke den aktive Unreal-himmelen. [sky_capture.py](../../../unreal/Signal47/Tools/sky/sky_capture.py) bruker dessuten en forenklet capture-gren. Å endre bare det synlige panoramaet gir derfor ikke automatisk tilsvarende lys og refleksjoner.

Anbefalt prøve er å integrere fotografisk himmeldetalj i eksisterende oppsett og beholde de historiedrevne kveldskurvene, kometen, stjernehullet og natta. HDRI-ens sol må samsvare med retningslyset. Et statisk fotografi følger ikke automatisk klokka eller været, og overgang mellom ulike skybilder kan gi doble skyformer. En dramatisk, skyfylt HDRI bør heller ikke automatisk få et fullt ekstra volumetrisk skylag oppå seg. Bruk en enkel kontrollert variant først; velg mellom fotografisk bakgrunn, begrensede bevegelige skyer og forbedret prosedyresky ut fra faktisk sammenheng i bevegelse.

Et mulig startpunkt på denne maskinen er 8K synlig panorama med lavere oppløsning for lys/refleksjoner, justert etter faktisk skarphet og minnebruk. 16K/24K og maksimal kvalitet overalt er ikke et mål. [Epics HDRI Backdrop-guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/hdri-backdrop-visualization-tool-in-unreal-engine) beskriver en innebygd Sky Light og behov for justering av intensitet/eksponering mellom HDRI-er. Ikke legg et ekstra standard-Backdrop med egen Sky Light og projisert bakke over vårt eksisterende rigg uten å integrere disse funksjonene.

For mennesker tilbyr [Epic UE Optimized](https://dev.epicgames.com/documentation/metahuman/assembly?lang=en-US) med High, Medium og Low. Det er relevant for spillpiloten. Linux er ikke en generell sperre for MetaHuman i UE 5.8; [utgivelsesnotatene](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-release-notes) beskriver utvidet Linux-støtte, men enkelte capture-/kroppsanimasjonsfunksjoner er plattformavgrenset. Ingen ny figur eller animasjonspipeline er verifisert lokalt her.

Første anbefalte sammenhengende produksjonsrunde er innkjøringen, parkeringsutsikten og turen gjennom SARO-inngangen: himmel/lys, synlige terrengoverganger og fullførte armaturer i samme strekning. Kontroller ettermiddag, overgang til natt og ett nærbilde i bevegelse, med samme eksisterende bildekvalitet ved før/etter. Utvid til byen og figurer når dette har et tydelig løft. Full terrengmigrering og bred utskifting av alle figurer har større integrasjonskostnad og kommer senere.

## Bibliotek og kildegrunnlag

Prosjektbiblioteket ble søkt først, deretter bredere med `Lumen fog shadows`. Relevant treff var [scenario-unreal-lighting-rendering](https://github.com/scenario-labs/skills/blob/f0adb5b5ebc1fc8f7e333554d2aeb767c5558d6b/skills/game-engines/unreal/scenario-unreal-lighting-rendering/SKILL.md), festet til `f0adb5b5`. Kilden er en arbeidsflytreferanse og oppgir selv at verktøyskriptet ikke var kjørt i ekte Unreal. Ingen skript, innstillinger eller kode fra den er gjenbrukt. Prosjektfunnene over bygger på faktisk C++/Config og installert UE 5.8.4; funksjonsbeskrivelsene er kontrollert mot lenkede Epic-kilder.

Eksisterende bevis, uten nye tester: `~/scratch-bm/Signal47-arrival-office-20261026/runtime-r5/final-smoke/runtime.log` og `dale-documents/runtime.log`. Funksjonsstøtte og konfigurert bruk er bekreftet; kvalitetsgevinst, FPS-gevinst og foreslåtte avstandsverdier er fortsatt uprøvd i SIGNAL / 47.
