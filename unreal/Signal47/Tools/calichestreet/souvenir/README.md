# Saucer Crash 1947

Original butikkmodell til den samlede Caliche-gatebiten, etter `Concepts/Caliche-2026-10-23/04-suvenirbutikk.png`. Nytt konseptpass 24. oktober: 10,65 meter bred mørkerød malt fasadetittel, avskallet kalkpuss, sokkelpatina, rustet bølgeblikk og grått råsaget treverk under taket. Olivenmalt stengt dør og reell innredet butikkdybde beholdes. Ingen ny åpen dør eller spillfunksjon.

`Saucer_Crash_1947.blend` er kilden. De navngitte originalkomponentene ligger i den skjulte samlingen `EDITABLE_SOURCE_enable_to_edit`; skjul de tre `SM_Souvenir_*`-objektene før denne vises. Separate skall-, interiør- og glassmesh ligger i `exports/`. Samlet GLB har de samme tre delene. Alle deler bruker samme nøytrale pivot og eksplisitte trekanter uten nullareal.

Innredningen har hyller med ulike bredder, høyder og ujevn varefylling, mørkere tre, korte gulvbord, mekanisk kasse med runde taster, papirposer og kvittering. Postkortstativet har flere originale trykk, kart, brosjyrer, keramikkmugger, UFO-leker, mineralbokser, brettede tekstiler og krøllete folieminner med «GENUINE DEBRIS». Vevd veggheng, souvenirskjorte og flettet gulvmatte gir større lesbare flater mellom småvarene. Ingen ny alien er laget; spillets opprinnelige A02-pappfigur med egen pris og skade beholdes av runtime.

R3 bygger videre på første native dag-/skumringsbilder: fire fylte hyllehøyder, både fremre varegrupper og bakenforliggende pakkvarer, runde souvenirflasker, flere mugger og oppstilte reisebøker. Runde kopper har 56 segmenter og glatte normaler; UFO-formene bruker 40 × 20-segments glatte sfærer. De gamle vektorpostkortene er erstattet av trykkatlas. Sokkelarr er lysere beige, teksturerte og har små bruddkanter; den nesten svarte bunnstripen er dempet. Tre fysiske skiltlamper har diffuserankre på Blender `[-3.7,-5.05,4.10]`, `[0,-5.05,4.10]`, `[3.7,-5.05,4.10]`. Integratoren eier de faktiske lysene og tidstilstanden.

## Koordinatkontrakt

Blender bruker meter og Z opp. Felles Three.js-pivot er `[8025, 4.952807474136353, 2064]`; `worldThree=(8025+localY, floor+localZ, 2064+localX)`. FBX bruker `-Y` fremover og `Z` opp. Unreal-importen følger den tidligere radiostasjonen, med runtime yaw `-90`.

Strukturell fasade ligger ved lokal Y `-4.5`, altså world X `8020.5`. Bygget er 12 meter langs gaten og 9 meter dypt, med parapet på høyde 4,05 meter. Det originale vinduet har senter `[-1.7,-4.5,1.45]` og lysåpning `2.2 × 1.7`. Døren beholder senter `[2.8,-4.5,1.1]`. Det frittstående TOURS-boardet er fjernet for å frigjøre fasadetittelen. Nytt visuelt skiltmål er `[0,-4.52,3.38]`; det tidligere proxyankeret `[.1,-6.3,3.60]` er bevart som metadata, slik at integratoren kan flytte målet uten å endre ID/historietekst. En liten TOURS-plakat ligger i venstre øvre vindushjørne ved `[-2.52,-4.32,1.98]`, utenfor A02-silhuetten. Canopy-stolpene følger de tre eksisterende ankerne.

Glasset ligger ved world X `8020.24`, foran A02-figuren ved `8020.31`, i en dyp utstikkende trekarm. Bevar figuren, prislappen, skaden og deres eksisterende runtime-logikk. Den gamle mørke displayflaten kan skjules av integrasjonen for å vise rommet; treffproxyen skal bevares. Glass skal ikke få runtimekollisjon. Figurens siktvolum foran den lave utstillingsbenken holdes fri.

`model_contract.json` gir faktiske lokale grenser, materialrekkefølge, UV-dekning og trekanttall etter generering. Felles terreng, nivåoverganger, belysning og gamle meshtrim håndteres av hovedoppgaven.

## Materialer og verifisering

`materials.json` bruker `base_color_linear`, metall, ruhet, emissjon og relative PBR-stier. Basecolor-PNG er sRGB; roughness er lineær; Unreal-normaler bruker DirectX. Blender og GLB bruker egne OpenGL-normalbilder fra samme originale høydefelt. Spillglasset har alpha 0,10. Praktiske lysflater har mild emissjon; faktiske Unreal-lys settes av integrasjonen.

Seks originale PBR-sett er nå 2048 × 2048: kalkpuss, betong, mørk valnøtt, diskvalnøtt, værslitt tre og oksidert metall. `make_textures.py` tegner langsgående trefibre, vekstknuter, lokal slitasje, trowelmerker, pussbrudd, kapillærsprekker og rustøyer. Det bruker ingen fotopiksler fra konseptet, sinusfelt eller en homogen støymasse. `texture_provenance.json` dokumenterer metoden. Høydekart lagres som kilde, men importeres ikke som ekstra runtimekart. Pillow er en lokal forfatteravhengighet, ikke en spillavhengighet.

Kun `M_Souvenir_PrintedArtwork.base_color` er AI-generert bitmapkunst. Den innebygde imagegen-skillen laget det originale 1254 × 1254-atlaset med fire trykk, uten inputbilder. Uendret original ligger i `source_art/souvenir_print_atlas_v3.original.png`, den eksakte prompten i `source_art/print_atlas_v3.prompt.txt`, og felt-/hashproveniens i `print_art_provenance_v3.json`. Runtimefilen i `textures/` er byteidentisk med originalen. Pris-/pakkeetiketter er egne deterministiske tekstkort i `make_merch_labels.py`, med 2048 × 1024-atlas og labels i `label_atlas.json`. Ingen brukerfoto, VLA-foto eller tredjepartkunst er brukt som trykktekstur.

```sh
python3 Tools/calichestreet/souvenir/run_guarded.py build
python3 Tools/calichestreet/souvenir/run_guarded.py verify
```

Render er uttrykkelig opt-in med `-- --render` og skal bare kjøres når den felles grafikkøen er fri. Kameraer finnes for utvendig visning, interiøret og sikten gjennom vinduet fra fortauet. Blender-renderinger er kildeforhåndsvisninger, ikke runtimebevis. Dag, skumring og natt prøves i den samlede gatebiten av hovedoppgaven.

`run_guarded.py preview` gir to små 720 × 540-forhåndsvisninger med åtte samples. Guard kontrollerer Unreal-kø/minne og stopper kun sin egen prosess hvis en ekstern Unreal-jobb starter. `authoring_guard.json` viser faktisk kjøring. `verify --baseline /sti/til/godkjent.blend` sammenligner i tillegg dør, åpninger, rammer og glass direkte mot en tidligere kilde. `concept_revision.json` oppgir trekant-, materialslot- og boundsendringer; runtimekvaliteten for den nye revisjonen forblir UNVERIFIED til integratoren har tatt nye bilder.

Prosjektbiblioteket ga en generell historisk `bpy-dev/blender-mcp`-profil med Blender 4.5-teststatus, men ingen kompatibel butikkmodell. Ingen kode eller assets derfra ble gjenbrukt. Modelløypen bygger på prosjektets egen, tidligere verifiserte Blender 5.2-radiostasjonseksport og kopiert original nullarealrensing. All ny modellgeometri og teksturer er egne; konseptbildet er visuell referanse og er ikke innebygd.
