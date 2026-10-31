# UFO og cloud tank i SC13

Den aktive visuelle koblingen ligger i `S47FinaleSky.cpp`. Den følger Chapter6 sin fase ved C8.15 og legger ingen ny progresjonsport på eldre lagringer. EVENT beholder47sekunder, uttrekket34sekunder, og den opprinnelige sluttsekvensen med1947-foto er urørt. `-S47FinaleSkyOff` beholder den tidligere avslutningen.

Fartøyet er opakt. To lokale volum tegner bare kubenes utgangsflater med reversert culling og deaktivert vanlig dybdetest. Strålemarsjen avsluttes ved faktisk SceneDepth, så sky foran skroget skjuler det og sky bak skroget klippes. Skyene bruker samme translucency-pass som stjernene, med sortering mellom stjerner og nær glassgeometri. Bare skymaterialet har variabel shading rate avslått, slik at Low beholder jevnere silhuetter. Lateral åpning skjer i fase5..21; første150m løft følger åpningen, resten av valgt totalhøyde følger en sen quintic-kurve i fase40..68. Fluidfoldene bruker fortsatt samme faseklokke. Skydome, historieklokke og globalt daggry endres ikke. Bare den private stjerne-MID-en får mindre diameter på de lyseste punktene og et lokalt dimmegulv, gradvis under innkjøringen og gjennom uttrekket. Originale stjernekoordinater og hullet beholdes; gammel og ny himmel nullstilles ved cleanup. Hjørneklokken går ut av bildet når kameraet forlater bilen; EVENT beholder47sekunder og teksting/pause har egne widgeter. En svak lokal kabinfylling og renderkopier av de eksisterende rattdelene gjør den tomme bilen lesbar under første uttrekk. Originalrattet og originalmaterialene er urørt; kopiene vises bare når den faktiske utvendige fitted-kabinen er synlig. Første kameraposisjon ser litt ned fra førersiden når finalearten er klar. Mellompartiets siktelinje heves mot horisonten og13..21sekunder bruker eased slerp mellom enderetningene, mens kamerabanens posisjoner, 34 sekunder og siste blikk ved031°/48° beholdes. Fallback beholder den tidligere regien. Tidligere himmelbesøk skjules og får frosset observasjonsklokken mens C6 er aktiv. To private bakke-MID-er fjerner de overlappende coarse/ribbon-nettenes ulike vertexfarger under finalen; overflatetekstur og originalgeometri beholdes. Bare disse to komponentenes egen skyggekasting dempes under finalen for å fjerne den grove rutenettflaten i lav sol; de mottar fremdeles andre skygger. Eksakte originalmaterialer og CastShadow-flagg gjenopprettes ved cleanup. Negativ fill demper native sol-, sky- og månelys bare i den aktive Roswell-visningen; dette er et regigrep, ikke et fysisk skyggeavtrykk.

## Arbeidsløype

Root kjører én tung jobb om gangen gjennom felles reservasjon og `Scripts/unreal_guard.py`. Kommandoene her er payload til den løypen. En aktiv spilløkt eller fremmed prosess skal ikke stoppes. Ingen kjøpte eller eksterne assets inngår i disse kildene.

`bake_clouds.py` lagrer egen redigerbar Blender-kilde og OpenVDB-cache i en ny outputmappe. Felles simulasjonskilde er [CloudTank-wavelet-v2](../../../ArtSource/FinaleSky/CloudTank-wavelet-v2/README.md), fra `cloud-wavelet128x2-v01`: base 128 med wavelet-noise ×2, 96 rammer og valgte rammer 48/72/96. Vorticity er 0.18, tetthetsoppdrift -0.14 og rotorer av. Databaken etterfølges av noise-baken; pakkeren leser `cache/noise/fluid_noise_####.vdb`, griden `density_noise`.

```sh
OMP_NUM_THREADS=6 blender -b --factory-startup -t 6 --python Tools/finalesky/bake_clouds.py -- --output /ny/cloud-kilde --resolution 128 --noise-scale 2 --noise-strength .35 --noise-pos-scale 2 --noise-time-anim .1 --atlas-size 256 --bit-depth 16 --frames 96 --picks 48 72 96 --vorticity .18 --buoyancy -.14 --vortex-strength 0
```

`pack_clouds.py` leser den faktiske VDB-tettheten, kontrollerer aksene mot VDB-accessor og gir valgte rammer felles koordinater/normalisering. Den aktive kilden er 256³, lagret som 16×16 Z-snitt i 4096×4096 lineære 16-bits gråtoneatlas. Standard uten noise beholder 128³/8-bit som alternativ. Datarekkefølgen er `density[z,y,x]`, X raskest, flis `(z % 16, z // 16)`, PNG-rader fra lav til høy Y. Ytterste texler er tomme. `atlas.json` oppgir faktisk kildeboks og tetthetsskala. NPY/VDB og Blender bevares som produksjonskilde. Små `BLENDER-density-projections` er datavisninger, ikke render eller Unreal-bilder.

Importer tre valgte atlas til egne Texture2D/VolumeTexture under `/Game/FinaleSky/Clouds`:

```sh
S47_CLOUD_ATLAS=/ny/cloud-kilde/atlases/atlas.json UnrealEditor-Cmd Signal47.uproject -run=pythonscript -script=Tools/finalesky/import_cloud_volumes.py -AllowCommandletRendering -RenderOffscreen -unattended
```

Importøren lagrer bare egne assets. Aktiv 16-bits kilde bruker `TC_HALF_FLOAT`, sRGB av, clamp, ingen mipmaps og eksplisitte 256×256 fliser. Filene heter `V_S47_CloudDensityA/B/C` og har egne atlas som kilde. Metadataen `S47DensityScale=2` føres videre til materialets `DecodeScale=2`, som gjenoppretter kildetettheten fra normaliserte atlas.

De aktive volumene er importert og faktisk RHI-avlest som 256³ `PF_R16F` etter et tegnet bilde. GPU-format og dimensjoner leses direkte fra `TextureRHI`. `Blueprint_GetBuiltTextureSize` beregner målplattformstørrelsen, og registryens `Format` kan vise en mellomressurs etter reimport; disse er konfigurasjonsinformasjon, ikke GPU-bevis.

`update_materials.py` bygger bare egne fartøy-/skymaterialer. Den velger `cloud_volume.hlsl` når de tre volumene finnes, ellers `cloud_field.hlsl`. `S47_CLOUD_FIELD=volume` krever volumene; `analytic` velger referansefeltet. Kjør med rendering tillatt, uten NullRHI:

```sh
S47_CLOUD_FIELD=volume UnrealEditor-Cmd Signal47.uproject -run=pythonscript -script=Tools/finalesky/update_materials.py -AllowCommandletRendering -RenderOffscreen -unattended
```

Det opprinnelige trefeltet har 18 innganger: de 13 opprinnelige pluss `DensityA/B/C`, `Density` og `DecodeScale`. To naboframes interpoleres i eksplisitt Chapter6-fase, med24sekunders kildeforskyvning mellom bankene for mindre gjentakelse. Derivater og lysprober leser samme 3D-tetthet; små slør får forsiktig feltwarp. Det analytiske feltet er et eget alternativ, uten fluidteksturer. Editor-runtime etterspør de egne shaderkartene med `EnsureIsComplete` før plassering. Cook inkluderer `/Game/FinaleSky`.

Aktiv avspilling bruker [CloudTank-sequence-v3](../../../ArtSource/FinaleSky/CloudTank-sequence-v3/README.md): ni frames48/54/60/66/72/78/84/90/96 fra den samme bevarte noise-cachen, med uendret kildeboks og DecodeScale2. `pack_clouds.py --frames 48 54 60 66 72 78 84 90 96 --size 256 --bit-depth 16 --cache-kind noise` pakker til en fersk outputmappe. `S47_CLOUD_SEQUENCE_ATLAS=/ny/atlas.json` velger de ni separate atlas-/volumparene i `import_cloud_volumes.py`, bare under `/Game/FinaleSky/CloudSequence`. Alle ni faktiske TextureRHI-er er bekreftet som256³ PF_R16F.

`update_sequence_material.py` dupliserer bare det egne skygrunnmaterialet til `M_S47_FinalCloudSequence` og bruker `cloud_volume_sequence.hlsl`, med `FrameMix` som inngang19. Runtime holder ni sterke teksturreferanser, binder bare et nabopar til hver private bank-MID og beregner blandingen fra samme Chapter6-fase. Ved siste frame brukes par7/8 og mix1. Både normaler og lysprober leser samme par. High bruker128 marsjsteg, Low28. Finkornet jitter fra volumets fjerne utgangsflate erstatter de synlige rutene fra den nære inngangsflaten. Dette øker resident volumdata fra96 til288MiB; det øker ikke antall tetthetslesninger per marsjsteg. `r.S47.Finale.CloudSequence=0` velger det tidligere trefeltet ved neste klargjøring. Manglende sekvensdata beholder også trefeltet.

Det aktive fartøyet er [FinalCraft-v5](../../../ArtSource/FinaleSky/FinalCraft-v5/README.md), importert med tre egne LOD-er og de fire eksisterende materialinstansene. `build_craft_v2.py --variant v5` bygger `geometry_v5.py` til en fersk Blender-/FBX-kilde; v2, v3 og v4 er fortsatt tilgjengelige som separate varianter. V5 beholder v4s brede solvendte kragefas, mørkere tilbakesatser og egne radiale hjørnenormaler. Åtte brede bærefelt med6–10m relieff gir en lesbar underside; de tre LOD-ene har26608/13744/7312 trekanter. `import_craft_v2.py` oppdaterer deretter bare `/Game/FinaleSky/SM_S47_FinalCraft`. Det bruker ingen gammel v1-gate.

## Visuell iterering

`r.S47.Finale.*` har `CraftForward`, `CraftRight`, `CraftHeight`, `CraftScale`, `CraftFill`, `CabFill`, `StarScale`, `StarDim`, `CloudSequence`, `BankForward`, `BankSpread`, `BankRevealSpread`, `BankRevealRise`, `BankHeight`, `BankDepth`, `BankWidth`, `BankThickness`, `Density`, `Glow`, `GroundCastShadow` og `Shade`. Avstander og bankenes halvutstrekning er meter. `CraftScale` er uniform skala på den normaliserte egne modellen. Readback viser anvendte verdier. `CabFill` måles i candela og fases bare inn/ut under uttrekk0.8..8.2sekunder. Dette er kunstparametere, ikke nye spillvalg.

`-S47FinalePreview=2,14,24,68 -S47FinaleShots=/ny/bildemappe -UserDir=/tmp/unik-profil` gir uttrykkelig frosne utviklerbilder fra C8.15 og eksisterende uttrekk. De er ikke en gjennomspilt kjøretur. `-S47FinaleClean` skjuler UI i bildene. Valgfri `-S47FinaleSweep=/sti/valg.json` leser `frames` med `name`, `phase` og `tune`; bare kjente finaleparametere kan endres. Tidspunktene sorteres stigende.

`-S47FinaleMovie=/ny/opptaksmappe -S47Jump=chapter6:line -S47Auto=24.6 -S47Case=0 -UserDir=/tmp/unik-profil` observerer vanlig gameplay etter en utviklerinngang, uten å fryse tid eller styre kamera, bil, UI eller lyd. Inngangene `chapter6` og `chapter6:lastmile` kan også brukes. Med `-S47FinaleMovieStartMile=5.9` venter opptakeren på faktisk mileposisjon før bilde og masterlyd starter; den flytter ikke bilen. PNG og `frames.json` inneholder request-tider for avspilling med variabel bildefrekvens. Hele tittel/kreditt/foto-forløpet beholdes; normalt stoppes opptaket8sekunder etter synlig foto. `-S47FinaleMovieEarlyEnd=8` gir bare uttrekket og merkes ufullstendig slutt. Skjermfangst er ikke ytelsesmåling. Opt-in `-S47FinaleMovieAudio` tar opp den faktiske mastermiksen og skriver lokal `master-mix.wav` synkront før avslutning. Manifestet oppgir faktisk lydformat og om filen ble ferdig; etterspurt lyd som feiler gir ufullstendig opptak, mens PNG beholdes. Med lyd brukes `wallSeconds` og opptaksforespørselens offset til avspilling; ingen ny lydasset eller miksruting opprettes.

Opt-in `-S47FinaleMovieAsync` beholder samme UI-skjermfangst, men komprimerer og skriver PNG på motorens bildekø. Maks tre egne skriv kan være i flyt, med én viewport-forespørsel om gangen. Standardmålet er24fps og eksplisitt12..30 støttes; uten flagget beholdes12..15/default15. Bare fullførte vellykkede filer føres i FIFO-rekkefølge, og egne skriv tømmes før lyd/manifest fullføres. GPU-avlesning har fortsatt kostnad, så opptaket gir fortsatt ikke FPS-bevis.

## Bevarte originaler

[Det opprinnelige forfatterarkivet](../../../ArtSource/FinaleSky/README.md), Blender/FBX, råmanifest og `NativeArchive` er bevart. `build_source.py`/`geometry.py` beskriver den tidligere geometrioppskriften. V2-kildene har egne mapper og manifester. Historiske prepare-/adapter-/SC12-gater fra tidligere sidegren er ikke dagens arbeidsløype; de gamle instruksjonene finnes i Git-historikken. Ingen originalkvittering eller arkivfil skrives om ved en material- eller formiterasjon.

Visuell referanse er blant annet [Scott Squires sin egen beskrivelse av cloud tank-opptak](https://effectscorner.blogspot.com/2009/02/cloud-tank-effect.html): store sammenhengende folder og langsom utbredelse brukes som regiretning. Spillets felt er egen simulasjon; ingen filmplater er kopiert.

Prosjektbiblioteket ble søkt for Unreal-skyer, shader og Blender/Mantaflow. Ingen ferdig kompatibel fluidløype ble funnet. Implementasjonen er egen kode, med API kontrollert i lokal UE5.8.4 og Blender5.2.2; ingen ekstern kode eller assets er kopiert.

Opt-in `-S47FinaleMovieJpeg` bruker samme avgrensede asynkrone bildekø med JPEG-kvalitet95. PNG beholdes som standard. Manifestet oppgir bildeformat og kvalitet; JPEG er en komprimert arbeidsvideo-kilde. Et faktisk HD-opptak med denne stien ga rundt24 fangede bilder/s i EVENT og uttrekk; dette er fremdeles ikke spill-FPS.
