# Caliche Homes, presis integrasjon

Denne modulen gjelder ENV02 og krever fungerende TB2 med de opprinnelige 28
native delene. Den endrer ikke TB2-kontrakten eller TownBuildings-assets.
Alle verktøy skriver til egne utmapper. Originaleksporten er skrivebeskyttet
input; bare egen `Content/CalicheHomes/assembly.json` aktiveres av staging.

Kildeidentitet er original manifest-SHA256 `69fcf95d…925c9` og
`geo_008.glb`-SHA256 `0e87d2ac…e2bfee`. Før nye kutt reproduseres hele den
aksepterte TownBuildings/signal-unionen fra originalindekser og sammenlignes
med dens faktiske `retainedIndexSha256`. Alle tre hjørner må ligge i samme
prisme; en trekant som spenner mellom hus klippes aldri som en prismeunion.

Fire hus erstattes av Shell/Roof/Fixtures, til sammen 12 deler. Det presise
huskuttet er **528 triangler**, ikke målegrunnlagets 578. De fire fungerende
trinnene beholder alle 48 triangler i g1006, og H02s mørke glass beholder sine
to triangler i g1007. Modellen har ingen nye kopier av dem. g1006 mister bare
de fire opprinnelige betongbåndene, 12 triangler per hus. Dørholdere
1436/1439/1442/1445 med barn, lisensdøren g1075, panelene 1594 til 1596,
tv1/tv2/kitchen/smoker-proxyer og Dressing beholder node-/komponentidentitet.
Lys, mennesker, medier og lagret progresjon følger sine eksisterende drivere.

| Geo | Node | TB2 før | Hus fjernet | Skilt fjernet | Etter |
| --- | --- | ---: | ---: | ---: | ---: |
| g999 | 1448 | 1404 | 0 | 48 | 1356 |
| g1000 | 1449 | 132 | 0 | 24 | 108 |
| g1001 | 1450 | 8 | 0 | 4 | 4 |
| g1002 | 1451 | 332 | 192 | 0 | 140 |
| g1003 | 1452 | 48 | 48 | 0 | skjult |
| g1005 | 1454 | 120 | 120 | 0 | skjult |
| g1006 | 1455 | 154 | 48 | 0 | 106 |
| g1013 | 1462 | 120 | 120 | 0 | skjult |

To eksisterende skilt kopieres fra kildens materiale 638/641/642, 38
triangler hver. Sørskiltet flyttes fra z2335 til z2315, foran den uendrede
innkjøringen ved z2335. Nordskiltet flyttes fra z1880 til z1895, med den
opprinnelige nordvendte forsiden for sørgående trafikk. Sørskiltet
beholder sin sørvendte forside for nordgående trafikk. X-posisjoner, størrelse, skiltgrafikk,
UV-er, farger og normaler beholdes. Hver stolpes fot følger terrenget ved sin
egen nye X/Z, mens platen følger senterhøyden. Runtime sammenligner de seks
kontaktene med faktisk `S47RoadTerrain::SurfaceY` før aktivering.

Alle fire stolpebokser flyttes samtidig i både `walks.road.colliders` og
`drive.legs.road.obstacles`. Andre bokser, husenes kollisjon og kjøreturen
endres ikke. Build-kroken ligger etter TownBuildings i Web::BeginPlay,
før S47HUD oppretter US47World og EnsureLegs kopierer kjøredataene.

## Kildepreparering og koordinater

Ren Python, ingen motor eller DCC:

```sh
python3 unreal/Signal47/Tools/calichehomes_integration/prepare_trims.py \
  /home/tom/scratch-bm/Signal47-web /ABS/ENV02-TRIMS
```

Resultatet har status `PREPARED_NATIVE_UNVERIFIED`: ett GLB med fem ikke-tomme
kuttmesh, ett GLB med to skilt, og `trim-manifest.json`. Tre helt tomme
originalmesh markeres som skjult i metadata; de importeres ikke.

`bounds_three_m` og GLB-posisjoner bruker Three (Y opp). `bounds_local_m`
bruker **Blender** (Z opp), også for husene fra kildemanifestet:

```text
Three(x,y,z) -> Blender(x,-z,y) -> native UE(100x,-100y,100z)
                                = Three -> UE(100x,100z,100y)
```

`convert_trims.py` kjøres eksplisitt i Blender gjennom prosjektets guard av
den som eier tunge jobber. Adapteren oppretter bare nye, tomme mesh. Den
sveiser ikke og bruker ikke mesh.validate. Den bevarer kildehjørnenes UV,
farge og normaler, gir stabile FBX-materialnavn og kontrollerer faktisk FBX
før `trim-fbx.json` kan få PASS. UV-V følger samme GLB/Blender-konvensjon som
prosjektets eksisterende FBX-adapter. Normalene lagres direkte som Blenders
`custom_normal` CORNER/FLOAT_VECTOR for å unngå split-setterens fanutjevning.

Kuttmaterialer heter `M_Homes_Trim_gN_slot<originalordinal>`. Skiltene har
`M_Homes_SignNorth_slot0/1/2` og tilsvarende South. FBX-kvitteringen binder
de faktisk eksporterte navnene, ordningen, bounds i Blender-meter og SHA256.
Rootens separate `Tools/calichehomes_native` importerer/verifiserer de 19
meshene. Husenes faktiske materialinstances heter MI_Homes_* og bruker
eksisterende Town-foreldre og teksturer. Runtime erstatter kutt-/skiltplasser
med de opprinnelige komponentenes materialer, og verifiserer slotordningen
før den første utskiftingen.

## Uavhengig native-gate

```sh
python3 unreal/Signal47/Tools/calichehomes_integration/stage_assembly.py \
  --source /ABS/CalicheHomes --trims /ABS/ENV02-TRIMS \
  --import-report /ABS/native-import.json --verify-report /ABS/saved-native.json \
  --project /ABS/Signal47 --report /ABS/stage.json
```

Import og en separat, lagret native-verifikasjon må begge ha PASS og nøyaktig
samme aktuelle kilde-/nativehashes. Native-inventaret må omfatte alle 19
meshene og husenes materialinstances. Kilder, alle tre LOD-er, akser/bounds,
materialnavn/ordning, sidefiler og native `.uasset`-filer kontrolleres før
assembly erstattes atomisk. Eksisterende assembly sikkerhetskopieres ved
hash. Endrede kilder, ukjent tidligere kutt eller et delvis resultat gir ingen
aktivering. Runtime gjør dessuten en samlet preflight før noen original
mesh eller kollisjonsboks endres. SHA1 av de faktiske pakkede manifest- og
TB2-metadatafilene binder runtime til den kontrollerte forgjengeren; staging
binder hele beviskjeden med SHA256.

`AS47Web::AuditCalicheHomes` kontrollerer alle 19 installerte mesh, tre skjulte
forgjengere, beskyttede node-/meshidentiteter og begge kollisjonskopier.
`-S47CalicheHomesOff`, manglende assembly eller mislykket preflight beholder
originale hus og skilt. Audit av en inaktiv mod skal ikke regnes som native
PASS. Storyens lovlige endringer i synlighet, bevegelser og materialparametere
fryses eller nullstilles ikke av audit.

## Bevisgrenser og målrettet runtime-kontroll

Den lette kildekontrollen beviser nøyaktige indekser, deterministiske bytes,
bevarte 48 trinntriangler, fjernede 48 betongbåndtriangler, g1007 utenfor
kutt, original normal/UV/farge, begge skiltføtter og same-box-regelen. Den
beviser ikke Blender-kjøring, nativeimport, rendering eller en spilløkt.

Før levering trengs faktisk import og fersk saved-verifikasjon, native bygg,
en hel innkjøring fra eksisterende commute-start forbi sørskiltet og alle
fire fasader, samt sørgående lesbarhet ved nordskiltet. Kontroller synlig
originaltekst på begge skilt, terrengføtter, stolpehindringer ved både kjøring
og gange, fri kjørebane, uendrede dører/trinn og de tre levende vindusbildene.
Test tv1/tv2/kitchen/smoker-proxyer og normal menneske-/lystilstand før og
etter lagring/Continue. Ta audit både før og etter områdeskifte og restore.
Kjør også med ufullstendig native-sett eller eksplisitt Off for å bevise hel
originalfallback uten halve huskutt eller flyttede usynlige stolpehindringer.
Pakking/cook må inkludere egen CalicheHomes-mappe og assembly; de endringene
eies av hovedagenten. Ingen av disse runtimepunktene er bevist av Python.
