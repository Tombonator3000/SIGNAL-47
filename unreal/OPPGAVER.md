# Arbeidsdeling og koordinering

**Status: Gjeldende** · Denne filen fører eierskap, filansvar og tunge kjørevinduer. Samlet [status](../Docs/Active/STATUS.md), [TODO](../Docs/Active/TODO.md) og [beslutninger](../Docs/Active/DECISIONS.md) oppdateres i Docs/Active.

| Arbeid | Eier / arbeidskopi ved ryddestart | Avgrensning |
| --- | --- | --- |
| SC01 og SC03 | Claude, egen SC03-arbeidskopi | Arrayarbeid, SC01 klask/lampetest, lyder og aggregat/port er kildeflettet via PR206, PR208 og PR210. Videre stemme ved døren og SC03-regi/timing; koordiner konkrete kildefiler før fletting. |
| Ny nattsekvens | Codex, `codex/night-sequence-20261028` i `motel-v2-unreal` | PR209, testet kilde b68d3a08 og kandidat R2. Natt 74/8, Arc/calm 14, SKY01 24 og fersk Continue High/Low-calm er kontrollert. Tunge prøver er ferdige og vinduet er frigitt. Lokal pakkeopprydding følger den etablerte regelen. Ingen installasjon/release. Felles hjerteslag i lokal HANDOFF. |
| Dokumentrydding | Codex, `codex/docs-organization-20261028` i `docs-organization` | Docs, dokumentinnganger, lenker og arkivmetadata. Ingen motorjobb eller installasjon. |

Én tung Blender-/Unreal-/pakkejobb om gangen. Reservasjonen gjelder hele den avtalte serien, også opphold mellom prosesser. Ikke stopp andres prosesser eller start tester samtidig med Toms spilløkt. Meld aktiv kandidat og når vinduet er frigitt her, uten å kopiere hele leveringsrapporten.

Når en oppgave endrer eier eller filer, oppdater den relevante raden. Når den leveres, oppdater STATUS/TODO og lenk kvitteringen. Ny historieplan og kveldsfordeling står i [DECISIONS](../Docs/Active/DECISIONS.md).

[Tidligere arbeidsdelings-/leveringshistorikk er bevart](../Docs/Archive/Superseded/Unreal/OPPGAVER-before-docs-organization.md). Lokale ucommittede koordineringer i andre arbeidskopier må samordnes før de slettes eller overskrives.
## Produksjonskoordinator med Claude, 29. oktober 2026

Tom ba om systemet skissert i Signal 47-samtalen. Codex eier
`Automation/production`, inngangen `Scripts/production.py`, den avgrensede
guard-hooken og produksjonsdokumentasjonen på `codex/production-coordinator-20261028`
i `~/scratch-bm/wt/production-coordinator`. En egen Claude Code-økt eier
Claude-adapteren og dens tester; to avgrensede hjelpere har kildeindeks og
reservasjon/prosesskontroll. Eksisterende kvelds-/SC03-arbeid har egne eiere.
Dette passet kjører raske Python-/prosesstester og en avgrenset Claude-probe.
Ingen tung motorjobb eller reservasjon av Toms spillmaskin er planlagt.

## Midlertidig hjelp til Claude, 29. oktober 2026 kl. 00:46

Tom ba Codex hjelpe Claude i `claude/evening-door-voice`, arbeidskopi `sc03`,
frem til Claudes automatiske fortsettelse kl. 01:11. Codex eier midlertidig
den opprinnelige dørstemmeendringen i Observing/HorrorTest og én liten
OnCharacters-krok i Flow. Test 11 var allerede PASS før kvotestoppet.
Codex retter Stop/Begin og talebryter og kjører én målrettet SC01-kontroll
under felles reservasjon. Vinduet frigis før 01:11; videre eierskap går til
samme Claude-økt. Ingen installasjon eller offentlig utgave i dette passet.

2026-10-29T01:06:26+01:00: Midlertidig dørstemmehjelp er kildeklar og kontrollert. Bygg/test er ferdig, reservasjonen er frigitt og Codex starter ingen nye motorjobber før overleveringen01:11. Claude beholder videre kvelds-/SC03-eierskap. Testet kilde 7a29935e796d6b036692a53d9f2b5712993ccf17; rapporten er lenket fra STATUS/TODO.
