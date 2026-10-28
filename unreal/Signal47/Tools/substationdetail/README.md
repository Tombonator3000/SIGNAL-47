# Caliche, lampefester og bakkekontakt

Status 28. oktober 2026: Blender-kilde og eksport, 76 kildekontroller, native import, editorbygg og Linux-pakke består. Begge Blender-forhåndsvisningene og pakkede bilder av oversikten og svak/sterk inspeksjonslampe er vurdert. T01 og T02 består. Den ene fullmatrisen med 110 scenarioer kjører; fletting og installasjon er ikke utført for dette passet.

Første kildekontroll stoppet på en lokal UV-variabel som overskrev bounds i rapportskriptet. Variabelen er rettet, og kontrollen ble kjørt på nytt med uendret geometri. Den faktiske kildekvitteringen er `verification.json`; råforsøk og runtimebevis ligger i `/home/tom/scratch-bm/Signal47-substation-environment-20261028`.

Fem originale tillegg følger eksisterende stasjon. Statuslinse, servicehåndtak, journal, skap og rampe beholdes. Kitet introduserer ingen ny elektrisk tilstand eller ekstra lyskilde. Native integrator bruker skulderens eksporterte topptriangler som ganghøyde og Visibility-gulv, uten nye veggblokkere.

| Del, prefiks `SM_SubstationDetail_` | Innhold | LOD0 / LOD1 / LOD2 |
|---|---|---:|
| GroundShoulder | Uregelmessig grusskulder mot målt terreng | 212 / 108 / 66 |
| InspectionFittings | Åpen lampebezel, regnhette og festeører | 388 / 196 / 164 |
| ServiceConduit | Sidekoblingsboks, conduit, koblinger og klammer | 532 / 388 / 316 |
| FoundationContact | Plintankre, dryppdetalj og sparsom kantgrus | 1284 / 732 / 428 |
| GravelContact | Uregelmessige små steinklynger innenfor grusbedet | 3990 / 1920 / 780 |
| Totalt | | 6406 / 3344 / 1754 |

`design.py` bruker lokale Blender B-meter, root Three `[7890,-1.10,536.20]`, yaw 180. Alle deloffsets er null. Native mapping før root er B `(x,y,z)` til UE `(100x,-100y,100z)`, samme målte FBX-oppskrift som Substation. `model_contract.json` inneholder faktisk geometri, bounds og materialer. Importøren skriver bare `/Game/SubstationDetail`, med tre modellerte LOD, Nanite av og NoCollision som standard. Bare GroundShoulder får kompleksgeometri tilgjengelig for runtime QueryOnly/Visibility-gulv. De andre delene beholdes NoCollision. `shoulder_floor.json` og `model_contract.json/shoulder_floor_triangles_b` inneholder de eksakte triangulerte LOD0-toppflatene fra Blender-objektet.

Lampens originale linse ligger i B `[4.62,-8.925,1.70]`. Native emitter flyttes foran glasset, til B `[4.62,-8.974,1.70]`. Kildesjekken sender fem faktiske stråler fra dette punktet gjennom ny geometri i alle tre eksport-LOD. Åpningen har ingen opak skive. Runtime eier retning, skygger og eksisterende svak/sterk-/service-/T02-tilstand.

Grusklumpene bruker det opprinnelige bedet `[-10,10] × [-8,8]` ved Bz −0.08, plinten ved Bz 0 og rampens opprinnelige helning. Skulderen går fra Bz −0.075 til faktisk barysentrisk terrenghøyde +0.006 m, med varierende bredde. `terrain_contact.json` inneholder de målte originale g873-trekantene og kildehashene. Port, rampe, T02-referanse og kulvert er utelatt fra skulderen. Ingen ny generell terrenghøyde eller ny terrengflate erstatter originalen.

## Kjøring

Kjør sekvensielt gjennom hovedagentens `unreal_guard.run_owned`-wrapper. Kommandoene nedenfor gjengir den lokale løypen; bruk nye loggnavn ved en bevisst ny revisjon, og ikke start dem mens fullmatrisen eller en annen tung jobb kjører.

```sh
project_dir=/home/tom/scratch-bm/wt/motel-v2-unreal/unreal/Signal47
proof_dir=/home/tom/scratch-bm/Signal47-substation-environment-20261028
blender_bin=/home/tom/.local/opt/blender-5.2.2-linux-x64/blender
editor_bin=/home/tom/.local/opt/UnrealEngine-5.8.4/Engine/Binaries/Linux/UnrealEditor-Cmd

python3 "$proof_dir/run_job.py" blender-r1 "$blender_bin" -b -t 2 --python-exit-code 1 --python "$project_dir/Tools/substationdetail/build_detail.py"
python3 "$project_dir/Tools/substationdetail/generate_runtime.py"
python3 "$project_dir/Tools/substationdetail/generate_runtime.py" --check
python3 "$proof_dir/run_job.py" source-verify-r2 "$blender_bin" -b -t 2 --python-exit-code 1 --python "$project_dir/Tools/substationdetail/verify_source.py"
python3 "$proof_dir/run_job.py" preview-r1 "$blender_bin" -b -t 2 --python-exit-code 1 --python "$project_dir/Tools/substationdetail/preview.py"

export S47_SUBSTATIONDETAIL_REPORT="$proof_dir/native-import-r1.json"
python3 "$proof_dir/run_job.py" native-import-r1 "$editor_bin" "$project_dir/Signal47.uproject" -run=pythonscript "-script=$project_dir/Tools/substationdetail/import_assets.py" -unattended -nullrhi -nosplash -NoSound '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0'
```

Forhåndsvisningen henter de eksisterende originale Blender-kitene som kontekst og legger en eksplisitt studioflate under. Disse bildene beviser modellutseende; de erstatter ikke spillbilder av terrengkontakt, lys eller interaksjoner. Den redigerbare detaljkilden er `Substation_Detail.blend`; previewkonteksten eksporteres ikke som nye native deler.

## Kilder

Alle nye former er originale. `../substation/meshkit.py` og fire egne materialfamilier, OlivePaint, GalvanizedSteel, DarkSteel og Gravel, gjenbrukes uendret via relative stier. Faktiske PBR-bildefiler pakkes i den nye Blend-filen. Ingen kjøpte modeller eller lisensierte tredjepartsassets legges til.

Prosjektbibliotekets Blender–Unreal-profil er UNVERIFIED og brukes bare som arbeidsflytreferanse. [arjun988/blender-skills](https://github.com/arjun988/blender-skills/tree/8f778d2405a214b508d4c7d80742be8e43acdd52), MIT, brukes som metodeveiledning for synlige begrunnede lyskilder, ryddige klynger og målt engine-eksport. Ingen kode derfra eller MCP-installasjon er brukt. Den eksisterende målte akseløypen beholdes.
