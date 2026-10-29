# Radio-kvinnens egne vinterklær

Dette er HUM-01 sin forfatterkilde for en nøktern 1986-vinterfrakk, løse bukser og lukkede sko. Klærne bygges fra egne ringer og paneler med stofftykkelse, krage, mørke knapper, lommer og skosåler. Servitrisens uniform brukes ikke som bekledning. Frakken har en faktisk åpen nedre front slik at den kan følge lårene i den senere sitteposen.

**Status: Under arbeid, ny lokal frakkretting har faktisk saved/reopen og separat fersk kildekontroll. Sist kontrollert: 29. oktober 2026.** R11 har [kontakt-/funksjonsbevis med utseende FAIL](../../Evidence/RADIO-WOMAN-2026-10-29/r11-editor-functional-receipt.json) på to skuldergliper. Et separat, lesende [Body/Face-uttak](../../Evidence/RADIO-WOMAN-2026-10-29/rest-surface-frame-r1-receipt.json) har nå bevist komplett hudgrunnlag og faktisk native→FBX-ramme. [Faktisk R4-kilde](../../Evidence/RADIO-WOMAN-2026-10-29/r4-source-r3-receipt.json) består med 23 368 trekanter og 16 bevarte deler. Ny ClothesR2/BakeR12, sittende skulderutseende og pakket runtime er fortsatt UNVERIFIED.

[coat_yoke.py](coat_yoke.py) former øvre egen `Coat_body_open_lower_front` mot den målte, private Body+Face-huden. De øvrige 16 delene beholdes. [Egen ytterflate](own-yoke-surface-recipe.json) har eksplisitt ny halsring og lokal oppdeling; ingen hudtriangler kopieres. Nedre ytre posisjoner/UV og nedre indre posisjoner har avgrensede, eksakte bevaringsområder. Normalvirkningen av nye/flyttede vertices og orienterte flater avledes fra faktisk nabotopologi. Utenfor dette området gjelder en stram numerisk kontroll, med alle rånormaler bevart.

Ytterstoffmålet er 12 mm og stofftykkelsen 2,5 mm. Faktiske flate-/hudprøver må holde minst 6 mm ytre klaring, og innerstoffet må være uten målt hudpenetrasjon. Prøvene er finitte; de beviser ingen kontinuerlig helflate. [verify_source.py](verify_source.py) åpner den faktisk lagrede kilden i en annen Blenderprosess og gjentar referanse-, payload-, delidentitets- og fitkontroll uten ny eksport eller lagring. De tidligere kappeforsøkene og [shoulder_caps.py](shoulder_caps.py) beholdes som historikk etter faktiske avslag.

## Privat inndata og utdata

Kilden er den eksisterende private kroppen `/home/tom/scratch-bm/Signal47-metahuman-20261026/blender/waitress_body.fbx`, SHA-256 `292e178b06f0d172d3a43220bf3ac42270e500671a6a8eec5969465348bc3e92`. Den kontrolleres før og etter kjøring. Original FBX og tidligere fitting-scener overskrives aldri. Det målte Blender-armaturet har 341 bein og object `root`, låst til den faktiske referansehashen. Den separate Unreal-importen har 342 bein; native Body-/Ref-/InvRef-kravene endres ikke. Kroppens flater kopieres ikke til klærne; verktøyet måler skalare tverrsnitt og overfører deformasjonvekter fra den samme kroppen.

En ny utmappe må ligge under `/home/tom/scratch-bm/Signal47-mannequins-20261029/private/`. Eksisterende utmapper avvises. Alle avledede kilder blir der:

- `radio_woman_clothes_rest.blend`: redigerbare klær, privat fitting-kropp og originalt armatur.
- `SKM_RadioWoman_Clothes.fbx`: bare klesmesh og uendret armatur, uten kropp eller animasjon.
- `clothing-report.json`: faktiske restposebounds, materialer, geometri-/vektkontroll, komplett beinnavn-/foreldre-/referansematriseliste, kildehash og outputhasher. Feil får `FAIL`, og eksport godkjennes ikke som mannequin.

MetaHuman-kropp, armatur, vektede meshes og fitting-scener er private lisensierte kilder. Ingen slik geometri eller tekstur skal legges i offentlig Git. Bare egne oppskrifter, kontrakt og dokumentasjon hører hjemme her. Eksisterende Epic-kreditering beholdes i prosjektets kreditgenerator.

## Grensesnitt mot autoritativ pose og baking

Avtalt med mannequin-bakeren: klærne leveres i uendret importert restpose på kroppens eksakte skjelett. Ingen bein flyttes, omdøpes, reskaleres eller får ny referansepose. Import bruker `automatic_bone_orientation=False`. Alle opprinnelige bein følger eksporten, ingen leaf-bones legges til, og høyst fire normaliserte beinvekter er tillatt per klesvertex. Skoene følger hvert sitt originale `foot_l`/`foot_r`-bein.

Blender-geometrien har meter, Z opp og front mot negativ Y. **FBX-eksporten har eksplisitt `axis_forward=-Z`, `axis_up=Y`**, identisk med de faktiske defaultene i den fungerende uniform-v5-eksporten. Modellfront og FBX-eksportakser er to forskjellige opplysninger. `global_scale=1`, `apply_unit_scale=True`, `bake_space_transform=False` og `use_armature_deform_only=False` bevarer den etablerte eksportløypa. Importøren må sammenholde faktisk FBX med sin private kopi av eksisterende kroppsskjelett, uten å endre servitrisens original.

De fem materialsporene er alltid `M_RW_Coat`, `M_RW_Trousers`, `M_RW_Shoe`, `M_RW_Trim`, `M_RW_Button` i denne rekkefølgen. [clothing.json](clothing.json) fører lineære farger og roughness; ingen huddel eller lisensiert tekstur eksporteres som klesmateriale. Stoffets enkle materialer skal senere deles av de bakte delene.

Nativebaker eier **én** sittepose for kropp og klær på samme beinbasis, og riktig følge for ansikt/hår/bryn. Klærne blir ikke forhåndsposert i Blender. Skriptet oppretter ingen animasjon, korrektiv pose eller runtime-skinning. Kontakt med benk, såler, knær, radio og hender må vurderes etter faktisk nativepose. Den lukkede frakkdelen over hoften og hofte-/beinovergangen må særlig kontrolleres for gjennomskjæring og strekk ved sitting. Det målte såleminimumet i rapporten må inngå i endelig fotkontakt.

## Kommandoer

Lett kilde-/topologikontroll, uten Blender og uten outputfiler:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 unreal/ArtSource/RadioWoman/build_clothes.py --check-source
```

Faktisk forfatting kjøres **bare av root i et ledig reservert tungjobb-vindu**, gjennom eksisterende `HeavyReservation`, `unreal_guard.py require-idle` og `run_owned`. Payloadkommandoen fra worktreeroten er:

```sh
/home/tom/.local/opt/blender-5.2.2-linux-x64/blender --background --factory-startup --threads 1 --python-exit-code 1 \
  --python unreal/ArtSource/RadioWoman/build_clothes.py -- \
  /home/tom/scratch-bm/Signal47-metahuman-20261026/blender/waitress_body.fbx \
  /home/tom/scratch-bm/Signal47-mannequins-20261029/private/clothes-r4-r3
```

Velg et ubrukt runnavn; eksemplet viser den aktuelle, private kandidaten. Suksessmarkøren er `RADIO_WOMAN_CLOTHES_REST_EXPORTED`. Skriptet krever faktisk lagret og reåpnet `.blend`, uendret armatur og gyldige vekter/materialspor. Det beviser ikke at FBX importerer identisk i Unreal; det er neste nativeport. Separat fersk leser må også bestå før den dedikerte [to-asset-importen](../../Signal47/Tools/metahuman_mannequins/garment_source.py) og [R12-runneren](../../Signal47/Tools/metahuman_mannequins/run_r12.py) kan brukes. De gjenbruker R10-materialene og R11-posen. Ingen preview-rendering eller installasjon startes av skriptet.

## Grunnlag og gjenstående kontroll

Prosjektbiblioteket ble søkt først. [Scenario Blender Rigging](https://github.com/scenario-labs/skills/blob/f0adb5b5ebc1fc8f7e333554d2aeb767c5558d6b/skills/dcc/blender/scenario-blender-rigging/SKILL.md) er MIT og oppgir faktisk Blender 5.2.1-prøving. Her brukes rådene om felles vektkilde, normaliserte begrensede vekter og separat deformasjonskontroll som referanse. Ingen helper er installert eller kopiert. Lokalt mål er Blender 5.2.2. Prosjektets egen [uniformkilde](../WaitressUniform/build_uniform.py) gir den eksisterende FBX-/Data Transfer-praksisen; denne kilden lager nye vinterplagg.

Lett kontroll undersøker egne geometriindekser, ikke-degenererte flater, positiv winding på lukket prøvevolum, UV-søm og faktisk åpen front, og avviser snudd radbasis. Den er kjørt med PASS. Separate lette fixtures består også for privatbane, kilde-PIN, normaliserte/fire vekter og avvisning av tomme/fremmede/fem vekter. Ny lokal frakkretting forventes å gi 23 368 klestriangler etter stofftykkelse, under uendret 24 000-grense. Faktisk produksjon og separat fersk kildeprosess bekrefter 23 368 trekanter. De finitte prøvene måler minst 6,819 mm ytre klaring og 4,577 mm indre klaring. Dette er ingen kontinuerlig helflate- eller sitteposeaksept.

Blender-API, faktisk kroppspassform, deformasjon ved sitting, FBX-/native-referanseskjelett, LOD/minnebudsjett og pakket visuell aksept er **UNVERIFIED** inntil de er kjørt. Hele HUM-01 krever også presis bevaring av radioen, opprinnelig proxy/samhandling og nattregel, ingen dobbel kropp, samt disk/title/Continue i faktisk pakket spill. Dagens ferdige animerte servitrise er separat og skal bevares.
