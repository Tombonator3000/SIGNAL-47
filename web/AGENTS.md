# SIGNAL / 47 web: arbeidsinstruks

Denne fila gjelder alt under `web/`. For denne mappen erstatter den Unity-reglene i rotens `AGENTS.md`. Unity-prosjektet i `Unity/` er arkiv og referanse. Ikke rediger det fra nettarbeid.

## Roller

- Claude: lead og koder. Arkitektur, gameplay, geometri, lyd, tester og innbygging av grafikk.
- ChatGPT: grafikk og støtte. Lager 2D-assets etter `ART_BRIEF.md`, konseptkunst og tekstutkast. Leverer filer med nøyaktig navn og størrelse fra briefen. Endrer ikke kode uten avtale med Tom.
- Tom: bestemmer retning, tester på mobil og PC, committer og fletter.

## Før du gjør noe

1. Les denne fila, `memory.md`, `todo.md` og de siste oppføringene i `log.md`.
2. Kontroller faktisk Git-status. Det som står i gamle samtaler er ikke fasit.

## Mens du jobber

- Logg alt i `log.md` med UTC-tid: hva du gjorde, hvorfor, hvilke filer, og testresultat.
- Oppdater `memory.md` når en beslutning eller fast verdi endres, og `todo.md` når oppgaver blir ferdige eller nye dukker opp.
- Dokumentasjon og svar til Tom skrives på norsk. Ingen emoji og ingen tankestrek. Tekst inne i spillet er engelsk (New Mexico, 1986).
- Bevar kanon: tallene og navnene i `memory.md` (frekvenser, 47 sekunder, -39 LY, 27 antenner, az 026).
- Stilisert lavpoly som skal gå på mobil. Budsjett i kontrollrommet: rundt 300 draw calls eller mindre, noen få ekte lys. Utendørs bruker vi falske flomlys via `floodlit()` i `src/world/kit.ts`.
- Ny grafikk legges i `src/assets/art/<kategori>/` med navnet fra briefen. Før kilde og lisens i `THIRD_PARTY_NOTICES.md`.
- Jobb i små steg som blir ferdige. Ikke start motorbytte, nye rammeverk eller store verktøypass uten at Tom ber om det.

## Før levering

```sh
npm install
npm run typecheck
npm run build:single
python3 tools/walkthrough.py shots 844x390 high
python3 tools/chapter1.py shots/ch1 passive
```

`chapter1.py` kom inn 3. oktober sammen med kapittel 1. Den dekker alt etter prologen og bør kjøres med begge metodene (`passive` og `active`) når kapittel 1 endres.

Merk resultat som PASS, FAIL eller UNVERIFIED med faktisk grunnlag. Headless-testen bruker programvare-rendering og sier ingenting om ekte fps. Ytelse er UNVERIFIED til noen har målt på ekte maskinvare.

Endringer som påvirker hvordan filer lastes (lyd, fonter, grafikk, stier, `vite.config.ts`), testes også mot det vanlige bygget servert fra en undermappe, slik Pages gjør. Se Testing i `README.md`.

## Publisering

Spillet ligger på https://tombonator3000.github.io/SIGNAL-47/. `.github/workflows/pages.yml` i roten bygger `web/` med `npm run build` og publiserer `dist/` på GitHub Pages hver gang main får endringer i `web/`. Det som flettes til main, er altså ute på nettet noen minutter senere. Kjør kontrollene over før du fletter.
