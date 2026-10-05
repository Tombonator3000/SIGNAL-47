# Runde 11: Roswell-veien og FLASH

Åtte tekstfrie bilder til `ART_BRIEF.md`, «Runde 11», ved Claude-commit `d539c4d`. Leveransen ligger i egen gren fra main `5a0282d`. Claude eier innkobling i spillet, tekst på flatene, opphav i `THIRD_PARTY_NOTICES.md` og én samlet integrasjonstest. Ingen runtime-kode, gamle assets, historie-/testfiler eller Unity-filer er endret.

| Fil under src/assets/art/ | Mål | Byte | Format |
|---|---|---:|---|
| road/milepost_blank.png | 256×640 | 336958 | RGBA PNG |
| road/witness_post.png | 128×768 | 197005 | RGBA PNG |
| road/survey_disk_brass.png | 512×512 | 519412 | RGBA PNG |
| road/tex_oldroad_asphalt.jpg | 1024×1024 | 465303 | RGB JPEG85 |
| flash/flash_eye.jpg | 1280×800 | 170483 | RGB JPEG85 |
| flash/flash_hand.jpg | 1280×800 | 153384 | RGB JPEG85 |
| flash/flash_face.jpg | 1280×800 | 152281 | RGB JPEG85 |
| flash/flash_man.jpg | 1280×800 | 176592 | RGB JPEG85 |

## Kontroller

- **PASS:** 8/8 eksakte navn, mål og formater, innebygd sRGB-ICC, ekte alfa i de tre PNG-ene og ingen alfa i JPEG. Alle sluttbilder er under 600000 byte. SHA-256 står i manifestet og maskinrapporten.
- **PASS:** visuelt tekstfrie kildeflater. Prøvetekst `MILE / 8` ligger innen x30..226/y60..580 og er lesbar. Loddrett `SURVEY MARK` og `STA 01 / C / 1947` med pil er lesbare i QA. Teksten finnes bare i kontrollbildene.
- **PASS:** asfalten er undersøkt som 2×2. Ingen synlig rett kontrastkant; oppmerkingen er utelatt. Mønsterrepetisjon i en gjentatt tekstur er fortsatt mulig. Målinger av motstående kanter er observasjoner, ikke bevis på matematisk identiske kantpiksler.
- **PASS:** FLASH-motivene er gjennomgått i full størrelse og sammen i 320×200: øye, fem lange fingre mot dugg, delvis skjult ansikt og en ugjenkjennelig oppdiktet landmåler. Håndens bakgrunn ble endret til lav mesa i gry. Ansiktet kopierer ingen navngitt filmfigur; mannen bruker den eksisterende oppdiktede avisbildesilhuetten som referanse.
- **UNVERIFIED:** lys, alfa/materialvalg, tekststørrelse i faktisk førerhus, bildeinnlasting, FLASH-timing, Slow fades og ytelse på PC/mobil. Dette kontrolleres av Claude etter innkobling. Bildene er foreløpig ikke importert av denne PR-en og følger ikke spillpakken.

Kontrollmaterialet ligger i [round11_qa](src/assets/art/production/round11_qa/). Maskinrapporten lar visuell kontroll stå som UNVERIFIED fordi den ikke kan avgjøres automatisk; [visual_review.json](src/assets/art/production/round11_qa/visual_review.json) dokumenterer den separate manuelle kontrollen. Uendrede grønne spilltester er gjenbrukt som baseline; egen kontroll gjelder filene i denne leveransen. Ingen ny full gjennomspilling er hevdet.

## Opphav og eksport

Laget med innebygd ChatGPT-bildegenerator gjennom imagegen-skillen. Eksakt modellversjon er ikke eksponert. Originale genererte PNG-er, alle promter, edit-referanser, frosset sRGB-profil og eksportoppskrift er bevart i `production/round11_qa/sources/`. Ingen eksterne bildearkiver, ekte personer eller merkenavn er brukt; grafikken er ikke hevdet CC0.

Etter generering er bare måltilpasning, utsnitt til PNG-konturen og PNG/JPEG/sRGB-eksport gjort med Pillow. PNG-utsnittet bruker alfa>=128 for å finne boksen og to kildepiksler margin; selve alfakanalen er ikke malt om. De to motivrettingene (vitnestolpens kant og håndens bakgrunn) er gjort med bildegeneratoren. Asfalten er regenerert etter sømkontrollen. JPEG bruker kvalitet 85, subsampling 0. [round11_manifest.json](src/assets/art/production/round11_manifest.json) fører mål, metode, slutt-/kildehasher, referanser, promter og QA-hasher.

## Innbygging hos Claude

Last `road/` med OldRoad og `flash/` ved kapittel 6. Behold reservebildene til innlasting er ferdig. Legg tall/ord/stempel i kode oppå de blanke flatene. Alt under `production/` er dokumentasjon og kontrollmateriale og skal aldri importeres i spillet.

Foreslått opphavstekst: «Runde 11: åtte originale oppdiktede bilder laget for SIGNAL / 47 med innebygd ChatGPT-bildegenerator; mål-/sRGB-/PNG-/JPEG-eksport og kontroll ved Codex. Promter og originale kilder i production/round11_qa/sources; eksakt modellversjon ikke eksponert; ikke hevdet CC0.»
