# Grafikkleveranse, runde 13 til 15

66 ferdige bildefiler etter bestillingen i `ART_BRIEF.md` på main `497a0d5`. Alle navn, mål og formater følger briefen. Portrettene kommer inn via eksisterende innlasting; resten er klar kildegrafikk til Claudes innbygging.

| Runde | Leveranse | Antall |
| --- | --- | ---: |
| 13 | Ansiktsportretter, inkludert de fem personene i Caliche | 11 |
| 14 | Åtte vindusinteriører, pappfigur, avis, sju skilt med og uten tekst, tre silhuetter og fire bortføringsbilder | 31 |
| 15 | Bevisbord, album, sju tegninger av utstyret, to tv-bilder, delingsbilde og ikon | 24 |

Filene ligger i `src/assets/art/people/`, `town/`, `alien/`, `abduct/`, `board/`, `album/`, `kit/`, `motel/` og `brand/`. Hver mappe har `MANIFEST.md` med navn, mål og innhold. [Samlet manifest](src/assets/art/production/round13_15_manifest.json) har SHA-256 og filstørrelser. Spillfilene samlet er cirka 5,2 MB; de elleve portrettene er cirka 0,87 MB.

## Se grafikken

- [Portrettene](src/assets/art/production/round13_15_qa/people-contact.jpg)
- [Vinduer, pappfigur og avis](src/assets/art/production/round13_15_qa/town-windows-contact.jpg)
- [Skiltene med blanke varianter](src/assets/art/production/round13_15_qa/town-signs-contact.jpg)
- [Utstyret](src/assets/art/production/round13_15_qa/kit-contact.jpg)
- [Bevisbordet](src/assets/art/production/round13_15_qa/board-contact.jpg)
- [Albumet](src/assets/art/production/round13_15_qa/album-contact.jpg)
- [Bortføringen](src/assets/art/production/round13_15_qa/abduct-contact.jpg)
- [Tv-bildene](src/assets/art/production/round13_15_qa/motel-contact.jpg)
- [Delingsbilde og ikon](src/assets/art/production/round13_15_qa/brand-contact.jpg)

## Kontroller

PASS: 117 av 117 filkontroller, inkludert alle 66 navn og mål, sRGB, RGB/JPEG og RGBA/PNG, vindusbilder under 350 kB og eksakte åpne felt i rammene. Instant-vinduet er 672×672 ved 48,48. Planfilm-vinduet er 896×700 ved 64,60. [Maskinrapport](src/assets/art/production/round13_15_qa/asset-report.json).

PASS: visuell kontroll av alle kontaktark og 2×2-gjentakelse for kork, albumpapir og garn. Skiltene har de bestilte ordene; R mangler med vilje i siste linje på Plaza. Betjentens blikk, personen som ligger på ryggen og nattlyset i det tomme radiostudioet er rettet i bildegeneratoren. [Visuell kontroll](src/assets/art/production/round13_15_qa/visual-review.json). Materialenes randpiksler er ikke matematisk identiske; fine korn og flekker gjentas.

PASS: `npm run typecheck`, `npm run build:single` og `npm run build`. Vite viser den eksisterende meldingen om at `Town.ts` er både statisk og dynamisk importert.

Resultatet av de sekvensielle spilltestene legges i `production/round13_15_qa/runtime-summary.json` før levering. Bildene av selve portrettene i spillet ligger i `production/round13_15_qa/runtime/`.

UNVERIFIED: subjektiv vurdering på ekte mobil/PC og maskinvareytelse. Runde 14 og 15 er ikke importert eller innbygget av denne leveransen. Visuelle spilltester kan derfor bare bekrefte portrettene og at eksisterende spill fortsatt virker.

## Opphav og kildefiler

36 valgte generatororiginaler er laget med det innebygde `image_gen.imagegen` etter imagegen-skillen. Eksakt modellversjon er ikke eksponert. Fulle promter, kildehash og tre avviste varianter er dokumentert i `production/round13_15_sources/generation-attempts.json`; de valgte originale PNG-filene er bevart i `raster/`. De tre avviste originalene ligger lokalt i denne samtalens generatorarkiv.

30 enkle rekvisitter er originale SVG-tegninger for dette prosjektet, med redigerbare kilder og `author_vectors.py`. Dette gjelder skiltparene, silhuettene, trerammen, stifter, garn, kort, tape, kaffering, albumrammer, fotohjørne, testbilde og ikon. Rastertekst på skiltene bruker Liberation Sans fra arbeidsmiljøet; ingen fontfil er lagt til spillet.

Eksport med Pillow 13.0.0: Lanczos til de eksakte målene, JPEG kvalitet 85 med subsampling 0, PNG med bevart alfa og frosset sRGB ICC. Ingen motivmaling eller skjøting av genererte bilder er gjort i Python. Reproduser fra web-mappa med Python som har Pillow, NumPy og CairoSVG:

```sh
python src/assets/art/production/round13_15_sources/export_rasters.py
python src/assets/art/production/round13_15_sources/author_vectors.py
python src/assets/art/production/round13_15_sources/check_assets.py
```

Ingen nye nedlastede fotografier, andre spills grafikk, betalte assettjenester eller påstått CC0-lisens. Biblioteksøk er utført; kortet `figurark-fast-storrelse` gjelder ScummVM og er ikke brukt som kode i dette three.js-prosjektet. Spillets eksisterende kanon og kode er bevart.
