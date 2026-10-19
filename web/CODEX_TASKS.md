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

### 7. [x] Historie og hendelser, direkte bestilt av Tom (`codex/story-events-20261019`)

Base: main `d1bfbc0`, med nyere main `c06bf33` tatt inn på arbeidsgrenen. Bestilt direkte 19. oktober med gauntlet-loop og gjennomgang av repo/prosjektbibliotek. Fire vitneoppfølginger med gjenlesbare notatkopier, styrket 1947-bulletin, faktisk lagret motell-TV-bilde og tilgjengelig returanrop etter rom 6. Art Bell-valget og nyere lyd-/grafikkendringer er bevart. Se `HENDELSER.md` og `production/story_events_20261019/qa/README.md` for tester og begrensninger. Opprinnelig lokal leveranse. Tom har senere i samtalen eksplisitt bestilt innfletting til main og vanlig Pages-publisering for denne oppgaven. Flettet som `2fbed14`, Pages `39310466192` SUCCESS, alle publiseringsfiler identiske med testet bygg og live historieprøve 56/56.

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


### 6. [x] Art Bell som radiovert, direkte bestilt av Tom (`codex/art-bell-radio`)

Base: main `aa38e2b`. Ny lokal syntetisk parodiprofil, alle Earls eksisterende radioreplikker ferdig innspilt, inkludert dynamiske klokker, gutten, avmeldingen og kjøreinnslag. Eget lagret valg og separat bank. Kinski, innringerne og Roswell beholdes. Levering og kontroller føres i `ART_BELL_RADIO.md`.

Sluttkandidaten inkluderer main `d6dc68b` og består 51/51 Pages, 52/52 streng-CSP enkeltfil, motor 48/48, alle 12 kompatibilitetskjøringer og 13 spillregresjoner. Privatbanken er bevart og prøvd med/uten. Se `production/artbell/qa/validation.json`. Innfletting og vanlig Pages-publisering følger Toms tidligere autorisasjon og kontrolleres mot riktig head.

### 7. [x] Innringere og 1947-radio, direkte bestilt av Tom (`codex/radio-cast`)

Base: main `b6483fe`, med den nyeste historieutvidelsen. Tom ba om å gjennomføre etter kildesøket. 28 ferdige opptak fra gjeldende manus, fire innringertyper og en nyhetsleser. Nyhetssendingens seks linjer beholdes i hele 47 sekunder med retur til den avbrutte sendingen. Eget lagret valg, samme radiobuss og pausefølsomme teksting. Art Bell, Kinski, private lydeffekter og alle nye historieforløp bevares. Se `RADIO_CAST.md`.

Sluttkontroll: radiostemmer 70/70 Pages og 71/71 streng-CSP enkeltfil, motor 77/77, Art Bell 48/48 og arkiv 51/51. Begge bygg, med/uten privatbank, alle 16 kompatibilitetskjøringer, 13 spillregresjoner og lydkostnad består. Stemmemanus og tidligere banker er bevart. Kilde-/bygghasher og grenser for faktisk verifisering står i `production/radiocast/qa/validation.json`. Vanlig innfletting og Pages-publisering følger den eksisterende autorisasjonen og kontrolleres etter push.

### 8. [x] Art Bell også som Earl i byen, direkte bestilt av Tom (`codex/art-bell-town`)

Base: main `210b52f`, der radiovert og de nye innringerne allerede er flettet inn. Tom ba om samme Art Bell-stemme i samtalen med Earl. Alle 16 eksisterende EARL-replikker får egne opptak fra samme lokale profil, uten å endre manus eller progresjonskrav. Samme lagrede Art Bell-valg styrer radio og Earl, uavhengig av Kinski. Automatisk stemme ved buzzer og nye svar, med replay/stopp og ryddig avbrudd. YOU, asider, andre personer og vitnepapirer beholdes som egne tekst-/fortellerroller. Tidligere 1698 MP3 og tre kataloger bevares. Produksjon og bevis i `production/artbell-town/`.
