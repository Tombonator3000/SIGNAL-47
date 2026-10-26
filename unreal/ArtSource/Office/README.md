# Dales Operations-kontor

Original Blender-modellering for SIGNAL / 47, 26. oktober 2026. Ingen nedlastet geometri. `build_office.py` bygger åtte separate native assettyper med sammenhengende meterskala og bevegelige skuff-, nøkkel- og dørdeler. `Signal47_Operations.blend` bevarer de redigerbare komponentene i egen samling. FBX-filene ligger i `Export`, PBR-kilder i `textures` og modellmål i `model_contract.json`.

Modellene omfatter detaljert stålpult og kontorstol, serviceskap med ekte skuffvolum og beslag, profilert nøkkel med ring og tagg, massiv kontordør med lås/vrider/hengsler/sparkeplater, terskel og karmer, gulvfliser/himlingsgrid, hyller/permer, oppslag, persienne, radiator og VHS-tralle. De gamle kubemodellene for dette innholdet er erstattet i `S47Office.cpp`. Eksisterende lesbare spilldokumenter, lampe og TV beholdes som egne runtimeobjekter.

Dørens reserve ligger fortsatt fysisk i serviceskuffen. Adgang er ikke lenger sperret til kapittel 2: spilleren kan hente nøkkelen, åpne rommet og lese papirene under den tidlige SARO-økten. Avreise til STATION 01 beholder egen historieport. Ingen uprodusert VHS markeres som avspilt.

`Preview` inneholder Blender-render, ikke spillbilder. Den separate renderer bruker lys og kameraer som ikke eksporteres. Spilltekst og dokumentark vises av runtime og er derfor ikke illustrert som ferdig lesestoff i Blender-bildene.

Kommandoer fra prosjektroten:

```
blender -b -t 2 --python unreal/ArtSource/Office/build_office.py -- --render
blender -b unreal/ArtSource/Office/Signal47_Operations.blend -t 2 --python unreal/Signal47/Tools/office/verify_source.py
```

`Tools/office/import_assets.py` kjøres i Unreal Python-commandlet og skriver bare `/Game/Signal47/Art/Office` pluss den eksisterende CPU-metadataforberedelsen av de seks kontorportalmeshene. Tre LOD-er importeres/genereres per asset. `Saved/office-native-import.json` er importkvittering.

`Tools/office/rt_proxy.py` aktiverer Nanite bare på de to stasjonære rom-/dekorassetene `SM_Office_Room` og `SM_Office_Dressing`. Kildens trekantandel beholdes på 100 %, trimfeil er 0, og separat rasterfallback genereres med 100 % / feil 0. RT-proxyen får 60 %; materialene er opake og hardware RT beholdes. Prosjektets `Tools/material_usage.py` setter, kompilerer og lagrer Nanite-permutasjonene på disse modellenes egne materialer; fersk verifikasjon sjekker flaggene. Lokal UE 5.8.4-kode i `StaticMeshBuilder.cpp` linje 166–170 og 845–856 viser at dedikert RT-proxy bare bygges når Nanite er aktivert. Den første innstillingsendringen med Nanite avslått kunne derfor ikke gi redusert RT-geometri, selv om innstillingene ble lagret. Ingen globale grenser eller cvars endres. Verifikasjonen sjekker kildebytes, fallbacktopologi, materialer, kollisjon, mål og begge sett innstillinger; den faktiske minnebesparelsen må måles separat i den kokte spillpakken.

`-S47OfficeArrival -S47Evening=parking -UserDir=/tmp/<egen profil>` kontrollerer den faktiske ankomst-/åpningsløypa før D01, uten å fullføre overleveringen eller endre fase. `-S47OfficeEarly -S47Jump=chapter1 -UserDir=/tmp/<egen profil>` kontrollerer eldre kapittel-1-saker. Begge er avgrensede runtimekontroller med ekte interaksjonsray, key/drawer/door, Walker-rute og disk/full-Flow-restore. Den første korridorplasseringen er fixture. Testen hevder ikke menneskelig input eller full kveldsgjennomspilling. Eksisterende T02-kontrakter kontrollerer videre kapittel-/feltforløp. Kilde, native import og runtime må rapporteres separat.
