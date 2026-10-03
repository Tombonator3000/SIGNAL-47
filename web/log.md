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
