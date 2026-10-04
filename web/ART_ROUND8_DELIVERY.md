# Grafikkleveranse, runde 8

To nye dinerflater er produsert etter `ART_BRIEF.md` fra commit `1a6b1f98db0e85b550c5cd9f714405a95af201f8`. Alle eksporter er RGB JPEG med kvalitet 85 og innebygd sRGB-profil. Filkontroll og visuell gjennomgang er fullført. Panelet har en dokumentert begrensning ved vertikal repetisjon.

| Fil | Mål | Flate per UV 0 til 1 | Bytes | Visuell status |
| --- | --- | --- | ---: | --- |
| [tex_booth_vinyl.jpg](src/assets/art/diner/tex_booth_vinyl.jpg) | 512 × 512 | 0,5 × 0,5 m | 54 802 | PASS ved vanlig 2 × 2-repetisjon |
| [tex_wall_panel.jpg](src/assets/art/diner/tex_wall_panel.jpg) | 1024 × 1024 | 2,0 × 2,0 m | 180 536 | Akseptert med små fiberavvik ved vertikal repeat |

[Manifestet](src/assets/art/production/round8-2026-10-04/manifest.json) inneholder fulle prompts, festet brief med hash, fire genereringssteg, originalbaner, kilde- og eksporthasher, bytes, fargerom, mål, skalering, UV-kontrakt og kontrollresultater. Eksakt generatormodell er ikke eksponert av verktøyet.

Grafikken er laget med innebygd ChatGPT-bildegenerering. Hver flate fikk én ny generering og én målrettet repeat-rettelse. Deretter er hele bildet skalert med LANCZOS og eksportert. Ingen pixelretusjering, beskjæring, tekst eller tegnede reparasjoner er gjort via kode.

Første vinylkandidat hadde en synlig overgang i knapper og tone. Første panelkandidat manglet ytterfuge og ga en dobbelt bred planke ved vannrett repeat. Begge ble korrigert med bildeverktøyet. Den valgte vinylen har et kontinuerlig knappet rutemønster uten vesentlig tonal eller geometrisk skjøt. Panelet har nå vanlig bordbredde og ingen vesentlig tonal skjøt, men små brudd i trefibrene kan ses tett på ved øvre og nedre repeat-grense. Fullstendig sømløshet er derfor **ikke** godkjent for panelet. Svak panelaldring er fordelt over materialet; en stedsspesifikk mørk stripe ved gulvet eller kjøkkenluka er ikke bakt inn.

Begge kontrollark er laget av fire uendrede slutteksporter med vanlig repetisjon, uten speiling eller blanding. De er inspisert av både produserende agent og root:

- [Kunstskinn, 2 × 2](src/assets/art/production/round8-2026-10-04/booth-repeat-preview.jpg)
- [Furupanel, 2 × 2](src/assets/art/production/round8-2026-10-04/wall-panel-repeat-preview.jpg)

De to valgte generatororiginalene er bevart byte for byte i repoet som [source-booth-vinyl.png](src/assets/art/production/round8-2026-10-04/source-booth-vinyl.png) og [source-wall-panel.png](src/assets/art/production/round8-2026-10-04/source-wall-panel.png), begge 1254 × 1254. De opprinnelige verktøyfilene og de første kandidatene ligger også urørt under `/home/tombonator3000t/.codex/generated_images/01a105a9-8c63-7183-9dba-5802782c084b/`; manifestet angir hvert filnavn og hver SHA-256. De profilerløse RGB-kildene er behandlet som sRGB ved eksport, uten fargekorrigering.

Teknisk PASS: eksakte dimensjoner, JPEG-format, RGB, sRGB ICC, kvalitet-85-kvantisering, kilde- og eksporthasher, byteidentiske kildekopier og kontrollarkenes mål/hasher. Runtimebruk, materialrespons, lys og spilltester er UNVERIFIED i denne grafikkdeloppgaven. Root og kodeagent håndterer Diner-forhåndsvisning og senere innkobling. Ingen eksisterende filer, felles krediteringsfiler eller runtimekode er endret av grafikkleveransen. Ingen commit eller push er gjort her.
