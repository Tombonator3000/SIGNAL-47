# SIGNAL / 47 (web)

Nettversjonen av SIGNAL / 47, bygget med three.js, TypeScript og Vite. Førstepersons etterforskning ved SARO i New Mexico, 1986. Denne mappen inneholder prologen "Night Shift" fra start til tittelkort.

Unity-versjonen i rotmappen er arkiv. Design, historie og spillkonstanter er hentet derfra.

## Kjør

```sh
cd web
npm install
npm run dev            # utviklingsserver, åpne adressen den skriver ut
npm run build:single   # én selvstendig fil: dist-single/index.html
npm run build          # vanlig bygg i dist/
npm run typecheck
```

`dist-single/index.html` kan åpnes direkte fra disk, legges på GitHub Pages eller publiseres som claude.ai-artefakt.

## Kontroller

PC: WASD for å gå, mus for å se, E eller klikk for å bruke, Tab for notatboka, Escape for pause.
Mobil: venstre tommel går, høyre tommel ser, trykk på ting for å bruke dem. Knappene Use, Notes og pause ligger i hjørnet.
Settings har lydnivå, blikkfart og grafikk (High eller Low). Innstillinger og sjekkpunkt lagres i nettleseren.

## Struktur

```
src/main.ts            oppstart, løkke, menyer, testkroker (window.S47)
src/core/              lyd, input, interaksjon, teksturer tegnet i kode, grafikknivå
src/player/            førstepersonsspiller med kollisjon
src/world/             kontrollrom, antenner, utendørs, himmel, byggeklosser (kit.ts)
src/story/Prologue.ts  hele prologen som faser og tidsstyrte hendelser
src/story/Signal.ts    mottakerlogikken, portert fra Unity
src/ui/                HUD, dokumenter, notatbok, RX-konsoll
src/assets/            lyd og fonter (se THIRD_PARTY_NOTICES.md)
tools/                 headless-tester med Playwright
```

## Testing

`tools/walkthrough.py` spiller hele prologen gjennom de ekte interaksjonene og konsollens glidebrytere, tar skjermbilder og skriver PASS eller FAIL per steg. Løkken holdes og spillet drives med `S47.tick()`, slik at testen går i en programvare-renderer.

```sh
pip install playwright && playwright install chromium
python3 tools/walkthrough.py shots 844x390 high
python3 tools/looks.py shots 844x390 high
```
