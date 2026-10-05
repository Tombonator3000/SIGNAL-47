# Runde 12: ørkenen sett ovenfra i uttrekket

Ett macro-bilde til Roswell Road etter `ART_BRIEF.md` på main `1e27e6c`, levert i egen gren. Det dekker et oppdiktet område på omtrent 2 × 2 km med sand, caliche, kort gress, små buskpunkter og ett svakt tørt bekkefar. Claude eier innbygging i `OLDROAD_ART`/`OldRoad.ts`, opphav og én samlet integrasjonstest.

| Fil under src/assets/art/ | Mål | Format | Byte |
|---|---|---|---:|
| road/tex_oldroad_macro.jpg | 2048×2048 | RGB JPEG85, subsampling 0, sRGB ICC | 844774 |

SHA-256: `724fa3c6dc99487fcb634112fb18415354e4acd2d9bb0aee9b45500c43553dfe`.

## Kontroller og begrensninger

- **PASS:** nøyaktig filnavn, 2048×2048, RGB JPEG uten alfa, innebygd sRGB-profil og under 900000 byte. Middelfargen er RGB **164,845 / 142,201 / 119,667**, innen ±10 av 164/145/123. Eksportoppskriften gjenskaper identisk slutt-SHA.
- **PASS:** manuell og uavhengig visuell kontroll av 2×2-flaten, 3×3 ved 1536×1536 og native utsnitt. Bekkefaret fortsetter sammenhengende, ingen tydelig rett kontrastkant eller stor særpreget S-bøy. Ingen veier, hjulspor, bygninger, vann, tekst eller harde retningsskygger funnet. Randpiksler er ikke matematisk identiske, og vanlig mønsterrepetisjon er mulig.
- **PASS med dokumentert restavvik:** buskutvalget har hovedsakelig 1–4 px kjerner; de tidligere grove klyngene er fjernet. **FAIL på universell eksakt 4 px-grense:** to isolerte P3-eksempler er 5×4 px ved (488,384) og 4×6 px ved (1760,493). Uavhengig kontroll fant ingen vesentlig P2 og anbefaler levering med disse små restavvikene dokumentert.
- **PASS på visuell lav/moderat kontrast; FAIL på et strikt pikselintervall 60..225:** minste RGB er **67/49/35**, største **253/232/210**. Blå under 60 utgjør 0,00796 %, rød over 225 0,00815 %. Briefens omtrentlige kontrastreferanse er ikke brukt til å skjule uteliggere.
- **UNVERIFIED:** materialblanding rundt 150 m, faktisk utsikt fra 126–650 m, innlasting fra Pages-undermappen og PC-/mobil-ytelse. Claude kontrollerer dette etter innbygging. Bildet er ikke importert av denne PR-en og følger foreløpig ikke spillpakken.

Kontrollmateriale: [round12_qa](src/assets/art/production/round12_qa/), [maskinrapport](src/assets/art/production/round12_qa/round12_qa_report.json), [separat visuell kontroll](src/assets/art/production/round12_qa/visual_review.json) og [manifest](src/assets/art/production/round12_manifest.json). Maskinrapporten lar visuelle spørsmål stå som UNVERIFIED; manuell kontroll står separat.

## Opphav og eksport

Laget med innebygd ChatGPT-bildegenerator via imagegen-skillen. Eksakt modellversjon er ikke eksponert. Sju generatortrinn gjelder samme bestilte bilde; seks mellomvarianter er avvist. Originale PNG-er, fulle promter, edit-referanser og frosset sRGB-profil ligger i `production/round12_qa/sources/`. Ingen eksterne bildearkiver eller andre spills kode/grafikk er brukt. Grafikken er ikke hevdet CC0.

Generatorens valgte kilde er 1254×1254. Sluttbildet er tilpasset til 2048×2048 med Lanczos og eksportert med Pillow 12.1.1 som JPEG85/subsampling0/optimize med frosset sRGB-ICC. Ingen motivmaling, kanaljustering, speiling eller sømblanding er gjort i Python. Motiv- og fargerettingene er gjort i generatoren. [Eksportoppskriften](src/assets/art/production/round12_qa/sources/export_asset.py) gjenskaper sluttfila; [QA-oppskriften](src/assets/art/production/round12_qa/sources/check_asset.py) kjøres i QA-only-modus med web-mappa som argument.

Uendret grønn baseline fra Roswell-integrasjonen gjenbrukes. Ingen ny full spilltest er hevdet for et uimportert bilde. Claude eier `art.ts`, `OldRoad.ts`, `artcheck.py`, `THIRD_PARTY_NOTICES.md`, kapittel-6-sjekken og én samlet test. Behold eksisterende bakke som reserve til bildet er lastet. Alt under `production/` er kontrollmateriale og skal ikke importeres.

Foreslått opphavstekst: «Runde 12: original oppdiktet ørken-macro laget for SIGNAL / 47 med innebygd ChatGPT-bildegenerator; mål-/sRGB-/JPEG-eksport og kontroll ved Codex. Promter og originalkilder i production/round12_qa/sources. Eksakt modellversjon ikke eksponert; ikke hevdet CC0.»
