# SC01: to fornyede dispatch-opptak

**Status:** Referanse, levert kandidat R1; [PR221](https://github.com/Tombonator3000/SIGNAL-47/pull/221) mot main. **Gjelder:** STORY-04, de to nye `SC01_FIRST_REPORT_001/002`-opptakene. **Sist kontrollert:** 29. oktober 2026.

To nye syntetiske opptak følger det gjeldende SC01-manuset om slaget mot kontrollromsvinduet. Lokal produksjon, RAW-import, teknisk lydkontroll, fersk ASR og hele den pakkede SC01-ruten består. Kandidat R1 har 220 kontroller, 21 disk/Flow-restores og 273,156790 m normal Walker-gange; menneskelig lytting er fortsatt UNVERIFIED. [Maskinlesbar produksjonskvittering](../../../unreal/Evidence/DISPATCH-2026-10-29/production-receipt.json) binder opptak, bevaringskontroller og privat råkjøring.

## Faktisk produksjon og bevaring

Eksisterende lokal VoiceStudio 0.5.6 / OmniVoice produserte bare disse to nye takes med uendret rolle `DISPATCH` / profil `caliche_dispatch` og eksakt gjeldende manus fra [HorrorStory](../../../unreal/Narrative/HorrorStory/modules/unified_scenarios.json). Rene masters er 48 kHz PCM16 mono; telefonfilteret legges på av spillet.

| Cue | Lengde / frames | Seed | WAV SHA-256 |
| --- | --- | --- | --- |
| `SC01_FIRST_REPORT_001` | 10,02 s / 480 960 | 49 | `8197162fafc407613415f43e75bf7146983f1469d5a705e7c2376039be54ed4c` |
| `SC01_FIRST_REPORT_002` | 10,86 s / 521 280 | 44 | `96370b07834e7b656cfc105a0fde36c950d867ee529f96610ae8be94a7e1b535` |

[Verifikasjonen](../../../unreal/Narrative/HorrorStory/voice/verification.json) har PASS for PCM, hash, nivå, kanter og taletider. Fersk ASR har null normaliserte ordavvik for alle fem eksisterende SC01-opptak; dette er maskinell tekstkontroll, ikke menneskelig vurdering av prestasjon. Første nye dispatch-take avviste seeds 46 til 48 på «Sorrow» og bestod med 49. Andre take bestod med 44. Spillteksten er ikke erstattet av ASR-teksten.

De tre tidligere Dale-masterne `H01_BRIEF_001/002/003` og deres genereringsmetadata er byteidentiske. Andre RAW-banker og alle gamle råopptak er bevart og kontrollert. Originaldatabasen ble åpnet med `mode=ro`; tjenesten skrev til en isolert kopi. Kontrollen binder de faktiske pinned profilfeltene og referansefilene, ikke en fersk hash av hele originaldatabasen før og etter. Egen tjeneste og etterkommere er stoppet; kvitteringen viser ingen egne overlevende prosesser eller lytter på port 3900. Privat rågrunnlag er `~/scratch-bm/Signal47-dispatch-report-20261029/run-20261029T051116Z`, med `result.json` PASS.

## Kilde, bygg og pakket lydaksept

Samordnet kilde `fa0e4120` inkluderer main/ENV02, produksjonsendringen `810bc839`, opt-in-testen `dd5cd23c` og presiseringen av minne-/diskkvitteringen `878493ee`. Kildekontrollen består 36 pack-tester og 13 tekstkontrakttester. Editorbygg R1 består: motorloggen oppgir 112,58 s. Bygget beviser kompilering, ikke avspilling.

Pakket R1 fra `c5d05fa2e998b7152737e228fb3503972c059178` inkluderer main/motell gjennom `d771cf9e`. [Pakkekvitteringen](../../../unreal/Evidence/DISPATCH-2026-10-29/packed-r1-receipt.json) binder 13 463 uendrede runtime-inputfiler / 8 378 391 412 logiske byte og åtte payloads. Binæren er 314 253 568 byte, SHA-256 `e557f4e1503a5cad993f8565dc68043ee0f5c65b57e0281a7bd2e9d8cbec6aaa`.

Hele `-S47HorrorContract=SC01 -S47DispatchRecordings` er faktisk kjørt med isolert profil `/tmp/S47-dispatch-packed-sc01-r1`. Motorloggen har 1 709 beståtte selvtester og sluttmarkøren `Horror SC01 PASS COMPLETE`: 220 kontroller, 21 disk/Flow-restores og 273,156790 m normal Walker/Resolve. Normal exit 0, ingen registrert tungjobbkonkurranse eller forbudte markører. [Det faktiske SC01-resultatet](../../../unreal/Evidence/DISPATCH-2026-10-29/sc01-result.json) bevarer prøvene fra den fulle ruten. [Bevisoversikten](../../../unreal/Evidence/DISPATCH-2026-10-29/README.md) har to originale, åpnete telefonbilder. Pending-bildet fanger oppstartsrammen med tom tekstboks og viser telefonkontekst; det beviser ikke lesbar replikktekst. Lydaksepten bygger på PCM-/grafkvitteringene.

Begge eksakte rolle-/tekst-/cue-ID-er dekodes som faktisk mono 48 kHz PCM og fullføres naturlig gjennom produksjonens telefondialog med mottakerfilter. Postvalideringen binder runtime-målingenes SHA-1 til de nye WAV-enes SHA-256 og henholdsvis 480 960 / 521 280 dekodede frames. De faktiske sluttmålingene er:

| Cue | Kilde-/rutede samples | Kilde-peak / rutet peak | Bus-gain / naturlig slutt |
| --- | --- | --- | --- |
| `SC01_FIRST_REPORT_001` | 481 280 / 481 280 | 0,610016 / 0,747779 | 1 / ja |
| `SC01_FIRST_REPORT_002` | 522 240 / 522 240 | 0,699799 / 0,820787 | 1 / ja |

Kilde-/rutet RMS er også positiv; master-gain er 0,8 og SFX-gain 1. Grafens sampletellere er målt prosessert lyd og er ikke de dekodede WAV-frametallene. Faktisk skip etter 0,426667 s og mute av neste svar er prøvd isolert. Den ventende samtalen gjenopprettes gjennom disk/full Flow etter hver kontroll før begge svar spilles fullt. Skippet/mutet tekst gir ingen for tidlig rapportkvittering.

Ruten er API-styrt med én innledende romplassering og 35 uttrykkelig syntetiske filmkvoteposter. Deretter brukes normal Walker/Resolve, faktiske siktstråler/produksjonsinteraksjoner og fokusert Slate Enter; referansefotoet er en faktisk ny renderfangst. Dette er ikke menneskelig input, fysisk gamepad eller helnatt-/helhistorieaksept.

`DispatchComplete` er et sett i testdriverens minne som husker de to fullstendige lydmålingene under samme testkjøring. Settet serialiseres ikke av spillets lagring. Disk/full Flow kontrollerer separat samlet rapportstatus, nøyaktig én `HR_INITIAL_REPORT`, fullført SC01 og opprinnelig `daleReferenceHeard`-verdi. De faktisk lagrede før-/etter-snapshottene viser `reportAcknowledged` false→true, `callCursor` 0→2 og `callActive` true→false, mens `daleReferenceHeard` er true→true. Et bevart testsett er ikke bevis på lagret lydfullføring.

I denne kjøringen er `dale_reference_heard_before=true` ved skipkontrollen, etter den ordinære tidligere Dale-hendelsen. Prøven bevarer **true** gjennom dispatch, skip, mute og disk/full Flow. Den beviser ikke fravær av en feilaktig false→true-overgang i en egen uhørt sak; en slik case er ikke kjørt. Rapport- og Dale-status må derfor ikke slås sammen til en ny påstand om hørt Dale-lyd.

## Omfang og kildegrunnlag

[Stemme-README](../../../unreal/Narrative/HorrorStory/voice/README.md), [genereringsmetadata](../../../unreal/Narrative/HorrorStory/voice/generation.json) og [proveniens](../../../unreal/Narrative/HorrorStory/voice/provenance.json) beskriver den eksisterende løypa. Ingen ny casting, modellnedlasting eller betalt tjeneste inngår. Profilreferanser, kildeopptak og modellvekter redistribueres ikke. Eksisterende lisensgrunnlag beholdes, inkludert modellkortets CC-BY-NC-angivelse; dette arbeidet gir ingen ny kommersiell klarering.

STORY-04 gjelder nøyaktig de to nye SC01-dispatch-takene og føringen om å unngå tidlige alienmøter. Disse to er levert som målrettet kandidat; Leveransen er samlet i [PR221](https://github.com/Tombonator3000/SIGNAL-47/pull/221) mot main. Andre HorrorStory-scener og deres eventuelle uproduserte cues ligger utenfor dette punktet. Ingen ny casting eller flere cues inngår.

Menneskelig lytting, stemmelikhet, rolleprestasjon, FPS og fullmatrise er **UNVERIFIED**. Ingen installasjon eller offentlig release er utført. Privat postvalidering er `native-acceptance-r1.json` fra `final-native-acceptance-r1.py`; faktisk runtime-resultat ligger i `/tmp/S47-dispatch-packed-sc01-r1/Saved/HorrorContract/SC01/result.json`, SHA-256 `8b1d3ab6f67f0afdfa038c1159882eaa7b23e563667442005827e4bc581eca7e`.
