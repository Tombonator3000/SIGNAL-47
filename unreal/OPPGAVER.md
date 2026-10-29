# Arbeidsdeling og koordinering

**Status: Gjeldende** · Denne filen fører eierskap, filansvar og tunge kjørevinduer. Samlet [status](../Docs/Active/STATUS.md), [TODO](../Docs/Active/TODO.md) og [beslutninger](../Docs/Active/DECISIONS.md) oppdateres i Docs/Active.

| Arbeid | Eier / arbeidskopi ved ryddestart | Avgrensning |
| --- | --- | --- |
| SC01 og SC03 | Claude, egen SC03-arbeidskopi | Arrayarbeid, SC01 klask/lampetest, lyder og aggregat/port er kildeflettet via PR206, PR208 og PR210. Videre stemme ved døren og SC03-regi/timing; koordiner konkrete kildefiler før fletting. |
| SC01-dispatch, STORY-04 | Codex, `codex/dispatch-report-20261029` i `sc03` | Kandidat R1 fra c5d05fa2, main/motell d771cf9e samordnet: to nye takes og full pakket SC01 220/21/273,16 m PASS, faktisk naturlig telefonavspilling/skip/mute og disk/full Flow. [Rapport](../Docs/Evidence/UnrealDeliveries/DISPATCH_RECORDINGS_2026-10-29.md). [PR221](https://github.com/Tombonator3000/SIGNAL-47/pull/221) er faktisk flettet i main `5dcaa9ba`; tungvinduet er frigitt. DispatchComplete er testminne; Dale-heard var true før dispatch og bevares, egen uhørt case er ikke prøvd. Ingen flere cues/casting; ingen installasjon/release. |
| Skrivebordsark ART-01, overtakelse | Codex, `codex/document-props-takeover-20261029` i `dale-document-assets` | Kandidat R1 fra `343e594d` PASS: 1720 selvtester, fysisk seks-papir-reise/580 kontroller/4,545 m/47 Slate-tastetrykk/fem diskforløp/elleve art-auditer, og tolv komplette gjennomsette dag/skumring/nattbilder/72 materialauditer. Normal exit0 uten konkurranse. [Rapport og pakket kvittering](../Docs/Evidence/UnrealDeliveries/DOCUMENT_PROPS_2026-10-29.md). Claudes tolv originalfiler og 34 råbevis er sikret; sc03 er urørt. Motorvindu frigitt; [PR226](https://github.com/Tombonator3000/SIGNAL-47/pull/226) leverer kilden. Gammel Motel R1-payload pensjonert med fersk hash-/Saved-kontroll. Øvrig ART-01 står åpent; ingen installasjon/release. |
| Radio-kvinne HUM-01 | Codex, `codex/radio-woman-20261029` i `evening` | [Utkast PR224](https://github.com/Tombonator3000/SIGNAL-47/pull/224). Restklær R1 og privat import9/deltMI er kontrollert. R5 måler 635 vektede Face-rammer, faktisk CopyPose-CDO og åtte små importerte kles-bindavvik; streng probe er fortsatt avvist. Face CopyPose-retting er fryst i `a8badcb3`; native privat klær-rebind med uendret geometri og fersk leser forfattes nå. Ingen ny statisk figur er akseptert. Native bake, benk/sko/hender/radio, nattregel, fysisk samtale/full disk-Continue og pakke gjenstår. Root eier neste motorvindu etter ART01. MetaHuman-kilder/avledede assets er private; ingen installasjon/release. |
| Ny nattsekvens | Codex, `codex/night-sequence-20261028` i `motel-v2-unreal` | PR209, testet kilde b68d3a08 og kandidat R2. Natt 74/8, Arc/calm 14, SKY01 24 og fersk Continue High/Low-calm er kontrollert. Tunge prøver er ferdige og vinduet er frigitt. Begge NATT-payloadene er pensjonert etter regelen; råbevis er bevart. Ingen installasjon/release. Felles hjerteslag i lokal HANDOFF. |
| FIX-01 diner-krasj | Codex, PR215, `motel-v2-unreal` | Linux-minnetiltak målrettet pakket R1 fra b70bf042: 1709 selvtester/full diner 2628 m/restore PASS, faktisk SystemSettingsIni 0,25. Natt-R2:3159 rå meldinger etter Continue uten bit8. Første historiske startup-OOM/samtidighet forblir åpen. Prøvevindu frigitt; ingen installasjon/release. |
| Motelpass FIX-02 til FIX-05 | Codex, `codex/motel-grounded-20261029` i `evening` | Kandidat R1 fra 2d13e231 består 1709 selvtester og pakket 58/14/7/4, 1611 gulvstråler, 178,70 m, TV-GPU og ordinær disk/Continue. 13 458 fryste inputs/åtte payloads er hashbundet; main/ENV02 er samordnet. Editor R11 og R6b/native32/g456-trim har separate kvitteringer. [Rapport og pakket bevis](../Docs/Evidence/UnrealDeliveries/MOTEL_GROUNDING_2026-10-29.md) er oppdatert; [PR220](https://github.com/Tombonator3000/SIGNAL-47/pull/220) er faktisk flettet i main `d771cf9e`. Bassengklumpen fra E04 er fortsatt ikke identifisert. Menneskelig input/lytting/FPS/fullmatrise er UNVERIFIED; ingen installasjon/release. |
| Trafo FIX-07, overtakelse | Codex etter Claude, `codex/fix07-handoff-20261029` i `sc03` | Overtatt kl. 04:28 etter 75-minuttersregelen og levert via [PR218](https://github.com/Tombonator3000/SIGNAL-47/pull/218), flettet i main31abf280. Cook-inkludering, 84 ferske LOD-kontroller og ny T01/1709 selvtester er kontrollert. T02/SC02 fra Claude beholdes med konkrete bevisgrenser. Ingen installasjon/release. |
| Caliche-boliger og innkjøringsskilt ENV-02 | Codex, `codex/caliche-homes-20261029` i `motel-v2-unreal` | Kandidat R2 fra `1665723f` er målrettet akseptert: 1709 selvtester, pakket API-rute 634 og Off-fallback 481; 13 459 fryste inputs/åtte payloads. [Leveringsrapport](../Docs/Evidence/UnrealDeliveries/CALICHE_HOMES_2026-10-29.md). [PR219](https://github.com/Tombonator3000/SIGNAL-47/pull/219) er bekreftet flettet i main232a8738 med eksakt headc2e359bb; dokument-/lenkekontroll PASS. Root eier videre tunge vinduer. Førerhustekstens subjektive lesbarhet og ekte input er UNVERIFIED; ingen fullmatrise/installasjon/release. |
| Gård og adkomst ENV-04 | Codex, `codex/farm-access-20261029` i `motel-v2-unreal` | [PR223](https://github.com/Tombonator3000/SIGNAL-47/pull/223), main/meteor samordnet i c301ead9. Pakket R1 PASS: 1720 selvtester, rute27/380,33 m bil/30,97 m gange, begge barrierer, ti geometri-/material-/RT-audits og disk/title/Continue. Alle sju bilder åpnet. Faktisk flettet i main `094a67cb`; sju gamle husR2-payloadfiler er pensjonert, øvrige registrerte payloadhash og alle 91 ferske personlige Saved-filer uendret. [Rapport](../Docs/Evidence/UnrealDeliveries/FARM_ACCESS_2026-10-29.md). Ingen installasjon/release. |
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

## Lokal installasjon etter Toms bestilling, 2026-10-29T09:05:01+01:00

Codex i denne samtalen eier installasjon til ~/Games/Signal47/Linux, med farmR1 som frosset kandidat. Filansvar er installasjonskvittering, STATUS og dette koordineringsfeltet; HUM01/ART01-kilder berøres ikke. Én samlet fullmatrise og pakket meteorLow/High går under felles reservasjon gjennom installasjon/etterkontroll. Ferdig: Farm R1 er installert med 111/111, meteor Low/High og installert grafikk/Fortsett PASS. [Kvittering](../Docs/Evidence/UnrealDeliveries/INSTALL_LAUNCHER_2026-10-29.md). Vinduet er faktisk frigitt kl. 10:20:44 Europe/Oslo; HUM01/ART01 kan fortsette under egen guard. Saved og launchere er uendret; ingen offentlig release.

## Komplett testnedlasting, 2026-10-29 kl. 11:13 CET

Codex i production-coordinator eier starter og distribusjonsdocs på codex/download-playtest-20261029. [Farm R1-nedlastingen](../Docs/Evidence/UnrealDeliveries/DOWNLOAD_LINUX_2026-10-29.md) er publisert og vedlegg/anonym tilgang verifisert. Ingen motorreservasjon, ny pakking/cook, endring av standardinstallasjonen eller arbeid i ART01/HUM01 inngår. Alle lokale kilde-, Saved- og rollbackfiler er bevart.
