# Tredjepartsinnhold i web/

Alt under er hentet fra Unity-prosjektet i samme repo, med samme lisens som der (se `Docs/THIRD_PARTY_NOTICES.md` i roten). Lyd er konvertert til MP3 for nettleseren.

## Musikk

- "Signal to Noise" av Scott Buckley, CC BY 4.0. https://www.scottbuckley.com.au/library/signal-to-noise/ (`title_music.mp3`)

## Lydeffekter, CC0 1.0

- old dot-matrix printer, viertelnachvier, Freesound 181420 (`printer.mp3`)
- telephonering.wav, transitking, Freesound 15826 (`phone_ring.mp3`)
- ceramic cup shatters on tile floor, geraldfiebig, Freesound 524999 (`ceramic.mp3`)
- desert_wind.wav, DarkShroom, Freesound 645305 (`wind.mp3`)
- Kenney Interface Sounds, https://kenney.nl/assets/interface-sounds (`click.mp3`, `switch.mp3`)
- Kenney Impact Sounds, https://kenney.nl/assets/impact-sounds (`thud_soft.mp3`)

### Natten, fottrinn, dører og papir (4. oktober 2026), CC0 1.0

Hentet fra Toms egne prosjekter: `Tombonator3000/morbidium` (`assets/lyd/`, kildeliste i `KILDER.md` og `lyd.json`) og `Tombonator3000/Loincloth-Legends` (`public/assets/sound/`, `KILDER.md`). Filene der er laget av Freesounds forhåndsvisning (128 kbps), trimmet, tonet inn og ut, mikset til mono og normalisert. Freesound-siden for hver fil ble åpnet 4. oktober 2026 og viser fortsatt CC0 1.0. Navnene under er filnavnene i spillet.

- crickets, FreethinkerAnon, Freesound 129678 (`crickets.mp3`, morbidium `amb_natt`)
- Buhos.wav, Gamba_Studio, Freesound 447211 (`owl.mp3`, morbidium `ugle`)
- Dogs Barking in Distance_Rural.wav, rvandemark, Freesound 581478 (`dog0.mp3`, morbidium `hund`)
- Distant Dog Bark, qubodup, Freesound 813116 (`dog1.mp3`, morbidium `hund_2`)
- wind_gust_short_sqeeeek.wav, sqeeeek, Freesound 381853 (`gust.mp3`, Loincloth-Legends `vindkast`)
- concrete footstep 2, Yoyodaman234, Freesound 166508 (`step_concrete0.mp3`, morbidium `fot_stein`)
- Concrete Footstep 2.mp3, matth3wc04, Freesound 690006 (`step_concrete1.mp3`, morbidium `fot_stein_2`)
- Cloth_And_Shoes_On_Concrete_22, BlondPanda, Freesound 778502 (`step_concrete2.mp3`, morbidium `fot_stein_3`)
- Footstep_Wood_Toe_1.wav, GiocoSound, Freesound 421153 (`step_wood0.mp3`, morbidium `fot_tre`)
- Wood step Sample 4, Notarget, Freesound 434759 (`step_wood1.mp3`, morbidium `fot_tre_2`)
- Footstep in the snow_04 [RAW], cabled_mess, Freesound 384424 (`step_dirt.mp3`, Loincloth-Legends `fot_sno`; spilles dypere som grus og jord)
- Squeaky door opened quickly.wav, CastIronCarousel, Freesound 216878 (`door_creak0.mp3`, morbidium `door`)
- Door - Creak.wav, JarredGibb, Freesound 219499 (`door_creak1.mp3`, morbidium `door_2`)
- Metal Door Slam_SoundSmith.wav, Lunardrive, Freesound 48980 (`door_metal.mp3`, morbidium `dorslag`)
- door_close, wjtaylor, Freesound 266682 (`door_close.mp3`, morbidium `dorslag_2`)
- PageTurn.wav, yatoimtop, Freesound 346835 (`paper0.mp3`, morbidium `paper`)
- Page Turn 01, LilMati, Freesound 397548 (`paper1.mp3`, morbidium `paper_2`)

Fjern torden over mesaen lages i kode (rullende lavpasset støy og en lav sinus), etter «far»-varianten av `thunderSyn` i Loincloth-Legends.

Brummen, bærebølgen, telefonopptaket, smellet og motorene lages i kode med Web Audio.

## Fonter

- VT323, SIL Open Font License 1.1, The VT323 Project Authors
- Reenie Beanie, SIL Open Font License 1.1, James Grieshaber
- Oswald, SIL Open Font License 1.1, The Oswald Project Authors
- Special Elite, Apache License 2.0, Astigmatic

Fontene er innebygd uendret. Lisenstekstene finnes på https://github.com/google/fonts.

## Grafikk til Roswell-veien og dineren

4. oktober 2026: seks originalbilder laget for SIGNAL / 47 med den innebygde ChatGPT-bildegeneratoren etter runde 6 i Claudes `ART_BRIEF.md` på `1781093`. Eksakt modellversjon er ikke eksponert. Dette er genererte prosjektassets, ikke nedlastede tredjepartsbilder eller en påstått CC0-lisens. Ingen ekstern betalt assettjeneste er brukt.

- `diner/sign_diner_blank.png`, `tex_counter_laminate.jpg`, `tex_floor_checker.jpg`, `menu_board_blank.png` og `clipping_photo_1947.jpg`.
- `road/tex_gravel_track.jpg`.

Stier er relative til `src/assets/art/`. Prompts, originale kildebaner, hash, eksporter og kontrollnotater står i `diner/DINER_SURFACES.json` og `diner/DINER_ROAD_PROPS.json`. Avisfotoet er oppdiktet for fortellingen og er ikke kopiert fra en historisk avis. Se `ART_ROUND6_DELIVERY.md`. Tidligere kreditering gjelder uendret.

## Grafikk

Geometrien og instrument-/bevisgrafikken er laget i kode for dette prosjektet.

4. oktober 2026: 17 nye bilder laget for SIGNAL / 47 med den innebygde ChatGPT-bildegeneratoren etter `ART_BRIEF.md`. Eksakt modellversjon er ikke oppgitt av verktøyet. Dette er genererte prosjektassets, ikke nedlastede tredjepartsbilder eller en påstått CC0-lisens. Ingen eksterne betalte assettjenester er brukt.

- `room/`: `tex_floor_hextile.jpg`, `tex_ceiling_tile.jpg`, `tex_wall_paint.jpg`, `tex_desk_laminate.jpg`, `poster_listen.png`, `poster_saro.png`, `map_new_mexico.png`.
- `ext/`: `tex_concrete.jpg`, `tex_desert_ground.jpg`, `tex_asphalt_wet.jpg`, `sign_sierra_on.png`, `sign_sierra_off.png`.
- `yard/`: `tex_cabinet_metal.jpg`, `sign_service_yard.png`, `label_s03_procedure.png`.
- `sky/sky_milkyway_equirect.jpg` og `brand/logo_saro.png`.

Stier er relative til `src/assets/art/`. Prompts, kildebaner, eksportmål og SHA-256 står i `room/ROOM_TEXTURES.json`, `ext/EXTERIOR_TEXTURES.json`, `brand/GRAPHICS.json` og `sky/SKY_SIGN_PROMPTS.json`. `runtime/manifest.json` knytter WebP-kopiene til originalene (sju fra PR #30, og fra 4. oktober også `lab/sign_blank.png` og `yard/floor_paint_frame.png`). Kartet er en stilisert spillrekvisitt, ikke et geografisk navigasjonskart. Tidligere lyd- og fontkreditering gjelder uendret.

Samme dato: seks miljøkonsepter i `concept/` og `maps/world-overview.jpg` er laget med den innebygde ChatGPT-bildegeneratoren. Eksakt modellversjon er ikke eksponert. Referanser er prosjektets egne spillbilder og tidligere konseptillustrasjoner, ikke nye nedlastede tredjepartsbilder. Fullstendige prompts og filhash ligger i `concept/room-concepts.json`, `concept/exterior-concepts.json` og `concept/additional-concepts.json`. `maps/*-plan.svg` er prosjektets egne redigerbare kartdiagrammer, med PNG-eksporter og kildegrunnlag i `maps/map-spec.json`. Ingen ny tredjepartslisens eller CC0-status er påstått. Se `CONCEPT_DELIVERY.md`.

Runde 3 samme dato: `yard/floor_paint_frame.png`, `lab/sign_blank.png`, `lab/tex_paper_card.jpg` og rom-6-revisjonen `concept/ch4_room6.jpg` er laget med innebygd ChatGPT-bildegenerator, med prompts og kildehash i `yard/FLOOR_FRAME.json`, `lab/LAB_SURFACES.json` og `concept/ROOM6_REVISION.json`. Eksakt modellversjon er ikke eksponert. `yard/vane_b12.png`, `yard/board_r07.png` og `lab/map_field_yard.png` er originale kodebaserte SVG-eksporter for dette prosjektet; redigerbare kilder og målkontrakt finnes i `production/`. Ingen nedlastede tredjepartsbilder inngår.

Runde 4 og 5 samme dato: `annex/tex_floor_vinyl.jpg`, `annex/vending_front.png` og de fire `station/tex_*.jpg` er laget med innebygd ChatGPT-bildegenerator etter Claudes brief. Fullstendige prompts, bevarte kildehash, eksportmål og begrensninger følger `annex/VINYL_TEXTURE.json`, `annex/VENDING_FRONT.json`, `station/STATION_SURFACES_A.json` og `station/STATION_SURFACES_B.json`. Eksakt modellversjon er ikke eksponert. Ingen ekstern betalt assettjeneste eller nedlastede tredjepartsbilder er brukt, og ingen CC0-lisens påstås. SARO-kartets oppdatering er original kodebasert SVG med PNG-eksport; det nyere kildegrunnlaget står under `saro` i `maps/map-spec.json`.

Rom 6s selvstendige geometri og enkle kodeteksturer er originale prosjektfiler. Ingen nye eksterne modeller, fotografier, varemerker eller fonter følger `src/world/Room6.ts`.

Runde 7, 4. oktober 2026: `motel/door_room_blank.jpg`, `window_night_lit.jpg`, `window_night_dark.jpg`, `tex_motel_wall.jpg`, `docs/card_field_1947.jpg` og `letter_paper_1947.jpg` er laget for prosjektet med innebygd ChatGPT-bildegenerator etter briefen på `db47e04`. Eksakt modellversjon er ikke eksponert. Det mørke vinduet er en lysvariant av den bevarte genererte kilden til det tente vinduet. Prosjektets eget motellkonsept er inspisert som stilreferanse. Ingen eksterne fotografier, betalte tjenester eller ny CC0-lisens påstås. Prompts, kildehash, eksportmål og begrensninger står i `motel/MOTEL_DOOR_WINDOWS.json` og `docs/MOTEL_PAPERS_WALL.json`. Papirene er tekstfrie spillunderlag, ikke historiske originaldokumenter.

`MotelFront.ts` har original kodebasert geometri og kodeteksturer. Den bruker også allerede krediterte oppstartsassets via den eksisterende delte bildebufferen. Ingen nye eksterne modeller eller fonter følger modulen.

Runde 8, 4. oktober 2026: `diner/tex_booth_vinyl.jpg` og `diner/tex_wall_panel.jpg` er laget for prosjektet med innebygd ChatGPT-bildegenerator etter briefen på `1a6b1f9`. Hver flate fikk én generering og én målrettet rettelse av repetisjonen. Eksakt modellversjon er ikke eksponert. Ingen eksterne fotografier, betalte tjenester eller ny CC0-lisens påstås. Fulle prompts, valgte kildekopier, SHA-256, eksportmål og begrensninger står i `production/round8-2026-10-04/manifest.json` og `ART_ROUND8_DELIVERY.md`.

`Diner.ts` har original kodebasert geometri og kodeteksturer. Den gjenbruker de fem allerede krediterte dinerbildene fra runde 6. Personene, den oppdiktede avisforsiden, skilt- og menytekst er laget i kode for prosjektet. Ingen nye eksterne modeller eller fonter følger modulen.

Runde 9, 4. oktober 2026: 19 supplerende høydeskisser er laget med innebygd ChatGPT-bildegenerator, med prosjektets egne eksisterende albedoer som referanser. Eksakt modellversjon er ikke eksponert. 38 normal-/ruhetskart er deterministisk utledet fra kildematerialenes semantiske masker og svakt, bandbegrenset støttebidrag fra skissene. De opprinnelige albedoene er uendret. Fullstendige prompts, bevarte generatororiginaler, SHA256, format, konverter og kontrollbevis følger `production/round9_manifest.json`, `production/round9_sources/` og `ART_ROUND9_DELIVERY.md`. Ingen bibliotekteksturer, eksterne fotografier eller betalte assettjenester er brukt, og ingen ny CC0-lisens påstås. Privat materialvisning gjenbruker den allerede krediterte three.js-avhengigheten.

Runde 10, 4. oktober 2026: `docs/photo_1947_master.jpg`, `docs/poster_halley_1986_blank.jpg`, `docs/fanfold_1986.jpg` og `docs/fanfold_1947.jpg` er fire originale, oppdiktede dokumentbilder laget for SIGNAL / 47 med innebygd ChatGPT-bildegenerator; tekniske komposisjonsguider og mål-, sRGB- og JPEG-eksport ved Codex. Ingen ekte avis, personer, merkenavn eller eksternt bildearkiv. Promter og originale kilder ligger i `production/round10_qa/sources/`, sjekksummer i `production/round10_manifest.json`. Eksakt modellversjon er ikke eksponert, og bildene er ikke hevdet CC0. Teksten på plakaten og papirene er tegnet i kode for prosjektet. Se `ART_ROUND10_DELIVERY.md`.
