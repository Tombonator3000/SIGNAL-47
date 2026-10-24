# Caliche: stolper, luftlinje og vedlikeholdsskur

Originalt tillegg til den ferdige trafostasjonen fra 1986: tre værslitte trestolper med porselensisolatorer og beslag, ni hengende faseledere, terminalstag med jordanker, låst bølgeblikkskur, kabeltromler, verktøykasse og avgrenset steinfot. Ingen kjøpte modeller eller eksterne bildeavhengigheter.

Faktisk oppdatert Blender-bygg og **222 eksportkontroller PASS** leverer sju deler med **25 666 / 14 342 / 8 652 trekanter** på LOD0/1/2. Hero-previewen er gjennomgått. Materialkorrigeringen gir større sammenhengende rustfelt på målt 21 % av bølgeblikk og 24 % av tak, roligere galvanisert grunn og svakere normalstyrke. Crossarmens endekapper har separat Y/Z-UV. Oppdatert Blend, FBX, GLB og de 21 pakkede teksturene er kontrollert. Native visuell godkjenning er fortsatt **UNVERIFIED**; forelderens native rapporter er autoritative for spillet.

## Originalkilder og eksport

`design.py` låser plassering og lederpunkter; `geometry.py` inneholder egne komponentformer. `shapes.py` gjenbruker den lokale, verifiserte `../substation/meshkit.py` uten å endre den. `make_textures.py` lager originale 2048-pikslers tre-, rust- og skiltkart. Galvanisert metall, betong og grus gjenbruker egne originalkart via relative stier til `../substation/textures/`. Alle 21 faktiske bildefiler pakkes i `Substation_Site.blend`.

Blender-filen har en sammenstilt scene og skjulte, komponentmerkede originalkopier. `exports/` inneholder hver av de sju delene som FBX i tre LOD-er samt GLB for LOD0. `Substation_Site.glb` er samlet backup med plassering påført én gang. `axis_probe.fbx` er en separat asymmetrisk prøve, ikke et runtimeobjekt.

| Del | LOD0 | LOD1 | LOD2 | Site-offset i Blender-meter |
|---|---:|---:|---:|---|
| SM_SubstationSite_Pole_A | 3970 | 2474 | 1754 | 0, 21.8, −0.240012 |
| SM_SubstationSite_Pole_B | 3970 | 2474 | 1754 | 0, 53.8, −0.228311 |
| SM_SubstationSite_Pole_C | 3970 | 2474 | 1754 | 0, 85.8, −0.301798 |
| SM_SubstationSite_Shed | 1992 | 1544 | 1008 | 6.5, 4.9, 0 |
| SM_SubstationSite_Equipment | 2740 | 1444 | 836 | 6.8, 2.65, −0.08 |
| SM_SubstationSite_Lines | 2028 | 872 | 510 | 0, 0, 0 |
| SM_SubstationSite_Ground | 6996 | 3060 | 1036 | 0, 0, 0 |

Materialer har prefiks `M_SubstationSite_` og PBR-sidetabell i `materials.json`. Normalteksturer følger DirectX-konvensjon; Blender-materialet korrigerer grønn kanal for visning. Metallplatene er tosidige. Fysiske bølger i platene er geometri, ikke normaltekstur. Oppdatert rustoppskrift skriver målt dekning til `texture_authoring.json`. Den første, forkastede småflekkede versjonen ligger i `~/scratch-bm/Signal47-substationsite/first-confetti-rust/`.

## Koordinater og kontakt

Blender bruker meter, Z opp. Hver eksportdel har lokal pivot null og vertskoordinater relativt sin egen offset. `site_local_offset_m` påføres én gang ved montering. Lines og Ground inneholder allerede site-koordinater og bruker offset null.

Felles root er Three `[7890, −1.10, 536.20]`, yaw 180°. Dermed er `worldThree = (7890 − bx, −1.10 + bz, 536.20 + by)`. Native import bruker den samme målte FBX-policyen som trafostasjonen: Blender `(x,y,z)` blir UE `(100x,−100y,100z)` med import-yaw 90°. Prøven har senter `(2,4,6)` m, størrelse `(0.2,0.4,0.6)` m og forventet UE-senter `(200,−400,600)` cm.

Faseavstand er 1.04 m. Lederne møter de eksisterende busstubbene ved `B[x,4.65,7.85]`, og mastenes fester ligger nøyaktig 9.2 m over hver mastbakke. Spennenes midtsig er 0.65 / 0.9 / 0.9 m; geometrien følger `z(t)=lerp(z0,z1,t)−4·sig·t·(1−t)`. Alle LOD-er har midtpunktvertex. Siste ledere avsluttes fysisk på mast C. Guy-anker er faktisk terrengmålt til `B[0,90,−0.342236688]`; trestolpene går 0.35 m ned i bakken.

Skuret har 3 × 2.4 m vegger, tak innen 3.2 × 2.8 m, dør mot −Y og sokkeltopp på Bz 0. Utstyret står på eksisterende grusnivå Bz −0.08. Steinene følger bare kjent grusbed og de målte mast-/ankerføttene. Ingen nye terrengplan, grøfter eller jordhaloer er lagt til. Meshene har NoCollision; runtimeagentens små fotproksyer håndterer mast/guy, og det eksisterende stengte området dekker skuret. Den eksisterende telefonstolpen og kulverten berøres ikke.

## Generering og verifisering

Maskinen deler én tungjobb-slot med andre agenter. Kjør bare etter eksplisitt samordning. `run_source.py` utfører teksturer, Blender-bygg og kontroll sekvensielt via `unreal_guard.run_owned`, med to CPU-tråder og uten UE eller render:

```bash
cd unreal/Signal47/Tools/substationsite
python3 run_source.py
```

`verify_site.py` leser de faktiske 21 FBX-ene og sju binære GLB-ene: mål/pivoter, antall og materialplasser, ikke-kollapsede UV-er, ledernes eksporterte ringer ved start/midt/slutt, eksakt sig, avrundet porselen med flate endekapper og de faktiske pakkede bildefilene. Den håndhever LOD0-budsjettet på 35 000 trekanter. Den utfører ingen native import eller visuell godkjenning. Importør, native kontroll og runtime-plassering eies av forelder/runtimeagent.
