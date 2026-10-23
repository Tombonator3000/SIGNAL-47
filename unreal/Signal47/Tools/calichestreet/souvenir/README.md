# Saucer Crash 1947

Original butikkmodell til den samlede Caliche-gatebiten, etter `Concepts/Caliche-2026-10-23/04-suvenirbutikk.png`. Krempuss, olivenmalt stengt dør, moderat slitt treverk, bølgeblikktak over portal, håndmalt skilt og en reell innredet butikkdybde. Ingen ny åpen dør eller spillfunksjon.

`Saucer_Crash_1947.blend` er kilden. De navngitte originalkomponentene ligger i den skjulte samlingen `EDITABLE_SOURCE_enable_to_edit`; skjul de tre `SM_Souvenir_*`-objektene før denne vises. Separate skall-, interiør- og glassmesh ligger i `exports/`. Samlet GLB har de samme tre delene. Alle deler bruker samme nøytrale pivot og eksplisitte trekanter uten nullareal.

Innredningen omfatter trehyller med dybde og braketter, mekanisk kasse med runde taster, papirposer og kvittering, postkortstativ med originale trykte ørkenmotiver, kart, brosjyrer, keramikkmugger, billige UFO-modeller og folieposer med «GENUINE DEBRIS». UFO-er er leketøysfat, ikke fremmede figurer. Den eneste alien er fortsatt spillets opprinnelige A02-pappfigur med egen pris og skade.

## Koordinatkontrakt

Blender bruker meter og Z opp. Felles Three.js-pivot er `[8025, 4.952807474136353, 2064]`; `worldThree=(8025+localY, floor+localZ, 2064+localX)`. FBX bruker `-Y` fremover og `Z` opp. Unreal-importen følger den tidligere radiostasjonen, med runtime yaw `-90`.

Strukturell fasade ligger ved lokal Y `-4.5`, altså world X `8020.5`. Bygget er 12 meter langs gaten og 9 meter dypt, med parapet på høyde 4,05 meter. Det originale vinduet har senter `[-1.7,-4.5,1.45]` og lysåpning `2.2 × 1.7`. Døren beholder senter `[2.8,-4.5,1.1]`. Skiltets senter er `[.1,-6.3,3.60]`, tilsvarende eksisterende skiltproxy. Canopy-stolpene følger de tre eksisterende ankerne.

Glasset ligger ved world X `8020.24`, foran A02-figuren ved `8020.31`, i en dyp utstikkende trekarm. Bevar figuren, prislappen, skaden og deres eksisterende runtime-logikk. Den gamle mørke displayflaten kan skjules av integrasjonen for å vise rommet; treffproxyen skal bevares. Glass skal ikke få runtimekollisjon. Figurens siktvolum foran den lave utstillingsbenken holdes fri.

`model_contract.json` gir faktiske lokale grenser, materialrekkefølge, UV-dekning og trekanttall etter generering. Felles terreng, nivåoverganger, belysning og gamle meshtrim håndteres av hovedoppgaven.

## Materialer og verifisering

`materials.json` bruker `base_color_linear`, metall, ruhet, emissjon og relative PBR-stier. Basecolor-PNG er sRGB; roughness er lineær; Unreal-normaler bruker DirectX. Blender og GLB bruker egne OpenGL-normalbilder fra samme originale høydefelt. Spillglasset har alpha 0,10. Praktiske lysflater har mild emissjon; faktiske Unreal-lys settes av integrasjonen.

```sh
blender -b --factory-startup --python build_souvenir.py
blender -b --factory-startup --python verify_souvenir.py
```

Render er uttrykkelig opt-in med `-- --render` og skal bare kjøres når den felles grafikkøen er fri. Kameraer finnes for utvendig visning, interiøret og sikten gjennom vinduet fra fortauet. Blender-renderinger er kildeforhåndsvisninger, ikke runtimebevis. Dag, skumring og natt prøves i den samlede gatebiten av hovedoppgaven.

Prosjektbiblioteket ga en generell historisk `bpy-dev/blender-mcp`-profil med Blender 4.5-teststatus, men ingen kompatibel butikkmodell. Ingen kode eller assets derfra ble gjenbrukt. Modelløypen bygger på prosjektets egen, tidligere verifiserte Blender 5.2-radiostasjonseksport og kopiert original nullarealrensing. All ny modellgeometri og teksturer er egne; konseptbildet er visuell referanse og er ikke innebygd.
