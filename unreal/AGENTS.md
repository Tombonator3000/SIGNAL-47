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
