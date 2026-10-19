# CODEX_TASKS.md: kodeoppgaver til ChatGPT og Codex

Tom avtalte 18. oktober 2026 at ChatGPT og Codex kan hjelpe med koding. Claude er fortsatt lead: Claude skriver oppgavene her, ser over det som leveres og fletter. Hver oppgave er avgrenset slik at den ikke krysser det Claude jobber med samtidig.

## Slik jobber du

- Les `AGENTS.md`, `memory.md` (særlig «Kjente fallgruver»), og de filene oppgaven nevner. Følg stilen i koden rundt. Tekst i spillet er engelsk (New Mexico, 1986), dokumentasjon er norsk uten emoji og uten tankestrek.
- Lag en egen gren og arbeidskopi for oppgaven: `git worktree add .claude/worktrees/codex-<navn> -b codex/<navn> <base>` fra repoets rot, med basen oppgaven oppgir. Lenk inn `node_modules`: `ln -s /home/tom/SIGNAL-47/web/node_modules web/node_modules`. Ikke kjør `npm install`.
- Bygg og sjekk fra `web/`: `npx tsc --noEmit -p .`, `npm run build:single`, `npm run build`.
- Nettlesertestene (`web/tools/*.py`, kjøres med `~/.venvs/signal47/bin/python`) er tunge. Kjør dem bare gjennom køen med to plasser hvis den finnes: `bash /tmp/claude-1000/-home-tom/3c32347d-240d-4adc-9b1a-757aeb6ab4e2/scratchpad/slot.sh <kommando>`. Finnes den ikke, kjør én test om gangen.
- Skriv et eget avsnitt nederst i `log.md` med UTC-tid, hva du gjorde, filer og testresultat (PASS, FAIL eller UNVERIFIED med tall).
- Commit på grenen din. Ikke push og ikke lag PR; Claude tar det videre.
- Når oppgaven er ferdig: kryss den av her med grennavnet.

## Oppgaver

### 1. [x] Lagring uten IndexedDB: hvert fotografi lagres én gang (`codex/saves-photo-store`)

Base: `integration`. Filer: `src/core/saves.ts`, `tools/saves.cjs`.

Uten IndexedDB (privat vindu i noen nettlesere) lagres sakene i `localStorage`, og hvert fotografi ligger da inne i hver eneste lagring. Et saksbilde er rundt 210 kB, og autolagringen i kapittel 2 alene er 442 kB. Fra kapittel 3 sprenger seks lagringsplasser grensen på 5 MB i Firefox og Safari, og den grensen deles med Toms andre sider på `tombonator3000.github.io`. Med IndexedDB lagres hvert bilde bare én gang etter innhold (`{id, url}`); gjør det samme i reserveløsningen: bildene i egne `s47.`-nøkler etter innholdshash, lagringene peker på dem, og et bilde som ingen lagring bruker lenger, slettes. Gamle lagringer med bildene inne skal fortsatt lastes. Utvid `tools/saves.cjs` med en sak med seks lagringer og fullt album (36 frie bilder, `core/album.ts`) uten IndexedDB, og mål størrelsen før og etter. Kjør også `tools/album.py` og `tools/chapter1.py` med `S47_NO_IDB=1` slik testene gjør det i dag.

### 2. [x] Bevisbordet også i kapittel 1 (`codex/board-chapter1`)

Base: `integration`. Filer: `src/story/boardRules.ts`, `src/story/caseBoard.ts`, `src/story/Chapter1.ts`, `src/ui/Panels.ts`, `tools/chapter1.py`, `tools/board.py`.

Bevisbordet (`B`, eller Board i notatboka) har regelbøker for kapittel 2 til 5, men kapittel 1 bruker fortsatt valgpanelene sine: B-12 mot R-07, hypotesen og de to eksponeringene. Tom ville ha samme bord i alle kapitler (`todo.md`, punktet om bevisbord i stedet for faner). Lag en regelbok for kapittel 1 etter mønsteret i `boardRules.ts`, slik at disse funnene registreres med tråd og RECORD på bordet, med de samme svarene og den samme gangen i historien som i dag (det kapittelet kaller når et funn er gjort, skal kalles likt). Feil kobling får et kort svar og slakker tråden. Behold det gamle panelet bare der bordet ikke kan brukes. Oppdater `tools/chapter1.py` og `tools/board.py`, og kjør `chapter1.py`, `chapter1.py` med `S47_NO_IDB=1`, `chapter2.py`, `board.py` og `walkthrough.py 1280x800`.


### 3. [x] Valgfri Kinski-opplesning, direkte bestilt av Tom (`codex/kinski-read-aloud`)

Base: main `bada636`. Bestilt direkte i samtalen 19. oktober. 328 ferdig genererte lydklipp, valgfri innstilling og lese-/stoppknapper i papir- og samtalepaneler. Begge bygg og pålagte tester PASS, med resultater i `production/kinski/qa/`. Se `KINSKI_READALOUD.md` for bruk, tekstdekning og lytting som gjenstår hos Tom. Lokal commit uten push eller PR.


### 4. [x] Telefonstemmene som ferdig Kinski, direkte bestilt av Tom (`codex/kinski-read-aloud`)

Videreføring av oppgave 3 fra `c3dd6b1`. 24 unike telefonreplikker, ni nye opptak. Automatisk avspilling ved svar/oppringing/telefonvalg, ekte bufferlengder, pause og ryddig avbrudd. Telefonmanuset følger leveransen. TypeScript, begge bygg, telefonprøver og pålagte spilltester PASS. Lokal commit uten push eller PR.

### 5. [x] Alle fortellerbeskrivelser som Kinski, direkte bestilt av Tom (`codex/kinski-descriptions`)

Base: main `95719a4`. Nærblikk etter 650 ms, gjenta med V eller mobilknapp, og leseknapper for synlig tekst i paneler, mottaker, notatbok, kart, bevisbord og frie bilder. 1555 ferdige klipp, med alle 337 tidligere MP3 bevart uendret. Dynamiske klokker og målinger settes sammen uten å miste tall eller ord. Beskrivelser 115/115 Pages og 116/116 offline, motor 55/55 og tekstkorpus 26/26; begge bygg, telefoner og full spillregresjon PASS. Se `production/kinski/descriptions-qa/README.md`. Tom har i samtalen autorisert innfletting og vanlig publisering til nettspillet.


### 6. [ ] Art Bell som radiovert, direkte bestilt av Tom (`codex/art-bell-radio`)

Base: main `aa38e2b`. Ny lokal syntetisk parodiprofil, alle Earls eksisterende radioreplikker ferdig innspilt, inkludert dynamiske klokker, gutten, avmeldingen og kjøreinnslag. Eget lagret valg og separat bank. Kinski, innringerne og Roswell beholdes. Levering og kontroller føres i `ART_BELL_RADIO.md`.
