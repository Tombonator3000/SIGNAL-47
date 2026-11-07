# Kilder og lisens, ekte terreng (7. november 2026)

## Kilde
- **USGS 3DEP 1/3 buesekund** (ca. 8,7 x 10,3 m), flisene `n33w107` og `n33w108` (`n33w106` ble også lastet ned for kartleggingen av kandidater).
  - Nedlasting: `https://prd-tnm.s3.amazonaws.com/StagedProducts/Elevation/13/TIFF/current/<flis>/USGS_13_<flis>.tif` (7. november 2026).
  - Lisens: USGS-produkter er offentlig eiendom (public domain). Kreditering er ikke påkrevd; anbefalt tekst: «Elevation data: U.S. Geological Survey, 3D Elevation Program».
- Rå GeoTIFF og mellomfiler ligger i `raw/` (ca. 2,4 GB, kan slettes; kan gjenskapes fra URLene).

## Valgt sted
UTM 13N (EPSG:26913) senter E 299000, N 3588000 (ca. 32,41 N, 107,14 V). Sørvestlige New Mexico, bred slette med arroyoer og vifter, lave fjell 5 til 12 km mot vest/nordvest og nord. Stedsnavn er ikke verifisert. Alternativer B (Doña Ana-bajada) og C (Tularosa vest) står i `preview/kandidat_*.png`.

## Filer
| Fil | Innhold |
|---|---|
| `dem/near_4km_2m_h16.{raw,png,json}` | Nærkart 2017 x 2017 punkter, 2 m, utstrekning 4032 m, høyde 1397,2 til 1453,2 m (56 m) |
| `dem/far_24km_10m_h16.{raw,png,json}` | Fjernkart 2401 x 2401 punkter, 10 m, utstrekning 24000 m, 1291 til 2020 m |
| `baked/{near,far}_mask_{slope,flow,lowhigh,height}.png` + `*_masks.json` | 8-bit masker, samme oppløsning og utsnitt som høydekartene (formler i JSON) |
| `baked/far_ao.png`, `far_lightmask_sun250_25.png`, `far_light_bake.json` | AO og sollysmaske for fjernkartet (kun terreng) |
| `mesh/bg_mountains_ring_lods.{fbx,json}` | Ringmesh med tomt indre 4032 m, LOD0 486 560 trekanter (celle 48 m), LOD1 121 808 (96 m), LOD2 30 536 (192 m) |
| `preview/` | Skyggelagte bilder med skala |
| `tools/` | Alle skript (Blender-python, numpy; PIL kun for etiketter) |

Raw: uint16 little-endian, rad 0 = nord, meter = `zmin_m + v/65535*(zmax_m-zmin_m)`. Punktene er vertekser (første og siste punkt ligger på kartkanten).

## Hva er data og hva er laget
- Fjernkartet er ren 3DEP, bilineært resamplet til UTM, ingen tillegg.
- Nærkartet er 3DEP oppsamplet fra ca. 10 m til 2 m. Alt under ca. 10 m er **laget, ikke målt**: fBm-støy skalert etter lokal helning (rms 0,09 m, maks total endring 0,97 m), hydraulisk rørmodell-erosjon (400 iterasjoner, endring lavpassfiltrert og klemt til ±0,6 m, rms 0,12 m) og termisk erosjon (talus 32 grader, 4 iterasjoner). Detaljen tones ut mot kartkanten (150 m), så kanten er lik kilden.
- **Vertikal skala er 1,0.** Ingen overdrivelse. Preview-bilder merket vz x2/x3 er kun for visning.
- Helningsmedian i nærkartet er 0,97 grader i kilden og 1,1 grader med detalj (første versjon med mer støy ga 2,2 grader og ble forkastet som for kornete).

## Søm og mål
- Mesh-hullet er 4032 m, lik nærkartets utstrekning. Største høydeforskjell mellom meshkanten og nærkartets kant er 0,14 m (snitt 0,012 m). 4 m skjørt i hullet skjuler sprekker.
- Mesh: 1 Blender-enhet = 1 m, `z=0` tilsvarer 1398 m o.h.; FBX med enhetsskala (UE: 100 uu per m). UV 0..1 over hele 24 km (samme utsnitt som `far_*.png`). Vertex color «AO».
- UE: nærkartets Landscape Z-skala 10,94 % med 16-bit-kodingen over; formel i JSON.
- LOD3 er utelatt fordi hullet (4032 m) og 24000 m bare tillater celler 48/96/192 m.

## Kritikerrunde (sonnet-agent, så 5 bilder og referanse 01-03)
- Regionalt riktig: slette, viftelignende fall, fjell i nord/vest. Dataene er ekte.
- Nærkartet er likevel nesten en jevn skråning. Mesoskala-form (arroyoer 1-3 m dype, kanter, rygger, vifter) mangler eller er svak; spillområdets relieff (56 m over 4 km, under 0,4 grader regional helning) leses som flatt fra bakken.
- Første støyversjon var isotrop «sandpapir»; den er redusert og modulert etter helning, men er fortsatt prosedyral og ikke strømstyrt.
- Fjell i ringen: nord-silhuetten er best (bred, lav kuppel), øst/vest/sør er lave bånd 1-3 % av bildehøyden; mesh er glatt og facettert, uten dis. Vertikal skala beholdt 1,0 (kritikeren foreslo 1,5-2 x; ikke gjort).
- Forbedringer rangert: (1) bygg arroyo-nettverk fra ekte flytretning (carve), (2) streng/strømstrukket støy, (3) dis og blåtone, vegetasjon (tuer, busker) og grus-/calichetekstur i materialet.
