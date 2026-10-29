# ENV-04: originale bilder og avgrensede kvitteringer

**Status:** Referanse, målrettet kandidat R1. **Gjelder:** Gårdens synlighet og den stengte porten. **Sist kontrollert:** 29. oktober 2026.

[Leveringsrapporten](../../../Docs/Evidence/UnrealDeliveries/FARM_ACCESS_2026-10-29.md) forklarer kilde, koordinater, fixtures og gjenstående tester. Bildene nedenfor er byteoriginale enginebilder, åpnet av Codex. Ingen bilder er konseptbilder eller retusjert.

| Fil | Faktisk innhold og begrensning |
| --- | --- |
| [road-before-handoff-ranch.png](road-before-handoff-ranch.png) | Baseline R2, faktisk cockpit før normal områdesovergang. Gården er liten i landskapet. |
| [saro-after-handoff-ranch.png](saro-after-handoff-ranch.png) | Samme landemerke etter overgangen fra en ny faktisk bilposisjon. Komponentmetadata viser alle tre gårdskomponenter skjult; bildene er ikke samme-pose A/B. |
| [failure-at-gate.png](failure-at-gate.png) | Baseline etter første målte OBB-kryssing og normal brems. Den første kryssingsposen ligger separat i kvitteringen; bildet alene måler ikke kollisjon. |
| [baseline-r2-receipt.json](baseline-r2-receipt.json) | Kompakt avledning av faktisk FAIL-resultat, pose-/kameradata og originalfilhasher. Hele råresultatet er bevart privat. Ingen senere park-/gang-/lagringsaksept. |
| [cpu-prepare-r2-original.json](cpu-prepare-r2-original.json) | Byteoriginal native-forberedelseskvittering. Strukturkontroll av tre CPU-flagg, ikke fysisk/runtime/pakkeaksept. |

Alle baselineforsøk og originalkopier beholdes i `~/scratch-bm/Signal47-farm-access-20261029`. Historisk E01-kamerapose og byggidentitet er fortsatt ukjent. Ingen installasjon eller offentlig release.

Bestått editor R3 er bundet til `575ffd15`: [kvittering](editor-r3-receipt.json), [gård etter vanlig områdeskifte](editor-r3-after-handoff-ranch.png), [bil stoppet ved porten](editor-r3-car-port-blocked.png) og [ALDERETE-postkassen](editor-r3-mailbox.png). Alle sju originale rutebilder er åpnet; de øvrige originalene og hele resultatet er bevart privat med hash i kvitteringen. Metadata/bufferaudit beviser synlig plassering; selve gården er liten ved 400 meters avstand, og ingen ny subjektiv fjernlesbarhet eller E01-kamerarekonstruksjon hevdes.

Pakket R1 fra `c301ead9` består 1720 selvtester og hel rute 27 / 380,332192 m bil / 30,965562 m gange med disk/title/Continue. [Pakket kvittering](packed-r1-receipt.json) binder kilde, åtte payloads og alle sju åpnede bilder. Fire byteoriginaler følger: [gården etter områdeskifte](packed-r1-after-handoff-ranch.png), [bil ved stengt port](packed-r1-car-port-blocked.png), [postkassen](packed-r1-mailbox.png), [tilbake på landeveien](packed-r1-returned-highway.png). Varselskriften om RT-budsjett er bevart; bildene gir ingen FPS-aksept. Ingen installasjon/release.
