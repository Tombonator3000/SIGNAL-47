# Motellreise, gangnivå og rekvisitter

**Status:** Under arbeid. **Gjelder:** FIX-02 til FIX-05, egen kildegren og kandidat uten installasjon. **Sist kontrollert:** 29. oktober 2026.

Reparasjonen samler gangnivå, dørenes kollisjon, resepsjon/TV, telefonveggen og bassengstien i én spillerreise. Kildekontrakten er skrevet, men kildeeksport, native-import, runtime og pakket kontroll skal kvitteres hver for seg før punktene lukkes.

## Kilde og avgrensning

Utgangspunktet er den eksisterende produksjonsfilen `Sierra_Production.blend`, SHA-256 `53b48aa90d4cf4cda385bdda1c345f58f1381529ce00a193b39054406e361bff`, med 24 deler, 24 materialer og 67 teksturer. Den leses som urørt input til en separat reparert pakke. Fem deler får avgrensede endringer: Office, Glass og stolene i Room02, Room04 og Room07. De øvrige 58 FBX-filene, atlasene og materialmanifestet skal forbli byteidentiske.

Målt resepsjonsgulv og fliser erstatter tidligere omtrentlige ganggrenser. En faktisk spalte på 75 mm mellom gulvet og svalgangen får en synlig betongterskel fra modellens eksisterende betongmateriale. ICE-/avisdeler flyttes samlet, telefonfronten vendes mot adkomsten og bekreftede gamle duplikater pensjoneres. Native potter har allerede korrekt fothøyde og beholdes; identifisert lampegruppe pensjoneres uten å fjerne krediteringen. Bassengobjektet fra spillerbildet fjernes først når faktisk identitet er fastslått.

`grounding-contract.json` binder forfattet gulv til terrengkutt og runtime-layout. Klipping av begge gamle terrenglag beholder opprinnelige binære prefikser og kontrollerer areal per trekant samt uavhengige stråler gjennom de skrevne GLB-filene. Staging avviser ulik kilde-/trimkontrakt eller endrede kildefiler etter import.

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
| Native R2 og rute | UNVERIFIED. | Neste faktiske kontroll. |

Normal-API-et snitter normaler innen samme glatte vifte før komprimering. Dette er bekreftet både i den isolerte proben og [Blenders kilde ved installert revisjon](https://github.com/blender/blender/blob/d13f752e3b9c/source/blender/blenkernel/intern/mesh_normals.cc#L1457). De fem endrede delene lagrer derfor de opprinnelige retningene direkte i `custom_normal/CORNER/FLOAT_VECTOR`. Kildeproben og R5 kontrollerer den faktiske float32-payloaden, vektorene og lagret fil. De øvrige attributtene og alle 19 uendrede deler beholder sin representasjon. Originalfilen er kildegrunnlaget; før eksplisitt promotering av den godkjente pakken sikres også en separat byteidentisk baseline.

Det private rågrunnlaget ligger i `~/scratch-bm/Signal47-motel-grounded-20261029`. Akseptert kilde, import, binær og konkrete resultatfiler tilføyes etter faktisk kjøring.

Reimporten viderefører den allerede aksepterte skjermpolicyen `[2,0.75,0.5625,0.005]`, målt på 24 av 24 modeller i tidligere `native-verify-r4.json` med SHA-256 `4ccd813e9cd9684a6c79e496e2bc8b62a1e7c6444873c4457f590f0d21a4426d`. Fersk LOD0/2 kopieres til fjern-LOD3 også ved omkjøring. Verifieren kontrollerer både faktisk lagret avstandspolicy og geometriposisjonenes hash, slik at likt trekanttall ikke skjuler gammel geometri.

## Målrettet runtimeprøve

Den nye driveren bruker en eksplisitt kapittel 4-fixture og fire isolerte kollisjonsplasseringer, fulgt av én sammenhengende Walker-rute med 58 punkter og cirka 183 m. Resepsjon, telefonfront, ICE, avis, bassengrunde og alle åtte dørinnstikk inngår. Rom 6 skal nås gjennom faktisk dørbruk, fulgt av ordinær isolert disk/title/Continue. TV-kontrollen leser den faktisk bundne GPU-teksturen og sammenligner film-/lydklokker. Dette er en API-styrt rute; tidligere kapitler, menneskelig input og subjektiv lytting er ikke underforstått bestått.

Ingen fullmatrise, installasjon i Games eller offentlig release inngår i denne leveransen.
