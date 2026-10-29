# Statiske MetaHuman-figurer, HUM-01

**Status:** Under arbeid, faktisk produksjon UNVERIFIED. **Gjelder:** privat forfatting av én sittende radio-kvinne i UE 5.8.4. **Sist kontrollert:** 29. oktober 2026.

Dette er bakeløypen, ikke ferdig HUM-01 eller runtime-integrasjon. Radiokvinnens eksisterende håndtak, radio, proxy, dialog, tidsplan og lagring eies av den senere integrasjonen. Servitrisens animasjon og assets beholdes. Ingen mannlig prototype, ny casting, nedlasting eller Creator-assembly inngår.

`radio_woman.json` beskriver én autoritativ native sittepose. Referanseskjelettet bestemmer beinbasis og faktisk kroppsmål. Lår peker frem med 3 graders fall, legger peker ned og føtter beholder sin referanseorientering. Ikke-strekkende TwoBoneIK plasserer hendene ved hver side av radioen. Håndleddet beholder faktisk referanselokal transform relativt til den nye underarmen; det beholder ikke en løs verdensrotasjon fra A-posen. Dette er ikke et ferdig radiogrep. Kroppens pose overføres til klær med samme navngitte bein, foreldre og målte referansematriser. Ansiktets felles bein følger kroppen; egne ansiktsgrener beholder faktiske lokale referansetransformer. Rigid hår og bryn bruker samme målte head-delta.

Native output har +X frem, +Y høyre, +Z opp og sete som origo. Pelvis er 8 cm over setet. Dette er ikke gamlefigurens stående fotanker. Ansiktet er den faktiske nøytrale referanseposen, uten RigLogic-evaluering. Korreksjonsbein følger sine poserte foreldre; dynamiske RBF-korreksjoner kjøres ikke. Særlig frakkens sete/lår, øyne, hender og skokontakt må derfor åpnes og vurderes i den faktiske plasseringen før aksept.

## Lokale innganger og bevaring

Den private `Signal47-mannequins-20261029/source-inventory.json` binder gjeldende Body/Face/HairCards i evening, korrigert hårmateriale, kvinnelig skjelett og eksisterende Creator. Creator ligger under `/Game/MetaHumans/Waitress/MHC_Waitress` i privat staging; dens assembly ligger under `MHC_Waitress`. Gammel `Tools/metahuman/integrate.py --apply` brukes aldri. Ingen staging-kopi overskriver gjeldende runtime.

Klær forfattes av [RadioWoman-kilden](../../../ArtSource/RadioWoman/README.md), på den uendrede private body-FBX-en med SHA256 `292e178b06f0d172d3a43220bf3ac42270e500671a6a8eec5969465348bc3e92`. Blender-modellfront er -Y/Z opp; faktisk FBX-eksport er samme -Z/Y som uniform-v5. Klærnes FBX har kun egen geometri og uendret armatur. Fersk faktisk eksport/reopen, fem slots, vekter, riggmatriser, UV-/normalpayload og outputhash ligger i privat `clothing-report.json`.

`prepare.py` nekter endrede kildestifter og manglende faktiske innganger. Før første native mutasjon kopierer den alle eksisterende MetaHumans-filer til en separat, byteidentisk reflink-backup og binder dem med SHA256. Hardlink godtas ikke. Hver operasjon krever uendrede originaler, inkludert Creator/body-FBX. Nye filer tillates bare i en ny privat run-mappe under ignorert `/Game/MetaHumans/Mannequins/RadioWoman/`. En avvist prøve bevares; neste forsøk bruker et nytt run-navn.

FBX importeres mot en privat kopi av eksisterende female body skeleton. Originalt skjelett, body, face, materialer, hår og servitrisens uniform lagres aldri av denne løypen. Referanseposeoppdatering og T0 som ny referansepose er eksplisitt av. Klærnes normaler/tangenter importeres. Én privat opaque game-master, `M_RW_ClothSurface`, har vektorparameter `RWColor` og skalarparameter `RWRoughness`. Fem `MaterialInstanceConstant` beholder navnene `M_RW_Coat`, `M_RW_Trousers`, `M_RW_Shoe`, `M_RW_Trim` og `M_RW_Button` og bindes til de samme klesslottene. Faktisk parent, begge parameterverdier, overrides, grafkoblinger og meshbinding leses tilbake ved import, bake og fersk saved-verifikasjon. Importkvitteringen binder alle ni nye assets: privat skjelett, klær, felles master, fem MI-er og brynmaterialet.

Hår bruker gjeldende korrigerte kortmateriale. Bryn beholder en egen privat maskert master med faktisk groom-deskriptor, atlas, layout og coverage-kanal. Dette erstatter groom-shaderens vertex factory; ingen ny tekstur eller atlas bakes. Ingen separat authored øyevippekortmesh er påvist, og ingen strands antas konvertert.

Den tidligere BakeR3-proben ble avbrutt ved guardens 360 sekunders grense mens det første separate klesmaterialet kompilerte. Dette var ingen poseaksept. R2/R3, delvise assets og råkvitteringer bevares. Den delte masteren erstatter fem separate klesgrafer i et nytt, friskt `BakeR4`; gamle jobber eller assets overskrives ikke.

Faktisk BakeR4-import besto med alle ni assets og de fem MI-ene. Native NullRHI-probe avviste deretter lik referanse-CS for `pelvis` i FaceMesh, ved kontroll 29. Etterkontrollen beholdt alle 350 eksisterende filer. Dette er ingen akseptert ansiktsbasis eller bake. Den eksisterende servitrisen bruker `ABP_Face_Waitress` med CopyPose for ansiktet og LeaderPose bare for uniformen. Installert UE5.8 CopyPose har som standard lokal kopiering (`bUseMeshPose=false`), med målreferanse for umappede ansiktsbein. Dette kan ikke erstattes av et lempet krav om lik Face/Body-bindpose.

Faktisk BakeR5 NullRHI-probe skrev hele `reference_diagnostics` før samme Face-avvisning. Den målte 32 delte Face/Body-bein og nøyaktig fire forskjellige, direkte uvektede bindrammer: pelvis og spine_01/02/03, med 4,30 til 5,03 cm posisjonsforskjell. Face spine_04 sin lengre lokale referansetransform kompenserer dette. Fra spine_04 stemmer de faktisk vektede felles komponentrammene med Body. Den faktiske CopyPose-klassen har én node, `bUseMeshPose=false`, og ingen avgrensende rot. Alle tre native vektskanninger var komplette. Dette var fremdeles et kontrollert probeavslag, uten runtimeposeaksept.

Face-overføringen følger nå akkurat denne lokale CopyPose-policyen over hele den samme navngitte foreldrekjeden. De fire målte broene må være direkte uvektede, ha samme rotasjon/skala og beholde Body-referansens nåværende torso-CS. Alle faktisk vektede Face-bein må etter en full Body-referansekopi beholde sine opprinnelige Face-bindrammer innen de uendrede 0,025 cm / 0,02 grader. Umappede ansiktsgrener beholder sin faktiske lokale referansepose. Posed CS for alle delte bein skal stemme med Body, med egne skulder-/hals-/hodekontroller. Dermed blir verken de avvikende forfedrene eller spine_04-kompensasjonen bare hoppet over. Uavhengig beregning på R5-data bevarte alle 635 vektede Face-rammer, med største posisjonsfeil 0,000020401 cm og normalisert vinkelfeil 0,000018861 grader. Dette er en kildeberegning, ikke ny native aksept.

`face_pose_transfer` binder broenes målte relative offsetter, hele navnemappingen og native/refkopiert/posert hode samt rigidkort-delta. Ved faktisk RHI-bake må en egen transient Face-referansekopi dessuten gi alle native CPU-skinnede vertexposisjoner tilbake til opprinnelig native bindgeometri på de valgte LOD-ene, innen samme 0,025 cm-grense. Kortene bruker Face-referansehode til faktisk kopiert hode, som i servitrisens eksisterende rigide hårbinding. Ingen ny absolutt hodeplassering eller separat kortflytting gjettes. Faktisk utseende, hals-/skuldersøm og kortdekning skal fortsatt vurderes i runtime.

Diagnosen beholdes: alle faktiske referansetransformer med translasjon, kvaternion og skala, alle delte avvik, ikke-null skin-weight-innflytelser via native BoneMap, vektede referansebounds og vektede etterkommere. Den omfatter også deaktiverte seksjoner og er payloadmåling, ikke animasjonsevaluering. CopyPose-data fra klassens standardobjekt er ikke bevis for en evaluert runtimeinstans. R5 påviste også åtte reelle rotasjonsavvik i klærnes høyre twist-kjeder, opptil 0,138899 grader, med faktisk vektede bein blant dem. Klærnes strenge krav om identiske bein, foreldre og referanser beholdes. De skal avvises inntil kun den nye private klesreferansen eventuelt er kanonisert og bevist mot uendret Body; denne Face-endringen gjør ingen slik rebind eller importkvitteringsendring.

R4 målte fire native LOD-er i både Body og Face. Body har 60 816 / 15 196 / 6 632 / 1 952 trekanter, Face 34 514 / 6 872 / 1 524 / 356. Det eksplisitte neste kildevalget er derfor overall 0/1/3, med uendrede outputtak 150k/70k/40k og screens 1/.25/.07. R5 målte kortene i `card_sources`: nært Hair 15 441 + Brow 2 744, fjernt 5 494 + 1 322. Sammen med kropp/ansikt/klær gir dette planlagt 134 803 / 50 172 / 30 412 trekanter. Dette er summer av faktiske kildetall, ikke en allerede produsert og akseptert outputmesh.

## Root kjører tunge steg sekvensielt

Disse kommandoene er oppskrift, ikke kjørte jobber. Alle Blender-/UE-/byggetrinn går gjennom prosjektets guard og isolerte profiler. Ikke start dem ved en annen tung jobb eller Toms spilløkt.

1. Root kjører klærnes Blender-oppskrift i nytt privat `private/clothes-r1` og kontrollerer den faktiske rapporten.
2. Kjør den lette preflighten fra repoets rot, med den faktiske private klesrapporten:

```sh
python3 unreal/Signal47/Tools/metahuman_mannequins/prepare.py \
  --inventory /home/tom/scratch-bm/Signal47-mannequins-20261029/source-inventory.json \
  --clothing-report /home/tom/scratch-bm/Signal47-mannequins-20261029/private/clothes-r1/clothing-report.json \
  --out /home/tom/scratch-bm/Signal47-mannequins-20261029/private/BakeR6
```

3. Bygg editor fra fryst kilde. `S47MannequinBakeEditor.cpp` og modulavhengighetene finnes bare når `Target.bBuildEditor` / `WITH_EDITOR` gjelder. Ingen runtime-krok registreres.
4. Kjør `run_editor.py` med `S47_MANNEQUIN_JOB` satt til `BakeR6/job-probe.json`, `-NullRHI` og et eget isolert `/tmp`-profilnavn. Dette importerer private klær/materialer og måler faktisk skeleton-/posekompatibilitet uten statisk konvertering. Krev `S47 MANNEQUIN PROBE PASS`. Hvis klærnes målte referanseavvik fremdeles finnes, skal de avvises ved de uendrede kravene. Behold alle avvisninger; bruk et nytt run-navn ved neste forsøk. Tom feilliste fra grafoppdateringen under NullRHI er ikke shader-, render- eller utseendeaksept.
5. Først etter akseptert pose/basis: kjør samme Python-entry i en egen ordinær editor/commandlet-jobb med det aksepterte run-navnets `job-bake.json`. Bake krever renderkontekst: bruk `-AllowCommandletRendering -RenderOffScreen`, uten NullRHI. Begge private mastere oppdateres i denne faktiske RHI-jobben; native audit krever ferdige shader maps uten compilefeil for alle materialslotter, og faktisk native Face-bindgeometri må bestå. Følgende BakeR6-eksempel viser flaggene, og er ikke tillatelse til å fortsette fra en avvist probe:

```sh
S47_MANNEQUIN_JOB=/home/tom/scratch-bm/Signal47-mannequins-20261029/private/BakeR6/job-bake.json \
  /home/tom/.local/opt/UnrealEngine-5.8.4/Engine/Binaries/Linux/UnrealEditor-Cmd \
  /home/tom/scratch-bm/wt/evening/unreal/Signal47/Signal47.uproject \
  -unattended -nosplash -nosound -AllowCommandletRendering -RenderOffScreen \
  -UserDir=/tmp/S47-radio-woman-bake-r6 \
  -run=pythonscript \
  -script=/home/tom/scratch-bm/wt/evening/unreal/Signal47/Tools/metahuman_mannequins/run_editor.py
```

6. Etter normal avslutning, kjør en ny editorprosess med `job-verify.json`, eget `/tmp`-profilnavn og samme Python-entry. Bruk igjen `-AllowCommandletRendering -RenderOffScreen`, uten NullRHI. Den må ha annen PID og leser det faktisk lagrede statiske assetet og MI-parentene/-verdiene uten reimport/lagring. Krev `S47 MANNEQUIN VERIFY PASS` og native shaderkontroll. Bake og verify avviser eksplisitt `-NullRHI`.
7. Bind filene med den lette etterkontrollen:

```sh
python3 unreal/Signal47/Tools/metahuman_mannequins/verify_files.py \
  --job /home/tom/scratch-bm/Signal47-mannequins-20261029/private/BakeR6/job-bake.json \
  --out /home/tom/scratch-bm/Signal47-mannequins-20261029/private/BakeR6/authoring-files-receipt.json
```

Ikke fortsett til runtime dersom en kontroll avvises. Kildene gjenopprettes ikke automatisk. Originaler, avviste prøver og deres kvitteringer beholdes for diagnose.

## Faktisk native kontroll og neste grense

Editorbaker bruker installert `USkinnedMeshComponent::GetCPUSkinnedVertices` og de faktiske native indeks-/UV-/farge-/seksjonsbufferne, samme underliggende API som `IMeshUtilities::ConvertMeshesToStaticMesh`. Den lokale flerkomponent-helperen kompakterer materialkartet etter hver skinned del og nullstiller da forrige importkart. Hull i en tidligere dels LOD-slotbruk kan dermed miste opprinnelig binding. Bakeren leser i stedet hvert opprinnelig seksjonskart og `LODMaterialMap` direkte, med faste slot-offsetter og entydige nye slotnavn. Originale materialreferanser bevares. Hver native CPU-lesning starter uten forhåndsaktivert CPU-skinning, slik at UE-helperen faktisk bytter renderstate og fyller cache for valgt LOD før kopiering; tilstanden kontrolleres også etterpå. Direkte MeshDescription-forfatting bevarer vertexposisjoner og corner-attributter uten legacy SaveRawMesh sin automatiske posisjonssveising/fjerning av degenererte flater.

Kropp, ansikt og klær er transient `UPoseableMeshComponent`; hår og bryn er faktiske statiske kort. Ingen skjelettkomponent lagres i output. Én statisk mesh får tre faktiske native kilde-LOD-er, overall 0/1/3. Kilder med færre LOD-er bruker sin siste faktiske LOD; klærne har foreløpig kun kilde-LOD0. Ingen skinned del kan utelates på grunn av min-LOD. Hår/bryn bruker authored LOD1 nært og LOD3 lenger unna. Dette er ingen påstand om egen klesreduksjon.

Output må ha fallende trekanttall innen 150k/70k/40k, screens 1/.25/.07, samme faktiske materialtabell, gyldige UV-er og nøyaktig videreførte native corners. Corner-hashen sammenligner posisjon, winding, UV, normal, tangent-X, tangentbasisens handedness, farge og materialslot uten numerisk kvantisering. Vertex-/triangel-ID og smoothing-mask er ikke del av denne semantiske hashen. Fersk saved-leser krever i tillegg identisk full råhash. Faktiske renderseksjoners materialindekser og trekanttall per slot må stemme med kilden, og faktisk renderbuffer må ha full UV-/høy tangentpresisjon. Alle materialers shaderressurser må være ferdige uten compilefeil eller default-surface-fallback. Nye materialgrafer må ha vellykkede koblinger og tom faktisk RecompileMaterial-feilliste. UV-presisjon er full, tangentbasis høy presisjon; normaler/tangenter recomputes ikke og lightmap-UV genereres ikke. Nanite er av, RT-fallback på og output NoCollision. Ingen global renderpolicy endres. Faktiske tall skrives først av motoren.

`sole_contacts_output_cm.left/right` binder faktiske laveste poserte klesvertexer fra både M_RW_Trim og M_RW_Shoe i fotregionen. Sålebunnen bruker Trim; Shoe alene er overdelen. `lowest`, `heel`, `toe`, `side_min`, `side_max`, `min_z_cm` og antall unike lave vertexer er i outputsete-basis. Ekstrempunktene er faktiske vertexer innen 2 mm av laveste såleflate, uten interpolering eller meshflytting. De er kontaktvitner til senere scenestråler, ikke bevis for at skoene faktisk berører benkens underlag.

Private jobber/rapporter binder kildefil-SHA256, skjelett-/pose-SHA1, faktisk material-/groomdekning, kilde-/render-LOD-tall, seksjoner, raw UV-/normal-/farge-/indekshasher og lagret asset-SHA256. Dette er interne forfatterkontroller. Bilder i faktisk plassering, LOD-skifter, radio-/sete-/skokontakt, ingen dobbel gammel kropp, proxy/tidsplan og disk/title/full Flow Continue gjenstår hos root. Det samme gjelder faktisk pakket kjøring, menneskelig input, FPS/fullmatrise, installasjon og release.

Prosjektbiblioteket ble søkt for MetaHuman/pose/bake og delte materialer. De generiske animation-/materials-treffene gir ingen prøvd statisk bakeløype i dette prosjektet; ingen kode derfra gjenbrukes. MI-løypen følger prosjektets prøvde `Tools/calichehomes_native/native.py`. Den lokale UE5.8-versjonens scalar-/vector-settere returnerer false selv etter utført setting; derfor kreves faktisk parameteravlesning. API-grunnlaget er installert UE 5.8.4 `Developer/MeshUtilities/{Public/MeshUtilities.h,Private/MeshUtilities.cpp}`, `Runtime/Engine/Private/Components/PoseableMeshComponent.cpp` / `SkinnedMeshComponent.cpp`, `Runtime/Engine/Public/Rendering/SkinWeightVertexBuffer.h`, `Runtime/AnimGraphRuntime/Private/AnimNodes/AnimNode_CopyPoseFromMesh.cpp`, `Runtime/AnimationCore/Private/TwoBoneIK.cpp`, `Editor/MaterialEditor/{Public/MaterialEditingLibrary.h,Private/MaterialEditingLibrary.cpp}` og `Shaders/Private/HairStrands/HairCardsAttributeCommon.ush`. Privat MetaHuman-lisens/proveniens fra inventaret beholdes, uten ny rettighetsklarering eller referanseredistribusjon.
