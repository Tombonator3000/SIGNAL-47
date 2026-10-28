# Gjeldende status for SIGNAL / 47

**Status:** Gjeldende. **Gjelder:** Unreal-utgaven, UE 5.8.4 på Linux/Vulkan. **Sist kontrollert:** 28. oktober 2026.

Dette er den samlede statusinngangen. [TODO](TODO.md) fører åpne oppgaver; [DECISIONS](DECISIONS.md) fører beslutningene som styrer dem. Leveringsrapporter og testbevis dokumenterer et bestemt bygg, ikke automatisk dagens installasjon eller alle senere feil.

## Fire forskjellige leveringsnivåer

| Nivå | Bekreftet utgangspunkt | Hva det betyr |
| --- | --- | --- |
| Kildekode på `main` | Kontrollgrunnlag ved ryddestart: `469335b1ceee680e338829692d95e0027fcb7e68`; samordnet med main `38717ff502a83cdde5a7066f53339ff87179b465` før levering. SC02/aliennavigasjon, motel R4, fotolab, trafo/lysbue og grafikkmeny er flettet. | Kildekode, modeller og planer på main er ikke i seg selv en ny spillpakke. |
| Akseptert og testet kandidat | Grafikk-R2 fra [PR197](https://github.com/Tombonator3000/SIGNAL-47/pull/197), feature-head `2bb2640fd60dff8975d917ee875db9995e680fd4`. | Samme frosne pakke består målrettet meny-/lagringskontroll og én full 111/111-matrise. |
| Lokal installasjon | Den identiske grafikk-R2-pakken i `/home/tom/Games/Signal47/Linux`, bekreftet gjennom installasjonskvittering og etterkontroll. | Ny grafikkmeny og tidligere leverte lokale forbedringer finnes her. Personlige lagringer er bevart. |
| Offentlig nedlasting | [Linux-testutgave 28. oktober](https://github.com/Tombonator3000/SIGNAL-47/releases/tag/playtest-linux-2026-10-28), publisert som prerelease kl. 08:44 Europe/Oslo. | Denne nedlastingen er fotolab-R1 fra PR181, med SC01. Den er eldre enn lokal grafikk-R2 og inneholder ikke den senere SC02-/trafo-/grafikkleveransen. |

Git-fletting endrer ikke installasjonen eller nedlastingen. Nedlastingssiden har historisk formulering om å være «samme som den nyeste installerte»; den formuleringen gjelder publiseringsøyeblikket, ikke den senere R2-installasjonen. Det finnes ingen stabil `latest`-release som kan brukes i stedet for den navngitte prerelease-lenken.

## Hva som er levert, og hva som fortsatt mangler

| Område | Gjeldende status | Kilde eller neste steg |
| --- | --- | --- |
| Unreal som spillutgave | Gjeldende utviklingsløype. Web og Unity er historikk/referanse. | [Arbeidsregler](../../unreal/AGENTS.md). |
| Grafikkmeny | Flettet, pakket og lokalt installert. Fem presets, Custom og elleve individuelle kontroller. | [Grafikkrapport](Guides/GRAPHICS_SETTINGS.md). |
| SC01 | Spillbar og lokalt installert; finnes også i den offentlige fotolab-R1-pakken. | Nye ønsker om klask/avtrykk og enklere lampetest er et eget pågående pass. |
| SC02 og aliennavigasjon | Flettet via [PR190](https://github.com/Tombonator3000/SIGNAL-47/pull/190), pakket kontrollert og videreført i den lokale R2-kilden/pakken. | [SC02-overlevering](../Evidence/UnrealDeliveries/SC02_AI_HANDOFF.md). Ny kveldsrekkefølge er fortsatt eget arbeid. |
| Arrayarbeid i den rolige kvelden | Ny kilde er flettet via [PR206](https://github.com/Tombonator3000/SIGNAL-47/pull/206): fire observasjoner 19:00 til 22:50 med papirer og hint. | Kildeendring, ikke en ny installert/offentlig pakke. Separat leveringskvittering kreves før pakkestatus endres. |
| SC03 og resten av historien | Produksjon pågår. Ingen bekreftet ferdig aktleveranse eller lokal installasjon av SC03 til SC13 i dette kontrollgrunnlaget. | [Kveldsplan](../Design/Story/KVELD_PLAN_2026-10-28.md), [revidert historie](../Design/Story/HISTORIE_SC02_SC13_REVISJON.md). |
| Motel, fotolab, trafo og lysbue | Tidligere leveranser er flettet og lokalt installert. Nye spillerfunn ved motellet og trafoen er fortsatt åpne. | [Motel](../Evidence/UnrealDeliveries/MOTEL_V2.md), [fotolab](../Evidence/UnrealDeliveries/PHOTOLAB_DETAIL.md), [trafo](../Evidence/UnrealDeliveries/SUBSTATION_DETAIL.md), [lysbue](../Evidence/UnrealDeliveries/FX_ARC_2026-10-28.md). |
| MetaHuman-servitrise | Én fungerende figur er lokalt installert, med uniform, hårkort og taleanimasjon. | [Figurpilot](../Evidence/UnrealDeliveries/METAHUMAN_WAITRESS.md). Videre visuell polering gjenstår. |
| Nye MetaHuman-mannekenger | Produksjonsretning og tre eksisterende ankere er kartlagt. Ingen nye mannekenger er bakt, importert eller installert. | [Undersøkelse og plan](../Research/Unreal/UNREAL_SKILLS_2026-10-28.md). Mannlig grunnvariant mangler lokalt. |
| Caliche TB2 | Nye Blender-kilder er flettet via PR179. Native import, materialer, assembly og levering i spillet mangler. Installerte bybygninger er fortsatt TB1. | [Bygningskilder](../../unreal/Signal47/Tools/townbuildings/README.md). Vanlige bolighus er et eget åpent punkt. |
| Nanite | Allerede i bruk på fire tette stein-/fjellmodeller langs ruten. Ingen generell konvertering av byen eller vegetasjonen er utført. | [Kandidatvurdering](../Research/Unreal/UNREAL_SKILLS_2026-10-28.md). |
| Ny UFO-finale og meteorpassasje | Regi, konsepter og referanser finnes. Effektene er ikke produsert i dette kontrollgrunnlaget. | [UFO](../Design/Proposals/FINALE_CLOUD_TANK_UFO.md), [meteor](../Design/Proposals/NIAGARA_VFX_2026-10-28.md). |
| Windows og andre maskiner | Byggegrunnlag/veiledning finnes. Ferdig verifisert Win64-leveranse og kompatibilitet på mottakermaskiner er ikke bekreftet. | Føres som åpne oppgaver i [TODO](TODO.md). |

## Pågående arbeid og kjente feil

Nattsekvensen og SC03 arbeides med i separate arbeidskopier. Den lokale arbeidsdelingen føres i [OPPGAVER](../../unreal/OPPGAVER.md). En arbeidsgren, en kjørende test eller en manusendring er ikke en ferdig leveranse. Neste statusoppdatering må vise nøyaktig testet kandidat, merge og eventuell installasjon hver for seg.

[Toms feilrapport med seks originalbilder og to krasjvedlegg](Reports/PLAYTEST_TODO_2026-10-28.md) står åpen: diner-krasj, motellnivå/kollisjon/TV/basseng, SARO-dialog/dørhint, trafodeler, Caliche og trafikksignal. Begge krasjutdrag peker mot Vulkan-bufferallokering. Vedleggene alene beviser ikke samtidig editor eller endelig rotårsak. Tidligere beståtte matriser lukker ikke disse nye funnene.

## Kontrollgrunnlag og begrensninger

R2-installasjonen er dokumentert i den lokale bevismappen `/home/tom/scratch-bm/Signal47-graphics-settings-20261028`: `acceptance-r1.json`, `installation-r1.json`, `installed-verification-r1.json` og `post-prune-verification.json`. Installert skrive-/lesekontroll består 47 + 15; alle 79 personlige Saved-filer (105 noder) er uendret også etter opprydding. [Rapporten i repoet](Guides/GRAPHICS_SETTINGS.md) beskriver omfang og begrensninger.

Denne dokumentryddingen leser kvitteringer og kilder. Den starter ingen ny motorjobb, spilltest, installasjon eller offentlig release. Menneskelig full gjennomspilling, fysisk gamepad, subjektiv lytting og ytelse på andre PC-er skal ikke regnes som bestått uten egne faktiske bevis.
