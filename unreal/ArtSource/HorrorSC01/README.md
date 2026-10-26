# SC01: originale vindusressurser

Redigerbar Blender-kilde for blek hånd/underarm, utvendige glasspor og en profilert stållamell. Alt er ny original geometri uten eksterne modeller eller referansebilder. `build_scene.py` bygger og eksporterer kildene deterministisk. `model_contract.json` binder FBX-hasher, materialspor, trekanttall og metriske grenser.

Native import bruker prosjektets eksisterende FBX/PBR-oppskrift. Integrasjonseieren kjører `import_assets.py` i UnrealEditor-Cmd når ingen andre motor-/spilljobber kjører. Bare `/Game/Signal47/HorrorSC01` skrives. Ikke kjør import parallelt med bygg eller tester.

Spillgeometrien ligger ved østre nordrute i kontrollrommet. Glasset står ved z=-4,56; håndroten beveger seg fra x=4,26 til 5,98 utenfor glasset ved z=-4,69 og forsvinner bak eksisterende østre veggpilar. Sporene står fysisk på utsiden ved z=-4,58. Skjermen har 26 solide lameller ved z=-4,34, ikke en usynlig synssperre. Lampen utenfor glasset har egen lavspenningsbryter og kobles ikke til mottaker, K3 eller byforsyning.

Sporene kontrolleres med faktisk sikt fra trygg side og to tilstander på en separat ekte romlampe. Å se gjennom et lesepanel gir ingen observasjon. `FS47HorrorVisual` produserer bare lokale verdensfakta; historiekoordinatoren eier scenetekst og progresjon.

Blender-eksport er utført. Native import, pakket kjøring og pikselkontroll får egne faktiske kvitteringer i SC01-leveransen; kildegeometri alene attesterer ingen spilltest.

## Lokal RT-proxy for hånden

Første pakkede kandidat r1 målte 80,7112 MiB AlwaysResident RT-geometri mot uendret grense på 80 MiB. NP1 målte 79,3446 MiB. Importskriptet har derfor en avgrenset native profil bare for hånden. Original Blender-kilde, FBX, materialer og synlig modell beholdes. Rasterens fallback og den komplekse kollisjonen bruker hele LOD0 med 19 388 trekanter; koden for observasjon og kollisjon endres ikke.

Pakket r2 med 25 prosent separat RT-proxy målte deretter stabilt 80,0648 MiB i 389 aktive samples. Det er 0,6464 MiB mindre enn r1, men fortsatt 0,0648 MiB over den samme grensen. R2 er derfor **FAIL for RT-minne**; rapporten `rt-memory-r2.json` er bevart i SC01-bevisroten. Ny r3-kilde setter den separate RT-proxyen til 10 prosent for mer margin. Ingen r3-minnegevinst eller kvalitetsaksept er ennå målt.

UE 5.8.4 har ikke `ray_tracing_min_lod` på StaticMesh. Denne egenskapen tilhører SkeletalMesh. `FMeshRayTracingProxySettings` gir i stedet en egen redusert RT-representasjon. Motorens `StaticMeshBuilder.cpp` bygger denne bare når håndasseten har Nanite aktivert. Importen beholder alle opprinnelige trekanter og en uredusert renderfallback, og setter bare den separate RT-proxyen til 10 prosent med prosentmålet eksplisitt valgt. Prosjektets eksisterende `r.Nanite.ProjectEnabled=True` støtter denne byggeløypa. Ingen global RT-/Nanite-innstilling eller minnegrense endres.

Ved import rapporteres faktisk render-LOD0, materialspor, mål, kollisjons-LOD og native proxyinnstillinger. RT-fraksjonens faktiske float32-verdi føres ved siden av målet 0,10 og kontrolleres med toleranse under 0,000001. Antallet trekanter i den dedikerte RT-proxyen er ikke eksponert i editorens Python-API; omtrent 1 939 er et mål, ikke et målt resultat. Faktisk RT-bruk, minne og High/Low-piksler må verifiseres i den nye pakkede kandidaten før profilen får aksept.

Integrasjonseieren kan oppdatere bare håndasseten med `S47_SC01_RT_ONLY=1`. Denne modusen laster eksisterende materialer uten omskriving og kontrollerer SHA256 før/etter for de to andre meshene og alle materialassetene. Originalgeometrien hashkontrolleres også. Kjør bare når andre Unreal-/spilljobber er avsluttet, og bruk ny rapportfil for å bevare r1- og r2-bevisene:

```sh
python3 unreal/Signal47/Scripts/unreal_guard.py require-idle
S47_SC01_RT_ONLY=1 \
S47_SC01_IMPORT_REPORT=/home/tom/scratch-bm/Signal47-sc01-20261026/native-import-r3.json \
/home/tom/.local/opt/UnrealEngine-5.8.4/Engine/Binaries/Linux/UnrealEditor-Cmd \
  /home/tom/scratch-bm/wt/horror-h01/unreal/Signal47/Signal47.uproject \
  -run=pythonscript \
  -script=/home/tom/scratch-bm/wt/horror-h01/unreal/ArtSource/HorrorSC01/import_assets.py \
  -unattended -nullrhi -nosplash -NoSound \
  '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0' \
  > /home/tom/scratch-bm/Signal47-sc01-20261026/native-import-r3.log 2>&1
```

Dette kildearbeidet hevder ingen utført r3-import, minnegevinst eller pakket aksept. Skriptet kan kjøres på nytt med samme kilder og profil; det oppretter ingen vanlig redusert render-LOD og beholder full kollisjonsgeometri.
