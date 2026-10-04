# Grafikk til SARO-arkivet og STATION 01

4. oktober 2026. Avtalt i Claude-økten `session_01LVHLZRk2YVK6KDkobNYQQv`; prioritering og filansvar fra `todo.md` på `1a08434`. Grafikk fra rundene 4 og 5 i `ART_BRIEF.md` er levert i egne kategorier, med lokalt galleri i `src/assets/art/production/round45.html`.

| Fil under `src/assets/art/` | Eksport | Fysisk mapping |
| --- | --- | --- |
| `annex/tex_floor_vinyl.jpg` | 1024 × 1024, RGB-JPEG85, sRGB | 1,2 × 1,2 m, fire ganger fire fliser på nominelt 30 cm |
| `annex/vending_front.png` | 512 × 1024, opak RGB-PNG, sRGB | 0,74 × 1,48 m |
| `station/tex_stucco_wall.jpg` | 1024 × 1024, RGB-JPEG85, sRGB | 2,0 × 2,0 m |
| `station/tex_concrete_old.jpg` | 1024 × 1024, RGB-JPEG85, sRGB | 1,0 × 1,0 m |
| `station/tex_wood_weathered.jpg` | 1024 × 1024, RGB-JPEG85, sRGB | 0,5 × 1,0 m, fiber langs bilde-Y |
| `station/tex_floorboards.jpg` | 1024 × 1024, RGB-JPEG85, sRGB | 1,2 × 1,2 m, ti bord, fiber langs bilde-X |

Alle seks er laget med den innebygde ChatGPT-bildegeneratoren. Eksakt modellversjon er ikke eksponert. Fullstendige prompts, kilde- og leveransehash, eksportmål og QA står i `annex/VINYL_TEXTURE.json`, `annex/VENDING_FRONT.json`, `station/STATION_SURFACES_A.json` og `station/STATION_SURFACES_B.json`. Genererte originaler og forkastede varianter er bevart lokalt på manifestenes kildebaner. Skalering og formatkonvertering er dokumentert; ingen visuell retusjering med kode er brukt. Alle fem vanlige 2 × 2-kontrollbilder følger PR-en, uten speiling.

## Innkobling hos Claude

- Legg bare de seks eksportfilene til i `core/art.ts`, etter eksisterende preload/TRY AGAIN-mønster. Konsepter, kart, manifests og kontrollbilder skal fortsatt holdes utenfor spillpakken.
- Behold vinylens eksisterende repeat `floorW / 1.2, floorD / 1.2`. Se den ferdige flaten i lys før materialtint fastsettes; eksisterende gulvfarger kan mørkfarge bildet en gang til.
- Tegn `COLD DRINKS` i kode. På dagens 256 × 512-canvas anbefales midtstilt anker `(116, 491)` og omtrent 22 px skrift, maksbredde 187 px. Dagens baseline 498 ligger for nær panelkanten. Detaljer og alternativt tekstfelt står i `VENDING_FRONT.json`.
- Gulvbordet har ti bord i sluttvarianten og bruker opprinnelig 1,2 × 1,2 m mapping. Den tidligere diskusjonen om åtte bord og 0,96 m repeat gjelder en forkastet variant.
- Store PNG-er kan få runtime-WebP gjennom prosjektets eksisterende verktøy. Originalene beholdes.

## Materialkontroll og begrensninger

Alle fem repeat-ark og automatfronten er visuelt inspisert. Pussen har en svak smal overgang i finstrukturen ved nærstudium, uten vesentlig tonal kant; dette er merket `PASS MED MERKNAD`. Treverket har små fiberforskyvninger ved nærstudium. Repetisjon er visuelt kontrollert, ikke garantert pikselidentisk langs motstående kanter.

Gulvbordene er omtrent like brede: 93–106 piksler mot nominelt 102,4, rundt 10,9–12,4 cm ved bestilt mapping. Ti-bordsantallet er oppfylt; matematisk eksakt 12 cm for hvert bord er ikke oppfylt. Portrettkilden er skalert samlet til kvadrat. Ingen ekstra UV-unntak trengs.

## SARO-kart

`maps/saro-plan.svg` og PNG-kopien viser nå den bygde sørkorridoren og arkivet i main `3264fab`. Døråpninger, fysisk gulv og gangbare soner er skilt. Ganglinjen går gjennom begge faktiske døråpninger, og stengte bakgrunnsrom er skravert. Kartet er en produksjonsoversikt, ikke en ny navigasjonskontrakt eller en full kollisjonstest.

Fem aktuelle kildehash står under `saro` i `maps/map-spec.json`. De andre planforslagene og deres historiske kildegrunnlag er bevart byte-for-byte. `render_maps.py` regenererer bare SARO og bevarer STATION 01/motell.

## Kontroller

Teknisk assetkontroll følger `evidence/support-2026-10-04/asset-verification.json`. Nettlesergalleriet er kontrollert ved 1280 × 800 og 390 × 844: alle seks bilder lastet og ingen vannrett overflow. Uredigerte bilder og konsollresultat ligger i samme evidence-mappe.

Denne leveransen endrer ikke spillets bildelaster eller kapitler. Claude integrerer flatene og eier den samlede spilltesten. Fysisk mobil, Safari, faktisk fps, GPU-minne på telefon og subjektiv lydmiks er fortsatt uverifisert her.
