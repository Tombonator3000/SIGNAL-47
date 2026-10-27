# SIGNAL / 47 unreal: arbeidsinstruks

Denne fila gjelder alt under `unreal/`. For denne mappa erstatter den Unity-reglene i rotens `AGENTS.md`. Les `README.md`, `PORTERING.md`, `memory.md`, `todo.md` og de siste oppføringene i `log.md` før du starter, og kontroller faktisk Git-status.

- Unreal-utgaven er spillet (Toms beslutning 22. oktober 2026). `web/` er historisk referanse og endres ikke.
- `Unity/` er arkiv og referanse. Modellene der leses av importskriptet, men ingenting i `Unity/` redigeres.
- Repoet er offentlig. Kjøpte eller lisensierte assets legges ikke inn. Modellene i `Unity/Assets/Signal47/Art` er egne (Blender) eller CC0 (Poly Haven); se `web/THIRD_PARTY_NOTICES.md` og manifestene ved siden av modellene.
- Unreal: logikken er portert fra `web/src` (se `PORTERING.md`). Nye forbedringer (lys, modeller, mekanikk) er ønsket; når noe avviker fra nettversjonen, si det i en kort kommentar. Verdenen bygges fra eksporten i `Tools/webexport`, men kan byttes ut eller forbedres der Tom ber om det. Binærfiler (uasset) går gjennom Git LFS. Binaries, Intermediate, Saved, DerivedDataCache og de genererte mappene under Content (`Web`, `WebData`, `Audio`, `Licensed`) committes aldri.
- Bygg ikke editor-målet mens editoren er åpen.
- Etter en endring: bygg, kjør det som er endret og et kort utvalg av `Scripts/screenshots.sh` i den pakkede builden. Hele `screenshots.sh` kjøres før fletting til main eller når Tom ber om det.
- Kanon fra `web/memory.md` gjelder: 1420.405 MHz, 47 sekunder, 27 antenner, az 026. Spilltekst på engelsk (New Mexico, 1986). Dokumenter og logg på norsk, uten emoji og uten tankestrek.
- Felles oppgaveliste og arbeidsdeling med Codex: `OPPGAVER.md`.
- Logg alt i `log.md` med dato og klokkeslett (Europe/Oslo). Hold `memory.md` og `todo.md` oppdatert.

## PR-er går mot main, bestemt 27. oktober 2026

- Alle PR-er for spillet går mot `main`. Lag nye grener fra oppdatert `origin/main`, ikke fra en annen arbeidsgren.
- Sjekk grunnlagsgrenen før fletting, for eksempel med `gh pr view <nr> --json baseRefName`. Er den ikke `main`, rett den med `gh pr edit <nr> --base main` før fletting. Regelen om hele `screenshots.sh` før fletting gjelder alle PR-er som endrer spillet.
- `unreal-port-core` var avtalt integrasjonsgren for historiearbeidet fram til 26. oktober (`HISTORIE_IMPLEMENTERINGSPLAN.md`), og PR #147 til #166 og #168 gikk dit med vilje. Main fikk det meste via PR #167, men de 17 siste commitene (PR #163 til #166 og #168, med SC01) ble aldri flettet videre. Det er det regelen over skal hindre: ingen arbeid skal bli stående på en sidegren. Flettingen inn i main står øverst i `OPPGAVER.md`, og `unreal-port-core` slettes når den er flettet.

## Varig grense for lokale testbygg, bestemt 25. oktober 2026

- Behold gjeldende verifiserte bygg og maksimalt **to eldre komplette testbygg** lokalt. Installerte standardfiler beholdes alltid. Aktive pakkekjøringer, testkandidater som fortsatt undersøkes og nødvendige sammenligningsbygg beskyttes til oppgaven er avsluttet; tell og rydd dem når de frigis.
- Etter vellykket pakking og avsluttet test/levering: kjør `python3 Signal47/Scripts/prune-test-builds.py --current /absolutt/sti/til/gjeldende/Linux` fra `unreal/`. Oppgi andre aktive kandidatmapper med `--protect`. Verktøyet lager en tørrkjøringsplan og beholder de to nyeste eldre pakkene. Kontroller planen og kjør samme kommando med `--apply` når ingen Unreal-/spill-/pakkejobb er aktiv. Dette er vanlig autorisert vedlikehold; regelen krever ikke en ny brukerforespørsel for hver opprydding.
- Verktøyet pensjonerer bare genererte pakkecontainere og den tilhørende spillbinærfilen/debugfilen i gamle `Signal47*`-arbeidsmapper under `~/scratch-bm`. Kilder, Blender-/modellfiler, assets, `Saved`, personlige lagringer, logger, skjermbilder, testbevis, Engine-filer og selve mappeskallet bevares. Gamle lette mappeskall teller ikke som komplette testbygg. Slett aldri hele blandede arbeids-/bevismapper bare på grunn av navn eller alder.
- Ikke rydd en kandidat mens den bygges, kjøres, testes eller er nødvendig for en åpen undersøkelse. Verktøyet nekter `--apply` ved aktive Unreal-/spill-/pakkeprosesser og avviser symlink-/filendringer. Ved påkrevd midlertidig unntak fra antallsgrensen: dokumenter hvilke bygg som er beskyttet og hvorfor, og fullfør oppryddingen når behovet bortfaller. Før faktisk resultat og gjenværende bygg i oppdragets logg; tørrkjøring er ikke utført sletting.
