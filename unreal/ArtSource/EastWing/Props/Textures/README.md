# EASTWING-01 PBR-materialer

Åtte originale materialsett i 2048 × 2048. Seks flater fra bestillingen, pluss modulær gulvstripe og slukrist. Generert med NumPy/Pillow; ingen kjøpte teksturer, Tripo-utdata eller bildebasert falsk PBR. Det eksisterende PhotoLab-skriptet `surface_fields.py` gjenbrukes for lakkens mikrorespons og normal-konvensjon. AnyScene er bare en UNVERIFIED arbeidsflytreferanse; ingen kode eller assets derfra er brukt.

| Nøkkel | Én UV-rute i meter | Bruk |
| --- | --- | --- |
| EW_CinderblockCream | 2,4 × 2,4 | Kremmalt blokkvegg, 400 × 200 mm forband |
| EW_Stucco | 2 × 2 | Pusset fasade |
| EW_Concrete | 4 × 4 | Betonggulv med diskrete flekker, uten repeterte striper eller sluk |
| EW_GreenSteel | 1 × 1 | Grønn lakk på skap og benker; lakken er dielektrisk |
| EW_Pegboard | 0,6096 × 0,6096 | 24 × 24 hull med 25,4 mm deling og 6,35 mm diameter |
| EW_Galvanized | 1 × 1 | Sinkbelagt stål, synlige sinkkrystaller i refleksjonen |
| EW_YellowLine | 0,10 × 2 | Separat gulvstripemesh eller decal med opasitet |
| EW_DrainGrate | 0,50 × 0,30 | Egen slukrekvisitt, ikke flislagt gulvtekstur |

## Import og skala

`KEY_BaseColor.png` bruker sRGB. Roughness, Metallic, NormalDX, NormalGL, Height og eventuell Opacity er lineære datakart. Unreal bruker `NormalDX`; Blender bruker `NormalGL`. Normalmapstyrke er 1. Height er 16-bit og dekodes til meter med `offset + sample * scale`, der sample er normalisert 0..1. Eksakte verdier står i `materials.json`. Displacement skal være av til geometri, kollisjon og LOD er kontrollert i motoren.

UV0 må følge fysisk flateskala; én tilfeldig smart-UV-pakke per objekt gir ikke korrekt blokk-/hullstørrelse. Bruk fysisk UV-projeksjon eller objektkoordinater i meter delt på materialets tile-størrelse. Pegboard i nærbilde og slukristens silhuett krever faktiske geometriske hull; kartene erstatter ikke den geometrien.

Gulvstripen legges som en 100 mm bred stripe med egen UV, rundt 1 mm over gulvet eller som decal. Baneform og slukplassering bestemmes av Claude; derfor er disse ikke malt inn i det repeterende betongkartet. Slukristen er 500 × 300 mm og krever separat modell/plassering.

## Reprodusering og status

```sh
uv run --no-project --with numpy --with pillow python unreal/ArtSource/EastWing/Props/Textures/build_textures.py
```

`materials.json` gir kanalbaner, fysiske mål og hashes; hvert sett har eget `.license.json`. `validation.json` kontrollerer at alle kart er 2K og at normalvektorene er normaliserte innen kvantiseringsfeil. `PBR_ContactSheet.jpg` viser faktiske kart, ikke motor-render eller skjermbilde. Native Unreal-materialimport, lys og visuell vurdering i spillet gjenstår som del av Claudes integrasjon.
