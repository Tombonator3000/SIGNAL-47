# Gården øst for landeveien: synlighet og stengt port

**Status:** Referanse, levert målrettet kandidat R1 gjennom [PR223](https://github.com/Tombonator3000/SIGNAL-47/pull/223). **Gjelder:** ENV-04, den eksisterende ranchen og adkomsten fra landeveien. **Sist kontrollert:** 29. oktober 2026.

Den eksisterende ranchen er bakgrunnsdekor med hus, låve, tank og vindmølle. Innkjørselen går til en stengt rørport og ALDERETE-postkassen. Ingen implementert besøksinteraksjon, innendørsdestinasjon eller egen gårdsoppgave er funnet. Arbeidet bevarer dette omfanget.

## Sted og kildegrunnlag

[Toms E01](../../../unreal/Evidence/Playtest-2026-10-28/01-farm-road.png) passer med dette særpregede motivet og kildeplasseringen. Dette er en sterk identitetsslutning støttet av kilden og ny befaring; det historiske bildets eksakte kamerastilling, tidspunkt og bygg er fortsatt ukjent. Nye bilder skal ikke omtales som en rekonstruert E01-eksponering.

Koordinatene nedenfor er Three-rommets X/Z i meter. Road-world har X +8000; i SARO-området blir samme plassering road-lokal X −24 / Z +300. Høyden følger eksisterende terreng.

| Del | Road-lokal X/Z | Road-world X/Z | SARO X/Z |
| --- | --- | --- | --- |
| Hus | 418 / 192 | 8418 / 192 | 394 / 492 |
| Låve | 446 / 170 | 8446 / 170 | 422 / 470 |
| Tank | 462 / 200 | 8462 / 200 | 438 / 500 |
| Vindmølle | 456 / 214 | 8456 / 214 | 432 / 514 |
| Stengt port, midtpunkt | 21 / 196,584213 | 8021 / 196,584213 | −3 / 496,584213 |
| ALDERETE-postkasse | 9,6 / 191,975485 | 8009,6 / 191,975485 | −14,4 / 491,975485 |

Originalene finnes i [roadPlaces.ts](../../../web/src/drive/roadPlaces.ts) og [roadProps.ts](../../../web/src/drive/roadProps.ts), bevart som historiske kilder. Unreal bruker [S47RoadTerrain](../../../unreal/Signal47/Source/Signal47/Private/S47RoadTerrain.cpp), [S47World](../../../unreal/Signal47/Source/Signal47/Private/S47World.cpp) og de opprinnelige eksporterte meshene. Prosjektbiblioteket ble søkt først; generelle FBX-/Unreal-treff ga ingen ferdig løsning med verifisert kompatibilitet. Prosjektets eksisterende indeksdeling og sammenhengende landskap brukes.

## Faktisk negativ baseline R2

Baseline fra `70a1491dc36217954f0494cfe8940a314f36ecff` kjørte med `-S47Evening=commute -S47FarmAccessContract` i en isolert `/tmp`-profil. Én uttrykkelig `BeginCommute`-startfixture satte bilen ved road-lokal X1,8 / Z535. Deretter brukte prøven ordinær World/Drive-kontroll uten ny bilplassering, med naturlig road→SARO-overgang og sving inn til porten.

To produktfeil ble bekreftet:

- De tre opprinnelige gårdskomponentene, n1301–1303 / g910–912, blir faktisk usynlige ved områdeskiftet. De ligger under road-roten og inngikk ikke i det delte landskapet. Før-/etterbildene har forskjellige faktiske cockpitposisjoner; komponentavlesningen beviser skjulingen. Baseline hadde ikke CPU-lesbare native vertexdata, så et beregnet nulltall alene brukes ikke som geometribevis.
- Etter **351,871909 m** nådde bilens målte, orienterte kollisjonshjørne road-X **21,020552** gjennom den synlige portens plan X21. Første feilpose har senter X18,362235 / Z196,433973, heading −1,562385 rad og fart 0,837752 m/s. Porten manglet eget SARO-hinder; senterbegrensningen alene beskyttet ikke bilens nese.

Prøven avsluttet forventet med **FAIL / exit 1**, tre innledende kontroller og én vanlig områdesovergang. Guard registrerte ingen tungjobbkonkurranse. Den har ingen aksept av senere parkering, gange, retur eller lagring. [Avledet baselinekvittering](../../../unreal/Evidence/FARM-ACCESS-2026-10-29/baseline-r2-receipt.json) og [originalbildene](../../../unreal/Evidence/FARM-ACCESS-2026-10-29/README.md) binder de faktiske avlesningene. Hele råmappen er bevart byteidentisk privat som `Signal47-farm-access-20261029/baseline-r2-original`.

Første editorbygg stoppet på privat kameratilgang i testdriveren, før runtime. R2 brukte offentlig PlayerCameraManager og bestod bygg. En senere dokumentcommit endret ingen av de kjørte runtimefilene.

## Avgrenset retting og native-forberedelse

`797e1669` gir porten én eksplisitt boks etter hinderlastingen i begge områder. Portspennet følger de eksporterte stolpene, road-Z193,184213 til 199,984213, med X20,972 til 21,028 fra rørgeometrien. Den eksisterende bredere road-gjerdeboksen beholdes. Bilens OBB/SAT, toleranser, kjøreegenskaper og øvrige hindere er uendret; Walker bruker den ordinære hinderlisten.

Synlighetsrettingen deler bare ranchens 300 trekanter fra de tre blandede originalmeshene:

| Geometri | Opprinnelig | Ranch | Øvrig geometri |
| --- | --- | --- | --- |
| g910 | 204 | 12 | 192 |
| g911 | 3708 | 248 | 3460 |
| g912 | 360 | 40 | 320 |
| Sum | 4272 | 300 | 3972 |

Utvalget følger road-lokal X390–485 / Z140–235. Ingen trekant krysser utvalgsgrensen. Hele blandingsmeshene skal ikke flyttes, siden de også inneholder uvedkommende rekvisitter. Det delte gårdssettet bruker eksisterende kanonisk landskapsforskyvning og NoCollision. Originale assets beholdes; indeksdeling skal bevare vertex-/tangent-/UV-/fargebytes, winding, materialplasser og de solide delenes ray tracing. Restkomponentene skal beholde sin eksisterende kollisjon. Samlet originalfallback gjelder hvis preflight ikke består.

[Native CPU-forberedelse R2](../../../unreal/Evidence/FARM-ACCESS-2026-10-29/cpu-prepare-r2-original.json) består for akkurat tre meshes. Bare `allow_cpu_access` endres til true; trekanttall, slots, RT=true, Nanite=false og bodyflag-signaturer er uendret. Originalfilene ble sikkerhetskopiert før endringen, og kvitteringens førhashes stemmer med disse. R1 stoppet før første mutasjon på et Python-feltnavn; alle tre native filhashes var fortsatt uendret. R2 rettet feltlesingen med editorens property-API, normal exit0 og ingen tungjobbkonkurranse. Denne metadataforberedelsen er ikke alene buffer-, runtime- eller pakkeaksept.

## Avvist editorreise R1 etter produktretting

Editorbygg R3 fra `6cbf272e` består, 113,39 s. Reisen når faktisk positiv-gass-bilstopp, GetOutHere, postkassen, Walker-stopp ved porten, original bilproxy, samme parkerte bil, fysisk revers og ordinær disk/title/Continue. Målt rute er 380,331840 m kjøring og 30,971644 m gange. Begge barrierer og disk-/quest-/posekontrollene består.

Hele kandidaten er likevel **avvist** med FAIL/exit1 på siste geometrikontroll. Ranchdelingen valgte samlet originalfallback før noen komponent ble byttet. Ti avlesninger bekrefter manglende aktivering, og SARO skjuler derfor fortsatt gården. Lokal motorkilde viser at den nye indeksbufferens tomtilstand avviser `TrySetAllowCPUAccess(true)` før `SetIndices`; kravet skal utføres etter datainnfylling og før ressursinitialisering. Ingen bevaringskontroll fjernes. Privat råmappe `editor-r1-original` og `editor-r1-acceptance.json` bevarer forsøket. Passerte port-/diskdelsteg gir ikke samlet synlighets- eller pakkeaksept.

## Avvist oppstart R2 og bestått editorreise R3

R2 fra `3b419ff5` stoppet før kjøreruten med motorassert `StaticMesh->GetRenderData()->IsInitialized()`, etterfulgt av motorens SIGSEGV-avslutning. Det er en konkret oppstartsrekkefølgefeil ved den første komponentbindingen, ikke en ny tilfeldig trafikk- eller minnefeil. Guard registrerte exit−11 uten konkurranse. Nativefilene var byteidentiske med CPU-forberedelsen. Råcrash med kontekst er bevart privat, sammen med kilde-/modulhash; ingen R2-ruteaksept hevdes.

`575ffd15` initialiserer alle seks kloner etter full preflight og før noen komponentbinding eller originalbytte. Editorbygg R5 og hele R3-reisen fra denne kilden består: **1709 selvtester, 27 kontroller, 380,331508 m kjøring og 30,971449 m gange / 190,920167 s**. Både bil og Walker stoppes fysisk ved porten. Faktisk positivt gasspådrag ved bilstopp er registrert, etterfulgt av vanlig GetOutHere, postkassegang, original bilinteraksjon, samme bilpose, fysisk revers og full disk/title/Continue med normal Flow.WriteSave etterpå.

[Editor-R3-kvitteringen](../../../unreal/Evidence/FARM-ACCESS-2026-10-29/editor-r3-receipt.json) validerer sluttmarkør, normal exit0, alle ti bevaringsavlesninger, 300 faktisk synlige gårdstrekanter i riktig aktivt område før/etter områdeskiftet og etter Continue, samt original-/klonebuffere, indekser, materialer, RT, kollisjon og renderflagg. De CPU-forberedte nativefilene er uendret etter kjøring. Alle sju PNG-er har kontrollerte CRC/IEND/SHA og er åpnet. Den faktisk parkerte bilen har fremste kollisjonshjørne road-X 20.971988355 ved positiv gass 0.883110.

Main med [PR222](https://github.com/Tombonator3000/SIGNAL-47/pull/222) er deretter samordnet i `c301ead9`, uten å endre gårdskoden. Oppdatert editorbygg R6 består. Pakket R1 er produsert fra denne rene kilden: 13 465 fryste runtime-inputfiler / 8 378 607 341 logiske byte. Den pakkede prøven bekrefter også main sine 1720 selvtester.

## Pakket R1 består

Kandidat R1 fra `c301ead9313510125d037a4cc020d667cd9c5661` består bygg/cook, uendret kildefingerprint og hele farmreisen. [Pakket kvittering](../../../unreal/Evidence/FARM-ACCESS-2026-10-29/packed-r1-receipt.json) binder de 13 465 runtime-inputfilene / 8 378 607 341 logiske byte og åtte payloadfiler. Spillbinæren er 314 401 152 byte, SHA-256 `2b6b60bf13d9d821d6f3520ed1105a25ff85b3c009cce2dbb9e0a7727de8acea`. Guard rapporterer normal exit0 uten konkurranse, og sluttmarkøren `FarmAccess contract PASS` finnes.

Prøven består **1720 selvtester, 27 kontroller, 380,332192 m kjøring og 30,965562 m gange / 185,235679 s**. Alle ti native geometriavlesninger består gjennom road/SARO, porten og full Continue: 300 synlige ranchtriangler på riktig sted, uendrede originale buffer-/indeks-/materialfingeravtrykk og bevarte RT-/kollisjons-/renderflagg. De tre CPU-forberedte nativefilene er byteidentiske etter runtime. Selve portstoppet registrerer positiv gass, faktisk orientert bilfotavtrykk på nær side og senere Walker-stopp. Original bilproxy, normal retur/revers og hele diskgjenopprettingen består.

Alle sju originale pakkede PNG-er er kontrollert for CRC, IEND og SHA og åpnet. Fire følger [bevisoversikten](../../../unreal/Evidence/FARM-ACCESS-2026-10-29/README.md); resten og det originale resultatet er bevart privat. Bildene har eksisterende RT-budsjettvarsel ved 80,481/400 MiB. Dette er ingen måling av FPS eller samlet GPU-minneaksept. Gårdsbygningene er små på rundt 400 meters avstand; kvitteringen skiller komponent-/bufferbevis fra subjektiv fjernlesbarhet. De to egne OpenOrder-loggene er sikkerhetskopiert og gjenopprettet til eksakte førhashes; ingen andre filer ble tilbakestilt.

## Leveringsgrense

Spilleren kan kjøre den korte innkjørselen til den stengte porten, gå ut til postkassen og porten, gå tilbake til bilen og rygge ut. Gårdsbygningene bortenfor er bakgrunnsdekor. Ingen ny gårdsoppgave, person, innendørsdestinasjon eller adgang bak porten er produsert.

Én innledende road-Z535 BeginCommute-plassering er en uttrykkelig startfixture. Den første isolerte case1-disken er en uttrykkelig, uendret kopi av det faktiske case0-snapshotet; etterpå brukes ordinær title/Continue og normal Flow.WriteSave med fersk diskleser. Ruten bruker API-kontroll, normal Drive/Walker og faktiske interaksjonsstråler, ikke menneskelig input eller fysisk gamepad. Tiden er målt testkjøring, ikke dokumentert menneskelig spilletid.

Historisk E01-kamera/bygg, menneskelig input, subjektiv fjernlesbarhet/lytting, FPS og fullmatrise er **UNVERIFIED**. Denne farmreisen gir ikke målrettet avspillingsaksept for meteorpassasjen som kom inn med main. Ingen installasjon eller offentlig release er utført. Den konkrete leveransen ligger i [PR223](https://github.com/Tombonator3000/SIGNAL-47/pull/223); baseline og R1/R2 beholdes som avviste forsøk.

## Bekreftet merge og lokal opprydding

PR223 er faktisk flettet i main `094a67cb3e1ba68e34521fb19add28726609e89c`, med verifiserte foreldre `3cbaf99d` og forventet PR-head `010f38d0`. Etter at Claudes ART01-økt ble avsluttet av eier, fullførte vakten den autoriserte oppryddingen kl.08:38. Bare sju genererte husR2-payloadfiler, 4 475 806 238 logiske byte, ble pensjonert. Gjeldende farmR1, de to eldre dispatchR1/motellR1 og de særbeskyttede FIX01-/historiske sammenligningsbyggene beholdes. Alle andre registrerte payloadhash og alle 91 ferskt registrerte personlige Saved-filer er uendret. Privat `post-prune-after-r1.json` binder faktisk resultat; det første nektede forsøket er bevart i `cleanup-refused-r1/` og fjernet ingen filer. Kilder, mapper, logger, bilder og råbevis er bevart.
