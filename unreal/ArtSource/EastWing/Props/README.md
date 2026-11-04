# EASTWING-01: teksturer og rekvisitter

Codex-leveranse til Claudes østfløy. Ingen bygning, verdensplassering, S47EastWing eller S47FreeformWorld endres her. Romprogrammet følger `origin/claude/east-wing-20261103:Docs/Design/EastWing/EAST_WING_PLAN.md` og seks konseptbilder: generell testlab, delelager, kalibrering/reparasjon og garasje med port i sørfasaden.

## Innhold

- `Lab/`: åtte egne utstyrsmodeller med FBX LOD0/1/2, redigerbar Blender-samling, materialparametre, lisensmanifest og preview. Mikroskop, sentrifuge, laboratorievekt, prøveglass, kolbe, kjøleskap, multimeter og loddebolt.
- Oscilloskopet gjenbruker Thuskos RM565 (CC-BY-4.0). Lisens, kilde og privat leveransesti står i `Lab/SM_EW_Oscilloscope_RM565.license.json`. FBX, Blender-fil og 2K-teksturer ligger i `/home/tom/Assets/signal47-private/EastWing/Props/Lab/`. Kildearkivet er uendret. Disse binærfilene skal ikke inn i Git.
- `Workshop/`: 17 egne/gjenbrukte prosjektmodeller: skuffeskap, rød verktøykasse, grønn stålbenk, delevogn, FUSES-kasse, RELAYS-kasse, kabeltrommel, låst kjemikalieskap, dekkstabel, brannslukker, pegboard, veggverktøy, portsegment, slukrist, lysarmatur, fylt hylle og tom galvanisert hylle. Original V10-armatur og ArchiveProps-hylle gjenbrukes; øvrige opprinnelser står per asset.
- `Textures/`: seks overflater og to separate detaljmaterialer i 2048 x 2048. BaseColor, Roughness, Metallic, NormalDX, NormalGL, 16-bit Height; gulstripe har egen opasitet. Fysisk flisestørrelse og fargerom står i `materials.json`. Gjenbrukte arkivhylleteksturer har opprinnelig oppløsning og holdes atskilt i `Workshop/ReusedTextures/`.

## Importkontrakt

Modellene har lokale koordinater i meter, Blender Z opp, front -Y. Prosjektets målte FBX-oppskrift er brukt: `FBX_SCALE_UNITS`, forward -Y og up Z. Ikke legg inn absolutt verdensplassering i importen. Mål minst én kjent kant i Unreal før plassering, for eksempel benkens 1,8 m bredde eller portsegmentets 3,5 m bredde. Per-asset bounds og monteringsdatum står i `*.asset.json`. Fylt hylle er normalisert til samme frontakse. Portsegmentet har bunnsentrum som pivot; lysarmaturets nedre rørflate er Z=0 og festeflaten er omtrent Z=0,10 m. Slukristen er 0,5 x 0,3 m og plasseres med toppen i gulvplanet.

`*.blend` er redigerbare samlingsbiblioteker. Append samlingen `<asset>_EDITABLE` i Blender. Originale delobjekter og modifikatorer er beholdt. FBX-eksporten er en evaluert, triangulert sammenslåing med separate materialslotter. Materialparametre og filavhengigheter følger assetmanifestet. Delte flismaterialer har metrisk planar UV-skala på eksporten; importerte modeller beholder sine opprinnelige UV-er. UV0 er tekstur-UV, ikke unik lysmappe. Generer separat lysmappekanal ved behov.

LOD1/2 er geometrisk reduserte versjoner med samme koordinatramme. Velg skjermgrenser i integrasjonen. UCX er en grov omsluttende boks, ikke endelig gameplay-kollisjon. Deaktiver den på små instrumenter og innredningsdetaljer der benk/rom allerede har kollisjon; bruk passende kollisjon på åpne hyller og bevegelig port. Lysarmaturens mesh inneholder ingen aktiv lyskilde. Emissivitet, lys, strømtilstand, porter og interaksjoner kobles av Claude.

Importer BaseColor som sRGB, øvrige PBR-kanaler som lineære data; NormalDX brukes i Unreal. NormalGL er for Blender. Høydekart aktiverer ikke automatisk displacement. Prøveglass/kolbe har egne glasslotter; opprett native glassmateriale med angitt IOR/transmisjon. FBX alene gjenskaper ikke hele Blender-materialgrafen.

Gulv: bruk EW_Concrete over gulvet. Plasser EW_YellowLine som separate 10 cm brede striper etter faktisk Jeep-klaring. Ikke flis striper eller sluk over hele gulvet. Bruk fysisk SM_EW_DrainGrate; EW_DrainGrate er et separat kart for innfelt detalj/LOD, ikke en ekstra rist ovenpå modellen.

## Kjøring og kontroll

Alle Blender-jobber kjøres sekvensielt med `run_guarded.py`, som tar global `production_lease` og krever `require-idle PASS`. Eksempel fra repoets rot:

```sh
python3 unreal/ArtSource/EastWing/Props/run_guarded.py unreal/ArtSource/EastWing/Props/Lab/build_lab.py
python3 unreal/ArtSource/EastWing/Props/run_guarded.py unreal/ArtSource/EastWing/Props/Workshop/build_workshop.py
python3 unreal/ArtSource/EastWing/Props/run_guarded.py unreal/ArtSource/EastWing/Props/validate_fbx.py
```

`fbx-validation.json` dokumenterer fersk gjenimport av alle leverte FBX/LOD-er, dimensjoner, UV, endelige koordinater og materialslotter. `Textures/validation.json` dokumenterer 2K-kanaler og normalvektorer. Previewbildene er Blender-studiobilder, ikke Unreal-resultater. Native materialer, plassering, lys, gameplay og ytelse kontrolleres i Claudes integrasjon.

Prosjektbibliotekets AnyScene-profil er lest som metodegrunnlag for assetmanifest og PBR-kontroll, men er UNVERIFIED for Unreal. Ingen AnyScene-kode eller tredjeparts eksempelassets er brukt. Prosjektets egne PhotoLab-overflatefelt og eksportkonvensjon er konkret gjenbruksgrunnlag.


## Native import til Unreal

`python3 unreal/ArtSource/EastWing/Props/import_unreal.py` kontrollerer kildefilene uten å starte motoren. Importøren gjenbruker prosjektets målte FBX-/PBR-oppskrift i `Signal47/Tools/fieldintegration/import_recipe.py`. Native kjøring tas i samordnet motorvindu:

```sh
python3 unreal/ArtSource/EastWing/Props/run_unreal_import.py --apply --output /absolutt/privat/importmappe
```

Wrapperen tar `production_lease`, krever ledig motor og beholder minst100GB diskreserve med10GB importmargin. Bruk en ny resultatmappe per forsøk. Kjøringen lager bare native assets i `/Game/EastWing/Props` og privat RM565 i `/Game/Licensed/EastWing/Props`. Eksisterende assets uten importørens eierskapsmerke overskrives ikke. `S47_EASTWING_PRIVATE_ROOT` kan flytte den private kildeplasseringen. Importerte RM565-filer skal fortsatt holdes utenfor offentlig Git.

Tre authored LOD-er, materialslotter, målte akser/bounds, native glass, gulstripe-opasitet og RM565s pakkede metall/roughness-kanaler inngår. Høydekartene beholdes som kilder; automatisk displacement aktiveres ikke. Rapportstatus `PASS_NATIVE_IMPORT` gjelder faktisk importkontroll, ikke plassering, lys, gameplay eller visuell aksept i bygget. Importøren plasserer ingen aktører. 3. november 2026: native import av alle26modeller/78LOD-er og åtte materialsett består. Målt geometri, materialbinding og akser er kontrollert; plassering og samlet visuelt resultat behandles separat.

Unreal forkaster trekanter med identiske hjørneposisjoner også når `remove_degenerates` er av. `native-triangle-counts.json` fører kildehash, rå FBX-antall og forventet renderantall per LOD. Beregn med `python3 unreal/ArtSource/EastWing/Props/native_triangle_counts.py` etter endrede FBX-filer. Skriptet bruker Blenders medfølgende, frittstående FBX-parser uten å starte Blender eller Unreal; `S47_FBX_PARSER_DIR` kan angi parsermappen. Modellkoblinger identifiserer UCX; de rå `Vertices`/`PolygonVertexIndex`-feltene telles etter målt rotasjon, centimeterskala og float32. Ukjente transformkontrakter avvises.

Alle78 rå kildeantall må stemme nøyaktig med authored manifest og kildehash.4896 nullkanttrekanter og39 ytterligere nullarealtrekanter trekkes fra. FBX-importen forkaster null/ugyldig float32-arealnormal med terskel0 før MeshDescription (`FbxStaticMeshImport.cpp:853–886`); renderbyggeren forkaster også identiske hjørner, som allerede inngår i nullarealsettet for disse kildene. De to årsakene føres separat, uten dobbelttelling. Blenders `mesh.validate()` fjerner i tillegg89 dobbeltflater, også ved motsatt winding, som Unreal beholder; dette avviket telles separat og må forklare hele forskjellen mot den tidligere FBX-gjenimportkontrollen. Eksempel: kolbens LOD2 har554 rå trekanter,4 dobbeltflater og20 nullkanttrekanter; bpy rapporterer550, mens forventet Unreal-renderantall er534. Ingen toleranse eller vilkårlig slakking av importkontrollen brukes.

## Extras (Claude, 4. november)

`Extras/` fyller hullene fra første leveranse: laboratorievask (`SM_EW_LabSink`), glasskap med reagenser (`SM_EW_GlassCabinet`), to sikkerhetsplakater (`SM_EW_PosterRF`, `SM_EW_PosterGlasses`), veggkalender november 1986 (`SM_EW_Calendar1986`), veggur (`SM_EW_WallClock`) og førstehjelpsskap (`SM_EW_FirstAidBox`). Ute: takkjøler (`SM_EW_SwampCooler`) og slangetrommel (`SM_EW_HoseReel`). Garasjen: mekanikerbenk med skrustikke (`SM_EW_MechanicBench`), luftkompressor (`SM_EW_AirCompressor`), garasjejekk (`SM_EW_FloorJack`), bensinkanner (`SM_EW_JerryCans`) og oljeflekker (`SM_EW_OilStains`, flate polygoner uten alfa). Alt er egen geometri og egne trykk, uten tredjepartskilder. Merking: fire dørskilt (`SM_EW_Plaque*`) ved hver innerdør og en oppslagstavle i laben (`SM_EW_NoticeBoard`). Lappene på tavla forklarer spilleregler som prøvekjede, drivstoff og romfunksjoner. `electrical_service()` bygges ikke, fordi østveggen allerede har måler og bryter.

- `Extras/make_prints.py` (vanlig Python med Pillow) lager trykkene i `Extras/textures/`. De lagres i potens-av-to-størrelse; mesh-UV gir riktig format.
- `Extras/build_extras.py` bruker `prop_common.py` og samme eksportløype (LOD0/1/2, UCX, asset/license-JSON).
- Eget leveransesett: `S47_EASTWING_SET=extras` gjør at `validate_fbx.py`, `native_triangle_counts.py` og `import_unreal.py` bruker `delivery-extras.json`, `fbx-validation-extras.json` og `native-triangle-counts-extras.json`. Importen legger bare `Extras` i `/Game/EastWing/Props/Extras` og rører verken de 26 første modellene eller overflatematerialene.

```sh
export S47_EASTWING_SET=extras
python3 unreal/ArtSource/EastWing/Props/run_guarded.py unreal/ArtSource/EastWing/Props/Extras/build_extras.py
python3 unreal/ArtSource/EastWing/Props/run_guarded.py unreal/ArtSource/EastWing/Props/validate_fbx.py
(cd unreal/ArtSource/EastWing/Props && python3 native_triangle_counts.py)
python3 unreal/ArtSource/EastWing/Props/run_unreal_import.py --apply --output <ny privat mappe>
```

Plassering står i `S47EastWingProps.cpp` (`Layout`). Veggting har baksiden mot innerveggen.

### Prøveholder og fysiske arbeidsskilt, Codex 4. november

`Extras/build_sample_station.py` og `Extras/make_sample_station_prints.py` lager en 15 × 12 × 4,5 cm prøveholder og fire 42 cm brede bordskilt. Geometri og trykk er egne prosjektkilder; lisens står ved hver modell. Eksisterende delte stål-/lakkmaterialer gjenbrukes. Prøveholderen har senterpivot i kilden; runtime `Model()` normaliserer bunnen til den faktiske benkeplaten og beholder beholdningsstyrt synlighet.

Uten filter genereres `SM_EW_SampleHolder` og `SM_EW_DepotCheckoutSign`. Sett `S47_SAMPLE_STATION_ONLY=SM_EW_CalibrationSign,SM_EW_SampleAnalysisSign,SM_EW_ReceivingSign` i begge kommandoene for de tre øvrige skiltene. Kjør trykkskriptet med Python og byggeskriptet gjennom `run_guarded.py`. Deretter oppdateres `delivery-extras.json` fra alle Extras-manifester og den eksisterende extras-validerings-/importløypa brukes. Hele Extras-settet har nå 27 modeller / 81 FBX-LOD-er; native import består. Visuell runtime-kontroll føres i oppgaveloggen.
