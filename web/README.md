# SIGNAL / 47 (web)

Nettversjonen av SIGNAL / 47, bygget med three.js, TypeScript og Vite. Førstepersons etterforskning ved SARO i New Mexico, 1986. Denne mappen inneholder prologen "Night Shift" og kapittel 1 "The Second Exposure": servicegården øst for kontrollrommet, motorskapet S-03, feltkameraet, fotolaben og B-12-kontrollen, fram til den lokale rapporten er arkivert.

Forslag til hva som bør gjøres videre, og hva Tom må bestemme, står i `FORSLAG.md`.

Unity-versjonen i rotmappen er arkiv. Design, historie og spillkonstanter er hentet derfra.

Spill i nettleseren: https://tombonator3000.github.io/SIGNAL-47/

## Grafikkpakken fra 4. oktober 2026

Nettversjonen bruker nå bildegenererte materialer i kontrollrommet, servicegården og fotolaben, et Melkeveis-panorama, to plakater, New Mexico-kart, SARO-logo, prosedyreark og skilt. Originalene med briefens filnavn og mål ligger i `src/assets/art/`. Det slukkede motellskiltet er levert som kildevariant; spillet viser den tente varianten. Prolog, fotobevis og spillkonstanter er beholdt.

`src/core/art.ts` laster 16 bilder før scenen bygges. Tekstene på instrumenter, bevis og veimerking tegnes fortsatt i kode. High og Low deler bildekildene. Himmelen har en 2K-runtimekopi for å begrense GPU-minne. PNG-originalene beholdes, mens sju lette WebP-kopier brukes i spillet. De kan bygges på nytt med `python3 tools/prepare_art.py` (Pillow).

Se [grafikkontroll og testgrenser](ART_DELIVERY.md). Gulvets øvre/nedre fuge har en liten registreringsfeil ved gjentakelse; alle materialer er derfor ikke godkjent som perfekt sømløse. Nye kunstbilder er assets, mens bildene under `evidence/art-2026-10-04/` er uredigerte opptak fra spillet.

Konsepter, lokasjonskart og seks nye tekstfrie flater er levert separat for videre integrasjon. Se [overleveringen til Claude](CLAUDE_HANDOFF.md), [grafikkgalleriet](src/assets/art/production/index.html) og [konsept- og kartgalleriet](src/assets/art/concept/index.html). Kapittel 2 på Claudes arbeidsgren er ikke flettet inn her.

## Kjør

```sh
cd web
npm install
npm run dev            # utviklingsserver, åpne adressen den skriver ut
npm run build:single   # én selvstendig fil: dist-single/index.html
npm run build          # vanlig bygg i dist/
npm run typecheck
```

`dist-single/index.html` kan åpnes direkte fra disk eller publiseres som claude.ai-artefakt. GitHub Pages bruker det vanlige bygget, se Publisering.

## Publisering

`.github/workflows/pages.yml` i roten av repoet bygger mappen med `npm run build` og publiserer `dist/` på GitHub Pages hver gang main får endringer i `web/`. Den kan også startes for hånd under Actions, Publish web game to GitHub Pages, Run workflow. Pages er satt opp med GitHub Actions som kilde (Settings, Pages).

Pages legger spillet i undermappen `/SIGNAL-47/`. Det virker fordi `base: './'` i `vite.config.ts` gir relative stier. Ikke bytt til en absolutt base.

## Kontroller

PC: WASD for å gå, mus for å se, E eller klikk for å bruke, Tab for notatboka, Escape for pause. Med feltkameraet: C hever og senker kameraet, mellomrom, E eller klikk tar bildet, Escape senker det.
Mobil: venstre tommel går, høyre tommel ser, trykk på ting for å bruke dem. Knappene Use, Notes og pause ligger i hjørnet. Camera-knappen dukker opp når kameraet er hentet. Mens kameraet er hevet, tar et trykk på skjermen eller på Use-knappen bildet.
Settings har lydnivå, blikkfart og grafikk (High eller Low). Innstillinger, sjekkpunkt og saken i kapittel 1 (med fotografiene) lagres i nettleseren.

`?debug` bak adressen viser fps, 95-persentil for bildetid, draw calls og trekanter oppe til venstre. Den er laget for å måle på ekte telefoner.

## Struktur

```
src/main.ts                oppstart, løkke, menyer, testkroker (window.S47)
src/core/                  lyd, input, interaksjon, bildeinnlasting, kodeteksturer, grafikknivå
src/core/FieldCamera.ts    søker, eksponering og filmkopi (960x600 med papirkant og håndskrevet tekst)
src/core/debug.ts          målestripa bak ?debug
src/player/                førstepersonsspiller med gangbare soner og kollisjon
src/world/                 kontrollrom, antenner, utendørs, himmel, byggeklosser (kit.ts)
src/world/ServiceYard.ts   servicegården, S-03, B-12 og fotolaben
src/world/Dish.ts          de 27 antennene som instanser, med egen synlighetstest
src/world/glow.ts          alle lampeglød og blinklys i ett tegnekall
src/world/props.ts         lyktestolper, rekkverk, utklippstavle, feltkamera
src/story/Prologue.ts      hele prologen som faser og tidsstyrte hendelser
src/story/Chapter1.ts      kapittel 1 som trinn, med lagring av saken
src/story/Signal.ts        mottakerlogikken, portert fra Unity
src/ui/                    HUD, dokumenter, notatbok, RX-konsoll
src/ui/Panels.ts           fotolabens paneler: våtbenk, kontaktkopi, referansefil, B-12, to eksponeringer, rapport
src/assets/                lyd og fonter (se THIRD_PARTY_NOTICES.md)
tools/                     headless-tester med Playwright
```

## Testing

`tools/walkthrough.py` spiller hele prologen gjennom de ekte interaksjonene og konsollens glidebrytere, tar skjermbilder og skriver PASS eller FAIL per steg. Løkken holdes og spillet drives med `S47.tick()`, slik at testen går i en programvare-renderer.

`tools/chapter1.py` spiller kapittel 1 fra start til sluttkort med ekte klikk i panelene: gange gjennom døra og opp gangveien, begge eksponeringene, framkalling, merking på kopiene, valg av referanse, hypotese, metode og konklusjon, og til slutt Continue etter omlasting. Den tar enten passiv eller aktiv metode. `tools/csptest.py` laster enkeltfila under en streng innholdspolicy og sjekker at alle fonter og lyder kommer med.

```sh
pip install playwright && playwright install chromium
python3 tools/walkthrough.py shots 844x390 high
python3 tools/chapter1.py shots/ch1 passive
python3 tools/chapter1.py shots/ch1 active
python3 tools/csptest.py
python3 tools/looks.py shots 844x390 high
```

Testene leser `dist-single/index.html` når ikke annet er oppgitt. Med `S47_URL` kan `walkthrough.py` og `chapter1.py` teste et annet bygg, for eksempel Pages-bygget servert fra en undermappe:

```sh
npm run build
mkdir -p /tmp/s47site && rm -rf /tmp/s47site/SIGNAL-47 && cp -r dist /tmp/s47site/SIGNAL-47
python3 -m http.server 8047 --bind 127.0.0.1 --directory /tmp/s47site &
S47_URL=http://127.0.0.1:8047/SIGNAL-47/ python3 tools/walkthrough.py shots 844x390 high
```
