# Grafikk til Three.js, 4. oktober 2026

Dette er en grafikkleveranse til den eksisterende prologen og kapittel 1. Spillet var allerede portert til Three.js på `main`. Arbeidet startet ved `478bfd1`; PR29s nyere dokumentasjon er tatt inn fra `47a18b7`. Unity-arkivet er ikke endret.

## Levert

17 kildebilder med briefens filnavn og mål i `src/assets/art/`: åtte materialteksturer, nattehimmel, to plakater, New Mexico-kart, SARO-logo, servicegårdskilt, S-03-prosedyre og to motellskiltvarianter. 16 bilder lastes i spillet. Det slukkede motellskiltet er en levert kildevariant, ikke en ny spillhendelse. De tre konseptbildene for framtidige områder er ikke del av dette passet.

Bildene er laget med innebygd ChatGPT-bildegenerering. Eksakt modellversjon er ikke eksponert. Prompts, kildebaner, eksport og kontrollnotater ligger i `ROOM_TEXTURES.json`, `EXTERIOR_TEXTURES.json`, `GRAPHICS.json` og `SKY_SIGN_PROMPTS.json` under assetmappene. Alle leverte bildehash finnes i [assetlisten](evidence/art-2026-10-04/assets.json).

`art.ts` laster bildene før scenen og materialene bygges. sRGB, uavhengige repeat-transformer og delt `THREE.Source` sikrer samme kart i High og Low uten unødige GPU-kopier. Sju PNG-originaler har WebP-kopier fra `tools/prepare_art.py`: 1 229 926 bytes mot omtrent 9,5 MB originaler. Himmelkilden på 1774x887 er eksportert til briefens 4096x2048; runtime bruker 2048x1024 uten mipmapkjede. Dette er ikke en påstand om ekte 4K-generering.

Tekst og geometriske markører i instrumenter, fotobevis og veimerking beholdes i kode. Skapenes albedo, gulv, tak, vegger, bord, betong, asfalt og ørken bruker de nye materialene. Kartets gamle bakplate skjulte utskriften; kartet er flyttet foran platen. En eksisterende testfeil i `S47.hold` er rettet: accessoren ble tidligere kopiert som vanlig verdi og stoppet ikke spilløkka. Prologtesten og CSP-testen returnerer nå feilstatus ved mislykkede kontroller.

## Verifikasjon

Miljø: Kubuntu på brukerens PC, isolert Playwright 1.63.0 og Chromium Headless Shell 153.0.8010.12 med SwiftShader. Spillscenarier ved 844x390, High. Visuell kontroll ved 1280x800, High og Low. Programmatisk posisjonering og styrt spilltid brukes sammen med panelklikk og enkelte tastaturhandlinger. Dette er ikke en full manuell gjennomspilling eller måling av maskinvare-FPS.

| Kontroll | Status og bevis |
| --- | --- |
| TypeScript og begge Vite-bygg | PASS |
| Enkeltfil, prolog | PASS, 22/22 |
| Enkeltfil, kapittel 1 passiv og aktiv | PASS, 52/52 hver, inkludert fotografier, arkivering og Continue |
| Ordinært bygg fra `/SIGNAL-47/`, prolog | PASS, 22/22 |
| Ordinært bygg fra `/SIGNAL-47/`, kapittel 1 | PASS, 52/52 passiv, inkludert Continue |
| Lasting, High/Low, gjenoppretting ved manglende bilde | PASS, 8/8 i [art-pages.json](evidence/art-2026-10-04/art-pages.json) |
| Streng CSP | PASS på sluttbygget, 4 fonter, 11 lyder og 16 bilder. Se [CSP-logg](evidence/art-2026-10-04/csp.txt) |
| Visuell kontroll | Rom, lab, plakater og kart inspisert i High/Low. Motellskiltet inspisert i faktisk scene. Gulv har liten kjent skjøtfeil |
| Fysisk mobil, Safari, ekte FPS og subjektiv lydmiks | UNVERIFIED |

Det ordinære bygget testes med `S47_URL=http://127.0.0.1:8477/SIGNAL-47/`. Bildetestene omfatter også en bevisst avbrutt teksturforespørsel og et faktisk klikk på TRY AGAIN. Forventet nettverksfeil er skilt fra uventede feil i rapporten. Enkeltfilens fullstendige spillscenarier kjørte før den siste endringen i kartets plassering; denne endrer ingen kapitellogikk. Endelig nettbygg besto prolog og passivt kapittel etter kartrettelsen, se [nettbyggscenariene](evidence/art-2026-10-04/gameplay-pages.json). Bygghash og kildeinnholdets SHA-256 er i [build.json](evidence/art-2026-10-04/build.json); tidligere scenariohash er i [gameplay-single.json](evidence/art-2026-10-04/gameplay-single.json).

Kjør på nytt fra `web/`:

```sh
npm ci
npm run typecheck
npm run build:single
npm run build
python3 tools/walkthrough.py shots/prologue 844x390 high
python3 tools/chapter1.py shots/passive passive
python3 tools/chapter1.py shots/active active
python3 tools/csptest.py
python3 tools/artcheck.py shots/art
```

Python-testene krever Playwright og Chromium. `prepare_art.py` krever Pillow, men er ikke nødvendig for vanlige bygg siden runtime-kopiene er versjonert.

## Faktiske spillbilder

- [Kontrollrom, High](evidence/art-2026-10-04/room-high.png) og [Low](evidence/art-2026-10-04/room-low.png).
- [Servicegård](evidence/art-2026-10-04/yard-high.png) og [fotolab](evidence/art-2026-10-04/lab-high.png).
- [Veggkart etter rettelse](evidence/art-2026-10-04/map-high.png) og [motellskilt](evidence/art-2026-10-04/motel-high.png).

Bildene er uredigerte runtime-opptak med diagnostisk kamerastilling, ikke konseptbilder. De ni faste utsnittene måler 48 til 188 draw calls i begge kvalitetsnivåer, med 71 GPU-teksturer. Målingene står per utsnitt i JSON og er ikke omregnet til en FPS-påstand. Hele det ordinære bygget er 4 722 846 bytes; enkeltfila er 6 056 870 bytes.

## Begrensninger og levering

Gulvets vanlige vertikale repetisjon har en liten feil i fugeregistreringen. Flere genereringsforsøk og speiling ble vurdert; speiling laget smale romber og brukes ikke. Tak, vegg, bord og utendørsmaterialene besto visuell 2x2-kontroll. «Perfekt sømløst» er dermed ikke godkjent for hele pakken.

Kanon og eksisterende spillreise er beholdt. Senere Unity-kapitler er ikke portert i dette grafikkpasset. Frame- og B-12-markører er fortsatt presise kodegrafikker. Mobil-FPS og lydmiks må vurderes på faktisk utstyr.

Spill lokalt med `npm run dev`, eller åpne `dist-single/index.html` direkte. Kildeendringen ligger i [PR #30](https://github.com/Tombonator3000/SIGNAL-47/pull/30), implementeringscommit `f74e411`, på egen gren. Merge til `main` vil publisere til Pages og utføres ikke i dette passet.

## Senere konseptleveranse samme dato

Et separat pass leverte seks miljøkonsepter og fire lokasjonskart. Se [CONCEPT_DELIVERY.md](CONCEPT_DELIVERY.md) og [galleriet](src/assets/art/concept/index.html). Dette endrer ikke runtime-resultatene ovenfor.
