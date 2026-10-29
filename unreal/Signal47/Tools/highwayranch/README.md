# Delt, opprinnelig gårdsgeometri

**Status:** Under arbeid. **Gjelder:** ENV-04, bare det eksisterende gårdsmotivet over road/SARO-overgangen. **Sist kontrollert:** 29. oktober 2026.

`S47HighwayRanch.cpp` partisjonerer indeksene i de tre opprinnelige, materialblandede modellene. Originale native assets beholdes; alle posisjoner, pakkede tangenter/normaler, UV-er, farger og materialslots kopieres uten normalisering. Lokale vertexkoordinater og trekantvinding er uendret.

| Kilde / node / materiale | Opprinnelig | Delt gård | Rest i opprinnelig komponent |
| --- | --- | --- | --- |
| g910 / 1301 / 590 | 204 | 12 | 192 |
| g911 / 1302 / 589 | 3708 | 248 | 3460 |
| g912 / 1303 / 587 | 360 | 40 | 320 |
| Sum | 4272 | 300 | 3972 |

Alle hjørnene må ligge i opprinnelig road-lokal X 390 til 485 / Z 140 til 235 for å tilhøre gården. En trekant som krysser grensen avviser hele aktiveringen. Foreldre 1289/1240, material-ID-er, native assetstier, antall, faktiske materialbindinger og CPU-data kontrolleres før noen original komponent byttes. Manglende data eller `S47HighwayRanchOff`, `S47LegacyHighway`, `S47PropsOff` eller `S47LegacyCalicheStreet` gir samlet originalfallback. Kravene til backdrop og road-root følger `BuildHighwayLandscape`.

De tre `highway:ranch:`-komponentene får `NoCollision` og går gjennom eksisterende kanonisk deling, med road-origin 8000 og SARO-offset fra `SARO_IN_ROAD`. Restkomponentene beholder transform, synlighetsprodusent, kollisjonsflagg, responses og original `UBodySetup`. Klonene beholder solid native RT og initialiserer RT-representasjonen etter samme motorpolicy som `S47HighwayPower::MakeWirePatch`. Ingen Nanite-, VSM- eller global rasterpolicy endres.

`AuditHighwayRanch` kontrollerer originale og klonede renderbuffere/indeksrekkefølger med SHA-1, antall, slots, materialbindinger, kollisjon, bounds og faktisk kanonisk plassering/synlighet. Originalassetene holdes med `TStrongObjectPtr`; komponentenes `StaticMesh`-egenskap holder de transiente klonene. De nye vertex-/indeksbufferne beholder CPU-data for senere audit og rutekvittering. Audit er ingen påstand om bildeaksept før faktisk kjøring.

## CPU-retensjon

`prepare_cpu.py` kjøres av koordinatoren i et reservert, isolert editorvindu. Det importerer ingenting og endrer bare `allow_cpu_access` på de tre nøyaktige originalmodellene. Alle tre valideres først. Lagret struktursignatur for LOD-antall, slots, RT, Nanite og collision-bodyflagg må være uendret. Rapporten i profilens `Saved/highway-ranch-cpu.json` skiller dette fra runtimebuffer- og rutekontroll.

CPU-R1 ble avvist før første mutasjon fordi `ImportedMaterialSlotName` krever `get_editor_property` i denne UE-versjonen. R2 består med uendrede struktursignaturer og CPU-retensjon på alle tre. Råbevis ligger i `~/scratch-bm/Signal47-farm-access-20261029` og `/tmp/S47-farm-cpu-prepare-r2/Saved/highway-ranch-cpu.json`.

Statisk GLB-kontroll gir 300/3972 og null kryssende trekanter. C++-bygg, grafisk sammenheng, fysisk portprøve og disk/Continue må kvitteres av koordinatoren etter fryst kode. Ingen ny gårdsoppgave, modell, innendørsadgang eller endring av portens spillstatus inngår i denne geometridelen.
