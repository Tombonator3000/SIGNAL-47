# Avgrenset rutenatur, private Fab-kopier

Åtte allerede eide modeller. `selection.json` binder konkrete pakkestier,
originalhash, roller og lokale mål. `provenance.json` oppgir opphav og avgrensning.
Ingen vendor-piksler, modell-/UV-data eller uassets hører hjemme i det offentlige
repoet. Originalpakkene endres ikke. Lokale mellomkopier ligger i ignorerte
`Content/Northwood/` og `Content/JustBStudios/`; sluttsettet ligger bare under
`/Game/Licensed/RouteNature`. Cook skal kun referere sluttsettet.

## Kjøring

Alle motorjobber reserveres og kjøres sekvensielt gjennom `unreal_guard`.
Skriptene starter aldri motoren. `S47_RN_WORK` peker på en absolutt bevismappe;
standard er `~/scratch-bm/Signal47-terrain-20261026/assets`.

1. Vanlig Python: `python3 Tools/routenature/stage_assets.py` gir tørrkjøring.
   `--apply` kopierer den avgrensede closure etter faktisk `git check-ignore` og
   hashkontroll. Originale rotmapper kan angis med `S47_RN_DESERT_CONTENT` og
   `S47_RN_OLDWEST_CONTENT`; de skal peke på de to nedlastede `Content`-mappene.
2. UE5.8: `-run=pythonscript -script=.../Tools/routenature/inspect_assets.py`
   med `-unattended -nullrhi -nosplash -NoSound`. Kun lesing. Rapporten må være
   `PASS_READ_ONLY_NATIVE_INSPECTION`. Den aktuelle inspeksjonen 26. oktober
   ble kjørt uten konkurrerende jobb og bekreftet 70 native avhengighetspakker.
3. UE5.8: `-run=pythonscript -script=.../Tools/routenature/import_assets.py`.
   Krever kontrollert `materials.json` med `native_channel_review=PASS`.
   Importrapport og `model-contract.json` skrives i bevismappen.
En separat, fersk vedlikeholdskontroll er tilgjengelig med samme importskript og
`S47_RN_VERIFY_ONLY=1`. Den åpner sluttsettet uten mutasjoner og lager
`native-verify.json` og `model-contract-verified.json`. Dette er valgfritt, ikke et
ytt obligatorisk leveransetrinn; sluttpakking og pakket runtime åpner også
assetene på nytt. Ingen ny migrering eller materialbygging skjer i kontrollmodus.

Import kan kjøres med `runpy.run_path` i samme eksplisitt eide prosess som
backdropimporten, sekvensielt. Miljøvariablene over må være satt før skriptet
leses. Feil kastes videre og skal stoppe den samlede jobben.

## Faktisk materialkontrakt

Inspeksjonen leser de opprinnelige materialinstansenes teksturparametre og
modermaterialenes native nodeforbindelser. `materials.json` dokumenterer graf-ID
og rapporthash. Filnavn alene brukes ikke til kanalvalg.

- Stein: egen Base Color RGB og Base Normal RGB. Native `Base Mask.R` er ruhet,
  også når kartet heter `DpR`. Ingen RVT, wetness, world-aligned secondary texture
  eller demoens terrengmaterialer følger med.
- Tørt gress: Albedo RGB, Albedo A som opacity, ARM G som ruhet og egen normal.
- Død busk og billboard: Albedo RGB, ORT R som opacity, ORT G som ruhet og egen
  normal. Shaderne er maskerte og tosidige. Alle har faktisk HISM-bruksflagg.
- Originale teksturpiksler og UV-er bevares. Lokale teksturkopier har eksplisitt
  fargerom/kompresjon og maks størrelse, med 4K basefarge for stor klippe og 2K
  for de øvrige kartene. Dette begrenser renderresidens, ikke originalkildene.

## Geometri og lys

Originale MeshDescription-antall for kilde-LOD-er og samtlige reflekterte Nanite-,
LOD-build- og reduksjonsinnstillinger sammenlignes med den uendrede lokale
originalen. Alle tilgjengelige kilde-MeshDescriptions må ha identiske vertex- og
triangeltall. `GetNumTriangles(i)` leser derimot `RenderData.LODResources[i]` på
alle nivåer, ikke forfatterens kildedata. Nanite LOD0 er generert fallback;
lavere nivåer kan også være bygde reduksjoner. Hvert rendernivå rapporteres
separat og kontrolleres innen 15 % av inspeksjonens renderantall; samme LOD-antall
og uendrede kilde-/buildinnstillinger kreves fortsatt. Klippens påviste endring
fra 15 442 til 16 525 gjelder fallback; kilden beholder 1 998 242 trekanter.
Boulder B viste også en generert LOD3-endring fra 415 til 419, med uendret kilde
og reduksjonsinnstillinger.

Importer endrer ingen punkter, normaler, UV-er, pivoter eller Nanite-rastervalg.
Dette er kopi-, antalls- og konfigurasjonsbevis, ikke en full vertexbufferhash.
Bounds-avvik rapporteres i centimeter og begrenses til 0,25 % av originalens
største dimensjon, minimum 1 mm: generert fallback/proxy kan utvide bound-unionen.
Native bounds står i kontrakten i UE-centimeter med Z opp. Flere pivoter ligger
under/utenfor fotavtrykkets bunn; runtime skal bruke faktisk bounds, aldri anta Z0.

En avbrutt import kan fortsettes. Eksisterende materiale- og teksturgrafer
kontrolleres uten omskriving. En lagret mesh gjenbrukes uten setter eller save
når faktiske materialbindinger, RT-proxy, kildeidentitet og eventuelle
LOD-terskler stemmer; kilde-/geometrikontrollene kjøres fortsatt. Dette unngår en
ny dyr Nanite-bygging av en ferdig klippe. Rapporten oppgir alle mutasjoner og
`reused_without_mesh_rebuild` per mesh.

Primærgrunnlaget er UE5.8-kildene `StaticMesh.cpp::GetNumTriangles` og
`StaticMeshBuilder.cpp`: Nanite-fallback bygges inn i LOD0-renderdata, mens
`RayTracingFallbackMeshData` bygges separat. Ingen global fallback- eller
RT-budsjettgrense justeres.

Klippe og de tre større steinene beholder skygge og ray tracing. Bare en separat
RT-proxy reduseres på de private Nanite-kopiene: innledende mål 3000 trekanter for
klippen og 768 for øvrige steiner. Prosenten beregnes fra faktisk source count.
Dette lover ingen målt minnebesparelse; pakket RT-budsjett og silhuett vurderes
samlet. Smågrus, gress og busk trenger ikke RT eller skygger. Ingen global
grafikkgrense endres. Buskens fem originale LOD-er beholdes; billboardets terskel
legges etter runtime-cull slik at det ikke erstatter nærgeometrien.

Plassering, analytiske stein-colliders, terrengkontakt og begrensede pakkede
bilder eies av runtimearbeidet. Import lager ingen levels, terreng, kollisjon,
HISM-er, historieendringer eller demo-spawners. Visuell aksept i spillet følger
etter native import, ikke fra kildens små forhåndsvisninger.
