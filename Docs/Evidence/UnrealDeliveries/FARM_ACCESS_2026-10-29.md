# Gården øst for landeveien: synlighet og stengt port

**Status:** Under arbeid. **Gjelder:** ENV-04, den eksisterende ranchen og adkomsten fra landeveien. **Sist kontrollert:** 29. oktober 2026.

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

## Videre kontroll og leveringsgrense

Postfiks-bygg og den sammenhengende kjøre-/gangreisen er **UNVERIFIED** til faktisk resultat foreligger. Den målrettede reisen skal bruke normal innkjøring, målt bilstopp ved positiv gass, GetOutHere, postkasse/port til fots, samme bilinteraksjon, fysisk revers til landeveien og ordinær disk/title/Continue. Den første isolerte case1-disken er en uttrykkelig kopi av det faktiske case0-snapshotet; denne fixturen må skilles fra senere normal Flow.WriteSave.

Pakket spill, menneskelig input, subjektiv lesbarhet/lytting, FPS og fullmatrise er foreløpig **UNVERIFIED**. Ingen installasjon eller offentlig release er utført. Baseline er et feilbevis, ikke en ferdig levert kandidat.
