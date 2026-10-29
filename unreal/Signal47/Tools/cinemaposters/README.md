# Plaza: to utvendige plakatmotiver

**Status:** Under arbeid, forfattet kilde. **Gjelder:** ART-01, to utvendige TB2-plakatskap, UE 5.8.4/Linux/Vulkan. **Sist kontrollert:** 29. oktober 2026.

Dette er en avgrenset importoppskrift og runtime-modul. Native import, fersk leser, bygg, produksjonsbilder, fysisk Closed-interaksjon, disk/Continue og pakket kontroll er **UNVERIFIED** inntil root har kjørt dem. Ingen Blender-, Unreal-, import-, render- eller byggjobb er kjørt av kildeforfatteren. Standardinstallasjonen og offentlig nedlasting berøres ikke.

## Flater og konverteringsbasis

[surfaces.json](surfaces.json) binder bare følgende originale JPG-er fra [EnvironmentGraphics](../../../ArtSource/EnvironmentGraphics/README.md):

| Motiv | Originalt TB2-senter i Blender, meter | Originalt senter i Three-verdenen, meter | JPG |
| --- | --- | --- | --- |
| The Desert Sky, venstre | `(0.162, -7, 1.51)` | `(7985.162, 10.497393474578857, 2169)` | `01-cinema-desert-sky.jpg`, SHA256 `6b1db45bf4b4f8d5efc466f26f0d7b892da370e3ce3a4538111dffa73187fed7` |
| Friday Matinee, høyre | `(0.162, 7, 1.51)` | `(7985.162, 10.497393474578857, 2155)` | `02-cinema-friday-matinee.jpg`, SHA256 `bf6dcbe8a86c1b4558c60cc1a218944d1107a5cae0849b21ef147603dfb70ed6` |

Begge JPG-ene er 1024×1536, sideforhold 2:3. De to faktiske trykkflatene i [TB2-kilden](../townbuildings/tb2/cinema.py) er **1,13×1,61 m**, med normal +X. GLB-ene har samme mål og fire målte hjørner per plakat. Katalogens historiske 1,13×1,77 m brukes ikke til å endre bygningen. Motivet skrives inn som **1,073333333×1,61 m**, sentrert uten beskjæring eller strekk. Den gamle grafikkens smale sidekanter ligger fortsatt under glasset.

Hvert nytt motiv ligger **1 mm foran** originaltrykket, lokal X0,163. Det eksisterende glasset på X0,25, størrelse 1,16×1,64 m, står 87 mm foran motivet. Gamle rammer, lamper, glass, tekstetiketter, atlas, kildegeometri og materialbindinger endres ikke. De fire ferdige interiørplakatene er utenfor dette omfanget; lovlig utendørs sikt til dem er ikke bevist.

TB2 bruker `Three=(7985+Bx, 8.987393474578857+Bz, 2162-By)`. Den prøvde native-basen er `UE_cm=(100*Bx, -100*By, 100*Bz)`. De to originale plakatplanene er dermed funnet i GLB-skallets navngitte `M_Town_PrintAtlas`-primitiv, atlascellene 11/12. Den nåværende sloten er 12, men integrasjonen slår opp **navn, faktisk materialbinding og metadata**, og bruker ikke 12 som eneste identitet.

Den egne flaten er en geometri-/UV-kopi av `/Engine/BasicShapes/Plane`, ikke en omskriving av Engine-assetet. Bare kopien får CPU-tilgang. Native Plane er 100×100 cm, normal +Z, med målt `u=(x+50)/100`, `v=(y+50)/100`. Runtime-basen legger lokal +X langs UE −Y, lokal +Y langs UE −Z og normalen langs UE +X. Bildets høyre kant er Three −Z og nederste kant Three −Y, med `MapU=(1,0,0,0)` og `MapV=(0,1,0,0)`. Import og levende audit avviser avvik fra denne faktisk målte UV-basen.

## Eierskap, import og fersk leser

`import_assets.py` eier bare fem nye assets under `/Game/Signal47/Art/CinemaPosters`:

- `T_Cinema_DesertSky` og `T_Cinema_FridayMatinee`.
- `MI_Cinema_DesertSky` og `MI_Cinema_FridayMatinee`.
- `SM_CinemaPosterPlane`.

Begge materialinstansene deler eksisterende `/Game/Web/Base/M_Web_Lit_Opaque`. De har egne `Map`-bindinger, hvit Color, ruhet 0,88, vanlig lys, null metall/emisjon og fullflate-UV. JPG-fargene importeres som sRGB/BC7 med mip-kjede og samme kvadratiske POT-policy som den leverte documentprops-løypen. Flatenes 2:3-mål gjenoppretter kildebildets proporsjoner selv om den bygde teksturens plattformformat er kvadratisk. Ingen global PrintAtlas eller ny materialmaster forfattes.

Root kjører begge kommandoene i **hver sin ferske prosess**, gjennom eksisterende `Scripts/unreal_guard.py require-idle` og egen reservasjon. Eksemplene forutsetter prosjektmappen `unreal/Signal47`, korrekt `UE_ROOT` og nye rapportstier utenfor Content:

```sh
S47_CINEMA_MODE=import S47_CINEMA_REPORT=/absolutt/ny/import-r1.json \
  "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PWD/Signal47.uproject" \
  -run=pythonscript -script="$PWD/Tools/cinemaposters/import_assets.py" \
  -unattended -nullrhi -nosplash -NoSound -NoLiveCoding

S47_CINEMA_MODE=verify S47_CINEMA_REPORT=/absolutt/ny/saved-verify-r1.json \
  S47_CINEMA_IMPORT_REPORT=/absolutt/ny/import-r1.json \
  "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PWD/Signal47.uproject" \
  -run=pythonscript -script="$PWD/Tools/cinemaposters/import_assets.py" \
  -unattended -nullrhi -nosplash -NoSound -NoLiveCoding
```

Før import må editorbinæren være bygget med den avgrensede `S47.Cinema.ReadSavedMaterials`-kommandoen i `S47CinemaPosters.cpp`. Root fører `MeshDescription` og `StaticMeshDescription` som editor-only avhengigheter i Build.cs. Ingen ny UCLASS, plugin eller runtime-UI kreves. Kommandoen har to eksplisitte argumenter, eksakt Shell/Glass-meshsti og en fersk absolutt rapportsti utenfor Content. Python kaller den gjennom `SystemLibrary.execute_console_command(None, ...)`; den lokale UE5.8.4-implementasjonen ruter konsollkommandoer direkte også uten World. Manglende/gammel leser avvises før oppretting av assets.

Leseren bruker bare konstante `GetMeshDescription`/`FStaticMeshConstAttributes`-oppslag og leser `PolygonGroup.ImportedMaterialSlotName`, globalt display-/importnavn, faktisk `renderSection.MaterialIndex` og `SectionInfoMap` for hver LOD. Den kaller ingen setter/save og aktiverer ingen runtimeflate. Rårapportene heter eksempelvis `import-r2-materials-shell.json` og `import-r2-materials-glass.json`; de fullføres før Python vurderer bindingene.

Standardmodus er `verify`. Den importerer eller lagrer ingen assets. Første import skriver bare fem eide assets og en bindingskandidat i den private rapporten; aktiv metadata forblir `RUNNING`. Første ferske leser krever den komplette PASS-importrapporten fra en annen prosess, identiske native filhasher og identisk måling. Først da publiseres PASS-binding med `fresh_saved_reader=true` og importrapportens SHA256. Senere verifisering av et allerede akseptert, identisk sett trenger ikke importvariabelen og skriver heller ikke bindingen på nytt.

Eksisterende korrekte assets brukes uten settere/save ved gjentatt `import`. En feil, ukjent eller endret eksisterende fil avvises; skriptet sletter den ikke for å få en grønn prøve. Dersom en tidligere akseptert payload har endret hash, avviser `import` aktiveringen før annet arbeid. Råfiler og rapporter beholdes for roots vurdering.

Aktivering krever `Content/WebData/cinema-posters-binding.json`, versjon 1/PASS, fersklesermarkering og riktig SHA256 av surfaces-kontrakten. Root inkluderer denne i den eksisterende WebData-løypen og cooker eiermappen med fem assets. Ingen konfigurasjon eller cook-hook endres av disse forfatterfilene.

Importen leser **de faktisk lagrede StaticMeshDescription-ene** for kinoens Shell og Glass på alle tre LOD-er. Hvert plakatplan og glass må ha nøyaktig to trekanter med mål/senter fra kilden og korrekt navngitt materialsone. Gruppens faktiske importnavn løses gjennom motorens importnavn-tabell til global slot, og bindes til faktiske renderseksjoner og samme materialasset. Summert antall trekanter for dette materialet må stemme mellom kildegrupper og renderseksjoner. Python leser SectionInfoMap uavhengig ved **faktisk seksjonsindeks**, og rapporterer dette ved siden av renderkartet. Ingen likhet mellom gruppe-ID, kompaktert seksjonsindeks og global slot antas.

UV-hjørner, trekantpayload-hasher, totale LOD-tall, bounds, slotnavn, materialstier, faktisk gruppenavn-/seksjonsbinding og originale uasset-hasher skrives i bindingen. Hele den native materialrapporten bindes med en kanonisk SHA256. Den ferske leseren krever identisk måling og alle fem native filhasher. Det beskyttede førinventaret, forrige binding-hash og de målte gruppe-/navne-/seksjonsdataene skrives i privatrapporten **før** krav evalueres; et avslag beholder dermed faktisk diagnostikk.

Første native import R1 ble kontrollert avvist ved shell/desert-sky/LOD2 etter 35 kontroller, før noen av de fem assets ble opprettet. To riktige trekanter og riktige mål var funnet, men første oppskrift antok feilaktig at gruppe-ID/slot/seksjon var samme indeks. Originalkilder, TB2 og Engine var uendret, og bare en inaktiv RUNNING-binding fantes. Den nye navnebaserte leseren erstatter denne indeksantakelsen; det er ingen endring av kravet om to trekanter, kildeplassering, mål eller alle tre LOD-er. Nytt bygg/import/ferskleserutfall er fortsatt UNVERIFIED.

Alle eksisterende filer i `Content/TownBuildings`, den delte materialmasteren og EnginePlane hashes før/etter. Bindingen beholder et portabelt inventarfingeravtrykk; rapporten beholder hele førinventaret. JPG-ene kontrolleres mot katalogens kildehash og uendret etter import. Ingen beskyttet original lagres. Kildehash, importfilnavn og native materialreadback er ikke en GPU-pikselavlesning av plakatbildet.

## Runtime-API og bevisgrenser

[FS47CinemaPosters](../../Source/Signal47/Public/S47CinemaPosters.h) har følgende integrasjonspunkter for root:

```cpp
Posters = FS47CinemaPosters::Build(Web, TownBuildings.IsValid());
// Eksisterende vertsoppdatering, etter TB2s egen oppdatering:
if (Posters) Posters->Update();
// Målrettet driver kan serialisere hele avlesningen før/etter Continue:
const auto Actual = Posters->Readback();
Posters->Audit(Lines);
```

Ingen egen tick, timer, interaksjon eller lagring opprettes. `Build` krever komplett TB2, begge originale komponenter med riktige mesh/LOD/slot/bounds/transform og begge nye tekstur-/materialbindinger. Hele paret og de faktiske registrerte flatenes mål/basis/forelder valideres før nye noder eksponeres. Ved avslag forblir begge originaltrykk, glass og proxy urørt. `-S47CinemaPostersOff` og eksisterende `-S47PropsOff` beholder originalene. `Update` følger de eksisterende kino-/town-forfedrenes faktiske synlighet og komplett TB2-eierskap.

`Readback()` leser det faktiske komponentmaterialet i slot 0, `Map`, tekstursti/størrelse/sRGB, materialforelder, scalar-/vektorverdier, alle hjørne-UV-er og normaler fra **egen levende CPU-plane**, orienterte trekanter mot den målte lagrede kopien, faktisk transform/basis/mål/offset, forelder, kollisjon/skygge og synlighet. Den fører også originale komponent-/mesh-/material-/LOD-/bounds-/transformdata og proxyidentitet.

De originale TB2-meshene trenger ikke ny CPU-tilgang. Deres to lokale plakatmål og UV-hjørner kommer fra den ferske lagrede native-målingen i bindingen, og omregnes med faktisk runtime-transform. Disse feltene heter derfor `saved_original_*`. Levende originale trekantbuffere hevdes ikke avlest der de ikke er tilgjengelige. Runtime kontrollerer de aktuelle mesh-/slot-/bounds-/transformbindingene; native filhasher er import-/ferskleserbevis, ikke en uasset-hash av den omkodede cook-payloaden.

## Neste målrettede kontroll, eid av root

Produksjonsbilder foreslås som ett oversiktsbilde av den stengte kinoen og ett utenfra hvert skap i dag, skumring og natt. Eksempel på lovlige statiske kamerapunkter: Three `(7988,10.5,2169)` og `(7988,10.5,2155)`, rettet mot de aktuelle trykkflatene. Kamera/FOV/pose og faktisk førsteray gjennom eksisterende glass må følge hvert bilde. Synlige bildetekster og riktig UV-retning trenger gjennomsett PNG; import/audit alene beviser ikke lesbarhet eller glass-sortering.

Den fysiske ruten skal bruke eksisterende offentlig Caliche-adkomst, Walker og original `town:cinema`-proxy 1514 med range 2,6. Begge plakatene sees uten å gå inn. `Use` skal fortsatt gi den eksisterende Closed/toast-responsen. Deretter ordinær isolert disk/title/Continue, samme auditer og ny fysisk Closed-interaksjon, med uendrede quest-/dør-/adgangsflagg. Ingen interiørfixture brukes til å fremstille et interiørmotiv som integrert. Separat samlet Off-prøve skal vise originale trykk/glass/bindinger og samme Closed-interaksjon.

Bibliotekrutinen gav ingen spesifikk prøvd plakatintegrasjon. [scenario-unreal-materials ved festet commit](https://github.com/scenario-labs/skills/blob/f0adb5b5ebc1fc8f7e333554d2aeb767c5558d6b/skills/game-engines/unreal/scenario-unreal-materials/SKILL.md) er MIT og omtaler UE5.8, delte mastere/instanser, sRGB og mip-kjeder. Kildens egen status er kun offline/fake-unreal-test; den brukes som referanse og gir ingen aksept i dette prosjektet. Den konkrete master-/UV-/importløypen bygger på prosjektets leverte [documentprops](../documentprops/README.md), med egne materialinstanser, CPU-plane og strengere original-/ferskleserbinding for disse to flatene.
