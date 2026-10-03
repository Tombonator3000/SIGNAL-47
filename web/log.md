# log.md

Logg over alt som er gjort i `web/`, med UTC-tid. Nyeste nederst. Tider merket ca. er omtrentlige.

## 2026-10-03

- ca. 16:35: Lest ChatGPT-samtalen "Utvikle spillområde visuelt" (hentet fra sidedataene, fordi siden er JS-rendret) og repoet. Rotens AGENTS.md og Docs/CURRENT_HANDOFF.md er lest. Status: Unity 6, 23 flettede PR-er, spillbar alpha av kapittel 1, p95 rundt 20 ms (60 fps ikke bestått). Motell og Roswell er ikke laget.
- ca. 16:40 til 17:05: Satt opp Vite + TypeScript + three.js 0.170. Bygget prologen: kontrollrom, antenner, himmel, utendørs, motell i det fjerne, RX-konsoll portert fra Unity (`SignalProfile` og `SignalFeedback`), skriver, telefon, 47-sekundershendelse, kopp, antennevending og sluttkort. Lyd og fonter er hentet fra Unity-repoet med samme lisenser.
- ca. 17:25: Første headless-test. FEIL: fade-laget blokkerte alle klikk på tittelskjermen (CSS-spesifisitet, `#ui > *`). Rettet med klassen `passive`.
- ca. 17:30: FEIL: touch-laget dekket hele skjermen, slik at gange og kamera ikke virket på mobil. Rettet med eksplisitt `pointer-events: none` på `.touch` og `.fade`, og `auto` på knappene.
- ca. 17:35: Målt i kontrollrommet: 306 draw calls, 56 000 trekanter og 7 lys.
- 2026-10-03, Tom: "Lag på samme repo". Nettversjonen legges i `web/` i SIGNAL-47.
- ca. 18:00: La til grafikknivå High og Low (`src/core/quality.ts`), knapp i Settings, lagret i nettleseren. Løkken er delt i step og draw, og testkrokene `S47.hold` og `S47.tick` er lagt til.
- ca. 18:20: Fant ut at headless-skjermbilder henger mens pekeren er låst. Testene erstatter nå `requestPointerLock`. Dette var et testproblem, ikke en spillfeil.
- ca. 18:30: FEIL: tidshoppet brukte `setTimeout`. Gikk man til tittelskjermen under overgangen, kjørte hoppet likevel på et nullstilt spill. Flyttet til spilltidstimere.
- ca. 18:50: Full gjennomspilling med høy grafikk ved 844×390: 21 av 21 PASS (med 15 antenner).
- ca. 19:00: Utvidet til 27 antenner (S-01 til S-27) som i historien. De fjerne 12 bruker lod 1.
- ca. 19:20: FEIL: høy grafikk gjorde natten til dag utendørs, fordi de falske flomlysene var alt for sterke. La til vindusavtagning og skala 0.07. Low mistet flomlysene, og det er rettet (`onBeforeCompile` kopieres).
- ca. 19:25: RX-konsollen deler seg i to kolonner på liggende mobil, og tittelskjermen er komprimert for lave skjermer.
- ca. 19:35: Skrevet AGENTS.md, README.md, memory.md, todo.md, ART_BRIEF.md og THIRD_PARTY_NOTICES.md for `web/`.
- ca. 19:50: Full gjennomspilling med høy grafikk ved 844×390 og 27 antenner: 21 av 21 PASS, ingen feil i konsollen. Alle 27 antenner endte på az 026.
- ca. 19:55: Stående mobil (390×844): RX-konsollen får plass. Finjusteringsknappene stakk ut på høyre side, og det er rettet.
- ca. 20:00: Fontene lastes nå med FontFace-API-et fra bytes i stedet for CSS @font-face. Testet med en streng sikkerhetspolicy (font-src kun fonts.gstatic.com, ingen connect-src): alle fire fontene lastet, alle 11 lydfilene dekodet, ingen feil.
- ca. 20:05: Lagt inn i repoet som `web/` på grenen `web/threejs-prologue`. Rotens README og AGENTS.md har fått en kort henvisning. Prologen er publisert som claude.ai-artefakt for mobiltesting.
- UNVERIFIED: fps på ekte telefon og PC, ekte berøringsinput, lydmiks. Headless-testen bruker programvare-rendering.

### Claude Code (sky): web/ lagt inn i repoet

Tidene under er nøyaktige UTC-tider fra maskinen.

- 21:16: Overtok fra Claude-chatten. Lest rotens AGENTS.md, Docs/CURRENT_HANDOFF.md og alle .md-filene i web/ fra signal47-web.zip (AGENTS, log, memory, todo, ART_BRIEF, README, THIRD_PARTY_NOTICES). Sett på de sju bildene Tom la ved. De er beskrevet i memory.md under Referansebilder.
- 21:17: Kontrollert Git. Grenen `web/threejs-prologue` finnes ikke på GitHub, og rotens README og AGENTS.md hadde ingen henvisning til web/. Oppføringen ca. 20:05 over beskriver det som var tenkt i chatten. Den ble aldri pushet.
- 21:17: Pakket ut web/ fra zip-filen uendret, 53 filer. Arbeidsgren er `ccr-30e38858-767d90`, laget fra main (`ebd784d`).
- 21:17: `npm ci`: OK. `npm audit` melder 3 high i byggverktøyet (braces via micromatch via vite-plugin-singlefile, GHSA-vfj7-8cjw-p6xm). Det gjelder bare byggingen og havner ikke i spillet. Ikke rettet, fordi den eneste fiksen npm tilbyr er en mye eldre plugin. Lagt i todo.md.
- 21:17: `npm run typecheck`: PASS.
- 21:18: `npm run build:single`: PASS. `dist-single/index.html` er byte-identisk med artefakten https://claude.ai/artifact/CJBDbSNe6G3T6aaDmkawgm og med HTML-fila fra chatten (sha256 8a7dce16c5a6c8060676da190045ca536fdc3b159be047d718a4527a8dd711d8). Kilden i repoet er altså nøyaktig den versjonen Tom tester.
- 21:23: Installert Python Playwright 1.56.0, som passer Chromium 141 som allerede ligger i skymiljøet. Første pip-forsøk fikk tidsavbrudd mot files.pythonhosted.org, det andre gikk.
- 21:24: Commit `3dd5b37`: web/ lagt inn uendret.
- 21:24 til 21:26: `python3 tools/walkthrough.py shots 844x390 high`: 21 av 21 PASS, ingen feil eller advarsler i konsollen, alle 27 antenner endte på az 026. Skjermbildet av sluttkortet var likevel helt svart.
- 21:25: Rettet fast sti `/home/claude/signal47-web/` i `tools/shoot.py`, `probe.py`, `probe2.py` og `probe3.py`. De leser nå `dist-single/index.html` fra web/, slik `walkthrough.py` og `looks.py` gjør. `tools/csptest.py` leser fortsatt `/tmp/csp_test.html`, som ingen skript i repoet lager. Lagt i todo.md.
- 21:26: Rotens README.md og AGENTS.md har fått en kort henvisning til web/, det oppføringen ca. 20:05 sa var gjort.
- 21:27: Undersøkt det svarte sluttkortet med et eget skript. Sluttkortet virker: tittelen toner inn etter 1 s og knappene etter 6,5 s, slik CSS-en sier. I programvare-renderingen går animasjonene mye tregere enn vegguret, så de hadde ikke startet da testen tok bildet etter fast 2,5 s. Feilen lå i testen, ikke i spillet.
- 21:28: `tools/walkthrough.py` venter nå til knappene på sluttkortet er helt synlige før bildet tas, og har fått sjekken "end card title and buttons visible". 22 sjekker i alt.
- 21:28: Docs/CURRENT_HANDOFF.md har fått en kort seksjon øverst om nettversjonen, så agenter som følger rotens AGENTS.md finner web/.
- 21:28 til 21:31: Ny `python3 tools/walkthrough.py shots 844x390 high`: 22 av 22 PASS, ingen feil eller advarsler i konsollen. Sluttkortet er synlig på bildet.
- 21:33: Målt sluttkortet headless i fire størrelser. Liggende mobil får ikke plass: ved 844×390 kuttes toppen av tittelen med 20 px og siste kredittlinje med 8 px, ved 667×375 toppen av tittelen med 9 px. Knappene er synlige. 390×844 og 1280×800 er OK. Lagt i todo.md sammen med to andre funn: 3D-scenen tegnes fortsatt bak det svarte sluttkortet, og skjermen er svart til all lyd er dekodet etter Start.
- Spillkoden er ikke endret i denne økta. Repoet skal bygge nøyaktig den artefakten Tom tester på mobil. Rettelser av funnene over gjøres som egne steg.
- UNVERIFIED, uendret: fps på ekte telefon og PC, ekte berøringsinput, lydmiks.
- 22:00: Tom: «lag PR mot main og merge». Laget [PR #24](https://github.com/Tombonator3000/SIGNAL-47/pull/24) fra `ccr-30e38858-767d90` mot main. main sto fortsatt på `ebd784d`, så grenen flettes uten konflikt. Ingen arbeidsflyter i repoet kjører på pull requests, så det er ingen CI-sjekker å vente på. memory.md, todo.md og Docs/CURRENT_HANDOFF.md viser nå til PR-en, og todo-punktet om PR er fjernet. PR-en flettes som vanlig merge-commit, slik de tidligere PR-ene er flettet, rett etter denne oppføringen. GitHub viser faktisk flettestatus.

### Claude Code (sky): GitHub Pages

- 22:06: Tom: «Gjør så den kan spilles fra git med pages». https://tombonator3000.github.io/SIGNAL-47/ ga 404 (Site not found), så Pages var ikke slått på for repoet.
- 22:06: `npm run build`: PASS. `dist/` har en index.html med relative stier (`./assets/...`) og egne filer for kode, lyd og fonter, til sammen ca. 1,4 MB. Det virker fra en undermappe fordi `vite.config.ts` har `base: './'`.
- 22:08: Startet grenen `ccr-30e38858-767d90` på nytt fra main (`21806f9`), siden PR #24 er flettet.
- 22:08: La til `.github/workflows/pages.yml`. Den bygger web/ på GitHub sine egne maskiner (ubuntu-latest, Node 22) med `npm ci`, `npm run typecheck` og `npm run build`, og publiserer `dist/` med actions/deploy-pages. Den starter ved push til main som endrer web/ eller arbeidsflyten, og kan startes for hånd. Checkout er låst til samme commit som de andre arbeidsflytene i repoet bruker. De andre handlingene bruker hovedversjoner: setup-node v4, configure-pages v5, upload-pages-artifact v3 og deploy-pages v4. YAML-en er lest inn og kontrollert med PyYAML.
- 22:08: `tools/walkthrough.py` kan teste et annet bygg med miljøvariabelen `S47_URL`.
- 22:10: Oppdatert web/README.md (adresse, publisering, test fra undermappe), web/AGENTS.md (ny seksjon Publisering: det som flettes til main, er ute på nettet noen minutter senere), memory.md (beslutningen, og at undermappen og delt localStorage må tas hensyn til), todo.md (Tom må slå på Pages én gang), rotens README.md og Docs/CURRENT_HANDOFF.md.
- 22:08 til 22:11: Testet Pages-bygget slik Pages serverer det: `dist/` kopiert til `SIGNAL-47/` og servert med `python3 -m http.server`. Alle 12 filene svarte 200. `S47_URL=http://127.0.0.1:8047/SIGNAL-47/ python3 tools/walkthrough.py shots 844x390 high`: 22 av 22 PASS, ingen feil eller advarsler i konsollen, ingen 404 i serverloggen. Nettleseren hentet kode, CSS, alle fire fonter og de fem store lydfilene fra undermappen. De seks minste lydfilene ligger inne i koden.
