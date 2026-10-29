# Caliche Homes, nativeimport og lagret verifikasjon

Kjør disse inngangene eksplisitt i prosjektets eksisterende UE5.8.4-editor
gjennom `Scripts/unreal_guard.py require-idle` og hovedagentens reserverte
jobb. Verktøyene starter ingen prosess. Ingen Blender, cook eller spillpakke
kjøres fra disse Python-filene. Ikke kjør dem samtidig med andre tunge jobber.

Obligatoriske miljøvariabler for begge innganger:

```text
S47_HOMES_SOURCE=/ABS/CalicheHomes
S47_HOMES_TRIMS=/ABS/ENV02-TRIMS
S47_HOMES_REPORT=/ABS/BEVIS/native-import.json
```

SOURCE inneholder kanonisk `manifest.json`, byteidentisk `model_contract.json`,
`source-contract.json`, `materials.json` og de faktiske 36 authored FBX-filene.
TRIMS er resultatet fra `calichehomes_integration/prepare_trims.py` og den
separate Blender-adapteren, med `trim-manifest.json`, `trim-fbx.json`, to GLB-er
og sju faktiske FBX-er. Bevisrapporten må ligge utenfor kilde- og assetmappene.

Kjør først `import_assets.py`. Forvent `kind=native-import`, `status=PASS`
og stdout-markøren `HOMES_NATIVE_IMPORT_PASS`. Deretter avsluttes denne
editorprosessen. Kjør `verify_assets.py` i en **ny** editorprosess, med ny
REPORT-bane. Forvent `kind=saved-native-read-only`, `status=PASS` og
`HOMES_SAVED_NATIVE_PASS`. En kontrollert exception skriver FAIL med faktisk
traceback. Import-PASS er ikke en fersk lagret verifikasjon.

## Det avgrensede settet

Bare følgende egne assets importeres eller oppdateres:

- 12 husdeler, Shell/Roof/Fixtures for H01 til H04, med tre authored LOD-er.
- Fem ikke-tomme kuttmesh under `/Game/CalicheHomes/Trim`, én LOD hver.
- To kopier av de opprinnelige skiltene, én LOD og tre materialplasser hver.
- 15 `MI_Homes_*`-instanser under `/Game/CalicheHomes/Materials`.

Tre helt tomme kuttforgjengere skal skjules av runtime, og importeres ikke.
Ingen PBR-teksturer, nye Town-grafer eller native proxy-LOD-er opprettes.
TB2 beholder sine 28 deler. Husene beholder Nanite av og RT-støtte på.
Kollisjon bruker komplekse synsstråler; analytisk bevegelseskollisjon eies
fortsatt av eksisterende runtime. Skilt-/trimslotnavn beholdes i nativeassetet
og bindes midlertidig til Engine DefaultMaterial. Runtime setter de faktisk
opprinnelige materialpekerne etter sin samlede preflight.

Importøren gjenbruker prosjektets prøvde `motelv2/native.py::import_mesh` med
ForceFrontX, MATCH_UP_FORWARD_AXES og yaw90. Faktisk FBX-geometri kontrolleres
før import. Hele den lagrede geometriens vertexsett sammenlignes deretter
bidireksjonelt mot hver faktisk FBX, med toleranse **0,025 cm**, ikke bare AABB
eller trekanttall. Forventet UE er Blender `(100x,-100y,100z)`. Begge retninger
er nødvendige for å oppdage både ekstra og manglende kildepunkter.

Bygginnstillingene beholder kilde-normaler og full UV-presisjon uten å fjerne
tynne triangler. Tangenter beregnes med MikkTSpace fra de importerte normalene
og UV-ene. Faktiske render-/MeshDescription-trekanttall, antall LOD-er,
materialnavn og bindinger, screen sizes, aksene og alle query-seksjoner må
bestå også ved fersk verifikasjon. Denne posisjonskontrollen er ikke en
selvstendig verifikasjon av hver native UV-/normalhjørneverdi; FBX-adapteren
kontrollerer de faktiske eksporterte verdiene, og native bygginnstillingene
registrerer bevaringspolicyen. Visuell runtimekontroll gjenstår.

## Materialinstanser og uendrede avhengigheter

Husenes FBX-plasser heter `M_Homes_*`, men de lagrede bindingene er
`MI_Homes_*`. Hver instans peker på den avtalte eksisterende `M_Town_*`-forelderen.
Foreldregrafens faktiske aktive tilkoblinger, eksisterende teksturer, kildebytes,
fargerom, DirectX-normalformat, UV0 og normalstyrkegraf undersøkes uten endring.
Normalstyrken går gjennom den eksisterende neutral-normal-blandingen og
Normalize. Ingen normal-/UV-graf bygges på nytt.

Teksturerte Town-foreldre bruker kartet direkte til roughness og
tekstur×TextureTint til BaseColor. Derfor overstyres TextureTint, Metallic,
Specular og NormalStrength på disse instansene. Roughness-verdien arves fra
teksturkartet. På flate foreldre overstyres Color og scalar Roughness.
Frakoblede Color/Roughness-noder regnes ikke som et funksjonelt bevis.

Den lokale UE5.8.4-kilden `MaterialEditingLibrary.cpp`, rundt linje 1479 og
1573, setter scalar-/vektorverdier og oppdaterer instansen, men returnerer
`bResult=false` uten å endre variabelen. Returverdien brukes derfor ikke som
aksept. Verktøyet avleser den faktiske verdien umiddelbart og igjen ved fersk
lagret verifikasjon. Bare de ventede scalar-/vektoroverstyringene er tillatt;
teksturoverstyringer skal være tomme.

Rapportfeltene binder det faktiske resultatet:

- `source_sha256`: kanonisk manifest, materialmanifest, kildekontrakt og trimmanifest.
- `source_files_sha256`: alle aktuelle FBX-/GLB-/metadata-/PBR-kildefiler.
- `native_files_sha256`: alle 19 mesh-uassets og alle 15 MI-uassets, 34 totalt.
- `town_dependency_files_sha256`: separat inventar/hash av eksisterende
  TownBuildings-nativefiler og JSON, med eksakt før/etter-kontroll.
- `parent_materials`: faktisk forelder, teksturassets og arvet grafpolicy.
- `meshes`: faktiske slotnavn, materialbindingsbaner, alle lagrede LOD-tall/
  bounds, vertexhashes og deres faktiske FBX-hashes.

Importen stopper før første mutasjon hvis kildesettet eller foreldrene ikke
stemmer. En senere importfeil kan etterlate egne delvis importerte Homes-assets,
men gir ingen assembly og ingen runtimeaktivering. Eksisterende Town-bytes og
inputkilder kontrolleres igjen før PASS. Ved feil registreres også faktisk
Town-inventar etter avbruddet. Den ferske verifieren bruker ingen import-/save-/
setterfunksjon og krever identiske Homes- og Town-filer før og etter lesingen.

## Aktivering og bevisgrenser

Staging i `calichehomes_integration/stage_assembly.py` er et separat steg.
Begge faktiske rapporter, nativehashes og Town-avhengighetsinventaret må være
identiske og fortsatt matche disk før aktivering. Rapporten skriver aldri
assembly eller endrer gamle hus-/skiltmesh og kollisjoner.

Lett Python-/AST-/FBX-parserkontroll beviser ikke editor-API, nativeimport,
lagring, materialgjengivelse eller spillet. Disse er UNVERIFIED til de to
faktiske editorjobbene har bestått. Hovedagenten eier dem, guard/kø,
pakkeinnstillinger, runtimefallback, hel innkjøring, fasade-/skiltbilder,
proxy-/Continue-kontroll og levering. Ingen fullmatrise eller installasjon
er gjennomført av denne native-importøren.
