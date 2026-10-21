# SIGNAL / 47 unreal: arbeidsinstruks

Denne fila gjelder alt under `unreal/`. For denne mappa erstatter den Unity-reglene i rotens `AGENTS.md`. Les `README.md`, `PORTERING.md`, `memory.md`, `todo.md` og de siste oppføringene i `log.md` før du starter, og kontroller faktisk Git-status.

- Nettversjonen i `web/` er spillet som publiseres. Unreal-utgaven er en prøve som kan vokse til hele spillet; ingenting her endrer `web/`.
- `Unity/` er arkiv og referanse. Modellene der leses av importskriptet, men ingenting i `Unity/` redigeres.
- Repoet er offentlig. Kjøpte eller lisensierte assets legges ikke inn. Modellene i `Unity/Assets/Signal47/Art` er egne (Blender) eller CC0 (Poly Haven); se `web/THIRD_PARTY_NOTICES.md` og manifestene ved siden av modellene.
- Unreal: logikk i C++, portert fil for fil fra `web/src` etter `PORTERING.md`. Verdenen hentes fra nettversjonen med `Tools/webexport` (ingen håndbygd geometri). Binærfiler (uasset) går gjennom Git LFS. Binaries, Intermediate, Saved, DerivedDataCache og de genererte mappene under Content (`Web`, `WebData`, `Audio`, `Licensed`) committes aldri.
- Bygg ikke editor-målet mens editoren er åpen.
- Noe er ferdig først når det virker i den pakkede builden og `Scripts/screenshots.sh` går gjennom.
- Kanon fra `web/memory.md` gjelder: 1420.405 MHz, 47 sekunder, 27 antenner, az 026. Spilltekst på engelsk (New Mexico, 1986). Dokumenter og logg på norsk, uten emoji og uten tankestrek.
- Logg alt i `log.md` med dato og klokkeslett (Europe/Oslo). Hold `memory.md` og `todo.md` oppdatert.
