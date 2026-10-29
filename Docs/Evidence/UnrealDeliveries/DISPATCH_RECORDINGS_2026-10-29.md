# SC01: to fornyede dispatch-opptak

**Status:** Under arbeid, lokal lydproduksjon kontrollert; pakket runtimeprøve venter. **Gjelder:** Bare `SC01_FIRST_REPORT_001/002`, ikke hele STORY-04. **Sist kontrollert:** 29. oktober 2026.

To nye syntetiske opptak følger det gjeldende SC01-manuset om slaget mot kontrollromsvinduet. Lokal produksjon, RAW-import, teknisk lydkontroll og fersk ASR består. Ingen native runtimeaksept eller menneskelig lytting er dokumentert ennå. [Maskinlesbar produksjonskvittering](../../../unreal/Evidence/DISPATCH-2026-10-29/production-receipt.json) binder opptak, bevaringskontroller og privat råkjøring.

## Faktisk produksjon og bevaring

Eksisterende lokal VoiceStudio 0.5.6 / OmniVoice produserte bare disse to nye takes med uendret rolle `DISPATCH` / profil `caliche_dispatch` og eksakt gjeldende manus fra [HorrorStory](../../../unreal/Narrative/HorrorStory/modules/unified_scenarios.json). Rene masters er 48 kHz PCM16 mono; telefonfilteret legges på av spillet.

| Cue | Lengde / frames | Seed | WAV SHA-256 |
| --- | --- | --- | --- |
| `SC01_FIRST_REPORT_001` | 10,02 s / 480 960 | 49 | `8197162fafc407613415f43e75bf7146983f1469d5a705e7c2376039be54ed4c` |
| `SC01_FIRST_REPORT_002` | 10,86 s / 521 280 | 44 | `96370b07834e7b656cfc105a0fde36c950d867ee529f96610ae8be94a7e1b535` |

[Verifikasjonen](../../../unreal/Narrative/HorrorStory/voice/verification.json) har PASS for PCM, hash, nivå, kanter og taletider. Fersk ASR har null normaliserte ordavvik for alle fem eksisterende SC01-opptak; dette er maskinell tekstkontroll, ikke menneskelig vurdering av prestasjon. Første nye dispatch-take avviste seeds 46 til 48 på «Sorrow» og bestod med 49. Andre take bestod med 44. Spillteksten er ikke erstattet av ASR-teksten.

De tre tidligere Dale-masterne `H01_BRIEF_001/002/003` og deres genereringsmetadata er byteidentiske. Andre RAW-banker, originale profildata/-referanser og alle gamle råopptak er bevart og kontrollert. Produksjonen brukte en isolert databasekopi med eksisterende referanser. Egen tjeneste og etterkommere er stoppet; kvitteringen viser ingen egne overlevende prosesser eller lytter på port 3900. Privat rågrunnlag er `~/scratch-bm/Signal47-dispatch-report-20261029/run-20261029T051116Z`, med `result.json` PASS.

## Kilde, bygg og gjenstående lydaksept

Samordnet kilde `fa0e4120` inkluderer main/ENV02, produksjonsendringen `810bc839`, opt-in-testen `dd5cd23c` og presiseringen av minne-/diskkvitteringen `878493ee`. Kildekontrollen består 36 pack-tester og 13 tekstkontrakttester. Editorbygg R1 består: motorloggen oppgir 112,58 s. Bygget beviser kompilering, ikke avspilling.

Neste kontroll er hele ordinære SC01-ruten i pakket kandidat med `-S47HorrorContract=SC01 -S47DispatchRecordings` og eksplisitt isolert `/tmp`-profil, etter motell-leveringen. Testen krever eksakt rolle/tekst/ID, faktisk dekodet PCM og naturlig full avspilling av begge nye takes gjennom produksjonens telefondialog, med telefonfilter, kilde-/rutede samples, peak/RMS og positiv bus-gain. Skip-/mute-kontroller gjenoppretter den faktiske ventende samtalen før den fulle samtalen gjennomføres; SC01-ruten og ordinære disk/full Flow-kontroller beholdes. **Dette er testkrav, fortsatt UNVERIFIED i runtime.**

`DispatchComplete` er et sett i testdriverens minne som husker at de to fullstendige lydmålingene er sett under samme testkjøring. Settet serialiseres ikke av spillets lagring. Disk/full Flow skal separat bevare samlet rapportstatus, nøyaktig én `HR_INITIAL_REPORT`, fullført SC01 og opprinnelig `daleReferenceHeard`-verdi. Et bevart testsett er dermed ikke bevis på lagret lydfullføring. Dispatch-opptakene, tekst, skip eller mute skal ikke skape noe nytt Dale-hørt-flagg.

## Omfang og kildegrunnlag

[Stemme-README](../../../unreal/Narrative/HorrorStory/voice/README.md), [genereringsmetadata](../../../unreal/Narrative/HorrorStory/voice/generation.json) og [proveniens](../../../unreal/Narrative/HorrorStory/voice/provenance.json) beskriver den eksisterende løypa. Ingen ny casting, modellnedlasting eller betalt tjeneste inngår. Profilreferanser, kildeopptak og modellvekter redistribueres ikke. Eksisterende lisensgrunnlag beholdes, inkludert modellkortets CC-BY-NC-angivelse; dette arbeidet gir ingen ny kommersiell klarering.

Menneskelig lytting, stemmelikhet og rolleprestasjon er **UNVERIFIED**. Store deler av STORY-04 har fortsatt uproduserte cues; denne leveransen gjelder bare de to overtatte SC01-dispatch-takene. Ingen fullmatrise, installasjon eller offentlig release er utført.
