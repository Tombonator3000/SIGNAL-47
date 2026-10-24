# Caliche natur, 24. oktober 2026

**Kandidat under sluttkontroll, ikke installert.** Pakke `de38da9747d9680c9788414688de9f641f09086bc1c936a38a8685619bad7ea8`. Redigerbare modellkilder og native assets er samlet i `19ed326d`, med natur-runtime fra `faf278c6`. Claudes skyarbeid til `04d98231` og core/film `f9e1d4aa` er integrert. 721 faktiske Source/Config/Content-filer er bundet til kandidaten i `candidate-production-binding.json`.

## Resultat og sammenligning med konseptet

[Referansen](Concepts/Caliche-Nature-2026-10-24/01-street-and-nature.jpg) er bevart med [proveniens](Concepts/Caliche-Nature-2026-10-24/provenance.json), SHA256 `8dd8773f3970f5922e9877b79ec2685bd30ea5a6ad9f1c48d42d3a0d75df9516`. Den brukes som form- og fargereferanse, uten å kopiere bildepiksler inn i teksturene.

Førbildene viste lys, nesten tom bakke og eldre regelmessige fjellformer. Kandidaten har gråbrun mineraljord, grusskuldre, småstein, sølvgrønt kratt, strågult gress, yucca, mørkere einer og sammenhengende lagdelte bergbelter. Bakken og plantene møter eksisterende vei og bygg. Tidligere grå HISM-materialer, grove papirlignende blader, sammenfalte UV-er og markante hvite fjellstriper ble avvist og rettet gjennom faktiske spillbilder.

Kandidaten er fortsatt tydelig et stilisert spillbilde. Fjellveggene er jevnere og mer sammenhengende enn fotoet; den store lyse høyreskråningens hovedsilhuett består selv etter lokale bergbrudd og ribber. Forgrunnen har mindre relieff og drenering, og flere busker har beslektede silhuetter. Klar veimerking og enkle eksisterende bygg skiller seg også fra referansen. Aksept av dette produksjonsinkrementet betyr ikke at nær-foto-målet er nådd. Skyformene eies av Claudes integrerte arbeid og får ingen separat kunstnerisk godkjenning her.

## Blender og plassering

| Kilde | Innhold |
| --- | --- |
| [Mineralbakke](Signal47/Tools/calichenature/ground/Caliche_Nature_Ground.blend) | 36 terreng-/skulderdeler, 104 949 trekanter, FBX per del og samlet GLB. Originale 2048² basefarge-, ruhets- og DirectX-normalbilder ved fysisk 6 m UV-flis. |
| [Vegetasjon](Signal47/Tools/calichenature/vegetation/caliche_vegetation.blend) | 12 originale prototyper, tre forfattede LOD-er hver, 36 FBX og 36 GLB. Egne PBR-atlas med maskerte småblad/nåler og tett gress/yucca. |
| [Geologi](Signal47/Tools/calichestreet/backdrop/Caliche_Mesas.blend) | 103 550 trekanter, seks stabile materialslotter og ni originale PBR-kart. Lagdelte berg, ur og fjernrygger. |

35 597 deterministiske forekomster i 12 HISM-komponenter dekker naturfeltet rundt Caliche. Hovedfeltet ligger omtrent lokal X +/-600 m og world Z 1650..2450 m, med anker `[8000,0,2060]`. Beskyttelsen følger hele OldRoad, 124 originale colliderrektangler, bygg, gangarealer og viktige tilnærminger. Eksisterende fotunderlag, veihøyder, kollisjon, historieruter, saveformat og 27 antenner beholdes. Nye flater og planter har NoCollision.

CN5 endrer bare en påvist OldRoad-overgang i geologien. Alle 53 213 XY-punkter, veimasken og teksturene er byte-/hashkontrollert uendret fra CN4; 1253 høyder får lokal erosjon og berg-/urmaterialer. Konservativ klaring for hele hevede trekanter er minst 224,56 m fra OldRoad og 237,93 m fra hovedveien. Eksportkontrollen krever ikke-null UV-areal og oppadvendte faktiske trekanter.

## Faktiske kontroller

| Kontroll | Resultat |
| --- | --- |
| Terrengkilde | 442/442 PASS. |
| Plantekilde og eksport | 270/270 PASS; 100 produksjonsfiler hashbundet. |
| Geologieksport | PASS på faktisk lagret Blend, FBX og GLB; koordinater, winding, UV, bounds, teksturer og veiklaring. |
| Native natur, frisk editor | 1004 lesbare kontroller PASS, 48 mesher. |
| Eksisterende gateassets, frisk editor | 182 kontroller PASS. |
| Linux-pakke | PASS, materialcook uten erstatningsmaterialer. |
| Bildegjennomgang | 33 faktisk åpnede spillbilder, akseptert med kjente stilbegrensninger. |
| Dører, array, buzzer, foto og historie | 11/11 PASS på samme pakke. |
| Hele eksisterende screenshots.sh | Pågår eller venter, ingen slutt-PASS. |

[Native natur](</home/tom/scratch-bm/Signal47-caliche-nature/CN5/nature-native.json>), [gateassets](</home/tom/scratch-bm/Signal47-caliche-nature/CN5/street-native.json>), [funksjonstester](</home/tom/scratch-bm/Signal47-caliche-nature/runtime-latest.json>) og [full regresjon](</home/tom/scratch-bm/Signal47-caliche-nature/screenshots-full-result.json>) har separate bevis. Bevisroten er `~/scratch-bm/Signal47-caliche-nature`; tidligere forkastede kandidater er bevart der.

Terrengets faktisk lagrede render-UV-er er sammenlignet med kilde ved absolutt toleranse `1e-4`; FP32-UV unngår tidligere FP16-trinn. Render-RGBA kontrolleres separat. Alle tre plantematerialer har faktisk lagret instansbruksflagg, er rekompilert og kontrolleres også før runtime-opprettelse. Pakket logging leser faktiske kokte LOD-grenser 1/.12/.025, antall, cull-avstand, NoCollision og første/siste verdenstransform for alle 12 familier. Nature-spesifikke shader-/SMU-/fallbackvarsler avvises.

UE5.8 beskytter editorfeltet `bAutoComputeLODScreenSize`. 48 avlesninger står eksplisitt UNVERIFIED, uten oppdiktet bool-verdi. Dette er skilt fra de faktisk avleste lagrede og kokte LOD-grensene. Antallsbevis betyr heller ikke at hver enkelt plante er visuelt inspisert.

## Ytelse

High, 2560x1440, bildeeffekt av, RTX 3080 / driver 595.99, samme ståsted og klokke før/etter, målevindu synlig sekund 4..10, ingen konkurrerende tungjobb eller shader-/PSO-kø i vinduet.

| Klokke / utsnitt | Før mean / p95 ms | Kandidat mean / p95 ms | FPS fra mean |
| --- | ---: | ---: | ---: |
| day / outskirts | 11.02 / 12.11 | 14.39 / 15.80 | 69.48 |
| day / road-edge-long | 10.81 / 12.08 | 14.12 / 15.46 | 70.81 |
| dusk / outskirts | 11.01 / 12.20 | 14.95 / 17.21 | 66.89 |
| dusk / road-edge-long | 11.03 / 12.35 | 14.57 / 15.83 | 68.64 |

[Hele målerapporten](</home/tom/scratch-bm/Signal47-caliche-nature/final-CN5-performance/nature-review.json>) sammenligner med den frosne førpakken `6103dd97...`. Den samlede kandidaten inneholder natur, skyer og filmkode, så hele kostnadsforskjellen kan ikke tilskrives naturen. Dette er intervaller mellom rammer på spilltråden, ikke isolert GPU-tid. Skumring/outskirts har p95 over 16,67 ms. Gjennomsnittet er over 60 FPS i disse utsnittene, men låste 60 langs en spillerstyrt rute er ikke dokumentert. Teksturresidens og nattytelse er ikke målt.

## Levering og avgrensning

Eksisterende installasjon beholdes mens sluttprøvene kjører.

Den nye kildeleveransen er `~/Desktop/Signal47 Caliche natur`, med tre originale Blend-filer, tilhørende teksturer/eksporter, SHA256-manifest og offline `SPILLBILDER.html`. Den eldre leveransen `Signal47 Caliche konseptpass` beholdes. Leveransefilene opprettes først fra de fryste kildene og testbevisene; eventuell installasjons-/leveranse-PASS må finnes i egne JSON-rapporter.

Automatiske faste teleporter, API-bruk og screenshots.sh er ikke menneskelig gjennomspilling, lytting, komfortvurdering eller en spilt fullnatt. Nattbilder uten lommelykt bekrefter orientering/silhuett; fin plantedetalj vurderes fra dagslys. Ingen ny ekstern spillpublisering er utført.

Prosjektbiblioteket ble brukt før implementering. Scenario world-building/materials ved festet commit f0adb5b5 ga prinsipper, men toolkit var ikke motorverifisert og ble ikke installert. VibeUE krevde en ekstra editorplugin og ble ikke brukt. De faktiske FP32-UV- og HISM-bruksflaggene bygger på prosjektets eksisterende Free/Web-import og er kontrollert i dette prosjektet med UE 5.8.4 / Blender 5.2.2.
