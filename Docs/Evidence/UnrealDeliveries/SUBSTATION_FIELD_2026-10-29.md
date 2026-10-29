# Trafoutstyr og bakke ved Caliche junction

**Status:** Kilde og native kontrollert i [PR218](https://github.com/Tombonator3000/SIGNAL-47/pull/218). **Gjelder:** FIX-07. **Sist kontrollert:** 29. oktober 2026.

Claude har erstattet de enkle feltmodellene med et detaljert testskap, en referansestolpe, en kulvert og gradering ved servicefeltet. Modellene møter den målte bakken og fundamentene. Codex overtok den uleverte grenen kl. 04:28 etter HANDOFF-regelen om mer enn 75 minutter uten hjerteslag. De 44 eksisterende deltafilene ble sikret med kontrollsummer før overtakelsen.

De nye delene erstatter bare den synlige geometrien. T01/T02 beholder sine interaksjonsproxies, kollidere, foto-occludere, strøm-/lydlogikk og lagringsdata. Manglende eller foreldede native assets beholder hele den tidligere visningen. Blender-kilde, FBX-er, materialreferanser og gjenbyggbare verktøy ligger i [kildepakken](../../../unreal/Signal47/Tools/substationfield/README.md). Eksisterende trafomaterialer og deres krediteringer beholdes.

Codex fant at den nye mappen manglet i cook-konfigurasjonen og la til eksplisitt `/Game/SubstationField`. Dette klargjør neste pakke; en konfigurasjonslinje beviser ikke at et pakket spill er prøvd.

| Kontroll | Faktisk resultat | Begrensning |
| --- | --- | --- |
| Claudes native import R2 | PASS, 54 kontroller, fire modeller med tre LOD-er. | Importen aktiverer ingen installasjon. |
| Fersk lagret LOD-leser | PASS, 84 kontroller. Alle LOD-vertices matcher faktisk FBX i begge retninger med toleranse 0,025 cm. | Lesende editor-commandlet, ingen spillerinput. |
| T01 på fryst kilde `ed2f85486fdf01fae009461fd0a072debc9a9541` | PASS i R2: 1 709 selvtester, faktisk lampe, sammenligning, serialisering, gangadkomst, stengt yard, brukstråler og diskrestore. Normal avslutning 0. | R1-wrapper mottok SIGTERM etter testens PASS og stoppet sin egen child. Råforsøket beholdes som avbrutt økt. |
| Claudes T02 | PASS: 65 API-kontroller, 15 disk-/Flow-restores og 38,90 m gange. | Tidligere kapitler og felt-/returposisjoner er fixtures. Kvotefixture gjenbruker ekte fotograferte piksler. |
| Claudes SC02 | PASS: 249 kontroller, 21 disk-/Flow-restores, 352,19 m gange og faktisk bildeopptak. | Kontraktens fixtures og bevisgrenser består. Dette er ikke menneskelig gjennomspilling. |
| Før-/etterbilder | Faktiske runtimebilder dag og natt er åpnet og kontrollert. | Statiske kamerafixtures, ingen input- eller ytelsesmåling. |
| Pakket spill | UNVERIFIED. | Følger neste samlede kandidat. Ingen fullmatrise, Games-installasjon eller offentlig release her. |

[Før, dag](../../../unreal/Evidence/FIX07-2026-10-29/before-day.png) · [Etter, dag](../../../unreal/Evidence/FIX07-2026-10-29/after-day.png) · [Etter fra siden](../../../unreal/Evidence/FIX07-2026-10-29/after-left-day.png) · [Etter, natt](../../../unreal/Evidence/FIX07-2026-10-29/after-night.png).

Privat rågrunnlag: `~/scratch-bm/Signal47-fix07-20261029`, med Codex-kvitteringer i `codex-handoff/`. Alle 44 overtatte deltafiler er uendret gjennom testene. [Kontrollsummer og maskinlesbar kvittering](../../../unreal/Evidence/FIX07-2026-10-29/verification.json) binder kilden, nativefilene og resultatene. Ingen påstand om FPS, gamepad, subjektiv lytting eller hele spillets ferdigstatus følger av disse kontrollene.
