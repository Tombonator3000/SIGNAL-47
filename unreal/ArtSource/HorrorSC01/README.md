# SC01: originale vindusressurser

Redigerbar Blender-kilde for blek hånd/underarm, utvendige glasspor og en profilert stållamell. Alt er ny original geometri uten eksterne modeller eller referansebilder. `build_scene.py` bygger og eksporterer kildene deterministisk. `model_contract.json` binder FBX-hasher, materialspor, trekanttall og metriske grenser.

Native import bruker prosjektets eksisterende FBX/PBR-oppskrift. Integrasjonseieren kjører `import_assets.py` i UnrealEditor-Cmd når ingen andre motor-/spilljobber kjører. Bare `/Game/Signal47/HorrorSC01` skrives. Ikke kjør import parallelt med bygg eller tester.

Spillgeometrien ligger ved østre nordrute i kontrollrommet. Glasset står ved z=-4,56; håndroten beveger seg fra x=4,26 til 5,98 utenfor glasset ved z=-4,69 og forsvinner bak eksisterende østre veggpilar. Sporene står fysisk på utsiden ved z=-4,58. Skjermen har 26 solide lameller ved z=-4,34, ikke en usynlig synssperre. Lampen utenfor glasset har egen lavspenningsbryter og kobles ikke til mottaker, K3 eller byforsyning.

Sporene kontrolleres med faktisk sikt fra trygg side og to tilstander på en separat ekte romlampe. Å se gjennom et lesepanel gir ingen observasjon. `FS47HorrorVisual` produserer bare lokale verdensfakta; historiekoordinatoren eier scenetekst og progresjon.

Blender-eksport er utført. Native import, pakket kjøring og pikselkontroll får egne faktiske kvitteringer i SC01-leveransen; kildegeometri alene attesterer ingen spilltest.

## Native LOD-er og RT-minne

Første pakkede kandidat R1 målte 80,7112 MiB AlwaysResident RT-geometri mot uendret grense på 80 MiB. NP1s tidligere referanse målte 79,3446 MiB. R2s separate Nanite-/RT-proxy reduserte tallet til 80,0648 MiB, men R3s videre prosentendring ga ingen ytterligere gevinst. Alle tre forblir minne-FAIL; de tidligere bevisene bevares.

Motorens `RayTracingGeometryManager.cpp` bruker siste faktiske LOD som AlwaysResident med eksisterende innstilling på én slik LOD. Den nye profilen bruker derfor vanlige native LOD-er: full hånd med 19 388 trekanter som LOD0, samt en faktisk redusert LOD1 med mål på 30 prosent. LOD1 har skjermgrense 0,04; hånden ved vinduet beholder full LOD0. Kollisjon og CPU-geometri bruker LOD0. Nanite og separat RT-proxy er av på håndasseten. Ingen global innstilling, minnegrense eller RT-funksjon endres.

Original Blender-kilde, FBX, materialer, mål og kollisjonsgeometri beholdes. Importen måler begge LOD-trekanttallene, kontrollerer nøyaktig opprinnelig LOD0 og fører minne-/pikselaksept separat. Raster-LOD-streaming aktiveres ikke. Eksisterende RT-residensstyring velger selv når full RT-geometri trengs.

Den gjenværende katalogiserte hele Fairheaven-bilen bruker en tilsvarende avgrenset profil gjennom `Signal47/Tools/freemodels/rt_lods.py`. Den krediterte kildebilen og materialene beholdes; dette er ingen ny modell eller lisensendring. Native rapport og faktisk pakket minne avgjør om den samlede profilen består.

`S47_SC01_RT_ONLY=1` oppdaterer bare håndasseten. De to andre meshene og alle fire materialassetene hashkontrolleres før/etter og skal være byteidentiske. Integrasjonseieren kjører følgende når ingen annen Unreal-/spilljobb er aktiv:

```sh
python3 unreal/Signal47/Scripts/unreal_guard.py require-idle
S47_SC01_RT_ONLY=1 \
S47_SC01_IMPORT_REPORT=/home/tom/scratch-bm/Signal47-sc01-20261026/native-import-r4.json \
/home/tom/.local/opt/UnrealEngine-5.8.4/Engine/Binaries/Linux/UnrealEditor-Cmd \
  /home/tom/scratch-bm/wt/horror-h01/unreal/Signal47/Signal47.uproject \
  -run=pythonscript \
  -script=/home/tom/scratch-bm/wt/horror-h01/unreal/ArtSource/HorrorSC01/import_assets.py \
  -unattended -nullrhi -nosplash -NoSound \
  '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0'
```

Denne kilderevisjonen alene hevder ingen native R4-import, minnegevinst eller pakket aksept.
