# Caliche, originalt vegetasjonssett CN3

Gjeldende leveranse er `CN3-masked-small-leaf-needle-sprays`: tolv originale prototyper med tre forfattede detaljnivåer hver. Faktisk Blender-bygg, FBX-kontroll og GLB-kontroll er fullført med **270/270 kontroller PASS**. Den ferdige `botanical_preview.png` viser sage og einer ved LOD0/1/2 i samme skala per familie, sammen med gress, yucca og stein. Forhåndsvisningen viser finere blad- og nålekonturer; visuell aksept i den faktiske spillverdenen gjenstår.

## Innhold og geometri

`caliche_vegetation.blend` har et LOD0-galleri, en egen samling for LOD1/2 og skjulte, separate originalkomponenter i `EDITABLE_original_components`. `caliche_vegetation.glb` er et portabelt galleri. `exports/` inneholder 36 FBX-filer og 36 separate GLB-filer; hovednavnet, for eksempel `SM_CN_Sage_A`, er LOD0, med `_LOD1` og `_LOD2` som egne filer. `model_contract.json` beskriver tolv hovednavn i `parts`, hvert med tre oppføringer i `lods`, faktiske bounds, trekanttall, UV og materialrekkefølge.

| Familie | Varianter | Utformet høyde, m | Utformet bredde, m | Trekanter LOD0 / LOD1 / LOD2 |
|---|---|---|---|---|
| Sage | A / B / C | 0,45 / 0,62 / 0,34 | 1,00 / 1,25 / 0,84 | 6128 / 1800 / 504 |
| Gresstue | A / B | 0,43 / 0,64 | 0,62 / 0,88 | 2100 / 460 / 144 |
| Yucca | A / B | 0,78 / 1,05 | 1,32 / 1,64 | 1046 / 346 / 88 |
| Einer | A / B | 1,65 / 2,65 | 2,35 / 3,50 | 11388 / 3172 / 1104 |
| Stein | Caliche A / B, Sandstone A | 0,19 / 0,37 / 0,52 | 0,64 / 0,88 / 1,20 | 88 / 64 / 40 |

Tabellen viser faktiske eksporterte trekanttall. Uregelmessige silhuetter kan gi annen eksakt bredde i én akse; kontraktens `bounds_local_m` er autoritativ. Hver separat eksport har bakken ved lokal Z0, pivot ved origo og høyst to materialplasser. Kilden bruker meter og Blender Z opp; FBX eksporteres med -Y frem og Z opp. Galleriobjektene er flyttet fra hverandre kun i samlet Blender-/GLB-galleri.

Sage og einer har lave vedstrukturer og krumme, foldede 3D-skudd med mange små maskerte blad eller nåler. Grove nivåer beholder større skudd gjennom kronen. Gress og yucca bruker buede, avsmalnende geometriske blad; steinene bruker kantete, uregelmessige bruddflater. Plantefamiliene er visuelle tolkninger av konseptet, ingen verifisert artsbestemmelse.

## Originale materialer og teksturer

Tre delte materialnavn beholdes: `M_CN_Foliage`, `M_CN_Bark` og `M_CN_Stone`. Åtte originale PBR-PNGer følger leveransen: et 2048 × 1024 botanisk atlas og et 512 × 512 barkkart, hver med basefarge, ruhet og separate DirectX-/OpenGL-normaler. Atlasets åtte ruter gir sølvgrå sage, strå, olivengrønn yucca, to einerfarger, tørre hylstre, beige caliche og brun sandstein. Fargene har moderat variasjon uten innbakt belysning eller AO.

CN3 har perforert RGBA i atlasrute 0, 3 og 4. `masked_atlas.py` tegner småblad, nåler og fine kvister; transparente piksler beholder bladfarge for filtrering langs kanten. Øvrige ruter har alpha255. Geometri, UV-er, pivoter og trekanttall er beholdt fra CN2, mens løvflatene har glatte normaler.

`M_CN_Foliage` er tosidig og bruker binær maskering: `opacity_from_base_color=True`, `alpha_mode=masked`, `alpha_cutoff=0.333` og mipdekning i native. Blender-kilden kobler teksturens alpha gjennom Math GREATER_THAN til Principled Alpha; faktisk eksportert glTF bruker MASK. Bark og stein er opaque. Ingen planter bruker translucent materiale eller et billboard for hele planten. `materials.json` gir relative teksturstier, nøytral basefargemultiplikator, metallic0, ruhet0,84 og normalstyrke0,20; native bruker DirectX-kart, Blender/GLB OpenGL-kart.

## Kontroll og reproduksjon

`verification.json` dokumenterer Blender 5.2.2 og 270/270 beståtte kontroller. Alle 36 faktiske FBX-filer er tilbakeimportert og kontrollert for eksakte trekanttall, meter/pivot/bounds, materialrekkefølge, endelige normaler og UV-er, samt minste trekantareal minst 5 × 10⁻⁹ m². Alle 36 faktiske GLB-filer er kontrollert for indekserte trekanttall og material-/alfakoding. `alpha_texture_verification.json` kontrollerer både kilde-PNG og innebygde GLB-bilder for RGBA, perforerte løvruter, solide øvrige ruter, MASK/cutoff/tosidighet og opaque bark/stein. Den lagrede Blend-kildens shader og redigerbare komponenter er også kontrollert.

Kjør fra denne mappa, først etter eksplisitt tildelt tungt jobbvindu:

```sh
python3 run_guarded.py build
python3 run_guarded.py verify
python3 run_guarded.py preview
```

Wrapperen bruker prosjektets strenge guard uten grace, maksimalt to Blender-tråder, eksplisitt Python-feilkode og faktisk rapportstatus. Bygg regenererer teksturene med Pillow i eksisterende Python-runtime; `S47_VEGETATION_PYTHON` kan angi en annen runtime med Pillow. Ikke bruk `--reuse-textures` med gamle CN2-RGB-kart; CN3 krever RGBA. Preview er en separat jobb og endrer ingen kildemodell eller eksport. `asset_manifest.json` inneholder filstørrelser og SHA256.

Teknisk PASS beviser ikke naturlig lesbarhet, native materialtilordning eller stabile LOD-/mip-overganger. HISM-materialbruk, plassering, avstander, overganger og dag/skumring/natt må kontrolleres i faktiske spillbilder. Det geometriske projeksjonsestimatet fra `analyze_canopy.py` ignorerer alpha og er ikke et CN3-tetthetsbevis. Native import, terreng, plassering og himmel eies utenfor denne mappa.

## Kildevalg og tidligere iterasjoner

Prosjektbiblioteket ga ingen egnet, verifisert vegetasjonsoppskrift. Lokale CC0-kilder Grass Medium 02 og Boulder 01 fra Rico Cilliers/Poly Haven ble kontrollert som alternativer, men ingen tredjepartsmodell eller tekstur er brukt; `source_audit.json` dokumenterer gjennomgangen. Modellene og teksturene her er originalt generert fra de medfølgende kildene.

CN1 ble visuelt avvist for for glisne kroner; hele revisjonen er bevart med SHA-manifest i `/home/tom/scratch-bm/Signal47-caliche-nature/vegetation-CN1`. CN2 fikk tettere kroner, men ble avvist for store papiraktige bladflater, og er bevart tilsvarende i `/home/tom/scratch-bm/Signal47-caliche-nature/vegetation-CN2`.
