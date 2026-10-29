# Statiske MetaHuman-figurer, HUM-01

**Status:** Under arbeid, faktisk produksjon UNVERIFIED. **Gjelder:** privat forfatting av én sittende radio-kvinne i UE 5.8.4. **Sist kontrollert:** 29. oktober 2026.

Dette er bakeløypen, ikke ferdig HUM-01 eller runtime-integrasjon. Radiokvinnens eksisterende håndtak, radio, proxy, dialog, tidsplan og lagring eies av den senere integrasjonen. Servitrisens animasjon og assets beholdes. Ingen mannlig prototype, ny casting, nedlasting eller Creator-assembly inngår.

`radio_woman.json` beskriver én autoritativ native sittepose. Referanseskjelettet bestemmer beinbasis og faktisk kroppsmål. Lår peker frem med 3 graders fall, legger peker ned og føtter beholder sin referanseorientering. Ikke-strekkende TwoBoneIK plasserer hendene ved hver side av radioen. Håndleddet beholder faktisk referanselokal transform relativt til den nye underarmen; det beholder ikke en løs verdensrotasjon fra A-posen. Dette er ikke et ferdig radiogrep. Kroppens pose overføres til klær med samme navngitte bein, foreldre og målte referansematriser. Ansiktets felles bein følger kroppen; egne ansiktsgrener beholder faktiske lokale referansetransformer. Rigid hår og bryn bruker samme målte head-delta.

Native output har +X frem, +Y høyre, +Z opp og sete som origo. Pelvis er 8 cm over setet. Dette er ikke gamlefigurens stående fotanker. Ansiktet er den faktiske nøytrale referanseposen, uten RigLogic-evaluering. Korreksjonsbein følger sine poserte foreldre; dynamiske RBF-korreksjoner kjøres ikke. Særlig frakkens sete/lår, øyne, hender og skokontakt må derfor åpnes og vurderes i den faktiske plasseringen før aksept.

## Lokale innganger og bevaring

Den private `Signal47-mannequins-20261029/source-inventory.json` binder gjeldende Body/Face/HairCards i evening, korrigert hårmateriale, kvinnelig skjelett og eksisterende Creator. Creator ligger under `/Game/MetaHumans/Waitress/MHC_Waitress` i privat staging; dens assembly ligger under `MHC_Waitress`. Gammel `Tools/metahuman/integrate.py --apply` brukes aldri. Ingen staging-kopi overskriver gjeldende runtime.

Klær forfattes av [RadioWoman-kilden](../../../ArtSource/RadioWoman/README.md), på den uendrede private body-FBX-en med SHA256 `292e178b06f0d172d3a43220bf3ac42270e500671a6a8eec5969465348bc3e92`. Blender-modellfront er -Y/Z opp; faktisk FBX-eksport er samme -Z/Y som uniform-v5. Klærnes FBX har kun egen geometri og uendret armatur. Fersk faktisk eksport/reopen, fem slots, vekter, riggmatriser, UV-/normalpayload og outputhash ligger i privat `clothing-report.json`.

`prepare.py` nekter endrede kildestifter og manglende faktiske innganger. Før første native mutasjon kopierer den alle eksisterende MetaHumans-filer til en separat, byteidentisk reflink-backup og binder dem med SHA256. Hardlink godtas ikke. Hver operasjon krever uendrede originaler, inkludert Creator/body-FBX. Nye filer tillates bare i en ny privat run-mappe under ignorert `/Game/MetaHumans/Mannequins/RadioWoman/`. En avvist prøve bevares; neste forsøk bruker et nytt run-navn.

FBX importeres mot en privat kopi av eksisterende female body skeleton. Originalt skjelett, body, face, materialer, hår og servitrisens uniform lagres aldri av denne løypen. Referanseposeoppdatering og T0 som ny referansepose er eksplisitt av. Klærnes normaler/tangenter importeres. Fem egne overflatematerialer bindes etter eksakte slotnavn. Hår bruker gjeldende korrigerte kortmateriale. Bryn får et privat vanlig maskert overflatemateriale med faktisk groom-deskriptor, atlas, layout og coverage-kanal. Dette erstatter groom-shaderens vertex factory; ingen ny tekstur eller atlas bakes. Ingen separat authored øyevippekortmesh er påvist, og ingen strands antas konvertert.

## Root kjører tunge steg sekvensielt

Disse kommandoene er oppskrift, ikke kjørte jobber. Alle Blender-/UE-/byggetrinn går gjennom prosjektets guard og isolerte profiler. Ikke start dem ved en annen tung jobb eller Toms spilløkt.

1. Root kjører klærnes Blender-oppskrift i nytt privat `private/clothes-r1` og kontrollerer den faktiske rapporten.
2. Kjør den lette preflighten fra repoets rot, med den faktiske private klesrapporten:

```sh
python3 unreal/Signal47/Tools/metahuman_mannequins/prepare.py \
  --inventory /home/tom/scratch-bm/Signal47-mannequins-20261029/source-inventory.json \
  --clothing-report /home/tom/scratch-bm/Signal47-mannequins-20261029/private/clothes-r1/clothing-report.json \
  --out /home/tom/scratch-bm/Signal47-mannequins-20261029/private/BakeR1
```

3. Bygg editor fra fryst kilde. `S47MannequinBakeEditor.cpp` og modulavhengighetene finnes bare når `Target.bBuildEditor` / `WITH_EDITOR` gjelder. Ingen runtime-krok registreres.
4. Kjør `run_editor.py` med `S47_MANNEQUIN_JOB` satt til `BakeR1/job-probe.json`. Dette importerer private klær/materialer og måler faktisk skeleton-/posekompatibilitet uten statisk konvertering. Krev `S47 MANNEQUIN PROBE PASS`.
5. Kjør samme Python-entry i en egen ordinær editor/commandlet-jobb med `BakeR1/job-bake.json`. Bake krever renderkontekst: bruk `-AllowCommandletRendering -RenderOffScreen`, uten NullRHI. Eksempel på payload som root legger inn i guard-planen:

```sh
S47_MANNEQUIN_JOB=/home/tom/scratch-bm/Signal47-mannequins-20261029/private/BakeR1/job-bake.json \
  /home/tom/.local/opt/UnrealEngine-5.8.4/Engine/Binaries/Linux/UnrealEditor-Cmd \
  /home/tom/scratch-bm/wt/evening/unreal/Signal47/Signal47.uproject \
  -unattended -nosplash -nosound -AllowCommandletRendering -RenderOffScreen \
  -UserDir=/tmp/S47-radio-woman-bake-r1 \
  -run=pythonscript \
  -script=/home/tom/scratch-bm/wt/evening/unreal/Signal47/Tools/metahuman_mannequins/run_editor.py
```

6. Etter normal avslutning, kjør en ny editorprosess med `job-verify.json`, eget `/tmp`-profilnavn og samme Python-entry. Den må ha annen PID og leser det faktisk lagrede statiske assetet uten reimport/lagring. Krev `S47 MANNEQUIN VERIFY PASS`.
7. Bind filene med den lette etterkontrollen:

```sh
python3 unreal/Signal47/Tools/metahuman_mannequins/verify_files.py \
  --job /home/tom/scratch-bm/Signal47-mannequins-20261029/private/BakeR1/job-bake.json \
  --out /home/tom/scratch-bm/Signal47-mannequins-20261029/private/BakeR1/authoring-files-receipt.json
```

Ikke fortsett til runtime dersom en kontroll avvises. Kildene gjenopprettes ikke automatisk. Originaler, avviste prøver og deres kvitteringer beholdes for diagnose.

## Faktisk native kontroll og neste grense

Editorbaker bruker installert `USkinnedMeshComponent::GetCPUSkinnedVertices` og de faktiske native indeks-/UV-/farge-/seksjonsbufferne, samme underliggende API som `IMeshUtilities::ConvertMeshesToStaticMesh`. Den lokale flerkomponent-helperen kompakterer materialkartet etter hver skinned del og nullstiller da forrige importkart. Hull i en tidligere dels LOD-slotbruk kan dermed miste opprinnelig binding. Bakeren leser i stedet hvert opprinnelig seksjonskart og `LODMaterialMap` direkte, med faste slot-offsetter og entydige nye slotnavn. Originale materialreferanser bevares. Direkte MeshDescription-forfatting bevarer vertexposisjoner og corner-attributter uten legacy SaveRawMesh sin automatiske posisjonssveising/fjerning av degenererte flater.

Kropp, ansikt og klær er transient `UPoseableMeshComponent`; hår og bryn er faktiske statiske kort. Ingen skjelettkomponent lagres i output. Én statisk mesh får tre faktiske native kilde-LOD-er, overall 1/3/5. Kilder med færre LOD-er bruker sin siste faktiske LOD; klærne har foreløpig kun kilde-LOD0. Ingen skinned del kan utelates på grunn av min-LOD. Hår/bryn bruker authored LOD1 nært og LOD3 lenger unna. Dette er ingen påstand om egen klesreduksjon.

Output må ha fallende trekanttall innen 150k/70k/40k, screens 1/.25/.07, samme faktiske materialtabell, gyldige UV-er og nøyaktig videreførte native corners. Corner-hashen sammenligner posisjon, winding, UV, normal, tangent-X, tangentbasisens handedness, farge og materialslot uten numerisk kvantisering. Vertex-/triangel-ID og smoothing-mask er ikke del av denne semantiske hashen. Fersk saved-leser krever i tillegg identisk full råhash. Faktiske renderseksjoners materialindekser og trekanttall per slot må stemme med kilden, og faktisk renderbuffer må ha full UV-/høy tangentpresisjon. Alle materialers shaderressurser må være ferdige uten compilefeil eller default-surface-fallback. Nye materialgrafer må ha vellykkede koblinger og tom faktisk RecompileMaterial-feilliste. UV-presisjon er full, tangentbasis høy presisjon; normaler/tangenter recomputes ikke og lightmap-UV genereres ikke. Nanite er av, RT-fallback på og output NoCollision. Ingen global renderpolicy endres. Faktiske tall skrives først av motoren.

`sole_contacts_output_cm.left/right` binder faktiske laveste poserte klesvertexer fra både M_RW_Trim og M_RW_Shoe i fotregionen. Sålebunnen bruker Trim; Shoe alene er overdelen. `lowest`, `heel`, `toe`, `side_min`, `side_max`, `min_z_cm` og antall unike lave vertexer er i outputsete-basis. Ekstrempunktene er faktiske vertexer innen 2 mm av laveste såleflate, uten interpolering eller meshflytting. De er kontaktvitner til senere scenestråler, ikke bevis for at skoene faktisk berører benkens underlag.

Private jobber/rapporter binder kildefil-SHA256, skjelett-/pose-SHA1, faktisk material-/groomdekning, kilde-/render-LOD-tall, seksjoner, raw UV-/normal-/farge-/indekshasher og lagret asset-SHA256. Dette er interne forfatterkontroller. Bilder i faktisk plassering, LOD-skifter, radio-/sete-/skokontakt, ingen dobbel gammel kropp, proxy/tidsplan og disk/title/full Flow Continue gjenstår hos root. Det samme gjelder faktisk pakket kjøring, menneskelig input, FPS/fullmatrise, installasjon og release.

Prosjektbiblioteket ble søkt for MetaHuman/pose/bake. Det generiske `scenario-unreal-animation`-treffet gir ingen prøvd statisk bakeløype i dette prosjektet; ingen kode derfra gjenbrukes. API-grunnlaget er installert UE 5.8.4 `Developer/MeshUtilities/{Public/MeshUtilities.h,Private/MeshUtilities.cpp}`, `Runtime/Engine/Private/Components/PoseableMeshComponent.cpp` / `SkinnedMeshComponent.cpp`, `Runtime/AnimationCore/Private/TwoBoneIK.cpp` og `Shaders/Private/HairStrands/HairCardsAttributeCommon.ush`. Privat MetaHuman-lisens/proveniens fra inventaret beholdes, uten ny rettighetsklarering eller referanseredistribusjon.
