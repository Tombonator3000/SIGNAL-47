# Verifisering av Kinski-opplesning

Kjørt 19. oktober 2026, fra `codex/kinski-read-aloud`, base `bada636`. Bygg- og kildehash ligger i `../build-manifest.json`. Begge bygg er fra samme kilde og komplette lydsett. Vanlig bygg er servert med prosjektets `rangeserve.py` under `/SIGNAL-47/` på port 8497. Enkeltfilen er i tillegg kopiert til en ny midlertidig mappe og testet med prosjektets strenge CSP.

| Kontroll | Resultat |
| --- | --- |
| TypeScript og begge produksjonsbygg | PASS |
| Lydformat, hash, dekning, energi, varighet og ingen tapte tekstbiter | 328/328 PASS, se `../verification.json` |
| Avspillerens kø, avbrudd, feil og minnegrense | 34/34 PASS |
| Opplesning, vanlig bygg med undermappe | 37/37 PASS |
| Opplesning, offlinefil med streng CSP | 38/38 PASS |
| walkthrough, chapter1, chapter2, chapter3 | 57/57, 75/75, 76/76, 72/72 PASS |
| chapter4, chapter5, chapter6 | 69/69, 55/55, 42/42 PASS |
| devmenu, drives, ambience, diner | 14/14, 23/23, 7/7, 15/15 PASS |
| music, vanlig bygg og enkeltfil | 12/12 og 5/5 PASS |
| saves og steercheck | 19/19 og 5/5 PASS |

Nettleserprøvene bruker Chromium med prosjektets GPU-oppsett på RTX 3080. Opplesningsprøven dekker faktiske spilldokumenter, papir og beskrivelse hver for seg, Nora-samtalen, ekte MP3-dekoding, avspilling på hovedkanalen, volum, rekkefølge, stopp, manglende opptak, avbrutt lasting/dekoding, pause, lastet lagring og lagret valg. Den prøver både 1280×800 og 844×390 med emulert berøring. Nettleseren starter fra isolerte lagringer. Ingen konsollfeil eller lyd-404 i de beståtte prøvene.

Første offlineforsøk stoppet i testens `wait_for_function`, som krevde `unsafe-eval`. Testen ble rettet til polling via nettleserprotokollen, og hele offlineprøven ble kjørt på nytt. CSP ble beholdt. Første logg er bevart som `readaloud-single-first-attempt.log`.

De åtte skjermbildene i `pages/` og `single/` viser dokument, samtale og mobilkontroller. Pages-bildene er visuelt gjennomgått. Fullkapitlenes øvrige skjermbilder er bevart lokalt i samtalens `outputs/signal47/integration-qa/`; testkommandoene i `regression-results.json` gjenskaper dem. Dette begrenser mengden testbilder som følger kodeleveransen.

UNVERIFIED: fysisk berøring på telefon, faktisk lyd fra høyttaler/hodetelefoner, opplevd stemmelikhet og sinne, samt fps og minnebruk på brukerens enheter. Web Audio-målinger og automatisk transkripsjon er teknisk bevis, ikke en lyttetest. ASR-prøvens usikre A/B/C-linje er identifisert i `../README.md` og `../verification.json`.

Tester for denne funksjonen: `node tools/readaloud-core.cjs` og `python tools/readaloud.py OUTDIR`, med prosjektets Python-miljø og nettleserkø. Sett `S47_URL` til det vanlige bygget for undermappeprøven. Uten variabelen testes `dist-single/index.html` under streng CSP.
