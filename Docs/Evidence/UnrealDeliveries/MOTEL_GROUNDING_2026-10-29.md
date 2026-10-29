# Motellreise, gangnivå og rekvisitter

**Status:** Under arbeid. **Gjelder:** FIX-02 til FIX-05, målrettet editoraksept; pakket prøve venter. **Sist kontrollert:** 29. oktober 2026.

Editor-rute R11 fra `d707abd4` består hele motellreisen: 58 rutepunkter, 14 interaksjoner, sju gjestedørnisjer, fire blokkeringer og ordinær isolert disk/title/Continue etter faktisk inngang til rom 6. Kilde, lagret native-import og TV-ens GPU-bilde/lydklokke er kontrollert. Pakket motellrute er ikke kjørt. Den lille mørke klumpen utenfor bassenggjerdet i E04 er fortsatt uidentifisert og åpen; ingen ekstra geometri er slettet på dette grunnlaget.

## Kilde og avgrensning

Utgangspunktet er den eksisterende produksjonsfilen `Sierra_Production.blend`, SHA-256 `53b48aa90d4cf4cda385bdda1c345f58f1381529ce00a193b39054406e361bff`, med 24 deler, 24 materialer og 67 teksturer. Den er bevart som urørt input til separate reparerte pakker. R5 endret Office, Glass og stolene i Room02, Room04 og Room07, med 58 øvrige FBX-er og 67 teksturer bytebevart. R6b retter deretter 172 målte, lukkede gulvøyer / 32 336 trekanter i Office, CourtSurface, PoolDeck og Room01–Room08. Bare de identifiserte gulvøyenes vinding, hjørnerekkefølge og normalretning endres; posisjoner, UV-er og materialbindinger bevares. R6b har 40 øvrige eksportfiler og alle 67 teksturer byteidentiske med R5. De tre allerede korrekt orienterte CourtDetail-stiene og møblene får ingen global vending.

Målt resepsjonsgulv og fliser erstatter tidligere omtrentlige ganggrenser. En faktisk spalte på 75 mm mellom gulvet og svalgangen får en synlig betongterskel fra modellens eksisterende betongmateriale. ICE-/avisdeler flyttes samlet, telefonfronten vendes mot adkomsten og bekreftede gamle duplikater pensjoneres. Native potter har allerede korrekt fothøyde og beholdes; de tre native stolene grunnsettes etter målt laveste fotpunkt. Bare den aktive gamle marmorlampen, node 737, pensjoneres ved resepsjonen; asset og kreditering beholdes. Identifiserte gamle stol-, ICE- og avisdeler med tilhørende kollidere erstattes av de flyttede native delene. Gamle potter, brusboks og faktisk hus-/gjerdekollisjon beholdes. Bassengobjektet fra spillerbildet fjernes først når faktisk identitet er fastslått.

`grounding-contract.json` binder forfattet gulv til terrengkutt og runtime-layout. Klipping av begge gamle terrenglag beholder opprinnelige binære prefikser og kontrollerer areal per trekant samt uavhengige stråler gjennom de skrevne GLB-filene. Staging avviser ulik kilde-/trimkontrakt eller endrede kildefiler etter import. Den gamle asfaltflaten g456/node 671 er presist klippet under kontraktflatene: 24 opprinnelige trekanter blir 30 gjenværende kanttrekanter, med opprinnelig bufferprefiks og interpolerte grenseattributter bevart. Innkjøringen beholdes. Additiv lysdekor g530/node 770 beholder geometri, materiale og transform; bare dens fysiske kollisjon fjernes, slik at gulvstrålen når det synlige ganggulvet.

Bibliotekssøket fant ingen relevant prøvd reparasjonsløype som overstyrer prosjektets eksisterende. Arbeidet gjenbruker den allerede prøvde FBX-/materialimporten for Unreal 5.8.4; uprøvde generelle Meshy-/Unreal-eksempler er ikke brukt som motorbevis.

## Utførte og avviste kontroller

| Kontroll | Faktisk resultat | Begrensning |
| --- | --- | --- |
| Ren geometri/layout | PASS: 7 402 ruteprøver, 550 innvendige og 60 utvendige terrengstråler. | Statisk geometri, ingen spillerinput eller native-kontroll. |
| Editorbygg | PASS, også etter samordning med main PR217. | Bygg erstatter ikke runtime. |
| Blender-inventar | PASS; original hash og faktisk attributtliste er sikret. | Lesende kildeinventar. |
| Apply R1 | Avvist på eksplisitt kontroll av egendefinerte normaler. | Ingen ny kilde tatt i bruk. |
| Apply R2 | Avvist med SIGSEGV ved utvidelse av eksisterende meshdatablokk, før lagring. | Originalkilden er fortsatt byteidentisk. Crash og avviste utdata er beholdt privat. |
| Apply R3, nytt mesh | Avvist av normalgrensen før binding/save; første avvik 0,00232. | Ingen geometriendring er akseptert. |
| Isolert normalprobe | PASS som diagnose: setter på original kopi og nytt mesh gir opptil 0,00365117 avvik; rå INT16_2D-kopi gir komponentavvik 0 og dot 1. | Målt uten ny geometri eller lagring av input. |
| Apply R4 | Avvist før binding/save: rå buffer er bevart, men flyttet/skalert geometri endrer komprimeringens beregnede normalretninger. | Ingen toleranse ble senket. |
| Direkte vektorprobe E | PASS, også etter save/reopen: alle 829 275 gamle Office-hjørner har avvik 0; 1 692 nye har maks 2,98 × 10⁻⁸. | Diagnose av faktisk reparert Office, ikke native aksept. |
| Apply R5 og separat audit/reopen | PASS på alle fem deler: gamle normaler avvik 0, nye maks 2,98 × 10⁻⁸. 15 FBX-er regenerert; 58 FBX-er og 67 teksturer bevart. | Native og runtime kvitteres separat. |
| Native R1 | Avvist før staging fordi reimport gav [0,0,0] for eksisterende avstandsgrenser. | Delvis import er ikke akseptert. |
| Native R2 | PASS: 2 696 importkontroller og 1 408 kontroller fra fersk leser på 31 meshes. Fjern-LOD har samlet 19 915 trekanter, under grensen 24 000. | Importens stdout manglet markøren; faktisk rapport og separat engine-logg er sikret med SHA, rå guard-kvittering er bevart. |
| Editor-rute R1 | FAIL ved installasjonsaudit før gange: telefonretningens kontroll. Selvtesten avviste også bakkeforventningen ved SARO. | Ingen rute-, TV-, disk- eller bildeaksept fra dette forsøket. |
| Editor-rute R2/R3 | FAIL ved første gulvprøve etter fire beståtte blokkeringer; gammel smussflate treffes 6,8 mm under forventet flisnivå. | Seksjonsflaggene var allerede på for alle 31 modeller og alle LOD-er. Hypotesen om avskrudd seksjonskollisjon er avvist. |
| Gulvdiagnose | Faktisk Blender-BVH treffer alle fem flisprøver på Z=0,012, med geometrisk forside nedover. Faktisk runtime-bilde viser flisgulvet. | R6b retter den målte kildevindingen; ingen tosidig query er brukt som løsning. |
| R6 første apply | Avvist før eksport på kjent fabrikkattributt `.select_vert`. | Kildens eksisterende seleksjonsdata og fravær bevares nå eksplisitt. Ingen auditgrense er senket. |
| R6b apply og separat audit | PASS: 172 målte lukkede gulvøyer, 32 336 trekanter i 11 deler får utvendig forside. Faktisk lagring/reopen, eksport, kildeflater og 290 rutefotavtrykk er kontrollert. 40 øvrige eksportfiler og 67 teksturer er byteidentiske med R5. | Akseptert kilde; native og runtime har egne kontroller nedenfor. |
| Native R4 og fersk leser | PASS: 2 038 importkontroller og 1 542 lesekontroller på 31 meshes. | Bare 11 gulveiere reimportert; resten SHA-bevart. |
| Editor-rute R4 | FAIL ved punkt 11: gammel asfalt g456/node 671 treffes i samme høyde som det nye ganggulvet. De første 10 punktene, 1 709 selvtester og fire blokkeringer består. TV har 48 397 / 49 152 endrede GPU-piksler og 6,059 s lydklokkedelta. | Råbilder er åpnet. Ingen hel rute-/diskaksept eller subjektiv lytting. |
| Native R5, fersk leser og staging R5 | PASS: 1 787 importkontroller og 1 555 lesekontroller på 32 meshes; presist g456-trim inkludert. Staging binder 24 deler og 11 patcher til samme kilde-/trim-/importhasher. | Fjern-LOD er fortsatt 19 915 / 24 000 trekanter; dette er ingen GPU-minnemåling. |
| Editor-rute R5–R7 | FAIL ved avisfronten etter 15 punkter. Telefonens fysiske frontstråle består. | Glasset er tilsiktet NoCollision. Først fysikkforventningen, deretter utilgjengelig CPU-renderdata/readback ble avvist; ingen avisaksept fra disse forsøkene. |
| Editor-rute R8 | FAIL med 2/5 gulvtreff ved punkt 31: tre kollineære prøver ligger i en faktisk 2 cm kildefuge. | Ni prøver med fire hjørner erstatter fem; minst 6/9 kreves. Høyde- og native-eierkrav er uendret. |
| Editor-rute R9 | FAIL etter 31 punkter: alle ni stråler treffer additiv dekor node 770, 2 cm over gulvet. | Den avgrensede kollisjonsrettingen bevarer dekorens synlige geometri, materiale og transform. |
| R10-planens bygg | Avvist med `-Wshadow` i `build-editor-r11.log` fra `cdc79a55`. | Ingen editor-rute R10 ble kjørt. Lokalt auditscope ble rettet i `d707abd4`; `build-editor-r12` består. |
| Editor-rute R11 | PASS: 1 709 selvtester; 58 punkter, 14 interaksjoner, 7/7 dørnisjer, 4/4 blokkeringer og 1 611 fysiske gulvstråler. 178,6332 m / 142,9989 s; rom 6 og ordinær disk/title/Continue består. | API-styrt Walker med eksplisitt kapittel 4-inngang, fire blockerplasseringer og én rutestartplassering. |
| TV i R11 | PASS: native skjermmateriale slot 1 og faktisk 256 × 192 GPU-tekstur; 48 034 / 49 152 piksler endret. Film fra frame 185 til 258; lydklokkedelta 6,0373 s. | Produksjonens program-/lydklokker, ikke subjektiv lytting eller FPS. Ingen TV-produksjonsendring var nødvendig. |
| Editorbilder R2 | PASS, 24 faktiske bilder fra åtte motellkameraer i dag/skumring/natt. Fire utvalgte originaler følger repoet og er åpnet for inspeksjon. | Kameraplasseringene er fixtures fra før siste g456-trim; bildene beviser ikke siste trim, menneskelig gange eller klumpens identitet. |
| Pakket motellrute | UNVERIFIED / venter. | Ingen motellpakking eller pakket rutekjøring er gjort i dette kontrollgrunnlaget. |

Normal-API-et snitter normaler innen samme glatte vifte før komprimering. Dette er bekreftet både i den isolerte proben og [Blenders kilde ved installert revisjon](https://github.com/blender/blender/blob/d13f752e3b9c/source/blender/blenkernel/intern/mesh_normals.cc#L1457). De fem endrede delene lagrer derfor de opprinnelige retningene direkte i `custom_normal/CORNER/FLOAT_VECTOR`. Kildeproben og R5 kontrollerer den faktiske float32-payloaden, vektorene og lagret fil. I R5 beholdt de øvrige attributtene og alle 19 uendrede delene sin representasjon. Originalfilen er bevart i Git og i en separat byteidentisk baseline. R5-pakken ble promotert med Blender-hash `a82f9575462f5b29fb9721ec003f3d2ec534cfbe935c70813e78ddc401c7eeb7` og er bevart som R6b-input. Normalretningen på de identifiserte gulvøyene er deretter rettet i R6b, med separat kontroll av alle bevarte data.

Det private rågrunnlaget ligger i `~/scratch-bm/Signal47-motel-grounded-20261029`. Alle avviste forsøk beholdes. R11 gjelder testet kilde `d707abd4`; senere samordning med main/ENV02 i `8ea05a94` er ikke en ny R11-kjøring.

Reimporten viderefører den allerede aksepterte skjermpolicyen `[2,0.75,0.5625,0.005]`, målt på 24 av 24 modeller i tidligere `native-verify-r4.json` med SHA-256 `4ccd813e9cd9684a6c79e496e2bc8b62a1e7c6444873c4457f590f0d21a4426d`. Fersk LOD0/2 kopieres til fjern-LOD3 også ved omkjøring. Verifieren kontrollerer både faktisk lagret avstandspolicy og geometriposisjonenes hash, slik at likt trekanttall ikke skjuler gammel geometri.

## Målrettet runtimeprøve

R11 bruker `-S47Jump=chapter4 -S47MotelV2Route -S47SelfTest` og isolert `-UserDir=/tmp/S47-motel-grounded-editor-r11`. Etter fire eksplisitte blockerplasseringer og én rutestartplassering går normal Walker/Resolve sammenhengende 178,6332 m gjennom 58 punkter. Resepsjonens register, nøkler, telefon og TV, telefonfront, ICE/avis, bassengport/dekke og romfasaden inngår. Sju gjestedørnisjer prøves, og rom 6 nås gjennom faktisk `room6Door`-bruk. Deretter går vanlig diskleser, title/Continue og full Flow gjennom bevaring av rom, historietilstand, klokke, posisjon og motellinstallasjon. Ingen teleporter skjuler blokkeringer under selve gangruten.

De ni gulvstrålene krever minst seks faktiske første treff på native motellgulv innen 1,5 cm høydeavvik; Walker må ligge innen 1,2 cm fra forfattet gulv. Fire hjørneprøver er lagt til fordi en 2 cm forfattet fuge kan romme de tre kollineære midtprøvene. Høydekrav, native-eierkrav og gangrute er uendret. R11 teller 1 611 fysiske gulvstråler, inkludert adkomstprøvene.

Telefonfronten har et faktisk fysikktreff fra fri gangside. Avisglasset er tilsiktet `NoCollision`: testen leser faktiske native LOD0-vertex-/indeksbuffere fra GPU, bruker komponentens transform og beregner linje/trekant-treff på den avgrensede ruten. Dette er en test mot rendergeometri; den beviser verken fysisk glasskollisjon eller synlighet gjennom resten av scenen. Ingen ny produktinteraksjon er lagt til for prøven.

TV-prøven bekrefter at skjermmaterialet på den faktiske native TV-en er bundet til samme dynamiske tekstur før og etter venting. GPU-readback gir 48 034 endrede piksler av 49 152. Produksjonens bildeklokke går fra 15,48 s / frame 185 til 21,52 s / frame 258, med høyttalernivå 0,320 og faktisk lydklokkedelta 6,0373 s. Dette bekrefter levende film og avspillingens tidsfremdrift, ikke subjektiv lydkvalitet.

Bassengdiagnosen skriver 18 nærliggende meshkomponenter, 57 faktiske instanser med indeks/bounds/transform og åtte siktstråler i `pool-exterior.json`. Den binder ennå ingen kandidat til den lille mørke trekanten ved gjerdestolpen i [E04](../../../unreal/Evidence/Playtest-2026-10-28/04-pool-approach.png). Den tilsiktede brune pumpekassen er ikke identifisert som feilen. Klumpen er **UNVERIFIED / åpen**, og ingen flora, pumpekasse eller annen ukjent geometri er fjernet for å lukke punktet.

[Bevisoversikt med seks originale PNG-er](../../../unreal/Evidence/MOTEL-GROUNDING-2026-10-29/README.md) og [maskinlesbar R11-kvittering](../../../unreal/Evidence/MOTEL-GROUNDING-2026-10-29/editor-r11-receipt.json) binder de fire statiske R2-bildene og de to faktiske R11-TV-avlesningene. TV-bildene er åpnet og viser endring; de er teksturavlesninger, ikke helskjermbilder av TV-apparatet.

## Kilde- og resultatbinding

R6b er promotert i `9674ba9a` etter kildeaudit og nativeaksept. Manifestets SHA-256 er `4c66c66fa2c9ac8762e32844dffcf656313955aeb608daebd40e380bb901c831`, Blender-filens `8a32e308f53ce862f69985e90b564c5296097106e763e1b896080cd7e63bea1b`. Originalen og privat R5 er fortsatt bytebevart. Den arvede box-hjelperens innadvendte gulvøyer er rettet i faktisk kildegeometri; ingen usynlig erstatningsflate eller tosidig query er brukt.

| Privat kvittering | SHA-256 |
| --- | --- |
| `source-repair-r6b.json` | `bf025a0c6a34f2faa026c99a5f4a61974a7b3bed3d16346a38dc969b7b82f8e0` |
| `source-audit-r6b.json` | `3129e191a0332ba5f91e5c38283ea3c00633035e52c1e2194e7299ec880cf808` |
| `native-import-r5.json` | `5f9ef02c1f646f0267471f3611a102bd8f555f58d95718212d06c9cf3caf2014` |
| `native-verify-r5.json` | `25a4004a097993d57eb0ced5900384c9f0d1a17f2debada4bc89dbb4a4da3e40` |
| `stage-r5.json` | `b1f13b5c13612f7740bac0a5d3af9f9368f99d323554951f21300e5c39cf7591` |
| `editor-route-r11.result.json` | `ebc811985a92daaf5e620227ecc881431fa7925d5d4b63f0570986aa531a1be5` |
| Rutens `result.json` | `0072d26cb360c8e93ce716d7441194a3b7ff286919baa67a79343b06913195a1` |

Rutens siste resultat, TV-PNG-er, `room6.saved.json` og bassengdiagnose ligger i `/tmp/S47-motel-grounded-editor-r11/Saved/MotelV2Route/01A18608FAC476D58D47A5B93C4FDDE8`. Guard-kvitteringen har normal exit 0, ingen registrert tungjobbkonkurranse og ingen forbudte markører. Rutetid er 142,9989 s; guardens samlede prosesstid 178,2344 s er et annet mål.

Dette er en API-styrt motellreise med uttrykkelig kapittel 4-fixture. Tidligere kapitler, menneskelig input, subjektiv lytting, FPS og hele kapittel 4 er ikke gjennomspilt eller målt. Pakket motellkontroll venter, og ingen fullmatrise, installasjon i `~/Games/Signal47` eller offentlig release er utført.
