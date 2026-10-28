# Arbeidsdeling og koordinering

**Status: Gjeldende** · Denne filen fører eierskap, filansvar og tunge kjørevinduer. Samlet [status](../Docs/Active/STATUS.md), [TODO](../Docs/Active/TODO.md) og [beslutninger](../Docs/Active/DECISIONS.md) oppdateres i Docs/Active.

| Arbeid | Eier / arbeidskopi ved ryddestart | Avgrensning |
| --- | --- | --- |
| SC01 og SC03 | Claude, egen SC03-arbeidskopi | Arrayarbeidet er kildeflettet via PR206. Videre SC01 klask/avtrykk/lampetest og SC03-regi/timing; koordiner konkrete kildefiler før fletting. |
| Ny nattsekvens | Codex, `codex/night-sequence-20261028` i `motel-v2-unreal` | Story/Evening, signal/strøm/trafo, SkyVisitors og SC02-rekkefølge. Pågående arbeidskilder er ikke endret av dokumentryddingen. |
| Dokumentrydding | Codex, `codex/docs-organization-20261028` i `docs-organization` | Docs, dokumentinnganger, lenker og arkivmetadata. Ingen motorjobb eller installasjon. |

Én tung Blender-/Unreal-/pakkejobb om gangen. Reservasjonen gjelder hele den avtalte serien, også opphold mellom prosesser. Ikke stopp andres prosesser eller start tester samtidig med Toms spilløkt. Meld aktiv kandidat og når vinduet er frigitt her, uten å kopiere hele leveringsrapporten.

Når en oppgave endrer eier eller filer, oppdater den relevante raden. Når den leveres, oppdater STATUS/TODO og lenk kvitteringen. Ny historieplan og kveldsfordeling står i [DECISIONS](../Docs/Active/DECISIONS.md).

[Tidligere arbeidsdelings-/leveringshistorikk er bevart](../Docs/Archive/Superseded/Unreal/OPPGAVER-before-docs-organization.md). Lokale ucommittede koordineringer i andre arbeidskopier må samordnes før de slettes eller overskrives.

## Midlertidig hjelp til Claude, 29. oktober 2026 kl. 00:46

Tom ba Codex hjelpe Claude i `claude/evening-door-voice`, arbeidskopi `sc03`,
frem til Claudes automatiske fortsettelse kl. 01:11. Codex eier midlertidig
den ucommittede dørstemmeendringen i Observing/HorrorTest og én liten
OnCharacters-krok i Flow. Test 11 var allerede PASS før kvotestoppet.
Codex retter Stop/Begin og talebryter og kjører én målrettet SC01-kontroll
under felles reservasjon. Vinduet frigis før 01:11; videre eierskap går til
samme Claude-økt. Ingen installasjon eller offentlig utgave i dette passet.
