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
- Kenney Impact Sounds, https://kenney.nl/assets/impact-sounds (`step0-2.mp3`, `thud_soft.mp3`)

Brummen, bærebølgen, telefonopptaket, smellet og motorene lages i kode med Web Audio.

## Fonter

- VT323, SIL Open Font License 1.1, The VT323 Project Authors
- Reenie Beanie, SIL Open Font License 1.1, James Grieshaber
- Oswald, SIL Open Font License 1.1, The Oswald Project Authors
- Special Elite, Apache License 2.0, Astigmatic

Fontene er innebygd uendret. Lisenstekstene finnes på https://github.com/google/fonts.

## Grafikk

Geometrien og instrument-/bevisgrafikken er laget i kode for dette prosjektet.

4. oktober 2026: 17 nye bilder laget for SIGNAL / 47 med den innebygde ChatGPT-bildegeneratoren etter `ART_BRIEF.md`. Eksakt modellversjon er ikke oppgitt av verktøyet. Dette er genererte prosjektassets, ikke nedlastede tredjepartsbilder eller en påstått CC0-lisens. Ingen eksterne betalte assettjenester er brukt.

- `room/`: `tex_floor_hextile.jpg`, `tex_ceiling_tile.jpg`, `tex_wall_paint.jpg`, `tex_desk_laminate.jpg`, `poster_listen.png`, `poster_saro.png`, `map_new_mexico.png`.
- `ext/`: `tex_concrete.jpg`, `tex_desert_ground.jpg`, `tex_asphalt_wet.jpg`, `sign_sierra_on.png`, `sign_sierra_off.png`.
- `yard/`: `tex_cabinet_metal.jpg`, `sign_service_yard.png`, `label_s03_procedure.png`.
- `sky/sky_milkyway_equirect.jpg` og `brand/logo_saro.png`.

Stier er relative til `src/assets/art/`. Prompts, kildebaner, eksportmål og SHA-256 står i `room/ROOM_TEXTURES.json`, `ext/EXTERIOR_TEXTURES.json`, `brand/GRAPHICS.json` og `sky/SKY_SIGN_PROMPTS.json`. `runtime/manifest.json` knytter de sju WebP-kopiene til originalene. Kartet er en stilisert spillrekvisitt, ikke et geografisk navigasjonskart. Tidligere lyd- og fontkreditering gjelder uendret.

Samme dato: seks miljøkonsepter i `concept/` og `maps/world-overview.jpg` er laget med den innebygde ChatGPT-bildegeneratoren. Eksakt modellversjon er ikke eksponert. Referanser er prosjektets egne spillbilder og tidligere konseptillustrasjoner, ikke nye nedlastede tredjepartsbilder. Fullstendige prompts og filhash ligger i `concept/room-concepts.json`, `concept/exterior-concepts.json` og `concept/additional-concepts.json`. `maps/*-plan.svg` er prosjektets egne redigerbare kartdiagrammer, med PNG-eksporter og kildegrunnlag i `maps/map-spec.json`. Ingen ny tredjepartslisens eller CC0-status er påstått. Se `CONCEPT_DELIVERY.md`.
