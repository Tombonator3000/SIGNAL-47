# SARO: bakke, omgivelser og fjell, overlevering til Codex

**Status:** Under arbeid. **Eier nå:** Codex (overtar fra Claude 4. november). **Gren:** `claude/east-wing-20261103` (PR #380). **Sist kontrollert:** 4. november 2026.

Tom ønsker at bakken og omgivelsene rundt SARO ser ekte ut og henger sammen med bygningene, og at fjellene ser ut som New Mexico fra avstand. Spilleren kommer sjelden nær fjellene.

## Hva som finnes nå

| Del | Hvor | Status |
| --- | --- | --- |
| Fjell (CN5-bakgrunn `prop:streetGroundMesas`, gamle `mesas`/`mesas:near`) | `Source/.../S47MesaRock.cpp`, materiale fra `ArtSource/Backdrop/import_mesa.py` | Ferdig v1: fotosandstein i verdensskala (lisensiert 8K, `/Game/Licensed/Backdrop/M_S47_MesaRock`, utenfor Git), NM-palett per spor. Uten materialet beholdes originalen. |
| Bakke (eksportens sandmesh: SARO-node 7, saroCorridorGround, road/oldroad desert) | Samme modul, materiale fra `ArtSource/Backdrop/import_ground.py` (`/Game/Backdrop/M_S47_DesertGround`) | Ramme ferdig: tre lag (løs sand, lys caliche-skorpe, grus) blandet med storskala-masker, ganger eksportens toppunktfarge og farge. Tre egne ImageGen-sett med BaseColor, NormalGL og Roughness er laget og importert. T4 brukes fortsatt til makromasker og som eksplisitt fallback. |
| Overgang bygg/bakke, østfløyen | `S47EastWing::Grime()` | Ferdig v1: sprutskitt opp sokkelen og vindblåst støv på bakken langs alle fire fasader. |
| Overgang bygg/bakke, hovedbygget og andre bygg | Ikke laget | Gjenstår. |

Kontroll ved overlevering: 2295 selvtester OK; `-S47EastWingDriveTest` 18/0. Bilder: `~/scratch-bm/Signal47-eastwing-20261103/ground-r1/` (bruk `-S47Freeform -S47EastWingShots`, visning 02, 16 og 03 viser bakken).

## Oppgaver for Codex, i prioritet

1. **Bakketeksturer (ImageGen), implementert 4. november.** Tre tileable sett på 2048, sett ovenfra, uten innbakt lys: `T_GroundSand` (løs, lys rødbrun ørkensand med vindrifler), `T_GroundCrust` (hard, sprukket lys caliche-skorpe med små steiner) og `T_GroundGravel` (grov grus og småstein). Hver med BaseColor (sRGB), Roughness og NormalGL. Importer dem som `/Game/Backdrop/Textures/T_Ground{Sand,Crust,Gravel}_D` (og `_N`, `_R`), og bygg materialet på nytt med `S47_GROUND_TEXTURES=1 python3 unreal/ArtSource/Backdrop/run_ground_import.py --apply --output <ny mappe>`. Importskriptet bruker dem automatisk når de finnes. Normal og roughness er nå koblet til de samme lagmaskene. NormalGL konverteres ved import. Kilder, prompter og størrelsesbegrensning står i `unreal/ArtSource/Backdrop/GROUND_TEXTURES.md`.
2. **Juster blandingen etter bilder.** Parametre i materialet: `GroundTint`, `CrustTint`, `GravelTint`, `CrustAmount`, `GravelAmount`; skalaer i `import_ground.py` (2,4 m / 9 m sand, 3,2 m skorpe, 1,6 m grus, masker 150 m og 40 m). Målet er flekker og striper i stedet for gjentak, og ikke mørkere eller mer mettet enn dagens sand.
3. **Overgang ved hovedbygget og andre bygg.** Gjenbruk decalene i `/Game/EastWing/Decals` (sokkelskitt `M_EW_GrimeWallBase`, bakkestøv `M_EW_FloorEdgeDust`) etter mønsteret i `S47EastWing::Grime()`. **Merk:** Unreal legger decal-teksturens U/V langs lokal Z/Y, så vegg- og gulvdecals må ha roll 90 og byttede utstrekninger (se kommentaren der). Kontroller retningen med `-S47EastWingDecalProbe`, som farger båndene røde.
4. **Grusbelte rundt bygg.** 0,5–1 m grusstripe langs fasadene (ekte mesh eller decal med grusteksturen fra punkt 1), med en kant mot sanden.
5. **Asfalt og forplass.** Sprekker, lappede felt, oljeflekker under parkeringsplassene, sand innblåst på kantene og slitt oppmerking.
6. **Vegetasjon etter regler.** Tettere ved takrenner, i grøfter og langs veikanter, i klynger og aldri på kjøreflater. Eier: RouteNature og GroundCover (Codex).
7. **Atmosfære.** Litt mer høydedis og blåtone på avstand, så fjellene glir inn i bakken. Eier: `S47Lighting` (global tåke). Sjekk dag og natt (`-S47EastWingHour=23`).
8. **Fjell v2 (valgfritt).** Egne ImageGen-teksturer for rød og oker sandstein, lys kappe og talus i stedet for den lisensierte, slik at fjellene også fungerer uten `/Game/Licensed`. Samme materialoppsett i `import_mesa.py`.

## Regler

- Én tung jobb om gangen med `require-idle` og `production_lease`; importer går gjennom runnerne i `ArtSource/Backdrop`.
- Ikke overskriv Claudes prosedyrale generatorer eller Codex-fotoene: generatorene nekter uten `S47_EASTWING_PROCEDURAL=1`.
- Lisensierte filer forblir under `/Game/Licensed` og utenfor Git.
- Før en linje i `unreal/OPPGAVER.md` per levert punkt. Ta skjermbilder før og etter.

4. november Codex: punkt 1 implementert, med første fargejustering fra punkt 2. Ni PNG-er/native teksturer og materialbygg består; før-/etterbilder ved kl.16 ligger i `~/scratch-bm/Signal47-ground-textures-20261104/before-r2` og `after-r3`. Originaler 1254, resamplet til 2048. Ingen ny installasjon. Punkt 3 til 8 og videre visuell polering er ikke levert i dette passet.

4. november Codex: asfalt/oppmerking fra punkt 5 videreført i SARO-PARKING-02. Sammenhengende Caliche-asfalt uten gjentatt sandkant, diskret kantstøv og tre slitte parkeringsfelt vest for porten. Hele forplassen/innkjøringen åpnet for kjøring med bevarte reelle hindringer. Bygg/native import og Freeform24/Story9 består; ingen installasjon. Egne oljeflekker og større lokale asfaltreparasjoner er fortsatt ikke laget.

4. november Codex: Toms tillegg om mikrorelieff og små faktiske ujevnheter er implementert i material/import. Tre estimerte 16-bits høydekart rekonstrueres fra eksisterende normaler. Nærfelt-parallax følger D/N/R-lagene; Nanite bruker 7 cm total høydeforskjell (±3,5 cm) på fem ørkenmesher med opprinnelig kollisjonsflate. Detaljer og regenerering står i GROUND_TEXTURES.md. Dette er fortsatt editorleveranse, ikke installert spill.
