# SIGNAL / 47 – designbibel, arkiv 27. oktober 2026

[Les den leverte PDF-en](SIGNAL-47-Designbibel.pdf) · [Redigerbar designbibel](DESIGNBIBEL.md) · [Visuell retning](visuell-retning/VISUELL-BIBEL.md) · [Kart](KART.md)

Dette er leveransen fra samtale `01a17d06-3c66-78f3-b21b-b784af2cc91a`, samlet fra den lokale `designbibel`-mappen. PDF-en har **42 A4-sider** og er kopiert byteidentisk. Den er ikke bygget på nytt ved arkivering. Eldre [DesignBible13](../DesignBible13/design-bible.md) er bevart som historikk.

- PDF: `10988346` byte.
- PDF SHA-256: `f8cc0f2d7784098ec7dc8c4eaa633d72aadca0da76d053e2827b72163087fda5`.
- [Kildemanifest](archive-source-manifest.json) registrerer opprinnelige filer, SHA-256 og arkivstier. [Arkivmanifest](manifest.json) registrerer filene etter portabilitetstilpasning og avhengighetene.
- Markdown, fire SVG-er, bildeprompter og konsept-PNG-er er bevart som redigerbart grunnlag. PDF-bygger og kartgenerator følger med.

## Hvordan materialet skal brukes

Boken samler eksisterende verdenskilder, historieplan og visuelle produksjonsforslag. Kildestatus er datert **27. oktober 2026**; ord som «dagens», «under arbeid» og «nye» beskriver tidspunktet materialet ble laget. Prosjektets nyere statusfiler og bekreftet spilladferd må brukes ved implementering.

Konseptbilder og layoutforslag er visuelle mål. De er ikke bevis for ferdig modellering, riktig kjøretøygeometri, tilgjengelige interiører eller funksjon i en pakket spillutgave. De to modellrenderne viser arbeidsmodeller. Kildekartene viser ankerpunkter fra en sceneeksport, ikke oppmålte bygningsomriss, terrenghøyder eller en godkjent flytting av verdenen. Historieplanen dokumenterer heller ikke målt spilletid. Arkiveringen endrer ingen kanon og overstyrer ikke nyere Unreal-kilder.

Bildeavhengigheter som manglet i repoet er arkivert under `assets/`. Fem byteidentiske filer gjenbrukes fra eksisterende repoarkiv, blant annet ku-konseptene. UI-galleriet tar med historiske sammenligningsbilder og tilhørende prompter/manifester, uten private chatarkiv eller rå logger. [Avhengighetskartet](asset-dependencies.json) viser hver konkret fil.

Bildeprompter står i [BILDEPROMPTER.md](visuell-retning/BILDEPROMPTER.md). Detaljer om avvik i de genererte bildene står i [VISUELL-BIBEL.md](visuell-retning/VISUELL-BIBEL.md). Det følger ikke med vannmerkede tredjeparts referansefotografier i denne mappen.

## Bygge fra kildene senere

Kjør fra en full utsjekk av repoet med de oppførte bildeavhengighetene tilgjengelige:

```bash
python3 Docs/DesignBible-2026-10-27/pdf-produksjon/bygg_pdf.py
```

Byggeren krever Python med `reportlab` og `Pillow`, DejaVu Sans-fontene under `/usr/share/fonts/truetype/dejavu/`, samt Linux-bibliotekene librsvg, Cairo og GObject. Nye PDF-er og bildecache går til ignorert `build/`. Arkiv-PDF-en beskyttes mot overskriving. Kildelenkene er gjort repo-relative; dette kan påvirke paginering ved senere bygg. En ny PDF er derfor en ny versjon, ikke en erstatning for den leverte originalen.

Kartgeneratoren krever den opprinnelige typen sceneeksport, som normalt er en generert fil utenfor Git. Den starter ingen enginejobb:

```bash
python3 Docs/DesignBible-2026-10-27/lag_kart.py --manifest /sti/til/manifest.json
```

Nye kart går til `build/kart/`. Kartene i arkivet forblir uendret. Ikke anta at en nyere sceneeksport gir samme ankerpunkter som originalen.

## Kontroll ved arkivering

PDF-identitet og sidetall, Python-syntaks, lokale dokumentlenker, SVG og bildeavhengigheter er kontrollert uten ny pakking, kartregenerering eller PDF-bygg. De genererte PDF-sidebildene, cache-JPEG-er og midlertidig PDF-tekst er utelatt.

Prosjektbiblioteket ble søkt etter arkivering og dokumentleveranser uten relevant teknisk gjenbruk. Eksisterende builder videreføres; bare stier og plassering av nye utdata er tilpasset.
