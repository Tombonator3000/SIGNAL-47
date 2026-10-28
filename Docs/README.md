# SIGNAL / 47: start her

**Status: Gjeldende** · Gjelder Unreal-utgaven. Sist kontrollert 28. oktober 2026.

| Det du vil vite | Åpne |
| --- | --- |
| Hva er faktisk på main, testet, installert og nedlastbart? | **[STATUS](Active/STATUS.md)** |
| Hva skal gjøres nå? | **[TODO](Active/TODO.md)** |
| Hvilke beslutninger og planer gjelder? | **[DECISIONS](Active/DECISIONS.md)** |
| Hvordan er historien og den visuelle retningen? | [Design](Design/README.md) |
| Hvilke undersøkelser og ferdigheter kan brukes senere? | [Research](Research/README.md) |
| Hvor er bilder, kontrollresultater og leveringsrapporter? | [Evidence](Evidence/README.md) |
| Hvor ligger tidligere Unity-/web-arbeid og erstattede notater? | [Archive](Archive/README.md) |
| Hvor finner jeg et bestemt gammelt dokument? | [Dokumentkatalog med status og ny plassering](CATALOGUE.md) |
| Hvordan bygges Unreal, og hvem eier arbeidet? | [Unreal-inngang](../unreal/README.md) og [arbeidsdeling](../unreal/OPPGAVER.md) |
| Hvordan kjøres produksjonsoppgaver med Claude og testbevis? | [Produksjonskoordinator](../Automation/production/README.md) |

## Hva betyr statusen?

- **Gjeldende:** styrer den delen dokumentet gjelder. Aktuell leveringsstatus finnes bare i STATUS.
- **Under arbeid:** åpen produksjon eller spillerfunn, ikke ferdig spillinnhold.
- **Referanse:** design, undersøkelse eller bevis for et navngitt bygg; overstyrer ikke nyere beslutninger.
- **Erstattet:** historikk med peker til den nye inngangen. Gamle «current»-formuleringer beskriver sin opprinnelige dato.
- **Uavklart:** eldre restpunkt som må undersøkes før det regnes som en feil eller ny oppgave i dagens spill.

Et konseptbilde er ikke en modell. En kildecommit er ikke en installasjon. Et bestått eldre bygg lukker ikke nye spillerfunn.

## Bevarte kildepakker og kompatibilitet

[Designbibelen fra 27. oktober](DesignBible-2026-10-27/README.md) og [den samme dagens kilde-/bevispakke](LocalDeliveries-2026-10-27/README.md) beholder originalstiene fordi manifest, kontrollsummer og byggeoppskrifter viser til dem. De er referanser, ikke parallelle statusinnganger. PDF, originalbilder og historiske kvitteringer er bevart uten endring.

`DesignBible13`, `WorldCase22` og `VisualTargets` beholder også originalplasseringen. De inngår i frosne kildepakker, offentlige bildelenker og byggeoppskrifter som forutsetter denne mappestrukturen. Arkiv-/designinngangene klassifiserer dem tydelig uten å skrive om originalene. De er ikke parallelle statuskilder. `Menu14` har bare en kort kompatibilitetspeker for en uendret kildehenvisning. [THIRD_PARTY_NOTICES](THIRD_PARTY_NOTICES.md) beholder fulltekst og plassering fordi eldre byggeoppskrifter kopierer filen.

## Hold oversikten ryddig

Oppdater de tre hovedfilene i Active. `unreal/OPPGAVER.md` fører eierskap og koordinering; `unreal/log.md` fører datert historikk. Nye rapporter får riktig kategori og status. Ikke opprett en ny datert «gjeldende status»-fil etter hver chat. Se [dokumentreglene](AGENTS.md).
