# Hereford-kyr og den niende kua

Bestilt av Tom 25. oktober 2026: konseptbilder av levende ku og død ku etter bortføring, Blender-kilder og integrasjon i Unreal-spillet.

## Kilder og produksjon

Konseptene ligger i [ArtSource/Cows/Concepts](ArtSource/Cows/Concepts). `ku-levende.png` og `ku-dod-endelig.png` er de valgte referansene. Det tidligere dødkukonseptet er utforsking; sluttmodellen følger spillets eksisterende Hereford, frost, blodløse runde flankesnitt, bare kjevefelt og ALDERETE 25. Alle bildene er laget med innebygd image_gen; fullstendige prompt ligger i `Concepts/prompts.json`.

Magnifics bilde- og tekstbaserte 3D-generering feilet. Ingen ekstern mesh er brukt. Modellene er formet direkte med bpy i Blender 5.2.2 LTS. [build_cows.py](ArtSource/Cows/build_cows.py) bygger originalgeometrien, PBR-teksturene, tre LOD-er, FBX/glTF og den redigerbare [Signal47_Cows.blend](ArtSource/Cows/Signal47_Cows.blend). Levende og død modell har egne collections, kameraer og pakkede teksturer.

Biblioteket ble søkt før verktøyvalg. `SIGNAL-47:blender-mcp` og prosjektets `Tools/fieldintegration/import_recipe.py` er brukt for Blender-verifikasjon og den allerede målte FBX/PBR-importen. Scenario-3D-treffet var bare en generell referanse, uten en tilkoblet Scenario-tjeneste eller kodegjenbruk. Målplattformen er Linux og Unreal 5.8.4.

## Integrasjon

De native modellene ligger under `/Game/Signal47/Art/Cows`:

- `SM_Cow_Alive_Body`
- `SM_Cow_Alive_Head`
- `SM_Cow_Abducted`

Kropp og hode erstatter geometrien i begge opprinnelige instansflokker. Beiting, 026-retning, øyerefleks, nummeroppslag, manglende niende ku, kollisjon og lagring fortsetter gjennom eksisterende spillogikk. Det delte levende hode-meshet har blankt gult øremerke, slik at ikke alle ni får nummer 25. Dødmodellen bruker det konkrete merket ALDERETE 25.

På funnstedet byttes bare gammel kropp og gamle snittmarkører. Frost, fotomål, undersøkelsesområder og funntilstand beholdes. Alle tre native mesher valideres samlet før bytte; `-S47CowsOff` viser gammel geometri for sammenligning.

Kilder bruker meter, Blender Z opp og +Y frem. Den målte FBX-importen gir Unreal `(100*x, -100*y, 100*z)`. Hodepivot og øyepunkter følger eksisterende bevegelsesankre.

## Verifikasjon og levering

Arbeidsbevis og isolert kandidat ligger i `/home/tom/scratch-bm/Signal47-cows-20261025`.

Kilder og import er ferdige. Modellene er myke, stiliserte spillmodeller, enklere enn de realistiske konseptreferansene.

| Modell | LOD0 | LOD1 | LOD2 |
| --- | ---: | ---: | ---: |
| Levende kropp | 17 924 | 10 036 | 5 376 |
| Levende hode | 6 808 | 3 828 | 2 050 |
| Død ku | 28 217 | 15 804 | 8 466 |

Fersk gjenåpning av Blender-kilden består 133 kontroller, inkludert pakkede bilder, bakkekontakt, hodepivot, materialer, UV og eksplisitt rensing av mikroskopiske flater. Første native import avdekket ti forkastede trekantflater i øremerkets bevel. `mesh_cleanup.py` og `geometry_audit.json` dokumenterer rensing av 30 ustabile hodeflater og sju tilsvarende flater i dødmodellen. Den strenge tellekontrollen er beholdt. Sluttfilens SHA256 er `fbfaa3349f041e3a43e53710087c0650e2c46ce1fad4afc162234ab89566ab7a`.

Native import består 307 kontroller og tre mesher; en ny Unreal-prosess består 130 kontroller av de lagrede assetene. Alle ni LOD-er har nøyaktig samme trekanttall som kildene, riktige metriske bounds, materialer og instansbruk. Assetene er også kopiert til hovedprosjektets `Content/Signal47/Art/Cows`, og hovedprosjektets editor-modul er bygget.

Editor og Development-spill er bygget med Unreal 5.8.4. R1 fullførte full cook/pakking. R2 inneholder bare en bedre opt-in `S47Ninth=details` kamerafixture og bruker de samme ferdigkokte assetene; kodebygg og ny stage/archive består. Normal spillogikk endres ikke av dette testflagget.

Den pakkede R2 består ti sekvensielle scenarioer, 1 437 eksisterende selvtester og en supplerende nærvisning av levende ku på Low. Alle materialauditer har null fallback og null feil binding. Auditen gjenkjenner bare det komplette validerte ku-settet som tilsiktede materialseksjoner, slik at gammel Web-metadata med ett materiale ikke feilaktig underkjenner fem, åtte og tretten native slots.

Faktiske spillbilder viser levende ku på High/Low og død ku i frost med tydelig endret silhuett og bein/buk på High/Low. Detaljert aksept ligger i `visual-review-model-scope.json`; den uavhengige rapporten beholdes som `independent-packed-visual-review.json`. Fire ubelyste oversiktsbilder er for mørke til å bevise full flokk, 026 eller ni til åtte med piksler. De tilstandene dekkes av eksisterende logikkprøver og runtime-logger. Lesbarheten til ALDERETE 25 og kjevefeltets nærpiksler er ikke attestert av femmeterskameraene; geometri, materiale og den opprinnelige engelske inspeksjonsteksten er kontrollert separat. Ingen full visuell attest av alle ti bilder, menneskelig helnatt eller FPS-påstand.

Pelsen er tilpasset spillets faste eksponering gjennom den eksisterende native `TextureTint`-parameteren: 0,32 for pelsatlas/tørr kjeveflate, 0,45 for mule og 0,5 for horn. Dette bevarer mer farge og detalj under lommelykten. Et midlertidig RT-avslag under editorundersøkelsen er forkastet; alle tre sluttmesher beholder hardware ray tracing. Editorens gamle 82,922 MiB inkluderer omtrent 4,1 MiB editorgeometri og sammenlignes ikke direkte med pakket spill. Faktisk pakket måling er 79,3446 MiB alltid resident, med uendret 400 MiB-pool og 20 prosent varselgrense på 80 MiB, i 594 aktive samples fra en fullført 900-frame CSV. Det er en avgrenset minnemåling.

R2-binærens SHA256 er `60e63f4e7cac1afd5097534cdf885d009f73705dbe892d41c63fc991949da4c9`. Samme pakke er installert i `/home/tom/Games/Signal47/Linux` med full kontrollert reflinkbackup og Saved-ekskludert kopiering. Installasjonskvitteringen er `installation-cows-20261025T230243Z-758db57c/result.json` i arbeidsmappen. Alle 57 personlige Saved-noder, inkludert 40 filer, er identiske. Faktisk oppstart gjennom den installerte shellstarteren med isolert profil består og viser de nye kyrne. Payload og personlige Saved er kontrollert igjen etter oppstart og opprydding.

Canonical testbygg-vedlikehold er tørrkjørt, gjennomgått og anvendt uten aktive spilljobber. Tre gamle genererte payloadkopier er pensjonert, 11 434 912 704 logiske bytes. R2, R1 og original T02 er de tre komplette gjenværende testpakkene. Den nye duplikate T02-backupens tunge payload er også pensjonert etter vellykket levering; alle sju tilsvarende tunge filer i beholdt original T02 er kontrollert mot før-installasjonens SHA256. Kilder, Blender, Saved, Engine og bevis er bevart. Ingen fysisk diskgevinst påstås. `final-post-install.json` består åtte sluttkontroller, også byteidentitet mellom hovedprosjektets native assets/kode og den testede kopien.

Arbeidet er levert lokalt og installert; kilde-/assetendringene er ikke flettet eller pushet som del av denne bestillingen.

En selvstendig Blender-kopi med pakkede teksturer ligger også i `/home/tom/Documents/ChatGPT/Signal 47/modeller/ku/Signal47_Cows.blend`. Konseptbildene og promptsettet er kopiert til `/home/tom/Documents/ChatGPT/Signal 47/konseptbilder/2026-10-25-ku`.

## Videreført i NP1, 26. oktober 2026

Den allerede installerte R2-koden, native assetene og redigerbare kildene er tatt med uendret i PR164 og den senere NP1-pakken.40 payloadfiler og de personlige lagringene er verifisert ved ny installasjon. [NIGHT_PRODUCTION_RUNTIME.md](NIGHT_PRODUCTION_RUNTIME.md) fører denne videreleveransen; den opprinnelige ku-leveransens modell- og pikselgrenser ovenfor består.
