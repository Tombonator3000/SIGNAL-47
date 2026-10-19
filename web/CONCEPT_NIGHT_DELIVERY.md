# Nye konsepter, én natt i 1986

Åtte nye miljø- og fortellingskonsepter etter Toms bestilling 18. oktober 2026. [Åpne galleriet](src/assets/art/concept/night_20261018/index.html), eller se [alle åtte på ett kontaktark](src/assets/art/production/concepts_night_20261018/contact-sheet.jpg).

Bildene bygger på historien og stedene på main `f019dd6`, særlig `HISTORIE.md`, `KAPITLER.md`, `HENDELSER.md` og den faktiske plasseringen i `world/Town.ts`. Nyere beslutninger om Caliche og romvesenene i bakgrunnen gjelder foran den eldre regelen om ingen monstre. Ansiktene spares fortsatt til THE EVENT. Ingen nye steder, hendelser eller spillkonstanter er vedtatt i denne leveransen.

## Bildene

Alle ligger i `src/assets/art/concept/night_20261018/`.

| Fil | Motiv og kildegrunnlag |
| --- | --- |
| `caliche_main_street.jpg` | Hovedgata litt over tre, med bensinstasjon og Plaza i vest og AM-stasjon og kirke i øst. `HENDELSER.md` del 9 og `Town.ts`. |
| `caliche_payphone.jpg` | Telefonautomaten ved nattluka og betjenten bak glasset. Hendelse 1 og 5. |
| `night_line_window.jpg` | Earl i glassburet, den døde telefonlinja og nattpraten. Hendelse 9. |
| `laundromat_radio.jpg` | Kvinnen med transistorradioen på benken utenfor vaskeriet. Caliches personer og nattens rytme. |
| `motel_office_tv.jpg` | TV-en viser pickupen rett ovenfra mens den samme bilen står utenfor kontorvinduet. Hendelse 10, spesifisert for senere innbygging. |
| `saro_peripheral_glimpse.jpg` | Antennefeltet og én fjern, ubelyst silhuett i bildekanten. `HENDELSER.md` del 7 og `core/glimpses.ts`. |
| `station01_cut_cable.jpg` | De avkuttede kabelendene, feltkameraet og transitten, før gryet. The Survey Station. |
| `roswell_road_empty_truck.jpg` | Pickupen i høyre kjørefelt idet kameraet trekker seg ut etter THE EVENT. Gryet ligger foran og til høyre. Bildet røper slutten. |

Dette er komposisjoner for lys, materialer og fortelling, ikke målfast dokumentasjon av spillets geometri. Caliche er fortsatt den korte gata fra koden; den tegnede perspektivdybden gir ikke en lengre eller tettere by. Antennebildet viser noen av anleggets 27 antenner, og fastsetter ikke antallet på nytt. Motell-TV-en er et konsept for en spesifisert hendelse, ikke bevis på ferdig implementering. Spilleren får ikke ansikt. Slutten får ingen ny oppvåkning eller lysstråle fra en farkost.

## Opphav og filer

Alle åtte er laget med innebygd `image_gen.imagegen`. Eksakt modellversjon er ikke eksponert. Tre av de åtte ble korrigert i samme verktøy: rett ovenfra på TV-skjermen, nattlig horisont på STATION 01 og pickupen i høyre kjørefelt. Totalt elleve generatorløp; alle elleve originale PNG-er er bevart i `production/concepts_night_20261018/sources/` med valgte og avviste utgaver merket i `attempts.json`.

Generatororiginalene er 1672×941. Leveringsfilene er sRGB-JPEG i 1920×1080, tilpasset med Lanczos, kvalitet 90 og subsampling 0. De er oppskalert, ikke native 1080p. De åtte JPEG-filene er samlet cirka 3,6 MB. Ingen motivmaling er gjort i eksporten. Kontaktarket er en oversikt med titler utenfor bildene.

- [Fullstendige prompter og hash av historiekildene](src/assets/art/production/concepts_night_20261018/prompts.json)
- [Alle generatorløp og korrigeringene](src/assets/art/production/concepts_night_20261018/attempts.json)
- [Leveringsmål og SHA-256](src/assets/art/production/concepts_night_20261018/manifest.json)
- [Filkontroll](src/assets/art/production/concepts_night_20261018/verification.json)
- [Visuell gjennomgang](src/assets/art/production/concepts_night_20261018/visual-review.json)
- [Gallerikontroll i nettleser](src/assets/art/production/concepts_night_20261018/gallery-verification.json)
- [Byggkontroll og sammenligning med testet grunnlag](src/assets/art/production/concepts_night_20261018/build-verification.json)

Eksporten kan kjøres om igjen med Python og Pillow, fra hvilken som helst arbeidsmappe:

```sh
python web/src/assets/art/production/concepts_night_20261018/export_concepts.py
```

Ingen nye nedlastede tredjepartsbilder, merkenavn eller andre spills grafikk. Det påstås ingen CC0-lisens. Biblioteket ble søkt etter konseptkunst, historie og three.js og deretter med færre ord; ingen relevante treff ble gjenbrukt. De visuelle referansene er prosjektets egne tre eldre konsepter, inspisert av Codex og beskrevet i promptene.

## Kontroll og avgrensning

PASS: 21 filkontroller, inkludert kildehash og dekoding for alle elleve originaler, størrelse/format/RGB/sRGB for de åtte leveringsfilene og kontroll av at spillkode og eksisterende runtimegrafikk er uendret. Alle valgte bilder og kontaktarket er visuelt gjennomgått.

PASS: 13 kontroller av galleriet i Chromium ved 1360×900 og 390×844. Alle åtte bilder dekoder i begge visninger, alle lenker svarer 200, kildepanelene kan åpnes uten vannrett overflow, og ingen nettleserfeil eller mislykkede forespørsler oppstår. Begge skjermbildene er visuelt inspisert i `production/concepts_night_20261018/qa/`.

PASS: `npm run typecheck`, `npm run build` og `npm run build:single`. Begge byggenes `index.html` har samme SHA-256 som det tidligere spilltestede grunnlaget fra grafikkleveransen. Ingen av de nye konseptfilene er med i `dist/`. Vites eksisterende melding om at `Town.ts` er både statisk og dynamisk importert står fortsatt.

Spillets innlastinger er uendret. Konseptene er ikke importert i spillet. De tidligere spilltestene på samme kode og runtimegrafikk står i `ART_ROUND13_15_DELIVERY.md`; de tunge spilltestene er ikke kjørt om igjen for disse uimporterte konseptene, som ved konseptleveransen 4. oktober. Ingen ny måling av maskinvareytelse eller faktisk spilling er gjort.
