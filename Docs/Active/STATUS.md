# Gjeldende status for SIGNAL / 47

**Status:** Gjeldende. **Gjelder:** Unreal-utgaven, UE 5.8.4 på Linux/Vulkan. **Sist kontrollert:** 29. oktober 2026.

Dette er den samlede statusinngangen. [TODO](TODO.md) fører åpne oppgaver; [DECISIONS](DECISIONS.md) fører beslutningene som styrer dem. Leveringsrapporter og testbevis dokumenterer et bestemt bygg, ikke automatisk dagens installasjon eller alle senere feil.

## Fire forskjellige leveringsnivåer

| Nivå | Bekreftet utgangspunkt | Hva det betyr |
| --- | --- | --- |
| Kildekode på `main` | Ny nattsekvens gjennom [PR209](https://github.com/Tombonator3000/SIGNAL-47/pull/209), testet kilde `b68d3a08`, med main/PR206, PR208 og PR210 integrert. Tidligere SC02/aliennavigasjon, motel R4, fotolab, trafo/lysbue og grafikkmeny beholdes. | Kildekode, modeller og planer på main er ikke i seg selv en ny spillpakke. |
| Akseptert og testet kandidat | Grafikk-R2 fra [PR197](https://github.com/Tombonator3000/SIGNAL-47/pull/197), feature-head `2bb2640fd60dff8975d917ee875db9995e680fd4`. Ny [nattkandidat R2](../Evidence/UnrealDeliveries/NATTSEKVENS_2026-10-28.md) fra PR209 har egen avgrenset aksept. | Grafikk-R2 består én full 111/111-matrise. Natt-R2 består målrettet natt 74/8 og Arc/calm 14, med separate himmel-/Continue-kontroller. Natt-R2 har ingen fullmatrise eller installasjon. |
| Lokal installasjon | Den identiske grafikk-R2-pakken i `/home/tom/Games/Signal47/Linux`, bekreftet gjennom installasjonskvittering og etterkontroll. | Ny grafikkmeny og tidligere leverte lokale forbedringer finnes her. Personlige lagringer er bevart. |
| Offentlig nedlasting | [Linux-testutgave 28. oktober](https://github.com/Tombonator3000/SIGNAL-47/releases/tag/playtest-linux-2026-10-28), publisert som prerelease kl. 08:44 Europe/Oslo. | Denne nedlastingen er fotolab-R1 fra PR181, med SC01. Den er eldre enn lokal grafikk-R2 og inneholder ikke den senere SC02-/trafo-/grafikkleveransen. |

Git-fletting endrer ikke installasjonen eller nedlastingen. Nedlastingssiden har historisk formulering om å være «samme som den nyeste installerte»; den formuleringen gjelder publiseringsøyeblikket, ikke den senere R2-installasjonen. Det finnes ingen stabil `latest`-release som kan brukes i stedet for den navngitte prerelease-lenken.

## Hva som er levert, og hva som fortsatt mangler

| Område | Gjeldende status | Kilde eller neste steg |
| --- | --- | --- |
| Unreal som spillutgave | Gjeldende utviklingsløype. Web og Unity er historikk/referanse. | [Arbeidsregler](../../unreal/AGENTS.md). |
| Grafikkmeny | Flettet, pakket og lokalt installert. Fem presets, Custom og elleve individuelle kontroller. | [Grafikkrapport](Guides/GRAPHICS_SETTINGS.md). |
| SC01 | Spillbar og lokalt installert; finnes også i den offentlige fotolab-R1-pakken. | Klask/avtrykk og enklere lampetest er kildeflettet i PR208; den installerte og offentlige pakken har den tidligere SC01-versjonen. |
| SC02 og aliennavigasjon | Flettet via [PR190](https://github.com/Tombonator3000/SIGNAL-47/pull/190), pakket kontrollert og videreført i den lokale R2-kilden/pakken. | [SC02-overlevering](../Evidence/UnrealDeliveries/SC02_AI_HANDOFF.md). Den nye nattsekvensen har egen avgrenset rapport nedenfor. |
| Arrayarbeid i den rolige kvelden | Ny kilde er flettet via [PR206](https://github.com/Tombonator3000/SIGNAL-47/pull/206): fire observasjoner 19:00 til 22:50 med papirer og hint. | Kildeendring, ikke en ny installert/offentlig pakke. Separat leveringskvittering kreves før pakkestatus endres. |
| Dales stemme ved besøksinngangen | Implementert og målrettet editor/game-kontrollert med lagret hendelse og opprydding ved sak/talebryter. | [Kilde, romlig lydmåling og kontrollgrenser](../Evidence/UnrealDeliveries/DOOR_VOICE_2026-10-29.md). Ingen ny pakke, installasjon eller offentlig utgave. |
| Vulkan-minnetiltak på Linux | Kilde i PR215 og målrettet kandidat R1, med separat High/natt-R2. Første historiske startup-OOM er fortsatt uavklart. | [Kildebinding, målinger og begrensninger](../Evidence/UnrealDeliveries/DINER_VULKAN_2026-10-29.md). Ingen fullmatrise eller installasjon; R1 er før PR216/TB2. |
| Trafoens feltutstyr og bakke, FIX-07 | Kilde/native kontrollert: fire modeller, 84 lagrede LOD-kontroller og ny T01 med 1709 selvtester. Claudes T02/SC02 og før-/etterbilder er bevart. | [Kvittering og begrensninger](../Evidence/UnrealDeliveries/SUBSTATION_FIELD_2026-10-29.md). Pakket spill og installasjon føres separat. |
| Ny nattsekvens og SKY01 | Implementert gjennom PR209 og målrettet pakket i nattkandidat R2. Strømbrudd 00:13, trafotur 01:35, T01-strømrestore, SC02 ca. 02:00 og SC03-grense ca. 02:15. | [Leveringsrapport og bevisgrenser](../Evidence/UnrealDeliveries/NATTSEKVENS_2026-10-28.md). Ingen installasjon eller offentlig release. |
| SC03 og resten av historien | Produksjon pågår. Ingen bekreftet ferdig aktleveranse eller lokal installasjon av SC03 til SC13 i dette kontrollgrunnlaget. | [Kveldsplan](../Design/Story/KVELD_PLAN_2026-10-28.md), [revidert historie](../Design/Story/HISTORIE_SC02_SC13_REVISJON.md). |
| Motel, fotolab, trafo og lysbue | Tidligere leveranser er flettet og lokalt installert. Nye spillerfunn ved motellet og trafoen er fortsatt åpne. | [Motel](../Evidence/UnrealDeliveries/MOTEL_V2.md), [fotolab](../Evidence/UnrealDeliveries/PHOTOLAB_DETAIL.md), [trafo](../Evidence/UnrealDeliveries/SUBSTATION_DETAIL.md), [lysbue](../Evidence/UnrealDeliveries/FX_ARC_2026-10-28.md). |
| MetaHuman-servitrise | Én fungerende figur er lokalt installert, med uniform, hårkort og taleanimasjon. | [Figurpilot](../Evidence/UnrealDeliveries/METAHUMAN_WAITRESS.md). Videre visuell polering gjenstår. |
| Nye MetaHuman-mannekenger | Produksjonsretning og tre eksisterende ankere er kartlagt. Ingen nye mannekenger er bakt, importert eller installert. | [Undersøkelse og plan](../Research/Unreal/UNREAL_SKILLS_2026-10-28.md). Mannlig grunnvariant mangler lokalt. |
| Caliche TB2 | Blender-kilder og native import er flettet via PR179/PR216:20 meshes,78 nye materialer og21delte teksturer;15 editorbilder dag/skumring/natt kontrollert. Pakket spill og menneskelig gange gjenstår. | [Native bygningsløype](../../unreal/Signal47/Tools/townintegration/README.md). Installerte bybygninger er fortsatt TB1; vanlige bolighus er et eget åpent punkt. |
| Nanite | Allerede i bruk på fire tette stein-/fjellmodeller langs ruten. Ingen generell konvertering av byen eller vegetasjonen er utført. | [Kandidatvurdering](../Research/Unreal/UNREAL_SKILLS_2026-10-28.md). |
| Ny UFO-finale og meteorpassasje | Regi, konsepter og referanser finnes. Effektene er ikke produsert i dette kontrollgrunnlaget. | [UFO](../Design/Proposals/FINALE_CLOUD_TANK_UFO.md), [meteor](../Design/Proposals/NIAGARA_VFX_2026-10-28.md). |
| Windows og andre maskiner | Byggegrunnlag/veiledning finnes. Ferdig verifisert Win64-leveranse og kompatibilitet på mottakermaskiner er ikke bekreftet. | Føres som åpne oppgaver i [TODO](TODO.md). |

## Pågående arbeid og kjente feil

Codex sin nattsekvens er målrettet levert gjennom PR209. Claudes videre kvelds-/SC03-arbeid har egen arbeidskopi. Den lokale arbeidsdelingen føres i [OPPGAVER](../../unreal/OPPGAVER.md). En arbeidsgren, en kjørende test eller en manusendring er ikke en ferdig leveranse. Neste statusoppdatering må vise nøyaktig testet kandidat, merge og eventuell installasjon hver for seg.

[Toms feilrapport med seks originalbilder og to krasjvedlegg](Reports/PLAYTEST_TODO_2026-10-28.md) står åpen: diner-krasj, motellnivå/kollisjon/TV/basseng, SARO-dialog/dørhint, trafodeler, Caliche og trafikksignal. Gjenfunne komplette krasjlogger bekrefter Vulkan-OOM ved henholdsvis 128 og 256 MiB. Riktige binærer/debugfiler og den komplette andre pakken er sikret. Isolerte Ultra-prøver gjenskapte kritisk minnepress uten andre UE-jobber. Et enkelt VSM-reservetiltak fullførte deretter hele diner/2628 m-returruten med minst1767MiB globalt ledig; pakket Linux-R1 fra b70bf042 består 1709selvtester og full diner/2628 m-retur uten diagnoseoverstyring, minst1740MiBledig. Natt-R2 har 3159rå meldinger etter Continue uten synlig-instansoverløp. [Prøver og avgrensning](../Evidence/UnrealDeliveries/DINER_VULKAN_2026-10-29.md). Første historiske startup-OOM og samtidig editor er fortsatt uavklart. Tidligere beståtte matriser lukker ikke disse nye funnene.

## Kontrollgrunnlag og begrensninger

Produksjonsverktøy, 29. oktober: [produksjonskoordinator med Claude](Guides/PRODUCTION_SYSTEM.md)
samler varig oppgavekø, kildeoppslag, verktøykjøring og kontrollkvitteringer.
Leveransen gjelder utviklingsverktøy. Faktisk Claude-kjøring er avgrenset av
kontoens kvote; syntetiske tester og lokal kontroll er ført i veiledningen.
Dette endrer ingen av spillpakkenes leveringsnivåer ovenfor.

R2-installasjonen er dokumentert i den lokale bevismappen `/home/tom/scratch-bm/Signal47-graphics-settings-20261028`: `acceptance-r1.json`, `installation-r1.json`, `installed-verification-r1.json` og `post-prune-verification.json`. Installert skrive-/lesekontroll består 47 + 15; alle 79 personlige Saved-filer (105 noder) er uendret også etter opprydding. [Rapporten i repoet](Guides/GRAPHICS_SETTINGS.md) beskriver omfang og begrensninger.

Dokumentryddingens kontroll omfattet kvitteringer og kilder. De senere målrettede natt- og krasjprøvene har egne avgrensninger ovenfor; ingen senere installasjon eller offentlig release er gjort. Menneskelig full gjennomspilling, fysisk gamepad, subjektiv lytting og ytelse på andre PC-er skal ikke regnes som bestått uten egne faktiske bevis.
